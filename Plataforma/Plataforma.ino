/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-async-web-server-espasyncwebserver-library/
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

// Import required libraries
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "FastAccelStepper.h"

//////////////////////////CONTROL///////////////////////////////////////
#define dirPinStepper 12
#define enablePinStepper 16
#define stepPinStepper 14

const int SensorComienzo = 23;
const int SensorFinal = 21;

const int Relay1 = 32;
const int Relay2 = 33;

int PosicionFinal = 0;

int state;
const int CalibrateStart = 1;
const int LookingForZero = 2;
const int LookingForEnd = 3;
const int BuscandoHome = 4;
const int cccccc = 5;
const int dddddd = 6;
const int eeeeee = 7;

const int Calibrated = 10;
const int Iddle = -1;


FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepper = NULL;

char user_input;

////////////////////////NETWORK/////////////////////////////////////////////

// Replace with your network credentials
const char* ssid = "PlataformaESP";
const char* password = "11111111";

const char* PARAM_INPUT_1 = "output";
const char* PARAM_INPUT_2 = "state";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    buttons += "<h4>Output - GPIO 2</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"2\" " + outputState(2) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>Output - GPIO 4</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"4\" " + outputState(4) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>Output - GPIO 33</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"33\" " + outputState(33) + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}

String outputState(int output){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  pinMode(33, OUTPUT);
  digitalWrite(33, LOW);
  
WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
    }
    else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    Serial.print("GPIO: ");
    Serial.print(inputMessage1);
    Serial.print(" - Set to: ");
    Serial.println(inputMessage2);
    request->send(200, "text/plain", "OK");
  });

  // Start server
  server.begin();
    Serial.println("Server started");

Serial.println("Begin motor control");
  Serial.println();
  //Print function list for user selection
  Serial.println("Opciones por serial:");
  Serial.println("a. calibrar.");
  Serial.println("<. movimiento adelante.");
  Serial.println(">. movimiento de regreso.");
  Serial.println("s. stop abrupto.");
  Serial.println("1. toggle relay 1.");
  Serial.println("2. toggle relay 2.");
  Serial.println("c. mover inmediatamente counterclockwise.");
  Serial.println("w. mover inmediatamente clockwise.");
  Serial.println();

  pinMode(SensorComienzo, INPUT);
  pinMode(SensorFinal, INPUT);

  pinMode(Relay1, OUTPUT);

  engine.init();
  stepper = engine.stepperConnectToPin(stepPinStepper);
  if (stepper) {
    stepper->setDirectionPin(dirPinStepper);
    stepper->setEnablePin(enablePinStepper);
    stepper->setAutoEnable(true);

    // If auto enable/disable need delays, just add (one or both):
    // stepper->setDelayToEnable(50);
    // stepper->setDelayToDisable(1000);

    stepper->setSpeedInUs(500);
      stepper->setAcceleration(100);
  }

delay(1000);
  digitalWrite(Relay1, HIGH);
  digitalWrite(Relay2, HIGH);
  delay(1000);
digitalWrite(Relay1, LOW);
digitalWrite(Relay2, LOW);
}

void loop() {
/*if (digitalRead(SensorComienzo) == LOW)
    {
Serial.print("CL");
    }
    else
    {
Serial.print("CH");
    }
    if (digitalRead(SensorFinal) == LOW)
    {
Serial.println("FL");
    }
    else
    {
Serial.println("FH");
    }
*/
  if (state == CalibrateStart)
  {
    if (digitalRead(SensorComienzo) == LOW)
    {
      //brazo esta en pos inicial, zerow, buscar pos final
      stepper->setSpeedInUs(500);  // the parameter is us/step !!!
      stepper->setAcceleration(100);
      stepper->runBackward();
      state = LookingForZero;
      Serial.print("CalibrateStart LOW");
    }
    else
    {
      //buscar pos inicial
      stepper->setSpeedInUs(500);  // the parameter is us/step !!!
      stepper->setAcceleration(100);
      stepper->runForward();
      
      state = BuscandoHome;
      Serial.print("CalibrateStart HIGH");
    }

    
  } else if (state == LookingForZero)
  {
    if (digitalRead(SensorComienzo) == HIGH)
    {
      stepper->setCurrentPosition(0);
      state = LookingForEnd;
      Serial.print("LookingForZero HIGH");
    }
  }
   else if (state == LookingForEnd)
  {
    Serial.println("LookingForEnd");
    if (digitalRead(SensorFinal) == LOW)
    {
      PosicionFinal=stepper->getCurrentPosition();
      stepper->forceStop();
      state = Calibrated;
            Serial.print("LookingForZero LOW");

    }
  }
  else if (state == Calibrated)
  {
    state = Iddle;
    Serial.println("Done");
    Serial.print("Posicion final: ");
    Serial.println(PosicionFinal);
  }
  else if (state == BuscandoHome)
  {
          Serial.println("BuscandoHome");

    if (digitalRead(SensorComienzo) == LOW)
    {
      stepper->forceStop();

      stepper->setSpeedInUs(500);
      stepper->setAcceleration(100);
      stepper->runBackward();
      state = LookingForZero;
      Serial.print("BuscandoHome LOW");

    }
  }

  if (stepper) {
    if (stepper->isRunning()) {
      //Serial.print("@");
      //Serial.println(stepper->getCurrentPosition());
    }
  } else {
    Serial.println("Stepper died?");
    Serial.flush();
    delay(10000);
  }

  while (Serial.available()) {
    user_input = Serial.read(); //Read user input and trigger appropriate function
    if (user_input == 'c')
    {
      //counterclockwise
      stepper->runForward();
    }
    else if (user_input == 'w')
    {
      //clockwise
      stepper->runBackward();
    }
    else if (user_input == 's')
    {

      stepper->forceStop();
    }
    else if (user_input == '<')
    {
        stepper->setSpeedInUs(1000);
      stepper->setAcceleration(500);
      stepper->moveTo(-200);
    }
    else if (user_input == '>')
    {
      stepper->setSpeedInUs(1000);
      stepper->setAcceleration(500);
      stepper->moveTo(PosicionFinal+200);
    }
    else if (user_input == 'a')
    {
      state = CalibrateStart;
    }
    else if (user_input == '1')
    {
      digitalWrite(Relay1, !digitalRead(Relay1));
    }else if (user_input == '2')
    {
            digitalWrite(Relay2, !digitalRead(Relay2));
    }
    else
    {
      //Serial.println("Invalid option entered.");
    }
  }
}
