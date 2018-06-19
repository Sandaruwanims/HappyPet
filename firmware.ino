/* D0 - DataOut LS 
   D1 - Water Pump
   D2 - CLK LS
   D3 - Flow Meter
   D4 - Servo Moter
   A0 - Sharp IR
*/

// Include the libraries that we'll be using throughout the code
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include <Servo.h>
#include "HX711.h"

#define DOUT  D0
#define CLK  D2
#define WPUMP D1
#define FMETER D3
#define SERVO D4
#define DISTANCE A0

//servo angles
#define closed_position 50
#define open_position 8

//weight limit for 50 unit
#define STEP_WEIGHT 0.5

//WIFI credentials
const char* ssid = "Dialog 4G";
const char* password = "MT10E7FM8HF";

// Instantiate the ESP8266WebServer class, passing the argument 80 for the
// port that the server will be listening.
ESP8266WebServer server(80);
HX711 scale(DOUT, CLK);
Servo myservo;

int pos = 0;    // variable to store the servo position

int sent_weight = 0;
int count_50 = 0;
float weight = 0;
float remains = 0;
    
//Change this calibration factor as per your load cell once it is found you many need to vary it in thousands
float calibration_factor = -96650; //-106600 worked for my 40Kg max scale setup 

//flow meter variables
byte sensorInterrupt = D3;
float calibrationFactor = 4.5;
volatile byte pulseCount; 

float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long oldTime;

void flowRateMeasure();
void drop_50();
void measure();


void handleRoot() {
  server.send(200, "text/plain", String("Hello from esp8266! Usage: navigate to") +
              String(" /setleds?led1=XX&led2=YY changing XX/YY to ON or OFF."));
}


void dropwater() {
  //turn on the water pump
  digitalWrite(WPUMP, LOW);

  //drop 500ml using flow meter reading
  while(true){
    flowRateMeasure();
    if(totalMilliLitres > 500){
      //turn off the water pump
      digitalWrite(WPUMP, HIGH);
      totalMilliLitres = 0;
      break;
    }
    delay(5);
  }
}


//feed function
void handleNotFound() {
  
  //weight sent by user(app)
  sent_weight = server.arg(0).toFloat();

  //identify the dogs motion
  if(server.uri().equals("feed")){
    int ADCaverage = 0;
    while(ADCaverage > 160){
      ADCaverage = 0;
      for(int x=0; x<100; x++){
        ADCaverage += analogRead(A0);
        delay(1);
      }
      ADCaverage = ADCaverage/100;
    }
    


    //devide the weight into 50 units
    count_50 = sent_weight/50;

    //feed by 50 units
    for(int i=0; i<count_50; i++){
      drop_50();

      //wait until dog eat the droped food
      while(true){
        remains = 0;
        for(int x=0; x<20; x++){
          measure();
          remains = remains + weight;
          delay(50);
        }
        if(remains < 0.02){
          break;
        }
      }
      
    }
  }

  server.send(200, "text/plain", "Custom Plan"); 
}



void setup(void) {
  Serial.begin(115200);
  
  //  Initialize the WiFi client and try to connect to our Wi-Fi network
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for a successful connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // For debugging purposes print the network ID and the assigned IP address
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Associate the URLs with the functions that will be handling the requests
  server.on("/", HTTP_GET, handleRoot);
  server.on("/water", HTTP_GET, dropwater);
  server.onNotFound(handleNotFound);

  // Start running the webserver
  server.begin();
  Serial.println("HTTP server started");

  scale.set_scale(-96650);  //Calibration Factor obtained from first sketch
  scale.tare();             //Reset the scale to 0  
  myservo.attach(D4);

  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;
  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  
  pinMode(WPUMP, OUTPUT);
  digitalWrite(WPUMP, HIGH);
}



// The loop function is straight-forward, simply handle any incoming requests to the
// our ESP8266 host!
void loop(void) {
  server.handleClient();
}
