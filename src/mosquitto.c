/********************************************************************

  MQTT functions


********************************************************************/

/********************************************************************
  INCLUDES
********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#if defined(_MSC_VER)
#include <pthread.h>
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "mosquitto.h"
#include "util.h"
#include "cJSON.h"
#include "actionMain.h"



extern globalData_type *pGlobalData;


/********************************************************************
  FUNCTIONS
********************************************************************/


/********************************************************************
  on_connect()

  Parameters: Handle to client
	            void ptr
							reason code
  Returns:    void

  Description:
  Callback called when the client receives a CONNACK message from the broker

********************************************************************/
void on_connect(struct mosquitto *mosq, void *obj, int reason_code){
	int i;
	int rc;

	dbg_out( DBG_MQTT, "%s(): %s\n",__FUNCTION__, mosquitto_connack_string(reason_code));
	if(reason_code != 0){
		// mosquitto_disconnect(mosq);
		dbg_out( DBG_ERROR,"%s() MQTT connection failed.\n", __FUNCTION__  );
		return;
	}

	dbg_out(DBG_NORM, "Mosquitto MQTT client connected\n");
	pGlobalData->mqttConnected = 1;

	// Subscribe topics defined in mqttActionRegister
    for( i=0;i< (sizeof(mqttActionRegister)/sizeof(mqtt_action));i++ ){
      dbg_out( DBG_VERBOSE,"Registering action register index %d [%s].\n",i,mqttActionRegister[i].topic );
			
			/* PING-PONG SAMPLE CODE START */
			#if 0
			if( 1==pGlobalData->role && strstr(mqttActionRegister[i].topic,"ping") ) {
				dbg_out( DBG_VERBOSE,"Not subscribing to ping topic\n" );
				continue;
			}
			if (0 == pGlobalData->role && strstr(mqttActionRegister[i].topic, "pong")) {
				dbg_out(DBG_VERBOSE, "Not subscribing to pong topic\n");
				continue;
			}
			#endif
			/* PING-PONG SAMPLE CODE STOP */

			rc = mosquitto_subscribe(mosq, NULL, mqttActionRegister[i].topic, 1);
			if(rc != MOSQ_ERR_SUCCESS){
				dbg_out( DBG_ERROR, "%s() Failed to subscribe %s. Error: %s\n", __FUNCTION__, mqttActionRegister[i].topic, mosquitto_strerror(rc));
				mosquitto_disconnect(mosq);
				pGlobalData->mqttConnected = 0;
				return;
			}
    }	// End for()


}	// End of on_connect()


/********************************************************************
  on_publish()

  Parameters: Handle to client
	            void ptr
							message id
  Returns:    void

  Description:
  Callback called when the client knows to the best of its abilities that a
  PUBLISH has been successfully sent. For QoS 0 this means the message has
  been completely written to the operating system. For QoS 1 this means we
  have received a PUBACK from the broker. For QoS 2 this means we have
  received a PUBCOMP from the broker.

********************************************************************/
void on_publish(struct mosquitto *mosq, void *obj, int mid){
	dbg_out( DBG_MQTT, "%s() Message with mid %d has been published.\n",__FUNCTION__, mid);
}	// End of on_publish()


/********************************************************************
  on_subscribe()

  Parameters: Handle to client
	            void ptr
							message id
  Returns:    void

  Description:
  Callback called when the broker sends a SUBACK in response to a SUBSCRIBE

********************************************************************/
void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos){
	int i;
	bool have_subscription = false;

	/* In this example we only subscribe to a single topic at once, but a
	 * SUBSCRIBE can contain many topics at once, so this is one way to check
	 * them all. */
	for(i=0; i<qos_count; i++){
		dbg_out( DBG_MQTT, "%s() on_subscribe: %d:granted qos = %d\n", __FUNCTION__, i, granted_qos[i]);
		if(granted_qos[i] <= 2){
			have_subscription = true;
		}
	}
	if(have_subscription == false){
		/* The broker rejected our subscription */
		dbg_out( DBG_ERROR, "%s(), Error: MQTT subscription rejected.\n", __FUNCTION__ );
		mosquitto_disconnect(mosq);
		pGlobalData->mqttConnected = 0;
	}
}	// End of on_subscribe()


/********************************************************************
  on_message()

  Parameters: Handle to client
	            void ptr
							message id
  Returns:    void

  Description:
  Callback handler for MQTT topics

********************************************************************/
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg){
	int i;
	//dbg_out( DBG_NORM,"MQTT: %s %d %s\n", msg->topic, msg->qos, (char *)msg->payload);

	dbg_out( DBG_MQTT,"MQTT: Received '%s'\n", msg->topic );
  dbg_out( DBG_MQTT,"MQTT: Payload: %s\n", (char *)msg->payload );

  /* Loop the array of topics that must be reacted */
  for( i=0;i< (sizeof(mqttActionRegister)/sizeof(mqtt_action));i++ ){
    int (*pHandler)( const char*, const char* );
    //dbg_out( DBG_VERBOSE,"Checking action register index %d [%s].\n",i,mqttActionRegister[i].topic );
    // Instead of strcmp, use compare function that accepts "#" wildcard
    if( mqtt_topic_compare(mqttActionRegister[i].topic,msg->topic)==1 ){
      dbg_out( DBG_VERBOSE,"Action register MATCH at index %d\n",i );
      // Call the handler
      pHandler = mqttActionRegister[i].function;
      pHandler( msg->topic,(char *)msg->payload );
    }
	}	// End for()

}	// End of on_message()


/********************************************************************
  mqtt_interface_init()

  Parameters: void
  Returns:    void

  Description:
  Initializes the MQTT client

********************************************************************/
int mqtt_interface_init( void ){
	struct mosquitto *mosqClient;
	int rc;
	pthread_t  sender_daemon;

	dbg_out( DBG_VERBOSE, "MQTT initialize\n");

	/* Required before calling other mosquitto functions */
	mosquitto_lib_init();

	/* Create a new client instance.
	 * id = NULL -> ask the broker to generate a client id for us
	 * clean session = true -> the broker should remove old sessions when we connect
	 * obj = NULL -> we aren't passing any of our private data for callbacks
	 */
	mosqClient = mosquitto_new(NULL, true, NULL);
	if(mosqClient == NULL){
		dbg_out( DBG_ERROR, "%s() Out of memory\n",__FUNCTION__);
		return -1;
	}
	pGlobalData->mosquittoClient =mosqClient;

	/* Configure callbacks. This should be done before connecting ideally. */
	mosquitto_connect_callback_set(mosqClient, on_connect);
	mosquitto_publish_callback_set(mosqClient, on_publish);

	mosquitto_subscribe_callback_set(mosqClient, on_subscribe);
	mosquitto_message_callback_set(mosqClient, on_message);


	/* Connect to test.mosquitto.org on port 1883, with a keepalive of 60 seconds.
	 * This call makes the socket connection only, it does not complete the MQTT
	 * CONNECT/CONNACK flow, you should use mosquitto_loop_start() or
	 * mosquitto_loop_forever() for processing net traffic. */
	rc = mosquitto_connect(mosqClient, pGlobalData->mqttHost, atoi(pGlobalData->mqttPort) , 60);
	if(rc != MOSQ_ERR_SUCCESS){
		mosquitto_destroy(mosqClient);
		dbg_out( DBG_ERROR, "%s() Error: %s\n",__FUNCTION__, mosquitto_strerror(rc));
		return -1;
	}

	/* Run the network loop in a background thread, this call returns quickly. */
	rc = mosquitto_loop_start(mosqClient);
	if(rc != MOSQ_ERR_SUCCESS){
		mosquitto_destroy(mosqClient);
		dbg_out( DBG_ERROR, "%s() Error: %s\n",__FUNCTION__, mosquitto_strerror(rc));
		return -1;
	}

	if( pthread_create( &sender_daemon, NULL, mqtt_sender, &pGlobalData->mosquittoClient) ){
    dbg_out( DBG_ERROR,"MQTT: Failed to start MQTT sender daemon.\n");
    return -135;
  }

	return 0;
}	// End of mqtt_interface_init()



/** End of mosquitto.c ******************************************/