#define BLYNK_TEMPLATE_ID "TMPL6nDUbZAu1"
#define BLYNK_TEMPLATE_NAME "Plant Monitoring System"
#define BLYNK_AUTH_TOKEN "EF46phJPjiLPc9ofKiCK3pvwQGBzzaIe"

#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT11.h>

//---------------- WIFI ----------------
char ssid[] = "sim";
char pass[] = "12345678";

//---------------- DHT -----------------
#define DHTPIN D4
DHT11 dht(DHTPIN);

//---------------- PINS ----------------
#define PIR_PIN    D5
#define RELAY_PIN  D6
#define GREEN_LED  D0
#define RED_LED    D3
#define SOIL_PIN   A0

BlynkTimer timer;

//---------------- ALERT FLAGS ----------------
bool motionSent = false;
bool soilSent   = false;
bool tempSent   = false;

//==================================================
// INVERTED RELAY FIX
//==================================================
void pumpOn()
{
  digitalWrite(RELAY_PIN, LOW);
}

void pumpOff()
{
  digitalWrite(RELAY_PIN, HIGH);
}

//==================================================

void sendData()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h)) h = 0;
  if (isnan(t)) t = 0;

  int raw = analogRead(SOIL_PIN);

  int moisture = map(raw, 1023, 250, 0, 100);

  if (moisture < 0) moisture = 0;
  if (moisture > 100) moisture = 100;

  int pir = digitalRead(PIR_PIN);

  String motionStatus = "NO MOTION";
  String plantStatus  = "HEALTHY";
  String pumpStatus   = "OFF";

  //------------------------------------------------
  // MOTION LOGIC + EVENT
  //------------------------------------------------
  if (pir == HIGH)
  {
    motionStatus = "VISITOR DETECTED";

    if (!motionSent)
    {
      Blynk.logEvent("motion_alert", "🚶 Motion detected near plant!");
      motionSent = true;
    }
  }
  else
  {
    motionStatus = "NO MOTION";
    motionSent = false;
  }

  //------------------------------------------------
  // TEMPERATURE ALERT
  //------------------------------------------------
  if (t > 35)
  {
    if (!tempSent)
    {
      Blynk.logEvent("temp_alert", "🌡️ High temperature detected!");
      tempSent = true;
    }
  }
  else
  {
    tempSent = false;
  }

  //------------------------------------------------
  // SOIL + PUMP LOGIC + EVENT
  //------------------------------------------------
  if (moisture < 25)
  {
    plantStatus = "SUPER DRY";
    pumpStatus  = "ON";

    pumpOn();

    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);

    if (!soilSent)
    {
      Blynk.logEvent("soil_alert", "🌱 Soil is too dry! Watering started.");
      soilSent = true;
    }
  }
  else if (moisture < 50)
  {
    plantStatus = "LOW WATER";
    pumpStatus  = "OFF";

    pumpOff();

    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);

    soilSent = false;
  }
  else
  {
    plantStatus = "HEALTHY";
    pumpStatus  = "OFF";

    pumpOff();

    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);

    soilSent = false;
  }

  //------------------------------------------------
  // BLYNK DATA SEND
  //------------------------------------------------
  Blynk.virtualWrite(V0, t);
  Blynk.virtualWrite(V1, h);
  Blynk.virtualWrite(V2, moisture);
  Blynk.virtualWrite(V3, plantStatus);
  Blynk.virtualWrite(V4, motionStatus);
  Blynk.virtualWrite(V5, pumpStatus);

  //------------------------------------------------
  // SERIAL MONITOR
  //------------------------------------------------
  Serial.print("Temp: ");
  Serial.print(t);
  Serial.print("C  Hum: ");
  Serial.print(h);

  Serial.print("  Soil: ");
  Serial.print(moisture);
  Serial.print("%  Plant: ");
  Serial.print(plantStatus);

  Serial.print("  Pump: ");
  Serial.print(pumpStatus);

  Serial.print("  Motion: ");
  Serial.println(motionStatus);
}

//==================================================

void setup()
{
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  pumpOff();   // ensure pump OFF at start

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Initial values
  Blynk.virtualWrite(V3, "STARTING");
  Blynk.virtualWrite(V4, "NO MOTION");
  Blynk.virtualWrite(V5, "OFF");

  timer.setInterval(2000L, sendData);
}

//==================================================

void loop()
{
  Blynk.run();
  timer.run();
}