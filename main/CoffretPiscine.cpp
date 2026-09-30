#include "CoffretPiscine.h"
#include "CTCP_Modbus.h"
#include "CProtocoleModbusRJE.h"
#include <map>
#include "CLaunchThread.hpp"
#include "CTimeUtils.hpp"
#include <atomic>
#include <memory>
CCoffretPiscine::CCoffretPiscine()
{
}


CCoffretPiscine::~CCoffretPiscine()
{
	for (auto& kProt : m_map_IP)
	{

		delete kProt.second;
	}
	for (auto& kEvents : m_VectorEvents)
	{
		delete kEvents;
	}
}
void CCoffretPiscine::DoModeAction()
{
	switch (GetMode())
	{
	case 0:
		write_log("CoffretPiscine is in Manual mode");
		for (auto& entry : m_VectorEntry)
		{
			write_log("Relais: " + entry.Description + " IP: " + entry.IPAdress + " Register: " + std::to_string(entry.RegisterOn));
			SendRelay(entry.Description, true); // On -> démrre tous les relais en mode manuel
		}
		break;
	case 1:
		write_log("CoffretPiscine is in Off mode");
		for (auto& entry : m_VectorEntry)
		{
			write_log("Relais: " + entry.Description + " IP: " + entry.IPAdress + " Register: " + std::to_string(entry.RegisterOn));
			SendRelay(entry.Description, false); // Off -> arrete tous les relais en mode manuel
		}
		break;
	case 2:
		// On ne fait rien en mode programme, les events sont gérés par les threads CProgEvents
		// qui ont ete lus dans le fichier de configuration et qui sont en cours d'execution
		break;
	default:
		write_log("Unknown mode: " + std::to_string(GetMode()));
		break;
	}
}


void CCoffretPiscine::AddEvent(std::string Id, CTimeUtils::sUTCTime StartTime, CTimeUtils::sDurationTime Frequency, CTimeUtils::sDurationTime Duration)
{
	write_log("Creation d'un event");

	CProgEvents* Event = new CProgEvents(&m_LaPoste,this,Id, StartTime, Frequency, Duration);
	m_VectorEvents.push_back(Event);
}
void CCoffretPiscine::SendRelay(std::string Which, bool OnOff)
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

CTimeUtils::sUTCTime CCoffretPiscine::CProgEvents::CorrectGenericStartToLocalTime()
{
	CTimeUtils::sUTCTime out = m_StartTimeHourOfDay;
	// fonction utilisée pour les GET qui renvoient la date de départ en local time et non en UTC
	auto timeToday = CTimeUtils::SystemDateTime(CTimeUtils::GetMs());
	//       changer le local utilisateur en UTC
	// Date de changement d'heure d'été
	// 
	int DayOfMarch = CTimeUtils::GetLastSundayOfMonthInYear(3, timeToday.year);
	CTimeUtils::sUTCTime SummerChangingLocalTime = { timeToday.year,3,DayOfMarch,2,0,0,0 };
	// Date de changement d'heure d'hiver
	// 
	int DayOfOctober = CTimeUtils::GetLastSundayOfMonthInYear(10, timeToday.year);
	CTimeUtils::sUTCTime WinterChangingLocalTime = { timeToday.year,10,DayOfOctober,3,0,0,0 };
	// recherche du premeier element de date non nul en partant de year
	for (int iElement = 0; iElement < 7; iElement++)
	{
		if (out.GetIndexValue(iElement) != 0)
		{
			// si la dm_StartTimeHourOfDayate de depart est superieure a la date actuelle on garde
			// sinon on modifie l'element précédent + 1 
			// exemple si c'est l'heure demarage 8h et qu'il est 8h30 on decale au jour suivant.
						// transfomation UTC
			if (out > SummerChangingLocalTime && out < WinterChangingLocalTime)
			{
				out.hour += 2;
			}
			else
			{
				out.hour += 1;
			}
			if (out < timeToday)
				out.IndexPlusPlus(iElement - 1);
			break;
		}
		out.SetIndexValue(iElement, timeToday.GetIndexValue(iElement));
	}
	return out;
}

void CCoffretPiscine::CProgEvents::CorrectGenericStartToUTC()
{
	auto timeToday = CTimeUtils::SystemDateTime(CTimeUtils::GetMs());
	//       changer le local utilisateur en UTC
	// Date de changement d'heure d'été
	// 
	int DayOfMarch = CTimeUtils::GetLastSundayOfMonthInYear(3, timeToday.year);
	CTimeUtils::sUTCTime SummerChangingLocalTime = { timeToday.year,3,DayOfMarch,2,0,0,0 };
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
			// TODO : je pense que c'est indesMoinsMoins(iElement - 1) et pas IndexPlusPlus
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

CCoffretPiscine::CProgEvents::CProgEvents(CLaPoste* pLaPoste, CCoffretPiscine* pCoffret, std::string Id, CTimeUtils::sUTCTime StartTimeHourOfDay, CTimeUtils::sDurationTime Frequency, CTimeUtils::sDurationTime Duration)
	: CKernelLaunchThread(pLaPoste, nullptr), m_pCoffret(pCoffret)
{

	m_StartTimeHourOfDay = StartTimeHourOfDay;
	m_DeltaTime = Frequency;
	m_Duration = Duration;
	ActionId = Id;
	CorrectGenericStartToUTC();
	m_RemainingTimer = nullptr;
	m_Event_State = INIT_EVENT;
	auto timeToday = CTimeUtils::SystemDateTime(CTimeUtils::GetMs());
	m_StartTimeHourOfDay.year = timeToday.year;
	if (m_StartTimeHourOfDay.month == 0) m_StartTimeHourOfDay.month = timeToday.month;
	if(m_StartTimeHourOfDay.day == 0) m_StartTimeHourOfDay.day = timeToday.day;
	if (m_StartTimeHourOfDay < timeToday)
	{
		m_StartTimeHourOfDay.day += 1;
	}

	m_RemainingTime = m_StartTimeHourOfDay.ToMs() - CTimeUtils::GetMs();
	#ifdef _ESP32
	write_log("Lancement de la thread Event - Remaining Time :" + CTimeUtils::FormatDeltaTime(m_RemainingTime));
	#else
	write_log("Lancement de la thread Event - Remaining Time : " + CTimeUtils::FormatDeltaTime(m_RemainingTime) + " ID = " + std::to_string(GetCurrentThreadId()));
	#endif
	// tout est pret pour lancer la thread
	Start();
}
void CCoffretPiscine::CProgEvents::Function()
{
	CLaPoste::sMessage msg;
	while (GetLaPoste()->GetMessage(this, &msg))
	{
		HandleMessage(msg);

		// On laisse une respiration pour un éventuel CLOSE_THREAD
		CTimeUtils::CPUSleep(1);
	}
	if (GetState() !=THREAD_RUNNING)
		return;

	// Sinon → on exécute l’action
	switch (m_Event_State)
	{
	case INIT_EVENT:
		// On lance le premier timer pour attendre le démarrage de l'action
		m_RemainingTimer = new CTimerThread(GetLaPoste(), static_cast<CKernelLaunchThread*>(this), m_RemainingTime);
		m_Event_State = WAITING_START_TIME;
		#ifdef _ESP32
		write_log("Event INIT - Remaining Time :" + CTimeUtils::FormatDeltaTime(m_RemainingTime));
		#else
		write_log("Event INIT - Remaining Time :" + CTimeUtils::FormatDeltaTime(m_RemainingTime) + " ID = " + std::to_string(GetCurrentThreadId()));
		#endif
		break;
	case WAITING_START_TIME:
		// Si le timer est toujours present , on ne fait rien
		if (m_RemainingTimer)
			return;
		// Le <timer est termin� ? on ex�cute l�action et on lance le timer pour la dur�e de l�action
		#ifdef _ESP32
		write_log("Event START - Remaining Time :" + CTimeUtils::FormatDeltaTime(m_RemainingTime));
		#else
		write_log("Event START - Remaining Time :" + CTimeUtils::FormatDeltaTime(m_RemainingTime) + " ID = " + std::to_string(GetCurrentThreadId()));
		#endif
		m_pCoffret->SendRelay(ActionId, 1);
		m_Event_State = WAITING_END_TIME;
		m_RemainingTime = m_Duration.ToMs();
		m_RemainingTimer = new CTimerThread(GetLaPoste(), static_cast<CKernelLaunchThread*>(this), m_RemainingTime);
		break;

	case WAITING_END_TIME:
		// Si le timer est toujours present , on ne fait rien
		if (m_RemainingTimer)
			return;
		#ifdef _ESP32
		write_log("Event END - Remaining Time :" + CTimeUtils::FormatDeltaTime(m_RemainingTime));
		#else
		write_log("Event END - Remaining Time :" + CTimeUtils::FormatDeltaTime(m_RemainingTime) + " ID = " + std::to_string(GetCurrentThreadId()));
		#endif
		m_pCoffret->SendRelay(ActionId, 0);
		m_Event_State = INIT_EVENT;
		m_RemainingTime = m_DeltaTime.ToMs();
		break;
	}
}
void CCoffretPiscine::CProgEvents::HandleMessage(CLaPoste::sMessage msg)
{
	if (msg.Command == CLOSE_THREAD)
	{
		SetState(THREAD_ABORTED);
		if (m_RemainingTimer)
		{
			SendMessage(m_RemainingTimer, CLOSE_THREAD, 0);
		}
			
	}
	else if (msg.Command == THREAD_DESTROYED && msg.pParam == m_RemainingTimer)
	{ 
		// On teste ici si le m_RemainingTimer est toujours la map du Kernel
		// si c'est le cas il faut attendre qu'il diparaisse pour mettre le pointeur à 0
		m_RemainingTimer = nullptr;
		m_RemainingTime /= 2; // Inutilisé - la dichotomie n'est plus implementée
	}
}
void CCoffretPiscine::CProgEvents::Cleanup()
{

	switch (GetState())
	{
	case THREAD_ENDED:
		write_log("Fin de la thread Event -Adresse :" + std::to_string((uintptr_t)this));
		if (m_RemainingTimer)
			SendMessage(m_RemainingTimer, CLOSE_THREAD, 0);

		if (GetParent())
			SendMessage(GetParent(), THREAD_ENDED, static_cast<CKernelLaunchThread*>(this));

		SendMessage(nullptr, THREAD_ENDED, static_cast<CKernelLaunchThread*>(this));


		break;
	case THREAD_ABORTED:
		write_log(" thread Event Aborted - Address :" + std::to_string((uintptr_t)this));
		if (m_RemainingTimer)
			SendMessage(m_RemainingTimer, CLOSE_THREAD, 0);

		SendMessage(nullptr, THREAD_ENDED, static_cast<CKernelLaunchThread*>(this));

	}
}
