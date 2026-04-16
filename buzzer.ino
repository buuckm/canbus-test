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

// Definition von Konstanten.
#define TEMPERATURE_MESSAGE_ID 0x100
#define TEMPERATURE_THRESHOLD 25 // °C
#define D1 5
#define D2 4
#define BUZZER_PIN D2
// muss der in der Arduino IDE festgelegten Baudrate entsprechen.
#define BAUD_RATE 9600

// MCP_CAN objekt erstellen.
// Der erste parameter legt fest, welcher pin als CS pin fuer die SPI Verbindung zum CAN-Controller verwendet wird.
// Wir verbinden D1 des ESP8266 mit CS des CAN-Controllers.
MCP_CAN CAN(D1);

void setup(){
    // Seriellen Monitor initialisieren.
    Serial.begin(BAUD_RATE);
    Serial.println("[BUZZER] begin setup");
    
    /*
    Wir versuchen so lange den CAN-Controller zu initialisieren, bis der Rückgabewert CAN_OK ist.
    Der erste parameter (MCP_*) filtert Frames anhand ihres typen und der laenge der id. Es gibt folgende varianten:
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
    Der dritte parameter (MCP_*HZ) muss der Frequenz der Clock des CAN-Controllers entsprechen.
    Diese ist auf einem Großen silbernen Bauteil eingraviert. Jede verbundene Node muss die gleiche Frequenz haben.
    */
    while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK){
        Serial.println("[BUZZER] failed to init CAN. retrying...");
        delay(100);
    }

    /*  
    Konfigurieren, in welchem modus der CAN-Controller operiert. Moegliche Werte:
        - MCP_NORMAL: Frames Empfangen und Senden
        - MCP_SLEEP: Standby modus
        - MCP_LOOPBACK: Frames werden nicht nach aussen gesendet und selbst empfangen
        - MCP_LISTENONLY: Frames werden empfangen aber nicht gesendet
    */
    CAN.setMode(MCP_NORMAL);
    Serial.println("CAN init done!");
    
    // Hier konfigurieren wir den Signal-Pin fuer den Buzzer als Output.
    pinMode(BUZZER_PIN, OUTPUT);
    Serial.println("[BUZZER] setup done");
}

void loop(){
    // Hier testen wir, ob wir Daten erhalten haben (Polling).
    // Das ist nur noetig, wenn der Interrupt-Pin des CAN-Controllers nicht verwendet wird.
    if (CAN_MSGAVAIL == CAN.checkReceive()){
        // Hier lesen wir die empfangene Nachricht in einen von uns erstellten Buffer.
        // Eine Nachricht ist maximal MAX_CHAR_IN_MESSAGE (8) bytes lang, deshalb verwenden
        // wir das als die groesse des Buffers. Die tatsaechliche Laenge der Nachricht
        // und dessen ID wird von CAN.readMsgBuf() in die messageLength und messageId
        // variablen geschrieben, weshalb wir nicht dessen wert, sondern dessen Adressen
        // der Funktion als Parameter geben. "&messageId" = "Die Speicheradresse der Variable messageId".
        long unsigned int messageId;
        uint8_t messageLength;
        uint8_t dataBuffer[MAX_CHAR_IN_MESSAGE];
        CAN.readMsgBuf(&messageId, &messageLength, dataBuffer);
        Serial.println("-------------------------");
        Serial.printf("[BUZZER] got message with id %d and length %d\n", messageId, messageLength);
        
        // Wir ignorieren alle Nachrichten, welche nicht die von uns Festgelegte ID fuer Nachrichten mit Temperaturdaten haben.
        if (messageId != TEMPERATURE_MESSAGE_ID) return;
        
        // Wenn die laenge der Nachricht der groesse eines floats (4 Byte) entspricht,
        // dann interpretieren wir sie als einen float.
        if (messageLength == sizeof(float)){
            // Wir definieren eine Pointer-Variable, welche die adresse des floats speichert.
            // Sie wird mit der adresse des Nachrichtenbuffers initialisiert.
            // Das macht es möglich den Wert an dieser Adresse als einen float zu interpretieren.
            // Um den Wert aus der Adresse zu lesen, muss diese mit einem "*" dereferenziert werden.
            float* temperature = (float*)dataBuffer;
            Serial.printf("[BUZZER] temperature: %f\n", *temperature);
            if (*temperature > TEMPERATURE_THRESHOLD){
                // Wir erzeugen einen Dauerhaften ton mit einer Frequenz von 2kHz.
                tone(BUZZER_PIN, 2000);
            }else{
                // Buzzer deaktivieren
                noTone(BUZZER_PIN);
            }
        }
    }
}
