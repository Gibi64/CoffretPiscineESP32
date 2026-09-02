#pragma once
#include "Protocole.h"
class CProtocoleHbm : public CProtocole
{

public:
	CProtocoleHbm();
	virtual void EnvoieCommande();
	virtual int AttenteRetourCommande();
	virtual void EnvoieTrameQuestion(CExternalVariable* pVar);
	virtual int AttenteTrameQuestion();
	virtual double Conversion(std::string s, std::string sTrame, CSortie* pSor);
	char cAttente;
};
