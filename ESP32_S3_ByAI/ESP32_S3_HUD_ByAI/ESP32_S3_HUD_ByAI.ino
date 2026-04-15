#include <Arduino.h>
#include <TFT_eSPI.h>
#include <NimBLEDevice.h>
//#include <TinyGPS++.h>
#include <TinyGPSPlus.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <RTClib.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <esp_task_wdt.h>

////////////////////////////////////////////////////////////
// CONFIG
////////////////////////////////////////////////////////////

#define CORE_COMM 0
#define CORE_UI   1

#define LED_PIN 48
#define BTN_PIN 40
#define BUZZER  5

#define SPEED_LIMIT 80

////////////////////////////////////////////////////////////
// OBJECTS
////////////////////////////////////////////////////////////

TFT_eSPI tft = TFT_eSPI();
TinyGPSPlus gps;
HardwareSerial GNSS(1);
RTC_PCF8563 rtc;

Adafruit_NeoPixel led(1, LED_PIN, NEO_GRB + NEO_KHZ800);

AsyncWebServer server(80);

////////////////////////////////////////////////////////////
// WATCHDOG - New (v3.0.x and later)
////////////////////////////////////////////////////////////
// 1. Create a configuration struct
esp_task_wdt_config_t twdt_config = {
    .timeout_ms = 10000,           // Timeout in milliseconds
    .idle_core_mask = (1 << 0),    // Bitmask of cores to watch (e.g., core 0)
    .trigger_panic = true,         // Enable panic (reset) on timeout
};

// 2. Pass the address of the struct
//esp_task_wdt_init(&twdt_config); 

////////////////////////////////////////////////////////////
// HUD DATA
////////////////////////////////////////////////////////////

typedef struct
{
    float speed;
    float lat;
    float lon;
    bool bleConnected;
    bool wifiEnabled;
    bool gpsFix;
} HUDData;

HUDData hud;

SemaphoreHandle_t dataMutex;

////////////////////////////////////////////////////////////
// STATE
////////////////////////////////////////////////////////////

enum HUDState
{
    HUD_NORMAL,
    HUD_STANDBY,
    HUD_WARNING,
    HUD_ERROR,
    HUD_BLE
};

volatile HUDState hudState = HUD_NORMAL;

////////////////////////////////////////////////////////////
// TASK HANDLES
////////////////////////////////////////////////////////////

TaskHandle_t taskDisplay;
TaskHandle_t taskGNSS;
TaskHandle_t taskBLE;
TaskHandle_t taskLED;
TaskHandle_t taskAudio;
TaskHandle_t taskButton;
TaskHandle_t taskWiFi;

////////////////////////////////////////////////////////////
// LED
////////////////////////////////////////////////////////////

void setLED(uint8_t r,uint8_t g,uint8_t b)
{
    led.setPixelColor(0, led.Color(r,g,b));
    led.show();
}

////////////////////////////////////////////////////////////
// BUZZER
////////////////////////////////////////////////////////////

void beep(int t)
{
    digitalWrite(BUZZER, HIGH);
    delay(t);
    digitalWrite(BUZZER, LOW);
}

////////////////////////////////////////////////////////////
// DISPLAY
////////////////////////////////////////////////////////////

void drawSpeed(float s)
{
    tft.fillRect(0,0,320,150,TFT_BLACK);
    tft.setTextSize(4);
    tft.setCursor(40,50);
    tft.printf("%3.0f",s);
}

////////////////////////////////////////////////////////////
// GNSS TASK
////////////////////////////////////////////////////////////

void TaskGNSS(void *pv)
{
    GNSS.begin(9600, SERIAL_8N1, 18, 17);

    while(1)
    {
        while(GNSS.available())
            gps.encode(GNSS.read());

        if(gps.speed.isUpdated())
        {
            xSemaphoreTake(dataMutex, portMAX_DELAY);

            hud.speed = gps.speed.kmph();
            hud.gpsFix = gps.location.isValid();

            xSemaphoreGive(dataMutex);
        }

        esp_task_wdt_reset();
        vTaskDelay(5);
    }
}

////////////////////////////////////////////////////////////
// DISPLAY TASK
////////////////////////////////////////////////////////////

void TaskDisplay(void *pv)
{
    tft.init();
    tft.setRotation(1);

    while(1)
    {
        float speed;

        xSemaphoreTake(dataMutex, portMAX_DELAY);
        speed = hud.speed;
        xSemaphoreGive(dataMutex);

        switch(hudState)
        {
            case HUD_NORMAL:
                drawSpeed(speed);
                break;

            case HUD_STANDBY:
                tft.fillScreen(TFT_BLACK);
                break;

            case HUD_WARNING:
                drawSpeed(speed);
                break;
        }

        esp_task_wdt_reset();
        vTaskDelay(50);
    }
}

////////////////////////////////////////////////////////////
// LED TASK
////////////////////////////////////////////////////////////

void TaskLED(void *pv)
{
    while(1)
    {
        switch(hudState)
        {
            case HUD_NORMAL:
                setLED(0,255,0);
                break;

            case HUD_WARNING:
                setLED(255,0,0);
                vTaskDelay(100);
                setLED(0,0,0);
                break;

            case HUD_ERROR:
                setLED(255,100,0);
                vTaskDelay(100);
                setLED(0,0,0);
                break;

            case HUD_BLE:
                setLED(0,0,255);
                break;
        }

        esp_task_wdt_reset();
        vTaskDelay(200);
    }
}

////////////////////////////////////////////////////////////
// BUTTON TASK
////////////////////////////////////////////////////////////

void TaskButton(void *pv)
{
    pinMode(BTN_PIN, INPUT_PULLUP);

    uint32_t pressTime=0;

    while(1)
    {
        if(!digitalRead(BTN_PIN))
        {
            pressTime = millis();

            while(!digitalRead(BTN_PIN));

            uint32_t t = millis()-pressTime;

            if(t > 3000)
            {
                hud.wifiEnabled = !hud.wifiEnabled;
            }
            else
            {
                if(hudState==HUD_NORMAL)
                    hudState = HUD_STANDBY;
                else
                    hudState = HUD_NORMAL;
            }
        }

        vTaskDelay(50);
    }
}

////////////////////////////////////////////////////////////
// AUDIO TASK
////////////////////////////////////////////////////////////

void TaskAudio(void *pv)
{
    pinMode(BUZZER, OUTPUT);

    while(1)
    {
        float s;

        xSemaphoreTake(dataMutex, portMAX_DELAY);
        s = hud.speed;
        xSemaphoreGive(dataMutex);

        if(s > SPEED_LIMIT)
        {
            hudState = HUD_WARNING;
            beep(100);
        }
        else
        {
            hudState = HUD_NORMAL;
        }

        vTaskDelay(200);
    }
}

////////////////////////////////////////////////////////////
// BLE TASK
////////////////////////////////////////////////////////////

void TaskBLE(void *pv)
{
    NimBLEDevice::init("HUD");

    NimBLEServer *server = NimBLEDevice::createServer();

    NimBLEService *svc =
        server->createService("A000");

    NimBLECharacteristic *chr =
        svc->createCharacteristic(
            "A001",
            NIMBLE_PROPERTY::WRITE
        );

    svc->start();

    NimBLEDevice::getAdvertising()->start();

    while(1)
    {
        esp_task_wdt_reset();
        vTaskDelay(1000);
    }
}

////////////////////////////////////////////////////////////
// WIFI + WEB
////////////////////////////////////////////////////////////

void startWiFi()
{
    WiFi.softAP("HUD","12345678");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
    {
        req->send(200,"text/html",
        "<h1>HUD config</h1>");
    });

    server.begin();
}

void TaskWiFi(void *pv)
{
    while(1)
    {
        if(hud.wifiEnabled)
            startWiFi();

        vTaskDelay(1000);
    }
}

////////////////////////////////////////////////////////////
// SETUP
////////////////////////////////////////////////////////////

void setup()
{
    Serial.begin(115200);
    delay(1000);
    if (psramFound()) {
        Serial.println("PSRAM is available and working!");
        Serial.print("Total PSRAM: ");
        Serial.println(ESP.getPsramSize());
    } else {
        Serial.println("PSRAM not detected!");
    }
    Serial.print("Flash: ");
    Serial.println(ESP.getFlashChipSize());

    dataMutex = xSemaphoreCreateMutex();

    led.begin();

    //esp_task_wdt_init(5,true);        //error 
    esp_task_wdt_init(&twdt_config);    //New (v3.0.x and later)

    xTaskCreatePinnedToCore(TaskGNSS,"GNSS",4096,NULL,3,&taskGNSS,CORE_COMM);
    xTaskCreatePinnedToCore(TaskDisplay,"DISPLAY",4096,NULL,2,&taskDisplay,CORE_UI);
    xTaskCreatePinnedToCore(TaskLED,"LED",2048,NULL,1,&taskLED,CORE_UI);
    xTaskCreatePinnedToCore(TaskAudio,"AUDIO",2048,NULL,1,&taskAudio,CORE_UI);
    xTaskCreatePinnedToCore(TaskBLE,"BLE",4096,NULL,2,&taskBLE,CORE_COMM);
    xTaskCreatePinnedToCore(TaskButton,"BTN",2048,NULL,1,&taskButton,CORE_COMM);
    xTaskCreatePinnedToCore(TaskWiFi,"WIFI",4096,NULL,1,&taskWiFi,CORE_COMM);
}

void loop() {
  // put your main code here, to run repeatedly:
    esp_task_wdt_reset();
    vTaskDelay(1000);
}
