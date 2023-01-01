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
int HomePosition = 0;

int velocidadSecuencia = 500;
int aceleracionSecuencia = 4000;

int velocidadCalibrar = 500;
int aceleracionCalibrar = 4000;


int state;
const int CalibrateStart = 1;
const int LookingForZero = 2;
const int LookingForEnd = 3;
const int BuscandoHome = 4;
const int Moviendo = 5;
const int RutinaHaciaHome = 6;
const int RutinaHaciaFinal = 7;

const int Calibrated = 10;
const int Iddle = -1;
int velocidad = 1;

String posicionActial = "sincalibrar";

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
const char* PARAM_RETROCEDE = "retrocede";
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
      <p><button id="buttonC" class="buttonC">CALIBRAR</button></p>
      <p><button style="display:none" id="buttonA" class="buttonA">AVANZAR</button></p>
      <p><button style="display:none" id="buttonR" class="buttonA">RETROCEDER</button></p>
      <p><button style="display:none" id="buttonU" class="buttonA">RUTINA</button></p>
      <p class="state" style="display:none" id="textoVel">Velocidad: <span  id="state">%VEL%</span></p>
      <p><button style="display:none" id="buttonVmas" class="buttonV">+</button></p>
      <p><button style="display:none" id="buttonVmenos" class="buttonV">-</button></p>
      <p><button style="display:none" id="buttonS1" class="buttonS">SWITCH1</button></p>
      <p><button style="display:none" id="buttonS2" class="buttonS">SWITCH2</button></p>
    </div>
  </div>
<script>
var PARAM_RELAYA = "relaya";
var PARAM_RELAYB = "relayb";

var PARAM_AVANZA = "avanza";
var PARAM_RETROCEDE = "retrocede";
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
    document.getElementById('buttonR').style.display = 'inline';
    document.getElementById('buttonU').style.display = 'inline';
    document.getElementById('buttonS1').style.display = 'inline';
    document.getElementById('buttonS2').style.display = 'inline';
    document.getElementById('buttonVmenos').style.display = 'inline';
    document.getElementById('buttonVmas').style.display = 'inline';
    document.getElementById('textoVel').style.display = 'inline';
     document.getElementById('textoCalibrando').style.display = 'none';
    }
    else{
      document.getElementById('state').innerHTML = event.data;
    }
    
  }
  
  function onLoad(event) {
    initButtonS();
    initWebSocket();
    
  }
  function initButtonS() {
    document.getElementById('buttonC').addEventListener('click', () => { buttonHandler(PARAM_CALIBRA) });
    document.getElementById('buttonA').addEventListener('click', () => { buttonHandler(PARAM_AVANZA) });
    document.getElementById('buttonR').addEventListener('click', () => { buttonHandler(PARAM_RETROCEDE) });
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
      MueveAPosFinal();
      Serial.println("command PARAM_AVANZA ");
    }else if (strcmp((char*)data, PARAM_RETROCEDE) == 0) {
      state= Moviendo;
      MueveAPosInicial();
      Serial.println("command PARAM_RETROCEDE ");
    }else if (strcmp((char*)data, PARAM_RUTINA) == 0) {
       //EmpiezaRutina();
       state = CalibrateStart;
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
      if(velocidad>10)
      velocidad=10;
      EEPROM.writeInt(addressVelocity, velocidad); 
      EEPROM.commit();
       delay(100);
      notifyClients(String(velocidad));
      Serial.println("command PARAM_VELMAS ");
    }else if (strcmp((char*)data, PARAM_VELMENOS) == 0) {    
      velocidad--;
      if(velocidad<1)
      velocidad=1;
       EEPROM.writeInt(addressVelocity, velocidad); 
       EEPROM.commit();
       delay(100);
      notifyClients(String(velocidad));
      Serial.println("command PARAM_VELMAS ");
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

void EmpiezaRutina() {

    if(posicionActial=="enfinal"){
      Serial.println(posicionActial);
      Serial.println("EmpiezaRutina ");
      MueveAPosInicial();
      state= RutinaHaciaHome;
       
    }else if(posicionActial=="eninicio"){
      Serial.println(posicionActial);
      Serial.println("EmpiezaRutina ");
      MueveAPosFinal();
      state= RutinaHaciaFinal;
      
    }
}

void MueveAPosInicial() {
  Serial.println("MueveAPosInicial ");
  posicionActial="eninicio";
  stepper->setSpeedInUs(velocidadSecuencia);
      stepper->setAcceleration(aceleracionSecuencia);
      stepper->moveTo(HomePosition);
}

void MueveAPosFinal() {
          Serial.println("MueveAPosFinal ");

  posicionActial="enfinal";
  stepper->setSpeedInUs(velocidadSecuencia);
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
   if(velocidad<1){
    EEPROM.writeInt(addressVelocity, 1); 
      EEPROM.commit();
      velocidad=EEPROM.readInt(addressVelocity);
   }
 Serial.print("La velocidad guardada es: ");
  Serial.println(velocidad);

  if(!SPIFFS.begin()){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
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
  Serial.println("<. movimiento adelante.");
  Serial.println(">. movimiento de regreso.");
  Serial.println("s. stop abrupto.");
  Serial.println("1. toggle relay 1.");
  Serial.println("2. toggle relay 2.");

  Serial.println();

  pinMode(SensorComienzo, INPUT);
  pinMode(SensorFinal, INPUT);

  pinMode(Relay1, OUTPUT);
    pinMode(Relay2, OUTPUT);

  engine.init();
  stepper = engine.stepperConnectToPin(stepPinStepper);
  if (stepper) {
    stepper->setDirectionPin(dirPinStepper);
    stepper->setEnablePin(enablePinStepper);
    //stepper->setAutoEnable(true);

    // If auto enable/disable need delays, just add (one or both):
     //stepper->setDelayToEnable(50);
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
      stepper->setSpeedInUs(velocidadCalibrar+(velocidad*50));  // the parameter is us/step !!!
      stepper->setAcceleration(aceleracionCalibrar);
      stepper->runBackward();
      state = LookingForZero;
      Serial.print("CalibrateStart LOW");
    }
    else
    {
      //buscar pos inicial
      stepper->setSpeedInUs(velocidadCalibrar+(velocidad*50));  // the parameter is us/step !!!
      stepper->setAcceleration(aceleracionCalibrar);
      stepper->runForward();
      
      state = BuscandoHome;
      Serial.print("CalibrateStart HIGH");
    }
  } else if (state == LookingForZero)
  {
    if (digitalRead(SensorComienzo) == HIGH)
    {
      //stepper->setCurrentPosition(0);
      HomePosition=stepper->getCurrentPosition();
      state = LookingForEnd;
      Serial.print("LookingForZero HIGH");
      Serial.print("home position:>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> ");
      Serial.println(HomePosition);
    }
  }
   else if (state == LookingForEnd)
  {
    Serial.println("LookingForEnd");
    if (digitalRead(SensorFinal) == LOW)
    {
          Serial.println("PosicionFinal guardada");
              Serial.println(PosicionFinal);


      PosicionFinal=stepper->getCurrentPosition();
      stepper->stopMove();
       Serial.print("PosicionFinal:>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> ");
      Serial.println(PosicionFinal);
      state = Iddle;
    posicionActial="enfinal";
    Serial.println("Done");
    notifyClients("calibrado");

    }
  }
  else if (state == BuscandoHome)
  {
          //Serial.println("BuscandoHome");

    if (digitalRead(SensorComienzo) == LOW)
    {
      stepper->stopMove();

      stepper->setSpeedInUs(velocidadCalibrar+(velocidad*50));  // the parameter is us/step !!!
      stepper->setAcceleration(aceleracionCalibrar);
      stepper->runBackward();
      state = LookingForZero;
      Serial.print("BuscandoHome LOW");

    }
  }else if (state == RutinaHaciaHome)
  {
    if(stepper->getCurrentPosition()>=HomePosition){
      MueveAPosFinal();
          Serial.println("1ra parte de RutinaHaciaHome completa");

      state = Moviendo;
    }
  }else if (state == RutinaHaciaFinal)
  {
    if( stepper->getCurrentPosition()<=PosicionFinal){
       MueveAPosInicial();
      Serial.println("1ra parte de RutinaHaciaFinal completa");

      state = Moviendo;
    }
  }

  if (stepper) {
    if (stepper->isRunning() && (state ==Moviendo)) {
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

      stepper->forceStop();
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
     EmpiezaRutina();
    }else
    {
      //Serial.println("Invalid option entered.");
    }
  }
}
