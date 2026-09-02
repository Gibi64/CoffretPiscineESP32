#pragma once
#include "Protocole.h"
class CProtocoleJBus : public CProtocole
{

public:
	CProtocoleJBus();
	virtual void EnvoieCommande();
	virtual int AttenteRetourCommande();
	virtual void EnvoieTrameQuestion(CExternalVariable* pVar);
	virtual int AttenteTrameQuestion();
	char cAttente;
	void crc16(char* ptc, int nc);
	int Decode_Jbus(uint8_t pfort, uint8_t pfaible);
	virtual double Conversion(std::string s, std::string sTrame, CSortie* pSor);
};