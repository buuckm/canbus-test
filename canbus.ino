/*
Code hochladen:
1. Board für Sender auswählen (Olimexd MOD-WIFI-ESP8266)
2. SENDER Konstante muss definiert sein (#define SENDER)
3. Code hochladen
4. "#define SENDER" auskommentieren/löschen
5. Board für Empfänger auswählen (Olimexd MOD-WIFI-ESP8266)
6. Code hochladen
*/

#include <mcp_can.h>
#include <SPI.h>

/*############################ Sender und Empfänger ############################*/

// Wenn SENDER nicht definiert ist, ignorieren wir den code zwischen "#ifdef SENDER" und "#else"
// Wenn SENDER definiert ist ignorieren wir den code zwischen "#else" und "#endif".
// Hab ich gemacht, damit alles in einer Datei sein kann.
//#define SENDER

#define TEMPERATURE_MESSAGE_ID 0x100
#define D1 5
MCP_CAN CAN(D1); // CS on D1

void initCAN(){
  // initialize CAN controller
  // MCP_ANY: accept all messages (no filtering)
  // CAN_500KBPS: baud rate (all nodes must use same baud rate!)
  // MCP_8MHZ crystal speed on CAN controller board
  while(CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK){
    Serial.println("failed to init CAN, retrying...");
    delay(100);
  }
  Serial.println("CAN init ok!");
  // operation mode
  // MCP_NORMAL: send/receive messages on bus (other options are MCP_LOOPBACK, MCP_LISTENONLY, MCP_SLEEP)
  CAN.setMode(MCP_NORMAL);
}

#ifdef SENDER

/*############################ Sender (Sensor) spezifisch ############################*/

#include <DHT.h>

#define DHT_PIN 4 // D2
#define DHT_TYPE DHT11
#define DHT_READ_INTERVAL_MS 2000 // time between sensor reads

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(9600);
  initCAN();
  dht.begin();
}

void loop() {
  // wir können den DHT11 nur alle 2000ms auslesen, sonst kommt nur müll an.
  delay(DHT_READ_INTERVAL_MS);
  
  // Versuche temperatur zu lesen. Bei Fehler abbrechen.
  float temperature = dht.readTemperature();
  if(isnan(temperature)){
    Serial.println("Failed to read from DHT11!");
    return;
  }
  
  // Wir senden eine Nachricht mit der Temperatur.
  // Die Adresse der "temperature" variable interpretieren wir als die Startadresse eines
  // Buffers an n Bytes, wo n die größe eines floats in Bytes ist. (sizeof(temperature) = 4)
  const int8_t isExtendedId = 0; // false
  if (CAN.sendMsgBuf(TEMPERATURE_MESSAGE_ID, isExtendedId, sizeof(temperature), (uint8_t*)&temperature) == CAN_OK) {
    Serial.println("Message sent");
  } else {
    Serial.println("Send failed");
  }
}

#else

/*############################ Empfänger (Buzzer) spezifisch ############################*/
#define BUZZER_PIN 4 // D2
#define TEMPERATURE_THRESHOLD 25 // °C

void setup() {
  Serial.begin(9600);
  initCAN();
  
  // Den Buzzer pin als output Pin konfigurieren.
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  // haben wir eine Nachricht erhalten?
  if (CAN_MSGAVAIL == CAN.checkReceive()) {
    // Die id der Nachricht
    long unsigned int messageId = 0;
    // Die länge der Nachricht in Bytes
    uint8_t messageLength = 0;
    // 64 Byte großer Buffer auf dem Stack; Die Nachricht wird hier reinkopiert
    uint8_t dataBuffer[64];
    // Nachricht lesen und Werte in gegebene Variablen schreiben
    CAN.readMsgBuf(&messageId, &messageLength, dataBuffer);

    // Uns interessieren nur Nachrichten, die Temperaturen enthalten,
    // also ignorieren wir alle anderen.
    if(messageId != TEMPERATURE_MESSAGE_ID) return;

    // Wir erwarten einen einzigen float Wert (4 bytes), also testen wir
    // ob die Nachricht 4 bytes lang ist.
    if(messageLength == sizeof(float)){
      // Wir interpretieren den Buffer an Bytes als einen float (durch einen Pointer).
      // Nötig um funktionen und operatoren zu verwenden, welche einen float erwarten.
      float* temperature = (float*)dataBuffer;
      Serial.print("ID: 0x");
      Serial.print(messageId, HEX);
      Serial.print(" Temperatur: ");
      // * heißt gib mir den Wert, der sich an dieser Adresse befindet.
      Serial.println(*temperature);
      Serial.println();

      // Buzzer (de)aktivieren bei bestimmter Temperatur
      if(*temperature > TEMPERATURE_THRESHOLD){
        tone(BUZZER_PIN, 2000);
      }else{
        noTone(BUZZER_PIN);
      }
    }else{
      // Unerwartete Nachricht ignorieren.
    }
  }
}

#endif


