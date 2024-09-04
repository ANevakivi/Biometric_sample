/**
 * @file actionMain.h
 * @author Markku Heiskari
 * @brief Application main header file.
 * 
 * @copyright Copyright (c) 2023 Creoir Oy
 * 
 */

#ifndef __appmain_h
#define __appmain_h

#if !defined(_MSC_VER)
#include <semaphore.h>
#include <pthread.h>
#else
#pragma comment(lib, "Ws2_32.lib")
#endif
#include "mt_mutex.h"
#include "mt_semaphore.h"
#include "mosquitto.h"
#include "cJSON.h"


/********************************************************************
  GLOBAL DEFINES
********************************************************************/
#define APP_VERSION_MAJOR               1         //!< Defines the application version number major part (major.minor.build)
#define APP_VERSION_MINOR               0         //!< Defines the application version number minor part (major.minor.build)
#define APP_VERSION_BUILD               6         //!< Defines the application version number build part (major.minor.build)

#define DBG_VERBOSE                     1         //!< dbg_out() message category for verbose messages. Usually used for debugging
#define DBG_NORM                        2         //!< dbg_out() message category for normal messages.
#define DBG_NOTE                        4         //!< dbg_out() message category for notable messages.
#define DBG_IMPORTANT                   8         //!< dbg_out() message category for important messages.
#define DBG_ERROR                       16        //!< dbg_out() message category for error messages. 
#define DBG_FATAL                       32        //!< dbg_out() message category for fatal error messages.
#define DBG_MQTT                        64        //!< dbg_out() message category for MQTT messages. Usually used for MQTT debugging

#define MQTT_HOST_ADDRESS               "localhost"   //!< Default ip address for MQTT broker
#define MQTT_HOST_PORT                  "1883"        //!< Default ip port for MQTT broker

#define MQTT_SEND_TOPIC_SIZE            512         //!< Maximum size of MQTT send buffer. Increase if longer MQTT topics are used
#define MQTT_SEND_PAYLOAD_SIZE          32768       //!< Maximum size of outgoing MQTT payload


#if defined(_MSC_VER)
#define MQTT_SEND_MTX CRITICAL_SECTION
#define MQTT_COND_VAR CONDITION_VARIABLE
#else
#define MQTT_SEND_MTX pthread_mutex_t
#define MQTT_COND_VAR pthread_cond_t
#endif

/********************************************************************
  ENUMS and TYPES
********************************************************************/

/********************************************************************
  TYPEDEFS
********************************************************************/

/**
 * @brief Internal data block for MQTT messages
 * 
 */
typedef struct {
  char *pTopic;       //!< Ptr to MQTT topic
  char *pPayload;     //!< Ptr to MQTT payload
  int  dataSent;      //!< 0=if this block is not sent yet
} mqtt_Data_type;     //!< Internal data block for MQTT messages


/**
 * @brief Event loop event types.
 * Application event loop is the main loop where things happen in this application.
 * Each event type must be enumareted here. 
 * 
 */
typedef enum
{
  EVT_MQTT_WAKEWORD,                    //!< Wakeword topic received over MQTT
  EVT_MQTT_PING,                        //!< Ping received over MQTT -topic
  EVT_MQTT_PONG,                        //!< Pong received over MQTT -topic
  EVT_MQTT_INTENT_RECOGNIZED,           //!< ASR recognition result received
  EVT_MQTT_INTENT_NOT_RECOGNIZED,       //!< ASR recognition failure occurred
  EVT_KEYPRESS,                         //!< Keyboard event
  EVT_STARTUP,                          //!< Application startup
  EVT_MQTT_BIOM_IDENTIFICATION,         //!< Biometric identification
  EVT_APP_STOP                          //!< Application stop requested over MQTT
}APPLICATION_EVENT;


/**
 * @brief Event loop event data structure
 * 
 */
typedef struct
{
  char *payloadPtr;           //!< Pointer to data structure if allocated from heap (bigger than 10k). Must be released by handler.
  char topicPayload[10240];   //!< 10k storage for smaller data. No need to be released by handler
}APPLICATION_EVENTDATA;



/**
 * @brief Linked list of events in application event queue
 * 
 */
typedef struct EVENTNODE
{
  APPLICATION_EVENT        eventType;   //!< This event
  APPLICATION_EVENTDATA    eventData;   //!< Data structure for the event
  struct EVENTNODE *next;               //!< Pointer to next event in linked chain
}EVENTNODE_T;


/**
 * @brief Application scope global variables stored in heap. Pointer by pGlobalData-> pointer.
 * 
 */
typedef struct {
  struct mosquitto      *mosquittoClient;   //!< Handle to Mosquitto client
  short                 mqttConnected;      //!< Is MQTT connected
  char                  mqttHost[64];       //!< MQTT broker IP address
  char                  mqttPort[8];        //!< MQTT broker port
  MQTT_SEND_MTX         mqttSendMutex;      //!< Mutex to protect MQTT message memory
  MQTT_COND_VAR         mqttSend_cv;        //!< Condition variable for the send mutex
  mqtt_Data_type        mqttSharedData;     //!< Pointers to topic and payload
  short                 syslog;             //!< Output to: 0=stdout, 1=syslog, 2=stdout and syslog
  EVENTNODE_T           *eventListHead;     //!< Pointer to event linked list head
  EVENTNODE_T           *eventListTail;     //!< Pointer to event linked list tail
  MT_SEMAPHORE          *eventSemap;        //!< Event queue semaphore
  MQTT_SEND_MTX         *eventMutex;        //!< Mutex to protect event queue data
  unsigned int          debugMask;          //!< Debug output mask
  int                   mutexError;         //!< For debugging. 0=ok, 1=MQTT mutex send permission has failed.
  short                 appExit;           //!< If nonzero, application is terminating.
  #if defined(_MSC_VER)
  HWND                  hConsole;           /*!< Console handle (in windows) */
  #endif
} globalData_type;


/********************************************************************
  PROTOTYPES
********************************************************************/

/**
 * @brief Pushes event to application event queue. Event usually has an eventdata data block with it
 * 
 * @param event The event. See enum APPLICATION_EVENT
 * @param eventData Event data block. See definition of APPLICATION_EVENTDATA
 */
void pushEvent( APPLICATION_EVENT event, const APPLICATION_EVENTDATA *eventData );

/* MQTT handler function prototypes */



/**
 * @brief Sample function to handle wakeword event
 * 
 * @param pTopic MQTT topic name
 * @param pData  MQTT payload
 * @return int 0=OK, nonzero=error
 */
int handle_MQTTonWakeword( const char* pTopic, const char *pData );


/**
 * @brief Sample function to handle OVS topic creoir/asr/intentRecognized.
 * This topic is launched every time a valid recognition result occurs.
 * 
 * @param pTopic MQTT topic name
 * @param pData  MQTT payload
 * @return int 0=OK, nonzero=error
 */
int handle_MQTTintentRecognized(const char* pTopic, const char* pData);


/**
 * @brief Sample function to handle OVS topic creoir/asr/intentNotRecognized.
 * This topic is launched every time a recognition is rejected.
 * 
 * @param pTopic MQTT topic name
 * @param pData  MQTT payload
 * @return int 0=OK, nonzero=error
 */
int handle_MQTTintentNotRecognized(const char* pTopic, const char* pData);


/**
 * @brief Sample function to handle EdgeVUI topic creoir/biometrics/identification
 * This topic is launched every time a recognition is rejected.
 * 
 * @param pTopic MQTT topic name
 * @param pData  MQTT payload
 * @return int 0=OK, nonzero=error
 */
int handle_MQTTuserIdentified(const char* pTopic, const char* pData);


/**
 * @brief Sample function to handle topic creoir/app/stop. Stops the application.
 * 
 * @param pTopic MQTT topic name
 * @param pData  MQTT payload
 * @return int 0=OK, nonzero=error
 */
int handleMQTT_app_stop( const char* pTopic, const char *pData );


/**
 * @brief Initializes the MQTT client
 * 
 * @return int 0=OK, nonzero=error
 */
int mqtt_interface_init( void );

/**
 * @brief Structure with MQTT topics and related function pointers to handler for this topic
 * 
 */
typedef struct MQTT_actions {
  const char* topic;                                      //!< Incoming MQTT topic name
  int          (*function)(const char* ,const char*);     //!< Function pointer to be called when this MQTT topic is received
} mqtt_action;

/* Link MQTT topics and related handler functions */
static const mqtt_action mqttActionRegister[] = {
  {"creoir/asr/wakewordDetected",       &handle_MQTTonWakeword},
  {"creoir/asr/intentRecognized",       &handle_MQTTintentRecognized},
  {"creoir/asr/intentNotRecognized",    &handle_MQTTintentNotRecognized},
  {"creoir/biometrics/identification",  &handle_MQTTuserIdentified},
  {"creoir/app/stop",                   &handleMQTT_app_stop},
};

#endif

// End of actionMain.h
