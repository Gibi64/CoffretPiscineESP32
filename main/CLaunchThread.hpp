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
class CKernelLaunchThread
{
protected:
    std::atomic<int> m_State = THREAD_RUNNING;

#if defined(_WIN32) || defined(__linux__)
    std::thread m_thread;
#elif defined(_ESP32)
    TaskHandle_t m_taskHandle = nullptr;
#endif

    CLaPoste* m_pLaPoste = nullptr;
    CKernelLaunchThread* m_pParent = nullptr;

public:
    CKernelLaunchThread(CLaPoste* pLaPoste, CKernelLaunchThread* pParent)
        : m_pLaPoste(pLaPoste), m_pParent(pParent)
    {
        if (m_pLaPoste)
            m_pLaPoste->RegisterThread(this);

        SetState(THREAD_RUNNING);
    }

    virtual ~CKernelLaunchThread() {}

    int GetState() const { return m_State; }
    void SetState(int state) { m_State = state; }

    void Start()
    {
#if defined(_WIN32) || defined(__linux__)
        m_thread = std::thread(ThreadEntry, this);
        m_thread.detach();
#elif defined(_ESP32)
        xTaskCreate(ThreadEntry, "Thread", 16384, this, 5, &m_taskHandle);
#endif
    }


    void ThreadLoop()
    {
        while (GetState() == THREAD_RUNNING)
        {
            Function();
            CTimeUtils::CPUSleep(10);
        }

    }

    // Les 3 fonctions virtuelles métier
    virtual void Function() = 0;
    virtual void HandleMessage(CLaPoste::sMessage msg) = 0;
    virtual void Cleanup() = 0;

    CKernelLaunchThread* GetParent() const { return m_pParent; }

    void SendMessage(CKernelLaunchThread* pDest, int Command, void* pParam = nullptr)
    {
        if (!m_pLaPoste)
        {
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
        if (!m_pLaPoste)
        {
            return false;
		}
        return m_pLaPoste->GetMessage(this, pOut);
    }

    void SendToKernel(int Command, void* pParam)
    {
        if (!m_pLaPoste)
        {
            return;
        }
        m_pLaPoste->SendMessage(nullptr, Command, pParam);
    }
    static void ThreadEntry(void* arg)
    {
		auto* pThis = static_cast<CKernelLaunchThread*>(arg);
		if (pThis == nullptr) {
			write_log("Argument de ThreadEntry nullptr");
			return;
		}

        pThis->SetState(THREAD_RUNNING);
        //write_log("Start thread  at adress : " + std::to_string((uintptr_t)pThis) +" State set to RUNNING");
        pThis->ThreadLoop();
        if (pThis->GetLaPoste())
            pThis->Cleanup();
        //write_log("Cleanup DONE for " + std::to_string((uintptr_t)pThis));
		#if defined(_ESP32)
			vTaskDelete(NULL);     // OBLIGATOIRE
		#endif
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
        write_log("Nouveau Timer Temps restant " + CTimeUtils::FormatDeltaTime(ms) + " ms restantes");
        #else
		write_log("Nouveau Timer Temps restant " + CTimeUtils::FormatDeltaTime(ms) + " ID : " + std::to_string(GetCurrentThreadId()));
        #endif
        auto t = CTimeUtils::SystemDateTime(CTimeUtils::GetMs());
        t = CTimeUtils::LocalTimeFromUTC(t);
        #ifdef _ESP32
        write_log("Timer started at " + std::to_string(t.hour) + ":" + std::to_string(t.minute) + ":" + std::to_string(t.second) + "." + std::to_string(t.millisecond));
        #else
        write_log("Timer started at " + std::to_string(t.hour) + ":" + std::to_string(t.minute) + ":" + std::to_string(t.second) + "." + std::to_string(t.millisecond) + " ID : " + std::to_string(GetCurrentThreadId()));
        #endif
		Start();
    }
    long long m_Remaining_ms;

    virtual void Function() 
    {

        if (m_Remaining_ms <= 0) SetState(THREAD_ENDED);
        CTimeUtils::CPUSleep(1);
        m_Remaining_ms = (Ending_Time - CTimeUtils::GetMs());
    }
    virtual void HandleMessage(CLaPoste::sMessage msg) override
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
    virtual void Cleanup() override
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

class CSimpleLaunchThread : public CKernelLaunchThread
{
	// La fonction métier est toujours virtuelle et doit être surchargée par la classe fille
	// les 2 autres fonctions virtuelles ne font rien par défaut, mais peuvent être surchargées si nécessaire
public:
	// 
	// Le constructeur n'a pas besoin de laPoste , car cette classe ne gère pas de messages
public:
    CSimpleLaunchThread()
        : CKernelLaunchThread(nullptr, nullptr)   // ← clé du modèle
    {
    }

    void Function() override {}
    void HandleMessage(CLaPoste::sMessage) override {}
    void Cleanup() override {}
};