#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <DoubleResetDetector.h>  //https://github.com/datacute/DoubleResetDetector
#include <DNSServer.h>
#include <Ultrasonic.h>
#include <ESP8266mDNS.h>

#define DRD_TIMEOUT 5

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 20
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

// static const uint8_t D0   = 16;
// static const uint8_t D1   = 5;
// static const uint8_t D2   = 4;
// static const uint8_t D3   = 0;
// static const uint8_t D4   = 2;
// static const uint8_t D5   = 14;
// static const uint8_t D6   = 12;
// static const uint8_t D7   = 13;
// static const uint8_t D8   = 15;
// static const uint8_t RX   = 3;
// static const uint8_t TX   = 1;
#pragma GCC diagnostic ignored "-Wwrite-strings"

#define FIRE_PIN 4                          //maps to pin 2
const int PIN_LED = 2;

bool initialConfig = false;
boolean    continuousFire = false;
int delayTime = 0; //stored in seconds
boolean useDistance = false;
boolean useMotionSensor = false;
int disMax =0;
int disMin =0;

int continuousFire_eeprom = 100;
int delayTime_eeprom =101;
int useDistance_eeprom =102;
int disMin_eeprom =103;
int disMax_eeprom =104;
int useMotionSensor_eeprom=105;

String content ="";
String st ="";
String s = "";
const char* u = "";

ESP8266WebServer myserver(80);                     // create object

void ContinuousFire(){
  if (continuousFire==true){
    if (delayTime<4) return;
    delay(delayTime*1000);
    Serial.println(delayTime*1000);
    BreathFire();
  }else{
    return;
  }
}
void UseDistance(){
  if (useDistance==true){
    Ultrasonic ultrasonic(15,13); //D8 = 15 = Echo, don't forget to use a voltage divider to go to 3.3v D7=13=Trig
    int dis1 = 0;
    int dis2 = 0;
    int dis3 = 0;
    int dis4 = 0;
    int dis5 = 0;
    int disavg = 0;
    dis1 = ultrasonic.Ranging(INC);
    delay(100);
    dis2= ultrasonic.Ranging(INC);
    delay(100);
    dis3= ultrasonic.Ranging(INC);
    delay(100);
    dis4= ultrasonic.Ranging(INC);
    delay(100);
    dis5= ultrasonic.Ranging(INC);
    disavg = (dis1+dis2+dis3+dis4+dis5)/5;
    Serial.print(disavg); // CM or INC
    Serial.println(" in" );
    if (disavg < disMax){
      if(disavg > disMin){
        BreathFire();
        delay(4000);
      }
    }
  }else {return;}
}


void UseMotion(){
  if(useMotionSensor==true){
    if (digitalRead(12)){
      Serial.println(digitalRead(12));
      BreathFire();
      delay(5000);
    }
  }else{return;}
}


void BreathFire(){
  digitalWrite(FIRE_PIN, LOW);
  delay(200);
  digitalWrite(FIRE_PIN, HIGH);
  Serial.println(F("Breathing Fire!"));
}

String GetPass(){
	EEPROM.begin(512);
	String epass="";
	Serial.println("Reading EEPROM pass");
	for (int i = 32; i < 96; ++i)
	{
	  epass += char(EEPROM.read(i));
	}
	Serial.print("PASS: ");
	Serial.println(epass);
	EEPROM.end();
	return epass;
}

String GetSSID(){
	EEPROM.begin(512);
	String ssid="";
	Serial.println("Reading EEPROM ssid");
	for (int i = 0; i < 32; ++i)
	{
	ssid += char(EEPROM.read(i));
	}
	Serial.print("SSID: ");
	Serial.println(ssid);
	EEPROM.end();
	return ssid;
}

void setup()
{
  Serial.begin(74880);                     // native bootloader channel (DEV Only)
  bool webtype = false;
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);

  WiFi.mode(WIFI_STA);
  WiFi.begin(GetSSID().c_str(), GetPass().c_str());
  if (drd.detectDoubleReset()) {
    Serial.println("Double Reset Detected");
    webtype = true;
	digitalWrite(PIN_LED, LOW);
	//WiFi.mode(WIFI_AP);
	WiFi.softAP("Pumpkin", "principeazul");
  }
  while ( WiFi.status() != WL_CONNECTED ) {
		delay ( 500 );
		Serial.print ( "." );
	}


myserver.begin();



  myserver.on("/index.html", [](){
    myserver.send(200, "text/html", s);
  });

	if ( webtype == true ) {
		Serial.print("My IP is:");
	  Serial.println(WiFi.softAPIP());
		myserver.on("/", []() {
		  IPAddress ip = WiFi.softAPIP();
		  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
		  content = "<!DOCTYPE HTML>\r\n<html>Hello from The Pumpkin King I need your wifi info or I'll be quite lame. ";
		  content += ipStr;
		  content += "<p>";
		  content += st;
		  content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><label>password: </label><input name='pass' length=64><input type='submit'></form>";
		  content += "</html>";
		  myserver.send(200, "text/html", content);
		  Serial.println("Ok, I got this far...");
		});

		myserver.on("/setting", []() {
			EEPROM.begin(512);
			String qsid = myserver.arg("ssid");
			String qpass = myserver.arg("pass");
			// WiFi.mode(WIFI_STA);
			// WiFi.begin(qsid.c_str(), qpass.c_str());
			// while ( WiFi.status() != WL_CONNECTED ) {
				// delay ( 500 );
				// Serial.print ( "." );
			// }
			int statusCode;
			if (qsid.length() > 0 && qpass.length() > 0) {
			  Serial.println("clearing eeprom");
			  for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
			  Serial.println(qsid);
			  Serial.println("");
			  Serial.println(qpass);
			  Serial.println("");

			  Serial.println("writing eeprom ssid:");
			  for (int i = 0; i < qsid.length(); ++i)
				{
				  EEPROM.write(i, qsid[i]);
				  Serial.print("Wrote: ");
				  Serial.println(qsid[i]);
				}
			  Serial.println("writing eeprom pass:");
			  for (int i = 0; i < qpass.length(); ++i)
				{
				  EEPROM.write(32+i, qpass[i]);
				  Serial.print("Wrote: ");
				  Serial.println(qpass[i]);
				}
			  EEPROM.commit();
			  Serial.println(GetSSID());
			  Serial.println(GetPass());
			  EEPROM.end();
			  content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi at IP \"}";
				//Serial.println(WiFi.localIP());
			  statusCode = 200;
			} else {
			  content = "{\"Error\":\"404 not found\"}";
			  statusCode = 404;
			  Serial.println("Sending 404");
			}
			myserver.send(statusCode, "application/json", content);
		});
	}else{
		myserver.on("/", [](){
		myserver.send(200, "text/html", s);
	});
	}

  u = "<b>Remote Update: <form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
  myserver.on("/u", [](){
    myserver.send(200, "text/html", u);
  });
  myserver.on("/update", HTTP_POST, [](){
        myserver.sendHeader("Connection", "close");
        myserver.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
        ESP.restart();
      },[](){
        HTTPUpload& upload = myserver.upload();
        if(upload.status == UPLOAD_FILE_START){
          Serial.setDebugOutput(true);
          WiFiUDP::stopAll();
          Serial.printf("Update: %s\n", upload.filename.c_str());
          uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
          if(!Update.begin(maxSketchSpace)){//start with max available size
            Update.printError(Serial);
          }
        } else if(upload.status == UPLOAD_FILE_WRITE){
          if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
            Update.printError(Serial);
          }
        } else if(upload.status == UPLOAD_FILE_END){
          if(Update.end(true)){ //true to set the size to the current progress
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          } else {
            Update.printError(Serial);
          }
          Serial.setDebugOutput(false);
        }
        yield();
  });

  myserver.on("/Stop", [](){
    myserver.send(200, "text/html", s);
    continuousFire=false;
    EEPROM.begin(512);
    EEPROM.put(continuousFire_eeprom, continuousFire);   //EEPROM.put(address, data)
    EEPROM.put(delayTime_eeprom, delayTime);
    EEPROM.commit();
    EEPROM.end();
  });

  myserver.on("/UseMotion", [](){
    myserver.send(200, "text/html", s);
    useMotionSensor=true;
    EEPROM.begin(512);
    EEPROM.put(useMotionSensor_eeprom, useMotionSensor);   //EEPROM.put(address, data)
    EEPROM.commit();
    EEPROM.end();
  });

  myserver.on("/StopMotion", [](){
    myserver.send(200, "text/html", s);
    useMotionSensor=false;
    EEPROM.begin(512);
    EEPROM.put(useMotionSensor_eeprom, useMotionSensor);   //EEPROM.put(address, data)
    EEPROM.commit();
    EEPROM.end();
  });

  myserver.on("/Fire", [](){
    myserver.send(200, "text/html", s);
    BreathFire();
  });

  myserver.on("/ContinuousFire", [](){
    myserver.send(200, "text/html", s);
    if (myserver.arg("continuousFire")=="ON")
    {
      continuousFire=1;
      delayTime = myserver.arg("delayTime").toInt();
    }else{
      continuousFire =0;
    }
    EEPROM.begin(512);
    EEPROM.put(continuousFire_eeprom, continuousFire);   //EEPROM.put(address, data)
    EEPROM.put(delayTime_eeprom, delayTime);
    EEPROM.commit();
    EEPROM.end();
  });

  myserver.on("/StopDistance", [](){
    myserver.send(200, "text/html", s);
    useDistance = false;
    EEPROM.put(useDistance_eeprom, useDistance);   //EEPROM.put(address, data)
  });

  myserver.on("/Distance", [](){
    myserver.send(200, "text/html", s);

    useDistance=1;
    disMax = myserver.arg("disMax").toInt();
    disMin = myserver.arg("disMin").toInt();

    EEPROM.begin(512);
    EEPROM.put(useDistance_eeprom, useDistance);   //EEPROM.put(address, data)
    EEPROM.put(disMin_eeprom, disMin);
    EEPROM.put(disMax_eeprom, disMax);
    EEPROM.commit();
    EEPROM.end();
  });
  myserver.begin();

	//MDNS.addService("http", "tcp", 80);

  Serial.println(WiFi.localIP());

    s = "";
    s += " <html> \r\n";
  s += " <head>\r\n";
  s += " <link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css' integrity='sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u' crossorigin='anonymous'>\r\n";
  s += " \r\n";
  s += " <link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap-theme.min.css' integrity='sha384-rHyoN1iRsVXV4nD0JutlnGaslCJuC7uwjduW9SVrLvRYooPp2bWYgmgJQIXwl/Sp' crossorigin='anonymous'>\r\n";
  s += " \r\n";
  s += " <script src='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js' integrity='sha384-Tc5IQib027qvyjSMfHjOMaLkfuWVxZxUPnCJA7l2mCWNIpG9mGCD8wGNIcPD7Txa' crossorigin='anonymous'></script>\r\n";
  s+=" <style>\r\n";
  s+="body {\r\n";
  s+="background: black;\r\n";
  s+="background-image: url('https://s-media-cache-ak0.pinimg.com/originals/71/69/55/7169557886496c3f08080005e108d376.jpg');\r\n";
  s+="background-repeat: no-repeat;\r\n";
  s+="background-position: center;\r\n";
  s+="}\r\n";
  s+="</style>\r\n";
  s += " </head>\r\n";
  s += " <body style='text-align:center;'>\r\n";
  s += " <center>\r\n";
  s += " <br/><form action='/Fire'>\r\n";
  s += " <button type='submit' class='btn btn-default btn-lg btn-danger'>\r\n";
  s += "  <span class='glyphicon glyphicon-fire' ></span> Breath Fire!\r\n";
  s += " </button><br/><br/>\r\n";
  s += " </form>\r\n";
  s += " <table width='400px'>\r\n";
  s += " <tr><td>\r\n";
  s += " <form action='/ContinuousFire'> \r\n";
  s += " <input type='hidden'  name='continuousFire' value='ON'/>\r\n";
  s += " <div class='input-group'>\r\n";
  s += "   <input type='text' class='col-xs-2 form-control' style='width:150;' name='delayTime' placeholder='Delay in Seconds'>\r\n";
  s += "   <span class='input-group-btn'>\r\n";
  s += "         <button class='btn btn-default btn-danger' style='width:175;'type='submit'>Keep Breathing Fire!</button>\r\n";
  s += "    </span>\r\n";
  s += "   <span class='input-group-btn'>\r\n";
  s += "         <a href='/Stop'><button class='btn btn-default btn-warning' style='width:75;' type='button'>Pause</button></a>\r\n";
  s += "    </span>\r\n";
  s += " </div>\r\n";
  s += " </form>\r\n";
  s += " </tr></td>\r\n";
  s += " <tr><td>\r\n";
  s += " <form action='/Distance'> \r\n";
  s += " <div class='input-group'>\r\n";
  s += "   <span class='input-group-btn'>\r\n";
  s += "   <input type='text' class='col-xs-1 form-control' style='width: 75px;' name='disMin' placeholder='Min'>\r\n";
  s += "   </span'>\r\n";
  s += "   <span class='input-group-btn'>\r\n";
  s += "   <input type='text' class='col-xs-1 form-control' style='width: 74px;' name='disMax' placeholder='Max'>\r\n";
  s += "   </span'>\r\n";
  s += "   <span class='input-group-btn'>\r\n";
  s += "         <button class='btn btn-default btn-danger' style='width:175;' type='submit'>Range Based Fire!</button>\r\n";
  s += "    </span>\r\n";
  s += "   <span class='input-group-btn'>\r\n";
  s += "         <a href='/StopDistance'><button class='btn btn-default btn-warning' style='width:75;' type='button'>Pause</button></a>\r\n";
  s += "    </span>\r\n";
  s += " </div>\r\n";
  s += " </form>\r\n";
  s += " </tr></td>\r\n";
  s += " <tr><td>\r\n";
  s += " <form action='/UseMotion'> \r\n";
  s += " <div class='input-group'>\r\n";
  s += "   <span class='input-group-btn'>\r\n";
  s += "         <button class='btn btn-default btn-danger' style='width:200;' type='submit'>Fire on Motion!</button>\r\n";
  s += "    </span>\r\n";
  s += "   <span class='input-group-btn'>\r\n";
  s += "         <a href='/StopMotion'><button class='btn btn-default btn-warning' style='width:200;' type='button'>Pause</button></a>\r\n";
  s += "    </span>\r\n";
  s += " </div>\r\n";
  s += " </form>\r\n";
  s += " </tr></td>\r\n";
  s += " </table>\r\n";
  s += " </center>\r\n";
  s += " </html>\r\n";


    EEPROM.begin(512);
  EEPROM.get(continuousFire_eeprom, continuousFire);
  EEPROM.get(delayTime_eeprom, delayTime);
  delayTime=delayTime*1000;
  EEPROM.get(useDistance_eeprom, useDistance);
  EEPROM.get(disMin_eeprom, disMin);
  EEPROM.get(disMax_eeprom, disMax);
  EEPROM.get(useMotionSensor_eeprom, useMotionSensor);
  EEPROM.end();

  pinMode(FIRE_PIN, OUTPUT);
  digitalWrite(FIRE_PIN, HIGH);
  pinMode(12, INPUT);

    //MDNS.addService("http", "tcp", 80);
}

void loop()
{
	myserver.handleClient();
	drd.loop();
	ContinuousFire();
	UseDistance();
	UseMotion();
}
