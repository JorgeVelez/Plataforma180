#include "FastAccelStepper.h"

// As in StepperDemo for Motor 1 on AVR
//#define dirPinStepper    5
//#define enablePinStepper 6
//#define stepPinStepper   9  // OC1A in case of AVR

// As in StepperDemo for Motor 1 on ESP32
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

void setup() {

  Serial.begin(9600); //Open Serial connection for debugging
  Serial.println("Begin motor control");
  Serial.println();
  //Print function list for user selection
  Serial.println("Enter number for control option:");
  Serial.println("1. Turn at default microstep mode.");
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
    else
    {
      //Serial.println("Invalid option entered.");
    }
  }
}
