#include <memory>
#include "Protocole.h"
#include "CTimeUtils.hpp"

#include <math.h>
#include <time.h>
#include <stdio.h>
void CProtocole::EnvoieCommande()
{

}
double* CProtocole::GetValue()
{
	return xValues;
}

int CProtocole::AttenteRetourCommande()
{
	TimeEnd = (float)CTimeUtils::GetMs() + TimeOut;
	if ((float)CTimeUtils::GetMs() >= TimeEnd)
	{
		return false;
	}


	return true;
}
std::string* CProtocole::GetBufferReponse()
{
	return &BufferReponse;
}

void CProtocole::EnvoieTrameQuestion(CExternalVariable* pVar)
{


}
int CProtocole::AttenteTrameQuestion()
{
	int i;
	if (this->pLigne)
	{
		char c;
		i = pLigne->getArray(&c, 1);

		if (i <= 0)
		{
			return(false);
		}
		GetBufferReponse()->push_back(c);
		nReceive++;
		return true;
	}
	else
	{
		char* pBuf = new char[512];
		auto n = pTCPClient->ReceiveTCP(GetBufferReponse());
		if (!n)
		{
			return false;
		}
		else
		{
			nReceive = n;
			//for (auto i = 0; i < n; i++) *GetBufferReponse() += pBuf[i];
			//delete[] pBuf;
			return true;
		}
	}
	return false;
}

double CProtocole::Conversion(std::string s, std::string sTrame, CSortie* pSor)
{
	return (pSor->Gain * atof((char*)s.data()) + pSor->Offset);
}


void CProtocole::AddExternalVariable(std::string Id, std::string StringOfRequest, double Frequency)
{
	CExternalVariable* pExternalVariable = new (CExternalVariable);
	pExternalVariable->Id = Id;
	pExternalVariable->StringOfRequest = StringOfRequest;
	pExternalVariable->Frequency = Frequency;
	pExternalVariable->m_LastTimeOfRequest = CTimeUtils::GetMs	();
	m_ListOfExternalVariables.push_back(pExternalVariable);
}








////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _SOLARTRON
#pragma warning(disable:4691)
CProtocoleSolartron::CProtocoleSolartron()
{
	m_Name = "Solartron";
	bOpen = false;
	bStillWaitingforAnswer = false;
	virtualOrbServer = gcnew Solartron::Orbit3::OrbitServer;
	virtualOrbServer->Connect();
	if (virtualOrbServer->Connected)
	{
		virtualOrbServer->Networks[0]->Modules->FindHotswapped();
	}
}

void CProtocoleSolartron::EnvoieTrameQuestion(CExternalVariable* pVar)
{

	std::string question = pVar->StringOfRequest;// NameId du Module
	wchar_t* SearchId = new wchar_t[question.length() + 1];
	MultiByteToWideChar(CP_ACP, NULL, question.data(), -1, SearchId, (int)question.length() + 1);
	//			Recherche du nom de module
	if (virtualOrbServer->Networks->Count > 0)
	{
		for (int loop = 0; loop < virtualOrbServer->Networks[0]->Modules->Count; loop++)
		{
			System::String^ NameOfModule = virtualOrbServer->Networks[0]->Modules[loop]->GetProbeID();
			pin_ptr<const wchar_t> virtualNameOfModule = PtrToStringChars(NameOfModule);
			if (!wcscmp(virtualNameOfModule, SearchId))
			{
				double ModuleReading = virtualOrbServer->Networks[0]->Modules[loop]->ReadingInUnits;
				sprintf_s(pLigne->BufferReponse, 512, "%f", ModuleReading);
				break;
			}
		}
	}
	else
	{
		sprintf_s(pLigne->BufferReponse, 512, "NA");
	}

	delete[] SearchId;
	TimeEnd = (float)CTimeUtils::GetMs() + TimeOut;
}

int CProtocoleSolartron::AttenteTrameQuestion()
{
	bStillWaitingforAnswer = false;
	return true;
}
void CProtocoleSolartron::EnvoieCommande()
{
	strcpy_s(pLigne->BufferCommande, (char*)Commande.data());
	bCommand = false;
	//	bRequest=false;

}
int CProtocoleSolartron::AttenteRetourCommande()
{
	return false;
}

double CProtocoleSolartron::Conversion(std::string s, std::string sTrame, CSortie* pSor)
{
	return (pSor->Gain * atof((char*)s.data()) + pSor->Offset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
void CProtocole::SetRequestValue(CExternalVariable* pVar)
{
	if (!bStillWaitingforAnswer)
	{
		if (bNeedCommunicationPort && this->bOpen)
		{
			pLigne->PurgeTransmitBuffer();
			pLigne->PurgeReceiveBuffer();
		}

		GetBufferReponse()->clear();
		nReceive = 0;
		bStillWaitingforAnswer = true;
		EnvoieTrameQuestion(pVar);
		while (!AttenteTrameQuestion());
		bStillWaitingforAnswer = false;
	}


}





std::string CProtocole::GetRequestValue(void)
{

	return std::string(*GetBufferReponse());
}










