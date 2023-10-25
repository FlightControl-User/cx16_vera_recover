#include "Arduino.h"
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include "vera-spiffs.h"
#include "vera-spi.h"
#include "WiFiManager.h"

#include <ESPAsyncWebServer.h>

const char *ssid = "";
const char *password = "";
static bool flash = false;

AsyncWebServer server(80);
AsyncEventSource events("/vera");

void handleFlashVera()
{

  Serial.println("Preparing to update your VERA ...");

  vera_spiffs_list_dir(SPIFFS, "/", 0);

  Serial.println("Opening VERA.BIN");
  File file = SPIFFS.open("/VERA.BIN", "r");
  if (!file)
  {
    Serial.println("Failed to open %s for flashing VERA!");
    return;
  }
  else
  {

    // initialize the pcf2127
    events.send("Initializing VERA SPI communications with W25Q16 IC ...\n ", NULL, 0, 0);
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

    Serial.println("Flashing your VERA ...");
    events.send("\nFlashing VERA:\n\n ", NULL, 0, 0);

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
      events.send(".");
      Serial.println();
      Serial.println();
      Serial.printf("Checksum VERA.BIN: %08x\n", check_file);
      Serial.printf("Checksum VERA    : %08x\n", check_vera);
    }
    events.send("\nDone Flashing VERA! \n");
    char eventbuf[256];
    sprintf(eventbuf, "Checksum VERA.BIN: %08x \n", check_file);
    events.send(eventbuf);
    sprintf(eventbuf, "Checksum VERA    : %08x \n", check_vera);
    events.send(eventbuf);
    events.send("\n");
    events.send("\nDone.");
    flash = false;
  }
}


void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  static File file;
  if (!index)
  {
    events.send("Uploading ");
    Serial.printf("Starting file upload: %s\n", filename.c_str());
    file = SPIFFS.open("/" + filename, FILE_WRITE);
    if (!file)
    {
      Serial.println("There was an error opening the file for writing");
      return;
    }
    else
    {
      Serial.printf("Uploading file %s: ", filename.c_str());
    }
  }
  for (size_t i = 0; i < len; i++)
  {
    // Serial.printf("data = %p, index = %u, len = %u\n", data, index, len);
    file.write(data[i]);
  }
  Serial.println(final);
  if (final)
  {
    Serial.println(final);
    Serial.printf("Upload End: %s\n", filename.c_str());
    // file.close();
    events.send("Finished\n");
  }
}

// Replaces placeholder with LED state value
String processor(const String &var)
{
  Serial.println(var);
  // if(var == "STATE"){
  //   if(digitalRead(ledPin)){
  //     ledState = "ON";
  //   }
  //   else{
  //     ledState = "OFF";
  //   }
  //   Serial.print(ledState);
  //   return ledState;
  // }
  return String();
}

void setup()
{
  // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around

  Serial.begin(115200);
  while (!Serial)
    ;

  
  WiFiManager wm;

  // wm.resetSettings();

  bool res;
  res = wm.autoConnect("AutoConnectAP", "password"); // password protected ap

  if (!res)
  {
    Serial.println("Failed to connect");
    ESP.restart();
  }
  else
  {
    Serial.println("connected...very good!");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Read the reset reason
  esp_reset_reason_t reason = esp_reset_reason();

  // Print the reset reason
  Serial.print("Reset reason: ");
  Serial.println(reason);

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  Serial.println("Connected to SPIFFS");
  vera_spiffs_list_dir(SPIFFS, "/", 0);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Sending /index.html");
    request->send(SPIFFS, "/index.html", String(), false, processor); });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/style.css", "text/css"); });

  // upload a file to /upload
  server.on(
      "/upload", HTTP_POST, [](AsyncWebServerRequest *request)
      {
    Serial.println("/upload");    // <-- this I can see 
    request->send(200); },
      handleUpload);

  // handle a file to /flash
  server.on("/flash", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    Serial.println("/flash");    // <-- this I can see 
    request->client()->setRxTimeout(60000);
    if(!flash) {
      flash = true;
    }
    request->send(200); });

  server.onFileUpload(handleUpload);

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it gat is: %u\n", client->lastId());
    }
    //send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("",NULL,millis(),10000);
  });
  //HTTP Basic authentication
  // events.setAuthentication("user", "pass");
  server.addHandler(&events);

  server.begin();
}

void loop()
{
  if(flash) {
    handleFlashVera();
  }
}
