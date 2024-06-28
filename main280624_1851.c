/*     

Challenge -once wifi disconnected, it does not sense that and change status
s2pinit if added will set a test output , normally keep it off
UART2 not yet tested/not working
wifi/server disconnection to be tested


27062024
1.Ensure that all query commands are replied with username as date in same as received in last command.
2.UART query replies with username and date as in TCP query.
3.In UART setting, user name and date is not required, but save user name as “LOCAL” and date as localdate variable  for example



1. Retry in case of wifi error
2. Retry in case of server disconnection
3. Send data to UART2 for debugging
4. Add MQTT
5. Add logs

1445 - started working. picked up simple task
170624T1
when pulses received - recreate again. added in GPIO_read_n_act
checked and demoed to siddhi

130624T1
1. Start Smartconfig if Jumper2 is pressed else continue as normal
2. When ssid and password detected then save in NVRAM
3. Rest exmaple code copied
4. Able to scan and save ESP TOUCH SSID and PASSWORD and save in memory
4. Trying to connect ssid3

120624T1
added ESP-TOUCH
to be called only in Test Mode
SSID 1/2 were already there. Now 3 also added


070624T1
added following commands to uart
*CC# , *FW?# ,*V: , *SL: , *CA?# , *CA: ,*FOTA#

060624
UART2 some correction made
UART2 Working
FOTA Working

270524
add erase function when BOOT switch pressed for 2 seconds
boot is IO0




230524
INH saved in memory
*MAC: when KP Server
*MAC, when GVC Server

200521
adding commands to set and read INH status
*INH:0# or *INH:1# 
reply *INH-DONE,x#
save value in INHPOutputValue
ESP IO 14 is CINHO (reverse polarity)
demo done
while doing fota also send FOTA URL to server 
display*FOTA-OVER# at end



*INH?#
reply *INH,0# or *INH,1#
display from INHInputValue
IO23 is INHInput

160524T1
FOTA commands * and # added

110524T1
copied WiFi code from example 
ESP_RETRY_GAP added
When socket connected send *WiFi X# where x is wifi number

090525T1

080524T4

0805T3
retry wifiif disconnected, not working
saved 

0805T2
if wifi disconnected - do not try socket connection

0805T1

added * and # on SSID commands

0705T3
working and socket connected

050424T1
new command added to save Port in case of Jumper
*SP:xxxx# and save 
when using jumper, use GVC server and port as set using SP command
else use server and port as set by SIP command

RTOS Frequency changes from 100Hz to 200 Hz
some bugs removed



040524T3
ignore pulses of width less than 30 msec

040524T2
Sense pulse width of some minimum value - so if CA is set as 50 - ignore pulse of width 25
checked readNActDelay. Added led blinking and one ESP_LOGI  for testing 


040524T1
have variable size pulses - done

020524T1


010524T2
small bug of CA values correctd
capture multiple pulses
Tested by Prashant


010524T1
adding memorising CA and totals parameters
working 


300424T2
send reply to server when input pulse sensed -worked
send totals is now two command *TV and *TC
*TC for cash
*TV for V commands
(6603 V command becomne Coins for 6666)


300424T1
Added *CA:pulsewidth:PolarityCommand)
polarity 0 means active low output
polarity 1 means actibe high output
working
Input pulses being senses



210424
When V command is received pulse L1
when coin are sensed pulse L2
when reply is sent pulse L3

output pins configured
CH 1/ CH5 tested
CH 3 pulses okay but low voltage
CH 7 pulses okay but low voltage
CH 4 okay
CH 2 okay



190424
*V:TID:Plan:Pulses#
skip duplicate TID
*TC?# added


180424B
Added RST command
180424
blink one LED when packet received
Blick second LED when HBT Sent
*HBT,MACID#
160424C
MAC Address is actual
addimg *FW?#
160424B Led command being added, Jumper sense added
*SL:x:y# added, set light x is 1/2/3 y is 0/1
160424A - SIP command tested
090324A - *GP1:7# working okay
070324B - decode x and y from *GP:x:y# and then generate pulses
070324A - get command from server *GP:x:y# where x is pin and y is number of pulses
change *Tx:y# to *TP:x:y#
060324C -count number of pulses within two second and send to server as *Px:y# where x is pin number and y is pulses
060324B -count number of pulses within one second
060324A - send pulse sense to socket send *Px# where x is number
060324 - check led styles
led blinks 1/2/3/4 times every 2 seconds as per status. This makes it easy to detect status. 
fast/slow blink does not work 
input sense working well and displaying status when input pin changes status

020324D -socket does not work with vending-iot.com but works with 163.232.177.23
able to connect and send MAC address
Checked with nc - l 6666 command
also data sent from server is received and displayed on ESP_LOGI

020324D - working 4094 and 3 LEDs test mode 020324D
020324C - working 4094 test mode 020324C

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"   
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"
#include "driver/uart.h"
#include "esp_netif.h"
#include "rom/ets_sys.h"
#include "esp_smartconfig.h"
#include <sys/socket.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>




int INHInputValue = 0;
int INHOutputValue = 0;
int PreviousINHValue = 0;
#define INHIBITLevel 1
char WIFI_SSID_1[64];
char WIFI_PASS_1[64];
char WIFI_SSID_2[64];
char WIFI_PASS_2[64];
char WIFI_SSID_3[64];
char WIFI_PASS_3[64];

char server_ip_addr[100];
char ipstr[100]; // host mapped

char MAC_ADDRESS_ESP[40];
char FOTA_URL[356];
int16_t server_port;
int jumperPort;
int16_t caValue;
int16_t cashValue;
int sock = -1;

int sp_port;
#define ESP_MAXIMUM_RETRY       2
#define ESP_RETRY_GAP           2000
#define Production          1
#define NVS_INH_KEY           "INH"
#define NVS_SSID_1_KEY        "SSID1"
#define NVS_PASS_1_KEY        "PASS1"
#define NVS_SSID_2_KEY        "SSID2"
#define NVS_PASS_2_KEY        "PASS2"
#define NVS_SSID_3_KEY        "SSID3"
#define NVS_PASS_3_KEY        "PASS3"

#define NVS_SERVER_IP_KEY     "SERVER"
#define NVS_SERVER_PORT_KEY   "PORT"
#define NVS_SERVER_PORT_KEY_JUMPER "JUMPERPORT" 
#define NVS_OTA_URL_KEY   "OTA_URL"
#define NVS_CA_KEY          "CA"
#define NVS_CASHTOTAL_KEY   "CASHTOTAL"
#define NVS_VENDTOTAL_KEY   "VENDTOTAL"
#define NVS_CASH1_KEY   "CASH1"
#define NVS_CASH2_KEY   "CASH2"
#define NVS_CASH3_KEY   "CASH3"
#define NVS_CASH4_KEY   "CASH4"
#define NVS_CASH5_KEY   "CASH5"
#define NVS_CASH6_KEY   "CASH6"
#define NVS_CASH7_KEY   "CASH7"

#define DEFAULT_SSID1  "GVCSYS1"
#define DEFAULT_PASS1  "GVC3065V"
#define DEFAULT_SSID2  "GVCSYS2"
#define DEFAULT_PASS2  "GVC3065V"
#define DEFAULT_SSID3  "GVCSYS3"
#define DEFAULT_PASS3  "GVC3065V"


#define NVS_SIP_USERNAME     "USERNAME_SIP"
#define NVS_SIP_DATETIME   "DATETIME_SIP"

#define NVS_CA_USERNAME     "USERNAME_CA"
#define NVS_CA_DATETIME   "DATETIME_CA"

#define NVS_CC_USERNAME     "USERNAME_CC"
#define NVS_CC_DATETIME   "DATETIME_CC"

#define NVS_URL_USERNAME     "USERNAME_URL"
#define NVS_URL_DATETIME   "DATETIME_URL"

#define NVS_FOTA_USERNAME     "USERNAME_FOTA"
#define NVS_FOTA_DATETIME   "DATETIME_FOTA"

#define NVS_RST_USERNAME     "USERNAME_RST"
#define NVS_RST_DATETIME   "DATETIME_RST"

#define NVS_SS_USERNAME     "USERNAME_SS"
#define NVS_SS_DATETIME   "DATETIME_SS"

#define NVS_PW_USERNAME     "USERNAME_PW"
#define NVS_PW_DATETIME   "DATETIME_PW"

#define NVS_SS1_USERNAME     "USERNAME_SS1"
#define NVS_SS1_DATETIME   "DATETIME_SS1"

#define NVS_PW1_USERNAME     "USERNAME_PW1"
#define NVS_PW1_DATETIME   "DATETIME_PW1"

#define NVS_INH_USERNAME     "USERNAME_INH"
#define NVS_INH_DATETIME   "DATETIME_INH"

#define NVS_SP_USERNAME     "USERNAME_SP"
#define NVS_SP_DATETIME   "DATETIME_SP"

#define DEFAULT_SERVER_IP_ADDR_TRY "gvc.co.in"
#define DEFAULT_SERVER_IP_ADDR "gvc.co.in"
#define DEFAULT_SERVER_PORT    6666
#define DEFAULT_FOTA_URL  "http://gvc.co.in/esp/firmware.bin"
#define FWVersion "*GVCSYS-27JUNE24T5#"
#define HBTDelay    300000
#define LEDR    13
#define LEDG    12

#define JUMPER  15
#define JUMPER2  18


#define ErasePin 0
#define ICH1    33
#define ICH2    32
#define ICH3    35
#define ICH4    34
#define ICH5    26
#define ICH6    27
#define ICH7    25
#define INH     23

#define STRB    19
#define CLK     22
#define DAT    21

#define CINHO   14
#define CINHI   23

#define L1      2
#define L2      5
#define L3      4
#define LedHBT  L1
#define LedTCP  L2

#define SDA     21
#define SCL     22

#define MKM_IC_UART UART_NUM_2
#define MKM_IC_UART_TX 17
#define MKM_IC_UART_RX 16


// values used in erase pin
bool ErasePinStatus,LastErasePinStatus;
int ErasePinDebounce;

// valuses used in CA command
int numValue=0;
int polarity=0;
int pulseWitdh=0;
int SignalPolarity=0;
char dateTime[100];
char userName[100];

char SIPdateTime[100];
char SIPuserName[100];

char CAdateTime[100];
char CAuserName[100];

char CCdateTime[100];
char CCuserName[100];

char URLdateTime[100];
char URLuserName[100];

char FOTAdateTime[100];
char FOTAuserName[100];

char RSTdateTime[100];
char RSTuserName[100];

char SSdateTime[100];
char SSuserName[100];

char PWdateTime[100];
char PWuserName[100];

char SS1dateTime[100];
char SS1userName[100];

char PW1dateTime[100];
char PW1userName[100];

char INHdateTime[100];
char INHuserName[100];

char SPdateTime[100];
char SPuserName[100];






unsigned char  SwitchStatus,PreviousSwitchStatus,DebounceCount;
int16_t x;
unsigned char Test4094Count;

// used in Check input sense pins
uint16_t Counter,InputPin,LastValue,LastInputPin;


uint32_t current_interval = 0;
uint32_t numberOfPulses = 0;
uint16_t TotalPulses = 0; // total received pulses for coin input
uint16_t PulseStoppedDelay = 0; // time delay when it is assumed that pulses end
// uint16_t PulseCount =0; // for received pulses for coin input
// uint16_t PulseTimeOut = 0;  // received pulses time out time in ticks* delay

uint32_t ticks_100 =0;;
int LastTID = 0;
int TID = 0; // Transaction ID
int pin = 0; // output pin for Generating Pulses
int pulses = 0; // number of pulses for generating pulses on output pin
int edges = 0; // number of edges (pulses * 2) for generating pulses on output pin
int Totals[7];
int CashTotals[7];
int ledpin = 0;
int ledstatus = 0;
int blinkLEDNumber = 0;

#define EX_UART_NUM UART_NUM_2
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart0_queue;


#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif


typedef enum LED_STATES{
    STANDBY_LED,
    SEARCH_FOR_WIFI,
    SEARCH_FOR_ESPTOUCH,
    WIFI_FOUND_NO_INTERNET,
    WIFI_AND_INTERNET_NO_SERVER,
    EVERYTHING_OK_LED,
    OTA_IN_PROGRESS
}Led_State_t;

typedef enum TCPIP_SOCKET_STATE{
    SOCKET_CONNECTED,
    SOCKET_DISCONNECTED
}TCPIP_Socket_State;

Led_State_t led_state = STANDBY_LED;
TCPIP_Socket_State socket_state;
bool connected_to_wifi_and_internet = false;
int WiFiNumber = 0;


int tcp_sock = -1;
void set_led_state(Led_State_t st);
void uart_write_string(const char * str);
void uart_write_string_ln(const char * str);
uint32_t millis(void);
void http_fota( void ) ; 
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const int CONNECTED_BIT = BIT2;
static const int ESPTOUCH_DONE_BIT = BIT3;


static const char *TAG = "main";

static int s_retry_num = 0;
int rx_event_pending = 0;
int tx_event_pending = 0;


void Out4094 (unsigned char);

void WiFiConnection (void);

#define LED_ACTIVE_HIGH

nvs_handle_t utils_nvs_handle;

void utils_nvs_set_str(const char * key , const char * val){
    if(utils_nvs_handle){
        nvs_set_str(utils_nvs_handle, key, val);
        nvs_commit(utils_nvs_handle);
    }
}


void led_set_level(gpio_num_t gpio_num, int state){
    #ifdef LED_ACTIVE_HIGH
        gpio_set_level(gpio_num, state);
    #else
        gpio_set_level(gpio_num, !state);
    #endif
}



static int FirstWiFiConnection = 0;

static void smartconfig_example_task(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
    while (1) {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}




static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
         if (gpio_get_level(JUMPER2) == 0)
         {
             xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 6, NULL);
            set_led_state(SEARCH_FOR_ESPTOUCH);
            ESP_LOGI(TAG,"*Start Looking for ESP TOUCH#");
         }
         else
             esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
            ESP_LOGI(TAG, "*WiFi Connected %d#",WiFiNumber);
            s_retry_num = 0;
            FirstWiFiConnection = 1;
            connected_to_wifi_and_internet = true;
 
    }    
    
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        connected_to_wifi_and_internet = false;
        set_led_state(SEARCH_FOR_WIFI);
        ESP_LOGI(TAG,"*Connect WiFi after disconnection#");
        vTaskDelay(ESP_RETRY_GAP);
        if (s_retry_num < ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "*retry to connect to the AP  %d#",s_retry_num);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI(TAG, "*WiFi failed bit set %d#",WiFiNumber);
            if ((FirstWiFiConnection == 1) || (WiFiNumber == 3))
            {
                ESP_LOGI(TAG, "*restarting after 2 seconds#");
                vTaskDelay(2000/portTICK_PERIOD_MS);
                esp_restart();
            }
    }
        ESP_LOGI(TAG,"*connect to the AP fail#");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "*got ip:*" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "*Scan done#");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "*Found channel#");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "*Got SSID and password#");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        char ssid[33] = { 0 };
        char password[65] = { 0 };
        uint8_t rvd_data[33] = { 0 };

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(TAG, "*SSID3:%s#", ssid);
        ESP_LOGI(TAG, "*PASSWORD3:%s#", password);
        // memorise in NV RAM
        utils_nvs_set_str(NVS_SSID_3_KEY, ssid);
        utils_nvs_set_str(NVS_PASS_3_KEY, password);

        if (evt->type == SC_TYPE_ESPTOUCH_V2) {
            ESP_ERROR_CHECK( esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)) );
            ESP_LOGI(TAG, "*RVD_DATA:");
            for (int i=0; i<33; i++) {
                printf("%02x ", rvd_data[i]);
            }
            printf("\n#");
        }

        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        esp_wifi_connect();
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}


bool connect_to_wifi(char * ssid, char * psk){
    bool wifi_connected = false;
    esp_wifi_stop();
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };

    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, psk);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "*connected to ap ssid:%s password:%s \r\n#", ssid, psk);
        wifi_connected = true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "*failed to connect to ssid:%s, password:%s \r\n#", ssid, psk);
    } else {
        ESP_LOGE(TAG, "*unexpected event#");
    }

    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    ESP_LOGI(TAG,"*returning from connecting 2 wifi#");
    return wifi_connected;
}
 

void utils_nvs_init(){
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &utils_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "NVS Open Done\n");
    }
}


esp_err_t utils_nvs_get_str(const char * key , char * val, size_t max_length){
    esp_err_t ret = ESP_FAIL;
    if(utils_nvs_handle){
        ret = nvs_get_str(utils_nvs_handle, key, val, &max_length);
    }
    return ret;
}

esp_err_t utils_nvs_get_int(const char * key , int16_t * val){
    esp_err_t ret = ESP_FAIL;
    if(utils_nvs_handle){
        ret = nvs_get_i16(utils_nvs_handle, key, val);
    }
    return ret;
}

void utils_nvs_erase_all(){
    if(utils_nvs_handle){
        nvs_erase_all(utils_nvs_handle);
    }
}

void utils_nvs_erase_key(const char * key){
    if(utils_nvs_handle){
        nvs_erase_key(utils_nvs_handle, key);
    }
}

void utils_nvs_set_int(const char * key , int16_t val){
    if(utils_nvs_handle){
        nvs_set_i16(utils_nvs_handle, key, val);
    }
}

void load_settings_nvs(){
    
    ESP_LOGI(TAG, "*NVS Reading Started#");
    if (utils_nvs_get_str(NVS_CA_USERNAME, CAuserName, sizeof(CAuserName)) == ESP_OK) {
    if (strcmp(CAuserName, "Siddhi Gvc") == 0) {
        utils_nvs_get_str(NVS_CA_DATETIME, CAdateTime, sizeof(CAdateTime));
        ESP_LOGI(TAG, "*CA values %s|%s#", CAuserName, CAdateTime);
    }
    }

    if(utils_nvs_get_str(NVS_SSID_1_KEY, WIFI_SSID_1, 64) == ESP_OK){
        utils_nvs_get_str(NVS_PASS_1_KEY, WIFI_PASS_1, 64);
        ESP_LOGI(TAG, "*WIFI 1 Credentials %s|%s#", WIFI_SSID_1, WIFI_PASS_1);
    }else{
        ESP_LOGI(TAG, "*Could not get Wifi 1 Credentials From NVS#");
        strcpy(WIFI_SSID_1, DEFAULT_SSID1);
        strcpy(WIFI_PASS_1, DEFAULT_PASS1);
        ESP_LOGI(TAG, "*Default WIFI 1 Credentials %s|%s#", WIFI_SSID_1, WIFI_PASS_1);
        utils_nvs_set_str(NVS_SSID_1_KEY, WIFI_SSID_1);
        utils_nvs_set_str(NVS_PASS_1_KEY, WIFI_PASS_1);
    }

    if(utils_nvs_get_str(NVS_SSID_2_KEY, WIFI_SSID_2, 64) == ESP_OK){
        utils_nvs_get_str(NVS_PASS_2_KEY, WIFI_PASS_2, 64);
        ESP_LOGI(TAG, "*WIFI 2 Credentials %s|%s#", WIFI_SSID_2, WIFI_PASS_2);
    }else{
        ESP_LOGI(TAG, "*Could not get Wifi 2 Credentials From NVS#");
        strcpy(WIFI_SSID_2, DEFAULT_SSID2);
        strcpy(WIFI_PASS_2, DEFAULT_PASS2);
        ESP_LOGI(TAG, "*Default WIFI 2 Credentials %s|%s#", WIFI_SSID_2, WIFI_PASS_2);
        utils_nvs_set_str(NVS_SSID_2_KEY, WIFI_SSID_2);
        utils_nvs_set_str(NVS_PASS_2_KEY, WIFI_PASS_2);
    }

    if(utils_nvs_get_str(NVS_SSID_3_KEY, WIFI_SSID_3, 64) == ESP_OK){
        utils_nvs_get_str(NVS_PASS_3_KEY, WIFI_PASS_3, 64);
        ESP_LOGI(TAG, "*WIFI 3 Credentials %s|%s#", WIFI_SSID_3, WIFI_PASS_3);
    }else{
        ESP_LOGI(TAG, "*Could not get Wifi 3 Credentials From NVS#");
        strcpy(WIFI_SSID_3, DEFAULT_SSID3);
        strcpy(WIFI_PASS_3, DEFAULT_PASS3);
        ESP_LOGI(TAG, "*Default WIFI 3 Credentials %s|%s#", WIFI_SSID_3, WIFI_PASS_3);
        utils_nvs_set_str(NVS_SSID_3_KEY, WIFI_SSID_3);
        utils_nvs_set_str(NVS_PASS_3_KEY, WIFI_PASS_3);
    }




    if(utils_nvs_get_int(NVS_SERVER_PORT_KEY, &server_port) == ESP_OK){
        ESP_LOGI(TAG, "*Server Port From NVS %d#", server_port);
    }else{
        server_port = DEFAULT_SERVER_PORT;
        ESP_LOGI(TAG, "*Default Server Port %d#", server_port);
        utils_nvs_set_int(NVS_SERVER_PORT_KEY, server_port);
    }

    if(utils_nvs_get_int(NVS_INH_KEY, &INHOutputValue) == ESP_OK){
        if (INHOutputValue != 0)
            INHOutputValue = 1;
        ESP_LOGI(TAG, "*INH Value is %d#", INHOutputValue);
    }else{
        strcpy(server_ip_addr, DEFAULT_SERVER_IP_ADDR);
        ESP_LOGI(TAG, "*Default INH Output Value is 0#" );
        utils_nvs_set_int(NVS_INH_KEY, 0);
        INHOutputValue = 0;
    }
    if (INHOutputValue != 0)
    {
        INHOutputValue = 1;
        gpio_set_level(CINHO, 0);
    }
    else
    {
        gpio_set_level(CINHO, 1);
    }




    if(utils_nvs_get_str(NVS_SERVER_IP_KEY, server_ip_addr, 100) == ESP_OK){
        ESP_LOGI(TAG, "*Server IP From NVS %s#", server_ip_addr);
    }else{
        strcpy(server_ip_addr, DEFAULT_SERVER_IP_ADDR);
        ESP_LOGI(TAG, "*Default Server IP : %s#", server_ip_addr);
        utils_nvs_set_str(NVS_SERVER_IP_KEY, server_ip_addr);
    }



// if jumper set server address and server port as jumperPort
    if (gpio_get_level(JUMPER) == 0)
    {        
        strcpy(server_ip_addr, DEFAULT_SERVER_IP_ADDR_TRY);
        ESP_LOGI(TAG, "***************************");
        ESP_LOGI(TAG, "*JUMPER SENSED AT POWER ON#");
        ESP_LOGI(TAG, "*Server IP ADDRESS is %s#",server_ip_addr);
        if(utils_nvs_get_int(NVS_SERVER_PORT_KEY_JUMPER, &jumperPort) == ESP_OK){
               ESP_LOGI(TAG, "*JUMPER Port is %d#",jumperPort);
               server_port = jumperPort; 
        }
        
    }

    if(utils_nvs_get_int(NVS_CASH1_KEY, &cashValue) == ESP_OK){
        CashTotals[0] = cashValue;
    }
    if(utils_nvs_get_int(NVS_CASH2_KEY, &cashValue) == ESP_OK){
        CashTotals[1] = cashValue;
    }
    if(utils_nvs_get_int(NVS_CASH3_KEY, &cashValue) == ESP_OK){
        CashTotals[2] = cashValue;
    }
    if(utils_nvs_get_int(NVS_CASH4_KEY, &cashValue) == ESP_OK){
        CashTotals[3] = cashValue;
    }
    if(utils_nvs_get_int(NVS_CASH5_KEY, &cashValue) == ESP_OK){
        CashTotals[4] = cashValue;
    }
    if(utils_nvs_get_int(NVS_CASH6_KEY, &cashValue) == ESP_OK){
        CashTotals[5] = cashValue;
    }
    if(utils_nvs_get_int(NVS_CASH7_KEY, &cashValue) == ESP_OK){
        CashTotals[6] = cashValue;
    }

    if(utils_nvs_get_int(NVS_CA_KEY, &caValue) == ESP_OK){
        SignalPolarity = caValue%2;
        pulseWitdh = caValue/2;
        ESP_LOGI(TAG, "CA Values %d - %d", pulseWitdh,SignalPolarity);
    }
    else
    {
        SignalPolarity = 0;
        pulseWitdh = 50;
        ESP_LOGI(TAG, "Default CA Values %d - %d", pulseWitdh,SignalPolarity);
    }


    if(utils_nvs_get_str(NVS_OTA_URL_KEY, FOTA_URL, 256) == ESP_OK){
        ESP_LOGI(TAG, "*FOTA URL From NVS %s#", FOTA_URL);
    }else{
        strcpy(FOTA_URL, DEFAULT_FOTA_URL);
        ESP_LOGI(TAG, "*Default FOTA URL : %s#", FOTA_URL);
        utils_nvs_set_str(NVS_OTA_URL_KEY, FOTA_URL);
    }

}

bool pending_tcp_packet = false;
char tcp_packet[200];
void tcp_ip_client_send_str(const char * str){
    pending_tcp_packet = true;
    strcpy(tcp_packet, str);
    if(sock != -1){
        ESP_LOGI(TAG, "Sending packet to TCP socket : %s", str);
        uart_write_string(tcp_packet);
        int err = send(sock, tcp_packet, strlen(tcp_packet), 0);
        if (err < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            sock = -1;
            shutdown(sock, 0);
            close(sock);
        }
    }
}

bool extractSubstring(const char* str, char* result) {
    const char* start = strchr(str, '*');
    const char* end = strchr(str, '#');

    if (start != NULL && end != NULL && end > start + 1) {
        strncpy(result, start + 1, end - start - 1);
        result[end - start - 1] = '\0';
        return true;
    }

    return false;
}

void resolve_hostname(const char *hostname) {
    struct addrinfo hints, *res;
    int status;
    // char ipstr[100];
    char payload[200];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET for IPv4, AF_INET6 for IPv6
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(hostname, NULL, &hints, &res)) != 0) {
        // Corrected here: use gai_strerror instead of strerror
        fprintf(stderr, "getaddrinfo: %s\n", strerror(status));
        return;
    }

    printf("*IP addresses for %s:\n\n#", hostname);
    ESP_LOGI(TAG,"*IP addresses for %s:\n\n#", hostname);
    struct addrinfo *p;
    for (p = res; p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        sprintf(payload,"*IP VER - %s: IP STR - %s#", ipver, ipstr);
        uart_write_string(payload);
//        ESP_LOGI(TAG,payload);

    }

    freeaddrinfo(res); // free the linked list
}

void tcpip_client_task(){
    char payload[400];
    char rx_buffer[128];
    int addr_family = 0;
    int ip_protocol = 0;
    uint32_t lastPrint = 0;
    for(;;){
        if(connected_to_wifi_and_internet){ //continously check for wifi
            //if wifi connected try to connect to tcp server
             resolve_hostname(server_ip_addr);
            struct sockaddr_in dest_addr;
            dest_addr.sin_addr.s_addr = inet_addr(ipstr);
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_port = htons(server_port);
            addr_family = AF_INET;
            ip_protocol = IPPROTO_IP;
            ESP_LOGI(TAG, "*Trying to connect to TCP Server#");
            set_led_state(WIFI_AND_INTERNET_NO_SERVER);
            sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
            if (sock < 0) {
                ESP_LOGE(TAG, "*Unable to create socket: errno %d#", errno);
                shutdown(sock, 0);
                close(sock);
            }else{
                ESP_LOGI(TAG, "*Socket created, connecting to %s:%s:%d#", server_ip_addr,ipstr, server_port);
                int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
                if (err != 0) {
                    ESP_LOGE(TAG, "*Socket unable to connect: errno %d#", errno);
                    ESP_LOGE(TAG, "*Shutting down socket and restarting...#");
                    shutdown(sock, 0);
                    close(sock);
                    sock = -1;
                }else{
               
                    set_led_state(EVERYTHING_OK_LED); 
                    if (gpio_get_level(JUMPER) == 0)
                        sprintf(payload, "*MAC,%s#", MAC_ADDRESS_ESP);  // for GVC use ,
                    else
                        sprintf(payload, "*MAC,%s#", MAC_ADDRESS_ESP);  // for KP use :

                    int err = send(sock, payload, strlen(payload), 0);
                    ESP_LOGI(TAG, "*Successfully connected#");  
                    if (gpio_get_level(JUMPER) == 0)
                        ESP_LOGI(TAG, "*MAC,%s#", MAC_ADDRESS_ESP) ;
                    else
                        ESP_LOGI(TAG, "*MAC,%s#", MAC_ADDRESS_ESP) ;

                    sprintf(payload, "*WiFi,%d#", WiFiNumber); //actual when in production
                    err = send(sock, payload, strlen(payload), 0);

                    ESP_LOGI(TAG, "*%s#",FWVersion);
                    err = send(sock, FWVersion, strlen(FWVersion), 0);


                    if (err < 0) {
                        ESP_LOGE(TAG, "*Error occurred during sending: errno %d#", errno);
                        shutdown(sock, 0);
                        close(sock);
                        sock = -1;
                    }else{
                        while(1){
                            /*
                            if(pending_tcp_packet){
                                pending_tcp_packet = false;
                                ESP_LOGI(TAG, "Sending to TCP Socket : %s", tcp_packet);
                                
                            }
                            */

                            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
                            // Error occurred during receiving
                            if (len < 0) {
                                ESP_LOGE(TAG, "*recv failed: errno %d#", errno);
                                ESP_LOGE(TAG, "*Shutting down socket and restarting...#");
                                shutdown(sock, 0);
                                close(sock);
                                sock  = -1;
                                break;
                            }
                            else if(len == 0){
                                //No Data
                                if(millis() - lastPrint > 5000){
                                    lastPrint = millis();
                                    ESP_LOGI(TAG, "*Waiting For Data On TCP Port#");
                                }
                            }
                            // Data received
                            else {
                                blinkLEDNumber = 1;
                                rx_event_pending = 1;
                                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                                ESP_LOGI(TAG, "Received %d bytes from %s Pulses %d:", len, server_ip_addr,pulses);
                                ESP_LOGI(TAG, "%s", rx_buffer);
                                char buf[len+1];

                                if(strncmp(rx_buffer, "*SS:", 4) == 0){
                                    sscanf(rx_buffer, "*SS:%[^:]:%[^:]:%[^#]#",SSuserName,SSdateTime, buf);
                                    strcpy(WIFI_SSID_1, buf);
                                    utils_nvs_set_str(NVS_SSID_1_KEY, WIFI_SSID_1);
                                    utils_nvs_set_str(NVS_SSID_1_KEY, WIFI_SSID_1);
                                    sprintf(payload, "*SS-OK,%s,%s#",SSuserName,SSdateTime);
                                    utils_nvs_set_str(NVS_SS_USERNAME, SSuserName);
                                    utils_nvs_set_str(NVS_SS_DATETIME, SSdateTime);
                                    send(sock, payload, strlen(payload), 0);
                                    tx_event_pending = 1;
                                }
                                // done by siddhi
                                // totPolarity
                                else if(strncmp(rx_buffer, "*CA?#", 5) == 0){
                                        ESP_LOGI(TAG, "CA Values @ numValue %d polarity %d username %s dateTime %s",pulseWitdh,polarity,CAuserName,CAdateTime);
                                        
                                        sprintf(payload, "*CA-OK,%s,%s,%d,%d#",CAuserName,CAdateTime,pulseWitdh,SignalPolarity); //actual when in production
                                        send(sock, payload, strlen(payload), 0);
                                 }

                                else if(strncmp(rx_buffer, "*INH?#",6) == 0){
                                        if (INHInputValue !=0)
                                            INHInputValue = 1;
                                        ESP_LOGI(TAG, "INH Values @ numValue %d ",INHInputValue);
                                        sprintf(payload, "*INH-IN,%s,%s,%d,%d#",INHuserName,INHdateTime,INHInputValue,INHOutputValue); 
                                        send(sock, payload, strlen(payload), 0);
                                 }
                                else if(strncmp(rx_buffer, "*INH:", 5) == 0){
                                        sscanf(rx_buffer, "*INH:%[^:]:%[^:]:%d#",INHuserName,INHdateTime, &INHOutputValue);
                                        if (INHOutputValue != 0)
                                        {
                                            INHOutputValue = 1;
                                            gpio_set_level(CINHO, 0);
                                        }
                                        else
                                        {
                                              gpio_set_level(CINHO, 1);
                                        }
                                        ESP_LOGI (TAG, "Set INH Output as %d",INHOutputValue);
                                        sprintf(payload, "*INH-DONE,%s,%s,%d#",SSuserName,SSdateTime,INHOutputValue);
                                        utils_nvs_set_str(NVS_INH_USERNAME, INHuserName);
                                        utils_nvs_set_str(NVS_INH_DATETIME, INHdateTime);
                                        send(sock, payload, strlen(payload), 0);
                                        // sprintf(payload, "*INH-DONE,%d#",INHOutputValue); //actual when in production
                                        // send(sock, payload, strlen(payload), 0);
                                        utils_nvs_set_int(NVS_INH_KEY, INHOutputValue);
                                }    


                                else if(strncmp(rx_buffer, "*SP:", 4) == 0){
                                        sscanf(rx_buffer, "*SP:%[^:]:%[^:]:%d#",SPuserName,SPdateTime, &jumperPort);
                                         sprintf(payload, "*SP-OK,%s,%s,%d#",SPuserName,SPdateTime,jumperPort);
                                        utils_nvs_set_str(NVS_SP_USERNAME, SPuserName);
                                        utils_nvs_set_str(NVS_SP_DATETIME, SPdateTime);
                                        send(sock, payload, strlen(payload), 0);
                                        // sprintf(payload, "*SP-OK,%d#",jumperPort); //actual when in production
                                        // send(sock, payload, strlen(payload), 0);
                                        utils_nvs_set_int(NVS_SERVER_PORT_KEY_JUMPER, jumperPort);
 
                                }        
                                else if(strncmp(rx_buffer, "*CA:", 4) == 0){
                                        sscanf(rx_buffer, "*CA:%[^:]:%[^:]:%d:%d#",CAuserName,CAdateTime, &numValue,&polarity);
                                        ESP_LOGI(TAG, "Generate @ numValue %d polarity %d",numValue,polarity);
                                        sprintf(payload, "*CA-OK,%s,%s,%d,%d#",CAuserName,CAdateTime,numValue,polarity);
                                        utils_nvs_set_str(NVS_CA_USERNAME, CAuserName);
                                        utils_nvs_set_str(NVS_CA_DATETIME, CAdateTime);
                                        ESP_LOGI(TAG,"CA Values Saved %s,%s",CAuserName,CAdateTime);
                                        send(sock, payload, strlen(payload), 0);
                                        
                                        // sprintf(payload, "*CA-OK,%d,%d#",numValue,polarity); //actual when in production
                                        // send(sock, payload, strlen(payload), 0);
                                        // valid values are between 25 and 100
                                        if (numValue<10)
                                            numValue = 25;
                                        if (numValue>100)
                                            numValue = 100;
                                        // possible values are 0 and 1        
                                        if (polarity>0)
                                            polarity = 1;    
                                        pulseWitdh=numValue;
                                        SignalPolarity=polarity;
                                        tx_event_pending = 1;
                                        Out4094(0x00);
                                        utils_nvs_set_int(NVS_CA_KEY, numValue*2+polarity);
                                }
                                else if(strncmp(rx_buffer, "*SS1:", 5) == 0){
                                    sscanf(rx_buffer, "*SS1:%[^:]:%[^:]:%[^#]#",SS1userName,SS1dateTime, buf);
                                    strcpy(WIFI_SSID_2, buf);
                                    utils_nvs_set_str(NVS_SSID_2_KEY, WIFI_SSID_2);
                                     sprintf(payload, "*SS1-OK,%s,%s#",SS1userName,SS1dateTime);
                                    utils_nvs_set_str(NVS_SS1_USERNAME, SS1userName);
                                    utils_nvs_set_str(NVS_SS1_DATETIME, SS1dateTime);
                                    send(sock, payload, strlen(payload), 0);
                                    // send(sock, "*SS1-OK#", strlen("*SS1-OK#"), 0);
                                    tx_event_pending = 1;
                                }
                                else if(strncmp(rx_buffer, "*SS2:", 5) == 0){
                                    sscanf(rx_buffer, "*SS2:%[^:]:%[^:]:%[^#]#",userName,dateTime, buf);
                                    strcpy(WIFI_SSID_3, buf);
                                    utils_nvs_set_str(NVS_SSID_3_KEY, WIFI_SSID_3);
                                    send(sock, "*SS2-OK#", strlen("*SS2-OK#"), 0);
                                    tx_event_pending = 1;
                                }else if(strncmp(rx_buffer, "*PW:", 4) == 0){
                                    sscanf(rx_buffer, "*PW:%[^:]:%[^:]:%[^#]#",PWuserName,PWdateTime, buf);
                                    strcpy(WIFI_PASS_1, buf);
                                    utils_nvs_set_str(NVS_PASS_1_KEY, WIFI_PASS_1);
                                      sprintf(payload, "*PW-OK,%s,%s#",PWuserName,PWdateTime);
                                    utils_nvs_set_str(NVS_PW_USERNAME, PWuserName);
                                    utils_nvs_set_str(NVS_PW_DATETIME, PWdateTime);
                                    send(sock, payload, strlen(payload), 0);
                                    // send(sock, "*PW-OK#", strlen("*PW-OK#"), 0);
                                    tx_event_pending = 1;
                                }else if(strncmp(rx_buffer, "*PW1:", 5) == 0){
                                    sscanf(rx_buffer, "*PW1:%[^:]:%[^:]:%[^#]#",PW1userName,PW1dateTime, buf);
                                    strcpy(WIFI_PASS_2, buf);
                                    utils_nvs_set_str(NVS_PASS_2_KEY, WIFI_PASS_2);
                                      sprintf(payload, "*PW1-OK,%s,%s#",PW1userName,PW1dateTime);
                                    utils_nvs_set_str(NVS_PW1_USERNAME, PW1userName);
                                    utils_nvs_set_str(NVS_PW1_DATETIME, PW1dateTime);
                                    send(sock, payload, strlen(payload), 0);
                                    // send(sock, "*PW1-OK#", strlen("*PW1-OK#"), 0);
                                    tx_event_pending = 1;
                                }else if(strncmp(rx_buffer, "*PW2:", 5) == 0){
                                    sscanf(rx_buffer, "*PW2:%[^:]:%[^:]:%[^#]#",userName,dateTime, buf);
                                    strcpy(WIFI_PASS_3, buf);
                                    utils_nvs_set_str(NVS_PASS_3_KEY, WIFI_PASS_3);
                                    send(sock, "*PW2-OK#", strlen("*PW2-OK#"), 0);
                                    tx_event_pending = 1;
                                }else if(strncmp(rx_buffer, "*URL:", 5) == 0){
                                    sscanf(rx_buffer, "*URL:%[^:]:%[^:]:%[^#]#",URLuserName,URLdateTime, buf);
                                    strcpy(FOTA_URL, buf);
                                    utils_nvs_set_str(NVS_OTA_URL_KEY, FOTA_URL);
                                      sprintf(payload, "*URL-OK,%s,%s#",URLuserName,URLdateTime);
                                    utils_nvs_set_str(NVS_URL_USERNAME, URLuserName);
                                    utils_nvs_set_str(NVS_URL_DATETIME, URLdateTime);
                                    send(sock, payload, strlen(payload), 0);
                                    // send(sock, "*URL-OK#", strlen("*URL-OK#"), 0);
                                    tx_event_pending = 1;
                                }
                                else if (strncmp(rx_buffer, "*SSID?#", 7) == 0){
                                sprintf(payload, "*SSID,%d,%s,%s,%s#",WiFiNumber,WIFI_SSID_1,WIFI_SSID_2,WIFI_SSID_3); 
                                send(sock, payload, strlen(payload), 0);
                                tx_event_pending = 1;
                                }
                                else if(strncmp(rx_buffer, "*URL?#", 6) == 0){
                                    ESP_LOGI(TAG,"URL RECEIVED,%s,%s,%s",URLuserName,URLdateTime,FOTA_URL);
                                 char msg[600];
                                sprintf(msg,"*URL,%s,%s,%s#",URLuserName,URLdateTime,FOTA_URL); 
                                send(sock, msg, strlen(msg), 0);
                                tx_event_pending = 1;
                                }else if(strncmp(rx_buffer, "*FOTA:", 6) == 0){
                                    send(sock, "*FOTA-OK#", strlen("*FOTA-OK#"), 0);
                                  
                                    send(sock,FOTA_URL,strlen(FOTA_URL),0);
                                    tx_event_pending = 1;
                                    http_fota();
                                }else if(strncmp(rx_buffer, "*SIP:", 5) == 0){
                                  
                                    sscanf(rx_buffer, "*SIP:%[^:]:%[^:]:%[^:]:%d#",SIPuserName,SIPdateTime,server_ip_addr,
                                        &sp_port);
                                    char buf[100];
                                    sprintf(payload, "*SIP-OK,%s,%s#",SIPuserName,SIPdateTime);
                                    sprintf(buf, "%s",server_ip_addr);
                                  

                                    utils_nvs_set_str(NVS_SERVER_IP_KEY, buf);
                                    utils_nvs_set_int(NVS_SERVER_PORT_KEY, sp_port);
                                    utils_nvs_set_str(NVS_SIP_USERNAME, SIPuserName);
                                    utils_nvs_set_str(NVS_SIP_DATETIME, SIPdateTime);
                                    
                                    
                                    send(sock, payload, strlen(payload), 0);
                                    tx_event_pending = 1;
                                }else if (strncmp(rx_buffer, "*ERASE#", 7) == 0){
                                    utils_nvs_erase_all();
                                    send(sock, "*ERASE-OK#", strlen("*ERASE-OK#"), 0);
                                }else if(strncmp(rx_buffer, "*RESTART#", 9) == 0){
                                    send(sock, "*RESTART-OK#", strlen("*RESTART-OK#"), 0);
                                    tx_event_pending = 1;
                                    vTaskDelay(2000/portTICK_PERIOD_MS);
                                    esp_restart();
                                }
//                                 start genertaing pulses
//                                  INPUT  -   *V:{TID},{pin},{Pulses}#
//                                  *V-OK{TID}:{pin}:{Pulses}#
//                                   generate pulses on pin
//                                  *T-OK{TID}:{pin}:{Pulses}#
//                                  avoid duplicate TID
//                                  add up all pulses for specific pins
//                                  if to begin with
//                                  1 is 0, 2 is 0, 3 is 0.....
//                                  if I get commands say 3 pulses for pin1, 5 pulses for pin 2 etc.. add
//                                  1 is 3, 2 is 5 and so on
//                                  next time , add again to previous counts



                                else if(strncmp(rx_buffer, "*V:", 3) == 0){
                                    if (edges == 0) 
                                    {
                                        sscanf(rx_buffer, "*V:%d:%d:%d#",&TID,&pin,&pulses);
                                        if (INHInputValue == INHIBITLevel)
                                        {
                                          ESP_LOGI(TAG, "*UNIT DISABLED#");
                                          send(sock, "*VEND DISABLED#", strlen("*VEND DISABLED#"), 0);
                                            
                                        }
                                        else if (TID != LastTID)
                                        {
                                            edges = pulses*2;  // doubled edges
                                            // strcpy(WIFI_PASS_2, buf);
                                            // utils_nvs_set_str(NVS_PASS_2_KEY, WIFI_PASS_2);
                                            ESP_LOGI(TAG, "*V-OK,%d,%d,%d#",TID,pin,pulses);
                                            sprintf(payload, "*V-OK,%d,%d,%d#", TID,pin,pulses); //actual when in production
                                            send(sock, payload, strlen(payload), 0);
                                            vTaskDelay(1000/portTICK_PERIOD_MS);
                                            sprintf(payload, "*T-OK,%d,%d,%d#",TID,pin,pulses); //actual when in production
                                            ESP_LOGI(TAG, "*T-OK,%d,%d,%d#",TID,pin,pulses);
                                            send(sock, payload, strlen(payload), 0);
                                            tx_event_pending = 1;
                                            Totals[pin-1] += pulses;
                                            LastTID = TID;
                                        }
                                        else
                                        {
                                          ESP_LOGI(TAG, "Duplicate TID");
                                          send(sock, "*DUP TID#", strlen("*DUP TID#"), 0);
                                        }  

                                    }
                                }



                                else if(strncmp(rx_buffer, "*SL:", 4) == 0){
                                    if (edges == 0)
                                    {
                                        sscanf(rx_buffer, "*SL:%[^:]:%[^:]:%d:%d#",userName,dateTime, &ledpin,&ledstatus);
                                        // strcpy(WIFI_PASS_2, buf);
                                        // utils_nvs_set_str(NVS_PASS_2_KEY, WIFI_PASS_2);
                                        ESP_LOGI(TAG, "Set LED @ Pin %d Status %d",ledpin,ledstatus);
                                        send(sock, "*SL-OK#", strlen("*SL-OK#"), 0);
                                        tx_event_pending = 1;
                                        if (ledpin == 1)
                                            gpio_set_level(L1, ledstatus);
                                        if (ledpin == 2)
                                            gpio_set_level(L2, ledstatus);
                                        if (ledpin == 3)
                                            gpio_set_level(L3, ledstatus);
                                        
                                    }
                                }
                                // when TC command is received send totals

                                else if(strncmp(rx_buffer, "*TV?#", 5) == 0){
                                        sprintf(payload, "*TV,%d,%d,%d,%d,%d,%d,%d#", Totals[0],Totals[1],Totals[2],Totals[3],Totals[4],Totals[5],Totals[6]); //actual when in production
                                        send(sock, payload, strlen(payload), 0);
                                        ESP_LOGI(TAG, "TV Sending");
                                        
                                }

                                else if(strncmp(rx_buffer, "*TC?#", 5) == 0){
                                        sprintf(payload, "*TC,%d,%d,%d,%d,%d,%d,%d#", CashTotals[0],CashTotals[1],CashTotals[2],CashTotals[3],CashTotals[4],CashTotals[5],CashTotals[6]); //actual when in production
                                        send(sock, payload, strlen(payload), 0);
                                        ESP_LOGI(TAG, "*TC,%d,%d,%d,%d,%d,%d,%d#", CashTotals[0],CashTotals[1],CashTotals[2],CashTotals[3],CashTotals[4],CashTotals[5],CashTotals[6] );
                                        
                                }
                                  else if(strncmp(rx_buffer, "*SIP?#", 6) == 0){
                                        sprintf(payload, "*SIP,%s,%s,%s,%d#",SIPuserName,SIPdateTime,server_ip_addr,
                                        sp_port ); //actual when in production
                                        send(sock, payload, strlen(payload), 0);
                                        ESP_LOGI(TAG, "*SIP,%s,%s,%s,%d#",SIPuserName,SIPdateTime,server_ip_addr,
                                        sp_port );
                                        
                                }
                                else if(strncmp(rx_buffer, "*CC:", 4) == 0){
                                    sscanf(rx_buffer, "*CC:%[^:]:%[^#]#",CCuserName,CCdateTime);
                                        ESP_LOGI(TAG, "*CC-OK#");
                                        // sprintf(payload, "*CC-OK#"); //actual when in production
                                          sprintf(payload, "*CC-OK,%s,%s#",CCuserName,CCdateTime);
                                    utils_nvs_set_str(NVS_CC_USERNAME, CCuserName);
                                    utils_nvs_set_str(NVS_CC_DATETIME, CCdateTime);
                                    send(sock, payload, strlen(payload), 0);
                                        // send(sock, payload, strlen(payload), 0);
                                        for (int i = 0 ; i < 7 ; i++)
                                        {
                                            Totals[i] = 0;
                                            CashTotals[i] = 0;
                                        } 
                                        utils_nvs_set_int(NVS_CASH1_KEY, CashTotals[0]);
                                        utils_nvs_set_int(NVS_CASH2_KEY, CashTotals[1]);
                                        utils_nvs_set_int(NVS_CASH3_KEY, CashTotals[2]);
                                        utils_nvs_set_int(NVS_CASH4_KEY, CashTotals[3]);
                                        utils_nvs_set_int(NVS_CASH5_KEY, CashTotals[4]);
                                        utils_nvs_set_int(NVS_CASH6_KEY, CashTotals[5]);
                                        utils_nvs_set_int(NVS_CASH7_KEY, CashTotals[6]);
                                 }


                                else if(strncmp(rx_buffer, "*FW?#", 5) == 0){
                                        ESP_LOGI(TAG, "*%s#",FWVersion);
                                        send(sock, FWVersion, strlen(FWVersion), 0);
                                        tx_event_pending = 1;
                                        if (ledpin == 1)
                                            gpio_set_level(L1, ledstatus);
                                        if (ledpin == 2)
                                            gpio_set_level(L2, ledstatus);
                                        if (ledpin == 3)
                                            gpio_set_level(L3, ledstatus);
                                    }
                                else if(strncmp(rx_buffer, "*RST:", 5) == 0){
                                        ESP_LOGI(TAG, "**************Restarting after 3 second*******");
                                        send(sock, "*RST-OK#", strlen("*RST-OK#"), 0);
                                        ESP_LOGI(TAG, "*RST-OK#");
                                        vTaskDelay(3000/portTICK_PERIOD_MS);
                                        esp_restart();
                                }
                                else{
                                    if(extractSubstring(rx_buffer, buf) == true){
                                        uart_write_string("*");
                                        uart_write_string(buf);
                                        uart_write_string("#");
                                        tx_event_pending = 1;
                                    }
                                }
//                                Write On UART
                                uart_write_string(rx_buffer);
                                // gpio_set_level(LedTCP, 1);
                                // vTaskDelay(200/portTICK_PERIOD_MS);
                                // gpio_set_level(LedTCP, 0);
                            }
                            vTaskDelay(1000 / portTICK_PERIOD_MS);
                        }
                    }
                }
            }
        }
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}

// void WiFiConnection (void)
// {
//         set_led_state(SEARCH_FOR_WIFI);
//         bool connected_to_wifi = false;
//         int WiFiRetryCount = 0;
//         while (connected_to_wifi == 0)
//         {
//             if (WiFiRetryCount >= 3) 
//             {
//                 ESP_LOGI(TAG, "*Restarting as WiFi Not Sensed#");
//                 vTaskDelay(500);
//                 esp_restart();                
//             }   
//             if (WiFiNumber != 1)
//             {
//                 ESP_LOGI(TAG, "*Trying to connect to SSID1 %s %s#",WIFI_SSID_1, WIFI_PASS_1);
//                 if(!connect_to_wifi(WIFI_SSID_1, WIFI_PASS_1)){
//                     s_retry_num = 0;
//                     WiFiNumber = 1;
//                     ESP_LOGI(TAG, "*Failed to Connect SSID1#");
//                     WiFiRetryCount++;
//                 }else{
//                     ESP_LOGI(TAG, "*Connected To WiFi1#");
//                     connected_to_wifi = true;
//                     WiFiNumber = 1;
//                 }
//             }

//             else
//             {
//                 ESP_LOGI(TAG, "*Trying to connect to SSID2 %s %s#",WIFI_SSID_2, WIFI_PASS_2);
//                 if(!connect_to_wifi(WIFI_SSID_2, WIFI_PASS_2)){
//                     s_retry_num = 0;
//                     WiFiNumber = 2;
//                     ESP_LOGI(TAG, "*Failed to Connect SSID2#");
//                     WiFiRetryCount++;
//                 }else{
//                     ESP_LOGI(TAG, "*Connected To WiFi2#");
//                     connected_to_wifi = true;
//                     WiFiNumber = 2;
//                 }
//             }
//         }

//     if(connected_to_wifi){
//              connected_to_wifi_and_internet = true;
//         // esp_http_client_config_t config = {
//         //     .url = "http://www.google.com",  
//         // };
//         // esp_http_client_handle_t client = esp_http_client_init(&config);
//         // esp_err_t err = esp_http_client_perform(client);
//         // if (err == ESP_OK) {
//         //     ESP_LOGI(TAG, "*Internet connection test successful#");
//         //     connected_to_wifi_and_internet = true;
//         // } else {
//         //     ESP_LOGI(TAG,"*Internet connection test failed: %s#", esp_err_to_name(err));
//         //     set_led_state(WIFI_FOUND_NO_INTERNET);
//         // }
//         // esp_http_client_cleanup(client);
//     }


// }

void wifi_init_sta(void)
{

    load_settings_nvs();
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    /*
    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );*/

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id) );
    
    set_led_state(SEARCH_FOR_WIFI);
    ESP_LOGI(TAG,"*Connect WiFi at Power On#");
    bool connected_to_wifi = false;
    //ESP_LOGI(TAG, "Trying to connect to SSID1 %s | %s",DEFAULT_SSID1,DEFAULT_PASS1);
    ESP_LOGI(TAG, "*Trying to connect to SSID1#");
    WiFiNumber = 1;
    if(!connect_to_wifi(WIFI_SSID_1, WIFI_PASS_1)){
        //ESP_LOGI(TAG, "Trying to connect to SSID2 %S | %S",DEFAULT_SSID2, DEFAULT_PASS1);
        ESP_LOGI(TAG, "*Trying to connect to SSID2# ");
        WiFiNumber = 2;
        s_retry_num = 0;
        if(!connect_to_wifi(WIFI_SSID_2, WIFI_PASS_2)){

            ESP_LOGI(TAG, "*Trying to connect to SSID3# ");
            WiFiNumber = 3;
            s_retry_num = 0;

             if(!connect_to_wifi(WIFI_SSID_3, WIFI_PASS_3)){
                ESP_LOGI(TAG, "Could not connect to SSID3. Restarting....");
                ESP_LOGI(TAG, "*restarting after 2 seconds#");
                vTaskDelay(2000/portTICK_PERIOD_MS);
                esp_restart();
             }
             else{
            ESP_LOGI(TAG, "*Connected To WiFi3#");
            connected_to_wifi = true;
        }
        }
          else{
            ESP_LOGI(TAG, "*Connected To WiFi2#");
            connected_to_wifi = true;
        }
    }else{
        ESP_LOGI(TAG, "*Connected To WiFi1#");
        connected_to_wifi = true;
    }
  
    if(connected_to_wifi){
        connected_to_wifi_and_internet = true;
        // esp_http_client_config_t config = {
        //     .url = "http://www.google.com",  
        // };
        // esp_http_client_handle_t client = esp_http_client_init(&config);
        // esp_err_t err = esp_http_client_perform(client);
        // if (err == ESP_OK) {
        //     ESP_LOGI(TAG, "Internet connection test successful\n");
        //     set_led_state(WIFI_AND_INTERNET_NO_SERVER);
        // } else {
        //     ESP_LOGI(TAG,"Internet connection test failed: %s\n", esp_err_to_name(err));
        //     set_led_state(WIFI_FOUND_NO_INTERNET);
        // }
        // esp_http_client_cleanup(client);
    }

}

uint32_t millis(void) {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

uint32_t last_update_led1 = 0;
uint8_t led1_gpio_state = 0;
uint8_t led2_gpio_state = 0;

void set_led_state(Led_State_t st){
    last_update_led1 = 0;
    led_state = st;
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}

#define MAX_HTTP_RECV_BUFFER 1024

void http_fota( void ){
    
   // http_perform_as_stream_reader();
   // return;
    esp_err_t err;
    esp_ota_handle_t ota_handle = 0;
    const esp_partition_t *update_partition = NULL;

    Led_State_t prev_state = led_state;

    update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        printf("Failed to get OTA partition.\n#");
        //esp_http_client_cleanup(client);
        set_led_state(prev_state);
        return;
    }

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        printf("Failed to begin OTA update: %s\n", esp_err_to_name(err));
        //esp_http_client_cleanup(client);
        set_led_state(prev_state);
        return;
    }

    esp_http_client_config_t config = {
        .url = FOTA_URL,
        .event_handler = _http_event_handler
    };

    

    esp_http_client_handle_t client = esp_http_client_init(&config);
    /*
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        printf("Failed to download firmware image: %s\n", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        set_led_state(prev_state);
        return;
    }
    */
    
    if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
        ESP_LOGE(TAG, "*Failed to open HTTP connection: %s#", esp_err_to_name(err));
        send(sock, "*FOTA-ERROR#", strlen("*FOTA-ERROR#"), 0);
        esp_http_client_cleanup(client);
        set_led_state(prev_state);
        return;
    }

    ESP_LOGI(TAG, "*esp_http_client_open#");

    /*
    int read_bytes = 0;
    while (1) {
        read_bytes = esp_http_client_read(client, data, sizeof(data));
        if (read_bytes <= 0) {
            break;
        }

        err = esp_ota_write(ota_handle, (const void *)data, read_bytes);
        if (err != ESP_OK) {
            printf("Failed to write OTA data: %s\n", esp_err_to_name(err));
            esp_http_client_cleanup(client);
        }else{
            ESP_LOGI(TAG, "Written : %d", read_bytes);
        }
    }
    */
    char data[MAX_HTTP_RECV_BUFFER+1];
    int content_length =  esp_http_client_fetch_headers(client);
    int total_read_len = 0, read_len;
    set_led_state(OTA_IN_PROGRESS);
    if(content_length > 0){
        while (total_read_len < content_length ) {
            read_len = esp_http_client_read(client, data, MAX_HTTP_RECV_BUFFER);
            if (read_len <= 0) {
                ESP_LOGI(TAG, "*Error read data#");
                send(sock, "*FOTA-ERROR#", strlen("*FOTA-ERROR#"), 0);
            }
            //ESP_LOGI(TAG, "read_len = %d", read_len);
            total_read_len += read_len;
            err = esp_ota_write(ota_handle, (const void *)data, read_len);
            if (err != ESP_OK) {
                printf("Failed to write OTA data: %s\n", esp_err_to_name(err));
                send(sock, "*FOTA-ERROR#", strlen("*FOTA-ERROR#"), 0);
                esp_http_client_cleanup(client);
            }else{
                ESP_LOGI(TAG, "*OTA Percent : %d#", ((total_read_len*100)/content_length) );
            }
        }
    }
    

    

    if(err != ESP_OK){
        set_led_state(prev_state);
        return;
    }

    ESP_LOGI(TAG, "*ota data written#");

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        printf("*OTA update failed: %s\n#", esp_err_to_name(err));
        send(sock, "*FOTA-ERROR#", strlen("*FOTA-ERROR#"), 0);
        esp_http_client_cleanup(client);
        set_led_state(prev_state);
        return;
    }

    ESP_LOGI(TAG, "*esp_ota_end#");

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        printf("Failed to set boot partition: %s\n", esp_err_to_name(err));
        send(sock, "*FOTA-ERROR#", strlen("*FOTA-ERROR#"), 0);
        esp_http_client_cleanup(client);
        set_led_state(prev_state);
        return;
    }

    ESP_LOGI(TAG, "*esp_ota_set_boot_partition#");
    
    esp_http_client_cleanup(client);
    printf("*OTA update successful! Restarting...\n#");
    send(sock, "*FOTA-OVER#", strlen("*FOTA-OVER#"), 0);
    
    vTaskDelay(2000/portTICK_PERIOD_MS);
    esp_restart();
    set_led_state(prev_state);
}

void leds_update_task(){
    for(;;){

        //Handle Led 1 States

        ticks_100 = ticks_100+1;
        if (ticks_100 >= 20)
        {
            ticks_100 = 0;
         }    
        if (led_state == SEARCH_FOR_ESPTOUCH)
        {
            current_interval = 0;
            if(led2_gpio_state == 1){
                led2_gpio_state = 0;
                led_set_level(LEDG, led2_gpio_state);
            }
            
        }
        if(led_state == STANDBY_LED){
            current_interval = 0;
            if(led1_gpio_state == 1){
                led1_gpio_state = 0;
                led_set_level(LEDR, led1_gpio_state);
            }
        }else if(led_state == SEARCH_FOR_WIFI){
            numberOfPulses = 20;
        }else if(led_state == WIFI_FOUND_NO_INTERNET){
            numberOfPulses = 6;
        }else if(led_state == WIFI_AND_INTERNET_NO_SERVER){
            numberOfPulses = 4;
        }else if(led_state == EVERYTHING_OK_LED){
            numberOfPulses = 2;
        }    
        //     // if(rx_event_pending){
        //     //     rx_event_pending = 0; 
        //     //     led_set_level(LEDG, 0);
        //     //     vTaskDelay(50/portTICK_PERIOD_MS);
        //     //     led_set_level(LEDG, 1);
        //     //     vTaskDelay(50/portTICK_PERIOD_MS);
        //     // }

        //     // if(tx_event_pending){
        //     //     tx_event_pending = 0; 
        //     //     led_set_level(LEDG, 0);
        //     //     vTaskDelay(50/portTICK_PERIOD_MS);
        //     //     led_set_level(LEDG, 1);
        //     //     vTaskDelay(50/portTICK_PERIOD_MS);
        //     //     led_set_level(LEDG, 0);
        //     //     vTaskDelay(50/portTICK_PERIOD_MS);
        //     //     led_set_level(LEDG, 1);
        //     //     vTaskDelay(50/portTICK_PERIOD_MS);
        //     // }

        // }
           else if (led_state == OTA_IN_PROGRESS){
            led2_gpio_state = !led2_gpio_state;
            led_set_level(LEDG, led2_gpio_state);
        }

        if (numberOfPulses>ticks_100)
        {
            // if (led_state == SEARCH_FOR_ESPTOUCH)
            // {
            //     led2_gpio_state = !led2_gpio_state;
            //     led_set_level(LEDG, led2_gpio_state);
            //     led_set_level(LEDR, 0);

            // }
            // else
            // {
                led1_gpio_state = !led1_gpio_state;
                led_set_level(LEDR, led1_gpio_state);
                led_set_level(LEDG, 0);
            // }
        }
        else
        {
            led1_gpio_state = 0;
            led_set_level(LEDR, led1_gpio_state);
        }
        // // if(((last_update_led1 == 0) || (millis() - last_update_led1 > current_interval)) && (current_interval != 0)){
        // //     //This will be only called when there is some error
        // //     last_update_led1 = millis();
        // //     led1_gpio_state = !led1_gpio_state;
        // //     led_set_level(LEDR, led1_gpio_state);
        // // }
        vTaskDelay(200/portTICK_PERIOD_MS);
    }
}

void status_leds_init(){
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set
    io_conf.pin_bit_mask = 1ULL << LEDR | 1ULL << LEDG ;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    led_set_level(LEDR, 0);
    led_set_level(LEDG, 0);
    xTaskCreate(leds_update_task, "leds_update_task", 2048, NULL, 6, NULL);
}

void uart_write_number(uint8_t number){
    char str[5];
    if (number<10)
    {
         str[0] = number+'0';
         str[1] = 0x00;
    }   
    uart_write_bytes(EX_UART_NUM, str, 1);
}

void uart_write_string(const char * str){
    uart_write_bytes(EX_UART_NUM, str, strlen(str));
}

void uart_write_string_ln(const char * str){
    uart_write_bytes(EX_UART_NUM, str, strlen(str));
    uart_write_string("\r\n");
}

void process_uart_packet(const char *pkt){
    rx_event_pending = 1;
    char buf[100];
      if(strncmp(pkt, "*CA?#", 5) == 0){
         char buffer[300]; 
       sprintf(buffer,"*CA-OK,%s,%s,%d,%d#",CAuserName,CAdateTime,pulseWitdh,SignalPolarity);

       uart_write_string_ln(buffer);
        tx_event_pending = 1;
    }else if(strncmp(pkt, "*CC:", 4) == 0){
         for (int i = 0 ; i < 7 ; i++)
        {
            Totals[i] = 0;
            CashTotals[i] = 0;
        } 
        utils_nvs_set_int(NVS_CASH1_KEY, CashTotals[0]);
        utils_nvs_set_int(NVS_CASH2_KEY, CashTotals[1]);
        utils_nvs_set_int(NVS_CASH3_KEY, CashTotals[2]);
        utils_nvs_set_int(NVS_CASH4_KEY, CashTotals[3]);
        utils_nvs_set_int(NVS_CASH5_KEY, CashTotals[4]);
        utils_nvs_set_int(NVS_CASH6_KEY, CashTotals[5]);
        utils_nvs_set_int(NVS_CASH7_KEY, CashTotals[6]);
         char buffer[150]; 
        sprintf(buffer, "*CC-OK#");
      
        uart_write_string_ln(buffer);
        tx_event_pending = 1;
    }
     else if(strncmp(pkt, "*SL:", 4) == 0){
        if(edges==0)
        {
        sscanf(pkt, "*SL:%d:%d#", &ledpin,&ledstatus);
        if (ledpin == 1)
            gpio_set_level(L1, ledstatus);
        if (ledpin == 2)
            gpio_set_level(L2, ledstatus);
        if (ledpin == 3)
            gpio_set_level(L3, ledstatus);
      
        uart_write_string_ln("*SL-OK#");
        tx_event_pending = 1;
        }
    }
    else if(strncmp(pkt, "*CA:", 4) == 0){
        sscanf(pkt, "*CA:%d:%d#",&numValue,&polarity);
        strcpy(CAuserName,"LOCAL");
        strcpy(CAdateTime,"00/00/00");
         char buffer[100]; 
        sprintf(buffer,"*CA-OK,%d,%d#",numValue,polarity);
        utils_nvs_set_str(NVS_CA_USERNAME, CAuserName);
        utils_nvs_set_str(NVS_CA_DATETIME, CAdateTime);
       uart_write_string_ln(buffer);
        tx_event_pending = 1;
    }
     else if(strncmp(pkt, "*FOTA:", 6) == 0){
    
         char buffer[100]; 
        sprintf(buffer,"*FOTA-OK#");
     

       uart_write_string_ln(buffer);
       uart_write_string_ln(FOTA_URL);
       http_fota();
        tx_event_pending = 1;
    }
     else if(strncmp(pkt, "*RST:", 5) == 0){
    
         char buffer[100]; 
        sprintf(buffer, "*RST-OK#");
        uart_write_string_ln(buffer);
        vTaskDelay(3000/portTICK_PERIOD_MS);
        esp_restart();
    
      
    }
    else if(strncmp(pkt, "*V:", 3) == 0){
        if(edges==0)
        {
           sscanf(pkt, "*V:%d:%d:%d#",&TID,&pin,&pulses);
            if (INHInputValue == INHIBITLevel)
            {
                ESP_LOGI(TAG, "*UNIT DISABLED#");
                char buffer[100]; 
                sprintf(buffer, "*VEND DISABLED#");
                uart_write_string_ln(buffer);
                
            }
             else if (TID != LastTID)
            {
                edges = pulses*2;  // doubled edges
                // strcpy(WIFI_PASS_2, buf);
                // utils_nvs_set_str(NVS_PASS_2_KEY, WIFI_PASS_2);
                ESP_LOGI(TAG, "*V-OK,%d,%d,%d#",TID,pin,pulses);
                 char buffer[100]; 
                sprintf(buffer, "*V-OK,%d,%d,%d#", TID,pin,pulses); //actual when in production
                uart_write_string_ln(buffer);
                vTaskDelay(1000/portTICK_PERIOD_MS);
                sprintf(buffer, "*T-OK,%d,%d,%d#",TID,pin,pulses); //actual when in production
                ESP_LOGI(TAG, "*T-OK,%d,%d,%d#",TID,pin,pulses);
                uart_write_string_ln(buffer);
                tx_event_pending = 1;
                Totals[pin-1] += pulses;
                LastTID = TID;
            }
            else
            {
                ESP_LOGI(TAG, "Duplicate TID");
           
                 char buffer[100]; 
                sprintf(buffer,  "*DUP TID#");
                uart_write_string_ln(buffer);
            }  
      
      
        }
    
      
    }
      else if(strncmp(pkt, "*FW?#", 5) == 0){
         ESP_LOGI(TAG, "*%s#",FWVersion);
       
         uart_write_string_ln(FWVersion);
      
        tx_event_pending = 1;
        if (ledpin == 1)
            gpio_set_level(L1, ledstatus);
        if (ledpin == 2)
            gpio_set_level(L2, ledstatus);
        if (ledpin == 3)
            gpio_set_level(L3, ledstatus);
    }
    else if(strncmp(pkt, "*SS:", 4) == 0){
        sscanf(pkt, "*SS:%[^#]#",buf);
        //uart_write_string_ln(buf);
        strcpy(WIFI_SSID_1, buf);
        utils_nvs_set_str(NVS_SSID_1_KEY, WIFI_SSID_1);
        uart_write_string_ln("*SS-OK#");
        tx_event_pending = 1;
    }else if(strncmp(pkt, "*SS1:", 5) == 0){
        sscanf(pkt, "*SS1:%[^#]#", buf);
        strcpy(WIFI_SSID_2, buf);
        utils_nvs_set_str(NVS_SSID_2_KEY, WIFI_SSID_2);
        uart_write_string_ln("*SS1-OK#");
        tx_event_pending = 1;
    }else if(strncmp(pkt, "*PW:", 4) == 0){
        sscanf(pkt, "*PW:%[^#]#", buf);
        strcpy(WIFI_PASS_1, buf);
        utils_nvs_set_str(NVS_PASS_1_KEY, WIFI_PASS_1);
        uart_write_string_ln("*PW-OK#");
        tx_event_pending = 1;
    }else if(strncmp(pkt, "*PW1:", 5) == 0){
        sscanf(pkt, "*PW1:%[^#]#", buf);
        strcpy(WIFI_PASS_2, buf);
        utils_nvs_set_str(NVS_PASS_2_KEY, WIFI_PASS_2);
        uart_write_string_ln("*PW1-OK#");
        tx_event_pending = 1;
    }else if(strncmp(pkt, "*PW2:", 5) == 0){
        sscanf(pkt, "*PW2:%[^#]#", buf);
        strcpy(WIFI_PASS_3, buf);
        utils_nvs_set_str(NVS_PASS_3_KEY, WIFI_PASS_3);
        uart_write_string_ln("*PW2-OK#");
        tx_event_pending = 1;
    }else if(strncmp(pkt, "*URL:", 5) == 0){
        sscanf(pkt, "*URL:%[^#]#", buf);
        strcpy(FOTA_URL, buf);
        strcpy(URLuserName,"LOCAL");
        strcpy(URLdateTime,"00/00/00");
        utils_nvs_set_str(NVS_OTA_URL_KEY, FOTA_URL);
        utils_nvs_set_str(NVS_URL_USERNAME, URLuserName);
        utils_nvs_set_str(NVS_URL_DATETIME, URLdateTime);

        uart_write_string_ln("*URL-OK#");
        tx_event_pending = 1;
    }else if(strncmp(pkt, "*FOTA#", 6) == 0){
        uart_write_string_ln("*FOTA-OK#");
        tx_event_pending = 1;
        http_fota();
    }else if(strncmp(pkt, "*URL?#", 6) == 0){
         char buffer[650]; 
       sprintf(buffer,"*%s,%s,%s#",URLuserName,URLdateTime,FOTA_URL);
        uart_write_string_ln(buffer);
        tx_event_pending = 1;
    
    }
//    WIFI_SSID_1,WIFI_SSID_2,WIFI_SSID_3
    else if(strncmp(pkt, "*SSID?#", 7) == 0){
        uart_write_string("SSID Current/1/2/3 are - ");
        uart_write_number(WiFiNumber);
        uart_write_string(" , ");
        uart_write_string(WIFI_SSID_1);
        uart_write_string(" , ");
        uart_write_string(WIFI_SSID_2);
        uart_write_string(" , ");
        uart_write_string_ln(WIFI_SSID_3);

        tx_event_pending = 1;
    
    }
    
    else if(strncmp(pkt, "*TC?#", 5) == 0){
        char buffer[100]; 
        sprintf(buffer, "*TC,%d,%d,%d,%d,%d,%d,%d#", 
             CashTotals[0],CashTotals[1],CashTotals[2],CashTotals[3],CashTotals[4],CashTotals[5],CashTotals[6]);

   
        uart_write_string_ln(buffer);
        tx_event_pending = 1;
    }
    else if(strncmp(pkt, "*TV?#", 5) == 0){
        char buffer[100]; 
        sprintf(buffer, "*TV,%d,%d,%d,%d,%d,%d,%d#", 
            Totals[0], Totals[1], Totals[2], Totals[3], Totals[4], Totals[5], Totals[6]);

   
        uart_write_string_ln(buffer);
        tx_event_pending = 1;
    
    }
    else if(strncmp(pkt, "*SIP:", 5) == 0){
      
        sscanf(pkt, "*SIP:%[^:]:%d#",server_ip_addr,
                                        &sp_port);
        strcpy(SIPuserName,"LOCAL");
        strcpy(SIPdateTime,"00/00/00");
        char buf[100];
        sprintf(buf, "%s", server_ip_addr);
        
        utils_nvs_set_str(NVS_SERVER_IP_KEY, buf);
        utils_nvs_set_int(NVS_SERVER_PORT_KEY, sp_port);

        utils_nvs_set_str(NVS_SIP_USERNAME, SIPuserName);
        utils_nvs_set_str(NVS_SIP_DATETIME, SIPdateTime);
        uart_write_string_ln("*SIP-OK#");
        tx_event_pending = 1;
    } else if(strncmp(pkt, "*SIP?#", 6) == 0){
        char buffer[350]; 
       sprintf(buffer,"*SIP,%s,%s,%s,%d#",SIPuserName,SIPdateTime,server_ip_addr,
                                        sp_port );

   
        uart_write_string_ln(buffer);
        tx_event_pending = 1;
    
    }
    else if (strncmp(pkt, "*ERASE#", 7) == 0){
        utils_nvs_erase_all();
        uart_write_string("*ERASE:OK#");
    }else if(strncmp(pkt, "*RESTART#", 9) == 0){
        uart_write_string("*RESTART:OK#");
        tx_event_pending = 1;
        vTaskDelay(2000/portTICK_PERIOD_MS);
        esp_restart();
    }else{
        uart_write_string_ln(pkt);
        int l = strlen(pkt);
        char buff[l+1];
        /*   *---#  */

        if(extractSubstring(pkt, buff) == true){
            int l2 = strlen(buff);
            char b[l2+3];
            sprintf(b, "*%s#", buff);
            tcp_ip_client_send_str(b);
            tx_event_pending = 1;
        }
    }
}

static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    for(;;) {
        //Waiting for UART event.
        if(xQueueReceive(uart0_queue, (void * )&event, (TickType_t)portMAX_DELAY)) {
            switch(event.type) {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.*/
                case UART_DATA:{
                    char arr[event.size + 1];
                    uart_read_bytes(EX_UART_NUM, arr, event.size, portMAX_DELAY);
                    arr[event.size] = '\0';
                    process_uart_packet(arr);
                    break;
                }
                    
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                //Event of UART RX break detected
                case UART_BREAK:
                    ESP_LOGI(TAG, "uart rx break");
                    break;
                //Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "uart parity error");
                    break;
                //Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG, "uart frame error");
                    break;
                //UART_PATTERN_DET
                case UART_PATTERN_DET:
                    ESP_LOGI(TAG, "[UART PATTERN DETECTED] ");
                    break;
                //Others
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
    }
    vTaskDelete(NULL);
}


void console_uart_init(){
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
    uart_param_config(EX_UART_NUM, &uart_config);
    uart_set_pin(MKM_IC_UART, MKM_IC_UART_TX, MKM_IC_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
//    uart_set_pin(EX_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 6, NULL);
}

void read_mac_address(){
    uint8_t macAddress[6];
    esp_err_t err = esp_read_mac(macAddress, ESP_MAC_WIFI_STA);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "*MAC Address: %02x:%02x:%02x:%02x:%02x:%02x#\n",
            macAddress[0], macAddress[1], macAddress[2],
            macAddress[3], macAddress[4], macAddress[5]);
        sprintf(MAC_ADDRESS_ESP, "%02X:%02X:%02X:%02X:%02X:%02X",
            macAddress[0], macAddress[1], macAddress[2],
            macAddress[3], macAddress[4], macAddress[5]);
    } else {
        ESP_LOGE(TAG, "Failed to read MAC address. Error code: %d\n", err);
    }
}


void sendHBT (void)
{
    char payload[100];
    for (;;) {
        ESP_LOGI(TAG, "*HBT,%s#", MAC_ADDRESS_ESP);
        sprintf(payload, "*HBT,%s#", MAC_ADDRESS_ESP); //actual when in production
        int err = send(sock, payload, strlen(payload), 0);
        // gpio_set_level(LedHBT, 1);
        // vTaskDelay(200/portTICK_PERIOD_MS);
        // gpio_set_level(LedHBT, 0);
        vTaskDelay(HBTDelay/portTICK_PERIOD_MS);
    }
}

void Out4094Byte (unsigned char value)
{
    uint8_t i,j;
    uint8_t OutputMap[9] = {99,6,0,4,5,2,3,1,99};
    uint8_t ReverseBitMap[8] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};


    j =0;
    for (i = 1 ; i< 8 ; i++)
    {
        if (value & (0x01<<i))
            j = 0x01 << (OutputMap[i]);
    }

    for (i = 0 ; i < 8 ; i++)
    {
        if (j && (ReverseBitMap[i]))  
            gpio_set_level(DAT, 1);
        else    
            gpio_set_level(DAT, 0);
        ets_delay_us(10);
        gpio_set_level(CLK, 1);
        ets_delay_us(10);
        gpio_set_level(CLK, 0);
    }
    ets_delay_us(10);
    gpio_set_level(STRB, 1);
    ets_delay_us(10);
    gpio_set_level(STRB, 0);
}

static void gpio_read_n_act(void* arg)
{
    int testCounter = 0;
    int BlinkMode = 0;
    char payload[100];
    for (;;)
    {
        if (gpio_get_level(ErasePin) == 0)
        {
            if (LastErasePinStatus == 1)    
            {
                ErasePinDebounce = 2000;        
                LastErasePinStatus = 0;
                ESP_LOGI(TAG,"*Eraseing Sense Started#");
            }
            else
            {
                if (ErasePinDebounce)
                { 
                    ErasePinDebounce = ErasePinDebounce-1;
                    if (ErasePinDebounce == 0)
                    {
                        ErasePinStatus = 0;
                        ESP_LOGI(TAG,"*Eraseing All Parameters#");
                        send(sock, "*ERASE-LOCAL#", strlen("*ERASE-LOCAL#"), 0);
                        utils_nvs_erase_all();
                        ESP_LOGI(TAG, "**************Restarting after 3 second******#");
                        send(sock, "*RST-OK#", strlen("*RST-OK#"), 0);
                        ESP_LOGI(TAG, "*RST-OK#");
                        vTaskDelay(3000/portTICK_PERIOD_MS);
                        esp_restart();
                    }
                }

            }
        }
        else
        {
            if (LastErasePinStatus == 0)
            {
                ErasePinDebounce = 200; 
                LastErasePinStatus = 0;
            }
            else
            {
                if (ErasePinDebounce)
                { 
                    ErasePinDebounce = ErasePinDebounce-1;
                    if (ErasePinDebounce == 0)
                        ErasePinStatus = 1;
                }

            }
        }
        if (gpio_get_level(CINHI) == 0)
        {
            INHInputValue = 0;        
        }
        else
        {
            INHInputValue = 1;        
            
        }
        if (PreviousINHValue != INHInputValue)
        {
            PreviousINHValue = INHInputValue;
            // if (gpio_get_level(JUMPER) == 0)
            // {
                sprintf(payload, "*INH,%d#",INHInputValue); 
                send(sock, payload, strlen(payload), 0);
            // }
        }
        InputPin = 0;
        if (gpio_get_level(ICH1) == 0)
        {
            InputPin = 1;
        }
        else if (gpio_get_level(ICH2) == 0)
        {
            InputPin = 2;
        }
        else if (gpio_get_level(ICH3) == 0)
        {
            InputPin = 3;
        }
        else if (gpio_get_level(ICH4) == 0)
        {
            InputPin = 4;
        }
        else if (gpio_get_level(ICH5) == 0)
        {
            InputPin = 5;
        }
        else if (gpio_get_level(ICH6) == 0)
        {
            InputPin = 6;
        }
        else if (gpio_get_level(ICH7) == 0)
        {
            InputPin = 7;
        }
        else 
        {
            InputPin = 0;
        }    
        if (InputPin == 0)
        {
            if (PulseStoppedDelay>0)
            {
                PulseStoppedDelay--;
                if (PulseStoppedDelay == 0)
                {
                    
                    CashTotals[LastInputPin-1] += TotalPulses;
                    if (LastInputPin == 1)
                        utils_nvs_set_int(NVS_CASH1_KEY, CashTotals[0]);
                    if (LastInputPin == 2)
                        utils_nvs_set_int(NVS_CASH2_KEY, CashTotals[1]);
                    if (LastInputPin == 3)
                        utils_nvs_set_int(NVS_CASH3_KEY, CashTotals[2]);
                    if (LastInputPin == 4)
                        utils_nvs_set_int(NVS_CASH4_KEY, CashTotals[3]);
                    if (LastInputPin == 5)
                        utils_nvs_set_int(NVS_CASH5_KEY, CashTotals[4]);
                    if (LastInputPin == 6)
                        utils_nvs_set_int(NVS_CASH6_KEY, CashTotals[5]);
                    if (LastInputPin == 7)
                        utils_nvs_set_int(NVS_CASH7_KEY, CashTotals[6]);

                    // ESP_LOGI("COIN","Input Pin %d Pulses %d",LastInputPin,TotalPulses);
                   if (gpio_get_level(JUMPER) == 0)
                   {
                        sprintf(payload, "*RP,%d,%d#",LastInputPin,TotalPulses); 
                        send(sock, payload, strlen(payload), 0);
                   }
                   // create same pules on same output pin 17-06-24
                   edges = TotalPulses * 2;
                   pin = LastInputPin;
                   sprintf(payload, "*RP,%d,%d#",LastInputPin,TotalPulses); 
                   uart_write_string(payload);
                   ESP_LOGI(TAG,"*RP,%d,%d#",LastInputPin,TotalPulses);

                   TotalPulses = 0;
                }
            }
        }
        if (LastValue != InputPin)
        {
            LastValue = InputPin;
            DebounceCount = 5;
        }
        else
        {
            if (DebounceCount)
            {
                DebounceCount--;
                if (DebounceCount == 0)
                {
                    if (InputPin == 0)
                    {
                    }
                    if (InputPin != 0)
                    {
                    TotalPulses++;                                      
                    PulseStoppedDelay = 100;
                    LastInputPin = InputPin;
                    }
                }
            }

        }
        vTaskDelay(5/portTICK_PERIOD_MS);
    }
}


// static void gpio_read_n_act_old(void* arg)
// {
//     char buff[50];
//     uint16_t Pin = 0;
//     uint16_t Counter = 0;
//     for (;;) {

//         x=0;
//         Pin = 0;
//         if (gpio_get_level(ICH1) == 0)
//         {
//             x=x+1;
//             Pin = 1;
//         }
//         if (gpio_get_level(ICH2) == 0)
//         {
//             x=x+2;
//             Pin = 2;
//         }
//         if (gpio_get_level(ICH3) == 0)
//         {
//             x=x+4;
//             Pin = 3;
//         }
//         if (gpio_get_level(ICH4) == 0)
//         {
//             x=x+8;
//             Pin = 4;
//         }
//         if (gpio_get_level(ICH5) == 0)
//         {
//             x=x+16;
//             Pin = 5;
//         }
//         if (gpio_get_level(ICH6) == 0)
//         {
//             x=x+32;
//             Pin = 6;
//         }
//         if (gpio_get_level(ICH7) == 0)
//         {
//             x=x+64;
//             Pin = 7;
//         }
//         if (x!= PreviousSwitchStatus)
//         {
//              PreviousSwitchStatus =x;
//              DebounceCount = 1;   
//              ESP_LOGI("COIN","Counter %d Input Value:%d Pin %d",Counter,SwitchStatus,Pin);
//              if (Pin == 0)
//                 Counter = 0;
//              else  
//                 Counter++;      
//         }   
//         else if (DebounceCount>0)
//         {
//             DebounceCount--;
//             if (DebounceCount == 0)
//             {
//                 SwitchStatus = PreviousSwitchStatus;
//                 if (SwitchStatus != 0)
//                 {
//                     // if firts pulse, start timeout
//                     if (PulseTimeOut == 0)
//                         PulseCount = 1;
//                     else    
//                         PulseCount++;   
//                     PulseTimeOut = 200; // 100  * delay
//                 }         
//                     Out4094Byte(SwitchStatus);    
//  //                   blinkLEDNumber = 2;
//                     //tcp_ip_client_send_str(buff);
                
//             }
//         }
//         if (PulseTimeOut>0)
//         {
//             PulseTimeOut--;
//             if (PulseTimeOut == 0) 
//             {
//                  PulseCount = 0;    
//             //    ESP_LOGI("COIN","*RP:%d:%d#",Pin,PulseCount);
//                 sprintf (buff, "*RP:%d:%d#",Pin,PulseCount);
//                 blinkLEDNumber = 2;
//             //     tcp_ip_client_send_str(buff);
//              }
//         }
//         vTaskDelay(5/portTICK_PERIOD_MS);
//     }
// }


void ICH_init()
{
    ESP_LOGI(TAG,"********Starting ICH INIT*************");
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
//    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set
    io_conf.pin_bit_mask = 1ULL << ErasePin | 1ULL << JUMPER | 1ULL << JUMPER2 |1ULL << CINHI | 1ULL << INH | 1ULL << ICH1 | 1ULL << ICH2 | 1ULL << ICH3 | 1ULL << ICH4 | 1ULL << ICH5 | 1ULL << ICH6| 1ULL << ICH7;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    //configure GPIO with the given settings
    gpio_config(&io_conf);


    // //create a queue to handle gpio event from isr
    // gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    // //start gpio task
// **************** skip reading input
    if (Production)
        xTaskCreate(gpio_read_n_act, "gpio_read_n_act", 2048, NULL, 10, NULL);

    //install gpio isr service
    // gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // //hook isr handler for specific gpio pin
    // gpio_isr_handler_add(ICH1, gpio_isr_handler, (void*) ICH1);
    // gpio_isr_handler_add(ICH2, gpio_isr_handler, (void*) ICH2);
    // gpio_isr_handler_add(ICH3, gpio_isr_handler, (void*) ICH3);
    // gpio_isr_handler_add(ICH4, gpio_isr_handler, (void*) ICH4);
    // gpio_isr_handler_add(ICH5, gpio_isr_handler, (void*) ICH5);
    // gpio_isr_handler_add(ICH6, gpio_isr_handler, (void*) ICH6);
    // gpio_isr_handler_add(ICH7, gpio_isr_handler, (void*) ICH7);
 }


void Out4094 (unsigned char value)
{
    uint8_t i,j;
    uint8_t OutputMap[9] = {99,6,0,4,5,2,3,1,99};
    j = OutputMap[value];
//    ESP_LOGI("OUT4094","pin %d",j);
    for (i = 0 ; i < 8 ; i++)
    {
        if (SignalPolarity == 0)
        {
            if (j == 7-i)
               gpio_set_level(DAT, 1);
            else    
                gpio_set_level(DAT, 0);
        }
        else
        {
            if (j == 7-i)
               gpio_set_level(DAT, 0);
            else    
                gpio_set_level(DAT, 1);
        }

        ets_delay_us(10);
        gpio_set_level(CLK, 1);
        ets_delay_us(10);
        gpio_set_level(CLK, 0);
    }
    ets_delay_us(10);
    gpio_set_level(STRB, 1);
    ets_delay_us(10);
    gpio_set_level(STRB, 0);
    // if (value<7)
    //     ESP_LOGI("OUT4094","Start Pulse %d is %lu",edges,xTaskGetTickCount());
    // else
    //     ESP_LOGI("OUT4094","End Pulses %d is %lu",edges,xTaskGetTickCount());

}

// generate pulses in background
// as soon as pulses value is non zero - generate 1 pulse and decrement pulses by 1 

            // if (edges%2 == 0)
            // {
            //     Out4094(pin);
            //     ESP_LOGI("OUT4094","Start Pulse %d is %lu",edges,xTaskGetTickCount());
            // }
            // else
            // {    
            //     Out4094(8);
            //     ESP_LOGI("OUT4094","End Pulse %d is %lu",edges,xTaskGetTickCount());
            // }

// blink LED as per number - set led on, wait, led off, clear led number
void BlinkLED (void)
{
    for (;;)
    {
        if (blinkLEDNumber>0)
        {
            if (blinkLEDNumber==1)
            {
                gpio_set_level(L1, 1);
                vTaskDelay(500/portTICK_PERIOD_MS);
                gpio_set_level(L1, 0);
                blinkLEDNumber = 0;
            }
            if (blinkLEDNumber==2)
            {
                gpio_set_level(L2, 1);
                vTaskDelay(500/portTICK_PERIOD_MS);
                gpio_set_level(L2, 0);
                blinkLEDNumber = 0;
            }
            if (blinkLEDNumber==3)
            {
                gpio_set_level(L1, 1);
                vTaskDelay(500/portTICK_PERIOD_MS);
                gpio_set_level(L3, 0);
                blinkLEDNumber = 0;
            }
        }
        vTaskDelay(500/portTICK_PERIOD_MS);
    }
}


void GeneratePulsesInBackGround (void)
{
    for (;;)
    {
        if (edges)
        {
            if (edges%2 == 0)
            {
                Out4094(pin);
            }
            else
            {    
                Out4094(8);
            }
            edges--;
            if (edges == 0)
                 ESP_LOGI("GenPulse","Pulse Width is %d ",pulseWitdh);

        }
        vTaskDelay(pulseWitdh/portTICK_PERIOD_MS);
    }
}

void TestCoin (void)
{
    for (;;) 
    {
        // pin++;
        // if (pin>7)
            pin = 4;
        edges = 2;    
         ESP_LOGI("TestCoin","Pin Number %d ",pin);
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}
void Test4094 (void)
{
    for (;;) 
    {
        Test4094Count++;
        if (Test4094Count == 8)
            Test4094Count = 0;

        gpio_set_level(L1, 0);
        gpio_set_level(L2, 0);
        gpio_set_level(L3, 0);
        if ((Test4094Count == 0) || (Test4094Count == 3))
            gpio_set_level(L1, 1);
        if ((Test4094Count == 1) || (Test4094Count == 4))
            gpio_set_level(L2, 1);
        if ((Test4094Count == 2) || (Test4094Count == 5))
            gpio_set_level(L3, 1);

        Out4094(Test4094Count);  
        ESP_LOGI(TAG, "Pulse 4094 %d",Test4094Count);  
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}

void s2p_init(){
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set
    io_conf.pin_bit_mask = 1ULL << STRB | 1ULL << CLK | 1ULL << DAT | 1ULL << CINHO | 1ULL << L1 | 1ULL << L2 | 1ULL << L3 ;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    gpio_set_level(STRB, 0);
    gpio_set_level(CLK, 0);
    gpio_set_level(DAT, 0);
    gpio_set_level(L1, 0);
    gpio_set_level(L2, 0);
    gpio_set_level(L3, 0);
    if (INHOutputValue != 0)
    {
        INHOutputValue = 1;
        gpio_set_level(CINHO, 0);
    }
    else
    {
        gpio_set_level(CINHO, 1);
    }

    
    Test4094Count = 0;
    ESP_LOGI(TAG, "4094 IOs,RGB initialised");  
    xTaskCreate(GeneratePulsesInBackGround, "GeneratePulsesInBackGround", 2048, NULL, 9, NULL);

}




void app_main(void)
{
    //Initialize NVS
    //esp_log_level_set("*", ESP_LOG_NONE);
    // set totals to 0
    for (int i = 0 ; i < 7 ; i++)
    {
        Totals[i] = 0;
        CashTotals[i] = 0;
    }   
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "*FW:%s#",FWVersion);
    ESP_LOGI(TAG, "================================");
    utils_nvs_init();
    status_leds_init();
    console_uart_init();
    uart_write_string(FWVersion);
    read_mac_address();
    xTaskCreate(tcpip_client_task, "tcpip_client_task", 8192, NULL, 7, NULL);
   
    ESP_LOGI(TAG, "*Starting ICH#");
    ICH_init();
    ESP_LOGI(TAG, "*Starting WiFi#");
    wifi_init_sta();
    ESP_LOGI(TAG, "*Starting S2P#");
    s2p_init();
    ESP_LOGI(TAG, "*Clearing 4094 Output#");
    Out4094(0x00);; // set all outputs inactive
    xTaskCreate(sendHBT, "sendHBT", 2048, NULL, 6, NULL);
    xTaskCreate(BlinkLED, "BlinkLED", 2048, NULL, 6, NULL);
    if (!Production)
        xTaskCreate(TestCoin, "TestCoin", 2048, NULL, 6, NULL);
    for (;;) 
    {
        vTaskDelay(100);   
    }
}