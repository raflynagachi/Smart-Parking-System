#include <SoftwareSerial.h>
SoftwareSerial s(5,6);
#include<Servo.h>

//Definisi pin yang digunakan
#define LDR 13
#define TRIG 12
#define ECHO 11
#define BUZZ 10
#define SERVO1 9
#define SLOT1 4
#define SLOT2 3
#define LED 2

int intensity; //menyimpan nilai baca LDR
unsigned long duration; //menyimpan nilai durasi (ultrasonik)
unsigned int distance; //menyimpan nilai jarak (ultrasonik)

Servo myservo1;
int pos = 0;
bool statusGate=false, last_statusGate=false;

int count=0, slot1=0, slot2=0;

void setup() {
  Serial.begin(9600);
  s.begin(1000);
  pinMode(LDR, INPUT);
  pinMode(BUZZ, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(SLOT1, INPUT);
  pinMode(SLOT2, INPUT);
  myservo1.attach(SERVO1);
  while (!Serial) continue;
}

void loop() {
  //baca LDR dan LED
  intensity = digitalRead(LDR);
  //Menylakan LED
  if(intensity){
    digitalWrite(LED, HIGH);
  }else{
    digitalWrite(LED, LOW);
  }

  //menggunakan gelombang ultrasonik
  digitalWrite(TRIG, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(5);
  digitalWrite(TRIG, LOW);
  //baca jarak sensor ultrasonik
  duration = pulseIn(ECHO, HIGH);
  distance = duration/58.2;

  //status gerbang (buka/tutup)
  if(distance < 5){
      statusGate = true; //buka gerbang
  }else{
      statusGate = false; //tutup gerbang
  }
  //Gerakkan servo gerbang masuk
  if(statusGate != last_statusGate){
    digitalWrite(BUZZ, HIGH);
    gerakanServo(statusGate);
    digitalWrite(BUZZ, LOW);
    if(statusGate) count++;
    //simpan lastStatus dengan statusGate saat ini
    last_statusGate = statusGate;
  }

  //baca slot parkir
  if(digitalRead(SLOT1)==HIGH){
    slot1=1;
  }else slot1=0;
  if(digitalRead(SLOT2)==HIGH){
    slot2=1;
  }else slot2=0;

  //kirim data ke nodemcu
  String data = String(slot1)+','+String(slot2)+','+String(count);
  s.println(data);
  delay(1000);
}

void gerakanServo(bool tutup){
  if(!tutup){
      for(pos = 0; pos <= 80; pos+= 1){ 
        // step setiap 1 derajat
        myservo1.write(pos); 
        delay(15); // menunggu 15 milidetik
      }
    }else{
      for(pos = 80; pos>=0; pos-=1){
        myservo1.write(pos); 
        delay(15); // menunggu 15 milidetik
      }
    }
}
