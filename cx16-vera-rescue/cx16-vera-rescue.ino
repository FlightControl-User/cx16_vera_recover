#include <WiFiManager.h>
#include <strings_en.h>
#include <wm_consts_en.h>
#include <wm_strings_en.h>
#include <wm_strings_es.h>

#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include "vera-spiffs.h"
#include "vera-spi.h"

const char* ssid = "";
const char* password = "";

WebServer server(80);


void handleFlashVera() {

  server.send(200, "text/plain", "Preparing to update your VERA ...");

  Serial.println("Preparing to update your VERA ...");

  vera_spiffs_list_dir(SPIFFS, "/", 0);


  Serial.println("Opening VERA.BIN");
  File file = SPIFFS.open("/VERA.BIN", "r");
  if (!file) {
    Serial.println("Failed to open %s for flashing VERA!");
    return;
  } else {

    // initialize the pcf2127
    server.send(200, "text/plain", "Initializing VERA SPI communications with W25Q16 IC ...");
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

    unsigned long vera_flash_size = vera_bin_size - 32;  // Reduce the header
    unsigned int vera_bin_header_pos = 0;
    const unsigned int vera_bin_header_size = 32;
    while (file.available() && vera_bin_header_pos < vera_bin_header_size) {
      char ch = file.read();
      vera_bin_header_pos++;
    }

    unsigned int vera_page_total = (vera_bin_size + 255) / 256;

    const unsigned char hex_width = 32;

    unsigned long check_file = 0;
    unsigned long check_vera = 0;

    server.send(200, "text/plain", "Flashing your VERA ...");
    Serial.println("Flashing your VERA ...");

    for (unsigned int vera_page = 0; vera_page < vera_page_total; vera_page++) {
      Serial.println();
      Serial.print("FILE:");

      unsigned int vera_bin_block_size = 0;
      while (file.available() && vera_bin_block_size < 256) {
        char ch = file.read();
        buffer[vera_bin_block_size] = ch;
        vera_bin_block_size++;
      }

      // vera_spi_stream_open(vera_page, 0);  // Start write stream to VERA from vera_page.
      for (int file_block = 0; file_block < vera_bin_block_size; file_block++) {
        if (!(file_block % hex_width)) {
          Serial.println();
          Serial.printf("%04x%02x: ", vera_page, file_block);
        }
        Serial.printf("%02x ", buffer[file_block]);
        // vera_spi_stream_write(buffer[file_block]);  // Write to VERA.
        check_file += buffer[file_block];
      }
      Serial.println();
      // vera_spi_stream_close();  // Close stream of page to VERA.

      Serial.println();
      Serial.print("VERA:");
      unsigned int vera_block = 0;
      for (unsigned int page_address = 0; page_address < vera_bin_block_size; page_address++) {
        if (!(vera_block % hex_width)) {
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

      server.send(200, "text/plain", "Done flashing your VERA ...");
    }
  }
}


void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  static String filename = upload.filename.c_str();
  static File file;
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Starting file upload: %s\n", filename.c_str());
    if (!SPIFFS.begin(true)) {
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
    file = SPIFFS.open("/" + filename, FILE_WRITE);
    if (!file) {
      Serial.println("There was an error opening the file for writing");
      return;
    } else {
      Serial.printf("Writing file %s: ", filename.c_str());
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    Serial.print(".");
    if (file.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Serial.println("\n\nThere was an error writing the file");
      return;
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    Serial.printf("\nUpload file %s complete\n", filename.c_str());
    file.close();
    server.send(200, "text/html", "<form method='GET' action='/flash' enctype='multipart/form-data'> <input type='submit' value='Flash' formaction='/flash' formethod='GET'/> </form>");
  }
}

void handleRoot() {
  server.send(200, "text/html", "<form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='file'><input type='submit' value='Upload'/> </form>");
}


void setup() {
      //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wm;

    // reset settings - wipe stored credentials for testing
    // these are stored by the esp library
    // wm.resetSettings();

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    res = wm.autoConnect("AutoConnectAP","password"); // password protected ap

    if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
    }

  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);

  server.on(
    "/upload", HTTP_POST, []() {
      server.send(200, "text/plain", "");
    },
    handleFileUpload);

  server.on("/flash", handleFlashVera);

  server.begin();
}

void loop() {
  server.handleClient();
}
