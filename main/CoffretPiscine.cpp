// CoffretPiscine.cpp : définit le point d'entrée de l'application.
//

#include "CoffretPiscine.h"
#include "CTCP_Modbus.h"
#include "CProtocoleModbusRJE.h"
#include <map>
#include "CLaunchThread.hpp"
#include "CTimeUtils.hpp"
#include <atomic>
using namespace std;

class CCoffretPiscine
{

public:
	struct sAutomateEntry
	{
		std::string IPAdress;
		std::string Description;
		int RegisterOn;
	};
	std::map<std::string, CProtocoleModbusRJE*> m_map_IP;
	std::map < std::string, pair<CProtocoleModbusRJE*, int>> Actions;

private :
	std::vector<sAutomateEntry> m_VectorEntry;
public:
	CCoffretPiscine(CTCPLib * pTCPLib,std::vector<sAutomateEntry> VectorEntry	)
	{
		m_VectorEntry = VectorEntry;
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
				CTCP_Modbus* pTCP = new CTCP_Modbus(m_VectorEntry[i].IPAdress,true,pTCPLib);
				pProtModbusRJE->pTCPClient = pTCP;

			}
			Actions[m_VectorEntry[i].Description] = { pProtModbusRJE, m_VectorEntry[i].RegisterOn };
		}
	}
	~CCoffretPiscine()
	{
		for (auto& kProt : m_map_IP)
		{
			
			delete kProt.second;
		}
		for (auto& kEvents : m_VectorEvents)
		{
			kEvents->bStopThread = true;
			delete kEvents;
		}
	}
	void AddEvent(std::string Id, CTimeUtils::sUTCTime StartTime, CTimeUtils::sDurationTime Frequency, CTimeUtils::sDurationTime Duration)
	{
		CProgEvents* Event = new CProgEvents(this,Id, StartTime, Frequency, Duration);
		m_VectorEvents.push_back(Event);
	}
	class CProgEvents
	{
	private:
		CTimeUtils::sDurationTime m_Duration;
		CTimeUtils::sDurationTime m_DeltaTime; // Frequence en DeltaTime
		CTimeUtils::sUTCTime m_StartTimeHourOfDay; // Jour? et Heure de départ
		std::string ActionId;
		unique_ptr<CLaunchThread> m_thread;
		void CorrectGenericStart()
		{
			auto timeToday = CTimeUtils::SystemDateTime(CTimeUtils::GetMs());
			//       changer le local utilisateur en UTC
			// Date de changement d'heure d'été
			// 
			int DayOfMarch = CTimeUtils::GetLastSundayOfMonthInYear(3, timeToday.year);
			CTimeUtils::sUTCTime SummerChangingLocalTime = {timeToday.year,3,DayOfMarch,2,0,0,0 };
			// Date de changement d'heure d'hiver
			// 

			int DayOfOctober = CTimeUtils::GetLastSundayOfMonthInYear(10, timeToday.year);
			CTimeUtils::sUTCTime WinterChangingLocalTime = { timeToday.year,10,DayOfOctober,3,0,0,0 };

			// recherche du premeier element de date non nul en partant de year
			for (int iElement = 0; iElement < 7; iElement++)
			{
				if (m_StartTimeHourOfDay.GetIndexValue(iElement) != 0)
				{
					// si la dm_StartTimeHourOfDayate de depart est superieure a la date actuelle on garde
					// sinon on modifie l'element précédent + 1 
					// exemple si c'est l'heure demarage 8h et qu'il est 8h30 on decale au jour suivant.
								// transfomation UTC
					if (m_StartTimeHourOfDay > SummerChangingLocalTime && m_StartTimeHourOfDay < WinterChangingLocalTime)
					{
						m_StartTimeHourOfDay.hour -= 2;
					}
					else
					{
						m_StartTimeHourOfDay.hour -= 1;

					}

					if (m_StartTimeHourOfDay < timeToday)
						m_StartTimeHourOfDay.IndexPlusPlus(iElement - 1);
					break;
				}
				m_StartTimeHourOfDay.SetIndexValue(iElement, timeToday.GetIndexValue(iElement));
			}
		}
	public:
		atomic<bool> bStopThread{ false };
		struct sThreadArgs
		{
			CProgEvents* pProg;
			CCoffretPiscine* pCoffret;
		};
		CLaunchThread* GetThread() const
		{
			return m_thread.get();
		}
		CProgEvents(CCoffretPiscine*pCoffret,std::string Id,CTimeUtils::sUTCTime StartTimeHourOfDay, CTimeUtils::sDurationTime Frequency, CTimeUtils::sDurationTime Duration)
		{
			
			m_StartTimeHourOfDay = StartTimeHourOfDay;
			m_DeltaTime = Frequency;
			m_Duration = Duration;
			ActionId = Id;
			CorrectGenericStart();
			// tout est pret pour lancer la thread
			sThreadArgs* pArgs = new sThreadArgs{ this, pCoffret };
			m_thread = make_unique<CLaunchThread>(&CProgEvents::LoopTimer, pArgs);
		}
		static void LoopTimer(void* pArgs)
		{
			enum {WAITING_START_TIME,WAITING_END_TIME};
			int iState = WAITING_START_TIME;

			sThreadArgs* args = static_cast<sThreadArgs*>(pArgs);
			CProgEvents* pProg = args->pProg;
			CCoffretPiscine* pCoffret = args->pCoffret;

			auto RemainingTime = pProg->m_StartTimeHourOfDay.ToMs() - CTimeUtils::GetMs();
			write_log("Lancement de la thread - Remaining Time :" + std::to_string(RemainingTime));
			while (!pProg->bStopThread)
			{
				for (;;)
				{
					if (RemainingTime < 10) break;
					CTimeUtils::CPUSleep(RemainingTime / 2);
					RemainingTime /= 2;
					write_log("Apres Sleep - Remaining Time :" + std::to_string(RemainingTime));

				}
				// Do the action 
				switch (iState)
				{
				case WAITING_START_TIME:
					pCoffret->SendRelay(pProg->ActionId,1);
					iState = WAITING_END_TIME;
					RemainingTime = pProg->m_Duration.ToMs();
					write_log("Ouverture Relais - Remaining Time : " + std::to_string(RemainingTime));

					break;
				case WAITING_END_TIME:
					pCoffret->SendRelay(pProg->ActionId, 0);
					iState = WAITING_START_TIME;
					RemainingTime = pProg->m_DeltaTime.ToMs();
					write_log("Fermeture Relais - Remaining Time : " + std::to_string(RemainingTime));

					break;

				}
			}

		}

	};
private:
	std::vector<CProgEvents *> m_VectorEvents;
public:
	void SendRelay(std::string Which, bool OnOff)
	{
		//         Dans ce ptotocole ce n'est pas un coil mais un entier qui est écrit dans le registre 2 de l'adresse 1
		// la valeur dépend du relais qui est commandé et de l'état On ou Off
		// 	 Pour le relais 1 : 256 pour Off et 257 pour On
		// 	 Pour le relais 2 : 512 pour Off et 513 pour On
		// 
		// Le registre est toujours le 2 et l'adresse est toujours 1
		// protocole spécifique à la carte relais Dynan
		auto it = Actions.find(Which);

		if (it != Actions.end())
		{
			auto pProt = it->second.first;
			auto RelayNumber = it->second.second;
			int Value = RelayNumber + (OnOff ? 1 : 0);
			pProt->SetRegister(2);
			pProt->SetValue(reinterpret_cast<float&>(Value));
			pProt->SetTypeOfValue(CProtocoleModbusRJE::TYPE_INTEGER);
			pProt->EnvoieCommande();
		}
		else
		{
			write_log("Action inconnue : " + Which);
		}
	}

	
};
#if defined(_WINDOWS)
int main()
#elif defined(_ESP32)
extern "C" void app_main(void)
#endif
{
#define ON 1
#define OFF 0
	unique_ptr<CTCPLib> pTCPLIB = make_unique<CTCPLib>();
	std::vector< CCoffretPiscine::sAutomateEntry> VectorOfActions;
	VectorOfActions.push_back({ "127.0.0.1:24","Pompe",256 });
	VectorOfActions.push_back({ "127.0.0.1:24","Electrolyseur",512 });

	unique_ptr<CCoffretPiscine> pCoffret = make_unique<CCoffretPiscine>(pTCPLIB.get(), VectorOfActions);
	pCoffret->SendRelay("Pompe", ON);

	write_log("Creation d'un event");
	pCoffret->AddEvent( "Pompe", {0, 0, 0, 15, 45,0, 0}, {0,0,1,0,0,0,0}, {0,0,0,0,3,0,0});
	for (;;)
	{
		CTimeUtils::CPUSleep(2);
	}
#if defined(_WINDOWS)
	return 0;
#endif
}
