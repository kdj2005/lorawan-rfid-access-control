/*******************************************************************************
 * Code LoRaWAN ABP - Envoi d'UID RFID (MFRC522)
 *******************************************************************************/

#define CFG_EU 1

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <MFRC522.h>

// Broches du module RFID MFRC522
#define SS_PIN 7
#define RST_PIN 9

MFRC522 rfid(SS_PIN, RST_PIN);

// Variables pour la gestion du temps (anti-rebond / cooldown)
const unsigned long cooldown = 10000;
unsigned long lastReadTime = 0;

// Tableau global pour stocker l'UID à envoyer (Max 10 octets pour les badges longs)
uint8_t rfid_payload[10];
uint8_t rfid_size = 0;
bool card_waiting_to_send = false;

// LoRaWAN Configuration (ABP)
static const u4_t DEVADDR = votre cle;
static const PROGMEM u1_t NWKSKEY[16] = {votre cle};
static const u1_t PROGMEM APPSKEY[16] = {votre cle};

void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;

// Broches LoRa (Inchangées)
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 8,
    .dio = {6, 6, 6},
};

void onEvent (ev_t ev) {
    switch(ev) {
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (Envoi réussi)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            break;
         default:
            break;
    }
}

// Fonction d'envoi LoRaWAN
void do_send(osjob_t* j){
    // Vérifie si un envoi n'est pas déjà en cours
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND (Envoi déjà en cours, attente...)"));
    } 
    else if (card_waiting_to_send) {      
        // Envoi des données de l'UID de la carte
        LMIC_setTxData2(1, rfid_payload, rfid_size, 0);
        Serial.println(F("Paquet LoRaWAN envoyé avec l'UID."));
        
        // Réinitialise le drapeau d'attente
        card_waiting_to_send = false;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println(F("Initialisation..."));
    
    // Initialisation SPI et RFID
    SPI.begin();
    rfid.PCD_Init();
    Serial.println(F("RFID Pret ! Approche une carte..."));

    // Initialisation du protocole LMIC (LoRa)
    os_init();
    LMIC_reset();
    LMIC_setClockError(MAX_CLOCK_ERROR * 2 / 100);

    #ifdef PROGMEM
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
    #else
    LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    #if defined(CFG_EU)
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      
    #endif

    LMIC_setLinkCheckMode(0);
    LMIC.dn2Dr = DR_SF9;
    LMIC_setDrTxpow(DR_SF7, 14);
}

void loop() {
    // Fait tourner le moteur LoRaWAN en arrière-plan
    os_runloop_once();

    // Gestion de la lecture RFID (seulement si aucun envoi n'attend et hors cooldown)
    if (!card_waiting_to_send && (millis() - lastReadTime >= cooldown)) {
        
        // Détection d'une nouvelle carte
        if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
            
            rfid_size = rfid.uid.size;
            Serial.print(F("Carte détectée ! UID : "));
            
            // On remplit le payload avec les octets bruts de l'UID
            for (byte i = 0; i < rfid_size; i++) {
                rfid_payload[i] = rfid.uid.uidByte[i];
                
                // Affichage sur le moniteur série en Hexadécimal
                if (rfid_payload[i] < 0x10) Serial.print("0");
                Serial.print(rfid_payload[i], HEX);
            }
            Serial.println();

            // Active le signal pour le prochain envoi LoRaWAN
            card_waiting_to_send = true;
            lastReadTime = millis();
            
            // Stoppe la lecture de la carte actuelle
            rfid.PICC_HaltA();

            // Déclenche immédiatement l'envoi LoRa
            do_send(&sendjob);
        }
    }
}
