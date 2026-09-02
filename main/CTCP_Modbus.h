#pragma once
#if defined(_WIN32) || defined(_WINDOWS)
#include <WinSock2.h>
#include "ws2tcpip.h"

#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <mutex>
#endif
#include <stdlib.h>
#include <cstring>
#include <map>
#include <vector>
#include <iostream>
#include "initlog.hpp"
#include "CTimeUtils.hpp"
#include<memory>
class CTCPLib
{
private:
	bool m_bInitialized = false;
public:
	CTCPLib()
	{
#if defined(_WIN32) || defined(_WINDOWS)
		WSADATA wsaData;
		auto iRet = WSAStartup(MAKEWORD(2, 0), &wsaData);
		if (iRet)
		{
			WSAGetLastError();
			return;
		}
#endif
		m_bInitialized = true;
	}
	~CTCPLib()
	{
#if defined(_WIN32) || defined(_WINDOWS)
		if (IsInitialized()) WSACleanup();
#endif
	}
	enum
	{
		Success = 0,
		SocketCreationFailed = 1,
		InvalidAddress = 2,
		ConnectionFailed = 3,
		SendFailed = 4,
		ReceiveFailed = 5
	};
	static inline const std::map<int, std::string> ErrorMap=
	{
		{ (int)CTCPLib::SocketCreationFailed, "Socket creation failed" },
		{ (int)CTCPLib::InvalidAddress,       "Invalid IP address" },
		{ (int)CTCPLib::ConnectionFailed,     "Connection failed" },
		{ (int)CTCPLib::SendFailed,           "Send failed" },
		{ (int)CTCPLib::ReceiveFailed,        "Receive failed" }
	};
	bool IsInitialized()
	{
		return m_bInitialized;
	}
};
class CTCP_Modbus
{
private:
	std::string m_szIp;
	int m_Port;
	#if defined(_WIN32) || defined(_WINDOWS)
	SOCKET clientSocket; 

#else
	int clientSocket;
#endif
	bool m_bReceiveCheck; // Check if nReceive is 0 because of no data to receive
public:
	CTCP_Modbus(std::string szAdPort, bool bReceiveCheck=true,CTCPLib* ptcpLib=nullptr)
	{
		m_szIp = szAdPort;
		auto iEnd = m_szIp.find(':');
		m_Port = atoi(m_szIp.substr(iEnd + 1).c_str());
		m_szIp = m_szIp.substr(0, iEnd);
		m_bReceiveCheck = bReceiveCheck;
		if (!ptcpLib ||!ptcpLib->IsInitialized())
		{
			write_log("TCP library not initialized . Create a CTCPLib instance first.\n");
		}
	}
	void MakeError(int iErr)
	{
#if defined(_WIN32) || defined(_WINDOWS)
		int sysErr = WSAGetLastError();
#else
		int sysErr = errno;
#endif

		auto it = CTCPLib::ErrorMap.find(iErr);
		if (it != CTCPLib::ErrorMap.end())
		{
			write_log("TCP error " + std::to_string(iErr) +
				" : " + it->second +
				" system=" + std::to_string(sysErr) + "\n");
		}
		else
		{
			write_log("TCP error " + std::to_string(iErr) +
				" : <no description> system=" + std::to_string(sysErr) + "\n");
		}
	}
	std::string ReceiveFromServer;
	int Connect()
	{
		// Création socket
#if defined(_WIN32) || defined(_WINDOWS)
		static const int ArgIPPROTO = IPPROTO_TCP;
#else
		static const int ArgIPPROTO = 0;
#endif
		clientSocket = socket(AF_INET, SOCK_STREAM, ArgIPPROTO);
		if (clientSocket < 0)
		{
			MakeError(CTCPLib::SocketCreationFailed);
			return 1;
		}

		write_log("socket is OK!\n");

		// Préparation sockaddr
		sockaddr_in clientService;
		clientService.sin_family = AF_INET;

		if (inet_pton(AF_INET, m_szIp.c_str(), &clientService.sin_addr) <= 0)
		{
			MakeError(CTCPLib::InvalidAddress);
			return 1;
		}

		clientService.sin_port = htons(m_Port);

		// Connexion
		if (connect(clientSocket, (struct sockaddr*)&clientService, sizeof(clientService)) < 0)
		{
			MakeError(CTCPLib::ConnectionFailed);
			return 1;
		}

		write_log("Client: Connect() is OK!\n");
		return 0;
	}
	int TransmitTCP(char* szToSend, int len)
	{
		if (Connect() != 0) 
		{
			MakeError(CTCPLib::ConnectionFailed);
			return 1;
		}
		int nSent = send(clientSocket, szToSend, len, 0);

		bool bError =
#if defined(_WIN32)
			(nSent == SOCKET_ERROR);
#else
			(nSent < 0);
#endif

		if (bError) 
		{
			MakeError(CTCPLib::SendFailed);
			return 1;
		}

		return (nSent == len) ? 0 : 1;
	}
	void CloseSocket()
	{
		#if defined(_WIN32) || defined(_WINDOWS)
		closesocket(clientSocket);
#else
		close(clientSocket);
#endif
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	int ReceiveTCP(std::string* pText)
	{
		pText->clear();
		std::vector<char> rawText;
		char* buffer;
		int lBufferSize = 1024;
		std::unique_ptr<char[]> charBuffer = std::make_unique<char[]> (lBufferSize); // Buffer pour la conversion finale
		buffer = charBuffer.get();
		size_t startIndex = 0;
		bool bStarted = false; 
		time_t startTime = CTimeUtils::GetMs();

		for (;;)
		{
			CTimeUtils::CPUSleep(2);
			// Timeout
			if (CTimeUtils::GetMs() - startTime > 500) // 500 ms timeout
			{
				MakeError(CTCPLib::ReceiveFailed);
				CloseSocket();
				return 1;
			}
			int iReceive = recv(clientSocket, buffer, lBufferSize - 1, 0);
			if (iReceive > 0)
			{ 
				buffer[iReceive] = 0;
				bStarted = true; // On a reçu quelque chose, le verrou s'ouvre
				for (auto i=0; i<iReceive; i++) rawText.push_back(buffer[i]);

				int result = CheckIfCompleted(rawText, startIndex);
				if (bStarted && result == 1 && !rawText.empty()) break;
				if (result == -1) return 1; // Erreur inattendue

			}
			else if (iReceive == 0)
			{
				MakeError(CTCPLib::ReceiveFailed);
				CloseSocket();
				return 1;
			}

		}
		//*pText = rawText;
		CloseSocket();
		return 0;
	}
	int CheckIfCompleted(const std::vector<char>&Answer, size_t& startingAt)
	{
		// Check if the received data is complete based on the Modbus TCP protocol
		// The first 6 bytes are the MBAP header, and the 7th byte is the function code
		if (Answer.size() < 7)
		{
			return 0; // Not enough data to determine completion
		}
		// The length of the Modbus TCP message is specified in bytes 4 and 5 of the MBAP header
		uint16_t ExpectedLength = (static_cast<uint8_t>(Answer[4]) << 8) | static_cast<uint8_t>(Answer[5]);
		if (Answer.size() >= 6 + ExpectedLength)
		{
			return 1; // Complete message received
		}
		else
		{
			return 0;
		}
	}
};