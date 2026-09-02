
#pragma once
#include "CTCP_Modbus.h"
#include <string>
#include<stdlib.h>
#include<list>
#include "CSerialClient.h"
/*#ifdef _SOLARTRON
#include <vcclr.h>
#endif*/

std::string CastEscapedString(std::string UnParsedString);
class CExternalVariable
{
public:
	CExternalVariable()
	{
		bFromAdvise = false;
	};
	std::string StringOfRequest;
	double Frequency;
	std::string Id;
	double m_LastTimeOfRequest;
	std::string LastValue;
	int LineOfGrid;
	int AdressOfCard;
	int Register;
	int NumberOfRegistersToRead;
	int Code_Function_Read_Register;
	bool bFromAdvise;
};
class CSortie
{
public:
	double Offset;
	double Gain;
};

class CProtocole
{

public:
	CTCP_Modbus* pTCPClient;
	CSerialClient* pLigne;
	FILE* ptrLog;
	bool bNeedCommunicationPort;

	std::string* GetBufferReponse();


	float TimeOut;
	float TimeEnd;
	std::string m_Name;
	virtual void EnvoieCommande();
	virtual int AttenteRetourCommande();
	virtual void EnvoieTrameQuestion(CExternalVariable* pVar);
	virtual int AttenteTrameQuestion();
	virtual double Conversion(std::string s, std::string sTrame, CSortie* pSor);
	void AddExternalVariable(std::string Id, std::string StringOfRequest, double Frequency);
	std::list <CExternalVariable*> m_ListOfExternalVariables;
	std::string ComId;
	int NumberId;
	bool bOpen;
	bool bCommand;
	bool bRequest;
	CExternalVariable* pRequestedVariable;
	std::string Commande;
	double Parametre[10];
	std::string WaitingString;


protected:
	std::list<CExternalVariable*>::iterator PosOfExternalVariable;
	int nReceive;
	std::string BufferReponse;
	double xValues[128];
public:
	void SetRequestValue(CExternalVariable*);
	std::string GetRequestValue(void);
	double* GetValue();
	bool bStillWaitingforAnswer;
};


#define MOTINIT 20 

#ifdef _SOLARTRON
class CProtocoleSolartron : public CProtocole
{
public:
	CProtocoleSolartron();
	gcroot < Solartron::Orbit3::OrbitServer^> virtualOrbServer;
	virtual void EnvoieCommande();
	virtual int AttenteRetourCommande();
	virtual void EnvoieTrameQuestion(CExternalVariable* pVar);
	virtual int AttenteTrameQuestion();
	virtual double Conversion(std::string s, std::string sTrame, CSortie* pSor);
};
#endif
