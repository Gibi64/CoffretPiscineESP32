#include "CProtocoleModbusRJE.h"
#include <memory>
#ifndef _WINDOWS


#define LOWORD(l) ((unsigned short)((unsigned int)(l) & 0xFFFF))
#define HIWORD(l) ((unsigned short)((unsigned int)(l) >> 16))
#define LOBYTE(w) ((unsigned char)((unsigned int)(w) & 0xFF))
#define HIBYTE(w) ((unsigned char)((unsigned int)(w) >> 8))
#define MAKEWORD(a, b) ((unsigned short)(((unsigned char)((unsigned int)(a) & 0xFF)) | ((unsigned short)((unsigned char)((unsigned int)(b) & 0xFF))) << 8))
#endif // !_WINDOWS

CProtocoleModbusRJE::CProtocoleModbusRJE()
{
	m_Name = "ModBusRJE";
	TimeOut = 20L;
	bOpen = false;
	bCommand = false;
	bStillWaitingforAnswer = false;
	nWaitingBytes = 0;

}
void CProtocoleModbusRJE::EnvoieTrameQuestion(CExternalVariable* pVar)
{
    char ModbusMsg[200];

    // Modbus Header
    // 
    // Transaction id
    ModbusMsg[0] = 0;
    ModbusMsg[1] = 1;
    // Protocole use 0
    ModbusMsg[2] = 0;
    ModbusMsg[3] = 0;
    // Length : longueur de trame (nombre d'octets ap unit identifier)
    ModbusMsg[4] = 0;
    ModbusMsg[5] = 6;
    // Unit identifier
    ModbusMsg[6] = 1;
    //          code fonction
    ModbusMsg[7] = 3; // default = read holding registers
    //      Registre de debut
    ModbusMsg[8] =pVar->Register/256;// poids fort
    ModbusMsg[9] = pVar->Register - ModbusMsg[8]*256;// poids faible

	switch (pVar->Code_Function_Read_Register)
	{
		case 3:
		case 4:
		{
			// read holding registers
			//////////// 2 mot de 16 bits -> c'est un r�el code fonction 3
			ModbusMsg[7] = pVar->Code_Function_Read_Register;

			//      Nombre de registres 16 bits
			ModbusMsg[10] = static_cast<unsigned char>(HIWORD(pVar->NumberOfRegistersToRead));// poids fort
			ModbusMsg[11] = static_cast<unsigned char>(LOWORD(pVar->NumberOfRegistersToRead));// poids faible

			nWaitingBytes = pVar->NumberOfRegistersToRead * 2;
			this->bStillWaitingforAnswer = false;
			pTCPClient->TransmitTCP(ModbusMsg, 12);
		}
		break;
		case 1:
		{
			// read coils

			ModbusMsg[7] = 1;
			//Nombre de bits
			ModbusMsg[10] = static_cast<unsigned char>(HIWORD(pVar->NumberOfRegistersToRead));// poids fort
			ModbusMsg[11] = static_cast<unsigned char>(LOWORD(pVar->NumberOfRegistersToRead));// poids faible

			bStillWaitingforAnswer = false;
			pTCPClient->TransmitTCP(ModbusMsg, 12);
		}
		break;

		case 2:
		{
			// read discrete registers

			ModbusMsg[7] = 2;
			//Nombre de bits

			ModbusMsg[10] = static_cast<unsigned char>(HIWORD(pVar->NumberOfRegistersToRead));// poids fort
			ModbusMsg[11] = static_cast<unsigned char>(LOWORD(pVar->NumberOfRegistersToRead));// poids faible


			bStillWaitingforAnswer = false;
			pTCPClient->TransmitTCP(ModbusMsg, 12);
		}
		break;

	}


}
int CProtocoleModbusRJE::AttenteTrameQuestion()
{
    char receiveBuffer[200];
	memset(receiveBuffer, 0, sizeof(receiveBuffer));
    //int byteCount = pTCPClient->ReceiveTCP(GetBufferReponse());
    uint16_t src[2];
	int FunctionCode = receiveBuffer[7];
	switch (FunctionCode)
	{
		case 1:
		case 2:
		{
			auto nBytes = receiveBuffer[8];
			if (receiveBuffer[9]) BufferReponse = "1";
			else BufferReponse = "0";
			bStillWaitingforAnswer = false;
			int nGroup = nBytes / 2;
			int iValue = 0;
			for (auto iGroup = 0; iGroup < nGroup; iGroup++)
			{
				xValues[iValue] = MAKEWORD(receiveBuffer[9 +  2 * iGroup + 1], receiveBuffer[9 + 2 * iGroup]);
				iValue++;
			}
			if (nBytes % 2)
			{
				xValues[iValue] = MAKEWORD(0,receiveBuffer[9 + 2 * nGroup]);
			}
			return true;
		}
		break;
		case 3:
		case 4:
		{
			auto nReels = receiveBuffer[8] / 4;
			if (nReels)
			{
				float xx[128];

				for (auto i = 0; i < nReels; i++)
				{
					src[0] = MAKEWORD(receiveBuffer[10 + 4 * i], receiveBuffer[9 + 4 * i]);
					src[1] = MAKEWORD(receiveBuffer[12 + 4 * i], receiveBuffer[11 + 4 * i]);
					xx[i] = modbus_get_float_abcd(src);
					xValues[i] = xx[i];
					BufferReponse = std::to_string(xValues[i]);
				}
			}
			else
			{
				// integer 16 bits
				src[0] = 256 * receiveBuffer[9] + receiveBuffer[10];
				BufferReponse = std::to_string(src[0]);
				xValues[0] = src[0];
			}
		}
		break;
	}
    bStillWaitingforAnswer = false;
    return true;
}
void CProtocoleModbusRJE::EnvoieCommande()
{
	//int adresse = (int)Parametre[1];
	float valeur = (float)Parametre[2];
	int Registre = (int)Parametre[3];
	int TypeOfValue;
	//int NumberOfBytes = (int)Parametre[4];
	TypeOfValue = (int)Parametre[4];
	char ModbusMsg[200];
	int nBytes=0;

	// Modbus Header
	// 
	// Transaction id
	ModbusMsg[0] = 0;
	ModbusMsg[1] = 1;
	// Protocole use 0
	ModbusMsg[2] = 0;
	ModbusMsg[3] = 0;
	// Length : longueur de trame (nombre d'octets ap unit identifier)
	ModbusMsg[4] = 0;
	ModbusMsg[5] = 6;
	// Unit identifier
	ModbusMsg[6] = 1;
	//          code fonction
	ModbusMsg[7] = 16;
	switch (TypeOfValue)
	{
		case 1:
		{
			// Type Integer
			// Used by automates with no coil writing code the function code is 6
			int nValue = (int)valeur;
			ModbusMsg[5] = 6;
			ModbusMsg[7] = 6; // Function code
			ModbusMsg[8] = Registre/256;
			ModbusMsg[9] =Registre - ModbusMsg[8] * 256;
			ModbusMsg[10] = nValue/256;
			ModbusMsg[11] = nValue - ModbusMsg[13] * 256;
			nBytes = 12;
			
			break;
		}
		case 2:
		{
			// Type real
			ModbusMsg[8] = Registre/256;
			ModbusMsg[9] = Registre - ModbusMsg[8] * 256;
			ModbusMsg[10] = 0;
			ModbusMsg[11] = 2;
			// Byte Count
			ModbusMsg[12] = 4;
			uint16_t floatWord[2];
			modbus_set_float_abcd(valeur, floatWord);
			ModbusMsg[14] = floatWord[0]/256;
			ModbusMsg[13] = floatWord[0] - 256 * ModbusMsg[14];
			ModbusMsg[16] = floatWord[1] / 256;
			ModbusMsg[15] = floatWord[1] - 256 * ModbusMsg[16];

			ModbusMsg[5] = 11;
			nBytes = 17;

			break;
		}
		case 3:
		{
			//
			// Masque de Bit : la fonction n'est pas 16 mais 5
			ModbusMsg[7] = 5;
			// Registre
			ModbusMsg[8] = Registre / 256;
			ModbusMsg[9] = Registre - ModbusMsg[8] * 256;

			ModbusMsg[10] = 0;
			ModbusMsg[11] = valeur > 0;
			ModbusMsg[5] = 6;
			nBytes = 12;
			break;
		}
	}
	pTCPClient->TransmitTCP(ModbusMsg, nBytes);
	AttenteRetourCommande();
}
int CProtocoleModbusRJE::AttenteRetourCommande()
{
	int n = pTCPClient->ReceiveTCP(GetBufferReponse());
	return n;

}
