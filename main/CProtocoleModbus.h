#pragma once
#include "protocole.h"
class CProtocoleModBus : public CProtocole
{
public:
	CProtocoleModBus();
	virtual void EnvoieCommande();
	virtual int AttenteRetourCommande();
	virtual void EnvoieTrameQuestion(CExternalVariable* pVar);
	virtual int AttenteTrameQuestion();
	char cAttente;
	unsigned short crc(unsigned char* pucFrame, unsigned short usLen);
	virtual double Conversion(std::string s, std::string sTrame, CSortie* pSor);
	int nWaitingBytes;
protected:
};
