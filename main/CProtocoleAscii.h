#pragma once
#include "Protocole.h"
class CProtocoleAscii : public CProtocole
{
private:
	std::string CastEscapedString(std::string UnParsedString)
	{
		size_t i;
		std::string res = "";
		size_t length = UnParsedString.length();
		if (length != 0)
		{
			while (1)
			{
				i = UnParsedString.find('\\');
				if (i == -1) break;
				res += UnParsedString.substr(0, i);
				switch (UnParsedString[i + 1])
				{
				case 'r':
					res += '\r';
					break;
				case 'n':
					res += '\n';
					break;
				case 't':
					res += '\t';
					break;
				default:
					break;
				}
				UnParsedString = UnParsedString.substr(i + 2);
			}
			if (UnParsedString.length() > 0) res += UnParsedString;
		}
		return res;
	}
public:
	CProtocoleAscii();
	virtual void EnvoieCommande();
	virtual int AttenteRetourCommande();
	virtual void EnvoieTrameQuestion(CExternalVariable* pVar);
	virtual int AttenteTrameQuestion();
	virtual double Conversion(std::string s, std::string sTrame, CSortie* pSor);
	char cAttente;
};

