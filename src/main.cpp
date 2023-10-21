
#include "Arduino.h"
#include <SPI.h> //You must include the SPI library along with the W25Q16 library!
#include <stdio.h>
#include "esp32-hal-log.h"
#include "FS.h"
#include "SPIFFS.h"

/* You only need to format SPIFFS the first time you run a
   test or else use the SPIFFS plugin to create a partition
   https://github.com/me-no-dev/arduino-esp32fs-plugin */
#define FORMAT_SPIFFS_IF_FAILED true

static const char *TAG = "FileSystem";
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

void notBusy()
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
 *	Function name : powerDown
 *
 *	Returns :		None
 *
 *	Parameters :	None
 *
 *	Purpose :	Puts the flash in its low power mode.
 *
 *
 ******************************************************************************/
void powerDown()
{
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(POWER_DOWN);
    digitalWrite(_FLASH_SS, HIGH);
    notBusy();
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
void releasePowerDown()
{
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(RELEASE_POWER_DOWN);
    digitalWrite(_FLASH_SS, HIGH);
    notBusy();
}

void writeEnable()
{
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(WRITE_ENABLE);
    digitalWrite(_FLASH_SS, HIGH);
    notBusy();
}

void writeDisable()
{
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(WRITE_DISABLE);
    digitalWrite(_FLASH_SS, HIGH);
    notBusy();
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
    releasePowerDown();
    writeDisable();
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
    notBusy();
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
    writeEnable();
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(PAGE_PROGRAM);
    SPI.transfer((page >> 8) & 0xFF);
    SPI.transfer((page >> 0) & 0xFF);
    SPI.transfer(pageAddress);
    SPI.transfer(val);
    digitalWrite(_FLASH_SS, HIGH);
    notBusy();
    writeDisable();
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
    writeEnable();
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
    notBusy();
    writeDisable();
}

/*****************************************************************************
 *
 *	Function name : initStreamRead
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
void initStreamRead(unsigned int page, byte pageAddress)
{
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(READ_DATA);
    SPI.transfer((page >> 8) & 0xFF);
    SPI.transfer((page >> 0) & 0xFF);
    SPI.transfer(pageAddress);
}

/*****************************************************************************
 *
 *	Function name : streamRead
 *
 *	Returns :		byte
 *
 *	Parameters :	None
 *
 *	Purpose :	Reads a byte from the W25Q16.  Must be first called after
 *				initStreamRead and then consecutively to write multiple bytes.
 *
 ******************************************************************************/
byte streamRead()
{
    return SPI.transfer(0);
}

/*****************************************************************************
 *
 *	Function name : closeStreamRead
 *
 *	Returns :		None
 *
 *	Parameters :	None
 *
 *	Purpose :	Close the stream read. Must be called after the last call to
 *				streamRead.
 *
 ******************************************************************************/
void closeStreamRead()
{
    digitalWrite(_FLASH_SS, HIGH);
    notBusy();
}

/*****************************************************************************
 *
 *	Function name : icErase
 *
 *	Returns :		None
 *
 *	Parameters :	None
 *
 *	Purpose :	Erases all data from the flash.
 *
 *
 ******************************************************************************/
void icErase()
{
    writeEnable();
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(CHIP_ERASE);
    digitalWrite(_FLASH_SS, HIGH);
    notBusy();
    writeDisable();
}

/*****************************************************************************
 *
 *	Function name : manufacturerID
 *
 *	Returns :		byte
 *
 *	Parameters :	None
 *
 *	Purpose :	Reads the manufacturer ID from the W25Q16.  Should return 0xEF.
 *
 *
 ******************************************************************************/
byte manufacturerID()
{
    digitalWrite(_FLASH_SS, LOW);
    SPI.transfer(MANUFACTURER_ID);
    SPI.transfer(0);
    SPI.transfer(0);
    SPI.transfer(0);
    byte val = SPI.transfer(0);
    digitalWrite(_FLASH_SS, HIGH);
    notBusy();
    return val;
}

unsigned int startPage = 0;
unsigned int startPageAddress = 0;
unsigned int endPage = 0;
unsigned int endPageAddress = 0;

unsigned char i = 0;

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.name(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory())
    {
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while (file.available())
    {
        Serial.write(file.read());
    }
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- file written");
    }
    else
    {
        Serial.println("- frite failed");
    }
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
        Serial.println("- failed to open file for appending");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- message appended");
    }
    else
    {
        Serial.println("- append failed");
    }
}

void renameFile(fs::FS &fs, const char *path1, const char *path2)
{
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2))
    {
        Serial.println("- file renamed");
    }
    else
    {
        Serial.println("- rename failed");
    }
}

void deleteFile(fs::FS &fs, const char *path)
{
    Serial.printf("Deleting file: %s\r\n", path);
    if (fs.remove(path))
    {
        Serial.println("- file deleted");
    }
    else
    {
        Serial.println("- delete failed");
    }
}

void testFileIO(fs::FS &fs, const char *path)
{
    Serial.printf("Testing file I/O with %s\r\n", path);

    static uint8_t buf[512];
    size_t len = 0;
    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }

    size_t i;
    Serial.print("- writing");
    uint32_t start = millis();
    for (i = 0; i < 2048; i++)
    {
        if ((i & 0x001F) == 0x001F)
        {
            Serial.print(".");
        }
        file.write(buf, 512);
    }
    Serial.println("");
    uint32_t end = millis() - start;
    Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
    file.close();

    file = fs.open(path);
    start = millis();
    end = start;
    i = 0;
    if (file && !file.isDirectory())
    {
        len = file.size();
        size_t flen = len;
        start = millis();
        Serial.print("- reading");
        while (len)
        {
            size_t toRead = len;
            if (toRead > 512)
            {
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F)
            {
                Serial.print(".");
            }
            len -= toRead;
        }
        Serial.println("");
        end = millis() - start;
        Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
        file.close();
    }
    else
    {
        Serial.println("- failed to open file for reading");
    }
}

void setup()
{

    Serial.begin(9600);
    while (!Serial)
        ;

    esp_log_level_set("*", ESP_LOG_ERROR);

    // int i = 1;
    // for(int p=0; p<3; p++) {
    //   Serial.println("Page:" + p);
    //   for(int pa=0; pa<256; pa++) {
    //     Serial.print((byte)read(p, pa), HEX);
    //     Serial.print(" ");
    //   }
    //   Serial.println(" ");
    // }

    // //put the flash in lowest power state
    // powerDown();
}

static char mode = 0;
static char b = 0;

void loop()
{

    // esp_vfs_spiffs_conf_t config = {
    //   .base_path = "/",
    //   .partition_label = NULL,
    //   .max_files = 5,
    //   .format_if_mount_failed = true
    // };

    if (Serial.available() > 0)
    {
        i = Serial.read();
        if (i = 's' && mode == 0)
        {
            mode = 1;
            b = 0;
            Serial.println("Listening");

            unsigned result = 0;
            if (!(result = SPIFFS.begin(true)))
            {
                Serial.printf("Failed to initialize SPIFFS %i\n", result);
                delay(1000);
                return;
            }
            else
            {
                ESP_LOGI(TAG, "Partition OK!\n");
            }

            // writeFile(SPIFFS, "/hello.txt", "Hello ");
            // appendFile(SPIFFS, "/hello.txt", "World!\r\n");
            // readFile(SPIFFS, "/hello.txt");
            // renameFile(SPIFFS, "/hello.txt", "/foo.txt");
            // readFile(SPIFFS, "/foo.txt");
            // deleteFile(SPIFFS, "/foo.txt");
            // testFileIO(SPIFFS, "/test.txt");
            // deleteFile(SPIFFS, "/test.txt");
            // Serial.println("Test complete");

            Serial.println();

            File file = SPIFFS.open("/VERA.BIN", "r");
            if (!file)
            {
                Serial.println("Failed to open VERA.BIN for reading.");
                return;
            }
            else
            {

                listDir(SPIFFS, "/", 0);

                // initialize the pcf2127
                Serial.println("Initializing VERA SPI communications with W25Q16 IC ...");
                vera_spi_init(10);

                // read the manufacturer ID to make sure communications are OK
                // should output 0xEF
                byte manID = manufacturerID();
                Serial.print("Manufacturer ID: ");
                Serial.println(manID, HEX);

                Serial.println("VERA W25Q16 IC erase ...");
                icErase();

                size_t vera_bin_size = file.size();
                Serial.println("FILE info:");
                Serial.printf("FILE: VERA.BIN, SIZE (HEX): $%08x\n", vera_bin_size);

                delay(1000);

                size_t len = 0;

                static uint8_t buffer[512];

                unsigned long vera_flash_size = vera_bin_size - 32; // Reduce the header
                unsigned int vera_bin_header_pos = 0;
                const unsigned int vera_bin_header_size = 32;
                while (file.available() && vera_bin_header_pos < vera_bin_header_size)
                {
                    char ch = file.read();
                    vera_bin_header_pos++;
                }

                unsigned int vera_page_total = (vera_bin_size + 255) / 256;

                const unsigned char hex_width = 32;

                unsigned long check_file = 0;
                unsigned long check_vera = 0;

                for (unsigned int vera_page = 0; vera_page < vera_page_total; vera_page++)
                {
                    Serial.println();
                    Serial.print("FILE:");

                    unsigned int vera_bin_block_size = 0;
                    while (file.available() && vera_bin_block_size < 256)
                    {
                        char ch = file.read();
                        buffer[vera_bin_block_size] = ch;
                        vera_bin_block_size++;
                    }

                    vera_spi_stream_open(vera_page, 0); // Start write stream to VERA from vera_page.
                    for (int file_block = 0; file_block < vera_bin_block_size; file_block++)
                    {
                        if (!(file_block % hex_width))
                        {
                            Serial.println();
                            Serial.printf("%04x%02x: ", vera_page, file_block);
                        }
                        Serial.printf("%02x ", buffer[file_block]);
                        vera_spi_stream_write(buffer[file_block]); // Write to VERA.
                        check_file += buffer[file_block];
                    }
                    Serial.println();
                    vera_spi_stream_close(); // Close stream of page to VERA.

                    Serial.println();
                    Serial.print("VERA:");
                    unsigned int vera_block = 0;
                    for (unsigned int page_address = 0; page_address < vera_bin_block_size; page_address++)
                    {
                        if (!(vera_block % hex_width))
                        {
                            Serial.println();
                            Serial.printf("%04x%02x: ", vera_page, page_address);
                        }
                        byte vera_byte = (byte)vera_spi_read(vera_page, (byte)page_address);
                        Serial.printf("%02x ", vera_byte);
                        check_vera += vera_byte;
                        vera_block++;
                    }
                    Serial.println();
                    Serial.println();
                    Serial.printf("Checksum VERA.BIN: %08x\n", check_file);
                    Serial.printf("Checksum VERA    : %08x\n", check_vera);
                }
            }
        }

        delay(500);
    }
}
