/**
 * @file vera.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-10-21
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "Arduino.h"
#include "SPI.h"
#include "vera-spi.h"

/*This sketch shows the basics of using the W25Q16 library.*/

/*W25Q16 Connections:
 pin 1 to Arduino pin 10 or Atmega328 pin 15 (Chip Select)
 pin 2 to Arduino pin 12 or Atmega328 pin 18 (Master In Slave Out)
 pin 3 to 3.3V (Vdd)
 pin 4 to GND (Vss)
 pin 5 to Arduino pin 11 or Atmega328 pin 17 (Slave in Master Out)
 pin 6 to Arduino pin 13 or Atmega328 pin 19 (Clock)
 pin 7 to 3.3V (Vdd)
 pin 8 to 3.3V (Vdd)
 */

unsigned int _FLASH_SS = 10;
#define WRITE_ENABLE 0x06
#define WRITE_DISABLE 0x04
#define PAGE_PROGRAM 0x02
#define READ_STATUS_REGISTER_1 0x05
#define READ_DATA 0x03
#define CHIP_ERASE 0xC7
#define POWER_DOWN 0xB9
#define RELEASE_POWER_DOWN 0xAB
#define MANUFACTURER_ID 0x90

void vera_spi_not_busy()
{
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(READ_STATUS_REGISTER_1);
    while (bitRead(SPI.transfer(0), 0) & 1)
    {
    }
    digitalWrite(_FLASH_SS, HIGH);
}

/*****************************************************************************
 *
 *	Function name : vera_spi_power_down
 *
 *	Returns :		None
 *
 *	Parameters :	None
 *
 *	Purpose :	Puts the flash in its low power mode.
 *
 *
 ******************************************************************************/
void vera_spi_power_down()
{
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(POWER_DOWN);
    digitalWrite(_FLASH_SS, HIGH);
    vera_spi_not_busy();
}

/*****************************************************************************
 *
 *	Function name : powerDown
 *
 *	Returns :		None
 *
 *	Parameters :	None
 *
 *	Purpose :	Releases the flash from its low power mode.  Flash cannot be in
 *				low power mode to perform read/write operations.
 *
 *
 ******************************************************************************/
void vera_spi_release_power_down()
{
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(RELEASE_POWER_DOWN);
    digitalWrite(_FLASH_SS, HIGH);
    vera_spi_not_busy();
}

void vera_spi_write_enable()
{
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(WRITE_ENABLE);
    digitalWrite(_FLASH_SS, HIGH);
    vera_spi_not_busy();
}

void vera_spi_write_disable()
{
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(WRITE_DISABLE);
    digitalWrite(_FLASH_SS, HIGH);
    vera_spi_not_busy();
}

/*****************************************************************************
 *
 *	Function name : vera_spi_init
 *
 *	Returns :		None
 *
 *	Parameters :	int FLASS_SS	->	The Slave Select or Chip Select pin used by
 *										Arduino to select W25Q16.
 *
 *
 *	Purpose :	Initializes the W25Q16 by setting the input slave select pin
 *				as OUTPUT and writing it HIGH.  Also initializes the SPI bus,
 *				sets the SPI bit order to MSBFIRST and the SPI data mode to
 *				SPI_MODE3, ensures the flash is not in low power mode, and
 *				that flash write is disabled.
 *
 *
 ******************************************************************************/
void vera_spi_init(int FLASH_SS)
{
    pinMode(FLASH_SS, OUTPUT);
    digitalWrite(FLASH_SS, HIGH);
    _FLASH_SS = FLASH_SS;
    SPI.begin();
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE3);
    vera_spi_release_power_down();
    vera_spi_write_disable();
}

/*****************************************************************************
 *
 *	Function name : read
 *
 *	Returns :		byte
 *
 *	Parameters :	unsigned int page ->  The page to read from.
 *					byte pageAddress -> the page address from which a byte
                                                will be read
 *
 *
 *	Purpose :	Reads a byte from the flash page and page address.  The W25Q16 has
 *				8192 pages with 256 bytes in a page.  Both page and byte addresses
 *				start at 0. Page ends at address 8191 and page address ends at 255.
 *
 *
 ******************************************************************************/
byte vera_spi_read(unsigned int page, byte pageAddress)
{
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(READ_DATA);
    SPI.transfer((page >> 8) & 0xFF);
    SPI.transfer((page >> 0) & 0xFF);
    SPI.transfer(pageAddress);
    byte val = SPI.transfer(0);
    digitalWrite(_FLASH_SS, HIGH);
    vera_spi_not_busy();
    return val;
}

/*****************************************************************************
 *
 *	Function name : write
 *
 *	Returns :		None
 *
 *	Parameters :	unsigned int page ->  The page to write to.
 *					byte pageAddress -> The page address to which a byte
 *												will be written.
 * 					byte val -> the byte to write to the page and page address.
 *
 *	Purpose :	Writes a byte to the flash page and page address.  The W25Q16 has
 *				8192 pages with 256 bytes in a page.  Both page and byte addresses
 *				start at 0. Page ends at address 8191 and page address ends at 255.
 *
 *
 ******************************************************************************/
void vera_spi_write(unsigned int page, byte pageAddress, byte val)
{
    vera_spi_write_enable();
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(PAGE_PROGRAM);
    SPI.transfer((page >> 8) & 0xFF);
    SPI.transfer((page >> 0) & 0xFF);
    SPI.transfer(pageAddress);
    SPI.transfer(val);
    digitalWrite(_FLASH_SS, HIGH);
    vera_spi_not_busy();
    vera_spi_write_disable();
}

/*****************************************************************************
 *
 *	Function name : vera_spi_stream_open
 *
 *	Returns :		None
 *
 *	Parameters :	unsigned int page ->  The page to begin writing.
 *					byte pageAddress -> The page address to to begin
 *												writing.
 *
 *	Purpose :	Initializes flash for stream write, e.g. write more than one byte
 *				consecutively.  Both page and byte addresses start at 0. Page
 *				ends at address 8191 and page address ends at 255.
 *
 ******************************************************************************/
void vera_spi_stream_open(unsigned int page, byte pageAddress)
{
    vera_spi_write_enable();
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(PAGE_PROGRAM);
    SPI.transfer((page >> 8) & 0xFF);
    SPI.transfer((page >> 0) & 0xFF);
    SPI.transfer(pageAddress);
}

/*****************************************************************************
 *
 *	Function name : vera_spi_stream_write
 *
 *	Returns :		None
 *
 *	Parameters :	byte val -> the byte to write to the flash.
 *
 *	Purpose :	Writes a byte to the W25Q16.  Must be first called after
 *				vera_spi_stream_open and then consecutively to write multiple bytes.
 *
 ******************************************************************************/
void vera_spi_stream_write(byte val)
{
    SPI.transfer(val);
}

/*****************************************************************************
 *
 *	Function name : vera_spi_stream_close
 *
 *	Returns :		None
 *
 *	Parameters :	None
 *
 *	Purpose :	Close the stream write. Must be called after the last call to
 *				vera_spi_stream_write.
 *
 ******************************************************************************/
void vera_spi_stream_close()
{
    digitalWrite(_FLASH_SS, HIGH);
    vera_spi_not_busy();
    vera_spi_write_disable();
}

/*****************************************************************************
 *
 *	Function name : vera_spi_init_stream_read
 *
 *	Returns :		None
 *
 *	Parameters :	unsigned int page ->  The page to begin reading.
 *					byte pageAddress -> The page address to to begin
 *										reading.
 *
 *	Purpose :	Initializes flash for stream read, e.g. read more than one byte
 *				consecutively.  Both page and byte addresses start at 0. Page
 *				ends at address 8191 and page address ends at 255.
 *
 ******************************************************************************/
void vera_spi_init_stream_read(unsigned int page, byte pageAddress)
{
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(READ_DATA);
    SPI.transfer((page >> 8) & 0xFF);
    SPI.transfer((page >> 0) & 0xFF);
    SPI.transfer(pageAddress);
}

/*****************************************************************************
 *
 *	Function name : vera_spi_stream_read
 *
 *	Returns :		byte
 *
 *	Parameters :	None
 *
 *	Purpose :	Reads a byte from the W25Q16.  Must be first called after
 *				vera_spi_init_stream_read and then consecutively to write multiple bytes.
 *
 ******************************************************************************/
byte vera_spi_stream_read()
{
    return SPI.transfer(0);
}

/*****************************************************************************
 *
 *	Function name : vera_spi_close_stream_read
 *
 *	Returns :		None
 *
 *	Parameters :	None
 *
 *	Purpose :	Close the stream read. Must be called after the last call to
 *				vera_spi_stream_read.
 *
 ******************************************************************************/
void vera_spi_close_stream_read()
{
    digitalWrite(_FLASH_SS, HIGH);
    vera_spi_not_busy();
}

/*****************************************************************************
 *
 *	Function name : vera_spi_erase_ic
 *
 *	Returns :		None
 *
 *	Parameters :	None
 *
 *	Purpose :	Erases all data from the flash.
 *
 *
 ******************************************************************************/
void vera_spi_erase_ic()
{
    vera_spi_write_enable();
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(CHIP_ERASE);
    digitalWrite(_FLASH_SS, HIGH);
    vera_spi_not_busy();
    vera_spi_write_disable();
}

/*****************************************************************************
 *
 *	Function name : vera_spi_manufacturer_id
 *
 *	Returns :		byte
 *
 *	Parameters :	None
 *
 *	Purpose :	Reads the manufacturer ID from the W25Q16.  Should return 0xEF.
 *
 *
 ******************************************************************************/
byte vera_spi_manufacturer_id()
{
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(MANUFACTURER_ID);
    SPI.transfer(0);
    SPI.transfer(0);
    SPI.transfer(0);
    byte val = SPI.transfer(0);
    digitalWrite(_FLASH_SS, HIGH);
    vera_spi_not_busy();
    return val;
}
