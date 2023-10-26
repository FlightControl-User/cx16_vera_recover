/**
 * @file cx16-vera-rescue-async.cpp
 * @author Sven Van de Velde (email:sven.van.de.velde@telenet.be)
 * @brief An Arduino Nano ESP32 Asynchronous Web Server to flash the SPI W25Q16 VERA chip of the Commander X16.
 * It's been a long journey ...
 * 
 * @note **Consult the [cheat sheet](https://docs.arduino.cc/tutorials/nano-esp32/cheat-sheet)** of the Arduino Nano ESP32 for more info.
 * 
 * @note **Open this project in the PlatformIO GUI.**
 *   - This is a **Platform IO** repository. 
 *   - Install PlatformIO first from the VSCODE extensions.
 *   - Open this project folder using the PlatformIO GUI. Click on **PlatformIO icon**, then open **Quick Access > PIO Home > Open**.
 *   - After opening, PlatformIO will scan the folder.
 * 
 * @note **Important files**
 *   - The `platformio.ini` file is very important. It contains the settings how the Arduino Nano ESP32 will be flashed.
 *   - An other important file is `app3M_spiffs9M_fact512k_16MB.csv`, it is used to define the partitions of the Arduino Nano ESP32.
 * 
 * @note **Install data files on the SPIFFS partition**
 *   - The solution requires a **SPIFFS file partition** to be filled with data (index.html, style.css and the like).
 *   - You will find these files in the data folder in the **CX16-VERA-RESCUE-ASYNC > data folder**.
 *   - Install using **Project Tasks > arduino_nano_esp32 > Upload Filesystem Image**.
 *   - Before you install, ensure you **put the Arduino Nano in 
 *     [bootloader](https://support.arduino.cc/hc/en-us/articles/9810414060188-Reset-the-Arduino-bootloader-on-the-Nano-ESP32) mode**! 
 *     Use a female/female jumper cable to connect the GND PIN with the B1 PIN. You should see a green led. 
 *     Then press the reset button once, you'll hear a sound, and then unplug the jumper cable from one of the PINs. 
 *     A purple led should light up.  
 *   - In the `platformio.ini` file, uncomment the line `; upload_protocol = esptool` ( Delete the `;` ).
 *     Or, alternatively, you can execute the powershell command 
 *     `. $env:USERPROFILE\.platformio\penv\Scripts\platformio.exe project init --project-option "upload_protocol=esptool" -e arduino_nano_esp32 -s`
 *     from a terminal window in VSCODE.
 *   - You'll see that in **Project Tasks** the menu options seem to dissapear, but they will come back after a while!
 *   - Once the **Project Tasks** menu options return, click on **Project Tasks > arduino_nano_esp32 > Upload Filesystem Image**.
 *     The file system for the data will be created, and will be uploaded using the esptool.
 *     It needs the `COM6` port of the Arduino Nano ESP32 to flash the **SPIFFS partition** containing the data.
 *     You'll see a lot of verbose logs, and observe the interesting messages.
 *   - Once the SPIFFS partition has been flashed onto the Arduino Nano ESP32, press the reset button the device.
 * 
 * @note **Install the web server onto the Arduino Nano ESP32**  
 *   - The solution hosting the web server also needs to be built (compiled and uploaded onto the Arduino Nano ESP32).
 *   - Comment the line `upload_protocol = esptool` in the `platformio.ini` file, by placing a `;` in the front again.
 *     Or, alternatively, you can execute the powershell command 
 *     `. $env:USERPROFILE\.platformio\penv\Scripts\platformio.exe project init --project-option "upload_protocol=dfu" -e arduino_nano_esp32 -s`
 *   - The Platform IO workspace will rescan again the whole project and the **Project Tasks** will reappear after a while.
 *   - Now click on **Project Tasks > arduino_nano_esp32 > Build**.
 *   - Once done, click on **Project Tasks > arduino_nano_esp32 > Upload**.
 *   - The upload happens with the ´DFU tool´, which uses the `COM7` port of the Arduino Nano ESP32. No bootloader mode is required.
 * 
 * @note **Run the web server and use it**
 *   - Once the build has happened, it should run the web server on your Arduino Nano ESP32.
 *   - It hosts a WIFI provisioning solution. Find in your WIFI on your computer an ConnectAP SSID, and connect.
 *   - A web page should open from where you can select your WIFI SSID and enter your password.
 *   - Once this is correctly done, the connection parameters will be permanently stored on the Arduino Nano ESP32.
 *   - Open the **Project Tasks > arduino_nano_esp32 > Monitor** ... It broadcasts the IP number in the monitor, on which the Arduino Nano ESP32 is connected to.
 *   - Open a browser and you should be able to connect.
 *   - Once the web page pops up, select the VERA.BIN file to flash.
 *   - Click Upload.
 *   - Click Update.
 *   - The update progress should be displayed in the box underneath.
 *   - Before you do that, ensure that your Arduino Nano ESP32 PINS are properly connected with the VERA header pins!
 * 
 * @version 0.1
 * @date 2023-10-25
 * 
 * @copyright Copyright (c) 2023
 * 
 */


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

    events.send("Initializing VERA SPI communications with W25Q16 IC ...\n ", NULL, 0, 0);
    Serial.println("Initializing VERA SPI communications with W25Q16 IC ...");
    vera_spi_init(10);

    // read the manufacturer ID 
    // it should be should output 0xEF
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
  // while (!Serial)
    // ;

  
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
  server.addHandler(&events);

  server.begin();
}

void loop()
{
  if(flash) {
    handleFlashVera();
  }
}
