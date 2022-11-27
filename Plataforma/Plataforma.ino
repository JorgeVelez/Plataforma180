/*********
  Jorge Vélez

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

const char* PARAM_RELAYA = "relaya";
const char* PARAM_RELAYB = "relayb";

const char* PARAM_AVANZA = "avanza";
const char* PARAM_RETROCEDE = "retrocede";
const char* PARAM_CALIBRA = "calibra";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Plataforma</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;  background-color: rgb(0, 0, 0);}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
        .button {
  background-color: #04AA6D;
  border: none;
  color: white;
  padding: 20px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
  margin: 4px 2px;
  border-radius: 12px;
}
.button1 { background-color: #FC5C65;}
.button2 { background-color: #A65EEA;}
.button3 { background-color: #F7B731;}
.button4 { background-color: #10B9B1;}
  </style>
</head>
<body>

  %BUTTONPLACEHOLDER%
<script>
function toggleCheckbox(command) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/update?command="+command, true);
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
    buttons += "<button class=\"button button1\"  onclick=\"toggleCheckbox(\'avanza\')\" >Avanza</button>";
    buttons += "<button class=\"button button2\"  onclick=\"toggleCheckbox(\'retrocede\')\"  >Retrocede</button>";
    buttons += "</br>";
    buttons += "<button class=\"button button4\"  onclick=\"toggleCheckbox(\'relaya\')\" >RelayA</button>";
    buttons += "<button class=\"button button4\"  onclick=\"toggleCheckbox(\'relayb\')\" >RelayB</button>";
    buttons += "</br>";
    buttons += "<button class=\"button button3\"  onclick=\"toggleCheckbox(\'calibra\')\"  >Calibrar</button>";

  
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

  /*para wifi
   *  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());
  
   */

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam("command") ){
      String inputMessage;
      inputMessage = request->getParam("command")->value();
      
      if ((inputMessage==PARAM_RETROCEDE)){
stepper->setSpeedInUs(1000);
      stepper->setAcceleration(500);
      
      stepper->moveTo(PosicionFinal+200);
          Serial.println("command PARAM_RETROCEDE ");

    }else if ((inputMessage==PARAM_AVANZA) ){
      stepper->setSpeedInUs(1000);
      stepper->setAcceleration(500);
      stepper->moveTo(-200);
    Serial.println("command PARAM_AVANZA ");

    }else if ((inputMessage==PARAM_CALIBRA) ){
      
state = CalibrateStart;
    Serial.println("command PARAM_CALIBRA ");

    }else if ((inputMessage==PARAM_RELAYA) ){
      digitalWrite(Relay1, !digitalRead(Relay1));
    Serial.println("command PARAM_RELAYA ");

    }else if ((inputMessage==PARAM_RELAYB) ){
      
  digitalWrite(Relay2, !digitalRead(Relay2));
      Serial.println("command PARAM_RELAYB ");

    }
    else {
     
    Serial.println("command unknown ");

    }

    }

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
          //Serial.println("BuscandoHome");

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
