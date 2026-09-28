#pragma once
#if defined(_WIN32)
#include <winsock2.h>
#include <Windows.h>
#endif

#if defined(_WIN32) || defined(__linux__)
#include <thread>
#elif defined(_ESP32)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif
#include "CTimeUtils.hpp"
#include "InitLog.hpp"
#include <memory>
#include <unordered_set>
#include <mutex>
#include <vector>
#include <thread>
/////////////////////////////////// Define pour le Kernel LaPoste ////////////////////////////////
#define THREAD_ENDED   1
#define CLOSE_THREAD   2
#define THREAD_RUNNING 3
#define THREAD_ABORTED 4
#define THREAD_DESTROYED 5

class CLaunchThread
{
public:
    // La gestion des etats se fait essentiellement dans la loop:
	// - Running: la loop au  demarage de la thread
	// - AskToStop: Demande externe qui demande l'arret de la thread, la loop doit detecter cet etat et arreter la thread
	// - Stopped: renvoyé par la loop apres le debranchement : assure que la thread est bien arretée et que la loop a fini son execution
	// - le demandeur de l'arret de la thread doit attendre que la loop ait fini son execution avant de continuer
	// et eventuellement de detruire l'objet CLaunchThread
    CLaunchThread()
    {
    }
protected:
	// should be m-state mybe atomic variable to ensure thread safety
    std::atomic<int> m_State = THREAD_RUNNING;

#if defined(_WIN32) || defined(__linux__)
    std::thread m_thread;
#elif defined(_ESP32)
    TaskHandle_t m_taskHandle = nullptr;
    std::thread m_thread;   // For ESP32, we still keep a std::thread object for compatibility
#endif

public:
    int GetState() const
    {
        return m_State;
	}
    void SetState(int state)
    {
        m_State = state;
	}
    ~CLaunchThread()
    {
        if (GetThread()->joinable())
            GetThread()->join();
    }
    
    std::thread* GetThread()
    {
        return &m_thread;
    }

    CLaunchThread(void (*function)(void*), void* arg)
    {
       SetState(THREAD_RUNNING);

#if defined(_WIN32) || defined(__linux__)
        m_thread = std::thread(function,arg);
        m_thread.detach();

#elif defined(_ESP32)
xTaskCreatePinnedToCore(
        function,
        "loop",
        4096,
        arg,
        5,
        &m_taskHandle,
        1   // CPU1 = APP_CPU
    );
#endif
    }
};
class CKernelLaunchThread;
class CLaPoste
{
    std::mutex mtx;
public:
    struct sMessage
    {
        CKernelLaunchThread* pReceiver;   // nullptr => Kernel
        int Command;
        void* pParam;
    };
private:
    std::unordered_set<CKernelLaunchThread*> m_Threads;
    std::vector<sMessage> m_Messages;   // uniquement pour les threads

public:
    std::unordered_set<CKernelLaunchThread*>* GetThreadSet()
    {
        return &m_Threads;
    }
    // --- Enregistrement / désenregistrement ---
    void RegisterThread(CKernelLaunchThread* pThread);

    void UnregisterThread(CKernelLaunchThread* pThread);
    // --- Envoi de message ---
    void SendMessage(CKernelLaunchThread* pReceiver, int Command, void* pParam);

    // --- Lecture d’un message pour une thread ---
    bool GetMessage(CKernelLaunchThread* pThread, sMessage* pOut);

private:

    // --- Traitement immédiat des messages Kernel ---
    void HandleKernelMessage(int Command, void* pParam);

    // --- Destruction immédiate d’une thread ---
    void CloseThread(CKernelLaunchThread* pThread);
};
class CKernelLaunchThread : public CLaunchThread
{
    // class de message avec le Kernel LaPoste
    // 
private:
    CLaPoste* m_pLaPoste=nullptr;
    CKernelLaunchThread* m_pParent = nullptr;
public:
    CKernelLaunchThread(CLaPoste* pLaPoste, CKernelLaunchThread* pParent): m_pLaPoste(pLaPoste),m_pParent(pParent),CLaunchThread(ThreadEntry,this)
    {
        if (pLaPoste == nullptr) return;
        m_pLaPoste = pLaPoste;
        m_pParent = pParent;

        // Enregistrement de la thread dans LaPoste
        m_pLaPoste->RegisterThread(this);
        // Creation de la thread
        // 
        //write_log("Nouvelle KernelLaunchThread creee a l'adresse : " + std::to_string((uintptr_t)this) + " Thread " + std::to_string(GetCurrentThreadId()));
        // Démarrage de la thread
        SetState(THREAD_RUNNING);
    }
    virtual void Function() = 0; // Fonction principale de la thread à implémenter dans les classes dérivées
    // C'est cette fonction qui gere les arguments
    // Son constructeur recoit les arguments avec une structure métier

    virtual void HandleMessage(CLaPoste::sMessage msg) = 0; // gestion des messages reçus par la thread à implémenter dans les classes dérivées
    virtual void Cleanup() = 0; // Nettoyage avant la fin de la thread
    CKernelLaunchThread* GetParent() const
    {
        return m_pParent;
    }
    void SendMessage(CKernelLaunchThread* pDest, int Command, void* pParam = nullptr)
    {
        if (!m_pLaPoste)
        {
            write_log("CLaPoste is nullptr, cannot send message.");
            return;
        }
        m_pLaPoste->SendMessage(pDest, Command, pParam);


    }
    CLaPoste* GetLaPoste() const
    {
        return m_pLaPoste;
    }
    virtual bool GetMessage(CLaPoste::sMessage* pOut)
    {
        return m_pLaPoste->GetMessage(this, pOut);
    }

    void SendToKernel(int Command, void* pParam)
    {
        m_pLaPoste->SendMessage(nullptr, Command, pParam);
    }
    static void ThreadEntry(void* arg)
    {
        CKernelLaunchThread* pThis = (CKernelLaunchThread*)arg;
        pThis->SetState(THREAD_RUNNING);
        //write_log("Start thread  at adress : " + std::to_string((uintptr_t)pThis) +" State set to RUNNING");
        pThis->ThreadLoop();
        pThis->Cleanup();
        //write_log("Cleanup DONE for " + std::to_string((uintptr_t)pThis));

    }
    void ThreadLoop()
    {
        // Boucle principale de la thread
        while (GetState() == THREAD_RUNNING)
        {
            // Traitement principal de la thread ici
            // ...
            // Vérification des messages
            Function();
            CLaPoste::sMessage msg;
            while (GetMessage(&msg))
            {
                // Traitement du message reçu
                // C'est forcément un CLOSE_THREAD
                HandleMessage(msg);
                if (GetState() != THREAD_RUNNING) break; // Si la thread a été arrêtée, on sort de la boucle
            }
            // Pause ou attente pour éviter une boucle trop rapide
            CTimeUtils::CPUSleep(10);
        }
        // Nettoyage avant la fin de la thread
        //write_log("ThreadLoop FINISHED for " + std::to_string((uintptr_t)this));
    }
};



class CTimerThread : public CKernelLaunchThread
{
private:
    time_t Starting_Time;
    time_t Ending_Time;
public:
    CTimerThread(CLaPoste* poste, CKernelLaunchThread* parent, long long ms) : CKernelLaunchThread(poste, parent), m_Remaining_ms(ms)
    {
        // La thread est créée par la classe mère
        Starting_Time = CTimeUtils::GetMs();
        Ending_Time = Starting_Time + ms;
        #ifdef _ESP32
        write_log("Nouveau Timer avec " + std::to_string(ms) + " ms restantes");
        #else
		write_log("Nouveau Timer avec " + std::to_string(ms) + " ms restantes ID : " + std::to_string(GetCurrentThreadId()));
        #endif
        auto t = CTimeUtils::SystemDateTime(CTimeUtils::GetMs());
        #ifdef _ESP32
        write_log("Timer started at " + std::to_string(t.hour) + ":" + std::to_string(t.minute) + ":" + std::to_string(t.second) + "." + std::to_string(t.millisecond));
        #else
        write_log("Timer started at " + std::to_string(t.hour) + ":" + std::to_string(t.minute) + ":" + std::to_string(t.second) + "." + std::to_string(t.millisecond) + " ID : " + std::to_string(GetCurrentThreadId()));
        #endif

    }
    long long m_Remaining_ms;

    virtual void Function()
    {

        if (m_Remaining_ms <= 0) SetState(THREAD_ENDED);
        CTimeUtils::CPUSleep(1);
        m_Remaining_ms = (Ending_Time - CTimeUtils::GetMs());
    }
    virtual void HandleMessage(CLaPoste::sMessage msg)
    {
		switch (msg.Command)
        {
            case CLOSE_THREAD:
                SetState(THREAD_ABORTED);
				break;
        default:
            break;
        }
    }
    virtual void Cleanup()
    {
        switch (GetState())
        {
            case THREAD_ABORTED:
                write_log("Timer aborted at address: " + std::to_string((uintptr_t)this));
                SendMessage(nullptr, THREAD_ENDED, static_cast<CKernelLaunchThread*>(this)); // Unregister la thread
                break;
            case THREAD_ENDED:
            {
                auto t = CTimeUtils::SystemDateTime(CTimeUtils::GetMs());
                #ifdef _ESP32
                write_log("Timer ended at " + std::to_string(t.hour) + ":" + std::to_string(t.minute) + ":" + std::to_string(t.second) + "." + std::to_string(t.millisecond));
                #else
                write_log("Timer ended at " + std::to_string(t.hour) + ":" + std::to_string(t.minute) + ":" + std::to_string(t.second) + "." + std::to_string(t.millisecond) + " ID : " + std::to_string(GetCurrentThreadId()));
                #endif
                if (GetParent())
                    SendMessage(GetParent(), THREAD_ENDED, static_cast<CKernelLaunchThread*>(this)); // info au parent : ton timer est terminé (sorti de la boucle
                // on ne tue pas la thread ici, c'est le Kernel qui le fera
                SendMessage(nullptr, THREAD_ENDED, static_cast<CKernelLaunchThread*>(this)); // tue la thread
                break;
            }
            default:
                break;
        }
    }
};