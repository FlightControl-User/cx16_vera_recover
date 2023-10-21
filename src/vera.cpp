
#include "Arduino.h"
#include <SPI.h> //You must include the SPI library along with the W25Q16 library!
#include <stdio.h>
#include "esp32-hal-log.h"
#include "FS.h"
#include "SPIFFS.h"
#include "vera-spiffs.h"
#include "vera-spi.h"

/* You only need to format SPIFFS the first time you run a
   test or else use the SPIFFS plugin to create a partition
   https://github.com/me-no-dev/arduino-esp32fs-plugin */
#define FORMAT_SPIFFS_IF_FAILED true

static const char *TAG = "FileSystem";


unsigned int startPage = 0;
unsigned int startPageAddress = 0;
unsigned int endPage = 0;
unsigned int endPageAddress = 0;

unsigned char i = 0;


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

            // vera_spiffs_write_file(SPIFFS, "/hello.txt", "Hello ");
            // vera_spiffs_append_file(SPIFFS, "/hello.txt", "World!\r\n");
            // vera_spiffs_read_file(SPIFFS, "/hello.txt");
            // vera_spiffs_rename_file(SPIFFS, "/hello.txt", "/foo.txt");
            // vera_spiffs_read_file(SPIFFS, "/foo.txt");
            // vera_spiffs_delete_file(SPIFFS, "/foo.txt");
            // vera_spiffs_test_file_io(SPIFFS, "/test.txt");
            // vera_spiffs_delete_file(SPIFFS, "/test.txt");
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

                vera_spiffs_list_dir(SPIFFS, "/", 0);

                // initialize the pcf2127
                Serial.println("Initializing VERA SPI communications with W25Q16 IC ...");
                vera_spi_init(10);

                // read the manufacturer ID to make sure communications are OK
                // should output 0xEF
                byte manID = vera_spi_manufacturer_id();
                Serial.print("Manufacturer ID: ");
                Serial.println(manID, HEX);

                Serial.println("VERA W25Q16 IC erase ...");
                vera_spi_erase_ic();

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
