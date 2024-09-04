/********************************************************************

  9LV test application main module

  Author: Markku Heiskari
  Version history in github

  (C) Copyright 2023-2024, Creoir Oy

********************************************************************/

/********************************************************************
  INCLUDES
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#if !defined(_MSC_VER)
#include <unistd.h>
#include <pthread.h>
#include <termios.h> 
#else
#include <windows.h>
#include <crtdbg.h>
#endif
#include <string.h>
#if !defined(_MSC_VER)
#include "posix_sockets.h"
#endif
#include "actionMain.h"
#include "util.h"
#include "action.h"

/********************************************************************
  LOCAL DEFINES
********************************************************************/



/********************************************************************
  TYPES and ENUMS
********************************************************************/


// Globally available heap data
globalData_type *pGlobalData;


/********************************************************************
  LOCAL PROTOTYPES (Global ones are in header file)
********************************************************************/
int app_init( void );
int app_eventloop(void);
void* readKeyboard( void* voidParam );

/********************************************************************
  FUNCTIONS
********************************************************************/


/********************************************************************
  printUsage()

  Parameters: void
  Returns:    void

  Description:
  Divides command line argument into two parts.
  Example to be parsed: --parameter=value

********************************************************************/
void printUsage(void) {

  printf("Usage: biom_testapp --<param>=<value>\n");
  printf("  <param> allowed values:\n");
  printf("  --verbose=<0/1/2/3>\n");
  printf("  --mqttHost=<address>\n");
  printf("  --mqttPort=<port>>\n");
  
  printf("\n\n");

} // End of printUsage()


/********************************************************************
  getArg()

  Parameters: [in]  Pointer to command line arguments
              [out] pptr to property (key)
              [out] pptr to value of the property (key)
  Returns:    0 = ok, nonzero = error code.

  Description:
  Divides command line argument into two parts.
  Example to be parsed: --parameter=value

********************************************************************/
static int getArg(char* arg, char** prop, char** value) {
    char* q = NULL;
    char* p = arg;
    if (p == NULL) return 1;

    q = strchr(p, '=');
    if (q == NULL) return 2;

    *q = '\0';
    ++q;

    *prop = p;
    *value = q;

    return 0;
}  // End of getArg()



/********************************************************************
  readKeyboard()

  Parameters: Void

  Returns:    Void ptr

  Description:
  Reads keyboard and posts event to main loop

********************************************************************/
void* readKeyboard( void* voidParam ){

  char c;
  APPLICATION_EVENTDATA eventData;
  static struct termios oldTerm, newTerm;   // Needed by Ubuntu

  memset(&eventData, 0x00, sizeof(APPLICATION_EVENTDATA));
  eventData.payloadPtr = NULL;

  dbg_out( DBG_VERBOSE,"Keyboard reader thread started.\n" );

  // No pressing of Enter to have characters fed to application getchar()
  tcgetattr( STDIN_FILENO, &oldTerm);
  newTerm = oldTerm;
  newTerm.c_lflag &= ~(ICANON | ECHO); 
  tcsetattr( STDIN_FILENO, TCSANOW, &newTerm);

  // Loops forever. Unless application is exiting.
  while( !pGlobalData->appExit ){
    c = getchar();
    if( ' ' == c || 'w' == c | 'W' == c){
      eventData.topicPayload[0]=c;
      pushEvent( EVT_KEYPRESS, &eventData );
    }
  } // End while(forever)

  tcsetattr( STDIN_FILENO, TCSANOW, &oldTerm);

  dbg_out( DBG_NORM,"Keyboard reader thread stopping.\n" );

  return NULL;
}  // End of readKeyboard()



/********************************************************************
  main()

  The usual.
********************************************************************/
int main( int argc, char *argv[] ) {
  int  option;
  int  fd;
  int  i, rc;
  int  argIdx;
  char tmpStr[128];

  pGlobalData=NULL;

  // Allocate memory for global data
  pGlobalData = malloc( sizeof( globalData_type ) );
  if( pGlobalData == NULL ){
    printf( "FATAL ERROR: %s Unable to allocate %zd bytes for state machine", __FILE__ ,sizeof( globalData_type ) );
    exit( -1 );
  }
  memset( pGlobalData,0x00,sizeof(globalData_type) );

  pGlobalData->appExit = 0; // If set to nonzero, "forever" loops will terminate.

  // Allocate memory for shared MQTT send data
  pGlobalData->mqttSharedData.pTopic = malloc( MQTT_SEND_TOPIC_SIZE );
  pGlobalData->mqttSharedData.pPayload = malloc( MQTT_SEND_PAYLOAD_SIZE );
  pGlobalData->mqttSharedData.dataSent=1;

  // Default parameters for MQTT broker
  strcpy( pGlobalData->mqttHost, MQTT_HOST_ADDRESS );
  strcpy( pGlobalData->mqttPort, MQTT_HOST_PORT );

  pGlobalData->debugMask=DBG_FATAL+DBG_ERROR+DBG_NOTE+DBG_IMPORTANT;

  dbg_out(DBG_NOTE, "Biometrics test action code version %d.%d.%d\n", APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_BUILD);

  // Process command line arguments
  argIdx = 1;
  while(argIdx <argc ){
    char* argKey;
    char* argValue;

    rc = getArg(argv[argIdx], &argKey, &argValue);
    if (0 != rc) {
      dbg_out(DBG_NOTE, "Error in argument %d\n", argIdx);
      if(2==rc)dbg_out(DBG_NOTE, "= missing from between parameter and value\n");
      printUsage();
      return rc;
    }

    if (0 == strcmp(argKey, "--syslog")) {
      i = atoi(argValue);
      if (i)dbg_out(DBG_NORM, "Log entries moved to syslog.\n");
      pGlobalData->syslog = i;
    }
    else if (0 == strcmp(argKey, "--string1")) {
      //strcpy( pGlobalData->sampleString, argValue );
      //dbg_out(DBG_NORM, "Using sample string: %s\n", argValue);
    }
    else if (0 == strcmp(argKey, "--verbose")) {
      if( atoi(argValue) > 0 )pGlobalData->debugMask += DBG_NORM;
      if( atoi(argValue) > 1 )pGlobalData->debugMask += DBG_VERBOSE;
      if( atoi(argValue) > 2 )pGlobalData->debugMask += DBG_MQTT;
      dbg_out( DBG_VERBOSE, "Output mask: %x\n", pGlobalData->debugMask );
    }else if (0 == strcmp(argKey, "--mqttHost")) {
      strcpy( pGlobalData->mqttHost,argValue );
      dbg_out( DBG_VERBOSE, "Using MQTT host %s\n", pGlobalData->mqttHost );
    }else if (0 == strcmp(argKey, "--mqttPort")) {
      strcpy( pGlobalData->mqttPort,argValue );
      dbg_out( DBG_VERBOSE, "Using MQTT host %s\n", pGlobalData->mqttPort );
    }


    argIdx++;
  }  // End while()


  if( !strstr( cJSON_Version(), "1.7.15") ){
    dbg_out( DBG_NOTE, "Expected libcjson.so version 1.7.15. Using %s\n", cJSON_Version() );
  }
  dbg_out( DBG_VERBOSE, "cJSON version: %s\n", cJSON_Version() );

  app_init();

  app_eventloop();

  cleanMemAllocations();

  // Cleanup
  free( pGlobalData->mqttSharedData.pTopic );
  free( pGlobalData->mqttSharedData.pPayload );
  free( pGlobalData );
  dbg_out( DBG_NOTE, "*** Biometrics test action code execution terminating. ***" );
  
  #if !defined(_MSC_VER)
    if( pGlobalData->syslog ) closelog();  // Closes syslog entry
  #endif

  exit( 100 );

}  // End of main()


/********************************************************************
   timedifference_msec()

  Parameters: (in) struct timeval starttime
              (in) struct timeval endtime

  Returns:    Time difference between start end endtime in milliseconds

  Description:
  Calculates time diff in milliseconds.
    
********************************************************************/
long timedifference_msec( struct timeval t0, struct timeval t1 )
{
  return (long)( ( t1.tv_sec-t0.tv_sec )*1000.0f + ( t1.tv_usec-t0.tv_usec )/1000.0f );
}


/********************************************************************
  popEvent()

  Parameters: (in) Pointer to Event structure

  Returns:    void

  Description:
  Pops an event from event queue
    
********************************************************************/
void popEvent( APPLICATION_EVENT *event, APPLICATION_EVENTDATA *eventData )
{
  // sem_wait( pGlobalData->eventSemap );  // Acquire semaphore
  mt_semaphore_acquire( pGlobalData->eventSemap );
  
  //pthread_mutex_lock( pGlobalData->eventMutex );
  mt_mutex_lock( pGlobalData->eventMutex );
  if( pGlobalData->eventListHead != NULL ){
    EVENTNODE_T *eventNode = pGlobalData->eventListHead;
    *event = eventNode->eventType;
    strcpy( eventData->topicPayload, eventNode->eventData.topicPayload );
    eventData->payloadPtr = eventNode->eventData.payloadPtr;

    pGlobalData->eventListHead = pGlobalData->eventListHead->next;
    if( NULL == pGlobalData->eventListHead ){
      pGlobalData->eventListTail = NULL;
    }
    free( eventNode );
  }
  //pthread_mutex_unlock( pGlobalData->eventMutex );
  mt_mutex_unlock( pGlobalData->eventMutex );
  return;
} // End of popEvent()


/********************************************************************
  pushEvent()

  Parameters: (in) Event structure

  Returns:    void

  Description:
  Pushes a new event to event queue
    
********************************************************************/
void pushEvent( APPLICATION_EVENT event, const APPLICATION_EVENTDATA *eventData )
{
  EVENTNODE_T *newEvent = 0;
  //pthread_mutex_lock( pGlobalData->eventMutex );
  mt_mutex_lock( pGlobalData->eventMutex );

  newEvent = malloc( sizeof(EVENTNODE_T) );
  newEvent->eventType = event;
  strcpy( newEvent->eventData.topicPayload, eventData->topicPayload );
  newEvent->eventData.payloadPtr = eventData->payloadPtr;
  newEvent->next = NULL;
  if( NULL == pGlobalData->eventListHead ){
    pGlobalData->eventListHead = newEvent;
  }

  if( NULL == pGlobalData->eventListTail ){
    pGlobalData->eventListTail = newEvent;
  }
  else{
    pGlobalData->eventListTail->next = newEvent;
    pGlobalData->eventListTail = newEvent;
  }
  //pthread_mutex_unlock( pGlobalData->eventMutex );
  mt_mutex_unlock( pGlobalData->eventMutex );

  //sem_post( pGlobalData->eventSemap ); // Release semaphore
  mt_semaphore_release( pGlobalData->eventSemap );
}  // End of pushEvent()


/********************************************************************
  emptyEventList()

  Parameters: void

  Returns:    void

  Description:
  Clears event queue
    
********************************************************************/
void emptyEventList()
{
  //pthread_mutex_lock( pGlobalData->eventMutex );
  mt_mutex_lock( pGlobalData->eventMutex );

  while( pGlobalData->eventListHead != NULL ){
    EVENTNODE_T *eventNode = pGlobalData->eventListHead;
    pGlobalData->eventListHead = pGlobalData->eventListHead->next;
    free( eventNode );
  }
  // pthread_mutex_unlock( pGlobalData->eventMutex );
  mt_mutex_unlock( pGlobalData->eventMutex );
}  // End of emptyEventList()


/********************************************************************
  app_eventloop()

  Parameters: void
  Returns:    0=Requested stop, negative=error

  Description:
  Application main event loop
    
********************************************************************/
int app_eventloop(void){

  APPLICATION_EVENT     applicationEvent;
  APPLICATION_EVENTDATA eventData;
  int                   ret;
  long                  elapsedTime;
  //struct  timeval       nowtime;
  //struct  timeval       prevtime;
  uint64_t              delta_ms;
  char                  message[128];
  
  dbg_out( DBG_NORM, "Event dispatcher starting...\n" );
  //gettimeofday(&prevtime, NULL); // Previus trigger time is at device start

  // Run the main event loop
  do{

    popEvent( &applicationEvent, &eventData );
    switch( applicationEvent ){

      case EVT_MQTT_WAKEWORD:
        dbg_out(DBG_VERBOSE, "EVT_MQTT_WAKEWORD\n");
        handleEvt_onWakeword(&eventData);
        break;

      case EVT_MQTT_INTENT_RECOGNIZED:
        dbg_out(DBG_VERBOSE, "EVT_MQTT_INTENT_RECOGNIZED\n");
        handleEvt_intentRecognized(&eventData);
        break;

      case EVT_MQTT_INTENT_NOT_RECOGNIZED:
        dbg_out(DBG_VERBOSE, "EVT_MQTT_INTENT_NOT_RECOGNIZED\n");
        handleEvt_intentNotRecognized(&eventData);
        break;

      case EVT_MQTT_BIOM_IDENTIFICATION:
        dbg_out(DBG_VERBOSE, "EVT_MQTT_BIOM_IDENTIFICATION\n");
        handleEvt_MQTTuserIdentified(&eventData);
        break;

      case EVT_KEYPRESS:
        dbg_out(DBG_NORM, "EVT_KEYPRESS - Simulates push-to-talk button\n");
        handleEvt_onWakeword( &eventData );

        // Check what key was pressed and act upon that
        if( ' ' == eventData.topicPayload[0] ){
          // If space pressed, enable main grammar and after recognition result resume to Idle mode. (Waits for key press.)
          setGrammar( "MAIN_9LV", 4000, "resumeToIdle" );
        }else if( 'w' == eventData.topicPayload[0] || 'W' == eventData.topicPayload[0] ){
          // If 'W' pressed, enable main grammar and after recognition result start listening to wakeword (and still keep reading also keyboard)
          setGrammar( "MAIN_9LV", 4000, "goToAutomaticMode" );
        }
        break;

      case EVT_STARTUP:
        handleEvt_onStartup( &eventData );
        break;

      case EVT_APP_STOP:
        dbg_out( DBG_VERBOSE, "EVT_APP_STOP\n");
        pGlobalData->appExit=1;
        break;

      default:
        dbg_out( DBG_ERROR, "Unknown event %d received\n",applicationEvent );

    }  // End switch


  }while ( !pGlobalData->appExit );
  dbg_out( DBG_NOTE, "Application event loop exit.\n");

  return 0;
}  // End of app_eventloop()



/********************************************************************
  app_init()

  Parameters: -
  Returns:    0 = ok, nonzero = error code.

  Description:
  Initializes the application.
    
********************************************************************/
int app_init( void ){

  int       fd;
  int       rc = 1;
  int       i,ret;
  pthread_t kbrd_daemon;
  APPLICATION_EVENTDATA eventData;

  memset(&eventData, 0x00, sizeof(APPLICATION_EVENTDATA));
  eventData.payloadPtr = NULL;
  
  
  #if defined(_MSC_VER)
    // Enable ANSI color control
    pGlobalData->hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (pGlobalData->hConsole == NULL) {
      dbg_out(DBG_NOTE, "Unable to get console handle.");
    }else{
      SetConsoleTextAttribute(pGlobalData->hConsole, FOREGROUND_RED + FOREGROUND_GREEN + FOREGROUND_BLUE);
      SetConsoleTitle("MQTTclientApp");
    }
  #else
      // Open syslog, if enabled
    if( pGlobalData->syslog ){ 
      openlog("9LV_app", LOG_PID, LOG_USER);
    }
  #endif

  dbg_out( DBG_VERBOSE, "Application version %d.%d.%d\n",
           APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_BUILD );

  // MQTT mutex initialization
  //pthread_mutex_init( &pGlobalData->mqttSendMutex, NULL );
  InitializeMQTTsendMutex(&pGlobalData->mqttSendMutex);

  // MQTT conditional variable initialization
  #if defined(_MSC_VER)
  // MSC condition variable initialized at variable instantiation.
    InitializeConditionVariable(&pGlobalData->mqttSend_cv);
  #else
    pthread_cond_init(&pGlobalData->mqttSend_cv, NULL);
  #endif
  
  // Main message loop semaphore and mutex init
  pGlobalData->eventSemap = malloc( mt_semaphore_getinstancesize() );
  //sem_init( pGlobalData->eventSemap, 0, 0 );
  mt_semaphore_create(pGlobalData->eventSemap, 0, 5);
  pGlobalData->eventMutex = malloc( mt_mutex_getinstancesize() );
  //pthread_mutex_init( pGlobalData->eventMutex,NULL );
  mt_mutex_init(pGlobalData->eventMutex);

  #if defined(_MSC_VER ) && defined(DEBUGGAA)
    dbg_out(DBG_NOTE, "Waiting 15 seconds for debugger attach...\n");
    Sleep(15000);
    dbg_out(DBG_NOTE, "Now continuing\n");
  #endif

  mqtt_interface_init();

  dbg_out( DBG_NORM,"Starting keyboard reader thread.\n" );
  if( pthread_create( &kbrd_daemon, NULL, readKeyboard, NULL) ){
    dbg_out( DBG_ERROR,"KBRD: Failed to start keyboard reader daemon.\n");
  }

  dbg_out( DBG_NORM,"application initialization complete.\n" );
  pushEvent( EVT_STARTUP, &eventData );

  return 0;

} // End of app_init()


/** End of actionMain.c ***********************************************/

