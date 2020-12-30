#include <ESP8266WiFi.h>

#include <SoftwareSerial.h>
SoftwareSerial s(D6,D5);
#include<NTPClient.h>
#include<WiFiUdp.h>
#include<Adafruit_MQTT.h>
#include<Adafruit_MQTT_Client.h>

#include<Wire.h>
#include<LiquidCrystal_PCF8574.h>
#include<Servo.h>

#define MQTT_SERVER  "io.adafruit.com"
#define MQTT_PORT    1883
#define IO_USERNAME  "kelompok7_ITERA"
#define IO_KEY       "aio_cXGr35ZzaIcnvBST3nAZRKn0vHjH"
#define WIFI_SSID "Redmi" //Nama SSID yang terhubung ke internet
#define WIFI_PASSWORD "udong123" // Password WiFI
#define SERVO2 D3

Servo myservo2;
int pos=0;

//inisialisasi NTPClient
WiFiUDP ntpUDP;
//timeClient(wifiUDP, url_ntp, zona_waktu, interval_update)
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", 25200, 60000);

LiquidCrystal_PCF8574 lcd(0x27); //buat objek lcd

int slot1=1, slot2=1, countCar=0, last_countCar=0; //menampung nilai dari arduino
bool last_slot1=true, last_slot2=true; 
int timeSlot1, timeSlot2;
String biayaSlot1="0", biayaSlot2="0";
int hh, mm; //hour dan minute
bool status_exitGate=false;

//inisialisasi server adafruit
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT, IO_USERNAME, IO_KEY);

//button subscribe virtual adafruit
Adafruit_MQTT_Subscribe ExitGate = Adafruit_MQTT_Subscribe(&mqtt, IO_USERNAME "/feeds/exitgate");

//Set up the feed you're publishing to
Adafruit_MQTT_Publish CountCar = Adafruit_MQTT_Publish(&mqtt,IO_USERNAME "/feeds/countcar");
Adafruit_MQTT_Publish Slot1 = Adafruit_MQTT_Publish(&mqtt,IO_USERNAME "/feeds/slot1");
Adafruit_MQTT_Publish Slot2 = Adafruit_MQTT_Publish(&mqtt,IO_USERNAME "/feeds/slot2");

void setup() {
  Serial.begin(9600); 
  s.begin(1000); //inisialisasi baud rate serial komunikasi
  mqtt.subscribe(&ExitGate);
  //sambungkan nodemcu dengan wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
  Serial.print("Connecting to Wi-Fi"); 
  
  //loading koneksi dengan ..
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  
  //sambungkan timeClient dengan NTP server
  timeClient.begin();

  //Koneksi LCD
  Wire.begin();
  Wire.beginTransmission(0x27);
  int errorLCD = Wire.endTransmission();
  Serial.print("Error: ");
  Serial.println(errorLCD);
  if(errorLCD == 0){
    Serial.println(": LCD found");
  }else{
    Serial.println(": LCD not found");
  }
  lcd.begin(16, 2);
  myservo2.attach(SERVO2);
  while (!Serial) continue;
}

void loop() {
  //decode data dari arduino
  String data = s.readString();
  slot1 = getValue(data, ',', 0).toInt();
  slot2 = getValue(data, ',', 1).toInt();
  countCar = getValue(data, ',', 2).toInt();

  //hubungkan nodemcu dengan MQTT server adafruit
  MQTT_connect();

  //cek total kendaraan di parkiran
  if(countCar != last_countCar){
    CountCar.publish(countCar);
    last_countCar = countCar;
  }
  
  //update waktu saat ini
  timeClient.update();
  hh = timeClient.getHours();
  mm = timeClient.getMinutes();
  Serial.println("Waktu : " + String(hh) + ":" + String(mm));

  //cek slot parkir 1
  if(slot1 != last_slot1){
    Slot1.publish(slot1);
    if(!slot1){
      //waktu saat masuk slot parkir
      timeSlot1 = (hh*60) + mm; 
      biayaSlot1 = "0";
    }else{
      //waktu saat keluar slot parkir
      timeSlot1 = ((hh*60) + mm) - timeSlot1;
      //penentuan biaya parkir
      biayaSlot1 = kalkulasiHargaParkir(timeSlot1);
    }
    last_slot1 = slot1; //update nilai last_slot dengan status saat ini
  }

  //cek slot parkir 2
  if(slot2 != last_slot2){
    Slot2.publish(slot2);
    if(!slot2){
      //waktu saat masuk slot parkir
      timeSlot2 = (hh*60) + mm; 
      biayaSlot2 = "0";
    }else{
      //waktu saat keluar slot parkir
      timeSlot2 = ((hh*60) + mm) - timeSlot2;
      //penentuan biaya parkir
      biayaSlot2 = kalkulasiHargaParkir(timeSlot2);
    }
    last_slot2 = slot2; //update nilai last_slot dengan status saat ini
  }

  //Menampilkan harga pada LCD
  lcd.setBacklight(255);
  lcd.home();
  lcd.clear();
  lcd.print("Slot 1: Rp"+ biayaSlot1);
  lcd.setCursor(0,1);
  lcd.print("Slot 2: Rp"+ biayaSlot2);

  Adafruit_MQTT_Subscribe * subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
  // Check if its ExitGate feed
  if (subscription == &ExitGate) {
    Serial.print(F("Gerbang keluar: "));
    Serial.println((char *)ExitGate.lastread);
    
    if (!strcmp((char *)ExitGate.lastread, "BUKA")) {
      status_exitGate = true; //buka gerbang
    }
    if (!strcmp((char *)ExitGate.lastread, "TUTUP")) {
      status_exitGate = false; //tutup gerbang
    }
    gerakanServo2(status_exitGate);
  }
}
  delay(1000);
}

void MQTT_connect() {
  // Stop jika sudah terkoneksi
  if (mqtt.connected()){
    return;
  }
  mqtt.disconnect();
  mqtt.connect();
}

String kalkulasiHargaParkir(int timeSlot){
  if(timeSlot > 180) return "6000";
  else if(timeSlot > 120) return "4000";
  else return "2000";
}

void gerakanServo2(bool buka){
  if(buka){
      for(pos = 80; pos >= 0; pos-= 1){ 
        // step setiap 1 derajat
        myservo2.write(pos); 
        delay(15); // menunggu 15 milidetik
      }
    }else{
      for(pos = 0; pos<=80; pos+=1){
        myservo2.write(pos); 
        delay(15); // menunggu 15 milidetik
      }
    }
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;
 
    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
