#include "CProtocoleFaulhaber.h"
#include "CTimeUtils.hpp"

CProtocoleFaulhaber::CProtocoleFaulhaber()
{
	m_Name = "Faulhaber";
	TimeOut = 20L;
	bOpen = false;
	bCommand = false;
	bStillWaitingforAnswer = false;
}
void CProtocoleFaulhaber::EnvoieTrameQuestion(CExternalVariable* pVar)
{
	#ifndef _WINDOWS
		#define HIWORD(x) ((uint8_t)((x) >> 8))
		#define LOWORD(x) ((uint8_t)((x) & 0xFF))
	#endif
	//int noctet = 8;
	uint8_t bfc[20];
	uint16_t index = 0x1018;
	uint8_t subIndex = 0x00;  // Number of entries

	bfc[0] = 'S';
	bfc[1] = 7; //Request length 
	bfc[2] = 0;//Node number
	bfc[3] = 1;//Request code
	bfc[4] = LOWORD(index);
	bfc[5] = HIWORD(index);
	bfc[6] = subIndex;
	bfc[7] = crc(bfc, 6);
	bfc[8] = 'E';
	
	nWaitingBytes = 7;
	TimeEnd = (float)CTimeUtils::GetMs() + TimeOut;

	pLigne->sendArray((char*)bfc, 9);


}
uint8_t CProtocoleFaulhaber::crc(uint8_t* bfc, int n)
{
	uint8_t Crc = 0xFF;
	for (auto i = 1; i < n + 1; i++)
	{
		Crc = Crc ^ bfc[i];
		for (int iBit = 0; iBit < 8; iBit++)
		{
			if (Crc & 0x01)
				Crc = (Crc >> 1) ^ 0xD5;
			else
				Crc >>= 1;
		}
	}
	return Crc;
}
int CProtocoleFaulhaber::AttenteTrameQuestion()
{
	bool bEnd = false;
	//while ((float)CTimeUtils::GetMs() <= TimeEnd && !bEnd)
	for (;;)
	{
		nReceive = CProtocole::AttenteTrameQuestion();
		if (nReceive >= nWaitingBytes)
		{
			bEnd = true;
			//char c = (*GetBufferReponse())[6];
			break;
		}
	}
	xValues[0] = atof(BufferReponse.c_str());

	return bEnd;
}
