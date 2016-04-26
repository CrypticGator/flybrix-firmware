/*
    *  Flybrix Flight Controller -- Copyright 2015 Flying Selfie Inc.
    *
    *  License and other details available at: http://www.flybrix.com/firmware

    <bluetooth.h/cpp>

    Driver code for our UART Bluetooth module.

*/

#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <cstdint>
#include "cobs.h"

class Bluetooth {
   public:
    explicit Bluetooth(uint32_t baud_rate);

    bool read();
    void write(uint8_t* data, size_t length);
    bool isConnected() const;

    CobsReaderBuffer& buffer();

   private:
    enum ReadState { WAIT = 0, CR1 = 1, LF1 = 2, C = 3, O = 4, N = 5, CR2 = 6, DONE = 7 };
    void connect() const;

    char baud_case{0};
    uint8_t read_state{WAIT};
    CobsReaderBuffer data_input;
};

#endif