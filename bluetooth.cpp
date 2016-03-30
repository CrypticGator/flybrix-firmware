/*
    *  Flybrix Flight Controller -- Copyright 2015 Flying Selfie Inc.
    *
    *  License and other details available at: http://www.flybrix.com/firmware

    <bluetooth.h/cpp>

    Driver code for our UART Bluetooth module.

*/

#include "bluetooth.h"

#include <Arduino.h>

namespace {
constexpr int EXPECTED_CHAR[] = {13, 10, 'C', 'O', 'N', 13, 10, 0};
}

Bluetooth::Bluetooth(uint32_t baud_rate) {
    switch (baud_rate) {
        case 9600:
            baud_case = '1';
            break;
        case 19200:
            baud_case = '2';
            break;
        case 38400:
            baud_case = '3';
            break;
        case 57600:
            baud_case = '4';
            break;
        case 115200:
            baud_case = '5';
            break;
        case 230400:
            baud_case = '6';
            break;
        case 460800:
            baud_case = '7';
            break;
        case 921600:
            baud_case = '8';
            break;
    }
    if (!baud_case) {
        baud_rate = 9600;
        baud_case = '1';
    }
    Serial1.begin(baud_rate);
    connect();
}

void Bluetooth::update() {
    while (!isConnected() && Serial1.available()) {
        int c{Serial1.read()};
        if (c == EXPECTED_CHAR[read_state])
            read_state = read_state + 1;
        else if (read_state != CR2)
            read_state = WAIT;
    }
}

bool Bluetooth::isConnected() const {
    return read_state == DONE;
}

void Bluetooth::connect() const {
    if (isConnected())
        return;
    Serial1.printf("BST500000%c%c%c", baud_case, 13, 10);
    Serial1.printf("BCD3%c%c", 13, 10);
}
