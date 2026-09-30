#include "CProtocoleAleatoire.h"
//*****************************************************
CProtocoleAleatoire::CProtocoleAleatoire()
{
	m_Name = "Aleatoire";
	srand((unsigned)time(nullptr));
	bOpen = false;
	bCommand = false;
	bStillWaitingforAnswer = false;
	cAttente = 0x0d;
}

void CProtocoleAleatoire::EnvoieCommande()
{
	bCommand = false;

}
int CProtocoleAleatoire::AttenteRetourCommande()
{
	return false;
}
void CProtocoleAleatoire::EnvoieTrameQuestion(CExternalVariable* pVar)
{
}
#include "CProtocoleAleatoire.h"
#include <stdio.h>
int CProtocoleAleatoire::AttenteTrameQuestion()
{
	int lMax = 512;
	char* Mem = new char[lMax + 1];
	memset(Mem, 0, lMax);
#if defined(_WINDOWS)
	sprintf_s(Mem, lMax, "%d", rand());
	#else
	sprintf(Mem, "%d", rand());
	#endif
	*GetBufferReponse() = Mem;
	nReceive = static_cast<int>(GetBufferReponse()->length());
	xValues[0] = atof(BufferReponse.c_str());
	bStillWaitingforAnswer = false;
	delete[] Mem;
	return true;
}

double CProtocoleAleatoire::Conversion(std::string s, std::string sTrame, CSortie* pSor)
{
	return (pSor->Gain * atof((char*)s.data()) + pSor->Offset);
}
