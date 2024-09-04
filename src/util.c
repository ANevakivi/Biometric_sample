/********************************************************************

  Utility functions

  Author: Markku Heiskari
  Version history in github

  (C) Copyright 2023, Creoir Oy

********************************************************************/

/********************************************************************
  INCLUDES
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#if !defined(_MSC_VER)
#include <unistd.h>
#include <sys/time.h>
#else
#include <windows.h>
#endif
#include <string.h>
#include <signal.h>
#include <time.h>
#include "actionMain.h"
#include "util.h"

/********************************************************************
  LOCAL PROTOTYPES
********************************************************************/
static int  wakeMQTTsender(MQTT_COND_VAR* pConditionVariable);

/********************************************************************
  DEFINES
********************************************************************/

extern globalData_type *pGlobalData;



/********************************************************************
  FUNCTIONS
********************************************************************/


/********************************************************************
  wakeMQTTsender()

  Parameters: Pointer to mutex / critical section
  Returns:    0 = ok, nonzero = error code.

  Description:
  Wrapper for waking conditional variable

********************************************************************/
static int wakeMQTTsender(MQTT_COND_VAR* pConditionVariable) {
  #if defined(_MSC_VER)
    WakeConditionVariable(pConditionVariable);
  #else
    pthread_cond_signal(pConditionVariable);
  #endif
  return 0;
} // End of wakeMQTTsender()


/********************************************************************
  request_mutex_lock()

  Parameters: Pointer to mutex / critical section
  Returns:    0 = ok, nonzero = error code.

  Description:
  Wrapper for acquiring mutex lock. Compiles on linux and windows

********************************************************************/
int request_mutex_lock(MQTT_SEND_MTX* pMutex) {
#if defined(_MSC_VER)
  EnterCriticalSection(pMutex);
  return 0;
#else
  return(pthread_mutex_lock(pMutex));
#endif
}


/********************************************************************
  release_mutex_lock()

  Parameters: Pointer to mutex / critical section
  Returns:    0 = ok, nonzero = error code.

  Description:
  Wrapper for mutex unlock. Compiles on linux and windows

********************************************************************/
int release_mutex_lock(MQTT_SEND_MTX* pMutex) {
#if defined(_MSC_VER)
  LeaveCriticalSection(pMutex);
  return 0;
#else
  return(pthread_mutex_unlock(pMutex));
#endif
}


/********************************************************************
  InitializeMQTTsendMutex()

  Parameters: Pointer to mutex
  Returns:    void

  Description:
  Wrapper for mutex creation. Compiles on linux and windows

********************************************************************/
void InitializeMQTTsendMutex(MQTT_SEND_MTX* pMutex) {
#if defined(_MSC_VER)
  InitializeCriticalSection(pMutex);
#else
  pthread_mutex_init(pMutex, NULL);
#endif
  return;
}


/********************************************************************
  mqtt_topic_compare()

  Parameters: (in)  Expected topic (with possible wildcard)
              (in)  Topic to compare to expected.

  Returns:    1=Match, 0= no match

  Description:
  Compare MQTT topics. Accept # wildcard.

********************************************************************/
int mqtt_topic_compare(const char* haystack, const char* needle) {
  int iPos = 0;
  int iLen = strlen(haystack);

  while (needle[iPos]) {
    if (needle[iPos] != haystack[iPos] && haystack[iPos] != '#')return 0;   // Needle not found from haystack
    if (haystack[iPos] == '#')return 1; // If match so far and now hit wildcard, this matches
    if (haystack[iPos] == 0)return 0;   // Haystack too short
    iPos++;
  } // End while

  if (strlen(needle) != strlen(haystack))return 0; // Needle shorter than haystack

  // If reaching here, strings are exact match
  return 1;

} // End of mqtt_topic_compare()


#if defined(_MSC_VER)
/********************************************************************
  gettimeofday()

  Parameters: Address to store the time, Timezone
  Returns:    0 = ok

  Description:
  Wrapper for windows to get time using same call as in linux

********************************************************************/
int gettimeofday(struct timeval* tp, struct timezone* tzp)
{
  // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
  // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
  // until 00:00:00 January 1, 1970 
  static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

  SYSTEMTIME  system_time;
  FILETIME    file_time;
  uint64_t    time;

  GetSystemTime(&system_time);
  SystemTimeToFileTime(&system_time, &file_time);
  time = ((uint64_t)file_time.dwLowDateTime);
  time += ((uint64_t)file_time.dwHighDateTime) << 32;

  tp->tv_sec = (time_t)((time - EPOCH) / 10000000L);
  tp->tv_usec = (time_t)(system_time.wMilliseconds * 1000);
  return 0;
}
#endif


/********************************************************************
  dbg_out()

  Parameters: [in]  Message type
              [in]  Ptr to format string
              [in]  Parameters according to format string
  Returns:    0 = ok, nonzero = error code.

  Description:
  Debug output function. All output should be routed through this
  function.

********************************************************************/
void dbg_out(int type, const char* format, ...) {
  static MQTT_SEND_MTX* pLogMutex = NULL;
  char   message[2048];
  char   szType[32];
  time_t epochTime;
  struct tm tm;
  struct timeval currTimeMs;
  va_list argp;
  va_start(argp, *format);

  if (NULL == pLogMutex) {
    pLogMutex = malloc(sizeof(MQTT_SEND_MTX));
    InitializeMQTTsendMutex(pLogMutex);
  }

  gettimeofday(&currTimeMs, NULL);
  #if !defined(_MSC_VER)
    tm = *localtime(&currTimeMs.tv_sec);
  #else
    epochTime = currTimeMs.tv_sec;
    tm = *localtime(&epochTime);
  #endif

  switch (type) {
  case DBG_VERBOSE:
    strcpy(szType, "VRBOSE ");
    break;

  case DBG_MQTT:
    strcpy(szType, "MQTT   ");
    break;

  case DBG_NORM:
    strcpy(szType, "INFO   ");
    break;

  case DBG_NOTE:
    strcpy(szType, "NOTE   ");
    break;

  case DBG_ERROR:
#if defined(_MSC_VER)
    strcpy(szType, "ERROR  ");
#else
    strcpy(szType, "\033[1;31mERROR \033[1;33m ");
#endif
    break;

  case DBG_FATAL:
#if defined(_MSC_VER)
    strcpy(szType, "FATAL  ");
#else
    strcpy(szType, "\033[1;31mFATAL \033[1;33m ");
#endif
    break;

  case DBG_IMPORTANT:
#if defined(_MSC_VER)
    strcpy(szType, "NOTICE ");
#else
    strcpy(szType, "\033[1;33mNOTICE\033[1;33m ");
#endif
    break;

  default:
    strcpy(szType, "UNKNWN ");
    break;
  } // End switch()


  // Format the output
  vsnprintf(message, 2048, format, argp);
  //sprintf( pLogMessage,"%02d:%02d:%02d.%03d %s %s", tm.tm_hour, tm.tm_min, tm.tm_sec,currTimeMs.tv_usec/1000, szType, message );

#if !defined(_MSC_VER)
  // If syslog higher than 0, write to syslog
  if (pGlobalData->syslog) {
    switch (type) {
    case DBG_VERBOSE:
    case DBG_NORM:
      // Mask only normal and verbose messages
      if (pGlobalData->debugMask & type)syslog(LOG_NOTICE, "%s", message);
      break;

    case DBG_NOTE:
    case DBG_IMPORTANT:
      syslog(LOG_ALERT, "%s", message);
      break;

    case DBG_ERROR:
      syslog(LOG_ERR, "%s", message);
      break;

    case DBG_FATAL:
      syslog(LOG_CRIT, "%s", message);
      break;

    default:
      syslog(LOG_ALERT, "%s", message);
      break;
    }  // End switch
  }  // End if(syslog)
#endif


// If output wanted only to console - or console and syslog
  if (pGlobalData->syslog == 0 || pGlobalData->syslog == 2) {
    // Print only if debug mask allows
    if ((pGlobalData->debugMask & type) || (type == DBG_FATAL)) {
      request_mutex_lock(pLogMutex);
      printf("%02d:%02d:%02d.%03d %s", tm.tm_hour, tm.tm_min, tm.tm_sec, (int)(currTimeMs.tv_usec / 1000), szType);
#if defined(_MSC_VER)

      if (type == DBG_ERROR || type == DBG_FATAL) {
        SetConsoleTextAttribute(pGlobalData->hConsole, FOREGROUND_RED + FOREGROUND_INTENSITY);
        printf("%s", message);

        SetConsoleTextAttribute(pGlobalData->hConsole, FOREGROUND_RED + FOREGROUND_GREEN + FOREGROUND_BLUE);
      }
      else if (type == DBG_NOTE || type == DBG_IMPORTANT) {
        SetConsoleTextAttribute(pGlobalData->hConsole, FOREGROUND_RED + FOREGROUND_GREEN + FOREGROUND_INTENSITY);
        printf("%s", message);

        SetConsoleTextAttribute(pGlobalData->hConsole, FOREGROUND_RED + FOREGROUND_GREEN + FOREGROUND_BLUE);
      }
      else {
        SetConsoleTextAttribute(pGlobalData->hConsole, FOREGROUND_RED + FOREGROUND_GREEN + FOREGROUND_BLUE);
        printf("%s", message);
      }
#else
      if (type == DBG_ERROR || type == DBG_FATAL)printf("\033[1;31m");
      if (DBG_NOTE == type)printf("\033[1;33m");
      if (DBG_IMPORTANT == type)printf("\033[1;31m");
      if (DBG_NOTE == type || DBG_IMPORTANT == type || DBG_ERROR == type) {
        // Need to add esc sequence before newline to avoid problems in console output
        char* plfPosition;
        plfPosition = strstr(message, "\n");
        if (plfPosition) {
          strcpy(plfPosition, "\033[0m \n");
        }
        else {
          // No newline in message
          strcat(message, "\033[0m");
        }
        printf("%s", message);
      }
      else {
        printf("%s", message);
      }
#endif
      release_mutex_lock(pLogMutex);
    } // if (mask)
  } // End if( syslog 0|2 )

  va_end(argp);
} // End of dbg_out()




/********************************************************************
  mqtt_sender()

  Parameters: (in)  pointer to mqttClient data
  Returns:    void.

  Description:
  Sends MQTT messages from shared buffer whenever triggered.

********************************************************************/
#if defined(_MSC_VER)
  DWORD WINAPI mqtt_sender(LPVOID pVoid)
#else
  void* mqtt_sender(void* mqttClient)
#endif
{
  int  iRet;
  long mqttMaxDuration = 0;
  long mqttDuration;
  #if defined(_MSC_VER)
    SYSTEMTIME mqttStartTime;
    SYSTEMTIME mqttEndTime;
  #else
    struct timeval mqttStartTime;
    struct timeval mqttEndTime;
  #endif
  int urgency;


  dbg_out( DBG_MQTT,"MQTT sender thread started.\n" );
  while(1){
    // Wait for data to be sent.
    
    // Lock the mqtt send mutex
    //pthread_mutex_lock( &pGlobalData->mqttSendMutex );
    iRet = request_mutex_lock(&pGlobalData->mqttSendMutex);
    if (iRet != 0) {
      dbg_out(DBG_ERROR, "MQTT sender mutex lock return value:%d\n", iRet);
    }
    else {
      dbg_out(DBG_MQTT, "Mutex locked for sender thread.\n");
    }

    // If conditional variable signalled while waiting for mutex lock above, the flag "dataSent" is zero
    // In this case no waiting for conditional variable should be done
    if( pGlobalData->mqttSharedData.dataSent == 0 ){ 
      dbg_out( DBG_MQTT,"MQTT sender thread detects unsent data block. Not waiting for conditional variable.\n" );
    }else{
      // Wait for conditional variable (this unlocks the mutex while it waits)
      dbg_out( DBG_MQTT,"MQTT sender thread ready to send again. Waiting for a new post...\n" );
      // pthread_cond_wait( &pGlobalData->mqttSend_cv, &pGlobalData->mqttSendMutex ); 
      #if defined(_MSC_VER)
        dbg_out(DBG_MQTT, "Conditional variable %x wait. Mutex %x\n", pGlobalData->mqttSend_cv, &pGlobalData->mqttSendMutex);
        SleepConditionVariableCS(&pGlobalData->mqttSend_cv, &pGlobalData->mqttSendMutex, INFINITE);
      #else
        iRet = pthread_cond_wait(&pGlobalData->mqttSend_cv, &pGlobalData->mqttSendMutex);
      #endif
    }

    dbg_out( DBG_MQTT,"MQTT sender thread activated.\n" );

    // Compose the message based on data in shared memory
    dbg_out( DBG_MQTT,"Posting topic [%s] with payload [%s]\n",
             pGlobalData->mqttSharedData.pTopic,
             pGlobalData->mqttSharedData.pPayload );
 
    #if defined(_MSC_VER)
      GetSystemTime(&mqttStartTime);
    #else
      gettimeofday(&mqttStartTime, NULL);
    #endif

    iRet = mosquitto_publish( pGlobalData->mosquittoClient,NULL, pGlobalData->mqttSharedData.pTopic, 
                              strlen(pGlobalData->mqttSharedData.pPayload), pGlobalData->mqttSharedData.pPayload,2,false );
    if( MOSQ_ERR_SUCCESS != iRet ){
      dbg_out( DBG_ERROR,"MQTT publish error: %s\n", mosquitto_strerror(iRet) );
    }


    #if defined(_MSC_VER)
      GetSystemTime(&mqttEndTime);
      mqttDuration = ((mqttEndTime.wMilliseconds) + (mqttEndTime.wSecond * 1000) + (mqttEndTime.wMinute * 60000) +
        (mqttEndTime.wHour * 60 * 60000) + (mqttEndTime.wDay * 24 * 60 * 60000) + (mqttEndTime.wMonth * 30 * 24 * 60 * 60000)) -
        ((mqttStartTime.wMilliseconds) + (mqttStartTime.wSecond * 1000) + (mqttStartTime.wMinute * 60000) +
          (mqttStartTime.wHour * 60 * 60000) + (mqttStartTime.wDay * 24 * 60 * 60000) + (mqttStartTime.wMonth * 30 * 24 * 60 * 60000));
    #else
      gettimeofday(&mqttEndTime, NULL);
      mqttDuration = ((mqttEndTime.tv_sec * 1000000) + (mqttEndTime.tv_usec)) -
        ((mqttStartTime.tv_sec * 1000000) + (mqttStartTime.tv_usec));
    #endif

    if (mqttDuration > mqttMaxDuration)mqttMaxDuration = mqttDuration;
    urgency = DBG_MQTT;
    if (mqttDuration > 1000000 || (mqttDuration == mqttMaxDuration && mqttDuration > 25)) urgency = DBG_NOTE;
    dbg_out(urgency, "MQTT publish took %d u/mSecs. Max so far is %d u/mSecs.\n", mqttDuration, mqttMaxDuration);

    if( pGlobalData->mqttSharedData.dataSent == 0)dbg_out( DBG_MQTT,"Setting data sent -flag TRUE\n" );
    pGlobalData->mqttSharedData.dataSent=1;

    // Unlock the mutex
    //pthread_mutex_unlock( &pGlobalData->mqttSendMutex );
    release_mutex_lock(&pGlobalData->mqttSendMutex);

  }  // End while(1)
  #if defined(_MSC_VER)
    return 0;
  #else
    return NULL;
  #endif
}  // End of mqtt_sender()


/********************************************************************
  getMQTTsendAccess()

  Parameters: (in)  pointer pthread_mutex_t
              (in)  calling function name (__FUNCTION__)
  Returns:    0=Success, negative=error

  Description:
  Gets a mutex for protected MQTT data block.
  If the block contains unsent data, releases the lock and tries
  again after a short while.
  Returns once mutex is got for "empty" data block

********************************************************************/
int getMQTTsendAccess(MQTT_SEND_MTX* pMutex, const char* pCaller) {
  int i = 100;

  dbg_out(DBG_MQTT, "getMQTTsendAccess(%x) asked by %s()\n", pMutex, pCaller);
  // Loop until have "empty" data block or failed for 100 times.
  while (i) {
    request_mutex_lock(pMutex);
    if (pGlobalData->mqttSharedData.dataSent == 1) {
      dbg_out(DBG_MQTT, "getMQTTsendAccess() mutex locked for %s().\n", pCaller);
      return 0;
    }
    // Buffer is occupied. Release mutex and wait for the buffer to be consumed
    release_mutex_lock(pMutex);
    #if defined(_MSC_VER)
      Sleep(100); // Wait 100ms
    #else
      usleep(100000U); // Wait 100ms
    #endif
    i--;
  }
  pGlobalData->mutexError++;
  dbg_out(DBG_ERROR, "%s(%x) FAILED to get mutex lock for %s()\n",__FUNCTION__, pMutex, pCaller);
  return -1;

}  // End of getMQTTsendAccess()


/********************************************************************
  sendMQTTtopic()

  Parameters: (in)  pointer pthread_mutex_t
              (in)  calling function name (__FUNCTION__)
  Returns:    0=Success, negative=error

  Description:
  Sends data stored in MQTT global data area.

********************************************************************/
int sendMQTTtopic( const char* pCaller ) {

  dbg_out(DBG_MQTT, "%s() Sending MQTT topic requested by %s\n",__FUNCTION__, pCaller);
  pGlobalData->mqttSharedData.dataSent = 0;
  wakeMQTTsender(&pGlobalData->mqttSend_cv);

  // Unlock the mutex
  release_mutex_lock(&pGlobalData->mqttSendMutex);
  return 0;
} // End of sendMQTTtopic


#if !defined(_MSC_VER)
/********************************************************************
  initTimer()

  Parameters: (in)  pointer to timer structure
              (in)  ptr to function to call when timer expires
              (in)  signal to be generated when timer expires
  Returns:    0 = success, other = error code.

  Description:
  Initializes a timer

********************************************************************/
int initTimer( timer_t *pTimer, void *pCallBack, int signal )
{
  int ret = 0;
  struct sigaction sa;
  struct sigevent sev;
  sigset_t mask;
  
  /* Establish handler for timer signal */
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = pCallBack;
  sigemptyset(&sa.sa_mask);
  sigaction(signal, &sa, NULL);

  /* Create the timer */
  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = signal;
  sev.sigev_value.sival_ptr = pTimer;

  if (timer_create( CLOCK_REALTIME, &sev, pTimer ) == -1)
  {
    dbg_out( DBG_ERROR, "ERROR: initTimer() failed.\n");
    ret = -1;
  }

  return ret;
}  // End of initTimer()


/********************************************************************
  startTimer()

  Parameters: (in)  pointer to timer structure
              (in)  Timeout in milliseconds
  Returns:    0 = success, other = error code.

  Description:
  Starts the specified timer-

********************************************************************/
int startTimer( timer_t *pTimer, int timeoutMs )
{
  int ret = 0;
  struct itimerspec its;
  its.it_value.tv_sec = timeoutMs / 1000;
  its.it_value.tv_nsec = ((long int)timeoutMs * 1000000 ) % 1000000000;
  its.it_interval.tv_sec = 0;
  its.it_interval.tv_nsec = 0;
  
  //if (timer_settime(asr_timerid, 0, &its, NULL) == -1)
  if (timer_settime( *pTimer, 0, &its, NULL) == -1){
    dbg_out( DBG_ERROR, "ERROR: startTimer() failed.\n");
    ret = -1;
  }else{
    dbg_out( DBG_VERBOSE, "started timer %d\n", *pTimer);
  }

  return ret;
}  // End of startTimer()


/********************************************************************
  stopTimer()

  Parameters: (in)  pointer to timer structure
              (in)  Timeout in milliseconds
  Returns:    0 = success, other = error code.

  Description:
  Stops the specified timer-

********************************************************************/
int stopTimer( timer_t *pTimer )
{
  int ret = 0;
  struct itimerspec its;
  its.it_value.tv_sec = 0;
  its.it_value.tv_nsec = 0;
  its.it_interval.tv_sec = 0;
  its.it_interval.tv_nsec = 0;
  
  if( timer_settime( *pTimer, 0, &its, NULL ) == -1 ){
    dbg_out( DBG_ERROR, "ERROR: stopTimer() failed.\n");
    ret = -1;
  }else{
    dbg_out( DBG_VERBOSE, "stopped timer %d\n", *pTimer);
  }

  return ret;
}  // End of stopTimer()
#endif

/** EOF ************************************************************/
