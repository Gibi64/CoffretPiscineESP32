#include "CLaunchThread.hpp"
// --- Enregistrement / désenregistrement ---

void CLaPoste::RegisterThread(CKernelLaunchThread* pThread)
{
    std::lock_guard<std::mutex> lock(mtx);
    //write_log("RegisterThread: ptr = " + std::to_string((uintptr_t)pThread));
    m_Threads.insert(pThread);
}

void CLaPoste::UnregisterThread(CKernelLaunchThread* pThread)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto it = m_Threads.find(pThread);
    if (it == m_Threads.end())
        write_log("find() did NOT find the pointer");
    m_Threads.erase(pThread);
    // attention il faut retirer tous les messages destinés à cette thread
    m_Messages.erase(std::remove_if(m_Messages.begin(), m_Messages.end(),
        [pThread](const sMessage& msg) { return msg.pReceiver == pThread; }),
        m_Messages.end());
    m_Messages.erase(
        std::remove_if(m_Messages.begin(), m_Messages.end(),
            [pThread](const sMessage& msg) {
                return msg.pReceiver == nullptr && msg.pParam == pThread;
            }),
        m_Messages.end()
    );
    //write_log("Remaining Threads : " + std::to_string(m_Threads.size()) + " - Remaining messages : " + std::to_string(m_Messages.size()));
    // Peut etre egalament les messages pour le Kernel et dont l'emetteur est cette thread (pParam = pThread) mais c'est moins critique
    // m_Messages.erase(std::remove_if(m_Messages.begin(), m_Messages.end(),    
    //     [pThread](const sMessage& msg) { return msg.pReceiver == nullptr && msg.pParam == pThread; }),
    //	 m_Messages.end());
    // On pourrait aussi envisager de traiter les messages destinés à cette thread avant de la supprimer, mais cela dépend de la logique de l'application

}

void CLaPoste::CloseThread(CKernelLaunchThread* pThread)
{
    if (pThread->GetParent())
    {
        SendMessage(pThread->GetParent(), THREAD_DESTROYED, pThread);
    }

    UnregisterThread(pThread);
    write_log("Kernel is deleting thread at address: " + std::to_string((uintptr_t)pThread));
    delete pThread;
}
void CLaPoste::HandleKernelMessage(int Command, void* pParam)
{
    switch (Command)
    {
    case THREAD_ENDED:
    {
        CKernelLaunchThread* pThread = (CKernelLaunchThread*)pParam;
        CloseThread(pThread);
        break;
    }
    }
}
// --- Lecture d’un message pour une thread ---
bool CLaPoste::GetMessage(CKernelLaunchThread* pThread, sMessage* pOut)
{
    std::lock_guard<std::mutex> lock(mtx);
    for (auto it = m_Messages.begin(); it != m_Messages.end(); ++it)
    {
        if (it->pReceiver == pThread)
        {
            *pOut = *it;
            m_Messages.erase(it);
            return true;
        }
    }
    return false;
}

// --- Envoi de message ---
void CLaPoste::SendMessage(CKernelLaunchThread* pReceiver, int Command, void* pParam)
{
    if (pReceiver == nullptr)
    {
        // 1. On traite le Kernel SANS mutex
        HandleKernelMessage(Command, pParam);
        return;
    }

    // 2. On protège uniquement l'accès au vector
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (m_Threads.count(pReceiver))
            m_Messages.push_back({ pReceiver, Command, pParam });
    }
}
