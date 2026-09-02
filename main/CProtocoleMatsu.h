#pragma once
#include "Protocole.h"
class CProtocoleMatsu : public CProtocole
{

public:
	CProtocoleMatsu();
	virtual void EnvoieCommande();
	virtual int AttenteRetourCommande();
	virtual void EnvoieTrameQuestion(CExternalVariable* pVar);
	virtual int AttenteTrameQuestion();
	virtual double Conversion(std::string s, std::string sTrame, CSortie* pSor);
	char cAttente;
};

