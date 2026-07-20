#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "Audio.h"

const char* ssid = "ASUS";
const char* password = "1135432906";

#define I2S_DOUT  25
#define I2S_BCLK  27
#define I2S_LRC   26

Audio audio;
WebServer server(80);

int currentVolume = 12;
String currentStation = "";
bool isPlaying = false;

struct Station { const char* name; const char* url; };
Station stations[] = {
  {"Groove Salad", "http://ice1.somafm.com/groovesalad-128-mp3"},
  {"Drone Zone",   "http://ice1.somafm.com/dronezone-128-mp3"},
  {"Lush",         "http://ice1.somafm.com/lush-128-mp3"},
  {"DEF CON Radio","http://ice1.somafm.com/defcon-128-mp3"}
};
const int stationCount = sizeof(stations) / sizeof(stations[0]);

String stationsJson() {
  String json = "[";
  for (int i = 0; i < stationCount; i++) {
    json += "{\"name\":\"" + String(stations[i].name) + "\"}";
    if (i < stationCount - 1) json += ",";
  }
  json += "]";
  return json;
}

String htmlPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Веброзіо</title>
<style>
  body { font-family: sans-serif; background:#111; color:#eee; text-align:center; padding:20px; }
  button { padding:12px 20px; margin:6px; border:none; border-radius:8px; font-size:16px; cursor:pointer; }
  .station { background:#333; color:#fff; }
  .control { background:#0a84ff; color:#fff; width:100px; }
  input[type=range] { width:80%; }
</style>
</head>
<body>
  <h2>ESP32 Веброзіо</h2>
  <div id="stations"></div>
  <p>Гучність: <span id="volLabel">)rawliteral" + String(currentVolume) + R"rawliteral(</span></p>
  <input type="range" min="0" max="21" value=")rawliteral" + String(currentVolume) + R"rawliteral(" id="vol" oninput="setVol(this.value)">
  <br>
  <button class="control" onclick="stop()">Стоп</button>

<script>
const stations = )rawliteral" + stationsJson() + R"rawliteral(;
const stationsDiv = document.getElementById('stations');
stations.forEach((s, i) => {
  const b = document.createElement('button');
  b.className = 'station';
  b.innerText = s.name;
  b.onclick = () => playStation(i);
  stationsDiv.appendChild(b);
});

function playStation(i) {
  fetch('/play?station=' + i).then(() => location.reload());
}
function stop() {
  fetch('/stop').then(() => location.reload());
}
function setVol(v) {
  document.getElementById('volLabel').innerText = v;
  fetch('/volume?value=' + v);
}
</script>
</body>
</html>
)rawliteral";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handlePlay() {
  if (server.hasArg("station")) {
    int idx = server.arg("station").toInt();
    if (idx >= 0 && idx < stationCount) {
      audio.connecttohost(stations[idx].url);
      currentStation = stations[idx].name;
      isPlaying = true;
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleStop() {
  audio.stopSong();
  isPlaying = false;
  server.send(200, "text/plain", "OK");
}

void handleVolume() {
  if (server.hasArg("value")) {
    currentVolume = server.arg("value").toInt();
    audio.setVolume(currentVolume);
  }
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi підключено: " + WiFi.localIP().toString());

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(currentVolume);

  server.on("/", handleRoot);
  server.on("/play", handlePlay);
  server.on("/stop", handleStop);
  server.on("/volume", handleVolume);
  server.begin();
}

void loop() {
  server.handleClient();
  audio.loop();
}

void audio_info(const char *info) {
  Serial.print("info        "); Serial.println(info);
}
