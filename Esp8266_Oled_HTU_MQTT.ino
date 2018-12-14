#include <SparkFunHTU21D.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Credentials\Credentials.h>


const char* mqtt_server = SERVER_IP;
WiFiClient espClient;
unsigned long reconnectionPeriod = 10000; //miliseconds
unsigned long lastBrokerConnectionAttempt = 0;
unsigned long lastWifiConnectionAttempt = 0;

PubSubClient client(espClient);
long lastTempMsg = 0;
char msg[50];


String stringTemperature = "-";
String stringHumidity = "-";

//Create an instance of the sensor object
HTU21D sensor;
float humidity = 0.0;
float temperature = 0.0;
int sensorRequestPeriod = 10; // seconds
unsigned long lastGetSensorData = 0;

// The setup() function runs once each time the micro-controller starts
void setup()
{
	Serial.begin(115200);
	WiFi.mode(WIFI_STA);
	client.setServer(mqtt_server, 1883);
	client.setCallback(callback);
	setup_wifi();
	sensor.begin();
}

void setup_wifi() {

	delay(500);
	// We start by connecting to a WiFi network
	Serial.print(F("Connecting to "));
	Serial.println(SSID);
	WiFi.begin(SSID, PASSWORD);
	delay(3000);

	if (WiFi.waitForConnectResult() != WL_CONNECTED) {
		Serial.println(F("Connection Failed!"));
		return;
	}
	connectToBroker();
}

void callback(char* topic, byte* payload, unsigned int length) {
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
	}
	Serial.println("");

	if (strcmp(topic, "HTU21D/sensorRequestPeriod") == 0) {
		String myString = String((char*)payload);
		Serial.println(myString);
		sensorRequestPeriod = myString.toInt();
		Serial.print("Sensor request period set to :");
		Serial.print(sensorRequestPeriod);
		Serial.println(" s");
	}
}

//Connection to MQTT broker
void connectToBroker() {
	Serial.print("Attempting MQTT connection...");
	// Attempt to connect
	if (client.connect("HTU21D")) {
		Serial.println("connected");
		// Once connected, publish an announcement...
		client.publish("HTU21D/status", "Wemos D1 mini with HTU21D connected");
		// ... and resubscribe
		client.subscribe("HTU21D/sensorRequestPeriod");
	}
	else {
		Serial.print("failed, rc=");
		Serial.print(client.state());
		Serial.print(" try again in ");
		Serial.print(reconnectionPeriod / 1000);
		Serial.println(" seconds");
	}
}

void reconnectToBroker() {
	long now = millis();
	if (now - lastBrokerConnectionAttempt > reconnectionPeriod) {
		lastBrokerConnectionAttempt = now;
		{
			if (WiFi.status() == WL_CONNECTED)
			{
				if (!client.connected()) {
					connectToBroker();
				}
			}
			else
			{
				reconnectWifi();
			}
		}
	}
}

void reconnectWifi() {
	long now = millis();
	if (now - lastWifiConnectionAttempt > reconnectionPeriod) {
		lastWifiConnectionAttempt = now;
		setup_wifi();
	}
}

// Add the main program code into the continuous loop() function
void loop()
{
		if (!client.connected()) {
			reconnectToBroker();
		}
		client.loop();
		sendMessageToMqttInLoop();
}

	void sendMessageToMqttInLoop() {
		long now = millis();
		if (now - lastTempMsg > sensorRequestPeriod) {
			lastTempMsg = now;
			getSensorData();
			sendMessageToMqtt();
		}
	}

void getSensorData()
{
	long now = millis();
	if (now - lastGetSensorData > sensorRequestPeriod * 1000) {
		lastGetSensorData = now;
		humidity = sensor.readHumidity();
		temperature = sensor.readTemperature();
		stringHumidity = String(humidity, 1);
		stringTemperature = String(temperature, 1);
		Serial.print("Time:");
		Serial.print(millis());
		Serial.print(" Temperature:");
		Serial.print(stringTemperature);
		Serial.print("C");
		Serial.print(" Humidity:");
		Serial.print(stringHumidity);
		Serial.print("%");
		Serial.println();
	}
}

void sendMessageToMqtt() {
	client.publish("HTU/humidity", stringHumidity.c_str());
	client.publish("HTU/temperature", stringTemperature.c_str());
}

