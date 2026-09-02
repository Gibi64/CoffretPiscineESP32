#include "CProtocoleMatsu.h"
CProtocoleMatsu::CProtocoleMatsu()
{
	m_Name = "Matsu";
	bOpen = false;
	bStillWaitingforAnswer = false;
	cAttente = 0x0d;
}
void CProtocoleMatsu::EnvoieCommande()
{
	pLigne->sendArray((char*)Commande.c_str(), (int)Commande.length());

}
int CProtocoleMatsu::AttenteRetourCommande()
{
	int i = 0, lMax = 512;

	if (CProtocole::AttenteRetourCommande())
	{
		std::unique_ptr<char> Mem = std::make_unique<char>(lMax + 1);
		memset(Mem.get(), 0, lMax);
		i = pLigne->getArray(Mem.get(), lMax);

		if (i > 0)
		{
			*GetBufferReponse() = Mem.get();
			if (GetBufferReponse()->find(13) != std::string::npos) return true;
		}
	}

	return false;
}
void CProtocoleMatsu::EnvoieTrameQuestion(CExternalVariable* pVar)
{
	char Buffer[128];
	#if defined(_WINDOWS)
	strcpy_s(Buffer, (char*)pVar->StringOfRequest.data());
	strcat_s(Buffer, "\r");
	#else
	strcpy(Buffer, (char*)pVar->StringOfRequest.data());
	strcat(Buffer, "\r");
	#endif
	double xx = CTimeUtils::GetMs() + TimeOut;
	while (CTimeUtils::GetMs() <= xx);
	pLigne->sendArray(Buffer, (int)strlen(Buffer));

}
int CProtocoleMatsu::AttenteTrameQuestion()
{
	int j, nLf;
	if (CProtocole::AttenteTrameQuestion())
	{
		nLf = 0;
		for (j = 0; j < nReceive; j++)
		{
			if ((*GetBufferReponse())[j] == 13) nLf++;
			if ((*GetBufferReponse())[j] == ' ') (*GetBufferReponse())[j] = '0';
		}
		if (nLf >= 1)
			bStillWaitingforAnswer = false;
		xValues[0] = atof(BufferReponse.c_str());

		return true;
	}
	bStillWaitingforAnswer = false;
	return false;
}
double CProtocoleMatsu::Conversion(std::string s, std::string sTrame, CSortie* pSor)
{
	return (pSor->Gain * atof((char*)s.data()) + pSor->Offset);
}
