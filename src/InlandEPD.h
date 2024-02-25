#ifndef INLAND_EPD
#define INLAND_EPD

#include <vector>

#include "Arduino.h"
#include "SPI.h"

#include "epd2in13.h"

#define GATE_DRIVING_VOLTAGE_CONTROL 0x03
#define SOURCE_DRIVING_VOLTAGE_CONTROL 0x04

using byteVector = std::vector<byte>;

class InlandEPD {

    public:
        InlandEPD(int cs, int dc, int rst, int busy) {
            this->cs = cs;
            this->dc = dc;
            this->rst = rst;
            this->busy = busy;
            this->buffer = byteVector(EPD_WIDTH * EPD_HEIGHT / 8);
        }
        ~InlandEPD() {}

        void init();
        void wipe();

        void update_full();
        void update_partial();

        byteVector buffer;

    private:
        int cs, dc, rst, busy;

        int text_row = 1;

        void init_spi();
        void init_buffer();

        void hard_reset();
        void soft_reset();

        void command(byte command);
        void command(byte command, byteVector data);
        void data(byteVector data);

        void wait_until_idle();
        void deep_sleep();

        void update_common();
};

#endif
