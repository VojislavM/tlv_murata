#include "frame.h"
#include "message.h"
#include "STM32L0.h"

#define HORIZON_SERIAL Serial1

parser_t parser;
tlv_gps_location_t loc = {
            .itow = 123,
            .lon = 0,
            .lat = 0,
            .h_msl = 296,
            .h_acc = 1,
            .v_acc = 1,
        };

boolean horizon_gps_flag = false;
static void horizon_message_callback(const message_t *message);
void horizon_serial_callback();

void setup() {
    Serial.begin(115200);
    Serial.println("boot()");
    // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);

    HORIZON_SERIAL.begin(115200);
    HORIZON_SERIAL.setWakeup(1);
    HORIZON_SERIAL.onReceive(horizon_serial_callback);
    frame_parser_init(&parser);
    parser.handler = horizon_message_callback;
}

// the loop function runs over and over again forever
void loop() {
    STM32L0.stop();
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
