/**
 * @file util.h
 * @author Markku Heiskari
 * @brief Utility module header
 * @date 2023-02-10
 * 
 * @copyright Copyright (c) 2023 Creoir Oy
 * 
 */
 

#ifndef __util_h
#define __util_h

#if !defined(_MSC_VER)
#include <syslog.h>
#endif
#include "actionMain.h"

/********************************************************************
  PROTOTYPES
********************************************************************/


/**
 * @brief Debug output function. All output should be routed through this function.
 * 
 * @param type Message urgency level. See main header for values.
 * @param format Format string of output. See sprintf() documentation
 * @param ... Parameter values for format string. See sprintf() documentation
 * 
 * @returns void
 */
void dbg_out( int type, const char *format, ... );

/**
 * @brief Wrapper for mutex creation. Compiles on linux and windows.
 * 
 * @param pMutex Handle to mutex
 * 
 * @returns void
 */
void InitializeMQTTsendMutex(MQTT_SEND_MTX* pMutex);

/**
 * @brief Wrapper for acquiring mutex lock. Compiles on linux and windows
 * 
 * @param pMutex Handle to mutex.
 * @see InitializeMQTTsendMutex
 * @return int 0=OK, nonzero=error
 */
int  request_mutex_lock(MQTT_SEND_MTX* pMutex);

/**
 * @brief Wrapper for releasing mutex lock. Compiles on linux and windows
 * 
 * @param pMutex Handle to mutex.
 * @see InitializeMQTTsendMutex
 * @return int 0=OK, nonzero=error
 */
int  release_mutex_lock(MQTT_SEND_MTX* pMutex);

/**
 * @brief Compare MQTT topics. Accept # wildcard.
 * 
 * @param haystack Expected topic (with possible wildcard)
 * @param needle Topic to compare to expected
 * @return int 1=Match, 0=No match
 */
int  mqtt_topic_compare(const char* haystack, const char* needle);

#if defined(_MSC_VER)
  DWORD WINAPI mqtt_sender(LPVOID pVoid);
  DWORD WINAPI mqtt_client_refresher(LPVOID mqttClient);
#else
  void* mqtt_sender(void* mqttClient);
  void* mqtt_client_refresher(void* mqttClient);
#endif


/**
 * @brief Gets a mutex for protected MQTT data block.
  If the block contains unsent data, releases the lock and tries
  again after a short while.
  Returns once mutex is got for "empty" data block
 * 
 * @param pMutex Pointer to mutex object
 * @param pCaller Name of calling function. (For debug purposes)
 * @return int 0=Success, Negative=Error
 */
int   getMQTTsendAccess(MQTT_SEND_MTX* pMutex, const char* pCaller);


/**
 * @brief Sends data stored in MQTT global data area.
 * 
 * @param pCaller calling function name (__FUNCTION__)
 * @return int 0=Success, negative=error 
 */
int   sendMQTTtopic(const char* pCaller);


#endif

/* EOF *************************************************************/

