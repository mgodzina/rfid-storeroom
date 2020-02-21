#include <Wire.h> 
#include <RtcDS1307.h>
RtcDS1307<TwoWire> Rtc(Wire); //Real Time Clock
#include <SPI.h>
#include <MFRC522.h> //RFID Scanner
#include <SD.h>

const int chipSelect = 5; //SD Card Select
#define SS_PIN          10
#define RST_PIN         9
#define GRN_PIN         2 //LED
#define RED_PIN         3 //LED
#define BUZZER_PIN      4

MFRC522 mfrc522(SS_PIN, RST_PIN);  
MFRC522::StatusCode status; 
byte buffer[18];  //data transfer buffer (16+2 bytes data+CRC)
byte size = sizeof(buffer);
uint8_t pageAddr = 0x06; 

long lastscan = 0;
long lastsamecard = 0;
byte lastuid[10];
boolean newcard;
int n;

void setup() {
  Serial.begin(9600);
  SPI.begin(); 
  mfrc522.PCD_Init();
  Serial.println(F("Start programu!"));
  Rtc.Begin();
  pinMode(GRN_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(GRN_PIN, LOW);
  digitalWrite(RED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.print("Aktualny czas: ");
  timetoserial();
  Serial.println();
 //SD Card Inicialization
  Serial.println("Inicjalizacja karty SD...");

  // Check for card
  if (!SD.begin(chipSelect)) {
    Serial.println("Bład karty SD");
    flashred();
    digitalWrite(RED_PIN, HIGH);
    delay(2000);
    while (1);
  }
  Serial.println("Karta SD zainicjalizowana.");
  flashgreen();
  Serial.println(F("Zmiana czasu w formacie: tRRRR,MM,DD,HH,MM,SS"));
}

void loop() {
  if ( Serial.available() )
  {
    char c = toupper(Serial.read());
    if ( c == 'T'  ){      
      int rrrr = Serial.parseInt();
      int mr = Serial.parseInt();
      int dd = Serial.parseInt();
      int hh = Serial.parseInt();
      int mh = Serial.parseInt();
      int ss = Serial.parseInt();
      RtcDateTime ustaw = RtcDateTime(rrrr,mr,dd,hh,mh,ss);
      Rtc.SetDateTime(ustaw);
      RtcDateTime now = Rtc.GetDateTime();
      Serial.print("Nowy czas: ");
      timetoserial();
      Serial.println();
     }
  } 
  if ( ! mfrc522.PICC_IsNewCardPresent()) return;
  if ( ! mfrc522.PICC_ReadCardSerial()) return;

  if ((millis() - lastscan ) >= 400){ //time before next scan is ready
    newcard=0;
    for (n=0;n<7;n++) if (mfrc522.uid.uidByte[n]!=lastuid[n]) {newcard=1; break;}
    if (newcard || (millis() - lastsamecard ) >= 20000 ) {//czas pomiędzy skanami tego samego kodu
      //prawidlowy odczyt
      status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(pageAddr, buffer, &size);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        flashred();
        size= sizeof(buffer);
        mfrc522.PICC_HaltA();
        return;
      } else {
        RtcDateTime now = Rtc.GetDateTime();
        scantoserial(now);
        logtosd(now, buffer);
        mfrc522.PICC_HaltA();
        //sprzatanie po odczycie 
        memcpy(lastuid,mfrc522.uid.uidByte,7);
        lastsamecard = millis(); 
      }
    } else {
        Serial.println("Ta sama karta");
        flashwarning();
    }
        
    lastscan = millis();
    }
    

}


#define countof(a) (sizeof(a) / sizeof(a[0]))

void timetoserial() {
  RtcDateTime now = Rtc.GetDateTime();
  char datestring[20];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            now.Day(),
            now.Month(),
            now.Year(),
            now.Hour(),
            now.Minute(),
            now.Second() );
    Serial.print(datestring);
}

void scantoserial(const RtcDateTime& dt) {
    char datestring[20];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Day(),
            dt.Month(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
  Serial.print(F(" UID:"));
          for (byte i = 0; i < 7; i++) {
              Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
              Serial.print(mfrc522.uid.uidByte[i], HEX);
          } 
          Serial.println();
  
}

boolean logtosd(const RtcDateTime& dt, byte data[18]){
    char datestring[20];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%04u-%02u-%02u;%02u:%02u:%02u"),            
            dt.Year(),
            dt.Month(),
            dt.Day(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
  
  char nazwa[13];
  snprintf_P(nazwa, 13,
            PSTR("%04u%02u%02u.csv"),            
            dt.Year(),
            dt.Month(),
            dt.Day());
  
  if (!SD.begin(chipSelect)) {
            Serial.println("Bład karty SD");
            // don't do anything more:
            flashred();
            digitalWrite(RED_PIN, HIGH);
            delay(2000);
            while (1);
          }
  File dataFile = SD.open(nazwa, FILE_WRITE);
  if (dataFile) {
              dataFile.print(datestring);
                dataFile.print(";");
                for (byte i = 0; i < 7; i++) {
                if (mfrc522.uid.uidByte[i] < 16) {dataFile.print("0");}
                dataFile.print(mfrc522.uid.uidByte[i],HEX);
                }
                dataFile.print(";");
                for (byte i = 5; i < 16; i++) {
                  dataFile.write(data[i]);
                }
                dataFile.println();
    dataFile.close();
  } else {
    Serial.println("Bład otwarcia pliku");
    flashred();
    return 0;
  }

    Serial.print(F("Zapis SD, plik "));
    Serial.print(nazwa);
    Serial.print(F(" : "));
  //Dump a byte array to Serial
  Serial.print(datestring);
  Serial.print(";");
  for (byte i = 0; i < 7; i++) {
  if (mfrc522.uid.uidByte[i] < 16) {Serial.print("0");}
  Serial.print(mfrc522.uid.uidByte[i],HEX);
  }
  Serial.print(";");
  for (byte i = 5; i < 16; i++) {
    Serial.write(data[i]);
  }
  Serial.println();
  flashgreen();
  return 1;
}

void flashred() {
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(150);
  digitalWrite(BUZZER_PIN, LOW);
  delay(100);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(150);
  digitalWrite(BUZZER_PIN, LOW);
  delay(100);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(150);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RED_PIN, LOW);
  return;
}
void flashwarning() {
  digitalWrite(GRN_PIN, HIGH);
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(RED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  delay(100);
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(RED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  delay(100);
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(GRN_PIN, LOW);
  digitalWrite(RED_PIN, LOW);
  return;
}

void flashgreen() {
  digitalWrite(GRN_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
  delay(200);
  digitalWrite(GRN_PIN, LOW);
    return;
}
