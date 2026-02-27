#include <Wire.h>
#include <U8g2lib.h>
#include <DHT.h>
#include <SoftwareSerial.h>
#include <ModbusMaster.h>

// ---------------- PIN DEFINITIONS ----------------
#define DHTPIN 2
#define PIRPIN 3
#define IN1 A0
#define IN2 A1
#define ENA 10
#define DHTTYPE DHT22

#define RX_PIN 8
#define TX_PIN 9
#define REDE_PIN 13

// ---------------- DISPLAY ----------------
// Page buffer mode for UNO (fits in 2KB SRAM)
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// ---------------- OBJECTS ----------------
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial rs485(RX_PIN, TX_PIN);
ModbusMaster node;

// ---------------- FAN CONTROL ----------------
unsigned long fanOnUntil = 0;
const unsigned long FAN_MIN_ON_MS = 120000UL; // 2 min
const fanOffDelay = 90; // delay uints

int pirIndex = fanOffDelay;
// ---------------- RS485 CONTROL ----------------
void preTransmission() { digitalWrite(REDE_PIN, HIGH); }
void postTransmission(){ digitalWrite(REDE_PIN, LOW); }

// ---------------- WIND SENSOR ----------------
float readWind() {
    uint8_t result = node.readHoldingRegisters(0x0000, 1); // address 0x0000, 1 register
    if(result == node.ku8MBSuccess){
        uint16_t raw = node.getResponseBuffer(0);
        float wind = raw / 10.0; // scale to m/s
        if(wind < 0.3) wind = 0;
        return wind;
    }
    return -1;
}

// ---------------- SETUP ----------------
void setup() {
    Serial.begin(9600);
    dht.begin();

    pinMode(PIRPIN, INPUT);
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(ENA, OUTPUT);
    pinMode(REDE_PIN, OUTPUT);
    digitalWrite(REDE_PIN, LOW);

    rs485.begin(9600);
    node.begin(1, rs485);
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);

    // Initialize display
    u8g2.begin();
    u8g2.setFont(u8g2_font_ncenB08_tr); // readable font
    u8g2.setDrawColor(1); // white text
}

// ---------------- LOOP ----------------
void loop() {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int pirState = digitalRead(PIRPIN);
    
    float wind = readWind();
    if(wind < 0) wind = 0;

    int speed = 0;
    int fanFlag = 0; // 0 = OFF, 1 = ON

    // ---- SMART FAN LOGIC ----
    float effectiveTemp = t + (0.1 * h) - (0.7 * wind);

    int baseSpeed = 0;

    // --- Comfort Based Speed ---
    if(effectiveTemp < 24){
        baseSpeed = 0;
    }
    else if(effectiveTemp < 26){
        baseSpeed = 30;
    }
    else if(effectiveTemp < 28){
        baseSpeed = 50;
    }
    else if(effectiveTemp < 30){
        baseSpeed = 70;
    }
    else{
        baseSpeed = 90;
    }

    // --- reducing the sensitivity of PIR sensor to avoid false positives ---
    if(pirState == LOW && pirIndex > 0){
        pirIndex=pirIndex-1;
    }
    else if(pirState == HIGH ){
        pirIndex=fanOffDelay;
    }

    // --- Presence Adjustment ---
    if(pirIndex == 0){
        baseSpeed = 0;  
    }

    // --- Wind Reduction Logic ---
    if(wind >= 5){
        baseSpeed = 0;
    }
    else if(wind >= 3){
        baseSpeed *= 0.6;
    }
    else if(wind >= 1){
        baseSpeed *= 0.8;
    }

    speed = constrain(baseSpeed, 0, 100);

    fanFlag = (speed > 0) ? 1 : 0;


    // ---- MOTOR CONTROL ----
    if(speed > 0){
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        analogWrite(ENA, speed * 2.55);
    } else {
        analogWrite(ENA, 0);
    }

    // ---- DISPLAY ON GME12864 ----
    u8g2.firstPage();
    do {
        // Line 1: Temperature and Humidity
        u8g2.setCursor(0, 16);
        u8g2.print("T: "); u8g2.print(t); u8g2.print("C");

        u8g2.setCursor(80, 16);
        u8g2.print("H: "); u8g2.print(h);

        // Line 2: Wind and PIR
        u8g2.setCursor(0, 32);
        u8g2.print("W: "); u8g2.print(wind); u8g2.print("ms");

        u8g2.setCursor(80, 32);
        u8g2.print("PIR: "); u8g2.print(pirState);

        // Line 3: Fan flag and speed
        u8g2.setCursor(0, 48);
        u8g2.print("F: "); u8g2.print(fanFlag);

        u8g2.setCursor(80, 48);
        u8g2.print("Spd: "); u8g2.print(speed);

    } while(u8g2.nextPage());

    // ---- SERIAL DEBUG ----
    //Serial.print("Temp: "); Serial.print(t);
    //Serial.print(" Hum: "); Serial.print(h);
    //Serial.print(" Wind: "); Serial.print(wind);
    //Serial.print(" PIR: "); Serial.print(pirState);
    //Serial.print(" Fan: "); Serial.print(fanFlag);
    //Serial.print(" Speed: "); Serial.println(speed);

    // Output data to the serial monitor
    // Temp: 25.30 Hum: 59.70 Wind: 0.00 PIR: 0 Fan: 1 Speed: 50
    Serial.print(t);
    Serial.print(",");
    Serial.print(h);
    Serial.print(",");  
    Serial.print(wind);
    Serial.print(",");  
    Serial.print(pirState);
    Serial.print(",");  
    Serial.print(fanFlag);
    Serial.print(",");  
    Serial.println(speed);

    delay(1000);
}
