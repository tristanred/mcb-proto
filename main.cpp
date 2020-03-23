/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"

#include <PinNames.h>

#define WAIT_TIME 5000 //msec

#define EEPROM_MSG_LEN 10

#define READ 0x3
#define WRITE 0x2
#define WREN 0x6
#define WRDI 0x4
#define RDSR 0x5
#define WRSR 0x1


Serial pc(USBTX, USBRX);

SPI spi(SPI_MOSI, SPI_MISO, SPI_SCK);
DigitalOut cs(SPI_CS);

DigitalOut hold(D7);

void read_eeprom(uint8_t* buffer, int len);
void write_eeprom(const uint8_t* data, int len);
void output_data_serial(Serial* port, const uint8_t* data, int len);
void dump_status();
uint8_t read_status_reg();

int main()
{
    pc.printf("\n\n\n\n\n\n\n\n\n\n\n\n");
    pc.printf("Initializing board.\n");
    
    // Set CS to high +keep hold high at all times
    cs = 1;
    hold = 1;

    // 8 bit mode 0. 10Mhz freq
    spi.format(8, 0);
    spi.frequency(10000000);

    dump_status();

    // Set an array to a dummy 0xFA value and try to read over it
    pc.printf("Reading initial EEPROM data\n");
    uint8_t buf[EEPROM_MSG_LEN];
    memset(buf, 0xFA, EEPROM_MSG_LEN);

    // Read persisted data, hopefully overwriting the buffer;
    read_eeprom(buf, EEPROM_MSG_LEN);

    output_data_serial(&pc, buf, EEPROM_MSG_LEN);
    pc.printf("\n");

    uint8_t writebuf[EEPROM_MSG_LEN];
    // writebuf[0] = 0xEF;
    // writebuf[1] = 0xEF;
    // writebuf[2] = 0xEF;
    // writebuf[3] = 0xEF;
    // writebuf[4] = 0xEF;
    // writebuf[5] = 0xEF;
    // writebuf[6] = 0xEF;
    // writebuf[7] = 0xEF;
    // writebuf[8] = 0xEF;
    // writebuf[9] = 0xEF;
    memcpy(writebuf, "HELLOWORLD", EEPROM_MSG_LEN);

    pc.printf("Printing data : ");
    output_data_serial(&pc, writebuf, EEPROM_MSG_LEN);
    pc.printf("\n");

    write_eeprom(writebuf, EEPROM_MSG_LEN);
    
    memset(buf, 0xFA, EEPROM_MSG_LEN);
    read_eeprom(buf, EEPROM_MSG_LEN);

    pc.printf("Re-reading write result :");
    output_data_serial(&pc, buf, EEPROM_MSG_LEN);
    pc.printf("\n");

    dump_status();

    pc.printf("Starting program loop.\n");
    while (true)
    {
        // cs = 0;
        // spi.write(0x8F);
        // cs = 1;

        //pc.printf("Waiting for %ims\n", WAIT_TIME);
        thread_sleep_for(WAIT_TIME);
    }
}

void read_eeprom(uint8_t* buffer, int len)
{
    pc.printf("-----");
    pc.printf("Operation start : READ EEPROM\n");
    pc.printf(" Parameter 1 : Buffer ptr = %p\n", buffer);
    pc.printf(" Parameter 2 : Length = %i\n", len);
    
    pc.printf("Local values\n");
    pc.printf(" Read Address = %i\n", 0xFF);
    pc.printf(" Target addr HIGH = %i\n", 0);
    pc.printf(" Target addr LOW = %i\n", 0xFF & 0x00FF);
    pc.printf(" Opcode = %02X\n", READ);
    pc.printf(" Buffer content = ");
    output_data_serial(&pc, buffer, len);
    pc.printf("\n-----\n");

    cs = 0; // Select the EEPROM chip

    uint16_t base_addr = 0xFF;
    uint16_t eeprom_addr = base_addr;
    
    // Send initial read address.
    spi.write(READ); // Send READ command.
    spi.write(0); // Send the read address HIGH
    spi.write(eeprom_addr & 0x00FF); // Send the read address LOW
    
    pc.printf("Reading data from eeprom starting from: %i\n", eeprom_addr);
    for(int i = 0; i < len; i++)
    {
        // Issue blank writes to read the data
        int res = spi.write(0x0);
        pc.printf("Reading byte %02X \n", res);

        buffer[i] = res;
    }
    pc.printf("\n---\n");

    cs = 1; // Deselect the chip.
}

void write_eeprom(const uint8_t* data, int len)
{
    pc.printf("-----");
    pc.printf("Operation start : WRITE EEPROM\n");
    pc.printf(" Parameter 1 : Buffer ptr = %p\n", data);
    pc.printf(" Parameter 2 : Length = %i\n", len);
    
    pc.printf("Local values\n");
    pc.printf(" Read Address = %i\n", 0xFF);
    pc.printf(" Target addr HIGH = %i\n", 0);
    pc.printf(" Target addr LOW = %i\n", 0x00FF & 0x00FF);
    pc.printf(" Opcode 1 = %02X\n", WREN);
    pc.printf(" Opcode 2 = %02X\n", WRITE);
    pc.printf(" Buffer content = ");
    output_data_serial(&pc, data, len);
    pc.printf("\n-----\n");

    uint16_t base_addr = 0xFF;

    pc.printf("Writing data from eeprom starting from: %i\n", base_addr);

    cs = 0; // Select the EEPROM chip
    spi.write(WREN); // Set WREN latch
    cs = 1; // Release CS


    cs = 0; // Reselect the chip

    spi.write(WRITE); // Send the WRITE command.
    spi.write(0); // Send the write address HIGH
    spi.write(base_addr & 0x00FF); // Send the write address LOW

    for(int i = 0; i < len; i++)
    {
        pc.printf("Writing byte %02X \n", data[i]);
        int res = spi.write(data[i]);
        pc.printf("Reading byte %02X \n", res);
    }
    //pc.printf("\n---\n");
    cs = 1;

    pc.printf("Starting to wait for write op.\n");
    thread_sleep_for(3000);
    // while((read_status_reg() & 0x1) == 1)
    // {
    //     pc.printf("Waiting\n");
    //     thread_sleep_for(1);
    // }
    pc.printf("End of write op.\n");
}

void output_data_serial(Serial* port, const uint8_t* data, int len)
{
    for(int i = 0; i < len; i++)
    {
        port->printf("%02X ", data[i]);
    }
}

uint8_t read_status_reg()
{
    cs = 1;
    spi.write(RDSR);
    
    uint8_t res = spi.write(0x0);

    cs = 0;

    return res;
}

void dump_status()
{
    uint8_t status = read_status_reg();
    pc.printf("STATUS : [%02X]\n", status);
}