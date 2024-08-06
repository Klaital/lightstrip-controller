#include <Arduino.h>
#include <WiFiNINA.h>

// #define DEBUG 1

#include <Lightstrip.h>
#include <webserver.h>

char ssid[] = "WANNET";
char pass[] = "eatmithkabobs";
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

void enable_WiFi() {
    // check for the WiFi module:
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        // don't continue
        while (true);
    }

    String fv = WiFi.firmwareVersion();
    if (fv < "1.0.0") {
        Serial.println("Please upgrade the firmware");
    }
}
void connect_WiFi() {
    // attempt to connect to Wifi network:
    while (wifiStatus != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        wifiStatus = WiFi.begin(ssid, pass);
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
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
    Serial.print("IP: ");
    Serial.println(ip);
}

int HandleLightsOn(const Request& req, Response& resp) {
    Serial.println("HandleLightsOn called");
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
    Serial.println("HandleLightsOff called");
    lights.set_power(WarmWhite, 0.0f);
    lights.drive();
    resp.code = 204;
    strcpy(resp.status, "No Content");
    return 1;
}

int HandleLightsDim(const Request& req, Response& resp) {
    Serial.println("HandleLightsDim called");
    resp.code = 204;
    strcpy(resp.status, "No Content");

    lights.set_power(WarmWhite, 0.1f);
    lights.drive();

    return 1;
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


    enable_WiFi();
    connect_WiFi();
#ifdef DEBUG
    while(!Serial);
#endif
    printWifiStatus();

    // configure http server
    web.register_handler("PUT", "/lights/?state=on", &HandleLightsOn);
    web.register_handler("PUT", "/lights/?state=off", &HandleLightsOff);
    web.begin();
}

void loop() {
// write your code here
    client = server.available();
    Response resp;
    if (client) {
        if (web.listen_once(client, resp)) {
            resp.write(client);
        }
        client.stop();
    }

}