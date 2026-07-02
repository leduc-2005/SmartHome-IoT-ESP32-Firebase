#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "DHT.h"

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ================= WIFI =================
#define WIFI_SSID "TEN WIFI"
#define WIFI_PASSWORD "MAT KHAU"

// ================= FIREBASE =================
#define API_KEY "AIzaSyDPCCZt7F1Ma3s7kIMfAqB8MKuuObC0IgI"
#define DATABASE_URL "https://tt-iot-610c3-default-rtdb.firebaseio.com/"

#define USER_EMAIL "FIREBASE EMAIL"
#define USER_PASSWORD "MK"

// ================= CHÂN ESP32 =================
#define DHT_PIN 4
#define DHT_TYPE DHT11

#define PIR_PIN 18
#define GAS_PIN 34

// LED 1: cảnh báo chuyển động
#define LED_ALERT_PIN 23

// LED 2: điều khiển riêng từ web
#define LED_CONTROL_PIN 22

// Nếu bạn nối LED kiểu: GPIO -> điện trở -> chân dài LED, chân ngắn LED -> GND
// thì bật là HIGH, tắt là LOW
#define LED_ON HIGH
#define LED_OFF LOW

// PIR chờ ổn định 5 giây để dễ test
const unsigned long pirWarmupTime = 5000;

// ================= OBJECT =================
DHT dht(DHT_PIN, DHT_TYPE);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ================= BIẾN CẢM BIẾN =================
float nhietDo = 0;
float doAm = 0;
int gasValue = 0;
int motionValue = 0;

float lastNhietDoSent = -1000;
float lastDoAmSent = -1000;
int lastGasSent = -1;
int lastMotionSent = -1;
int lastCanhBaoSent = -1;
int lastLedAlertSent = -1;
int lastLedControlSent = -1;

int firebaseLedControl = 0;

bool motionAlarm = false;

int ledAlertState = 0;
int ledControlState = 0;

// ================= TIMER =================
unsigned long startTime = 0;
unsigned long lastSensorTime = 0;
unsigned long lastFirebaseReadTime = 0;
unsigned long lastForceSendTime = 0;

const unsigned long sensorInterval = 1000;
const unsigned long firebaseReadInterval = 500;
const unsigned long forceSendInterval = 10000;

void setup() {
    Serial.begin(115200);

    pinMode(PIR_PIN, INPUT);
    pinMode(LED_ALERT_PIN, OUTPUT);
    pinMode(LED_CONTROL_PIN, OUTPUT);

    digitalWrite(LED_ALERT_PIN, LED_OFF);
    digitalWrite(LED_CONTROL_PIN, LED_OFF);

    testLed();

    analogReadResolution(12);
    analogSetPinAttenuation(GAS_PIN, ADC_11db);

    dht.begin();

    startTime = millis();

    connectWiFi();
    connectFirebase();

    Serial.println("Cho PIR on dinh 5 giay dau...");
}

void loop() {
    unsigned long now = millis();

    if (now - lastSensorTime >= sensorInterval) {
        lastSensorTime = now;

        readSensors();
        checkMotionAlarm();
        handleAlertLed();
        sendDataToFirebaseIfChanged();
        printSerial();
    }

    if (now - lastFirebaseReadTime >= firebaseReadInterval) {
        lastFirebaseReadTime = now;

        readControlFromFirebase();
        handleControlLed();
    }
}

// ================= TEST LED KHI KHỞI ĐỘNG =================
void testLed() {
    for (int i = 0; i < 2; i++) {
        digitalWrite(LED_ALERT_PIN, LED_ON);
        digitalWrite(LED_CONTROL_PIN, LED_ON);
        delay(300);

        digitalWrite(LED_ALERT_PIN, LED_OFF);
        digitalWrite(LED_CONTROL_PIN, LED_OFF);
        delay(300);
    }
}

// ================= WIFI =================
void connectWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Dang ket noi WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }

    Serial.println();
    Serial.println("Da ket noi WiFi");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

// ================= FIREBASE =================
void connectFirebase() {
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;

    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    config.token_status_callback = tokenStatusCallback;

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    Serial.println("Dang ket noi Firebase bang Email/Password...");
}

// ================= ĐỌC CẢM BIẾN =================
void readSensors() {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t)) {
        nhietDo = t;
    }

    if (!isnan(h)) {
        doAm = h;
    }

    gasValue = analogRead(GAS_PIN);
    motionValue = digitalRead(PIR_PIN);
}

// ================= KIỂM TRA CHUYỂN ĐỘNG =================
void checkMotionAlarm() {
    bool pirReady = millis() - startTime >= pirWarmupTime;

    if (pirReady && motionValue == HIGH) {
        motionAlarm = true;
    } else {
        motionAlarm = false;
    }
}

// ================= LED CẢNH BÁO =================
void handleAlertLed() {
    if (motionAlarm) {
        digitalWrite(LED_ALERT_PIN, LED_ON);
        ledAlertState = 1;
    } else {
        digitalWrite(LED_ALERT_PIN, LED_OFF);
        ledAlertState = 0;
    }
}

// ================= LED ĐIỀU KHIỂN RIÊNG =================
void handleControlLed() {
    if (firebaseLedControl == 1) {
        digitalWrite(LED_CONTROL_PIN, LED_ON);
        ledControlState = 1;
    } else {
        digitalWrite(LED_CONTROL_PIN, LED_OFF);
        ledControlState = 0;
    }
}

// ================= GỬI FIREBASE =================
void sendDataToFirebaseIfChanged() {
    if (!Firebase.ready()) {
        return;
    }

    bool forceSend = millis() - lastForceSendTime >= forceSendInterval;

    bool nhietDoChanged = abs(nhietDo - lastNhietDoSent) >= 0.2;
    bool doAmChanged = abs(doAm - lastDoAmSent) >= 1.0;
    bool gasChanged = abs(gasValue - lastGasSent) >= 30;
    bool motionChanged = motionValue != lastMotionSent;
    bool canhBaoChanged = (motionAlarm ? 1 : 0) != lastCanhBaoSent;
    bool ledAlertChanged = ledAlertState != lastLedAlertSent;
    bool ledControlChanged = ledControlState != lastLedControlSent;

    if (nhietDoChanged || forceSend) {
        Firebase.RTDB.setFloat(&fbdo, "/quan1/NhietDo", nhietDo);
        lastNhietDoSent = nhietDo;
    }

    if (doAmChanged || forceSend) {
        Firebase.RTDB.setFloat(&fbdo, "/quan1/DoAm", doAm);
        lastDoAmSent = doAm;
    }

    if (gasChanged || forceSend) {
        Firebase.RTDB.setInt(&fbdo, "/quan1/KhiGas", gasValue);
        lastGasSent = gasValue;
    }

    if (motionChanged || forceSend) {
        Firebase.RTDB.setInt(&fbdo, "/quan1/ChuyenDong", motionValue);
        lastMotionSent = motionValue;
    }

    if (canhBaoChanged || forceSend) {
        Firebase.RTDB.setInt(&fbdo, "/quan1/CanhBao", motionAlarm ? 1 : 0);
        Firebase.RTDB.setInt(&fbdo, "/quan1/CanhBaoChuyenDong", motionAlarm ? 1 : 0);
        lastCanhBaoSent = motionAlarm ? 1 : 0;
    }

    Firebase.RTDB.setInt(&fbdo, "/quan1/LuongMua", 0);

    if (ledAlertChanged || forceSend) {
        Firebase.RTDB.setInt(&fbdo, "/thietbi/ledCanhBao", ledAlertState);
        lastLedAlertSent = ledAlertState;
    }

    if (ledControlChanged || forceSend) {
        Firebase.RTDB.setInt(&fbdo, "/thietbi3/den", ledControlState);
        lastLedControlSent = ledControlState;
    }

    if (forceSend) {
        lastForceSendTime = millis();
    }
}

// ================= ĐỌC LỆNH LED TỪ WEB =================
void readControlFromFirebase() {
    if (Firebase.ready()) {
        if (Firebase.RTDB.getInt(&fbdo, "/control/den")) {
            firebaseLedControl = fbdo.intData();
        } else {
            Serial.print("Loi doc /control/den: ");
            Serial.println(fbdo.errorReason());
        }
    }
}

// ================= SERIAL =================
void printSerial() {
    Serial.println("========== DATA ==========");

    Serial.print("Nhiet do: ");
    Serial.print(nhietDo);
    Serial.println(" *C");

    Serial.print("Do am: ");
    Serial.print(doAm);
    Serial.println(" %");

    Serial.print("Gas MQ-4: ");
    Serial.println(gasValue);

    Serial.print("PIR chuyen dong: ");
    Serial.println(motionValue);

    Serial.print("Canh bao chuyen dong: ");
    Serial.println(motionAlarm ? "CO" : "KHONG");

    Serial.print("LED canh bao GPIO23: ");
    Serial.println(ledAlertState ? "SANG" : "TAT");

    Serial.print("Lenh web control/den: ");
    Serial.println(firebaseLedControl);

    Serial.print("LED dieu khien GPIO22: ");
    Serial.println(ledControlState ? "SANG" : "TAT");

    Serial.println("==========================");
}