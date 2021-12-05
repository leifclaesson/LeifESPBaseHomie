#include <LeifESPBaseHomie.h>

// *** Don't forget to add your MQTT credentials to LeifESPBaseHomie\environment_setup.h. See environment_setup.h.example for details. ***
// *** Don't forget to add your WiFi SSID/Key to LeifESPBase\environment_setup.h See environment_setup.h.example for details. ***


//This host name will be used for the HTTP server and the Homie topic.
//We will update it in the setup function, based on the MAC address.

const char * szHostName="HomieExampleMCU-dev";
const char * szFriendlyName="Homie Example MCU (dev)";


const char * GetHostName()	//this host name will reported to for example mDNS, telnet
{
	return szHostName;
}

const char * GetHeadingText()	//friendly system name, used for example for the HTTP page
{
	return szFriendlyName;
}


// We'll need a place to save pointers to our created properties so that we can access them after creation.
HomieProperty * pPropLedInvert=NULL;
HomieProperty * pPropUptime=NULL;
HomieProperty * pPropColor=NULL;
HomieProperty * pPropBacklight=NULL;
HomieProperty * pPropTemperature=NULL;
HomieProperty * pPropSpeed=NULL;
HomieProperty * pPropStandardMQTT=NULL;


//terrible little web interface. I truly suck at webdesign. You can do better.

void genHtmlHeader(String & output)
{
	output.concat("<!DOCTYPE html>");
	output.concat("<meta name=\"viewport\" content=\"width=device-width, initial-scale=0.95\">");
	output.concat("<html><head><style>table, th, td {  border: 1px solid black;  border-collapse: collapse;}th, td { padding: 5px;}</style></head>");
	output.concat("<body>");
	output.concat("<h2>");
	output.concat(GetHeadingText());
	output.concat("</h2>");
}

//rather than building and buffering the entire web page in memory, we can flush it out a bit at a time.
void FlushOutput(String & output, bool bFinal=false)
{
	if(output.length()>=256 || bFinal)
	{
		server.sendContent(output); output="";
	}
}

//this function will be called by all the URL handlers.
void genHtmlPage(const String & strInsert)
{

	String output;
	output.reserve(512);


	genHtmlHeader(output);

	FlushOutput(output);

	LeifHtmlMainPageCommonHeader(output);


	FlushOutput(output);

	output.concat("<table><tr>");

	output.concat("<td><a href=\"/\">Reload</a></td>");
	output.concat("<td><a href=\"/ping\">Minimal test page</a></td>");
	output.concat("<td><a href=\"/sysinfo\">System Info</a></td>");
	output.concat("<td><a href=\"/invert\">Invert status LED</a></td>");
	output.concat("<td><a href=\"/restart\">Restart</a></td>");
	output.concat("</tr></table>");

	FlushOutput(output);

	output.concat(strInsert);

	output.concat("</body></html>");

	FlushOutput(output, true);

}


void handleRoot()
{
	String strInsert;

	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200,"text/html","");

	genHtmlPage(strInsert);

}


void handleInvert()	//this gets called when we access /invert on the HTTP server
{

	LeifSetInvertLedBlink(!LeifGetInvertLedBlink());	//toggle the state
	pPropLedInvert->SetBool(LeifGetInvertLedBlink());	//update the homie property too


	String strInsert;

	strInsert="<p>Status LED pattern inverted. Now normally ";
	strInsert+=LeifGetInvertLedBlink()?"OFF":"ON";
	strInsert+=".</p>";

	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200,"text/html","");

	genHtmlPage(strInsert);

}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}



void setup()
{

	LeifSetupConsole(1024);		//Scrollback buffer 1024 bytes

	//We can use the MAC address to select between hardcoded configurations.
	//This greatly reduces the risk of mistakes.
	//We can for example have separate host names for a unit on the bench compared to a deployed unit,
	//which may be in a difficult to access location. This way you know'll know which unit you're communicating with.

	if(WiFi.macAddress()=="BC:DD:C2:23:68:54")
	{
		szHostName="HomieExampleMCU";
		szFriendlyName="Homie Example MCU";
	}

	//The LED is used for status indication. Flashes twice a second while connecting WiFi.
	//Flashes once every two seconds when connected.

	//By default, LED_BUILTIN is used for status output. 2 Hz flashing while connecting to WiFi, 0.5 Hz when connected.
	//You can use any pin by calling the LedSetStatusLedPin(x) function like below. -1 disables output altogether.
	//LeifSetStatusLedPin(-1);	//No status LED output
	//LeifSetStatusLedPin(1);	//ESP-01 on-board LED. This is actually the serial port so it will disable serial console output!

	LeifSetupBegin();	//starts initialization of the HTTP server and other objects.
						//Set your serial console to 115200 bps to see console output. The console will print the IP address and many other things.
						//Once you know the IP address you can connect to port 23 with a telnet client like PuTTY to see console output.
						//You'll need to enable the "Implicit CR option in every LF" on the Terminal Page in PuTTY.

	LeifUpdateCompileTime();	//Macro to capture __DATE__ and __TIME__ while compiling main sketch.
								//This way compile time as displayed by the main HTTP page will accurately reflect
								//sketch compile time, as opposed to library LeifESPBase compile time.

	server.onNotFound(handleNotFound);	//serve 404
	server.on("/", handleRoot);		//serve root page, defined above.

	//define your own URL handlers here.
	server.on("/invert", handleInvert);		//invert the LED flashing pattern.

	server.on("/restart", []()
		{
			handleRoot();
			LeifScheduleRestart(1000);
		}
	);

	LeifHomieSetupDefaults();

	{

		HomieNode * pNode=homie.NewNode();	//We don't really need a hierarchy, but we must have at least one node -- so I'm going to call it properties.
		pNode->strID="properties";
		pNode->strFriendlyName="Properties";

		HomieProperty * pProp=NULL;

		pPropStandardMQTT=pProp=pNode->NewProperty();
		pProp->strFriendlyName="Standard MQTT item";
		pProp->strID="standard_mqtt_item";
		pProp->SetStandardMQTT("some/topic");
		pProp->AddCallback([](HomieProperty * pSource)
		{	//this lambda function gets called when we receive a message on this property's topic.
			csprintf("%s is now %s\n",pSource->strFriendlyName.c_str(),pSource->GetValue().c_str());
		});


		// This parameter is fully functional. It controls whether the status LED is normally on or normally off.
		// It publishes its initial state (normally on = false) if there's no retained value, otherwise it uses the retained value.


		pPropLedInvert=pProp=pNode->NewProperty();
		pProp->strFriendlyName="Invert Status LED";
		pProp->strID="invertled";
		pProp->SetUnit("");
		pProp->SetRetained(true);
		pProp->SetSettable(true);
		pProp->datatype=homieBool;
		pProp->SetBool(LeifGetInvertLedBlink());	//Set the initial value. It'll get published if there is no retained value.
		pProp->AddCallback([](HomieProperty * pSource)
		{	//this lambda function gets called when we receive a message on this property's topic.
			LeifSetInvertLedBlink(pSource->GetValue()=="true");	//controls whether the LED is normally off or normally on.
		});



		pPropSpeed=pProp=pNode->NewProperty();
		pProp->strFriendlyName="Speed";
		pProp->strID="speed";
		pProp->SetUnit("");
		pProp->SetRetained(true);
		pProp->SetSettable(true);
		pProp->SetValue("OFF");
		pProp->datatype=homieEnum;
		pProp->strFormat="OFF,LOW,MEDIUM,HIGH";
		pProp->AddCallback([](HomieProperty * pSource)
		{	//this lambda function gets called when we receive a message on this property's topic.
			csprintf("The %s is now %s\n",pSource->strFriendlyName.c_str(),pSource->GetValue().c_str());
		});




		// This is an example of a string property. In this case we will publish our uptime every minute.

		pPropUptime=pProp=pNode->NewProperty();
		pProp->strID="uptime";
		pProp->datatype=homieString;
		pProp->SetRetained(false);
		pProp->strFriendlyName="Uptime";
		pProp->SetPublishEmptyString(false);




		// These parameters will print to the console when they're updated, that's all.

		pPropColor=pProp=pNode->NewProperty();
		pProp->strID="color";
		pProp->datatype=homieColor;
		pProp->SetRetained(true);
		pProp->SetSettable(true);
		pProp->strFriendlyName="Color Selector";
		pProp->strFormat="hsv";
		pProp->SetValue("60,100,100");	//default bright yellow
		pProp->AddCallback([](HomieProperty * pSource)
		{

			uint32_t rgbcolor=0;
			if(HomieParseHSV(pSource->GetValue().c_str(),rgbcolor))
			{
				csprintf("The selected RGB value is: #%06X\n",rgbcolor);
			}
		});


		pPropBacklight=pProp=pNode->NewProperty();
		pProp->strID="dimmer";
		pProp->datatype=homieFloat;
		pProp->SetRetained(true);
		pProp->SetSettable(true);
		pProp->strFriendlyName="Dimmer";
		pProp->strFormat="0:1";
		pProp->SetValue("0.5");	//default value
		pProp->AddCallback([](HomieProperty * pSource)
		{
			float fValue=atof(pSource->GetValue().c_str());
			csprintf("The %s is now at %.0f %%\n",pSource->strFriendlyName.c_str(),fValue*100.0f);
		});

		pPropTemperature=pProp=pNode->NewProperty();
		pProp->strID="temp_setpoint";
		pProp->datatype=homieInt;
		pProp->SetRetained(true);
		pProp->SetSettable(true);
		pProp->strFriendlyName="Temperature (setpoint)";
		pProp->strFormat="16:32";
		pProp->SetValue("27.0");	//default value
		pProp->AddCallback([](HomieProperty * pSource)
		{
			csprintf("The %s is now %s\n",pSource->strFriendlyName.c_str(),pSource->GetValue().c_str());
		});



	}





	homie.Init();

	LeifSetupEnd();	//finishes initialization of the HTTP server and other objects
}



void loop()
{
	LeifLoop();

	homie.Loop();

	//Automatically reboot if we're unable to ping the gateway for 5 minutes. ESP8266 only for now!
	//LeifGatewayKeepalive();	//uncomment if desired


	if(Interval100())
	{
		//True once every 100ms. Just for convenience.
	}

	if(Interval250())
	{
		//True once every 250ms. Just for convenience.
	}

	if(Interval1000())
	{
		//True once every second. Just for convenience.
	}

	if(Interval10s())
	{
		//True once every second. Just for convenience.
	}

	if(Interval1000() && (seconds()%60)==0)	//once a minute
	{
		String strUptime;
		LeifUptimeString(strUptime);

		String strUptimeWiFi;
		LeifSecondsToUptimeString(strUptimeWiFi,homie.GetUptimeSeconds_WiFi());

		String strUptimeMQTT;
		LeifSecondsToUptimeString(strUptimeMQTT,homie.GetUptimeSeconds_MQTT());



		//print a status message to the serial console and the telnet console.
		//you can disable the serial console by compiling with NO_SERIAL_DEBUG
		csprintf("Uptime=%s  WiFi=%s  MQTT=%s  HeapFree=%u  WiFi: %i\n",strUptime.c_str(),strUptimeWiFi.c_str(),strUptimeMQTT.c_str(),ESP.getFreeHeap(),WiFi.RSSI());



		//publish something outside of the homie tree, too

		if(homie.IsConnected())
		{
			String strTopic;
			strTopic="mcu/";
			strTopic+=GetHostName();
			strTopic+="/uptime";

			homie.PublishDirect(strTopic, 2, false, strUptime);
		}

		if(homie.IsConnected())
		{
			String s;
			s="Uptime=";
			s+=strUptime;
			s+="  WiFi=";
			s+=strUptimeWiFi;
			s+="  MQTT=";
			s+=strUptimeMQTT;
			s+="  Heap=";
			s+=ESP.getFreeHeap();
			s+="  RSSI=";
			s+=WiFi.RSSI();

			pPropUptime->SetValue(s);
		}


	}


}
