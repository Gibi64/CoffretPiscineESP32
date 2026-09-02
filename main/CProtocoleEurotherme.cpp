#include "CProtocoleEuroTherme.h"
#include "CTimeUtils.hpp"
#include <cmath>
//*****************************************************
CProtocoleEuroTherme::CProtocoleEuroTherme()
{
	m_Name = "Eurotherm";
	bOpen = false;
	bCommand = false;
	bStillWaitingforAnswer = false;
	cAttente = 0x0d;

}

int CProtocoleEuroTherme::AttenteTrameQuestion()
{
	bool bFind = false;
	bool bETX = false;
	int jETX;
	int j;
	while (!bETX)
	{
		if ((float)CTimeUtils::GetMs() > TimeEnd)
		{
			if (nReceive)
				write_log("TimeOut Receive without ETX  = " + *GetBufferReponse());

			else
				write_log("TimeOut Receive without ETX  - Empty Buffer");
			bStillWaitingforAnswer = false;
			return true;
		}
		bool iRet = CProtocole::AttenteTrameQuestion();
		if (iRet)
		{
			for (j = 0; j < nReceive; j++)
			{
				if ((*GetBufferReponse())[j] == 3)
				{
					bETX = true;
					jETX = j;

					//			Lecture du bcc
					// wait for the last caracter after ETX
					if (nReceive < jETX + 1)
						while (!CProtocole::AttenteTrameQuestion() && ((float)CTimeUtils::GetMs() <= TimeEnd));
					unsigned char bcc = 0;
					for (int k = 1; k <= jETX; k++)
						bcc = bcc ^ (*GetBufferReponse())[k];
					if (bcc != (*GetBufferReponse())[jETX + 1])
					{
						write_log("Erreur bcc");
						if (nReceive)
							write_log("Receive : " + *GetBufferReponse());
						xValues[0] = atof(BufferReponse.c_str());

						return true;
					}
					bStillWaitingforAnswer = false;
					return true;

				}
			}
		}
	}
	bStillWaitingforAnswer = false;
	return true;
}

void CProtocoleEuroTherme::EnvoieCommande()
{
	char EOT = 0x4;
	char STX = 0x2;
	char ETX = 0x3;
	unsigned char bcc;
	int decimal;
	bool bvaleur;
	double valeur;
	int adresse;
	size_t l, i;
	//  MessageBox(NULL, L"Envoi de la commande EuroTherme", L"", MB_OK);
	char form[26] = "%c%d%d%d%d%c%s%-#6.1f%c";
	char Print_Buffer[512];
	bvaleur = Parametre[2] < 1e+30;
	adresse = (int)Parametre[1];
	valeur = Parametre[2];
	if (bvaleur)
	{
		if (valeur == 0) decimal = 3;
		else
		{
			l = (int)log10(fabs(valeur));
			if (l < 0)
				decimal = 4;
			else
				decimal = 4 - (int)l;
		}
		if (valeur < 0) --decimal;
		if (decimal < 0) decimal = 0;
		form[19] = decimal + '0';
		#if defined(_WINDOWS)
		sprintf_s(Print_Buffer, form,
			EOT, adresse / 10, adresse / 10, adresse % 10,
			adresse % 10, STX, Commande.data(), valeur, ETX);
		Commande = Print_Buffer;
		#else
		sprintf(Print_Buffer, form,
			EOT, adresse / 10, adresse / 10, adresse % 10,
			adresse % 10, STX, Commande.data(), valeur, ETX);
		Commande = Print_Buffer;
		#endif	
		l = Commande.length();
		for (i = 6; i < l; i++)
		{
			if (Commande[i] == ' ') Commande[i] = '0';
		}
	}
	else
	{
		#if defined(_WINDOWS)
		sprintf_s(Print_Buffer, "%c%d%d%d%d%c%s%c",
			EOT, adresse / 10, adresse / 10, adresse % 10,
			adresse % 10, STX, Commande.data(), ETX);
		Commande = Print_Buffer;
		#else
		sprintf(Print_Buffer, "%c%d%d%d%d%c%s%c",
			EOT, adresse / 10, adresse / 10, adresse % 10,
			adresse % 10, STX, Commande.data(), ETX);
		Commande = Print_Buffer;
		#endif
	}
	bcc = 0;
	l = Commande.length();
	for (i = 6; i < l; i++) bcc = bcc ^ Commande[i];

	Commande.append(1, bcc);
	Commande.append(1, 0);

	{
		CTimeUtils::CPUSleep(5);
		pLigne->sendArray((char*)Commande.c_str(), (int)Commande.length());
	}


}
int CProtocoleEuroTherme::AttenteRetourCommande()
{
	int i, j;
	char c;
	{
		i = pLigne->getArray(&c, 1);
		if (i > 0)
		{
			*GetBufferReponse() += c;
			for (j = 0; j < i; j++)
			{
				if (c == 6)
				{
					return true;
				}
			}
		}
	}

	return false;
}
void CProtocoleEuroTherme::EnvoieTrameQuestion(CExternalVariable* pVar)
{
	int j;
	char RequestEuro[20];
	char stx = 0x4;
	char etx = 0x5;
	RequestEuro[0] = 4;
	for (j = 1; j <= (int)pVar->StringOfRequest.length(); j++)
		RequestEuro[j] = (char)pVar->StringOfRequest[j - 1];
	RequestEuro[j++] = 5;
	RequestEuro[j] = 0;
	Commande = RequestEuro;
	pLigne->sendArray((char*)Commande.c_str(),(int)Commande.length());
	std::string RequestKeyword = pVar->StringOfRequest.substr(4);
	std::string ReceiveKeyword = *GetBufferReponse();
	int iEtx = (int)ReceiveKeyword.find(2);
	if (iEtx != -1)
	{
		ReceiveKeyword = ReceiveKeyword.substr(iEtx + 1);
		ReceiveKeyword = ReceiveKeyword.substr(0, RequestKeyword.length());
		if (ReceiveKeyword != RequestKeyword) (*GetBufferReponse())[0] = 0;
	}
	else
		GetBufferReponse()->clear();

}

double CProtocoleEuroTherme::Conversion(std::string s, std::string sTrame, CSortie* pSor)
{
	return (pSor->Gain * atof((char*)s.data()) + pSor->Offset);
}
