#include "CoffretPiscine.h"
#include "CTCP_Modbus.h"
#include "CServerThread.hpp"
#include "InitLog.hpp"
/////////////////////////// Server Wifi ESP32
#ifdef _ESP32
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
/////////////////////// time utc initialisation
#include "esp_sntp.h"

#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(_WINDOWS)
int main()
#elif defined(_ESP32)
void init_time()
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "192.168.1.1");   // La box comme serveur NTP
    sntp_init();
}


bool wait_for_time()
{
    time_t now = 0;
    struct tm timeinfo = {};

    for (int i = 0; i < 20; i++)
    {
        time(&now);
        localtime_r(&now, &timeinfo);

        if (timeinfo.tm_year > (1970 - 1900))
        {
            return true; // L'heure est valide
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    return false;
}

void InitWiFi_STA_FixedIP()
{
	// --- NVS obligatoire ---
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	// --- Interface STA ---
	esp_netif_t* netif = esp_netif_create_default_wifi_sta();
	esp_netif_set_hostname(netif, "ESP32_PISCINE");

	// --- Configuration IP fixe ---
	esp_netif_ip_info_t ip_info;
	esp_netif_str_to_ip4("192.168.1.95", &ip_info.ip);
	esp_netif_str_to_ip4("192.168.1.1",  &ip_info.gw);
	esp_netif_str_to_ip4("255.255.255.0", &ip_info.netmask);

	ESP_ERROR_CHECK(esp_netif_dhcpc_stop(netif));
	ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ip_info));

	// --- Init WiFi ---
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	// --- Configuration STA ---
	wifi_config_t wifi_config = {};
	strcpy((char*)wifi_config.sta.ssid, "Box_salon_uzos_2G");
	strcpy((char*)wifi_config.sta.password, "arpege64");

	wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
	wifi_config.sta.pmf_cfg.capable = true;
	wifi_config.sta.pmf_cfg.required = false;

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_connect());

	write_log("WiFi STA actif\n");
	write_log("SSID : " + std::string((char*)wifi_config.sta.ssid));
	write_log("IP fixe : 192.168.1.95\n");
	write_log("Gateway : 192.168.1.1\n");
}
void InitSPIFFS()
{
	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = "spiffs",
		.max_files = 5,
		.format_if_mount_failed = true
	};

	ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

	size_t total = 0, used = 0;
	esp_spiffs_info(conf.partition_label, &total, &used);

	write_log("SPIFFS monte. Taille totale="+std::to_string(total)+", utilise="+std::to_string(used)+"\n");
}

extern "C" void app_main(void)
#endif
{
#define ON 1
#define OFF 0
	std::unique_ptr<CTCPLib> pTCPLIB = std::make_unique<CTCPLib>();
#if defined(_WINDOWS)
	std::string szFullPath = "c:\\Local\\Softwares\\CoffretPiscine\\config\\config.xml";
#elif defined(_ESP32)
	std::string szFullPath = "/spiffs/config.xml";
	InitSPIFFS();
	InitWiFi_STA_FixedIP();
// Initialisation de l'heure
	init_time();
	if (!wait_for_time())
	{
		write_log("Heure non valide, timers en attente");
		return;
	}
#endif

	std::unique_ptr<CCoffretPiscine> pCoffret = std::make_unique<CCoffretPiscine>();

	pCoffret->ReadConfigFile(szFullPath, pTCPLIB.get());

	pCoffret->DoModeAction();
	CServerThread Server(pCoffret.get(), pTCPLIB.get());


	for (;;)
	{
		CTimeUtils::CPUSleep(2);
	}
	
#if defined(_WINDOWS)
	return 0;
#endif
}
