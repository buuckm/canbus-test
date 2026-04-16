/*
Code hochladen:
1. Board für Sender auswählen: LOLIN(WEMOS) D1 R2 & mini
2. Korrekten Port auswaehlen: /dev/ttyUSB0, /dev/ttyUSB1, ... oder aehnlich
3. Upload Knopf in der IDE druecken um den Code auf den ESP zu flashen.
*/

// Funktions- und Klassendeklarationen der Bibliotheken inkludieren.
// Header und Source Dateien von mcp_can: https://github.com/coryjfowler/MCP_CAN_lib
#include <mcp_can.h>
// Header und Source Dateien von SPI: https://github.com/arduino/ArduinoCore-avr/tree/master/libraries/SPI
#include <SPI.h>
// Header und Source Dateien von DHT: https://github.com/adafruit/DHT-sensor-library
#include <DHT.h>

// Definition von Konstanten.
#define D1 5
#define D2 4
#define DHT_PIN D2
#define DHT_TYPE DHT11
#define BAUDE_RATE 9600
#define DHT_READ_INTERVAL_MS 2000
#define TEMPERATURE_MESSAGE_ID 0x100

// MCP_CAN objekt erstellen.
// Der erste parameter legt fest, welcher pin als CS pin
// fuer die SPI Verbindung zum CAN-Controllers verwendet wird.
// Wir verbinden D1 des ESP8266 mit CS des CAN-Controllers.
MCP_CAN CAN(D1);

// DHT objekt erstellen.
// Der erste Parameter legt den Signal-Pin fest.
// Der zweite Parameter legt die Variante des DHT-Sensors fest (DHT11, DHT22, ...).
DHT dht(DHT_PIN, DHT_TYPE);

void setup(){
    // Seriellen Monitor initialisieren.
    Serial.begin(BAUDE_RATE);

    /*
    Serial.println("[BUZZER] receiver setup begin");  
    Wir versuchen so lange den CAN-Controller zu initialisieren,
    bis der Rückgabewert CAN_OK ist.
    Der erste parameter (MCP_*) filtert Frames anhand ihres typen
    und der laenge der id. Es gibt folgende varianten:
        - MCP_STDEXT: Frames mit Standard und Erweiterten IDs (11bit und 29bit)
        - MCP_STD: Nur Frames mit Standard IDs
        - MCP_EXT: Nur Frames mit Erweiterten IDs
        - MCP_ANY: Jede Art von ID und jede Art von Frame, inklusive Error/Overload Frames
    Der zweite parameter (CAN_*BPS) legt die Uebertragungsrate fest. Jede verbundene Node
    muss mit der gleichen Uebertragungsrate arbeiten. Mögliche varianten:
        - CAN_4K096BPS
        - CAN_5KBPS
        - CAN_10KBPS
        - CAN_20KBPS
        - CAN_31K25BPS
        - CAN_33K3BPS
        - CAN_40KBPS
        - CAN_50KBPS
        - CAN_80KBPS
        - CAN_100KBPS
        - CAN_125KBPS
        - CAN_200KBPS
        - CAN_250KBPS
        - CAN_500KBPS
        - CAN_1000KBPS
    Der dritte parameter (MCP_*HZ) muss der Frequenz der Clock des CAN-Controllers
    entsprechen. Diese ist auf einem Großen silbernen Bauteil eingraviert.
    Jede verbundene Node muss die gleiche Frequenz haben.
    */
    while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK){
        Serial.println("[SENSOR] failed to init CAN. retrying...");
        delay(100);
    }
    /*  
    Konfiguriert, in welchem modus der CAN-Controller operiert.
        - MCP_NORMAL: Frames Empfangen und Senden
        - MCP_SLEEP: Standby modus
        - MCP_LOOPBACK: Frames werden nicht nach aussen gesendet und selbst empfangen
        - MCP_LISTENONLY: Frames werden empfangen aber nicht gesendet
    */
    CAN.setMode(MCP_NORMAL);
    Serial.println("[SENSOR] CAN init done!");
        
    // Beginne mit dem Lesen der Sensordaten.
    dht.begin();
    Serial.println("[SENSOR] setup done");
}

void loop(){
    // Wir warten mindestens 2 Sekunden, weil das der minimale Intervall ist,
    // mit dem der DHT11 Sensor gelesen werden kann.
    delay(DHT_READ_INTERVAL_MS);
    
    // Wir lesen die Temperatur und ignorieren invalide Werte (NaN = Not a Number)
    float temperature = dht.readTemperature();
    if (isnan(temperature)){
        Serial.println("[SENSOR] invalid temperature!");
        return;
    }
    Serial.printf("[SENSOR] temperature: %f\n", temperature);
    
    // Hier senden wir eine Nachricht, welche die Temperatur enthaelt.
    // Der erste Parameter ist die ID, die der Frame haben soll.
    // Der zweite Parameter legt fest, ob wir eine Erweiterte ID verwenden (29 anstatt 11 Bits).
    // Der dritte Parameter ist die Groesse des Nachrichtenbuffers. In diesem Fall nehmen
    // wir einfach die Groesse der temperature Variable.
    // Der dritte Parameter ist die Adresse des Nachrichtenbuffers. Wir nehmen einfach die
    // Adresse der temperature Variable, und reinterpretieren diese als die Adresse eines
    // Buffers an 1-Byte unsigned integers (einzelne Bytes).
    uint8_t isExtendedId = 0;
    uint8_t messageBufferLength = sizeof(temperature);
    uint8_t* messageBuffer = (uint8_t*)&temperature;
    CAN.sendMsgBuf(TEMPERATURE_MESSAGE_ID, isExtendedId, messageBufferLength, messageBuffer);
}

