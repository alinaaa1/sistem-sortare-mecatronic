#include <Servo.h>
#include <TM1637Display.h>

const int pinStep = 9;
const int pinDir = 8;
Servo servo1, servo2, servo3;

// === CONFIGURARE PINI PENTRU CELE 3 ECRANE ===
#define CLK_ROSU 12
#define DIO_ROSU 13
#define CLK_VERDE A0
#define DIO_VERDE A1
#define CLK_ALBASTRU A2
#define DIO_ALBASTRU A3

TM1637Display displayRosu(CLK_ROSU, DIO_ROSU);
TM1637Display displayVerde(CLK_VERDE, DIO_VERDE);
TM1637Display displayAlbastru(CLK_ALBASTRU, DIO_ALBASTRU);

// Contoare piese
int pieseRosii = 0;
int pieseVerzi = 0;
int pieseAlbastre = 0;

// PINII PENTRU SENZORUL TCS3200
const int S0 = 2;
const int S1 = 4;
const int S2 = 7;
const int S3 = 10;
const int sensorOut = 11;

unsigned long counter = 0;


unsigned long asteptareServo1 = 300; 
unsigned long asteptareServo2 = 300; 
unsigned long asteptareServo3 = 850; 

boolean piesaPeDrum = false;
unsigned long momentDeschidere = 0;
int bratDeActionat = 0;

unsigned long blocareSenzorPanaLa = 0; 

const unsigned long durataStandard = 2200;  
const unsigned long durataAlbastru = 4500;  

unsigned long durataCurenta = 0; 

// MATRICE PENTRU ECRANELE ROTITE LA 180 GRADE
const uint8_t cifreInversate[] = {
  0b00111111, // 0
  0b00110000, // 1
  0b01011011, // 2
  0b01111001, // 3
  0b01110100, // 4
  0b01101101, // 5
  0b01101111, // 6
  0b00111000, // 7
  0b01111111, // 8
  0b01111101  // 9
};

//  FUNCTIA CARE AFIȘEAZĂ  ORDINEA ȘI SEGMENTELE
void afiseazaInversatMacheta(TM1637Display &disp, int numar) {
  disp.clear();
  
  if (numar < 10) {
    uint8_t date[] = { cifreInversate[numar] };
    disp.setSegments(date, 1, 0); 
  } 
  else if (numar >= 10 && numar < 100) {
    int unitati = numar % 10;
    int zeci = numar / 10;
    
    uint8_t dateZeci[] = { cifreInversate[zeci] };
    uint8_t dateUnitati[] = { cifreInversate[unitati] };
    
    disp.setSegments(dateZeci, 1, 1);    
    disp.setSegments(dateUnitati, 1, 0); 
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("--- SYSTEM ONLINE: SENSIBILITATE VERDE SPORITĂ ---");

  displayRosu.setBrightness(7);
  displayVerde.setBrightness(7);
  displayAlbastru.setBrightness(7);
  
  displayRosu.clear();
  displayVerde.clear();
  displayAlbastru.clear();

  afiseazaInversatMacheta(displayRosu, pieseRosii);       
  displayVerde.showNumberDec(pieseVerzi);                 
  afiseazaInversatMacheta(displayAlbastru, pieseAlbastre); 

  pinMode(pinStep, OUTPUT);
  pinMode(pinDir, OUTPUT);
  digitalWrite(pinDir, LOW);

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);
  
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  servo1.attach(3); 
  servo2.attach(5); 
  servo3.attach(6); 

  servo1.write(170); 
  servo2.write(15);  
  servo3.write(170); 
  delay(1000);

  for (int i = 0; i < 100; i++) {
    int v = map(i, 0, 99, 500, 300);
    digitalWrite(pinStep, HIGH);
    delayMicroseconds(10);
    digitalWrite(pinStep, LOW);
    delayMicroseconds(v);
  }
}

void loop() {
  digitalWrite(pinStep, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinStep, LOW);
  delayMicroseconds(300); 

  counter++;
  unsigned long timpCurent = millis();

  if (!piesaPeDrum && timpCurent > blocareSenzorPanaLa && counter % 2000 == 0) {
    digitalWrite(S2, LOW);  digitalWrite(S3, LOW);
    int rosu = pulseIn(sensorOut, LOW, 4000); 

    digitalWrite(S2, LOW);  digitalWrite(S3, HIGH);
    int albastru = pulseIn(sensorOut, LOW, 4000);

    digitalWrite(S2, HIGH); digitalWrite(S3, HIGH);
    int verde = pulseIn(sensorOut, LOW, 4000);

    
    if (rosu < 140 || albastru < 140 || verde < 140) {
      
      // A) DETECTAT VERDE
      if (verde < albastru && verde < rosu) {
        bratDeActionat = 2; 
        momentDeschidere = timpCurent + asteptareServo2; 
        durataCurenta = durataStandard; 
        piesaPeDrum = true;
        blocareSenzorPanaLa = timpCurent + 1500;
        Serial.println(">> DETECTAT VERDE");
      }
      // B) DETECTAT ROȘU
      else if (rosu < albastru && rosu < verde) {
        bratDeActionat = 1; 
        momentDeschidere = timpCurent + asteptareServo1; 
        durataCurenta = durataStandard; 
        piesaPeDrum = true;
        blocareSenzorPanaLa = timpCurent + 1500;
        Serial.println(">> DETECTAT ROȘU");
      }
      // C) DETECTAT ALBASTRU
      else if (albastru < rosu && albastru < verde) {
        bratDeActionat = 3; 
        momentDeschidere = timpCurent + asteptareServo3; 
        durataCurenta = durataAlbastru; 
        piesaPeDrum = true;
        blocareSenzorPanaLa = timpCurent + 1500;
        Serial.println(">> DETECTAT ALBASTRU");
      }
    }
  }

  if (piesaPeDrum) {
    if (timpCurent >= momentDeschidere && timpCurent < (momentDeschidere + durataCurenta)) {
      if (bratDeActionat == 1) servo1.write(80); 
      if (bratDeActionat == 2) servo2.write(95); 
      if (bratDeActionat == 3) servo3.write(80); 
    } 
    else if (timpCurent >= (momentDeschidere + durataCurenta)) {
      
      servo1.write(170); 
      servo2.write(15);  
      servo3.write(170); 

      if (bratDeActionat == 1) {
        pieseRosii++;
        afiseazaInversatMacheta(displayRosu, pieseRosii); 
      }
      else if (bratDeActionat == 2) {
        pieseVerzi++;
        displayVerde.showNumberDec(pieseVerzi);    
      }
      else if (bratDeActionat == 3) {
        pieseAlbastre++;
        afiseazaInversatMacheta(displayAlbastru, pieseAlbastre); 
      }
      
      piesaPeDrum = false;
      bratDeActionat = 0;
      durataCurenta = 0;
    }
  }
}