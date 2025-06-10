#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <iostream>

/*---Settings---*/
// WIFI
const char *ssid = ""; // Enter SSID
const char *password = "";   // Enter Password

// Data Server
const bool enableServer = false; // Enable server mode, send data to server
const char *dataServer = ""; // the server data url, with "http://" at the beginning
const char *registerServer = ""; // the server register url, with "http://" at the beginning
const char *StationID = ""; // a unique id for the server to recognize your station

// Discord
const bool enableDiscord = true; // Enable discord mode, send notification to discord
const char *webhook = ""; // discord webhook url

// intensity type setting

    /* uncomment the one you want to use and comment the others*/

// #define MMI; // Global (Mercalli intensity scale)
// #define CWASIS ; // Taiwan (Taiwan seismic intensity scale)
// #define JMA; // Japan (Japan Meteorological Agency seismic intensity scale)
// #define CSIS; // China (China seismic intensity scale)

    /* note that PEIS (PHIVOLCS earthquake intensity scale) is not supported*/

// Constant Setting (do not touch if you don't know what you're doing)
const float dataRatio = 980.0 / 8028.6; // the ratio of data:gal, 8028.6 is the experiment result
const float threshold = 3.5; // the threshold of earthquake or noise

// Pin setting (do not touch if you don't know what you're doing)
int const INTERRUPT_PIN = 15; // Define the interruption pin
#define LED_BUILTIN 2 // define the led pin, for ESP32 it's 2

/*---End Settings---*/

MPU6050 mpu;
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

int16_t ax, ay, az;
int lax[4], lay[4], laz[4];
int mainCount;

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
bool discordSignal = false;
bool discordLock = false;
int interest = 0;
float pga = 0;
String mode = "unknown";
JsonDocument sendData;
JsonDocument buildData;

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
void calibrate();
int average(int list[]);
void getMode();
String toIntensity(float pga);

HTTPClient timer;
void DataControl(void *pvParameters) {
    while (true) {
        if (WiFi.status() != WL_CONNECTED) {
            initWifi();
        }
        if (sendSignal) {
            digitalWrite(LED_BUILTIN, HIGH);
            HTTPClient sender;
            sender.begin(dataServer);
            sender.addHeader("Content-Type", "application/json; charset=UTF-8");
            sender.addHeader("Connection", "keep-alive");
            sendData["id"] = StationID;
            String sendString;
            serializeJson(sendData, sendString);
            int code = sender.POST(sendString);
            if (code == HTTP_CODE_OK) {
                String payload = sender.getString();
                if (payload == "false") { // restart signal from server
                    ESP.restart();
                }
            }
            sender.end();
            // Serial.println(code);
            sendSignal = false;
            // blinkState = !blinkState;
            if (code == 200) {
                digitalWrite(LED_BUILTIN, LOW);
            } else {
                digitalWrite(LED_BUILTIN, HIGH);
            }
        }
        if (discordSignal) {
            discordSignal = false;
            /*
            {
                "content": "⚠️Warning(yyy)⚠️\nEarthquake Detected\n",
                "tts": false,
                "embeds": [
                    {
                    "description": "",
                    "fields": [
                        {
                        "name": "📊 Peak Ground Acceleration (PGA)",
                        "value": "xxx **gal (cm/s^2)**",
                        "inline": true
                        },
                        {
                        "name": "📈 Intensity",
                        "value": "yyy (lol)",
                        "inline": false
                        }
                    ],
                    "title": "Earthquake Report",
                    "footer": {
                        "text": "report for reference only."
                    },
                    "color": 16711680
                    }
                ],
                "components": [],
                "actions": {},
                "flags": 0,
                "username": "QuakeCord_id",
                "avatar_url": "https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/src/icon.png"
            }
            */
            String intensity = toIntensity(pga);
            String msg = "{\"content\": \"⚠️Warning(" +
                intensity+
                ")⚠️\nEarthquake Detected\n\",\"tts\": false,\"embeds\": [{\"description\": \"\",\"fields\": [{\"name\": \"📊 Peak Ground Acceleration (PGA)\",\"value\": \""+ 
                pga+ 
                " **gal (cm/s^2)**\",\"inline\": true},{\"name\": \"📈 Intensity\",\"value\": \"" +
                intensity+
                " ("+
                mode+ 
                ")\",\"inline\": false}],\"title\": \"Earthquake Report\",\"footer\": {\"text\": \"report for reference only.\"},\"color\": 16711680}],\"components\": [],\"actions\": {},\"flags\": 0,\"username\": \"QuakeCord_id\",\"avatar_url\": \"https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/src/icon.png\"}";
            JsonDocument jsn;
            deserializeJson(jsn, msg);
            serializeJson(jsn, msg);
            HTTPClient sender;
            sender.begin(webhook);
            sender.addHeader("Content-Type", "application/json;");
            sender.addHeader("Connection", "keep-alive");
            // Serial.println(msg);
            int code = sender.POST(msg);
            // Serial.println(code);
        }
        delay(1);
        
    }
}

void setup() {
    mainCount = 0;
    initWire();
    Serial.begin(115200);
    while (!Serial)
        ;
    initWifi();
    registerTime();
    initDevice();
    verifyConnection();
    initDMP();
    getMode() ;
    xTaskCreatePinnedToCore(DataControl, /*任務實際對應的Function*/
                            "Task1",     /*任務名稱*/
                            10000,       /*堆疊空間*/
                            NULL,        /*無輸入值*/
                            0,           /*優先序0*/
                            &Task1,      /*對應的任務變數位址*/
                            0);          /*指定在核心0執行 */

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

    /* Read a packet from FIFO */
    if (mpu.dmpGetCurrentFIFOPacket(FIFOBuffer)) { // Get the Latest packet
        /* Display initial world-frame acceleration, adjusted to remove gravity
        and rotated based on known orientation from Quaternion */
        long nowMill = millis();
        long long dt = nowMill - baseSystemTime;
        dt = (dt > 0) ? dt : dt + 2147483648;
        // Serial.println(dt);

        mpu.dmpGetQuaternion(&q, FIFOBuffer);
        mpu.dmpGetAccel(&aa, FIFOBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
        // mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);

        // lax[mainCount] = aaReal.x;
        // lay[mainCount] = aaReal.y;
        // laz[mainCount] = aaReal.z;
        float galx = aaReal.x * dataRatio;
        float galy = aaReal.y * dataRatio;
        float galz = aaReal.z * dataRatio;
        Serial.print("x:");
        Serial.print(galx);
        Serial.print(",y:");
        Serial.print(galy);
        Serial.print(",z:");
        Serial.print(galz);
        Serial.print(",rx:");
        Serial.print(aaReal.x);
        Serial.print(",ry:");
        Serial.print(aaReal.y);
        Serial.print(",rz:");
        Serial.print(aaReal.z);
        Serial.print(",interest:");
        Serial.print(interest);
        Serial.print(",discord:");
        Serial.println(discordSignal);
        JsonObject appendData = buildData["data"].add<JsonObject>();
        appendData["dt"] = nowMill;
        appendData["x"] = aaReal.x;
        appendData["y"] = aaReal.y;
        appendData["z"] = aaReal.z;
        if (abs(galx) >= threshold || abs(galy) >= threshold || abs(galz) >= threshold) {
            interest += 1;
            if (abs(pga - max(pga, max(abs(galx), max(abs(galy), abs(galz))))) > 10) {
                discordLock = false;
            }
            pga = max(pga, max(abs(galx), max(abs(galy), abs(galz))));
        }else if (interest > 0) {
            interest -= 1;
        }
        if (interest > 5 && !discordLock) {
            discordSignal = true;
            discordLock = true;
        }
        if (interest == 0 && discordLock) {
            discordLock = false;
            pga = 0;
        }

        if (dt > 10000) {
            baseSystemTime = millis();
            if (buildData.size() > 0) {
                sendData = buildData;
                sendSignal = true;
            }
            buildData.clear();
        }
        delay(32);
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

void registerTime() {
    JsonDocument startSignal;
    startSignal["id"] = StationID;
    String ss;
    serializeJson(startSignal, ss);
    timer.begin(registerServer);
    timer.POST(ss);
    timer.end();
}

/*-----Calibrateing-----*/
void calibrate() {
    Initialize(); // Initializate and calibrate the sensor
    for (i = iAx; i <= iGz; i++) {
        Target[i] = 0; // Fix for ZAccel
        HighOffset[i] = 0;
        LowOffset[i] = 0;
    }
    Target[iAz] = 0;     // Set the taget for Z axes, original: 16384
    SetAveraging(NFast); // Fast averaging
    PullBracketsOut();
    PullBracketsIn();
    Serial.println("-------------- DONE --------------");
}

/*Initializate function*/
void Initialize() {
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    Wire.begin();
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
    Fastwire::setup(400, true);
#endif
    // Check module connection
    Serial.println("Testing device connections...");
    if (mpu.testConnection() == false) {
        Serial.println("MPU6050 connection failed");
        while (true)
            ;
    } else {
        Serial.println("MPU6050 connection successful");
    }

    Serial.println("\nPID tuning Each Dot = 100 readings");
    /*
      PID tuning (actually PI) works like this: changing the offset in the
      MPU6050 gives instant results, allowing us to use the Proportional and
      Integral parts of the PID to find the ideal offsets. The Integral uses the
      error from the set point (which is zero) and adds a fraction of this error
      to the integral value. Each reading reduces the error towards the desired
      offset. The greater the error, the more we adjust the integral value.

      The Proportional part helps by filtering out noise from the integral
      calculation. The Derivative part is not used due to noise and the sensor
      being stationary. With the noise removed, the integral value stabilizes
      after about 600 readings. At the end of each set of 100 readings, the
      integral value is used for the actual offsets, and the last proportional
      reading is ignored because it reacts to any noise.
      */
    Serial.println("\nXAccel\t\tYAccel\t\tZAccel\t\tXGyro\t\tYGyro\t\tZGyro");
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
    Serial.println("\n600 Readings");
    mpu.PrintActiveOffsets();
    mpu.CalibrateAccel(1);
    mpu.CalibrateGyro(1);
    Serial.println("700 Total Readings");
    mpu.PrintActiveOffsets();
    mpu.CalibrateAccel(1);
    mpu.CalibrateGyro(1);
    Serial.println("800 Total Readings");
    mpu.PrintActiveOffsets();
    mpu.CalibrateAccel(1);
    mpu.CalibrateGyro(1);
    Serial.println("900 Total Readings");
    mpu.PrintActiveOffsets();
    mpu.CalibrateAccel(1);
    mpu.CalibrateGyro(1);
    Serial.println("1000 Total Readings");
    mpu.PrintActiveOffsets();
    Serial.println("\nAny of the above offsets will work nicely \n\nProving "
                   "the PID with other method:");
}

void SetAveraging(int NewN) {
    N = NewN;
    Serial.print("\nAveraging ");
    Serial.print(N);
    Serial.println(" readings each time");
}

void PullBracketsOut() {
    boolean Done = false;
    int NextLowOffset[6];
    int NextHighOffset[6];

    Serial.println("Expanding:");
    ForceHeader();

    while (!Done) {
        Done = true;
        SetOffsets(LowOffset); // Set low offsets
        GetSmoothed();
        for (i = 0; i <= 5; i++) { // Get low values
            LowValue[i] = Smoothed[i];
            if (LowValue[i] >= Target[i]) {
                Done = false;
                NextLowOffset[i] = LowOffset[i] - 1000;
            } else {
                NextLowOffset[i] = LowOffset[i];
            }
        }
        SetOffsets(HighOffset);
        GetSmoothed();
        for (i = 0; i <= 5; i++) { // Get high values
            HighValue[i] = Smoothed[i];
            if (HighValue[i] <= Target[i]) {
                Done = false;
                NextHighOffset[i] = HighOffset[i] + 1000;
            } else {
                NextHighOffset[i] = HighOffset[i];
            }
        }
        ShowProgress();
        for (int i = 0; i <= 5; i++) {
            LowOffset[i] = NextLowOffset[i];
            HighOffset[i] = NextHighOffset[i];
        }
    }
}

void PullBracketsIn() {
    boolean AllBracketsNarrow;
    boolean StillWorking;
    int NewOffset[6];

    Serial.println("\nClosing in:");
    AllBracketsNarrow = false;
    ForceHeader();
    StillWorking = true;
    while (StillWorking) {
        StillWorking = false;
        if (AllBracketsNarrow && (N == NFast)) {
            SetAveraging(NSlow);
        } else {
            AllBracketsNarrow = true;
        }
        for (int i = 0; i <= 5; i++) {
            if (HighOffset[i] <= (LowOffset[i] + 1)) {
                NewOffset[i] = LowOffset[i];
            } else { // Binary search
                StillWorking = true;
                NewOffset[i] = (LowOffset[i] + HighOffset[i]) / 2;
                if (HighOffset[i] > (LowOffset[i] + 10)) {
                    AllBracketsNarrow = false;
                }
            }
        }
        SetOffsets(NewOffset);
        GetSmoothed();
        for (i = 0; i <= 5; i++) {         // Closing in
            if (Smoothed[i] > Target[i]) { // Use lower half
                HighOffset[i] = NewOffset[i];
                HighValue[i] = Smoothed[i];
            } else { // Use upper half
                LowOffset[i] = NewOffset[i];
                LowValue[i] = Smoothed[i];
            }
        }
        ShowProgress();
    }
}

void ForceHeader() { LinesOut = 99; }

/*Function to smooth the read values*/
void GetSmoothed() {
    int16_t RawValue[6];
    long Sums[6];
    for (i = 0; i <= 5; i++) {
        Sums[i] = 0;
    }

    /* Get Sums*/
    for (i = 1; i <= N; i++) {
        mpu.getMotion6(&RawValue[iAx], &RawValue[iAy], &RawValue[iAz], &RawValue[iGx], &RawValue[iGy], &RawValue[iGz]);
        delayMicroseconds(usDelay);
        for (int j = 0; j <= 5; j++) {
            Sums[j] = Sums[j] + RawValue[j];
        }
    }
    for (i = 0; i <= 5; i++) {
        Smoothed[i] = (Sums[i] + N / 2) / N;
    }
}

/*Function for configure the oba=tained offsets*/
void SetOffsets(int TheOffsets[6]) {
    mpu.setXAccelOffset(TheOffsets[iAx]);
    mpu.setYAccelOffset(TheOffsets[iAy]);
    mpu.setZAccelOffset(TheOffsets[iAz]);
    mpu.setXGyroOffset(TheOffsets[iGx]);
    mpu.setYGyroOffset(TheOffsets[iGy]);
    mpu.setZGyroOffset(TheOffsets[iGz]);
}

/*Print the progress of the reading averages, add formatting for better
 * visualization*/
void ShowProgress() {
    /*Header*/
    if (LinesOut >= LinesBetweenHeaders) {
        Serial.println("\t\tXAccel\t\t\tYAccel\t\t\t\tZAccel\t\t\tXGyro\t\t\tYG"
                       "yro\t\t\tZGyro");
        LinesOut = 0;
    }
    Serial.print(' ');
    for (i = 0; i <= 5; i++) {
        Serial.print('[');
        Serial.print(LowOffset[i]), Serial.print(',');
        Serial.print(HighOffset[i]);
        Serial.print("] --> [");
        Serial.print(LowValue[i]);
        Serial.print(',');
        Serial.print(HighValue[i]);
        if (i == 5) {
            Serial.println("]");
        } else {
            Serial.print("]\t");
        }
    }
    LinesOut++;
}

void getMode() {
    #ifdef MMI; // Global (Mercalli intensity scale)
        mode = "MMI";
    #endif

    #ifdef CWASIS ; // Taiwan
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
    int arraySum = 0;
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