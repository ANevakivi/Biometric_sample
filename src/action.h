/**
 * @file action.h
 * @author Markku Heiskari
 * @brief Header file for actual action functions of the sample application
 * 
 * @copyright Copyright (c) 2023 Creoir Oy
 * 
 */


#ifndef __action_h
#define __action_h

/********************************************************************
  INCLUDES
********************************************************************/

/********************************************************************
  DEFINES
********************************************************************/

#define JSONKEY_INTENT                    "intent"                //!< Intent name in recognition result or recognition failure
#define JSONKEY_GRAMMAR                   "grammar"               //!< Grammar name in recognition result or recognition failure
#define JSONKEY_CONFIDENCE                "confidence"            //!< Confidence level in recognition result or recognition failure. Range 0-10000
#define JSONKEY_UTTERANCE                 "utterance"             //!< Recognized utterance in recognition result
#define JSONKEY_SLOTS                     "slots"                 //!< Slot array name in recognition result
#define JSONKEY_SLOTNAME                  "slotName"              //!< Slot name in recognition result
#define JSONKEY_SLOTVALUE                 "slotValue"             //!< Slot value in recognition result
#define JSONKEY_SLOTUNIT                  "unit"                  //!< Slot unit in recognition result (if available)
#define JSONKEY_SLOTID_LIST               "IDs"                   //!< Array of grammar ID values in recognition result

#define JSONKEY_REASONCODE                "reasonCode"            //!< Numeric reason why recognition failed
#define JSONKEY_REASONTEXT                "reasonText"            //!< Textual reason why recognition failed

#define INTENT_SAVE_TSP_DUMP              "SAVE_TSP_DUMP"
#define INTENT_SAVE_MAIN_DISPLAY_DUMP     "SAVE_MAIN_DISPLAY_DUMP"
#define INTENT_OPEN_OWN_SHIP_SETTINGS     "OPEN_OWN_SHIP_SETTINGS"
#define INTENT_DISPLAY_PATTERNS           "DISPLAY_PATTERNS"
#define INTENT_HIDE_PATTERNS              "HIDE_PATTERNS"
#define INTENT_DISPLAY_ROUTES             "DISPLAY_ROUTES"
#define INTENT_HIDE_ROUTES                "HIDE_ROUTES"
#define INTENT_MAP_NORTH_UP               "MAP_NORTH_UP"
#define INTENT_MAP_HEADING_UP             "MAP_HEADING_UP"
#define INTENT_TRUE_MOTION                "MAP_TRUE_MOTION"
#define INTENT_DISPLAY_MAP_RANGE_RINGS    "DISPLAY_MAP_RANGE_RINGS"
#define INTENT_HIDE_MAP_RANGE_RINGS       "HIDE_MAP_RANGE_RINGS"
#define INTENT_DSPLY_BEARING_SCALE_RANGE  "DISPLAY_BEARING_SCALE_RANGE"
#define INTENT_HIDE_BEARING_SCALE_RANGE   "HIDE_BEARING_SCALE_RANGE"
#define INTENT_SWITCH_TO_DAY_MODE         "SWITCH_TO_DAY_MODE"
#define INTENT_SWITCH_TO_DUSK_MODE        "SWITCH_TO_DUSK_MODE"
#define INTENT_SWITCH_TO_NIGHT_MODE       "SWITCH_TO_NIGHT_MODE"
#define INTENT_CENTRE_MAP_TO_OWN_SHIP     "CENTRE_MAP_TO_OWN_SHIP"
#define INTENT_DISPLAY_TACTICAL_FIGURES   "DISPLAY_TACTICAL_FIGURES"
#define INTENT_HIDE_TACTICAL_FIGURES      "HIDE_TACTICAL_FIGURES"
#define INTENT_REDUCE_MAP_SIZE            "REDUCE_MAP_SIZE"
#define INTENT_GO_TO_NORMAL_MAP_SIZE      "GO_TO_NORMAL_MAP_SIZE"
#define INTENT_SAVE_ACTIVE_WINDOW         "SAVE_ACTIVE_WINDOW"
#define INTENT_TOGGLE_PATTERNS            "TOGGLE_PATTERNS"
#define INTENT_TOGGLE_ROUTES              "TOGGLE_ROUTES"
#define INTENT_TOGGLE_MAP_RANGE_RINGS     "TOGGLE_MAP_RANGE_RINGS"
#define INTENT_TOGGLE_BEARING_SCALE_RANGE "TOGGLE_BEARING_SCALE_RANGE"
#define INTENT_TOGGLE_TACTICAL_FIGURES    "TOGGLE_TACTICAL_FIGURES"
#define INTENT_MINIMIZE_ALL_WINDOWS       "MINIMIZE_ALL_WINDOWS"
#define INTENT_DISPLAY_ALL_WINDOWS        "DISPLAY_ALL_WINDOWS"



/********************************************************************
  DATA TYPES
********************************************************************/



/********************************************************************
  PROTOTYPES
********************************************************************/


/**
 * @brief Handles intent SAVE_TSP_DUMP
 * 
 * @param int iConfidence      Confidence level of intent
 * @param cJSON* jsonSlotArray Ptr to slot array (may be null)
 * @return int 0=OK, nonzero=Error code
 */
int action_saveTacticalSituation( int iConfidence, cJSON* jsonSlotArray );


/**
 * @brief Just speech response to an intent.
 * 
 * @param char* pUtterance   Ptr to utterance to be spoken
 *
 * @return int 0=OK, nonzero=Error code
 */
int action_JustRespondSpeech( const char* pUtterance );


/**
 * @brief Requests vocalizer to play tune low_confidence.wav
 * 
 * @return int 0=OK, nonzero=Error code
 */
int action_playLowConfidence( void );


/**
 * @brief Sample function to handle wakeword event
 * 
 * @param eventData Event data structure for application interal events
 * @return int 0=OK, nonzero=Error code
 */
int handleEvt_onWakeword( APPLICATION_EVENTDATA *eventData );



/**
 * @brief Sample function to handle application startup event
 * 
 * @param eventData Event data structure for application interal events
 * @return int 0=OK, nonzero=Error code
 */
int handleEvt_onStartup( APPLICATION_EVENTDATA *eventData );


/**
 * @brief Sample function to handle event "ping"
 * 
 * @param eventData Event data structure for application interal events
 * @return int 0=OK, nonzero=Error code
 */
int handleEvt_ping(APPLICATION_EVENTDATA* eventData);


/**
 * @brief Sample function to handle event "pong"
 * 
 * @param eventData Event data structure for application interal events
 * @return int 0=OK, nonzero=Error code
 */
int handleEvt_pong(APPLICATION_EVENTDATA* eventData);


/**
 * @brief Sample function to handle recognition results
 * 
 * @param eventData Event data structure for application interal events
 * @return int 0=OK, nonzero=Error code
 */
int handleEvt_intentRecognized(APPLICATION_EVENTDATA* eventData);


/**
 * @brief Sample function to handle biometric identification
 * 
 * @param eventData Event data structure for application interal events
 * @return int 0=OK, nonzero=Error code
 */
int handleEvt_MQTTuserIdentified(APPLICATION_EVENTDATA* eventData);



/**
 * @brief Sample function to handle recognition failures
 * 
 * @param eventData Event data structure for application interal events
 * @return int 0=OK, nonzero=Error code
 */
int handleEvt_intentNotRecognized(APPLICATION_EVENTDATA* eventData);


/**
 * @brief Placeholder for application exit memory cleaning operations
 * 
 * @return int 0=OK, nonzero=error
 */
int cleanMemAllocations( void );



/**
 * @brief Activates given grammar
 * 
 * @param char* pGrammarName        ptr to grammar name
 * @param int iTimeout              grammar timeout in milliseconds
 * @param char* pActionAfterResult  What to do after result or timeout
 *
 * @return int 0=OK, nonzero=Error code
 */
int setGrammar( const char* pGrammarName, int iTimeout, const char* pActionAfterResult );



#endif
/** End of rar_detector.h ******************************************/
