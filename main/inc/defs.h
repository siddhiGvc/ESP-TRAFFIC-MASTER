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
#define INHIBITLevel 1


#define EX_UART_NUM UART_NUM_2
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)


