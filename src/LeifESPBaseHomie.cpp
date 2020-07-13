#include "LeifESPBase.h"
#include "LeifESPBaseHomie.h"

#include "..\environment_setup.h"

HomieDevice homie;


bool bLeifHomieSetupDefaults_DeferHomieID=false;

bool bLeifHomieSetupDefaultsDone=false;

void LeifHomieSetupDefaults(bool bDebug)
{
	while(bLeifHomieSetupDefaultsDone)
	{
		csprintf("LeifHomieSetupDefaults must be called only once\n");
		delay(5000);
	}
	bLeifHomieSetupDefaultsDone=true;

	if(bDebug)
	{
	HomieLibRegisterDebugPrintCallback([](const char * szText){
		csprintf("%s",szText);
		});
	}


	if(!bLeifHomieSetupDefaults_DeferHomieID)
	{
		homie.strFriendlyName=GetHeadingText();
		homie.strID=HomieDeviceName(GetHostName());
	}

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
