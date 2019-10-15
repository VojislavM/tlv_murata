#include "frame.h"
#include "message.h"
#include "STM32L0.h"
#include "LoRaWAN.h"
#include <LibLacuna.h>

#define HORIZON_SERIAL Serial1

parser_t parser;
tlv_gps_location_t loc = {
            .itow = 123,
            .lon = 0,
            .lat = 0,
            .h_msl = 296,
            .h_acc = 1,
            .v_acc = 1,
            .timestamp = 0;
        };

boolean horizon_gps_flag = false;
static void horizon_message_callback(const message_t *message);
void horizon_serial_callback();


/**
 * @brief LoraWAN gps packet setup - port 1
 * 
 */
struct horizonData_t{
  uint8_t lat1;
  uint8_t lat2;
  uint8_t lat3;
  uint8_t lon1;
  uint8_t lon2;
  uint8_t lon3;
}__attribute__((packed));

union horizonPacket_t{
  horizonData_t data;
  byte bytes[sizeof(horizonData_t)];
};

static const uint8_t horizon_packet_port = 10;
horizonPacket_t horizon_packet;

#define LED_RED PA0
// Keys and device address are MSB
static byte networkKey[] = { 0x78, 0xAC, 0xDE, 0x45, 0xA5, 0x4F, 0xAB, 0xD7, 0x1E, 0x85, 0xB8, 0x82, 0xED, 0x5D, 0x96, 0x67 };
static byte appKey[] = { 0x79, 0x3B, 0x12, 0x24, 0x50, 0xFC, 0x19, 0x5C, 0x76, 0xE8, 0x3D, 0x94, 0xEB, 0x05, 0x9B, 0x94 };
//TODO: Replace with your device address

static lsLoraWANParams loraWANParams;

static byte payload[] = "Lacuna";

lsLoraETxParams etxParams;
lsLoraTxParams txParams;

const char *devAddr = "26011F78"; // test-1
static byte deviceAddress[] = { 0x26, 0x01, 0x1D, 0x79 };
//const char *devAddr = "26011190"; // test-2
//static byte deviceAddress[] = { 0x26, 0x01, 0x11, 0x90 };
//const char *devAddr = "260119B2"; // test-3
//static byte deviceAddress[] = { 0x26, 0x01, 0x19, 0xB2 };

const char *nwkSKey = "78ACDE45A54FABD71E85B882ED5D9667";
const char *appSKey = "793B122450FC195C76E83D94EB059B94";

void setup() {
    Serial.begin(115200);
    Serial.println("boot()");
    // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_RED, OUTPUT);

    HORIZON_SERIAL.begin(115200);
    HORIZON_SERIAL.setWakeup(1);
    HORIZON_SERIAL.onReceive(horizon_serial_callback);
    frame_parser_init(&parser);
    parser.handler = horizon_message_callback;

    lsCreateDefaultLoraWANParams(&loraWANParams, networkKey, appKey, deviceAddress);

    lsSX126xConfig cfg;
    lsCreateDefaultSX126xConfig(&cfg);

    cfg.nssPin = PB12;
    cfg.resetPin = PA8;
    cfg.antennaSwitchPin = PB6;
    cfg.busyPin = PA5;                // pin 2 for Lacuna shield, pin 3 for Semtch SX126x eval boards
    cfg.osc = lsSX126xOscTCXO;      // for Xtal: lsSX126xOscXtal
    cfg.type = lsSX126xTypeSX1262;  // for SX1261: lsSX126xTypeSX1261

    int result = lsInitSX126x(&cfg);
    if(result) Serial.println(lsErrorToString(result));

    lsCreateDefaultLoraETxParams(&etxParams);

    etxParams.frequency = 866600000;
    etxParams.grid = lsLoraEGrid_3_9_khz;
    etxParams.codingRate = lsLoraECodingRate_1_3;
    etxParams.bandwidth = lsLoraEBandwidth_187_5_khz;
    etxParams.nbSync = 4;
    etxParams.power = 21;
    etxParams.hopping = 1;

    lsCreateDefaultLoraTxParams(&txParams);

    txParams.power = 21;
    txParams.spreadingFactor = lsLoraSpreadingFactor_11;
    txParams.codingRate = lsLoraCodingRate_4_5;
    txParams.invertIq = false;
    txParams.frequency = 868300000;
    txParams.bandwidth = lsLoraBandwidth_125_khz;
    txParams.syncWord = LS_LORA_SYNCWORD_PUBLIC;
    txParams.preambleLength = 8;
  
    LoRaWAN.begin(EU868);
    LoRaWAN.addChannel(1, 868300000, 0, 6);
    LoRaWAN.addChannel(3, 867100000, 0, 5);
    LoRaWAN.addChannel(4, 867300000, 0, 5);
    LoRaWAN.addChannel(5, 867500000, 0, 5);
    LoRaWAN.addChannel(6, 867700000, 0, 5);
    LoRaWAN.addChannel(7, 867900000, 0, 5);
    LoRaWAN.addChannel(8, 868800000, 7, 7);
    LoRaWAN.setRX2Channel(869525000, 3);

    LoRaWAN.setDutyCycle(false);     
    // LoRaWAN.setAntennaGain(2.0);
    LoRaWAN.setTxPower(20);
    LoRaWAN.joinABP(devAddr, nwkSKey, appSKey);

    Serial.println("JOIN( )");
    lora_tx();
}

// the loop function runs over and over again forever
void loop() {
    STM32L0.stop(500);
    //delay(500);
    if(horizon_gps_flag){
        horizon_gps_flag=false;
        Serial.print("ok lon: ");
        Serial.print((int)loc.lon);
        Serial.print(", ");
        Serial.print("lat: ");
        Serial.print((int)loc.lat);
        Serial.print(", ");
        Serial.print("itow: ");
        Serial.println((int)loc.itow);

        // 3 of 4 bytes of the variable are populated with data
        float lat = (float)loc.lat/10000000;
        float lon = (float)loc.lon/10000000;
        uint32_t lat_packed = (uint32_t)(((lat + 90) / 180.0) * 16777215);
        uint32_t lon_packed = (uint32_t)(((lon + 180) / 360.0) * 16777215);
        horizon_packet.data.lat1 = lat_packed >> 16;
        horizon_packet.data.lat2 = lat_packed >> 8;
        horizon_packet.data.lat3 = lat_packed;
        horizon_packet.data.lon1 = lon_packed >> 16;
        horizon_packet.data.lon2 = lon_packed >> 8;
        horizon_packet.data.lon3 = lon_packed;

        lora_tx();
    }          
}

/**
 * @brief callback on serial message received from the horizon device
 * 
 */
void horizon_serial_callback() {
    uint8_t rx_data[FRAME_MAX_LENGTH];
    int rx_count;
    do{
        rx_count = Serial1.read(&rx_data[0], sizeof(rx_data));
        if (rx_count > 0){         
            frame_parser_push_buffer(&parser, &rx_data[0], rx_count);
        }
    }
    while (rx_count > 0);
}

/**
 * @brief callback from the message decoder on successfulr eception of a TLV
 * 
 * @param message 
 */
static void horizon_message_callback(const message_t *message) {
    if(message_tlv_get_gps_location(message, &loc) == MESSAGE_SUCCESS){
        horizon_gps_flag=true;
        STM32L0.wakeup();
    }
}


void lora_tx( void )
{
    if (LoRaWAN.joined() && !LoRaWAN.busy())
    {
      Serial.println("Sending TTN message");
        Serial.print("TRANSMIT( ");
        Serial.print("TimeOnAir: ");
        Serial.print(LoRaWAN.getTimeOnAir());
        Serial.print(", NextTxTime: ");
        Serial.print(LoRaWAN.getNextTxTime());
        Serial.print(", MaxPayloadSize: ");
        Serial.print(LoRaWAN.getMaxPayloadSize());
        Serial.print(", DR: ");
        Serial.print(LoRaWAN.getDataRate());
        Serial.print(", TxPower: ");
        Serial.print(LoRaWAN.getTxPower(), 1);
        Serial.print("dbm, UpLinkCounter: ");
        Serial.print(LoRaWAN.getUpLinkCounter());
        Serial.print(", DownLinkCounter: ");
        Serial.print(LoRaWAN.getDownLinkCounter());
        Serial.println(" )");

        LoRaWAN.sendPacket(horizon_packet_port, &horizon_packet.bytes[0], sizeof(horizonData_t),false);
    }


  Serial.println("Sending Lora message");
  int result = lsSendLoraWAN(&loraWANParams, &txParams, (byte *)payload, sizeof payload-1);
  Serial.println(lsErrorToString(result));
  loraWANParams.framecounter++;
  delay(1000);
  
  /*Serial.println("Sending Lora-E message");
  int result = lsSendLoraEWAN(&loraWANParams, &etxParams, (byte *)horizon_packet.bytes[0], sizeof(horizonData_t)-1);
  Serial.println(lsErrorToString(result));
  loraWANParams.framecounter++; */

  digitalWrite(LED_RED, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);                       // wait for a second
  digitalWrite(LED_RED, LOW);    // turn the LED off by making the voltage LOW 

}