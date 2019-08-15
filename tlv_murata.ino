#include "frame.h"
#include "message.h"

int32_t lon = 42;
int32_t lat = 16;
int togl_led = 0;

parser_t parser;
tlv_gps_location_t loc = {
            .itow = 123,
            .lon = 0,
            .lat = 0,
            .h_msl = 296,
            .h_acc = 1,
            .v_acc = 1,
        };

static void my_incoming_message_handler(const message_t *message);

void setup() {
    Serial.begin(115200);
    // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);

    frame_parser_init(&parser);
    parser.handler = my_incoming_message_handler;
}

// the loop function runs over and over again forever
void loop() {
    // digitalWrite(LED_BUILTIN, togl_led);   // turn the LED on (HIGH is the voltage level)
    // delay(1000);                       // wait for a second 
    // togl_led = !togl_led;
    
    // message_t msg;
    // message_init(&msg);
    // tlv_gps_location_t loc = {
    //     .itow = 123,
    //     .lon = lon,
    //     .lat = lat,
    //     .h_msl = 296,
    //     .h_acc = 1,
    //     .v_acc = 1,
    // };
    // message_tlv_add_gps_location(&msg, &loc);

    // // Frame message.
    // uint8_t frame[64];
    // ssize_t frame_size = frame_message(frame, sizeof(frame), &msg);
    // if (frame_size < 0) {
    //     Serial.println("Failed to frame message!");
    //     return;
    // }

    // //Serial.println("Sending framed message:\n");
    // //Serial.write(frame, frame_size);
    //  for (size_t j = 0; j < frame_size; j++) {
    //      Serial.print(frame[j], HEX);
    //  }
    // Serial.println();

    // // Increment lat/long.
    // lat = (lat + 1) % 90;
    // lon = (lon + 1) % 90;              
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte;
    char inChar = (char)Serial.read();
    uint8_t uiChar = (uint8_t)inChar;
    // When you receive some bytes from the serial port you call.
    frame_parser_push_buffer(&parser, &uiChar, 1);
  }
}

// callback will be called when a message is received.
static void my_incoming_message_handler(const message_t *message) {
    message_print(message);
    if(message_tlv_get_gps_location(message, &loc) == MESSAGE_SUCCESS){
        Serial.print("ok lon: ");
        Serial.print((int)loc.lon);
        Serial.print(", ");
        Serial.print("lat: ");
        Serial.println((int)loc.lat);
    }else{
        Serial.println("not ok");
    }
}
