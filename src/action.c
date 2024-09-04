/********************************************************************

  action functions

  Author: Markku Heiskari
  Version history in github

  (C) Copyright 2023, Creoir Oy

********************************************************************/

/********************************************************************
  INCLUDES
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#if !defined(_MSC_VER)
#include <unistd.h>
#else
#include <windows.h>
#endif
#include <string.h>
#include "actionMain.h"
#include "util.h"
#include "action.h"

/********************************************************************
  DEFINES
********************************************************************/


/********************************************************************
  FILE SCOPE VARIABLES
********************************************************************/

extern globalData_type  *pGlobalData;


/********************************************************************
  FUNCTIONS
********************************************************************/




/********************************************************************
  postSampleMessage()

  Parameters: [in]  ptr. The app name in applications.json
  Returns:    0 = ok, nonzero = error code.

  Description:
  Posts sample MQTT topic

********************************************************************/
int postSampleMessage( const char *pString ){
  char  *pPayload;
  cJSON *jsonString;
  cJSON *jsonSomeNumber;
  cJSON *jsonAnotherString;
  cJSON *jsonPayload;

  jsonString = cJSON_CreateString( pString );
  jsonSomeNumber = cJSON_CreateNumber( 1024 );
  jsonAnotherString = cJSON_CreateString( "lorem ipsum" );
  jsonPayload = cJSON_CreateObject();
  cJSON_AddItemToObject( jsonPayload,"sample_data_string_1", jsonString );
  cJSON_AddItemToObject( jsonPayload,"sample_numeric_value", jsonSomeNumber );
  cJSON_AddItemToObject( jsonPayload,"sample_data_string_1", jsonAnotherString );
  pPayload = cJSON_PrintUnformatted( jsonPayload );

  if( !pPayload ){
    dbg_out(DBG_ERROR,"Fatal error creating payload at %s()\n", __FUNCTION__ );
    cJSON_Delete( jsonPayload );
    return -1;
  }

  if( getMQTTsendAccess( &pGlobalData->mqttSendMutex, __FUNCTION__) < 0 ){
    dbg_out( DBG_ERROR,"Did not get mutex lock for %s(). Aborting MQTT publish.\n",__FUNCTION__ );
    return -1;
  }

  // Write topic data to shared memory
  sprintf( pGlobalData->mqttSharedData.pTopic,"%s", "creoir/sample/testTopic" );
  strcpy( pGlobalData->mqttSharedData.pPayload, pPayload );

  // Send the topic
  sendMQTTtopic(__FUNCTION__);

  free( pPayload );
  cJSON_Delete( jsonPayload );

  return 0;

} // End of postSampleMessage()


/********************************************************************
  cleanMemAllocations()

  Parameters: void
  Returns:    0 = ok, nonzero = error code.

  Description:
  Releases heap memory blocks allocated by action.c

********************************************************************/
int cleanMemAllocations( void ){

  return 0;
} // End of cleanMemAllocations()


/********************************************************************
  handleEvt_onWakeword()

  Parameters: Pointer to event loop data structure
  Returns:    0 = ok, nonzero = error code.

  Description:
  Handles event EVT_MQTT_WAKEWORD
  From main event loop

********************************************************************/
int handleEvt_onWakeword( APPLICATION_EVENTDATA *eventData ){


  cJSON *jsonWAW;
  cJSON *jsonPayload;
  char  *pPayload;

  if( NULL == eventData ){
    dbg_out(DBG_ERROR, "%s() eventData null pointer error\n", __FUNCTION__);
  }

  dbg_out(DBG_NOTE,"Wakeword detected\n" );

  jsonPayload = cJSON_CreateObject();

  jsonWAW = cJSON_CreateString( "/usr/share/creoir/wakeup.wav" );
  cJSON_AddItemToObject( jsonPayload,"file", jsonWAW );
  pPayload = cJSON_PrintUnformatted( jsonPayload );

  if( !pPayload ){
    dbg_out(DBG_ERROR,"Fatal error creating payload at %s()\n", __FUNCTION__ );
    cJSON_Delete( jsonPayload );
    return -1;
  }

  if( getMQTTsendAccess( &pGlobalData->mqttSendMutex, __FUNCTION__) < 0 ){
    dbg_out( DBG_ERROR,"Did not get mutex lock for %s(). Aborting MQTT publish.\n",__FUNCTION__ );
    return -1;
  }

  // Write topic data to shared memory
  sprintf( pGlobalData->mqttSharedData.pTopic,"%s", "creoir/talk/speak" );
  strcpy( pGlobalData->mqttSharedData.pPayload, pPayload );

  // Send the topic
  sendMQTTtopic(__FUNCTION__);

  free( pPayload );
  cJSON_Delete( jsonPayload );

  return 0;

} // End of handleEvt_onWakeword()



/********************************************************************
  handleEvt_intentRecognized()

  Parameters: Pointer to event loop data structure
  Returns:    0 = ok, nonzero = error code.

  Description:
  Recognition result handling

********************************************************************/
int handleEvt_intentRecognized(APPLICATION_EVENTDATA* eventData) {

  int    i, numSlots;
  int    ret = 0;
  cJSON* jsonAll;
  cJSON* jsonIntent;
  cJSON* jsonConfidence;
  cJSON* jsonSlotArray;
  cJSON* jsonSingleSlot;
  cJSON* jsonSlotName;
  cJSON* jsonSlotValue;

  static short patternOn=0;
  static short routesOn=0;
  static short rangeOn=0;
  static short bearingScaleOn=0;
  static short tacticalFigOn=0;


  if (NULL == eventData) {
    dbg_out( DBG_ERROR,"%s() No payload. Nothing to parse!\n", __FUNCTION__ );
    return -1;
  }

  jsonAll = cJSON_Parse(eventData->topicPayload);
  if (!jsonAll) {
    dbg_out(DBG_ERROR, "%s() Cannot parse topic payload\n", __FUNCTION__);
    return -2;
  }

  // Parse intent /////////////////
  jsonIntent = cJSON_GetObjectItem(jsonAll, JSONKEY_INTENT);
  if (!jsonIntent) {
    dbg_out(DBG_ERROR, "No intent in recognition result\n");
    cJSON_Delete(jsonAll);
    return -100;
  }

  // Parse confidence /////////////////
  jsonConfidence = cJSON_GetObjectItem(jsonAll, JSONKEY_CONFIDENCE);
  if (!jsonConfidence) {
    dbg_out(DBG_ERROR, "No confidence in recognition result\n");
    cJSON_Delete(jsonAll);
    return -100;
  }
  dbg_out( DBG_NOTE,"Intent %s recognized with confidence %d\n" , jsonIntent->valuestring, jsonConfidence->valueint );


  // Parse slots /////////////////
  jsonSlotArray = cJSON_GetObjectItem(jsonAll, JSONKEY_SLOTS);


  // CATCH THE INTENTS ///////////
  ////////////////////////////////

  if( strcmp( jsonIntent->valuestring,INTENT_SAVE_TSP_DUMP ) == 0 ){
    // Specific handler for this intent. Called function should do the nuts and bolts for the intent.
    action_saveTacticalSituation( jsonConfidence->valueint, jsonSlotArray );

  }else if( strcmp( jsonIntent->valuestring,INTENT_SAVE_MAIN_DISPLAY_DUMP ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    action_JustRespondSpeech( "Main display dump saved." );

  }else if( strcmp( jsonIntent->valuestring,INTENT_OPEN_OWN_SHIP_SETTINGS ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    action_JustRespondSpeech( "Ship settings available at left side display." );
  
  }else if( strcmp( jsonIntent->valuestring,INTENT_DISPLAY_PATTERNS ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    patternOn = 1;
    action_JustRespondSpeech( "Display patterns enabled." );

  }else if( strcmp( jsonIntent->valuestring,INTENT_HIDE_PATTERNS ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    patternOn = 0;
    action_JustRespondSpeech( "Display patterns disabled." );

  }else if( strcmp( jsonIntent->valuestring,INTENT_DISPLAY_ROUTES ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    routesOn=1;
    action_JustRespondSpeech( "Routes are now visible." );

  }else if( strcmp( jsonIntent->valuestring,INTENT_HIDE_ROUTES ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    routesOn=0;
    action_JustRespondSpeech( "Routes are now hidden." );

  }else if( strcmp( jsonIntent->valuestring,INTENT_MAP_NORTH_UP ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    action_JustRespondSpeech( "Map orientation is north up." );

  }else if( strcmp( jsonIntent->valuestring,INTENT_MAP_HEADING_UP ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    action_JustRespondSpeech( "Map orientation is ship heading up." );

  }else if( strcmp( jsonIntent->valuestring,INTENT_TRUE_MOTION ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    action_JustRespondSpeech( "True motion mode on map is active." );

  }else if( strcmp( jsonIntent->valuestring,INTENT_DISPLAY_MAP_RANGE_RINGS ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    rangeOn=1;
    action_JustRespondSpeech( "Map range rings enabled." );

  }else if( strcmp( jsonIntent->valuestring,INTENT_HIDE_MAP_RANGE_RINGS ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    rangeOn=0;
    action_JustRespondSpeech( "Map range rings hidden." );

  }else if( strcmp( jsonIntent->valuestring,INTENT_DSPLY_BEARING_SCALE_RANGE ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    bearingScaleOn=1;
    action_JustRespondSpeech( "Bearing scale range enabled." );

  }else if( strcmp( jsonIntent->valuestring,INTENT_HIDE_BEARING_SCALE_RANGE ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    bearingScaleOn=0;
    action_JustRespondSpeech( "Bearing scale range hidden." );
  
  }else if( strcmp( jsonIntent->valuestring,INTENT_SWITCH_TO_DAY_MODE ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    action_JustRespondSpeech( "Day mode activated." );
  
  }else if( strcmp( jsonIntent->valuestring,INTENT_SWITCH_TO_DUSK_MODE ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    action_JustRespondSpeech( "Dusk mode activated." );
  
  }else if( strcmp( jsonIntent->valuestring,INTENT_SWITCH_TO_NIGHT_MODE ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    action_JustRespondSpeech( "Night mode activated." );
  
  }else if( strcmp( jsonIntent->valuestring,INTENT_CENTRE_MAP_TO_OWN_SHIP ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    action_JustRespondSpeech( "Map center set to ship position." );
  
  }else if( strcmp( jsonIntent->valuestring,INTENT_DISPLAY_TACTICAL_FIGURES ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    tacticalFigOn=1;
    action_JustRespondSpeech( "Tactical figures shown." );
    
  }else if( strcmp( jsonIntent->valuestring,INTENT_HIDE_TACTICAL_FIGURES ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    tacticalFigOn=0;
    action_JustRespondSpeech( "Tactical figures hidden." );
    
  }else if( strcmp( jsonIntent->valuestring,INTENT_REDUCE_MAP_SIZE ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    action_JustRespondSpeech( "Changed to small map window size." );

  }else if( strcmp( jsonIntent->valuestring,INTENT_GO_TO_NORMAL_MAP_SIZE ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    action_JustRespondSpeech( "Changed to full map window size." );

  }else if( strcmp( jsonIntent->valuestring,INTENT_SAVE_ACTIVE_WINDOW ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    action_JustRespondSpeech( "Active window saved." );

  }else if( strcmp( jsonIntent->valuestring,INTENT_MINIMIZE_ALL_WINDOWS ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    action_JustRespondSpeech( "All windows minimized." );

  }else if( strcmp( jsonIntent->valuestring,INTENT_DISPLAY_ALL_WINDOWS ) == 0 ){
    // General speech response to recognized intent. This is for demonstration purposes only.
    action_JustRespondSpeech( "All windows shown." );

  }else if( strcmp( jsonIntent->valuestring,INTENT_TOGGLE_PATTERNS ) == 0 ){
    // Keep track of pattern status. Response based on changed state.
    if(patternOn){
      patternOn = 0;
      action_JustRespondSpeech( "Patterns disabled." );
    }else{
      patternOn = 1;
      action_JustRespondSpeech( "Patterns enabled." );
    }

  }else if( strcmp( jsonIntent->valuestring,INTENT_TOGGLE_ROUTES ) == 0 ){
    // Keep track of route display status. Response based on changed state.
    if(routesOn){
      routesOn = 0;
      action_JustRespondSpeech( "Route display disabled." );
    }else{
      routesOn = 1;
      action_JustRespondSpeech( "Route display enabled." );
    }

  }else if( strcmp( jsonIntent->valuestring,INTENT_TOGGLE_MAP_RANGE_RINGS ) == 0 ){
    // Keep track of range ring status. Response based on changed state.
    if(rangeOn){
      rangeOn = 0;
      action_JustRespondSpeech( "Map range rings disabled." );
    }else{
      rangeOn = 1;
      action_JustRespondSpeech( "Map range rings enabled." );
    }
  
  }else if( strcmp( jsonIntent->valuestring,INTENT_TOGGLE_BEARING_SCALE_RANGE ) == 0 ){
    // Keep track of scale range status. Response based on changed state.
    if(bearingScaleOn){
      bearingScaleOn = 0;
      action_JustRespondSpeech( "Bearing scale range disabled." );
    }else{
      bearingScaleOn = 1;
      action_JustRespondSpeech( "Bearing scale range enabled." );
    }

  }else if( strcmp( jsonIntent->valuestring,INTENT_TOGGLE_TACTICAL_FIGURES ) == 0 ){
    // Keep track of tactical figure status. Response based on changed state.
    if(tacticalFigOn){
      tacticalFigOn = 0;
      action_JustRespondSpeech( "Tactical figures hidden." );
    }else{
      tacticalFigOn = 1;
      action_JustRespondSpeech( "Tactical figures shown." );
    }

  } // End if(INTENTS)





  #if 0

  // Parse slots /////////////////
  if( !jsonSlotArray ){
    dbg_out(DBG_ERROR, "No slot array in recognition result\n");
    cJSON_Delete(jsonAll);
    return -100;
  }
  numSlots = cJSON_GetArraySize( jsonSlotArray );
  for( i=0;i<numSlots;i++ ){
    jsonSingleSlot = cJSON_GetArrayItem( jsonSlotArray,i );
    if( !jsonSingleSlot ){
      dbg_out( DBG_ERROR,"%s() Failed to parse slot array index %d/%d\n", __FUNCTION__, i, numSlots );
      continue;
    }
    jsonSlotName = cJSON_GetObjectItem( jsonSingleSlot, JSONKEY_SLOTNAME );
    if( !jsonSlotName ){
      dbg_out( DBG_ERROR,"%s() No slot name in array index %d/%d\n", __FUNCTION__, i, numSlots );
      continue;
    }
    jsonSlotValue = cJSON_GetObjectItem( jsonSingleSlot, JSONKEY_SLOTVALUE );
    if( !jsonSlotName ){
      dbg_out( DBG_ERROR,"%s() No slot value in array index %d/%d\n", __FUNCTION__, i, numSlots );
      continue;
    }

    // Now print out this slot name/value pair
    dbg_out( DBG_NOTE,"Slot [%s] value [%s]\n", jsonSlotName->valuestring, jsonSlotValue->valuestring );

  } // End for(slots)

  #endif


// Free the memory allocated by cJSON object
cJSON_free( jsonAll );

return 0;

} // End of handleEvt_intentRecognized()


/********************************************************************
  handleEvt_intentNotRecognized()

  Parameters: Pointer to event loop data structure
  Returns:    0 = ok, nonzero = error code.

  Description:
  Recognition failure handling
  
********************************************************************/
int handleEvt_intentNotRecognized(APPLICATION_EVENTDATA* eventData) {

  int    i, numSlots;
  int    ret = 0;
  cJSON* jsonAll;
  cJSON* jsonIntent;
  cJSON* jsonConfidence;
  cJSON* jsonReasonCode;
  cJSON* jsonReasonText;


  if (NULL == eventData) {
    dbg_out( DBG_ERROR,"%s() No payload. Nothing to parse!\n", __FUNCTION__ );
    return -1;
  }

  jsonAll = cJSON_Parse(eventData->topicPayload);
  if (!jsonAll) {
    dbg_out(DBG_ERROR, "%s() Cannot parse topic payload\n", __FUNCTION__);
    return -2;
  }

  // Parse reason /////////////////
  jsonReasonCode = cJSON_GetObjectItem(jsonAll, JSONKEY_REASONCODE);
  if( !jsonReasonCode ) {
    dbg_out(DBG_ERROR, "No reasonCode in intentNotRecognized\n");
    cJSON_Delete(jsonAll);
    return -100;
  }
  jsonReasonText = cJSON_GetObjectItem(jsonAll, JSONKEY_REASONTEXT);
  if( !jsonReasonText ) {
    dbg_out(DBG_ERROR, "No reasonText in intentNotRecognized\n");
    cJSON_Delete(jsonAll);
    return -100;
  }
  dbg_out( DBG_NOTE,"Reconition rejected because of code %d [%s]\n", jsonReasonCode->valueint, jsonReasonText->valuestring );


  // Parse intent /////////////////
  jsonIntent = cJSON_GetObjectItem(jsonAll, JSONKEY_INTENT);
  if( jsonIntent ){
    dbg_out( DBG_NORM, "Rejected intent: %s\n",jsonIntent->valuestring );
  }

  // Parse confidence /////////////////
  jsonConfidence = cJSON_GetObjectItem(jsonAll, JSONKEY_CONFIDENCE);
  if( jsonConfidence ){
    dbg_out( DBG_NORM,"Rejected confidence: %d\n", jsonConfidence->valueint );
  }

  if( 2==jsonReasonCode->valueint || 1==jsonReasonCode->valueint ){
    action_playLowConfidence();
  }


// Free the memory allocated by cJSON object
cJSON_free( jsonAll );

return 0;

} // End of handleEvt_intentNotRecognized()


/********************************************************************
  handleEvt_MQTTuserIdentified()

  Parameters: Pointer to event loop data structure
  Returns:    0 = ok, nonzero = error code.

  Description:
  Biometric identification
  
********************************************************************/
int handleEvt_MQTTuserIdentified(APPLICATION_EVENTDATA* eventData) {

  int    i, numSlots;
  int    ret = 0;
  cJSON* jsonAll;
  cJSON* jsonIntent;
  cJSON* jsonConfidence;
  cJSON* jsonName;
  cJSON* jsonReasonText;
  cJSON* jsonUtterance;
  cJSON* jsonOutPayload;
  char  *pPayloadOut;
  char   szPrompt[512];
  time_t timeNow;
  static time_t  previousSpeech=0;


  if (NULL == eventData) {
    dbg_out( DBG_ERROR,"%s() No payload. Nothing to parse!\n", __FUNCTION__ );
    return -1;
  }

  jsonAll = cJSON_Parse(eventData->topicPayload);
  if (!jsonAll) {
    dbg_out(DBG_ERROR, "%s() Cannot parse topic payload\n", __FUNCTION__);
    return -2;
  }

  // Parse reason /////////////////
  jsonName = cJSON_GetObjectItem(jsonAll, "name");
  if( !jsonName ) {
    dbg_out(DBG_VERBOSE, "No name in biometrics/identification\n");
    cJSON_Delete(jsonAll);
    return -100;
  }

  // Parse name /////////////////
  if( NULL == jsonName->valuestring ){
    dbg_out(DBG_ERROR, "Name is not a string in biometrics/identification\n");
    cJSON_Delete(jsonAll);
    return -100;
  }

  dbg_out( DBG_NOTE,"Name: %s\n",jsonName->valuestring );

  // Parse confidence /////////////////
  jsonConfidence = cJSON_GetObjectItem(jsonAll, "score");
  if( jsonConfidence ){
    dbg_out( DBG_NORM,"Confidence score: %d\n", jsonConfidence->valueint );
  }


  timeNow = time( NULL );
  if( timeNow - previousSpeech <10000 ){
    dbg_out( DBG_NOTE,"Not greeting since previous prompt less than 10 seconds ago\n" );
    return 0;
  }

  sprintf( szPrompt,"Well hello my friend %s. How are you today?",jsonName->valuestring );


  jsonOutPayload = cJSON_CreateObject();
  if (!jsonOutPayload) {
    dbg_out(DBG_ERROR, "%s() Fatal error creating JSON object\n", __FUNCTION__);
    cJSON_Delete(jsonAll);
    return -10;
  }

  dbg_out( DBG_VERBOSE,"Prompt: %s\n",szPrompt );

  jsonUtterance = cJSON_CreateString( szPrompt );

  cJSON_AddItemToObject(jsonOutPayload, "utterance", jsonUtterance);
  

  pPayloadOut = cJSON_PrintUnformatted(jsonOutPayload);

  if (!pPayloadOut) {
    dbg_out(DBG_ERROR, "Fatal error in creating creoir/talk/speak JSON payload\n");
    cJSON_Delete(jsonOutPayload);
    return -10;
  }

  dbg_out(DBG_VERBOSE, "Sending speech request\n");

  // Send the topic
  /////////////////
  if (getMQTTsendAccess(&pGlobalData->mqttSendMutex, __FUNCTION__) < 0) {
    dbg_out(DBG_ERROR, "Did not get mutex lock for %s(). Aborting MQTT publish.\n", __FUNCTION__);
    free(pPayloadOut);
    cJSON_Delete(jsonOutPayload);
    return -100;
  }

  // Write message data to shared memory
  sprintf(pGlobalData->mqttSharedData.pTopic, "%s", "creoir/talk/speak");
  strcpy(pGlobalData->mqttSharedData.pPayload, pPayloadOut);

  // Send the topic
  sendMQTTtopic(__FUNCTION__);

  dbg_out(DBG_VERBOSE, "Speech on the way\n");

  free(pPayloadOut);
  cJSON_Delete(jsonOutPayload);

  // Free the memory allocated by cJSON object
  cJSON_free( jsonAll );

return 0;

} // End of handleEvt_MQTTuserIdentified()


/********************************************************************
  handle_MQTTonWakeword()

  Parameters: Pointer to event loop data structure
  Returns:    0 = ok, nonzero = error code.

  Description:
  Gets called once MQTT topic creoir/asr/wakewordDetected is received

********************************************************************/
int handle_MQTTonWakeword( const char* pTopic, const char *pData ){

  APPLICATION_EVENTDATA eventData;
  memset(&eventData, 0x00, sizeof(APPLICATION_EVENTDATA));
  eventData.payloadPtr = NULL;

  dbg_out( DBG_VERBOSE,"%s() handler called.\n", __FUNCTION__ );


  if( !pData ){
    dbg_out( DBG_ERROR,"No MQTT data in topic.\n" );
    return -1;
  }

  dbg_out( DBG_MQTT,"Data:%s\n",pData );

  strcpy( eventData.topicPayload, pData );
  dbg_out(DBG_VERBOSE,"Pushing event EVT_MQTT_WAKEWORD\n" );
  pushEvent( EVT_MQTT_WAKEWORD, &eventData );

  return 0;

} // End of handle_MQTTonWakeword()




/********************************************************************
  handle_MQTTintentRecognized()

  Parameters: Pointer to event loop data structure
  Returns:    0 = ok, nonzero = error code.

  Description:
  Gets called once MQTT topic creoir/asr/intentRecognized is received

********************************************************************/
int handle_MQTTintentRecognized(const char* pTopic, const char* pData) {

  APPLICATION_EVENTDATA eventData;
  memset(&eventData, 0x00, sizeof(APPLICATION_EVENTDATA));
  eventData.payloadPtr = NULL;

  dbg_out(DBG_VERBOSE, "%s() handler called.\n", __FUNCTION__);

  if (!pData) {
    dbg_out(DBG_ERROR, "No MQTT data in topic.\n");
    return -1;
  }

  dbg_out(DBG_MQTT, "Data:%s\n", pData);

  strcpy(eventData.topicPayload, pData);
  dbg_out(DBG_VERBOSE, "Pushing event EVT_MQTT_INTENT_RECOGNIZED\n");
  pushEvent( EVT_MQTT_INTENT_RECOGNIZED, &eventData );

  return 0;

} // End of handle_MQTTintentRecognized()



/********************************************************************
  handle_MQTTintentNotRecognized()

  Parameters: Pointer to event loop data structure
  Returns:    0 = ok, nonzero = error code.

  Description:
  Gets called once MQTT topic creoir/asr/intentNotRecognized is received

********************************************************************/
int handle_MQTTintentNotRecognized(const char* pTopic, const char* pData) {

  APPLICATION_EVENTDATA eventData;
  memset(&eventData, 0x00, sizeof(APPLICATION_EVENTDATA));
  eventData.payloadPtr = NULL;

  dbg_out(DBG_VERBOSE, "%s() handler called.\n", __FUNCTION__);

  if (!pData) {
    dbg_out(DBG_ERROR, "No MQTT data in topic.\n");
    return -1;
  }

  dbg_out(DBG_MQTT, "Data:%s\n", pData);

  strcpy(eventData.topicPayload, pData);
  dbg_out(DBG_VERBOSE, "Pushing event EVT_MQTT_INTENT_NOT_RECOGNIZED\n");
  pushEvent( EVT_MQTT_INTENT_NOT_RECOGNIZED, &eventData );

  return 0;

} // End of handle_MQTTintentNotRecognized()



/********************************************************************
  handle_MQTTuserIdentified()

  Parameters: Pointer to event loop data structure
  Returns:    0 = ok, nonzero = error code.

  Description:
  Gets called once MQTT topic creoir/biometrics/identification is received

********************************************************************/
int handle_MQTTuserIdentified(const char* pTopic, const char* pData) {

  APPLICATION_EVENTDATA eventData;
  memset(&eventData, 0x00, sizeof(APPLICATION_EVENTDATA));
  eventData.payloadPtr = NULL;

  dbg_out(DBG_VERBOSE, "%s() handler called.\n", __FUNCTION__);

  if (!pData) {
    dbg_out(DBG_ERROR, "No MQTT data in topic.\n");
    return -1;
  }

  dbg_out(DBG_MQTT, "Data:%s\n", pData);

  strcpy(eventData.topicPayload, pData);
  dbg_out(DBG_VERBOSE, "Pushing event EVT_MQTT_BIOM_IDENTIFICATION\n");
  pushEvent( EVT_MQTT_BIOM_IDENTIFICATION, &eventData );

  return 0;

} // End of handle_MQTTuserIdentified()



/********************************************************************
  handle_app_stop()

  Parameters: (in) Pointer to MQTT payload
  Returns:    0 = ok, nonzero = error code.

  Description:
  Parses creoir/app/stop MQTT topic
    
********************************************************************/
int handleMQTT_app_stop( const char* pTopic, const char *pData ){

  APPLICATION_EVENTDATA eventData;
  memset(&eventData, 0x00, sizeof(APPLICATION_EVENTDATA));
  eventData.payloadPtr = NULL;
  
  dbg_out( DBG_VERBOSE,"%s() handler called.\n", __FUNCTION__ );

  if( !pData ){
    dbg_out( DBG_ERROR,"No MQTT data in topic.\n" );
    return -1;
  }
  dbg_out( DBG_MQTT,"Data:%s\n",pData );

  strcpy( eventData.topicPayload, pData );
  pushEvent( EVT_APP_STOP, &eventData );

  return 0;
}  // End of handleMQTT_app_stop()



/********************************************************************
  action_saveTacticalSituation()

  Parameters: [in]  Confidence
              [in]  Ptr to slot array (cJSON) 

  Returns:    0 = ok, nonzero = error code.

  Description:
  Handles intent SAVE_TSP_DUMP
    
********************************************************************/
int action_saveTacticalSituation( int iConfidence, cJSON* jsonSlotArray ){

  char  *pPayloadOut;
  cJSON *jsonOutPayload;
  cJSON *jsonUtterance;

  // Compose speech request
  ///////////////////////////////////////////
  jsonOutPayload = cJSON_CreateObject();
  if (!jsonOutPayload) {
    dbg_out(DBG_ERROR, "%s() Fatal error creating JSON object\n", __FUNCTION__);
    return -10;
  }


  jsonUtterance = cJSON_CreateString( "Tactical situation dump saved." );

  cJSON_AddItemToObject(jsonOutPayload, "utterance", jsonUtterance);
  

  pPayloadOut = cJSON_PrintUnformatted(jsonOutPayload);

  if (!pPayloadOut) {
    dbg_out(DBG_ERROR, "Fatal error in creating creoir/talk/speak JSON payload\n");
    cJSON_Delete(jsonOutPayload);
    return -10;
  }

  dbg_out(DBG_VERBOSE, "Sending speech request\n");

  // Send the topic
  /////////////////
  if (getMQTTsendAccess(&pGlobalData->mqttSendMutex, __FUNCTION__) < 0) {
    dbg_out(DBG_ERROR, "Did not get mutex lock for %s(). Aborting MQTT publish.\n", __FUNCTION__);
    free(pPayloadOut);
    cJSON_Delete(jsonOutPayload);
    return -100;
  }

  // Write message data to shared memory
  sprintf(pGlobalData->mqttSharedData.pTopic, "%s", "creoir/talk/speak");
  strcpy(pGlobalData->mqttSharedData.pPayload, pPayloadOut);

  // Send the topic
  sendMQTTtopic(__FUNCTION__);

  dbg_out(DBG_VERBOSE, "Speech on the way\n");

  free(pPayloadOut);
  cJSON_Delete(jsonOutPayload);
  return 0;


} // End of action_saveTacticalSituation()



/********************************************************************
  action_JustRespondSpeech()

  Parameters: [in]  Speech to speak out

  Returns:    0 = ok, nonzero = error code.

  Description:
  General response to handle intent by just saying something
    
********************************************************************/
int action_JustRespondSpeech( const char* pUtterance ){

  char  *pPayloadOut;
  cJSON *jsonOutPayload;
  cJSON *jsonUtterance;

  // Compose speech request
  ///////////////////////////////////////////
  jsonOutPayload = cJSON_CreateObject();
  if (!jsonOutPayload) {
    dbg_out(DBG_ERROR, "%s() Fatal error creating JSON object\n", __FUNCTION__);
    return -10;
  }


  jsonUtterance = cJSON_CreateString( pUtterance );

  cJSON_AddItemToObject(jsonOutPayload, "utterance", jsonUtterance);
  

  pPayloadOut = cJSON_PrintUnformatted(jsonOutPayload);

  if (!pPayloadOut) {
    dbg_out(DBG_ERROR, "%s() Fatal error in creating creoir/talk/speak JSON payload\n", __FUNCTION__ );
    cJSON_Delete(jsonOutPayload);
    return -10;
  }

  dbg_out(DBG_VERBOSE, "Sending speech request\n");

  // Send the topic
  /////////////////
  if (getMQTTsendAccess(&pGlobalData->mqttSendMutex, __FUNCTION__) < 0) {
    dbg_out(DBG_ERROR, "Did not get mutex lock for %s(). Aborting MQTT publish.\n", __FUNCTION__);
    free(pPayloadOut);
    cJSON_Delete(jsonOutPayload);
    return -100;
  }

  // Write message data to shared memory
  sprintf(pGlobalData->mqttSharedData.pTopic, "%s", "creoir/talk/speak");
  strcpy(pGlobalData->mqttSharedData.pPayload, pPayloadOut);

  // Send the topic
  sendMQTTtopic(__FUNCTION__);

  dbg_out(DBG_VERBOSE, "Speech on the way\n");

  free(pPayloadOut);
  cJSON_Delete(jsonOutPayload);
  return 0;


} // End of action_JustRespondSpeech()




/********************************************************************
  action_playLowConfidence()

  Parameters: void

  Returns:    0 = ok, nonzero = error code.

  Description:
  Requests low confidence tune to be played
    
********************************************************************/
int action_playLowConfidence( void ){

  char  *pPayloadOut;
  cJSON *jsonOutPayload;
  cJSON *jsonWAV;

  // Compose payload
  ///////////////////////////////////////////
  jsonOutPayload = cJSON_CreateObject();
  if (!jsonOutPayload) {
    dbg_out(DBG_ERROR, "%s() Fatal error creating JSON object\n", __FUNCTION__);
    return -10;
  }


  jsonWAV = cJSON_CreateString( "/usr/share/creoir/low_confidence.wav" );

  cJSON_AddItemToObject(jsonOutPayload, "file", jsonWAV);
  

  pPayloadOut = cJSON_PrintUnformatted(jsonOutPayload);

  if (!pPayloadOut) {
    dbg_out(DBG_ERROR, "Fatal error in creating creoir/talk/speak JSON payload\n");
    cJSON_Delete(jsonOutPayload);
    return -10;
  }

  dbg_out(DBG_VERBOSE, "Sending play request\n");

  // Send the topic
  /////////////////
  if (getMQTTsendAccess(&pGlobalData->mqttSendMutex, __FUNCTION__) < 0) {
    dbg_out(DBG_ERROR, "Did not get mutex lock for %s(). Aborting MQTT publish.\n", __FUNCTION__);
    free(pPayloadOut);
    cJSON_Delete(jsonOutPayload);
    return -100;
  }

  // Write message data to shared memory
  sprintf(pGlobalData->mqttSharedData.pTopic, "%s", "creoir/talk/speak");
  strcpy(pGlobalData->mqttSharedData.pPayload, pPayloadOut);

  // Send the topic
  sendMQTTtopic(__FUNCTION__);

  dbg_out(DBG_VERBOSE, "Request on the way\n");

  free(pPayloadOut);
  cJSON_Delete(jsonOutPayload);
  return 0;

} // End of action_playLowConfidence()


/********************************************************************
  setGrammar()

  Parameters: [in]  Name of grammar to be set
              [in]  Milliseconds to keep grammar active
              [in]  Action to take after result or timeout

  Returns:    0 = ok, nonzero = error code.

  Description:
  Sets given grammar active
  Does not handle returning MQTT topic so incorrect grammar or other
  parameter does not return error code from this function.
    
********************************************************************/
int setGrammar( const char* pGrammarName, int iTimeout, const char* pActionAfterResult ){

  char  *pPayloadOut;
  cJSON *jsonOutPayload;
  cJSON *jsonGrammar;
  cJSON *jsonTimeout;
  cJSON *jsonActionAfterResult;
  cJSON *jsonContextArray;

  // Compose payload
  ///////////////////////////////////////////
  jsonOutPayload = cJSON_CreateObject();
  if (!jsonOutPayload) {
    dbg_out(DBG_ERROR, "%s() Fatal error creating JSON object\n", __FUNCTION__);
    return -10;
  }


  jsonGrammar = cJSON_CreateString( pGrammarName );

  jsonContextArray = cJSON_CreateArray();
  cJSON_AddItemToArray( jsonContextArray,jsonGrammar );
  cJSON_AddItemToObject( jsonOutPayload, "contextNames", jsonContextArray );

  jsonTimeout = cJSON_CreateNumber( iTimeout );
  cJSON_AddItemToObject( jsonOutPayload, "timeOut", jsonTimeout );

  jsonActionAfterResult = cJSON_CreateString( pActionAfterResult );
  cJSON_AddItemToObject( jsonOutPayload, "actionAfterResult", jsonActionAfterResult );

  pPayloadOut = cJSON_PrintUnformatted( jsonOutPayload );

  if (!pPayloadOut) {
    dbg_out(DBG_ERROR, "Fatal error in creating creoir/asr/setContext JSON payload\n");
    cJSON_Delete(jsonOutPayload);
    return -10;
  }

  dbg_out(DBG_VERBOSE, "Sending grammar request\n");

  // Send the topic
  /////////////////
  if (getMQTTsendAccess(&pGlobalData->mqttSendMutex, __FUNCTION__) < 0) {
    dbg_out(DBG_ERROR, "Did not get mutex lock for %s(). Aborting MQTT publish.\n", __FUNCTION__);
    free(pPayloadOut);
    cJSON_Delete(jsonOutPayload);
    return -100;
  }

  // Write message data to shared memory
  sprintf(pGlobalData->mqttSharedData.pTopic, "%s", "creoir/asr/setContext");
  strcpy(pGlobalData->mqttSharedData.pPayload, pPayloadOut);

  // Send the topic
  sendMQTTtopic(__FUNCTION__);

  dbg_out(DBG_VERBOSE, "Context request on the way\n");

  free(pPayloadOut);
  cJSON_Delete(jsonOutPayload);
  return 0;


} // End of setGrammar()


/********************************************************************
  handleEvt_onStartup()

  Parameters: Pointer to event loop data structure
  Returns:    0 = ok, nonzero = error code.

  Description:
  Handles event EVT_STARTUP
  From main event loop

********************************************************************/
int handleEvt_onStartup( APPLICATION_EVENTDATA *eventData ){


  cJSON *jsonWAW;
  cJSON *jsonPayload;
  char  *pPayload;

  if( NULL == eventData ){
    dbg_out(DBG_ERROR, "%s() eventData null pointer error\n", __FUNCTION__);
  }

  dbg_out(DBG_VERBOSE,"Requesting startup chime\n" );

  jsonPayload = cJSON_CreateObject();

  jsonWAW = cJSON_CreateString( "/usr/share/creoir/startup.wav" );
  cJSON_AddItemToObject( jsonPayload,"file", jsonWAW );
  pPayload = cJSON_PrintUnformatted( jsonPayload );

  if( !pPayload ){
    dbg_out(DBG_ERROR,"Fatal error creating payload at %s()\n", __FUNCTION__ );
    cJSON_Delete( jsonPayload );
    return -1;
  }

  if( getMQTTsendAccess( &pGlobalData->mqttSendMutex, __FUNCTION__) < 0 ){
    dbg_out( DBG_ERROR,"Did not get mutex lock for %s(). Aborting MQTT publish.\n",__FUNCTION__ );
    return -1;
  }

  // Write topic data to shared memory
  sprintf( pGlobalData->mqttSharedData.pTopic,"%s", "creoir/talk/speak" );
  strcpy( pGlobalData->mqttSharedData.pPayload, pPayload );

  // Send the topic
  sendMQTTtopic(__FUNCTION__);

  free( pPayload );
  cJSON_Delete( jsonPayload );

  return 0;

} // End of handleEvt_onStartup()


/** End of action.c ******************************************/
