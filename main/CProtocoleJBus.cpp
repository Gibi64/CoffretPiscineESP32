#include "CProtocoleJBus.h"
#include "CTimeUtils.hpp"
//*****************************************************
CProtocoleJBus::CProtocoleJBus()
{
	m_Name = "JBus";
	bOpen = false;
	bStillWaitingforAnswer = false;
	cAttente = 0x0d;
}

void CProtocoleJBus::EnvoieCommande()
{

}
int CProtocoleJBus::AttenteRetourCommande()
{
	return false;
}
void CProtocoleJBus::EnvoieTrameQuestion(CExternalVariable* pVar)
{

	int noctet = 8;
	char bfc[20];

	bfc[0] = 1;
	bfc[1] = 4;
	bfc[2] = 0;
	bfc[3] = 20; //        mot initial � d�terminer par les devices
	bfc[4] = 0;
	bfc[5] = 12;//         nombre de mot � lire (� d�terminer par les devices)
	crc16(bfc, 6);
	bfc[8] = 0;
	double xx = CTimeUtils::GetMs() + TimeOut;
	while (CTimeUtils::GetMs() <= xx);
	pLigne->sendArray(bfc, noctet);

}
int CProtocoleJBus::AttenteTrameQuestion()
{
	bool bFind = false;
	bool bETX = false;
	int jETX;
	int j;


	CProtocole::AttenteTrameQuestion();
	for (j = 0; j < nReceive; j++)
	{
		if ((*GetBufferReponse())[j] == 3)
		{
			bETX = true;
			jETX = j;

			//			Lecture duCrc
			while (!CProtocole::AttenteTrameQuestion() && ((float)CTimeUtils::GetMs() <= TimeEnd));
			unsigned char bcc = 0;
			for (int k = 1; k <= jETX; k++)
				bcc = bcc ^ (*GetBufferReponse())[k];
			if (bcc != (*GetBufferReponse())[jETX + 1])
			{
				write_log("Erreur bcc");
				if (nReceive) write_log("Receive : " + *GetBufferReponse());
				bStillWaitingforAnswer = false;
				xValues[0] = atof(BufferReponse.c_str());

				return true;
			}
			bStillWaitingforAnswer = false;
			return true;

		}
	}

	if ((float)CTimeUtils::GetMs() > TimeOut)
	{
		write_log("TimeOut");
		bStillWaitingforAnswer = false;
		return true;
	}
	else
	{

		return false;
	}
}
void CProtocoleJBus::crc16(char* ptc, int nc)
{
	//Crc16(ptc,nc);
}
int CProtocoleJBus::Decode_Jbus(unsigned char pfort, unsigned char pfaible)
{
	int Reponse;
	if (pfort > 128)
	{
		pfort = pfort & 0x7F;
		Reponse = 256 * pfort + pfaible;
		Reponse = Reponse | 0x8000;
		Reponse = (~Reponse) + 1;
		Reponse = -Reponse;
	}
	else Reponse = 256 * pfort + pfaible;
	return (Reponse);
}
double CProtocoleJBus::Conversion(std::string sReponse, std::string sTrame, CSortie* pSor)
{

	int k = (int)sTrame.find("/");
	double x;
	if (k == -1)
	{
		int MotaLire = (atoi((char*)sTrame.data()) - MOTINIT) * 2 + 3;
		x = Decode_Jbus((unsigned char)sReponse.at(MotaLire), (unsigned char)sReponse.at(MotaLire + 1));
	}
	else
	{
		int MotaLire = (atoi((char*)sTrame.data()) - MOTINIT) * 2 + 4;
		unsigned char BitaLire = atoi((char*)sTrame.at(k + 1));
		if (BitaLire > 7)
		{
			BitaLire -= 8;
			--MotaLire;
		}
		x = (((1 << BitaLire) & ((unsigned char)sReponse.at(MotaLire))) > 0);
	}

	return (pSor->Gain * x + pSor->Offset);
}
