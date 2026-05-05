/* This code uses ws instead of http*/
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include <WiFi.h>
#include <math.h>
#include <mbedtls/md.h>

/*---Settings---*/
// WIFI
const char *ssid = ""; // Enter SSID
const char *password = "";   // Enter Password

// Data Server
const bool enableServer = false; // Enable server mode, send data to server
const char *serverIP = "";
const uint16_t serverPort = 8000;
const char *serverPath = "";                            // the server data url, with "http://" at the beginning
const char *StationID = "";                             // a unique id for the server to recognize your station
const char *SHARED_SECRET = "";                         // Keep this safe!
// intensity type setting

/* uncomment the one you want to use and comment the others*/

// #define MMI; // Global (Mercalli intensity scale)
#define CWASIS ; // Taiwan (Taiwan seismic intensity scale)
// #define JMA; // Japan (Japan Meteorological Agency seismic intensity scale)
// #define CSIS; // China (China seismic intensity scale)

/* note that PEIS (PHIVOLCS earthquake intensity scale) is not supported*/

// Constant Setting (do not touch if you don't know what you're doing)
const float dataRatio = 980.0 / 8028.6; // the ratio of data:gal, 8028.6 is the experiment result
const float threshold = 8.2;            // the threshold of earthquake or noise

/*---End Settings---*/

MPU6050 mpu;

#define LED_BUILTIN 2

int const INTERRUPT_PIN = 15; // Define the interruption #0 pin
bool blinkState;

/*---MPU6050 Control/Status Variables---*/
bool DMPReady = false;  // Set true if DMP init was successful
uint8_t MPUIntStatus;   // Holds actual interrupt status byte from MPU
uint8_t devStatus;      // Return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // Expected DMP packet size (default is 42 bytes)
uint8_t FIFOBuffer[64]; // FIFO storage buffer

/*---Calibrate---*/
const int usDelay = 3150; // Delay in ms to hold the sampling at 200Hz
const int NFast = 1000;   // Number of quick readings for averaging, the higher the better
const int NSlow = 10000;  // Number of slow readings for averaging, the higher the better
const int LinesBetweenHeaders = 5;

const int iAx = 0;
const int iAy = 1;
const int iAz = 2;
const int iGx = 3;
const int iGy = 4;
const int iGz = 5;

int LowValue[6];
int HighValue[6];
int Smoothed[6];
int LowOffset[6];
int HighOffset[6];
int Target[6];
int LinesOut;
int N;
int i;
// float zDatas[1000];
// float zOffset;

int16_t ax, ay, az;

/*---Orientation/Motion Variables---*/
Quaternion q;        // [w, x, y, z]         Quaternion container
VectorInt16 aa;      // [x, y, z]            Accel sensor measurements
VectorInt16 gy;      // [x, y, z]            Gyro sensor measurements
VectorInt16 aaReal;  // [x, y, z]            Gravity-free accel sensor measurements
VectorInt16 aaWorld; // [x, y, z]            World-frame accel sensor measurements
VectorFloat gravity; // [x, y, z]            Gravity vector

/*---Controling variable---*/
long long worldtime = 0;
unsigned long baseSystemTime = 0;
int t = 0;
bool sendSignal = false;
bool warningMode = false;
bool challengeSignal = false;
bool authenticated = false;
int interest = 0;
int errorLevel = 0;
// float pga = 0.0;
float start_x = 0.0;
float start_y = 0.0;
unsigned int start_time = 0;
float prev_x = 0.0;
float prev_y = 0.0;
float prev_z = 0.0;
// float prev_pga = 0.0;
// float pgx = -5000.0;
// float pgy = -5000.0;
// float pgz = -5000.0;
// float vgx = 5000.0;
// float vgy = 5000.0;
// float vgz = 5000.0;

unsigned int baseTime = 0;
String mode = "unknown";
JsonDocument warningData;
JsonDocument buildData;
String jsonBuffer = "";
char warningBuffer[] = "";
SemaphoreHandle_t jsonMutex;

// JsonDocument waveRecord;
// const char *baseRecord = "{type:\"line\",data:{labels:[],datasets:[{label:\"x\",data:[]},{label:\"y\",data:[]},{label:\"z\",data:[]}]}}";
// deserializeJson(waveRecord, baseRecord);

/*---Next core control---*/
TaskHandle_t Task1;

/*------Interrupt detection routine------*/
volatile bool MPUInterrupt = false; // Indicates whether MPU6050 interrupt pin has gone high
void DMPDataReady() { MPUInterrupt = true; }

void initWire();
void initDevice();
void verifyConnection();
void initDMP();
void initWifi();
void initWS();
int average(int list[]);
float average(float list[]);
void getMode();
float getDirection(float x, float y);
void addToZ(float data);
String toIntensity(float pga);
String calculateHMAC(String payload, String key);

WebSocketsClient webSocket;
bool webSocketConnected = false;

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
    case WStype_DISCONNECTED:
        webSocketConnected = false;
        Serial.println("[WS] Disconnected!");
        digitalWrite(LED_BUILTIN, HIGH);
        authenticated = false;
        break;
    case WStype_CONNECTED:
        webSocketConnected = true;
        Serial.println("[WS] Connected to Server");
        digitalWrite(LED_BUILTIN, LOW);
        // char msg[64];
        // snprintf(msg, sizeof(msg), "{\"type\":\"init\",\"data\":{\"id\":\"%s\",\"dt\":%lu}}", StationID, millis());
        // webSocket.sendTXT(msg);
        break;
    case WStype_TEXT: {
        Serial.println("recieved text");
        String text = (char *)payload;
        Serial.println(text);
        if (text == "restart") {
            ESP.restart();
        }
        if (text == "ack" || text.indexOf("error") >= 0) {
            break;
        }
        if (text == "Authenticated") {
            authenticated = true;
            Serial.println("[WS] Authenticated successfully!");
            break;
        }
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, text);
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            break;
        }
        const char *type = doc["type"];
        if (strcmp(type, "challenge") == 0) {
            String challengeNonce = doc["data"].as<String>();
            String responseHash = calculateHMAC(challengeNonce, SHARED_SECRET);
            JsonDocument outDoc;
            outDoc["type"] = "auth_response";
            outDoc["id"] = "CHY";
            outDoc["data"]["hash"] = responseHash;
            outDoc["data"]["time"] = millis();

            String responseJson = "";
            serializeJson(outDoc, responseJson);
            Serial.println(responseJson);
            webSocket.sendTXT(responseJson);
        }
        break;
    }
    }
}

void DataControl(void *pvParameters) {
    while (true) {
        if (WiFi.status() != WL_CONNECTED) {
            initWifi();
        }
        webSocket.loop();
        if (sendSignal && webSocketConnected && authenticated) {
            digitalWrite(LED_BUILTIN, HIGH);
            Serial.println(jsonBuffer);
            webSocket.sendTXT(jsonBuffer);
            sendSignal = false;
            digitalWrite(LED_BUILTIN, LOW);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void setup() {
    jsonMutex = xSemaphoreCreateMutex();
    initWire();
    Serial.begin(115200);
    while (!Serial)
        ;
    initWifi();
    initWS();
    initDevice();
    verifyConnection();
    initDMP();
    getMode();
    xTaskCreatePinnedToCore(DataControl, /*任務實際對應的Function*/
                            "Task1",     /*任務名稱*/
                            20000,       /*堆疊空間*/
                            NULL,        /*無輸入值*/
                            0,           /*優先序0*/
                            &Task1,      /*對應的任務變數位址*/
                            1);          /*指定在核心0執行 */

    /* Making sure it worked (returns 0 if so) */
    if (devStatus == 0) {
        mpu.CalibrateAccel(50); // Calibration Time: generate offsets and calibrate our MPU6050
        mpu.CalibrateGyro(50);
        // calibrate();
        Serial.println("These are the Active offsets: ");
        mpu.PrintActiveOffsets();
        Serial.println(F("Enabling DMP...")); // Turning ON DMP
        mpu.setDMPEnabled(true);

        /*Enable Arduino interrupt detection*/
        Serial.print(F("Enabling interrupt detection (Arduino external interrupt "));
        Serial.print(digitalPinToInterrupt(INTERRUPT_PIN));
        Serial.println(F(")..."));
        attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), DMPDataReady, RISING);
        MPUIntStatus = mpu.getIntStatus();

        /* Set the DMP Ready flag so the main loop() function knows it is okay
         * to use it */
        Serial.println(F("DMP ready! Waiting for first interrupt..."));
        DMPReady = true;
        packetSize = mpu.dmpGetFIFOPacketSize(); // Get expected DMP packet size
                                                 // for later comparison
    } else {
        Serial.print(F("DMP Initialization failed (code ")); // Print the error code
        Serial.print(devStatus);
        Serial.println(F(")"));
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
    }
    
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    if (!DMPReady)
        return; // Stop the program if DMP programming fails.

    vTaskDelay(pdMS_TO_TICKS(1)); // delay(1);
    long nowMill = millis();
    unsigned long dt = nowMill - baseSystemTime;
    /* Read a packet from FIFO */
    if (mpu.dmpGetCurrentFIFOPacket(FIFOBuffer)) { // Get the Latest packet
        /* Display initial world-frame acceleration, adjusted to remove gravity
        and rotated based on known orientation from Quaternion */

        // Serial.println(dt);

        mpu.dmpGetQuaternion(&q, FIFOBuffer);
        mpu.dmpGetAccel(&aa, FIFOBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
        // mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);

        ax = aaReal.x;
        ay = aaReal.y;
        az = aaReal.z;
        // addToZ(galz);
        // zOffset = average(zDatas);
        /*
        Serial.print("rx:");
        Serial.print(ax);
        Serial.print(",ry:");
        Serial.print(ay);
        Serial.print(",rz:");
        Serial.print(az);
        Serial.print(",interest:");
        Serial.print(interest);
        */
        JsonObject appendData = buildData["data"].add<JsonObject>();
        appendData["dt"] = nowMill;
        appendData["x"] = ax;
        appendData["y"] = ay;
        appendData["z"] = az;
        if (abs(ax - prev_x) >= threshold || abs(ay - prev_y) >= threshold || abs(az - prev_z) >= threshold) { // || abs(az) >= threshold) {
            if (interest == 0) {
                start_x = ax;
                start_y = ay;
                start_time = millis();
            }
            interest += 1;
        } else if (interest > 0) {
            interest -= 1;
        } else {
            warningMode = false;
        }
        prev_x = ax;
        prev_y = ay;
        prev_z = az;
        if (interest > 6) {
            warningMode = true;
        }
        if (dt > (warningMode ? 500 : 1000) && enableServer) {
            baseSystemTime = millis();
            if (buildData.size() > 0) {
                buildData["id"] = StationID;
                buildData["type"] = "data";
                serializeJson(buildData, jsonBuffer);
                sendSignal = true;
            }
            buildData.clear();
        }
        errorLevel = 0;
    } else {
        // delay(32);
        errorLevel++;
        if (errorLevel > 1000) {
            if (mpu.testConnection()) {
                Serial.println("MPU6050 connection re-established.");
                // If connection is good, try to re-enable DMP.
                // This might be redundant if the issue was just a momentary glitch,
                // but good practice if the MPU6050's internal state got corrupted.
                Serial.println("Attempting to re-enable DMP...");
                devStatus = mpu.dmpInitialize();
                if (devStatus == 0) {
                    mpu.CalibrateAccel(50); // Calibration Time: generate offsets and calibrate our MPU6050
                    mpu.CalibrateGyro(50);
                    Serial.println("These are the Active offsets: ");
                    mpu.PrintActiveOffsets();
                    Serial.println(F("Enabling DMP...")); // Turning ON DMP
                    mpu.setDMPEnabled(true);
                    DMPReady = true; // Mark DMP as ready again
                    Serial.println("DMP re-enabled successfully!");
                } else {
                    Serial.print(F("DMP re-initialization failed (code "));
                    Serial.print(devStatus);
                    Serial.println(F("). May need a hard reset."));
                    DMPReady = false; // DMP still not ready
                }
            } else {
                Serial.println("MPU6050 connection still failed. Checking I2C bus...");
                Wire.end();
                initWire();
                if (devStatus == 0) {
                    mpu.CalibrateAccel(50); // Calibration Time: generate offsets and calibrate our MPU6050
                    mpu.CalibrateGyro(50);
                    Serial.println("These are the Active offsets: ");
                    mpu.PrintActiveOffsets();
                    Serial.println(F("Enabling DMP...")); // Turning ON DMP
                    mpu.setDMPEnabled(true);
                    DMPReady = true; // Mark DMP as ready again
                    Serial.println("DMP re-enabled successfully!");
                } else {
                    ESP.restart();
                }
            }
            errorLevel = 0;
        }
    }
}

void initWire() {
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    Wire.begin();
    Wire.setClock(400000); // 400kHz I2C clock. Comment on this line if having
                           // compilation difficulties
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
    Fastwire::setup(400, true);
#endif
}

void initDevice() {
    /*Initialize device*/
    Serial.println(F("Initializing I2C devices..."));
    mpu.initialize();
    pinMode(INTERRUPT_PIN, INPUT);
}

void verifyConnection() {
    /*Verify connection*/
    Serial.println(F("Testing MPU6050 connection..."));
    if (mpu.testConnection() == false) {
        Serial.println("MPU6050 connection failed");
        while (true)
            ;
    } else {
        Serial.println("MPU6050 connection successful");
    }
}

void initDMP() {
    /* Initializate and configure the DMP*/
    Serial.println(F("Initializing DMP..."));
    devStatus = mpu.dmpInitialize();
}

void initWifi() {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("connecting to wifi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("Connected");
    Serial.print("IP位址:");
    Serial.println(WiFi.localIP()); //讀取IP位址
    digitalWrite(LED_BUILTIN, LOW);
}

void initWS() {
    Serial.println("[WS] initializing WS...");
    webSocket.begin(serverIP, serverPort, serverPath);
    Serial.println("[WS] OK");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(500);
}

String calculateHMAC(String payload, String key) {
    byte hmacResult[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char *)key.c_str(), key.length());
    mbedtls_md_hmac_update(&ctx, (const unsigned char *)payload.c_str(), payload.length());
    mbedtls_md_hmac_finish(&ctx, hmacResult);
    mbedtls_md_free(&ctx);

    String hash = "";
    for (int i = 0; i < 32; i++) {
        char str[3];
        sprintf(str, "%02x", (int)hmacResult[i]);
        hash += str;
    }
    return hash;
}

void getMode() {
#ifdef MMI; // Global (Mercalli intensity scale)
    mode = "MMI";
#endif

#ifdef CWASIS; // Taiwan
    mode = "CWASIS";
#endif

#ifdef JMA; // Japan (Japan Meteorological Agency seismic intensity scale)
    mode = "JMA";
#endif

#ifdef CSIS; // China (China seismic intensity scale)
    mode = "CSIS";
#endif
}

int average(int list[]) {
    int items = sizeof(list) / sizeof(list[0]);
    long long arraySum = 0;
    for (int index = 0; index < items; index++) {
        arraySum += list[index];
    }
    return arraySum / items;
}

float average(float list[]) {
    int items = sizeof(list) / sizeof(list[0]);
    long double arraySum = 0;
    for (int index = 0; index < items; index++) {
        arraySum += list[index];
    }
    return arraySum / items;
}

String toIntensity(float pga) {
#ifdef MMI; // Global (Mercalli intensity scale)
    if (pga < 1.0) {
        return "1(I)";
    }
    if (pga <= 2.1) {
        return "2(II)";
    }
    if (pga <= 5.0) {
        return "3(III)";
    }
    if (pga <= 10) {
        return "4(IV)";
    }
    if (pga <= 21) {
        return "5(V)";
    }
    if (pga <= 44) {
        return "6(VI)";
    }
    if (pga <= 94) {
        return "7(VII)";
    }
    if (pga <= 202) {
        return "8(VIII)";
    }
    if (pga <= 432) {
        return "9(IX)";
    }
    return "10(X)";
#endif

#ifdef CWASIS; // Taiwan
    if (pga < 0.8) {
        return "0";
    }
    if (pga <= 2.5) {
        return "1";
    }
    if (pga <= 8) {
        return "2";
    }
    if (pga <= 25) {
        return "3";
    }
    if (pga <= 80) {
        return "4";
    }
    if (pga <= 140) {
        return "5弱*";
    }
    if (pga <= 250) {
        return "5強*";
    }
    if (pga <= 440) {
        return "6弱*";
    }
    if (pga <= 800) {
        return "6強*";
    }
    return "7*";
#endif

#ifdef JMA; // Japan (Japan Meteorological Agency seismic intensity scale)
    if (pga < 0.8) {
        return "0";
    }
    if (pga <= 2.5) {
        return "1";
    }
    if (pga <= 8) {
        return "2";
    }
    if (pga <= 25) {
        return "3";
    }
    if (pga <= 80) {
        return "4";
    }
    if (pga <= 140) {
        return "5弱";
    }
    if (pga <= 250) {
        return "5強";
    }
    if (pga <= 315) {
        return "6弱";
    }
    if (pga <= 400) {
        return "6強";
    }
    return "7";
#endif

#ifdef CSIS; // China (China seismic intensity scale)
    if (pga <= 2.57) {
        return "1(I)";
    }
    if (pga <= 5.28) {
        return "2(II)";
    }
    if (pga <= 10.8) {
        return "3(III)";
    }
    if (pga <= 22.2) {
        return "4(IV)";
    }
    if (pga <= 45.6) {
        return "5(V)";
    }
    if (pga <= 93.6) {
        return "6(VI)";
    }
    if (pga <= 194) {
        return "7(VII)";
    }
    if (pga <= 401) {
        return "8(VIII)";
    }
    if (pga <= 830) {
        return "9(IX)";
    }
    if (pga <= 1720) {
        return "10(X)";
    }
    if (pga <= 35.5) {
        return "11(XI)";
    }
    return "12(XII)";
#endif
}

float getDirection(float x, float y) {
    double res = atan2(x, y);
    if (res < 0) {
        res += M_PI;
    }
    return (180 / M_PI) / res;
}