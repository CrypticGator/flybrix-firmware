/*
    *  Flybrix Flight Controller -- Copyright 2015 Flying Selfie Inc.
    *
    *  License and other details available at: http://www.flybrix.com/firmware

    <serialFork.h/cpp>

    Interface for forking serial communication channels.
*/

#include "serialFork.h"
#include <Arduino.h>
#include "bluetooth.h"

namespace {
struct USBComm {
    USBComm() {
        Serial.begin(9600);  // USB is always 12 Mbit/sec
    }

    bool read() {
        while (Serial.available()) {
            data_input.AppendToBuffer(Serial.read());
            if (data_input.IsDone())
                return true;
        }
        return false;
    }

    void write(uint8_t* data, size_t length) {
        Serial.write(data, length);
    }

    CobsReaderBuffer& buffer() {
        return data_input;
    }

   private:
    CobsReaderBuffer data_input;
};

USBComm usb_comm;
Bluetooth bluetooth{115200};
}

CobsReaderBuffer* readSerial() {
    if (usb_comm.read())
        return &usb_comm.buffer();
    if (bluetooth.read())
        return &bluetooth.buffer();
    return nullptr;
}

void writeSerial(uint8_t* data, size_t length) {
    usb_comm.write(data, length);
    bluetooth.write(data, length);
}
