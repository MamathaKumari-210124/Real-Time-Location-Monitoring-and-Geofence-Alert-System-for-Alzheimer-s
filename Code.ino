#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// ================== GPS CONFIG ==================
HardwareSerial gpsSerial(2);
#define GPS_RX 16
#define GPS_TX 17
#define GPS_BAUD 9600

TinyGPSPlus gps;

// ================== WIFI ==================
const char* ssid = "Redmi Note 14 5G";
const char* password = "jyothi33";

// ================== API ==================
const char* apiKey = "cd_23w_260326_GpyfB3";
const char* templateID = "110";
const char* mobileNumber = "6303570269";

// ================== GEOFENCE ==================
const float homeLat = 17.526346;
const float homeLon = 78.366180;
bool alertSent = false;

// ================== DISTANCE FUNCTION ==================
double distanceBetween(double lat1, double lon1, double lat2, double lon2) {
  double R = 6371000;
  double dLat = radians(lat2 - lat1);
  double dLon = radians(lon2 - lon1);

  double a = sin(dLat/2) * sin(dLat/2) +
             cos(radians(lat1)) * cos(radians(lat2)) *
             sin(dLon/2) * sin(dLon/2);

  double c = 2 * atan2(sqrt(a), sqrt(1-a));
  return R * c;
}

// ================== SEND SMS ==================
void sendSMS(float lat, float lon) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  String url = "https://www.circuitdigest.cloud/api/v1/send_sms?ID=" + String(templateID);

  http.begin(client, url);
  http.addHeader("Authorization", apiKey);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"mobiles\":\"" + String(mobileNumber) +
                   "\",\"var1\":\"ESP32\",\"var2\":\"" +
                   String(lat,6) + "," + String(lon,6) + "\"}";

  int code = http.POST(payload);

  Serial.print("SMS Status: ");
  Serial.println(code);

  http.end();
}

// ================== SEND TO CLOUD ==================
void sendToCloud(float lat, float lon) {

  HTTPClient http;

  String url = "https://www.circuitdigest.cloud/api/v1/update?api_key=" 
               + String(apiKey) + 
               "&lat=" + String(lat,6) + 
               "&lon=" + String(lon,6);

  http.begin(url);

  int code = http.GET();

  Serial.print("Cloud Status: ");
  Serial.println(code);

  http.end();
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);

  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected ✅");
}

// ================== LOOP ==================
void loop() {

  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isUpdated()) {

    float lat = gps.location.lat();
    float lon = gps.location.lng();

    Serial.print("Lat: ");
    Serial.println(lat,6);

    Serial.print("Lon: ");
    Serial.println(lon,6);

    // Send to cloud
    sendToCloud(lat, lon);

    // Geofence
    double dist = distanceBetween(homeLat, homeLon, lat, lon);

    Serial.print("Distance: ");
    Serial.println(dist);

    if (dist > 50 && !alertSent) {
      sendSMS(lat, lon);
      alertSent = true;
    }

    if (dist <= 50 && alertSent) {
      alertSent = false;
    }
  } else {
    Serial.println("Waiting for GPS...");
  }

  delay(2000);
}