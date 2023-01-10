// Import required libraries
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "FastAccelStepper.h"
#include "EEPROM.h"

//////////////////////////CONTROL///////////////////////////////////////
#define dirPinStepper 12
#define enablePinStepper 16
#define stepPinStepper 14

IPAddress local_ip(192, 168, 0, 1);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

#define isDebug false

const int SensorComienzo = 23;
const int SensorFinal = 21;

const int Relay1 = 32;
const int Relay2 = 33;

int PosicionFinal = 0;
int PosicionInicial = 0;

bool estaEnInicio = false;

int velocidadSecuencia = 900;
int aceleracionSecuencia = 4000;

int velocidadCalibrar = 500;
int aceleracionCalibrar = 4000;

int state;
const int CalibrateStart = 1;
const int LookingForZero = 2;
const int LookingForEnd = 3;
const int BuscandoHome = 4;
const int Moviendo = 5;
const int Rutina = 6;

const int Iddle = -1;
int velocidad = 7;

unsigned long StartTime = 0;


/////////////////////////////EEPROM////////////////////////////
int addressVelocity = 0;
#define EEPROM_SIZE 64


////////////////////////////STEPPER////////////////////////////
FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepper = NULL;

char user_input;

// Replace with your network credentials
const char* ssid = "Plataforma180";
const char* password = "180180180";

const char* ssid_debug = "INFINITUMC651";//ChosgangT//INFINITUMC651
const char* password_debug = "2Me4bbcEds";//Sonisgang2//2Me4bbcEds

const char* PARAM_RELAYA = "relaya";
const char* PARAM_RELAYB = "relayb";

const char* PARAM_AVANZA = "avanza";
const char* PARAM_PARAR = "parar";
const char* PARAM_CALIBRA = "calibra";
const char* PARAM_RUTINA = "rutina";
const char* PARAM_VELMAS = "velmas";
const char* PARAM_VELMENOS = "velmenos";


bool ledState = 0;
const int ledPin = 2;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Plataforma 180</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
  html {
    font-family: Arial, Helvetica, sans-serif;
    text-align: center;
  }
  h1 {
    font-size: 1.8rem;
    color: white;
  }
  h2{
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
  }
  .topnav {
     background-color: #000000;
  }
  body {
    margin: 0;
    background-color: #000000;
  }
  .content {
    padding: 30px;
    max-width: 600px;
    margin: 0 auto;
  }
  .card {
    background-color: #F8F7F9;;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:20px;
  }
  .buttonC {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #FC5C65;
    border: none;
    border-radius: 5px;
   }
     .buttonA {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #A65EEA;
    border: none;
    border-radius: 5px;
   }
     .buttonP {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #FF0000;
    border: none;
    border-radius: 5px;
   }
     .buttonS {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #F7B731;
    border: none;
    border-radius: 5px;
   }

    .buttonV {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #10B9B1;
    border: none;
    border-radius: 5px;
   }
   /*.button:hover {background-color: #0f8b8d}
   .buttonC:active {
     background-color: #FC5C65;
     box-shadow: 2 2px #CDCDCD;
     transform: translateY(2px);
   }*/
   .state {
     font-size: 1.5rem;
     color:#8c8c8c;
     font-weight: bold;
   }
  </style>
<title>Plataforma 180</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body>
  <div class="topnav">
    <img src="logo">
  </div>
  <div class="content">
    <div>
      <p class="state" style="display:none" id="textoCalibrando">Calibrando...</p>
      <p><button style="display:none" id="buttonU" class="buttonA">RUTINA</button></p>
      <p class="state" style="display:none" id="textoVel">Velocidad: <span  id="state">%VEL%s</span></p>
      <p><button style="display:none" id="buttonVmas" class="buttonV">+</button></p>
      <p><button style="display:none" id="buttonVmenos" class="buttonV">-</button></p>
      <p><button style="display:none" id="buttonA" class="buttonA">CAMBIO LADO</button></p>
      <p><button style="display:none" id="buttonP" class="buttonP">PARAR</button></p>
      <p><button style="display:none" id="buttonS1" class="buttonS">SWITCH1</button></p>
      <p><button style="display:none" id="buttonS2" class="buttonS">SWITCH2</button></p>
      <p><button id="buttonC" class="buttonC">CALIBRAR</button></p>

    </div>
  </div>
<script>
var PARAM_RELAYA = "relaya";
var PARAM_RELAYB = "relayb";

var PARAM_AVANZA = "avanza";
var PARAM_PARAR = "parar";
var PARAM_CALIBRA = "calibra";

var PARAM_RUTINA = "rutina";
var PARAM_VELMAS = "velmas";
var PARAM_VELMENOS = "velmenos";

  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    var state;
    if (event.data == "calibrado"){
      
     document.getElementById('buttonA').style.display = 'inline';
     document.getElementById('buttonP').style.display = 'inline';
    document.getElementById('buttonU').style.display = 'inline';
    document.getElementById('buttonS1').style.display = 'inline';
    document.getElementById('buttonS2').style.display = 'inline';
    document.getElementById('buttonVmenos').style.display = 'inline';
    document.getElementById('buttonVmas').style.display = 'inline';
    document.getElementById('textoVel').style.display = 'inline';
     document.getElementById('textoCalibrando').style.display = 'none';
     document.getElementById('buttonC').style.display = 'inline';
    }
    else{
      document.getElementById('state').innerHTML = event.data+"s";
    }
  }
  
  function onLoad(event) {
    initButtonS();
    initWebSocket();
  }
  function initButtonS() {
    document.getElementById('buttonC').addEventListener('click', () => { buttonHandler(PARAM_CALIBRA) });
    document.getElementById('buttonA').addEventListener('click', () => { buttonHandler(PARAM_AVANZA) });
    document.getElementById('buttonP').addEventListener('click', () => { buttonHandler(PARAM_PARAR) });
    document.getElementById('buttonU').addEventListener('click', () => { buttonHandler(PARAM_RUTINA) });
    document.getElementById('buttonS1').addEventListener('click', () => { buttonHandler(PARAM_RELAYA) });
    document.getElementById('buttonS2').addEventListener('click', () => { buttonHandler(PARAM_RELAYB) });
    document.getElementById('buttonVmas').addEventListener('click', () => { buttonHandler(PARAM_VELMAS) });
    document.getElementById('buttonVmenos').addEventListener('click', () => { buttonHandler(PARAM_VELMENOS) });
  }
  function buttonHandler(msg){
    websocket.send(msg);

    if(msg==PARAM_CALIBRA){
      document.getElementById('buttonC').style.display = 'none';
      document.getElementById('textoCalibrando').style.display = 'inline';
    }
    console.log('msg:'+msg);
  }
</script>
</body>
</html>
)rawliteral";

void notifyClients(String msg) {
  ws.textAll(msg);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    //aqui notificas y haces cambio de hardware
    if (strcmp((char*)data, PARAM_AVANZA) == 0) {
      state= Moviendo;
      if(!estaEnInicio){
        estaEnInicio=true;
        MueveAPosInicial();
      }else{
        estaEnInicio=false;
        MueveAPosFinal();
      }
      Serial.println("command PARAM_AVANZA ");
    }if (strcmp((char*)data, PARAM_PARAR) == 0) {
      stepper->forceStop();
      state = Iddle;
      Serial.println("command PARAM_PARAR ");
    }else if (strcmp((char*)data, PARAM_RUTINA) == 0) {
      state= Rutina;
      StartTime = millis();
      Serial.print("estaEnInicio: ");
      Serial.println(estaEnInicio);
       if(!estaEnInicio){
        MueveAPosInicial();
      }else{
        MueveAPosFinal();
      }
      Serial.println("command PARAM_RUTINA ");
    }else if (strcmp((char*)data, PARAM_CALIBRA) == 0) {    
      state = CalibrateStart;
        if(isDebug){
      delay(2000);
      notifyClients("calibrado");
        }
      Serial.println("command PARAM_CALIBRA ");
    }else if (strcmp((char*)data, PARAM_VELMAS) == 0) {    
      velocidad++;
      if(velocidad>15)
      velocidad=15;
      EEPROM.writeInt(addressVelocity, velocidad); 
      EEPROM.commit();
       delay(100);
      notifyClients(String(velocidad));
      Serial.println("command PARAM_VELMAS ");
      Serial.println(velocidad);
    }else if (strcmp((char*)data, PARAM_VELMENOS) == 0) {    
      velocidad--;
      if(velocidad<5)
      velocidad=5;
       EEPROM.writeInt(addressVelocity, velocidad); 
       EEPROM.commit();
       delay(100);
      notifyClients(String(velocidad));
      Serial.println("command PARAM_VELMENOS ");
      Serial.println(velocidad);
    }else if (strcmp((char*)data, PARAM_RELAYA) == 0) {
      digitalWrite(Relay1, !digitalRead(Relay1));
      Serial.println("command PARAM_RELAYA ");
    }else if (strcmp((char*)data, PARAM_RELAYB) == 0) {
      digitalWrite(Relay2, !digitalRead(Relay2));
      Serial.println("command PARAM_RELAYB ");
    }
    else {
      Serial.println("command unknown ");
    }
  }
}

void MueveAPosInicial() {
  Serial.println("MueveAPosInicial ");
  stepper->setSpeedInUs(velocidadSecuencia-((15-velocidad)*90));
      stepper->setAcceleration(aceleracionSecuencia);
      stepper->moveTo(PosicionInicial);
}

void MueveAPosFinal() {
  Serial.println("MueveAPosFinal ");
  stepper->setSpeedInUs(velocidadSecuencia-((15-velocidad)*90));
      stepper->setAcceleration(aceleracionSecuencia);
      stepper->moveTo(PosicionFinal);
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var){
  Serial.println(var);
  if(var == "VEL"){
    return String(velocidad);
  }
  return String();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  delay(3000);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

    pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  pinMode(33, OUTPUT);
  digitalWrite(33, LOW);

  if(isDebug){
     // Connect to Wi-Fi
     
  WiFi.begin(ssid_debug, password_debug);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
    // Print ESP Local IP Address
  Serial.println(WiFi.localIP());
  }else{
    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  }

if (!EEPROM.begin(1000)) {
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
   velocidad=EEPROM.readInt(addressVelocity);
 Serial.print("La velocidad guardada es: ");
  Serial.println(velocidad);

  if(!SPIFFS.begin()){
        Serial.println("An Error has occurred while mounting SPIFFS");
        //return;
  }

  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/logo", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/logo.jpg", "image/png");
  });

  // Start server
  server.begin();

  Serial.println("Begin motor control");
  Serial.println();
  //Print function list for user selection
  Serial.println("Opciones por serial:");
  Serial.println("a. calibrar.");
  Serial.println("r. rutina.");
  Serial.println("<. mover CReloj.");
  Serial.println(">. mover Reloj.");
  Serial.println("+. mas rápido.");
  Serial.println("-. mas lento.");
  Serial.println("s. parar.");
  Serial.println("1. toggle relay 1.");
  Serial.println("2. toggle relay 2.");

  Serial.println();

  pinMode(SensorComienzo, INPUT);
  pinMode(SensorFinal, INPUT);

  pinMode(Relay1, OUTPUT);

  engine.init();
  stepper = engine.stepperConnectToPin(stepPinStepper);
  if (stepper) {
    stepper->setDirectionPin(dirPinStepper);
    stepper->setEnablePin(enablePinStepper);
    //stepper->setAutoEnable(true);
     stepper->enableOutputs();

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
  ws.cleanupClients();
   if (state == CalibrateStart)
  {
    if (digitalRead(SensorComienzo) == LOW)
    {
      //brazo esta en pos inicial, zerow, buscar pos final
      stepper->setSpeedInUs(velocidadCalibrar);  // the parameter is us/step !!!
      stepper->setAcceleration(aceleracionCalibrar);
      stepper->runBackward();
      state = LookingForZero;
      Serial.println("CalibrateStart LOW");
    }
    else
    {
      //buscar pos inicial
      stepper->setSpeedInUs(velocidadCalibrar);  // the parameter is us/step !!!
      stepper->setAcceleration(aceleracionCalibrar);
      stepper->runForward();
      
      state = BuscandoHome;
      Serial.println("CalibrateStart HIGH");
    }
  } else if (state == LookingForZero)
  {
    if (digitalRead(SensorComienzo) == HIGH)
    {
      Serial.print("encontro posicion 0 LookingForZeroHIGH: ");
      Serial.println(stepper->getCurrentPosition());
      stepper->setCurrentPosition(0);
      PosicionInicial = -300;
      Serial.print("esta posicion ahora es: ");
      Serial.println(stepper->getCurrentPosition());
      state = LookingForEnd;
    }
  }
   else if (state == LookingForEnd)
  {
    //Serial.println("LookingForEnd");
    if (digitalRead(SensorFinal) == LOW)
    {
      PosicionFinal=stepper->getCurrentPosition()+200;
      stepper->stopMove();
      state = Iddle;
    Serial.println("Done");
    Serial.print("Posicion final: ");
    Serial.println(PosicionFinal);
    notifyClients("calibrado");
    estaEnInicio=false;
    MueveAPosFinal();
    }
  }
  else if (state == BuscandoHome)
  {
    //Serial.println("BuscandoHome");
    if (digitalRead(SensorComienzo) == LOW)
    {
      stepper->stopMove();
      stepper->setSpeedInUs(velocidadCalibrar);
      stepper->setAcceleration(aceleracionCalibrar);
      stepper->runBackward();
      state = LookingForZero;
      Serial.println("BuscandoHome LOW");
    }
  }else if (state == Rutina)
  {
    if(!estaEnInicio){
    //Serial.print("diff: ");
    //Serial.println(abs((abs(PosicionInicial)-abs(stepper->getCurrentPosition() )))<10 );
        if(abs((abs(PosicionInicial)-abs(stepper->getCurrentPosition() )))<10 ){
          MueveAPosFinal();
          Serial.print("tardo: ");
          Serial.println( millis() - StartTime);
          state = Moviendo;
        }
      }else{
        //Serial.print("diff: ");
        //Serial.println(abs((abs(PosicionFinal)-abs(stepper->getCurrentPosition() )))<10 );
        if(abs((abs(PosicionFinal)-abs(stepper->getCurrentPosition() )))<10 ){
          MueveAPosInicial();
          Serial.print("tardo: ");
          Serial.println( millis() - StartTime);
          state = Moviendo;
        }
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
if (user_input == 's')
    {
      stepper->stopMove();
    }
    else if (user_input == '<')
    {
        state= Moviendo;
      MueveAPosFinal();
      Serial.println("command PARAM_AVANZA ");
   
    }
    else if (user_input == '>')
    {
        state= Moviendo;
      MueveAPosInicial();
      Serial.println("command PARAM_RETROCEDE ");
    }else if (user_input == '+')
    {
        
      Serial.println("mas rapido ");
   
    }
    else if (user_input == '->')
    {
        Serial.println("mas lento ");
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
     else if (user_input == 'r')
    {
      state= Rutina;
       MueveAPosFinal();
    }else
    {
      //Serial.println("Invalid option entered.");
    }
  }
}
