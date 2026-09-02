#pragma once
#include "Protocole.h"
#include "modbus_convert.h"
class CProtocoleModbusRJE : public CProtocole
{
private:
    int nWaitingBytes;
	enum {PARAMETER_ADRESS=1, PARAMETER_VALUE, PARAMETER_REGISTER, PARAMETER_TYPE_OF_VALUE, PARAMETER_NUMBER_OF_REGISTERS_TO_READ};
public:
	enum { TYPE_INTEGER = 1, TYPE_REAL, TYPE_BIT };

    CProtocoleModbusRJE();
	~CProtocoleModbusRJE()
	{
		if (pTCPClient) delete pTCPClient;
	}
    void EnvoieTrameQuestion(CExternalVariable* pVar);
    int AttenteTrameQuestion();
    void EnvoieCommande();
    int AttenteRetourCommande();
	bool m_bRJE_Transport;
	void SetAdress(int Adress)
	{
		if (Adress < 1) Adress = 1;
		if (Adress > 247) Adress = 247;
		Parametre[PARAMETER_ADRESS] = (double)Adress;
	}
	void SetRegister(int Register)
	{
		if (Register < 0) Register = 0;
		if (Register > 65535) Register = 65535;
		Parametre[PARAMETER_REGISTER] = (double)Register;
	}
	void SetNumberOfRegistersToRead(int NumberOfRegistersToRead)
	{
		// ce parametre n'est pas utilisé car on lit qu'un seul registre à la fois
		if (NumberOfRegistersToRead < 0) NumberOfRegistersToRead = 0;
		if (NumberOfRegistersToRead > 65535) NumberOfRegistersToRead = 65535;
		Parametre[PARAMETER_NUMBER_OF_REGISTERS_TO_READ] = (double)NumberOfRegistersToRead;
	}
	void SetValue(float Value)
	{
		if (Value < 0) Value = 0;
		if (Value > 65535) Value = 65535;
		Parametre[PARAMETER_VALUE] = (double)Value;
	}
	void SetTypeOfValue(int TypeOfValue)
	{
		if (TypeOfValue < 1) TypeOfValue = 1;
		if (TypeOfValue > 3) TypeOfValue = 3;
		Parametre[PARAMETER_TYPE_OF_VALUE] = (double)TypeOfValue;
	}
};

