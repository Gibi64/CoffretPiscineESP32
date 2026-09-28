#include "CoffretPiscine.h"
#include "CTCP_Modbus.h"

//////////////////////////////////////////// pour le serveur //////////////////////////////////////
#ifdef _WIN32

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <mutex>
#pragma comment(lib, "ws2_32.lib")

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <mutex>
#endif
#include "CTimeUtils.hpp"
#include "CProtocoleModbusRJE.h"
class CServerCoffertPiscine
{
private:
	struct SServerArgs
	{
		CServerCoffertPiscine* pServer;
		CCoffretPiscine* pCoffret;
		CTCPLib* pTCPLib;
	};
	CCoffretPiscine* m_pCoffret;
	CTCPLib* m_pTCPLib;
	CLaunchThread* m_pServerThread;
	SServerArgs m_Args;
public:
	CServerCoffertPiscine(CCoffretPiscine* pCoffret, CTCPLib* pTCPLib)
		: m_pCoffret(pCoffret), m_pTCPLib(pTCPLib)
	{
		StartServer();
	}
	void StartServer()
	{
		m_Args = { this,m_pCoffret,m_pTCPLib };
		m_pServerThread = new CLaunchThread(ServerThreadFunction,(void*)&m_Args);
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	 bool CheckIfCompleted(const std::string& receivedData, size_t& offsetOfCompletion)
	{
		///////////// dispatching suivant GET ou POST
		// 
		if (receivedData.find("GET") == 0)
		{
			// For GET requests, we consider the request complete when we find the end of the headers
			size_t endOfHeadersPos = receivedData.find("\r\n\r\n");
			if (endOfHeadersPos != std::string::npos)
			{
				offsetOfCompletion = endOfHeadersPos + 4; // Move past the end of headers
				return true; // Request is complete
			}
		}
		else if (receivedData.find("POST") == 0)
		{
			// For POST requests, we consider the request complete when we find the end of the XML data
			size_t endPos = receivedData.find("</config>");
			if (endPos != std::string::npos)
			{
				offsetOfCompletion = endPos + strlen("</config>");
				return true; // XML is complete
			}
		}
		else
		{
			// le type n'est pas encore recu (bizarre mais possible)
			return false; // Request is not complete yet
		}
		return false; // Request is not complete yet
	}
	 void DispatchProcessAnswer(CCoffretPiscine* pCoffret,CTCPLib *ptrLib, std::string receivedData, int ClientSocket)
	 {
		 if (receivedData.find("GET") == 0)
		 {
			 // Handle GET request
			 write_log("Received GET request");
			// On renvoie un json du  relais pompe et des events 
			 std::string szJson = "{\"mode\":" + std::to_string(pCoffret->GetMode()) + ",";

			 // --- RELAIS ---
			 szJson += "\"relais\": [";

			 bool firstRelais = true;
			 for (auto& Entry : *pCoffret->GetVectorEntry())
			 {
				 if (!firstRelais) szJson += ",";
				 firstRelais = false;

				 szJson += "{";
				 szJson += "\"name\": \"" + Entry.Description + "\",";
				 szJson += "\"ip\": \"" + Entry.IPAdress + "\",";
				 szJson += "\"register\": " + std::to_string(Entry.RegisterOn);
				 szJson += "}";
				 break;// pour l'instant on envoie que la pompe
			 }

			 szJson += "],";

			 // --- EVENTS ---
			 szJson += "\"events\": [";

			 bool firstEvent = true;
			 for (auto& Event : *pCoffret->GetVectorEvents())
			 {
				 CTimeUtils::sUTCTime CorrectGenericStartToLocalTime = Event->CorrectGenericStartToLocalTime();
				 if (!firstEvent) szJson += ",";
				 firstEvent = false;

				 szJson += "{";
				 szJson += "\"action\": \"" + Event->GetActionId() + "\",";
				 szJson += "\"heure\": " + std::to_string(CorrectGenericStartToLocalTime.hour) + ",";
				 szJson += "\"minute\": " + std::to_string(Event->GetStartMinute()) + ",";
				 szJson += "\"dureeHeure\": " + std::to_string(Event->GetDurationHour()) + ",";
				 szJson += "\"dureeMinute\": " + std::to_string(Event->GetDurationMinute()) + ",";
				 szJson += "\"freqJour\": " + std::to_string(Event->GetDeltaTimeDay()) + ",";
				 szJson += "\"freqHeure\": " + std::to_string(Event->GetDeltaTimeHour()) + ",";
				 szJson += "\"freqMinute\": " + std::to_string(Event->GetDeltaTimeMinute());
				 szJson += "}";
			 }

			 szJson += "]";
			 szJson += "}";
			 // --- HEADER HTTP ---
			 std::string header =
				 "HTTP/1.1 200 OK\r\n"
				 "Content-Type: application/json\r\n"
				 "Access-Control-Allow-Origin: *\r\n"
				 "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
				 "Access-Control-Allow-Headers: Content-Type\r\n"
				 "Content-Length: " + std::to_string(szJson.size()) + "\r\n"
				 "Connection: close\r\n"
				 "\r\n";
			 // --- ENVOI ---
			 send(ClientSocket, header.c_str(), header.size(), 0);
			 send(ClientSocket, szJson.c_str(), szJson.size(), 0);
			 closesocket(ClientSocket);
		 }
		 else if (receivedData.find("POST") == 0)
		 {
			 write_log("Received POST request");

			 // Handle POST request
						// Process the received data (e.g., save to a file, parse XML, etc.)
			// Close the client socket
			 size_t iPos = receivedData.find("<config>");
			 if (iPos != std::string::npos)
			 {
				 receivedData = receivedData.substr(iPos);
			 }
			 std::string szOk = "HTTP/1.1 200 OK\r\n\r\n";
			 ::send(ClientSocket, szOk.c_str(), szOk.length(), 0);
			 closesocket(ClientSocket);
			 //////////////////////////// On arrête les threads en cours  /////////////////////////////
			 for (auto& prog : *pCoffret->GetVectorEvents())
			 {
				 pCoffret->GetLaPoste()->SendMessage(static_cast<CKernelLaunchThread*>(prog), CLOSE_THREAD, nullptr);
			 }
			 // On attend que les threads se terminent
			 while (!pCoffret->GetLaPoste()->GetThreadSet()->empty())
			 {
					 CTimeUtils::CPUSleep(2);
			 }
			 ////////////////////////////// On vide le vecteur des events  /////////////////////////////
			 pCoffret->GetVectorEvents()->clear();
			 //////////////////////////// On supprime les pointeurs de CProtocoleModbusRJE  /////////////////////////////
			 for (auto& entry : pCoffret->m_map_IP)
			 {
				auto a = entry.second;
				 delete a;
			 }
			 pCoffret->m_map_IP.clear();
			 ///////////////////////////// On ecrit le fichier de config  /////////////////////////////
#if defined(_WINDOWS)

			 pCoffret->WriteConfigFile("c:\\Local\\Softwares\\CoffretPiscine\\config\\config.xml", receivedData);
#elif defined(_ESP32)
			 pCoffret->WriteConfigFile("/spiffs/config.xml", receivedData);
#endif
			 ///////////////////// On relance la lecture du fichier de config  /////////////////////////////
#if defined(_WINDOWS)
			 pCoffret->ReadConfigFile("c:\\Local\\Softwares\\CoffretPiscine\\config\\config.xml", ptrLib);
#elif defined(_ESP32)
			 pCoffret->ReadConfigFile("/spiffs/config.xml", ptrLib);
#endif

			 pCoffret->DoModeAction();

		 }
		 else
		{
			 write_log("Received unknown request type");
			 // On renvoie l'erreur 404 sur le socket client et on ferme le socket
			 std::string szError = "HTTP/1.1 404 Not Found\r\n\r\n";
			 ::send(ClientSocket, szError.c_str(), szError.length(), 0);
			 closesocket(ClientSocket);
		 }


	}
	static void ServerThreadFunction(void* Args)
	{
		SServerArgs* args = static_cast<SServerArgs*>(Args);

		auto pServer = args->pServer;
		CCoffretPiscine* pCoffret = args->pCoffret;
		CTCPLib* ptrLib = args->pTCPLib;

		struct sockaddr_in addr;
		unsigned long long sock;
		int ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (ListenSocket < 0)
		{
			write_log("Socket creation failed");
			return;
		}
		addr.sin_family = AF_INET;
		addr.sin_port = htons(27);

		// Windows + Linux
#if defined(_WIN32) || defined(__linux__)
		inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr);
#endif

		// ESP32
#if defined(ESP_PLATFORM)
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif

		if (bind(ListenSocket, (sockaddr*)&addr, sizeof(addr)) < 0)
		{
			write_log("Bind failed");
			return;
		}
		if (listen(ListenSocket, 5) < 0)
		{
			write_log("Listen failed");
			return;
		}
		for (;;)
		{
			// Accept a new connection
			sockaddr_in clientAddr{};
			socklen_t clientAddrLen = sizeof(clientAddr);
			int clientSock = accept(ListenSocket, (sockaddr*)&clientAddr, &clientAddrLen);
			if (clientSock < 0)
			{
				write_log("Accept failed");
				continue;
			}
			// Handle the client connection in a separate thread or function
			// Receive data from the client and process it as needed
			char buffer[1024];
			// Loop until the xml is fully received
			std::string receivedData;
			size_t offsetOfCompletion = 0;
			bool bCompleted = false;
			while (!bCompleted)
			{
				int bytesRead = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
				if (bytesRead > 0)
				{
					buffer[bytesRead] = '\0'; // Null-terminate the received data
					receivedData += buffer;
					bCompleted = pServer->CheckIfCompleted(receivedData, offsetOfCompletion);
				}
				else if (bytesRead == 0)
				{
					write_log("Client disconnected");
					break;
				}
				else
				{
					write_log("Receive failed");
					break;
				}
			}
			pServer->DispatchProcessAnswer(pCoffret, ptrLib, receivedData, clientSock);
			CTimeUtils::CPUSleep(2);
		}
	}
	~CServerCoffertPiscine()
	{
		if (m_pServerThread)
		{
			delete m_pServerThread;
		}
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(_WINDOWS)
int main()
#elif defined(_ESP32)
extern "C" void app_main(void)
#endif
{
#define ON 1
#define OFF 0
	std::unique_ptr<CTCPLib> pTCPLIB = std::make_unique<CTCPLib>();
#if defined(_WINDOWS)
	std::string szFullPath = "c:\\Local\\Softwares\\CoffretPiscine\\config\\config.xml";
#elif defined(_ESP32)
	std::string szFullPath = "/spiffs/config.xml";
#endif

	std::unique_ptr<CCoffretPiscine> pCoffret = std::make_unique<CCoffretPiscine>();

	pCoffret->ReadConfigFile(szFullPath, pTCPLIB.get());
	pCoffret->DoModeAction();
	CServerCoffertPiscine Server(pCoffret.get(), pTCPLIB.get());


	for (;;)
	{
		CTimeUtils::CPUSleep(2);
	}
	
#if defined(_WINDOWS)
	return 0;
#endif
}
