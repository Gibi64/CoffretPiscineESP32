#include <vector>
#include <string>
#include "CoffretPiscine.h"
#include "InitLog.hpp"
#include "CParserXML.hpp"
#include "CTimeUtils.hpp"
#include "CProtocoleModbusRJE.h"
#ifdef _ESP32
#include <cstdio>
#else
#include <io.h>
#endif
///////////////////// fonction de conversion d'une chaine de caract�res en un type de date/heure sp�cifique
///////////////////// utimis�e pour lire les dates dans le fichier de configuration XML
template<typename TDate>
TDate Convert(const std::string& input)
{
	TDate sReturn{};
	std::string str = input;   // copie locale modifiable

	int iIndex = 0;

	for (;;)
	{
		size_t pos = str.find(',');
		if (pos == std::string::npos)
		{
			sReturn.SetIndexValue(iIndex, std::stoi(str));
			break;
		}

		sReturn.SetIndexValue(iIndex, std::stoi(str.substr(0, pos)));
		str.erase(0, pos + 1);
		iIndex++;
	}

	return sReturn;

}
void CCoffretPiscine::ReadConfigFile(std::string FileName, CTCPLib* pTCPLib)
{
	///////////////////// Lecture du fichier de configuration XML
	// Purge des anciennes données
	for (auto & entry : m_VectorEntry)
	{
		if (m_map_IP.count(entry.IPAdress))
		{
			delete m_map_IP[entry.IPAdress];
			m_map_IP.erase(entry.IPAdress);
		}
	}
	for (auto& entry : m_VectorEvents)
	{
		// On met le relais à l'état OFF avant de supprimer l'événement
		SendRelay(entry->GetActionId(), false);
		delete entry;
	}
	m_VectorEntry.clear();
	FILE* ptr = nullptr;
#ifdef _WIN32
	fopen_s(&ptr, FileName.c_str(), "r");
#else
	ptr = fopen(FileName.c_str(), "r");
#endif
	if (!ptr)
	{
		write_log("Failed to open file: " + FileName);
		return;
	}
	fseek(ptr, 0, SEEK_END);
	auto size = ftell(ptr);
	fseek(ptr, 0, SEEK_SET);
	std::vector<char> Vec(size + 1);
	fread(Vec.data(), 1, size, ptr);
	fclose(ptr);
	Vec[size] = 0; // Null-terminate the string
	XMLDocumentA doc;
	if (Vec.size() >= 3 && (unsigned char)Vec.at(0) == 0xEF && (unsigned char) Vec.at(1) == 0xBB && (unsigned char) Vec.at(2) == 0xBF)
	{
		//			On retire les trois premiers éléments

		for (int i = 0; i < 3; i++) Vec.erase(Vec.begin());
	}
	Vec.push_back(0);
	if (doc.parseXML(Vec.data()) == -1)
	{
		write_log("Failed to parse XML: " + FileName);
		return;
	}
	auto pRoot = doc.first_node("config");
	if (pRoot)
	{
		/////////////// TODO: Lire le mode de fonctionnement du coffret (manuel ou automatique)
		/////////////// Doit etre dans CoffretPiscine.h, et doit etre lu ici dans ReadConfigFile
		SetMode(pRoot->GetValueInt("Mode"));
		auto pRelaisNode = pRoot->first_node("ListRelais");
		for (auto pNode = pRelaisNode->first_node("Relais"); pNode; pNode = pNode->next_sibling("Relais"))
		{
			std::string IPAdress = pNode->GetValue("IPAdress");
			std::string Description = pNode->GetValue("Id");
			int RegisterOn = std::stoi(pNode->GetValue("Register"));
			m_VectorEntry.push_back({ IPAdress, Description, RegisterOn });
			CProtocoleModbusRJE* pProtModbusRJE = nullptr;

			for (auto i = 0; i < m_VectorEntry.size(); i++)
			{
				if (m_map_IP.count(m_VectorEntry[i].IPAdress))
				{
					pProtModbusRJE = m_map_IP[m_VectorEntry[i].IPAdress];
				}
				else
				{
					pProtModbusRJE = new CProtocoleModbusRJE();
					m_map_IP[m_VectorEntry[i].IPAdress] = pProtModbusRJE;
					CTCP_Modbus* pTCP = new CTCP_Modbus(m_VectorEntry[i].IPAdress, true, pTCPLib);
					pProtModbusRJE->pTCPClient = pTCP;

				}

				Actions[m_VectorEntry[i].Description] = { pProtModbusRJE, m_VectorEntry[i].RegisterOn };
			}


			auto pEventsNode = pRoot->first_node("Events");
			if (pEventsNode)
			{
				for (auto pEventNode = pEventsNode->first_node("Event"); pEventNode; pEventNode = pEventNode->next_sibling("Event"))
				{
					std::string Id = pEventNode->GetValue("Relais");
					std::string StartTimeStr = pEventNode->GetValue("StartTime");
					auto StartTime = Convert<CTimeUtils::sUTCTime>(StartTimeStr);
					std::string FrequencyStr = pEventNode->GetValue("Frequency");
					auto Frequency = Convert<CTimeUtils::sDurationTime>(FrequencyStr);
					std::string DurationStr = pEventNode->GetValue("Duration");
					auto Duration = Convert<CTimeUtils::sDurationTime>(DurationStr);
					bool bFound = false;
					for (auto& entry : m_VectorEntry)
					{
						if (entry.Description == Id)
						{
							bFound = true;
							if (GetMode() == 2) AddEvent(Id, StartTime, Frequency, Duration);
							break;
						}
					}
					if (!bFound)
					{
						write_log("Event refers to unknown Relais: " + Id);
					}
				}
			}
		}
	}
	else
	{
		write_log("Failed to parse XML: " + FileName);
	}

}
void CCoffretPiscine::WriteConfigFile(std::string FileName, std::string Data)
{
	FILE* ptr = nullptr;
#ifdef _WIN32
	fopen_s(&ptr, FileName.c_str(), "wt+");
#else
	ptr = fopen(FileName.c_str(), "wt+");
#endif
	if (!ptr)
	{
		write_log("Failed to open file for writing: " + FileName);
		return;
	}
	// On recherche la fi du header du POST qui est \r\n\r\n et on ne garde que la partie après
	auto iStart = Data.find("\r\n\r\n");
	Data = Data.substr(iStart + 4);
	fwrite(Data.c_str(), 1, Data.size(), ptr);
	fclose(ptr);
}
