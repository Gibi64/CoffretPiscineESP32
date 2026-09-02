//*************************************************
#include "CProtocoleAscii.h"
#include "CTimeUtils.hpp"

CProtocoleAscii::CProtocoleAscii()
{
	//TimeOut=1000L;
	CProtocole();
	m_Name = "Ascii";
	bOpen = false;
	bCommand = false;
	bStillWaitingforAnswer = false;
	cAttente = 0x0d;
}

void CProtocoleAscii::EnvoieCommande()
{
	//Commande.Format("%d %s set%c",Parametre[2],Commande.data(), 0xd);
	std::string ParsedCommande = CastEscapedString(Commande);
	size_t length = ParsedCommande.length();
	pLigne->sendArray((char*)ParsedCommande.data(), (int)length);
	write_log("[%f] Sent ascii command : "+ ParsedCommande);
	TimeEnd = (float)CTimeUtils::GetMs() + TimeOut;
}
int CProtocoleAscii::AttenteRetourCommande()
{
	CProtocole::AttenteRetourCommande();

	std::string szBufferReponse = *GetBufferReponse();

	if (szBufferReponse.find(WaitingString) != -1)
	{
		bStillWaitingforAnswer = false;
		write_log("[%f] Received ascii : "+ szBufferReponse);
		return true;
	}
	else if ((float)CTimeUtils::GetMs()  > TimeEnd)
	{
		write_log("[%f] TimeOut : "+ szBufferReponse);
		bStillWaitingforAnswer = false;
		return true;
	}
	return false;
}

void CProtocoleAscii::EnvoieTrameQuestion(CExternalVariable* pVar)
{
	std::string question = pVar->StringOfRequest;
	size_t length = question.length();
	pLigne->sendArray((char*)question.data(), (int)length);
	write_log("[%f] Sent ascii : "+ question);
	TimeEnd = (float)CTimeUtils::GetMs() + TimeOut;
}
int CProtocoleAscii::AttenteTrameQuestion()
{
	CProtocole::AttenteTrameQuestion();
	std::string szBufferReponse = *GetBufferReponse();

	if (szBufferReponse.find(WaitingString) != std::string::npos)
	{
		bStillWaitingforAnswer = false;
		write_log("[%f] Received ascii : "+ szBufferReponse);
		xValues[0] = atof(BufferReponse.c_str());
		return true;
	}
	else if ((float)CTimeUtils::GetMs() > TimeEnd)
	{
		write_log("[%f] TimeOut : "+ szBufferReponse);
		bStillWaitingforAnswer = false;
		xValues[0] = atof(BufferReponse.c_str());
		return true;
	}
	return false;
}
double CProtocoleAscii::Conversion(std::string s, std::string sTrame, CSortie* pSor)
{
	return (pSor->Gain * atof((char*)s.data()) + pSor->Offset);
}