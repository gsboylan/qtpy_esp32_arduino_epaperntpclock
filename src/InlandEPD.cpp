#include "InlandEPD.h"

byteVector PART_UPDATE_LUT_TTGO_DKE = {
    0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00
};

void InlandEPD::init() {
    this->init_spi();
    this->init_buffer();

    this->hard_reset();
    this->soft_reset();
}

void InlandEPD::init_spi() {
    SPI.begin();
    SPI.beginTransaction(SPISettings(20*1000*1000, MSBFIRST, SPI_MODE0));

    pinMode(cs, OUTPUT);
    pinMode(dc, OUTPUT);
    pinMode(rst, OUTPUT);
    pinMode(busy, INPUT);

    digitalWrite(cs, HIGH);
}

void InlandEPD::wipe() {
    this->init_buffer();
    this->update_full();
}

void InlandEPD::init_buffer() {
    std::fill(buffer.begin(), buffer.end(), 0xff);
}

void InlandEPD::hard_reset() {
    digitalWrite(rst, HIGH);
    delay(10);
    digitalWrite(rst, LOW);
    delay(10);
    digitalWrite(rst, HIGH);
}

void InlandEPD::soft_reset() {
    this->command(SW_RESET);
    this->wait_until_idle();
}

void InlandEPD::command(byte command) {
    this->command(command, {});
}

void InlandEPD::command(byte command, byteVector data) {
    digitalWrite(cs, 1);
    digitalWrite(dc, 0);
    digitalWrite(cs, 0);
    SPI.write(command);
    digitalWrite(cs, 1);

    if (!data.empty()) {
        this->data(data);
    }
}

void InlandEPD::data(byteVector data) {
    digitalWrite(cs, 1);
    digitalWrite(dc, 1);
    digitalWrite(cs, 0);
    SPI.writeBytes(data.data(), data.size());
    digitalWrite(cs, 1);
}

void InlandEPD::wait_until_idle() {
    while (digitalRead(busy) == 1) {
        delay(10);
    }
}

void InlandEPD::deep_sleep() {
    this->command(DEEP_SLEEP_MODE);
}

void InlandEPD::update_common() {
    this->hard_reset();
    this->soft_reset();

    this->command(DATA_ENTRY_MODE_SETTING, {0x03});
    this->command(SET_RAM_X_ADDRESS_START_END_POSITION, {0x00, 0x0F});
    this->command(SET_RAM_Y_ADDRESS_START_END_POSITION, {0x00, 0x00, 0xF9, 0x00});
    this->command(SET_RAM_X_ADDRESS_COUNTER, {0x00});
    this->command(SET_RAM_Y_ADDRESS_COUNTER, {0x00, 0x00});
}

void InlandEPD::update_full() {
    this->update_common();

    this->command(WRITE_RAM, buffer);
    this->command(MASTER_ACTIVATION);
    this->wait_until_idle();

    this->deep_sleep();
}

void InlandEPD::update_partial(bool twice) {
    this->update_common();

    this->command(WRITE_LUT_REGISTER, PART_UPDATE_LUT_TTGO_DKE);

    // this->command(SET_GATE_TIME, {0x22});

    this->command(GATE_DRIVING_VOLTAGE_CONTROL, {0x17});
    this->command(GATE_DRIVING_VOLTAGE_CONTROL, {0x41, 0x00, 0x32});
    this->command(WRITE_VCOM_REGISTER, {0x00});
    this->wait_until_idle();

    this->command(BORDER_WAVEFORM_CONTROL, {0x01});
    this->wait_until_idle();

    this->command(DISPLAY_UPDATE_CONTROL_2, {0xB9});
    this->command(MASTER_ACTIVATION);
    this->wait_until_idle();

    this->command(WRITE_RAM, buffer);
    this->command(DISPLAY_UPDATE_CONTROL_2, {0xCF});
    this->command(MASTER_ACTIVATION);
    this->wait_until_idle();

    if (twice) {
        this->command(WRITE_RAM, buffer);
        this->command(DISPLAY_UPDATE_CONTROL_2, {0xCF});
        this->command(MASTER_ACTIVATION);
        this->wait_until_idle();
    }

    this->deep_sleep();
}
