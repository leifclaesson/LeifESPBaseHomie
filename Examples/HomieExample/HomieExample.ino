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
HomieProperty * pPropQuote=NULL;
HomieProperty * pPropColor=NULL;
HomieProperty * pPropBacklight=NULL;
HomieProperty * pPropTemperature=NULL;
HomieProperty * pPropSpeed=NULL;
HomieProperty * pPropStandardMQTT=NULL;


//this function will be called by all the URL handlers.
//I truly suck at webdesign. You can do better.
void genHtmlPage(String & output, const String & strInsert)
{

	output.reserve(2048);

	output.concat("<!DOCTYPE html>");
	output.concat("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
	output.concat("<html><head><style>table, th, td {  border: 1px solid black;  border-collapse: collapse;}th, td {  padding: 5px;}</style></head>");
	output.concat("<body>");
	output.concat("<h2>");
	output.concat(GetHeadingText());
	output.concat("</h2>");

	LeifHtmlMainPageCommonHeader(output);

	output.concat("<table><tr>");

	output.concat("<td><a href=\"/\">Reload</a></td>");
	output.concat("<td><a href=\"/ping\">Minimal test page</a></td>");
	output.concat("<td><a href=\"/sysinfo\">System Info</a></td>");
	output.concat("<td><a href=\"/invert\">Invert status LED</a></td>");
	output.concat("<td><a href=\"/restart\">Restart</a></td>");
	output.concat("</tr></table>");

	output.concat(strInsert);

	output.concat("</body></html>");
}


void handleRoot()
{
	String strInsert;

	String strPage;
	genHtmlPage(strPage,strInsert);

	server.send(200, "text/html", strPage);
}


void handleInvert()	//this gets called when we access /invert on the HTTP server
{

	LeifSetInvertLedBlink(!LeifGetInvertLedBlink());	//toggle the state
	pPropLedInvert->SetBool(LeifGetInvertLedBlink());	//update the homie property too


	String strInsert;

	strInsert="<p>Status LED pattern inverted. Now normally ";
	strInsert+=LeifGetInvertLedBlink()?"OFF":"ON";
	strInsert+=".</p>";

	String strPage;
	genHtmlPage(strPage,strInsert);

	server.send(200, "text/html", strPage);
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
			delay(500);
			ESP.restart();
		}
	);		//invert the LED flashing pattern.

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




		// This is an example of a string property. In this case we will publish a quote once every 90 seconds.

		pPropQuote=pProp=pNode->NewProperty();
		pProp->strID="quote";
		pProp->datatype=homieString;
		pProp->SetRetained(false);
		pProp->strFriendlyName="Quote Generator";
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

	}



	if(Interval1000() && (seconds()%90)==45)	//once every 90 seconds
	{
		static int iQuoteIdx=0;

		//lightly obfuscated list of quotes
		char strObfBuffer[256];

		switch(iQuoteIdx)
		{
		default:
			iQuoteIdx=0;	//no break here
		case 0: { unsigned char obfdata[]={0xaf,0xa3,0x27,0x3e,0xe6,0xc3,0x18,0x83,0x7b,0x50,0x38,0x1d,0x3a,0xa4,0xc0,0x74,0x24,0x3c,0x1d,0xd5,0xf6,0xcf,0x73,0x23,0x3b,0x18,0xd0,0xe1,0x65,0x74,0x24,0x1,0x26,0xd2,0xbe,0xc9,0x45,0x28,0x16,0x25,0xdd,0xba,0x61,0x4d,0x22,0x0,0xfe,0xdf,0xf6,0x91,0x4f,0x37,0xe,0xe8,0xa6,0xf9,0xde,0x78,0x36,0xd,0x2a,0xfc,0xf0,0x74,0x4f,0x37,0x10,0x29,0xa7,0xbc,0x6f,0x4e,0x6e,0x3c,0xf1,0xa9,0x83,0x62,0x53,0x6d,0xe6,0xf0,0xb5,0xf9,0x6c,0x5e,0x6,0x55,0xca,0xa3,0x85,0x7d,0x8a,0xe,0x11,0xc1,0xaa,0xd5,0xac,0x7a,0x33,0xed,0xcf,0xe5,0x93,0x40,0x94,0x8,0xed,0xd5,0xac,0x98,0xb8,0xb3}; int a; for(a=0;a<114;a++) { strObfBuffer[a]=(obfdata[a]^0xe6)-((a*37)&0xFF); } } break;
		case 1: { unsigned char obfdata[]={0xa3,0xaf,0x58,0x3a,0xe0,0xf4,0x14,0x8e,0xa2,0x5e,0xe,0x16,0xf7,0xbf,0xcc,0x77,0x5f,0xb,0x11,0xd9,0xa7,0x92,0x76,0x2c,0xa,0xec,0xd8,0xed,0x69,0x70,0x20,0x9,0x2a,0xd3,0xb8,0x9e,0xbe,0x51,0x0,0xee,0xc1,0xb2,0x6f,0x8f,0xb6}; int a; for(a=0;a<45;a++) { strObfBuffer[a]=(obfdata[a]^0xea)-((a*37)&0xFF); } } break;
		case 2: { unsigned char obfdata[]={0xa4,0x74,0x9c,0xf,0x59,0xf2,0xbd,0x85,0x60,0x2a,0x36,0xe8,0xce,0xec,0x8a,0x74,0x59,0x78,0xe0,0xc0,0xbe,0x6d,0x7a,0x2c,0x32,0x50,0xc6,0xb8,0xc1,0x92,0x56,0x1f,0x2d,0xf3,0xb4,0x6c,0x72,0x6b,0x46,0xf7,0xc0,0xe0,0x6d,0x71,0x2c,0x8,0x2b,0xdf,0xb1,0x69,0x71,0x2d,0x1d,0x24,0xa8,0xb9,0x67,0x47,0x26,0x18,0xfe,0x1f,0x1b}; int a; for(a=0;a<63;a++) { strObfBuffer[a]=(obfdata[a]^0xed)-((a*37)&0xFF); } } break;
		case 3: { unsigned char obfdata[]={0xe0,0x3d,0xdd,0x60,0x42,0x98,0xf4,0x94,0x3e,0xda,0x10,0x0,0xa9,0xfd,0xc5,0x20,0xe,0x53,0xbf,0x68,0xff,0xcf,0x2d,0x8,0x6e,0xb8,0x55,0xe9,0xc6,0x24,0x7e,0x53,0xa3,0x9d,0xaf,0x98,0xca,0xce,0x51,0xb3,0x89,0xe5,0x85,0x1d,0x7c,0x16,0xa4,0x9b,0xe9,0x39,0xed,0x61,0x5e,0xbc,0xf6,0xeb,0x3b,0x15,0x62,0x2,0x7b,0xad,0xa1,0x34,0x16,0x6c,0x58,0x78,0xff,0xd6,0x34,0x7,0x3f,0x59,0xa9,0xf4,0xab,0x27,0xb,0x67,0xb5,0x62,0xf9,0xd0,0x3e,0xde,0x69,0x48,0xae,0xf7,0xd0,0xe2,0xcd,0x28,0x1,0xb3,0xb7,0xda,0x24,0x76,0x6e,0xe,0x99,0xff,0xc0,0x2a,0x9,0x6b,0x7d,0x58,0xa3,0x9c,0x2e,0x7e,0x2d,0xb5,0x84,0xeb,0x35,0x11,0xcf,0x5d,0xb0,0x50,0xd7,0xce,0x1f,0x77,0x52,0x72,0x99,0xe9,0x37,0x12,0x8,0x42,0xac,0x4c,0x45}; int a; for(a=0;a<139;a++) { strObfBuffer[a]=(obfdata[a]^0xb7)-((a*37)&0xFF); } } break;
		case 4: { unsigned char obfdata[]={0x8e,0x5d,0xa9,0x16,0x3e,0xe4,0x86,0xab,0x59,0x3,0x51,0x3b,0xed,0x85,0xe5,0x54,0x7c,0x20,0xc2,0x1c,0x86,0xb4,0x51,0xb0,0x19,0xc1,0xe0,0x9f,0x43,0x59,0x7f,0x33,0xcf,0x32,0xc9,0xb3,0x64,0x46,0x5d,0xcf,0xff,0xce,0xb9,0x5b,0xc,0x62,0xcb,0xf3,0x96,0x40,0x99,0x15,0x2a,0xd6,0xf9,0xd0,0xbf,0x6f,0x6,0x3f,0xda,0xf5,0xa7,0x4c,0x66,0xa,0x75,0xc,0xf6,0xa8,0x40,0xa0,0x1d,0x2d,0xe7,0x83,0xa8,0x55,0x7a,0x48,0x3c,0xea,0x81,0xa7,0x55,0xaa,0x1d,0x37,0xe9,0x92,0xb6,0x84,0x78,0x14,0x75,0xeb,0x9a,0xe6,0x4f,0x7d,0x1b,0xd1,0x2f,0x20}; int a; for(a=0;a<104;a++) { strObfBuffer[a]=(obfdata[a]^0xc3)-((a*37)&0xFF); } } break;
		case 5: { unsigned char obfdata[]={0xd1,0xdd,0x26,0x4f,0x65,0xbf,0xd1,0xbb,0x4,0x2d,0x4f,0x2f,0xb3,0xd7,0xea,0x3c,0xe8,0x41,0x9b,0xbd,0xd2,0xf6,0x38,0x20,0x7e,0x98,0xbf,0x9f,0xf6,0xe,0x52,0x6a,0x9d,0xb2,0xc0,0xb7,0x39,0x26,0x6,0x9c,0xae,0xc9,0xaa,0x33,0x5c,0x7e,0x5e,0xa2,0xfc,0xe5,0x7,0x49,0x3c,0x94,0xb7,0xf9,0xe4,0x3e,0x5e,0x70,0xb8,0xae,0xf1,0xa3,0x31,0x40,0x32,0xbb,0xa4,0xc2,0xa,0xfb,0x29,0x2c,0x87,0x6f,0xf1,0x8,0x20,0x48,0x28,0x85,0xd1,0xf5,0x11,0x24,0x7a,0x2b,0x81,0xd3,0xfe,0xdf,0x21,0x72,0x2e,0xaa,0xd7,0xea,0x17,0x25,0xc,0x62,0xa8,0xd0,0xb0,0x2,0x59,0x71,0x99,0x79,0xd0,0xe8,0x9,0x52,0x76,0x90,0xb2,0xc6,0xe2,0xf9,0xc0}; int a; for(a=0;a<121;a++) { strObfBuffer[a]=(obfdata[a]^0x98)-((a*37)&0xFF); } } break;
		case 6: { unsigned char obfdata[]={0xd2,0xd7,0x2c,0x14,0x9f,0xb0,0xd6,0xe3,0x17,0xf6,0x7a,0x66,0x47,0xd5,0xe4,0xd0,0x25,0x41,0x95,0x44,0xc0,0xe3,0x3b,0x5c,0x7b,0x43,0x79,0xab,0xa8,0x3c,0x20,0x0,0x9f,0xaf,0xc3,0xef,0xcf,0x21,0x77,0x58,0xb2,0xc1,0xe8,0x2a,0x5e,0x74,0x96,0x70,0xc1,0x1f,0x7,0x16,0x29,0x4c,0xb9,0xc0,0xe6,0xc6,0x4d,0x6d,0xbb,0xa9,0xf9,0xa0,0x32,0x43,0x31,0x48,0x97,0xbe,0xce,0xed,0x4,0x22,0x7c,0x90,0xb7,0xd6,0xed,0x0,0x5b,0x4e,0xb4,0xc9,0xf3,0xec,0x15,0x47,0x44,0xd1,0xb9,0xe,0x20,0x7e,0x2d,0x84,0xd4,0xf5,0x3,0x2d,0xf,0x96,0xbd,0xdf,0xe7,0xd6,0x5d,0x7d,0x27,0xb8,0xc3,0xf7,0xc,0x1a,0x1,0x9a,0xb2,0xd7,0xf4,0x33,0x50,0x79,0x59,0xdb,0xc0,0x1d,0xcd,0x5d,0x75,0x8f,0xda,0x94,0x10,0x3a,0x24,0x6c,0x4f,0x76,0xa0,0xac,0x3b,0x4b,0x6f,0x49,0xdf,0x8e,0x19,0x3b,0x41,0x75,0x55,0xdc,0xfc,0xa6,0x3f,0x42,0x76,0x83,0x9f,0x60}; int a; for(a=0;a<160;a++) { strObfBuffer[a]=(obfdata[a]^0x9b)-((a*37)&0xFF); } } break;
		};

		iQuoteIdx++;

		pPropQuote->SetValue(strObfBuffer);

	}


}
