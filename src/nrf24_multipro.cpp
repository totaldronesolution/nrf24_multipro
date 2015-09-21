/*
 * multiRemote.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <Arduino.h>
#include <EEPROM.h>
#include "nrf24_multipro.h"

/**
 * global transmitterID
 * @todo use function
 */
uint8_t transmitterID[4];

/**
 * called on startup
 */
void nrf24_multipro::begin(void) {

    randomSeed((analogRead(A4) & 0x1F) | (analogRead(A5) << 5));

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW); //start LED off
    pinMode(CS_pin, OUTPUT);
    pinMode(CE_pin, OUTPUT);

#ifdef SOFTSPI
    pinMode(MOSI_pin, OUTPUT);
    pinMode(SCK_pin, OUTPUT);
    pinMode(MISO_pin, INPUT);
#else
    SPI.begin();
#endif

    DEBUG_MULTI("[MR] set_txid...\n");

    set_txid(false);

    channel_num = CH_MAX_CONTROL;
    current_protocol = (t_protocols) constrain(EEPROM.read(ee_PROTOCOL_ID), 0, PROTO_END - 1);

    DEBUG_MULTI("[MR] current_protocol: ");
    DEBUG_MULTI(current_protocol);
    DEBUG_MULTI("\n");

    reset();
    DEBUG_MULTI("[MR] begin done.\n");

}

/**
 * reset the Protocol + RF
 */
void nrf24_multipro::reset(void) {

    /**
     * reset ppm values
     */
    for(uint8_t ch = 0; ch < CH_MAX_CONTROL; ch++) {
        ppm[ch] = PPM_MID;
    }

    ppm[CH_THROTTLE] = PPM_MIN;

    initRF();
    initProt();
}

/**
 * called from arduino loop
 */
void nrf24_multipro::loop(void) {
    static unsigned long next;
    if(micros() >= next) {
        protAPI * protocol = protocols[current_protocol];
        next = protocol->loop();
    }
}

/**
 *
 * @return t_protocols
 */
t_protocols nrf24_multipro::getProtocol(void) {
    return current_protocol;
}

/**
 * switch protocol
 * @param protocol t_protocols
 */
void nrf24_multipro::setProtocol(t_protocols protocol) {

    if(current_protocol == protocol) {
        return;
    }

    DEBUG_MULTI("[MR] setProtocol to: ");
    DEBUG_MULTI(current_protocol);
    DEBUG_MULTI("\n");
    current_protocol = protocol;

    // update eeprom
    EEPROM.update(ee_PROTOCOL_ID, current_protocol);

    reset();
}

/**
 * get channel num
 * @return
 */
uint8_t nrf24_multipro::getChannelNum(void) {
    return channel_num;
}

/**
 * sets the channel num
 * @param num uint8_t
 */
void nrf24_multipro::setChannelNum(uint8_t num) {
    channel_num = num;

    if(channel_num >= CH_MAX_CONTROL) {
        channel_num = (CH_MAX_CONTROL - 1);
    }
}

/**
 *
 * @param ch t_channelOrder
 * @return channel value
 */
uint16_t nrf24_multipro::getChannel(t_channelOrder ch) {
    if(ch < CH_MAX_CONTROL) {
        return ppm[ch];
    }
    return 0;
}

/**
 *
 * @param ch t_channelOrder
 * @return true if channel value is > PPM_MAX_COMMAND
 */
bool nrf24_multipro::getChannelIsCMD(t_channelOrder ch) {
    if(ch < CH_MAX_CONTROL) {
        return (ppm[ch] > PPM_MAX_COMMAND);
    }
    return false;
}

/**
 *
 * @param ch t_channelOrder
 * @param value uint16_t
 */
void nrf24_multipro::setChannel(t_channelOrder ch, uint16_t value) {
    if(ch < CH_MAX_CONTROL) {
        ppm[ch] = value;
    }
}

/**
 *
 */
void nrf24_multipro::initRF(void) {

    DEBUG_MULTI("[MR] initRF...\n");
    NRF24L01_Reset();
    NRF24L01_Initialize();
    DEBUG_MULTI("[MR] initRF... Done.\n");
}

/**
 *
 */
void nrf24_multipro::initProt(void) {

    DEBUG_MULTI("[MR] initProt...\n");
    protAPI * protocol = protocols[current_protocol];
    protocol->init();
    protocol->bind();
    DEBUG_MULTI("[MR] initProt... Done.\n");

}

/**
 *
 * @param renew bool
 */
void nrf24_multipro::set_txid(bool renew) {
    uint8_t i;
    for(i = 0; i < 4; i++)
        transmitterID[i] = EEPROM.read(ee_TXID0 + i);
    if(renew || (transmitterID[0] == 0xFF && transmitterID[1] == 0x0FF)) {
        for(i = 0; i < 4; i++) {
            transmitterID[i] = random() & 0xFF;
            EEPROM.update(ee_TXID0 + i, transmitterID[i]);
        }
    }
}

/**
 * global class access
 */
nrf24_multipro multipro;