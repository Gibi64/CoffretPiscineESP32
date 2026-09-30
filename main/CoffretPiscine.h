// CoffretPiscine.h : fichier Include pour les fichiers Include système standard,
// ou les fichiers Include spécifiques aux projets.
#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <atomic>
#include "CLaunchThread.hpp"
#include "CTimeUtils.hpp"
#include "CTCP_Modbus.h"
class CProtocoleModbusRJE;
class CTCPLib;
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
	std::map < std::string, std::pair<CProtocoleModbusRJE*, int>> Actions;

private:
	std::vector<sAutomateEntry> m_VectorEntry;
	int m_Mode = 0; // 0 = On, 1 = Off, 2 = Programme
	CLaPoste m_LaPoste;


public:
	CCoffretPiscine();
	~CCoffretPiscine();
	int GetMode() const
	{
		return m_Mode;
	}
	void SetMode(int mode)
	{
		if (mode < 0) mode = 0;
		if (mode > 2) mode = 2;
		m_Mode = mode;
	}
	CLaPoste* GetLaPoste()
	{
		return &m_LaPoste;
	}

	void ReadConfigFile(std::string FileName, CTCPLib* pTCPLib);
	void WriteConfigFile(std::string FileName, std::string Data);
	void DoModeAction();
	std::vector<sAutomateEntry>* GetVectorEntry()
	{
		return &m_VectorEntry;
	}
	void AddEvent(std::string Id, CTimeUtils::sUTCTime StartTime, CTimeUtils::sDurationTime Frequency, CTimeUtils::sDurationTime Duration);
	class CProgEvents :public CKernelLaunchThread
	{
	private:
		enum { INIT_EVENT,WAITING_START_TIME, WAITING_END_TIME };
		int m_Event_State = WAITING_START_TIME;
		long long m_RemainingTime = 0;
		CTimeUtils::sDurationTime m_Duration;
		CTimeUtils::sDurationTime m_DeltaTime; // Frequence en DeltaTime
		CTimeUtils::sUTCTime m_StartTimeHourOfDay; // Jour? et Heure de départ
		std::string ActionId;
		CTimerThread* m_RemainingTimer = nullptr;
	public:
		std::string GetActionId() const
		{
			return ActionId;
		}
		int GetStartHour() const
		{
			return m_StartTimeHourOfDay.hour;
		}
		int GetStartMinute() const
		{
			return m_StartTimeHourOfDay.minute;
		}
		int GetStartSecond() const
		{
			return m_StartTimeHourOfDay.second;
		}
		int GetDurationHour() const
		{
			return m_Duration.hour;
		}
		int GetDurationMinute() const
		{
			return m_Duration.minute;
		}
		int GetDeltaTimeDay() const
		{
			return m_DeltaTime.day;
		}
		int GetDeltaTimeHour() const
		{
			return m_DeltaTime.hour;
		}
		int GetDeltaTimeMinute() const
		{
			return m_DeltaTime.minute;
		}
		CCoffretPiscine* m_pCoffret;
		CProgEvents(CLaPoste* pLaPoste, CCoffretPiscine* pCoffret, std::string Id, CTimeUtils::sUTCTime StartTimeHourOfDay, CTimeUtils::sDurationTime Frequency, CTimeUtils::sDurationTime Duration);
		

		virtual void Function () ;
		virtual void HandleMessage(CLaPoste::sMessage msg) override;
		virtual void Cleanup() override;
		
		void CorrectGenericStartToUTC();
		CTimeUtils::sUTCTime CorrectGenericStartToLocalTime();
		~CProgEvents()
		{

		}

	};
private:
	std::vector<CProgEvents*> m_VectorEvents;
public:
	void SendRelay(std::string Which, bool OnOff);
	std::vector< CProgEvents*>* GetVectorEvents()
	{
		return &m_VectorEvents;
	}	
};
