#include "CProtocoleHbm.h"
// *************************************************

CProtocoleHbm::CProtocoleHbm()
{
	//	TimeOut=1000L;
	m_Name = "Hbm";
	bOpen = false;
	bStillWaitingforAnswer = false;
}

void CProtocoleHbm::EnvoieCommande()
{
	//	Ligne->msComm()->SetHandshaking(2);
	if (Commande.back() != 13) Commande.push_back(13);
	pLigne->sendArray((char*)Commande.c_str(), (int)Commande.length());

}
int CProtocoleHbm::AttenteRetourCommande()
{
	int i;
	int lMax = 512;
	std::unique_ptr<char> Mem = std::make_unique<char>(lMax + 1);
	memset(Mem.get(), 0, lMax);
	if (CProtocole::AttenteRetourCommande())
	{
		i = pLigne->getArray(Mem.get(), lMax);
		if (i > 0)
		{
			*GetBufferReponse() = Mem.get();
			if (GetBufferReponse()->find(10) != std::string::npos)
				return true;
		}
	}

	return false;
}
void CProtocoleHbm::EnvoieTrameQuestion(CExternalVariable* pVar)
{
	char Buffer[128];
#if defined(_WINDOWS)
	strcpy_s(Buffer, (char*)pVar->StringOfRequest.data());
	if (Commande.at(pVar->StringOfRequest.length() - 1) != 13) strcat_s(Buffer, "\r");
#else
	strcpy(Buffer, (char*)pVar->StringOfRequest.data());
	if (Commande.at(pVar->StringOfRequest.length() - 1) != 13) strcat(Buffer, "\r");
#endif
	double xx = CTimeUtils::GetMs() + TimeOut;
	while (CTimeUtils::GetMs() <= xx);
	pLigne->sendArray(Buffer, (int)strlen(Buffer));

}
int CProtocoleHbm::AttenteTrameQuestion()
{
	int j, nLf;
	if (CProtocole::AttenteTrameQuestion())
	{
		nLf = 0;
		for (j = 0; j < nReceive; j++)
		{
			if ((*GetBufferReponse())[j] == 10) nLf++;
			if ((*GetBufferReponse())[j] == ' ') (*GetBufferReponse())[j] = '0';
		}
		if (nLf >= 1)
		{
			(*GetBufferReponse())[0] = 'A';
			bStillWaitingforAnswer = false;
			xValues[0] = atof(BufferReponse.c_str());

			return true;
		}
	}
	bStillWaitingforAnswer = false;
	return false;
}
double CProtocoleHbm::Conversion(std::string s, std::string sTrame, CSortie* pSor)
{
	return (pSor->Gain * atof((char*)s.data()) + pSor->Offset);
}
// *************************************************
