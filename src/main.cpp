#include <Arduino.h>
#include <WiFiNINA.h>

// #define DEBUG 1

#include <Dimmer.h>
#include <Lightstrip.h>
#include <webserver.h>

#include "secrets.h"
#include "config.h"
char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASS;
int wifiStatus = WL_IDLE_STATUS;
WiFiServer server(80);
Webserver web(&server);
WiFiClient client = server.available();

constexpr pin_size_t LED_PIN_RED = 2;
constexpr pin_size_t LED_PIN_GREEN = 3;
constexpr pin_size_t LED_PIN_WHITE = 4;
constexpr pin_size_t LED_PIN_BLUE = 5;
pin_size_t led_pins[4] = {LED_PIN_RED, LED_PIN_GREEN, LED_PIN_WHITE, LED_PIN_BLUE};
Lightstrip lights(led_pins);

unsigned long wakeup_start_time = 0;
unsigned long wakeup_last_update = 0;
constexpr float wakeup_duration_seconds = 1800.0;
constexpr unsigned long wakeup_update_interval = 10; // seconds
bool wakeup_in_progress = false;

constexpr pin_size_t BUTTON_PIN = 7;
unsigned long button_last_press = 0;
PinStatus button_previous_state = LOW;
constexpr unsigned long button_debounce_interval = 250;


// MQTT setup
#include <ArduinoMqttClient.h>
constexpr char mqttBrokerHost[] = MQTT_BROKER_HOST;
WiFiClient mqttWifi;
MqttClient mqttClient(mqttWifi);

void enable_WiFi() {
    // check for the WiFi module:
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        // don't continue
        while (true)
            ;
    }

    const String fv = WiFi.firmwareVersion();
    if (fv < "1.0.0") {
        Serial.println("Please upgrade the firmware");
    }

    WiFi.setHostname(HOSTNAME);
}
void connect_WiFi() {
    // attempt to connect to Wifi network:
    while (wifiStatus != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.print(ssid);
        Serial.print(" with password ");
        Serial.println(pass);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        wifiStatus = WiFi.begin(ssid, pass);
        Serial.print("WiFi Status: ");
        Serial.println(wifiStatus);
        // wait 10 seconds for connection:
        delay(5000);
    }
}

void printWifiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    const long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
    Serial.print("IP: ");
    Serial.println(ip);
}

int HandleLightsOn(const Request& req, Response& resp) {
    wakeup_in_progress = false;
    resp.code = 204;
    strcpy(resp.status, "No Content");

    float dimmer = 1.0;
    if (req.body.length() > 0) {
        dimmer = req.body.toFloat();
        Serial.print("New dimmer setting: ");
        Serial.println(dimmer);
    }
    lights.set_power(WarmWhite, dimmer);
    lights.drive();

    return 1;
}

int HandleLightsOff(const Request& req, Response& resp) {
    wakeup_in_progress = false;
    lights.set_power(WarmWhite, 0.0f);
    lights.drive();
    resp.code = 204;
    strcpy(resp.status, "No Content");
    return 1;
}

// helper function that configures the variables needed to enable gradual wakeup dimmer mode.
void start_wakeup() {
    wakeup_in_progress = true;
    wakeup_start_time = WiFi.getTime();
    wakeup_last_update = wakeup_start_time;
}

// helper function to break out of gradual wakeup mode
void halt_wakeup() {
    wakeup_in_progress = false;
}


int HandleLightsWakeup(const Request& req, Response& resp) {
    Serial.println("HandleLightsWakeup called");
    resp.code = 204;
    strcpy(resp.status, "No Content");

    // TODO: we could use the request body to specify the duration of the wakeup routine.
    lights.set_power(WarmWhite, 0.0f);
    lights.drive();

    start_wakeup();

    return 1;
}

constexpr pin_size_t dimmer_pin = A1;
Dimmer dimmer(dimmer_pin, 12, 4095, 500);

void HandleDimmerChange(const float dimmer_read) {
#ifdef DEBUG
     Serial.println(dimmer_read);
#endif
    wakeup_in_progress = false; // abort wakeup routine if it was in progress
    lights.set_power(WarmWhite, dimmer_read);
    lights.drive();
}

void setup() {
// write your initialization code here
    Serial.begin(9600);
    // Initialize the LED driver pins
    pinMode(LED_PIN_RED, OUTPUT);
    digitalWrite(LED_PIN_RED, LOW);
    pinMode(LED_PIN_GREEN, OUTPUT);
    digitalWrite(LED_PIN_GREEN, LOW);
    pinMode(LED_PIN_WHITE, OUTPUT);
    digitalWrite(LED_PIN_WHITE, LOW);
    pinMode(LED_PIN_BLUE, OUTPUT);
    digitalWrite(LED_PIN_BLUE, LOW);

    // Initialize the button sensor
    pinMode(BUTTON_PIN, INPUT_PULLDOWN);


#ifdef DEBUG
    while(!Serial)
        ; //no-op, just wait here until someone connects to the serial port
#endif

    enable_WiFi();
    connect_WiFi();
    printWifiStatus();

    // configure http server
    web.register_handler("PUT", "/lights/?state=on", &HandleLightsOn);
    web.register_handler("PUT", "/lights/?state=off", &HandleLightsOff);
    web.register_handler("PUT", "/lights/wakeup", &HandleLightsWakeup);
    web.begin();

    // configure the Dimmer sensor
    dimmer.register_change_handler(&HandleDimmerChange);
    dimmer.begin();

    // Configure the MQTT listener
    mqttClient.setId("bedroom-lights");
    Serial.println("Connecting to MQTT broker");
    if (!mqttClient.connect(mqttBrokerHost, MQTT_BROKER_PORT)) {
        Serial.print("MQTT connection failed. Error = ");
        Serial.println(mqttClient.connectError());
        while(true); // halt here
    }

    Serial.println("Connection successful");
    Serial.print("Subscribing to light control topic: ");
    Serial.println(MQTT_TOPIC_DIMMER);

    mqttClient.subscribe(MQTT_TOPIC_DIMMER);
}

void loop() {

    // listen for http commands
    client = server.available();
    if (client) {
        Response resp;
        if (web.listen_once(client, resp)) {
            resp.write(client);
        }
        client.stop();
    }

    // listen for mqtt commands
    const int messageSize = mqttClient.parseMessage();
    if (messageSize) {
        // message recieved - print the contents
        const String msg = mqttClient.readString();
        Serial.println(msg);
        const float dimmerVal = msg.toFloat();
        if (dimmerVal >= 0.0f && dimmerVal <= 1.0f) {
            wakeup_in_progress = false;
            lights.set_power(WarmWhite, dimmerVal);
            lights.drive();
        }
    }

    // check the dimmer for a light override
    dimmer.poll();

    // check the button for a light override
    const PinStatus button_state = digitalRead(BUTTON_PIN);
    if (button_state == HIGH && button_previous_state == LOW) {
        const unsigned long now = millis();
        if (now - button_last_press > button_debounce_interval) {
            // when the button is pressed, toggle the lights.
            wakeup_in_progress = false;
            lights.toggle_power();
            lights.drive();
            button_last_press = now;
        }
    }
    button_previous_state = button_state;

    // run the gentle wakeup routine
    if (wakeup_in_progress) {
        // how long since the wakeup alarm triggered, in seconds
        const unsigned long current_time = WiFi.getTime();
        const auto elapsed_seconds = static_cast<float>(current_time - wakeup_start_time);
        const unsigned long elapsed_since_last_update = current_time - wakeup_last_update;
        if (elapsed_seconds > wakeup_duration_seconds) {
            lights.set_power(WarmWhite, 1.0);
            lights.drive();
            wakeup_in_progress = false;
        } else if (elapsed_since_last_update > wakeup_update_interval) {
            // drive the lights slightly brighter based on
            // the elapsed time since the wakeup call began.
            const float dimmer = static_cast<float>(elapsed_seconds) / wakeup_duration_seconds;
            lights.set_power(WarmWhite, dimmer);
            lights.drive();
            wakeup_last_update = current_time;
        }
    }
}
