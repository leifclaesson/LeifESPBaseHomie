#include "LeifESPBase.h"
#include "LeifESPBaseHomie.h"

#include "..\environment_setup.h"

HomieDevice homie;

void LeifHomieSetupDefaults(bool bDebug)
{

	if(bDebug)
	{
	HomieLibRegisterDebugPrintCallback([](const char * szText){
		csprintf("%s",szText);
		});
	}


	homie.strFriendlyName=GetHeadingText();
	homie.strID=HomieDeviceName(GetHostName());

	homie.strMqttServerIP=mqtt_server;
	homie.strMqttUserName=mqtt_user;
	homie.strMqttPassword=mqtt_password;



	LeifRegisterOnShutdownCallback([](const char * pszReason){
		if(pszReason) {}
		homie.Quit();
		});


}

void LeifPublishMQTT(const char* topic, const char* payload, bool retain)
{

	String strTopic;
	strTopic="home/mcu/";
	strTopic+=GetHostName();
	strTopic+="/";
	strTopic+=topic;

	if(homie.IsConnected())
	{
		csprintf("MQTT publish [%s]: %s%s\n",strTopic.c_str(),payload,retain?" (retained)":" (volatile)");
		homie.PublishDirect(strTopic, 2, retain, String(payload));
	}
	else
	{
		csprintf("MQTT not connected, can't publish [%s]: %s\n",strTopic.c_str(),payload);
	}
}
