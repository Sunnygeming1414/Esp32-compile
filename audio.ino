#include <Wire.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <SD.h>
#include <time.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <esp_sleep.h> 
#include <math.h> 
#include "Audio_nopsram.h"

// --- KONFIGURASI WIFI & NTP ---
const char* ssid = "NAMA_WIFI";     
const char* password = "PASSWORD_WIFI"; 
const char* ntpServer1 = "ntp.bmkg.go.id"; 
const char* ntpServer2 = "time.cloudflare.com";
const char* ntpServer3 = "time.google.com";
const long  gmtOffset_sec = 7 * 3600;   
const int   daylightOffset_sec = 0;

// --- KONFIGURASI PIN ---
#define OLED_SDA 7
#define OLED_SCL 6
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

#define SD_CS    3
#define SD_SCK   4
#define SD_MOSI  5
#define SD_MISO  10 

#define BTN1 21 
#define BTN2 20 
#define BTN3 2  

#define I2S_LRC  1
#define I2S_BCLK 0
#define I2S_DIN  8

Audio audio;
bool audioReady = false;

WebServer server(80); 

bool webUp = false;
bool webDown = false;
bool webSelect = false;
bool webBack = false; 
bool webOpPlus = false, webOpMinus = false, webOpMul = false, webOpDiv = false;
bool wirelessEnabled = false; 

// --- VARIABEL MENU ---
#define MAX_MENU 99
String menuNames[MAX_MENU];
String menuSubFiles[MAX_MENU];
int totalMenu = 0;
int menuTerpilih = 0;

#define MAX_SUB_MENU 99
String subMenuNames[MAX_SUB_MENU];
String subMenuFiles[MAX_SUB_MENU];
char subMenuTypes[MAX_SUB_MENU]; 
unsigned long subMenuDurations[MAX_SUB_MENU];
int totalSubMenu = 0;
int subMenuTerpilih = 0;
String currentSubMenuFile = "";

#define MAX_LIRIK 999
struct LirikData {
  unsigned long startMs;
  unsigned long endMs;
  String baris1;
  String baris2;
};
LirikData daftarLirik[MAX_LIRIK];
int totalLirik = 0;

// --- VARIABEL ALARM ---
#define MAX_ALARM 99
int alarmYear[MAX_ALARM];
int alarmMonth[MAX_ALARM];
int alarmDay[MAX_ALARM];
int alarmHour[MAX_ALARM];
int alarmMinute[MAX_ALARM];
bool alarmActive[MAX_ALARM];
int totalAlarm = 0;
int alarmTerpilih = 0;
bool isAlarmRinging = false;
String lastAlarmTimeString = ""; 
int alarmMenuState = 0; 
int alarmActionChoice = 0; 
int alarmDeleteChoice = 0; 

// --- VARIABEL TIMER ---
unsigned long swStartTime = 0;
unsigned long swElapsedTime = 0;
bool isTimerRunning = false;

// --- VARIABEL KALKULATOR ---
String calcNum1 = "0";
String calcNum2 = "0";
char calcOp = '+';
int calcState = 0; 

// --- VARIABEL CHECK CRYPTO ---
int cryptoCoinIdx = 0;      // 0: BTC, 1: ETH, 2: SOL
bool cryptoUseIDR = true;   // true: IDR, false: USD
String cryptoPriceStr = "-";
String cryptoStatusStr = "Siap";
bool cryptoLoaded = false;
int wifiPromptChoice = 0;   // 0: Konek, 1: Skip/Engk dulu
int wifiPromptReturnState = 0;

// --- VARIABEL CHECK SAHAM ---
int sahamIdx = 0;           // 0: BBCA, 1: BBRI, 2: TLKM
String sahamPriceStr = "-";
String sahamStatusStr = "Siap";

// --- VARIABEL JADWAL SHOLAT (KOTA SUKABUMI) ---
String sholatFajr = "--:--";
String sholatDhuhr = "--:--";
String sholatAsr = "--:--";
String sholatMaghrib = "--:--";
String sholatIsha = "--:--";
String sholatStatusStr = "Siap";
int sholatPage = 0;         // 0: Subuh/Dzuhur/Ashar, 1: Maghrib/Isya

// --- VARIABEL DINO GAME ---
int dinoY = 38; 
int dinoVy = 0;
int obsX = 128;
int gameScore = 0;
int bestDino = 0;
int groundOffset = 0;
bool gameOver = false;

// --- VARIABEL FLAPPY BIRD ---
float birdY = 30.0;
float birdVy = 0.0;
int pipeX = 128;
int pipeGapY = 28;
int pipeGapH = 22;
int flappyScore = 0;
int bestFlappy = 0;
int flappyGroundOffset = 0;
bool flappyGameOver = false;

// --- VARIABEL RAYCASTER GAME ---
float px = 1.5, py = 1.5, pa = 0.0; 
const int MAP_W = 40;
const int MAP_H = 40;
const int mazeMap[MAP_H][MAP_W] = {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
  {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1},
  {1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,0,1},
  {1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1},
  {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,0,1},
  {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// --- STATE MACHINE ---
// 14: Startup Tanya WiFi (Konek / Skip aja)
// 15: Prompt WiFi di dalam fitur (Konek / Engk dulu)
// 16: Check Crypto
// 17: Gagal WiFi saat startup (Konek lagi / Yaudah skip)
// 18: Gagal WiFi saat buka fitur (Konek lagi / Yaudah g ush)
// 19: Check Saham
// 20: Jadwal Sholat Kota Sukabumi
// 21: Flappy Bird
int appState = 0; 
int wifiMenuTerpilih = 0;
int wifiRetryChoice = 0;
uint8_t frameBuffer[1024]; 

// --- VARIABEL DETEKSI 3 TOMBOL BERSAMAAN ---
unsigned long btn1Time = 0, btn2Time = 0, btn3Time = 0;
const unsigned long MULTI_PRESS_WINDOW = 250;

void gambarMenu(int offsetX = 0);
void gambarSubMenu(int offsetX = 0);
bool konekWiFi();

// --- FUNGSI AUDIO I2S ---
void setupAudio() {
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DIN);
  audio.setVolume(12);
  audioReady = true;
}

void putarAudioScroll() {
  if (!audioReady) return;
  audio.stopSong();
  if (SD.exists("/scroll.wav")) {
    audio.connecttoFS(SD, "/scroll.wav");
  } else if (SD.exists("/scroll.mp3")) {
    audio.connecttoFS(SD, "/scroll.mp3");
  }
}

void putarAudioAnimasi(const char* animPath) {
  if (!audioReady || animPath == nullptr) return;
  audio.stopSong();
  String pathStr(animPath);
  int dotIdx = pathStr.lastIndexOf('.');
  String basePath = (dotIdx > 0) ? pathStr.substring(0, dotIdx) : pathStr;
  String wavPath = basePath + ".wav";
  String mp3Path = basePath + ".mp3";

  if (SD.exists(wavPath)) {
    audio.connecttoFS(SD, wavPath.c_str());
  } else if (SD.exists(mp3Path)) {
    audio.connecttoFS(SD, mp3Path.c_str());
  }
}

void hentikanAudio() {
  if (!audioReady) return;
  audio.stopSong();
}

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Remote</title>
  <style>
    body { font-family: Arial; text-align: center; margin-top: 10px; background-color: #222; color: white; }
    .btn { background-color: #4CAF50; border: none; color: white; padding: 12px 20px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 5px; cursor: pointer; border-radius: 8px; width: 180px; outline: none; }
    .btn:active { background-color: #3e8e41; }
    .red { background-color: #f44336; }
    .red:active { background-color: #da190b; }
    .blue { background-color: #2196F3; }
    .input-group { margin: 10px auto; background: #333; padding: 10px; border-radius: 8px; width: 250px; }
    input[type="datetime-local"] { font-size: 16px; padding: 5px; border-radius: 5px; border: none; width: 90%; }
    .op-btn { width: 40px; padding: 10px; font-size: 18px; background-color: #ff9800; }
  </style>
</head>
<body>
  <h3>Remote OLED</h3>
  <button class="btn" onclick="sendCmd('up')">ATAS / KIRI (B1)</button><br>
  <button class="btn" onclick="sendCmd('down')">BAWAH / KANAN (B2)</button><br>
  <button class="btn" onclick="sendCmd('select')">SELECT / KELUAR (B3)</button><br>
  <button class="btn red" onclick="sendCmd('back')">BACK / EXIT</button>
  
  <div class="input-group">
    <h4>Operator Kalkulator</h4>
    <button class="btn op-btn" onclick="sendCmd('op+')">+</button>
    <button class="btn op-btn" onclick="sendCmd('op-')">-</button>
    <button class="btn op-btn" onclick="sendCmd('op*')">*</button>
    <button class="btn op-btn" onclick="sendCmd('op/')">/</button>
  </div>
  
  <div class="input-group">
    <h4>Set Alarm (Tgl & Jam)</h4>
    <input type="datetime-local" id="alarmTime"><br><br>
    <button class="btn blue" onclick="addAlarm()">Tambah</button>
  </div>
  
  <script>
    function sendCmd(cmd) {
      fetch('/' + cmd).then(response => {
        let btn = event.target;
        btn.style.backgroundColor = '#555';
        setTimeout(() => { btn.style.backgroundColor = ''; }, 150);
      }).catch(err => console.error(err));
    }
    function addAlarm() {
      let time = document.getElementById('alarmTime').value;
      if(!time) return alert('Pilih tanggal dan waktu dulu!');
      fetch('/addalarm?t=' + time).then(response => {
        alert('Alarm ' + time + ' ditambahkan!');
      });
    }
  </script>
</body>
</html>
)rawliteral";

void handleRoot() { server.send(200, "text/html", htmlPage); }
void handleUp() { webUp = true; server.send(200, "text/plain", "OK"); }
void handleDown() { webDown = true; server.send(200, "text/plain", "OK"); }
void handleSelect() { webSelect = true; server.send(200, "text/plain", "OK"); }
void handleBack() { webBack = true; server.send(200, "text/plain", "OK"); }
void handleOpPlus() { webOpPlus = true; server.send(200, "text/plain", "OK"); }
void handleOpMinus() { webOpMinus = true; server.send(200, "text/plain", "OK"); }
void handleOpMul() { webOpMul = true; server.send(200, "text/plain", "OK"); }
void handleOpDiv() { webOpDiv = true; server.send(200, "text/plain", "OK"); }

void handleAddAlarm() {
  String timeStr = server.arg("t"); 
  if (timeStr.length() >= 16 && totalAlarm < MAX_ALARM) {
    int y = timeStr.substring(0, 4).toInt();
    int m = timeStr.substring(5, 7).toInt();
    int d = timeStr.substring(8, 10).toInt();
    int h = timeStr.substring(11, 13).toInt();
    int min = timeStr.substring(14, 16).toInt();
    
    alarmYear[totalAlarm] = y;
    alarmMonth[totalAlarm] = m;
    alarmDay[totalAlarm] = d;
    alarmHour[totalAlarm] = h;
    alarmMinute[totalAlarm] = min;
    alarmActive[totalAlarm] = true;
    totalAlarm++;
    
    File file = SD.open("/alarm.txt", FILE_APPEND);
    if (file) { 
      file.printf("%04d-%02d-%02d %02d:%02d,1\n", y, m, d, h, min); 
      file.close(); 
    }
  }
  server.send(200, "text/plain", "OK");
}

// --- FUNGSI BEST SCORE SD CARD ---
int bacaBestDariSD(const char* path) {
  File file = SD.open(path, FILE_READ);
  if (!file) return 0;
  String isi = file.readStringUntil('\n');
  isi.trim();
  file.close();
  return isi.toInt();
}

void simpanBestKeSD(const char* path, int val) {
  File file = SD.open(path, FILE_WRITE);
  if (file) {
    file.seek(0);
    file.println(val);
    file.close();
  }
}

// --- FUNGSI CHECK CRYPTO ---
const char* cryptoCoinId(int idx) {
  if (idx == 1) return "ethereum";
  if (idx == 2) return "solana";
  return "bitcoin";
}

const char* cryptoCoinSymbol(int idx) {
  if (idx == 1) return "ETH";
  if (idx == 2) return "SOL";
  return "BTC";
}

void ambilHargaCrypto() {
  if (WiFi.status() != WL_CONNECTED) {
    cryptoStatusStr = "WiFi Off";
    cryptoPriceStr = "-";
    return;
  }

  cryptoStatusStr = "Loading...";
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 24, "Ambil harga crypto...");
  u8g2.drawStr(0, 40, cryptoCoinSymbol(cryptoCoinIdx));
  u8g2.sendBuffer();

  const char* coinId = cryptoCoinId(cryptoCoinIdx);
  const char* vsCurr = cryptoUseIDR ? "idr" : "usd";
  String url = "https://api.coingecko.com/api/v3/simple/price?ids=" + String(coinId) + "&vs_currencies=" + String(vsCurr);

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(7000);

  if (!http.begin(client, url)) {
    cryptoStatusStr = "Gagal URL";
    cryptoPriceStr = "-";
    cryptoLoaded = true;
    return;
  }

  int code = http.GET();
  if (code == HTTP_CODE_OK) {
    String payload = http.getString();
    String key = "\"" + String(vsCurr) + "\":";
    int pos = payload.indexOf(key);
    if (pos >= 0) {
      int startNum = pos + key.length();
      int endNum = startNum;
      while (endNum < (int)payload.length()) {
        char c = payload.charAt(endNum);
        if ((c >= '0' && c <= '9') || c == '.') endNum++;
        else break;
      }
      String valStr = payload.substring(startNum, endNum);
      if (valStr.length() > 0) {
        if (cryptoUseIDR) {
          long long valInt = (long long)valStr.toDouble();
          cryptoPriceStr = "Rp " + String(valInt);
        } else {
          cryptoPriceStr = "$" + valStr;
        }
        cryptoStatusStr = "OK";
      } else {
        cryptoStatusStr = "Data kosong";
        cryptoPriceStr = "-";
      }
    } else {
      cryptoStatusStr = "Parse gagal";
      cryptoPriceStr = "-";
    }
  } else {
    cryptoStatusStr = "HTTP " + String(code);
    cryptoPriceStr = "-";
  }

  http.end();
  cryptoLoaded = true;
}

void tampilkanCrypto() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  String judul = String(cryptoCoinSymbol(cryptoCoinIdx)) + " / " + (cryptoUseIDR ? "IDR" : "USD");
  u8g2.drawStr(0, 11, judul.c_str());

  u8g2.setFont(u8g2_font_7x14B_tr);
  u8g2.drawStr(0, 32, cryptoPriceStr.c_str());

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(0, 46, ("Status: " + cryptoStatusStr).c_str());
  u8g2.drawStr(0, 58, "B1:Ganti B2:Curr B3:Ref");
  u8g2.sendBuffer();
}

// --- FUNGSI CHECK SAHAM ---
const char* sahamSymbol(int idx) {
  if (idx == 1) return "BBRI";
  if (idx == 2) return "TLKM";
  return "BBCA";
}

const char* sahamYahooTicker(int idx) {
  if (idx == 1) return "BBRI.JK";
  if (idx == 2) return "TLKM.JK";
  return "BBCA.JK";
}

void ambilHargaSaham() {
  if (WiFi.status() != WL_CONNECTED) {
    sahamStatusStr = "WiFi Off";
    sahamPriceStr = "-";
    return;
  }

  sahamStatusStr = "Loading...";
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 24, "Ambil harga saham...");
  u8g2.drawStr(0, 40, sahamSymbol(sahamIdx));
  u8g2.sendBuffer();

  String url = "https://query1.finance.yahoo.com/v8/finance/chart/" + String(sahamYahooTicker(sahamIdx)) + "?interval=1d&range=1d";

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(7000);

  if (!http.begin(client, url)) {
    sahamStatusStr = "Gagal URL";
    sahamPriceStr = "-";
    return;
  }

  int code = http.GET();
  if (code == HTTP_CODE_OK) {
    String payload = http.getString();
    String key = "\"regularMarketPrice\":";
    int pos = payload.indexOf(key);
    if (pos >= 0) {
      int startNum = pos + key.length();
      int endNum = startNum;
      while (endNum < (int)payload.length()) {
        char c = payload.charAt(endNum);
        if ((c >= '0' && c <= '9') || c == '.') endNum++;
        else break;
      }
      String valStr = payload.substring(startNum, endNum);
      if (valStr.length() > 0) {
        long valInt = (long)valStr.toDouble();
        sahamPriceStr = "Rp " + String(valInt);
        sahamStatusStr = "OK";
      } else {
        sahamStatusStr = "Data kosong";
        sahamPriceStr = "-";
      }
    } else {
      sahamStatusStr = "Parse gagal";
      sahamPriceStr = "-";
    }
  } else {
    sahamStatusStr = "HTTP " + String(code);
    sahamPriceStr = "-";
  }

  http.end();
}

void tampilkanSaham() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  String judul = String(sahamSymbol(sahamIdx)) + " (IDX)";
  u8g2.drawStr(0, 11, judul.c_str());

  u8g2.setFont(u8g2_font_7x14B_tr);
  u8g2.drawStr(0, 32, sahamPriceStr.c_str());

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(0, 46, ("Status: " + sahamStatusStr).c_str());
  u8g2.drawStr(0, 58, "B1/B2:Ganti B3:Ref");
  u8g2.sendBuffer();
}

// --- FUNGSI JADWAL SHOLAT (KOTA SUKABUMI) ---
String ambilFieldJamSholat(const String& payload, const char* fieldName) {
  String key = "\"" + String(fieldName) + "\":\"";
  int pos = payload.indexOf(key);
  if (pos < 0) return "--:--";
  int startIdx = pos + key.length();
  if (startIdx + 5 > (int)payload.length()) return "--:--";
  return payload.substring(startIdx, startIdx + 5);
}

void ambilJadwalSholatSukabumi() {
  if (WiFi.status() != WL_CONNECTED) {
    sholatStatusStr = "WiFi Off";
    return;
  }

  sholatStatusStr = "Loading...";
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 24, "Jadwal Sholat...");
  u8g2.drawStr(0, 40, "Kota Sukabumi");
  u8g2.sendBuffer();

  String url = "https://api.aladhan.com/v1/timingsByCity?city=Sukabumi&country=Indonesia&method=20";

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(7000);

  if (!http.begin(client, url)) {
    sholatStatusStr = "Gagal URL";
    return;
  }

  int code = http.GET();
  if (code == HTTP_CODE_OK) {
    String payload = http.getString();
    sholatFajr = ambilFieldJamSholat(payload, "Fajr");
    sholatDhuhr = ambilFieldJamSholat(payload, "Dhuhr");
    sholatAsr = ambilFieldJamSholat(payload, "Asr");
    sholatMaghrib = ambilFieldJamSholat(payload, "Maghrib");
    sholatIsha = ambilFieldJamSholat(payload, "Isha");
    if (sholatFajr != "--:--" && sholatDhuhr != "--:--") {
      sholatStatusStr = "OK";
    } else {
      sholatStatusStr = "Parse gagal";
    }
  } else {
    sholatStatusStr = "HTTP " + String(code);
  }

  http.end();
}

void tampilkanJadwalSholat() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 11, "Sholat Sukabumi");

  u8g2.setFont(u8g2_font_6x10_tr);
  if (sholatPage == 0) {
    u8g2.drawStr(0, 25, ("Subuh  : " + sholatFajr).c_str());
    u8g2.drawStr(0, 37, ("Dzuhur : " + sholatDhuhr).c_str());
    u8g2.drawStr(0, 49, ("Ashar  : " + sholatAsr).c_str());
  } else {
    u8g2.drawStr(0, 25, ("Maghrib: " + sholatMaghrib).c_str());
    u8g2.drawStr(0, 37, ("Isya   : " + sholatIsha).c_str());
    u8g2.drawStr(0, 49, ("Sts    : " + sholatStatusStr).c_str());
  }

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(0, 61, "B1/B2:Hal B3:Ref");
  u8g2.sendBuffer();
}

void gambarPromptWifi(const char* judul, const char* opsi1, const char* opsi2) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 11, judul);
  String l1 = (wifiPromptChoice == 0) ? ("> " + String(opsi1)) : ("  " + String(opsi1));
  String l2 = (wifiPromptChoice == 1) ? ("> " + String(opsi2)) : ("  " + String(opsi2));
  u8g2.drawStr(0, 28, l1.c_str());
  u8g2.drawStr(0, 44, l2.c_str());
  u8g2.sendBuffer();
}

// --- FUNGSI SD CARD ---
void bacaMenuDariSD() {
  File file = SD.open("/menu.txt"); 
  if (!file) return;
  totalMenu = 0;
  while (file.available() && totalMenu < MAX_MENU) {
    String baris = file.readStringUntil('\n'); 
    baris.trim();
    if (baris.length() > 0) {
      int koma = baris.indexOf(',');
      if (koma > 0) {
        menuNames[totalMenu] = baris.substring(0, koma); 
        menuNames[totalMenu].trim();
        String fileSub = baris.substring(koma + 1); 
        fileSub.trim();
        if (!fileSub.startsWith("/")) fileSub = "/" + fileSub;
        menuSubFiles[totalMenu] = fileSub; 
        totalMenu++;
      } else { 
        menuNames[totalMenu] = baris; 
        menuSubFiles[totalMenu] = ""; 
        totalMenu++; 
      }
    }
  }
  file.close();
}

void bacaSubMenuDariSD(String namaFile) {
  totalSubMenu = 0; 
  File file = SD.open(namaFile); 
  if (!file) return;
  while (file.available() && totalSubMenu < MAX_SUB_MENU) {
    String baris = file.readStringUntil('\n'); 
    baris.trim();
    if (baris.length() > 0) {
      int koma1 = baris.indexOf(',');
      if (koma1 > 0) {
        subMenuNames[totalSubMenu] = baris.substring(0, koma1); 
        subMenuNames[totalSubMenu].trim();
        String sisa = baris.substring(koma1 + 1); 
        sisa.trim();
        int koma2 = sisa.indexOf(','); 
        String namaFileAnim = "";
        if (koma2 > 0) { 
          namaFileAnim = sisa.substring(0, koma2); 
          namaFileAnim.trim(); 
        } else { 
          namaFileAnim = sisa; 
        }
        if (!namaFileAnim.startsWith("/")) namaFileAnim = "/" + namaFileAnim;
        subMenuFiles[totalSubMenu] = namaFileAnim;
        subMenuDurations[totalSubMenu] = 0;
        
        String namaLower = subMenuNames[totalSubMenu];
        namaLower.toLowerCase();
        String fileLower = namaFileAnim;
        fileLower.toLowerCase();

        if (namaFileAnim.endsWith(".txt")) { 
          subMenuTypes[totalSubMenu] = 'T'; 
        } else if (namaFileAnim.endsWith(".game")) { 
          if (fileLower.indexOf("raycast") >= 0) { 
            subMenuTypes[totalSubMenu] = 'R'; 
          } else if (fileLower.indexOf("flappy") >= 0 || fileLower.indexOf("fb") >= 0 || namaLower.indexOf("flappy") >= 0) {
            subMenuTypes[totalSubMenu] = 'F';
          } else { 
            subMenuTypes[totalSubMenu] = 'G'; 
          }
        } else if (koma2 > 0) { 
          String setelahKoma2 = sisa.substring(koma2 + 1);
          setelahKoma2.trim();
          int koma3 = setelahKoma2.indexOf(',');
          if (koma3 > 0) {
            subMenuTypes[totalSubMenu] = setelahKoma2.substring(0, koma3).charAt(0);
            String durStr = setelahKoma2.substring(koma3 + 1);
            durStr.trim();
            subMenuDurations[totalSubMenu] = (unsigned long)durStr.toInt();
          } else {
            subMenuTypes[totalSubMenu] = setelahKoma2.charAt(0);
          }
        } else { 
          subMenuTypes[totalSubMenu] = 'X'; 
        }
        totalSubMenu++;
      }
    }
  }
  file.close();
}

void bacaAlarmDariSD() {
  totalAlarm = 0; 
  File file = SD.open("/alarm.txt"); 
  if (!file) return;
  while (file.available() && totalAlarm < MAX_ALARM) {
    String baris = file.readStringUntil('\n'); 
    baris.trim();
    if (baris.length() >= 18) { 
      int koma = baris.indexOf(',');
      if (koma > 0) {
        String waktu = baris.substring(0, koma);
        int aktif = baris.substring(koma + 1).toInt();
        
        alarmYear[totalAlarm] = waktu.substring(0, 4).toInt();
        alarmMonth[totalAlarm] = waktu.substring(5, 7).toInt();
        alarmDay[totalAlarm] = waktu.substring(8, 10).toInt();
        alarmHour[totalAlarm] = waktu.substring(11, 13).toInt();
        alarmMinute[totalAlarm] = waktu.substring(14, 16).toInt();
        alarmActive[totalAlarm] = (aktif == 1);
        totalAlarm++;
      }
    }
  }
  file.close();
}

void simpanAlarmKeSD() {
  File file = SD.open("/alarm.txt", FILE_WRITE);
  if (file) { 
    file.seek(0); 
    for (int i = 0; i < totalAlarm; i++) { 
      file.printf("%04d-%02d-%02d %02d:%02d,%d\n", alarmYear[i], alarmMonth[i], alarmDay[i], alarmHour[i], alarmMinute[i], alarmActive[i] ? 1 : 0); 
    } 
    file.close(); 
  }
}

// --- IKON KUSTOM ---
void gambarIkonAM(int x, int y, bool besar) {
  if (besar) {
    u8g2.drawCircle(x, y, 8, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT | U8G2_DRAW_UPPER_LEFT);
    u8g2.drawCircle(x - 1, y, 5, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
    u8g2.drawCircle(x - 2, y + 1, 3, U8G2_DRAW_LOWER_RIGHT | U8G2_DRAW_LOWER_LEFT);
  } else {
    u8g2.drawCircle(x, y, 5, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT | U8G2_DRAW_UPPER_LEFT);
    u8g2.drawCircle(x - 1, y, 3, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
  }
}

void gambarIkonEyes(int x, int y, bool besar) {
  if (besar) {
    u8g2.drawDisc(x - 5, y - 1, 4, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
    u8g2.drawDisc(x + 5, y - 1, 4, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
  } else {
    u8g2.drawDisc(x - 3, y - 1, 2, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
    u8g2.drawDisc(x + 3, y - 1, 2, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
  }
}

void gambarIkonJamPasir(int x, int y, bool besar) {
  if (besar) {
    u8g2.drawRBox(x - 8, y - 9, 16, 3, 1);
    u8g2.drawRBox(x - 8, y + 6, 16, 3, 1);
    u8g2.drawLine(x - 6, y - 6, x + 6, y - 6);
    u8g2.drawLine(x - 6, y - 6, x, y);
    u8g2.drawLine(x + 6, y - 6, x, y);
    u8g2.drawLine(x - 6, y + 6, x + 6, y + 6);
    u8g2.drawLine(x - 6, y + 6, x, y);
    u8g2.drawLine(x + 6, y + 6, x, y);
    u8g2.drawTriangle(x - 4, y + 5, x + 4, y + 5, x, y + 1);
  } else {
    u8g2.drawHLine(x - 4, y - 4, 9);
    u8g2.drawHLine(x - 4, y + 4, 9);
    u8g2.drawLine(x - 3, y - 4, x + 3, y + 4);
    u8g2.drawLine(x + 3, y - 4, x - 3, y + 4);
  }
}

void gambarIkonLoncengAlarm(int x, int y, bool besar) {
  if (besar) {
    u8g2.drawDisc(x, y - 7, 2);
    u8g2.drawDisc(x, y - 1, 6, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
    u8g2.drawBox(x - 6, y - 1, 13, 5);
    u8g2.drawHLine(x - 8, y + 4, 17);
    u8g2.drawDisc(x, y + 7, 2, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
    u8g2.drawCircle(x, y, 9, U8G2_DRAW_UPPER_LEFT);
    u8g2.drawCircle(x, y, 9, U8G2_DRAW_UPPER_RIGHT);
  } else {
    u8g2.drawDisc(x, y - 1, 3, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
    u8g2.drawBox(x - 3, y - 1, 7, 3);
    u8g2.drawHLine(x - 4, y + 2, 9);
    u8g2.drawPixel(x, y + 4);
  }
}

void gambarIkonKalkulator(int x, int y, bool besar) {
  if (besar) {
    u8g2.drawFrame(x - 9, y - 9, 8, 8);
    u8g2.drawFrame(x + 1, y - 9, 8, 8);
    u8g2.drawFrame(x - 9, y + 1, 8, 8);
    u8g2.drawFrame(x + 1, y + 1, 8, 8);
    u8g2.drawHLine(x - 7, y - 5, 4);
    u8g2.drawLine(x + 3, y - 7, x + 7, y - 3);
    u8g2.drawLine(x + 7, y - 7, x + 3, y - 3);
    u8g2.drawHLine(x - 7, y + 5, 5);
    u8g2.drawVLine(x - 5, y + 3, 5);
    u8g2.drawHLine(x + 3, y + 4, 4);
    u8g2.drawHLine(x + 3, y + 6, 4);
  } else {
    u8g2.drawFrame(x - 5, y - 5, 4, 4);
    u8g2.drawFrame(x + 1, y - 5, 4, 4);
    u8g2.drawFrame(x - 5, y + 1, 4, 4);
    u8g2.drawFrame(x + 1, y + 1, 4, 4);
  }
}

void gambarTeksDinamisDiLogo(String namaMenuItem, int x, int y, bool besar) {
  if (besar) {
    u8g2.drawRBox(x - 18, y - 11, 36, 22, 3);
    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_5x7_tr);
    String pendek = namaMenuItem.substring(0, 6);
    int w = u8g2.getStrWidth(pendek.c_str());
    u8g2.drawStr(x - (w / 2), y + 3, pendek.c_str());
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawRFrame(x - 10, y - 8, 20, 16, 2);
    u8g2.setFont(u8g2_font_5x7_tr);
    String pendek = namaMenuItem.substring(0, 3);
    int w = u8g2.getStrWidth(pendek.c_str());
    u8g2.drawStr(x - (w / 2), y + 3, pendek.c_str());
  }
}

void gambarIkonKecil(int x, int y, String nama) {
  nama.toLowerCase();
  u8g2.drawRFrame(x - 8, y - 8, 16, 16, 2);
  if (nama.indexOf("lyric") >= 0 || nama.indexOf("lirik") >= 0) {
    gambarIkonAM(x, y, false);
  } else if (nama.indexOf("eye") >= 0 || nama.indexOf("mata") >= 0) {
    gambarIkonEyes(x, y, false);
  } else if (nama.indexOf("musik") >= 0 || nama.indexOf("anim") >= 0 || nama.indexOf("audio") >= 0) {
    u8g2.drawDisc(x - 2, y + 3, 2);
    u8g2.drawVLine(x, y - 4, 7);
    u8g2.drawHLine(x, y - 4, 4);
  } else if (nama.indexOf("game") >= 0 || nama.indexOf("dino") >= 0 || nama.indexOf("ray") >= 0 || nama.indexOf("flappy") >= 0) {
    u8g2.drawRFrame(x - 4, y - 2, 8, 5, 1);
  } else if (nama.indexOf("timer") >= 0) {
    gambarIkonJamPasir(x, y, false);
  } else if (nama.indexOf("alarm") >= 0) {
    gambarIkonLoncengAlarm(x, y, false);
  } else if (nama.indexOf("jam") >= 0) {
    u8g2.drawCircle(x, y, 4);
    u8g2.drawVLine(x, y - 3, 3);
  } else if (nama.indexOf("kalk") >= 0) {
    gambarIkonKalkulator(x, y, false);
  } else {
    u8g2.drawBox(x - 3, y - 3, 6, 6);
  }
}

void gambarIkonBesar(int x, int y, String nama) {
  nama.toLowerCase();
  u8g2.drawRBox(x - 14, y - 12, 28, 24, 3);
  u8g2.setDrawColor(0);
  if (nama.indexOf("lyric") >= 0 || nama.indexOf("lirik") >= 0) {
    gambarIkonAM(x, y, true);
  } else if (nama.indexOf("eye") >= 0 || nama.indexOf("mata") >= 0) {
    gambarIkonEyes(x, y, true);
  } else if (nama.indexOf("musik") >= 0 || nama.indexOf("anim") >= 0 || nama.indexOf("audio") >= 0) {
    u8g2.drawDisc(x - 3, y + 5, 3);
    u8g2.drawVLine(x, y - 6, 11);
    u8g2.drawHLine(x, y - 6, 6);
    u8g2.drawVLine(x + 6, y - 6, 5);
  } else if (nama.indexOf("game") >= 0 || nama.indexOf("dino") >= 0 || nama.indexOf("ray") >= 0 || nama.indexOf("flappy") >= 0) {
    u8g2.drawRFrame(x - 8, y - 4, 16, 9, 2);
    u8g2.drawPixel(x - 4, y);
    u8g2.drawDisc(x + 4, y, 1);
  } else if (nama.indexOf("timer") >= 0) {
    gambarIkonJamPasir(x, y, true);
  } else if (nama.indexOf("alarm") >= 0) {
    gambarIkonLoncengAlarm(x, y, true);
  } else if (nama.indexOf("jam") >= 0) {
    u8g2.drawCircle(x, y, 7);
    u8g2.drawLine(x, y, x + 3, y - 3);
  } else if (nama.indexOf("kalk") >= 0) {
    gambarIkonKalkulator(x, y, true);
  } else {
    u8g2.drawFrame(x - 7, y - 5, 14, 10);
    u8g2.drawLine(x - 7, y - 5, x, y + 1);
    u8g2.drawLine(x + 7, y - 5, x, y + 1);
  }
  u8g2.setDrawColor(1);
}

void tampilkanLoading3Detik() {
  unsigned long mulai = millis();
  while (millis() - mulai < 3000) {
    int persen = ((millis() - mulai) * 100) / 3000;
    if (persen > 100) persen = 100;
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(28, 16, "LOADING...");
    char barTxt[10];
    sprintf(barTxt, "%d%%", persen);
    u8g2.drawStr(52, 30, barTxt);
    u8g2.drawFrame(14, 34, 100, 8);
    u8g2.drawBox(16, 36, (96 * persen) / 100, 4);
    u8g2.setFont(u8g2_font_6x10_tr);
    int wStore = u8g2.getStrWidth("Sunny.store");
    u8g2.drawStr((128 - wStore) / 2, 56, "Sunny.store");
    u8g2.sendBuffer();
    delay(50);
  }
}

void tampilkanLoadingSubMenu1Detik() {
  unsigned long mulai = millis();
  while (millis() - mulai < 1000) {
    int persen = ((millis() - mulai) * 100) / 1000;
    if (persen > 100) persen = 100;
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(28, 22, "LOADING...");
    u8g2.drawFrame(14, 30, 100, 8);
    u8g2.drawBox(16, 32, (96 * persen) / 100, 4);
    u8g2.sendBuffer();
    delay(50);
  }
}

void putarAnimasiMasukSD(const char* filename = "/boot.bin", char mode = 'X') {
  File file = SD.open(filename, FILE_READ);
  if (!file) return;
  while (file.available() >= 1024) {
    file.read(frameBuffer, 1024);
    u8g2.clearBuffer();
    if (mode == 'B' || mode == 'b') u8g2.drawBitmap(0, 0, 16, 64, frameBuffer);
    else u8g2.drawXBMP(0, 0, 128, 64, frameBuffer);
    u8g2.sendBuffer();
    if (digitalRead(BTN1) == LOW) { delay(150); break; }
    delay(160);
  }
  file.close();
}

// --- FUNGSI UI ---
void gambarMenuTanyaWifi() {
  u8g2.clearBuffer(); 
  u8g2.setFont(u8g2_font_6x10_tr); 
  u8g2.drawStr(0, 9, "Konek ke WiFi?");
  String opsi1 = (wifiMenuTerpilih == 0) ? "> Konek WiFi" : "  Konek WiFi";
  String opsi2 = (wifiMenuTerpilih == 1) ? "> Skip Saja" : "  Skip Saja";
  u8g2.drawStr(0, 22, opsi1.c_str()); 
  u8g2.drawStr(0, 35, opsi2.c_str());
  u8g2.sendBuffer();
}

void gambarMenu(int offsetX) {
  u8g2.clearBuffer(); 
  if (totalMenu <= 0) {
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(10, 32, "Menu Kosong");
    u8g2.sendBuffer();
    return;
  }
    
  u8g2.setFont(u8g2_font_5x7_tr);
  char infoAtas[16];
  sprintf(infoAtas, "%d/%d", menuTerpilih + 1, totalMenu);
  int wPage = u8g2.getStrWidth(infoAtas);
  u8g2.drawStr(128 - wPage, 8, infoAtas);

  int prevIdx = (menuTerpilih - 1 + totalMenu) % totalMenu;
  int nextIdx = (menuTerpilih + 1) % totalMenu;

  if (totalMenu > 1) gambarIkonKecil(22 + offsetX, 31, menuNames[prevIdx]);
  gambarIkonBesar(64 + offsetX, 31, menuNames[menuTerpilih]);
  if (totalMenu > 1) gambarIkonKecil(106 + offsetX, 31, menuNames[nextIdx]);

  u8g2.setFont(u8g2_font_6x10_tr);
  String label = "< " + menuNames[menuTerpilih] + " >";
  int textW = u8g2.getStrWidth(label.c_str());
  int xLabel = (128 - textW) / 2;
  if (xLabel < 0) xLabel = 0;
  u8g2.drawStr(xLabel, 61, label.c_str());

  u8g2.sendBuffer();
}

void geserMenu(int arah) {
  putarAudioScroll();
  for (int step = 1; step <= 3; step++) {
    if (audioReady) audio.loop();
    gambarMenu(arah * step * 12);
    delay(20);
  }
  if (arah > 0) {
    if (menuTerpilih > 0) menuTerpilih--;
    else menuTerpilih = totalMenu - 1;
  } else {
    if (menuTerpilih < totalMenu - 1) menuTerpilih++;
    else menuTerpilih = 0;
  }
  for (int step = 2; step >= 0; step--) {
    if (audioReady) audio.loop();
    gambarMenu(-arah * step * 12);
    delay(20);
  }
}

void gambarSubMenu(int offsetX) {
  u8g2.clearBuffer(); 
  if (totalSubMenu <= 0) {
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(10, 32, "SubMenu Kosong");
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_5x7_tr);
  char infoAtas[16];
  sprintf(infoAtas, "%d/%d", subMenuTerpilih + 1, totalSubMenu);
  int wPage = u8g2.getStrWidth(infoAtas);
  u8g2.drawStr(128 - wPage, 8, infoAtas);

  int prevIdx = (subMenuTerpilih - 1 + totalSubMenu) % totalSubMenu;
  int nextIdx = (subMenuTerpilih + 1) % totalSubMenu;

  String subFileLower = currentSubMenuFile;
  subFileLower.toLowerCase();
  bool isEyeOrLyricSubMenu = subFileLower.indexOf("eye") >= 0 ||
                             subFileLower.indexOf("mata") >= 0 ||
                             subFileLower.indexOf("lyric") >= 0 ||
                             subFileLower.indexOf("lirik") >= 0;

  if (totalSubMenu > 1) {
    String namaPrev = subMenuNames[prevIdx];
    String namaPrevLower = namaPrev;
    namaPrevLower.toLowerCase();
    bool isAudioPrev = namaPrevLower.indexOf("audio") >= 0 || namaPrevLower.indexOf("musik") >= 0;
    if (isEyeOrLyricSubMenu && !isAudioPrev) {
      gambarTeksDinamisDiLogo(namaPrev, 22 + offsetX, 31, false);
    } else {
      gambarIkonKecil(22 + offsetX, 31, namaPrev);
    }
  }

  String namaCurr = subMenuNames[subMenuTerpilih];
  String namaCurrLower = namaCurr;
  namaCurrLower.toLowerCase();
  bool isAudioCurr = namaCurrLower.indexOf("audio") >= 0 || namaCurrLower.indexOf("musik") >= 0;
  if (isEyeOrLyricSubMenu && !isAudioCurr) {
    gambarTeksDinamisDiLogo(namaCurr, 64 + offsetX, 31, true);
  } else {
    gambarIkonBesar(64 + offsetX, 31, namaCurr);
  }

  if (totalSubMenu > 1) {
    String namaNext = subMenuNames[nextIdx];
    String namaNextLower = namaNext;
    namaNextLower.toLowerCase();
    bool isAudioNext = namaNextLower.indexOf("audio") >= 0 || namaNextLower.indexOf("musik") >= 0;
    if (isEyeOrLyricSubMenu && !isAudioNext) {
      gambarTeksDinamisDiLogo(namaNext, 106 + offsetX, 31, false);
    } else {
      gambarIkonKecil(106 + offsetX, 31, namaNext);
    }
  }

  u8g2.setFont(u8g2_font_6x10_tr);
  String label = "< " + subMenuNames[subMenuTerpilih] + " >";
  int textW = u8g2.getStrWidth(label.c_str());
  int xLabel = (128 - textW) / 2;
  if (xLabel < 0) xLabel = 0;
  u8g2.drawStr(xLabel, 61, label.c_str());

  u8g2.sendBuffer();
}

void geserSubMenu(int arah) {
  putarAudioScroll();
  for (int step = 1; step <= 2; step++) {
    if (audioReady) audio.loop();
    gambarSubMenu(arah * step * 16);
    delay(15);
  }
  if (arah > 0) {
    if (subMenuTerpilih > 0) subMenuTerpilih--;
    else subMenuTerpilih = totalSubMenu - 1;
  } else {
    if (subMenuTerpilih < totalSubMenu - 1) subMenuTerpilih++;
    else subMenuTerpilih = 0;
  }
  gambarSubMenu(0);
}

void gambarMenuWifiGagal() {
  u8g2.clearBuffer(); 
  u8g2.setFont(u8g2_font_6x10_tr); 
  u8g2.drawStr(0, 9, "WiFi Gagal/Tdk Ada");
  String opsi1 = (wifiMenuTerpilih == 0) ? "> Konek Lagi?" : "  Konek Lagi?";
  String opsi2 = (wifiMenuTerpilih == 1) ? "> Skip Saja" : "  Skip Saja";
  u8g2.drawStr(0, 22, opsi1.c_str()); 
  u8g2.drawStr(0, 35, opsi2.c_str());
  u8g2.sendBuffer();
}

void tampilkanJam() {
  u8g2.clearBuffer(); 
  time_t now = time(nullptr); 
  struct tm* timeinfo = localtime(&now);
  char waktuStr[20], tanggalStr[20];
  sprintf(waktuStr, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  sprintf(tanggalStr, "%02d/%02d/%04d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900);
  u8g2.setFont(u8g2_font_5x7_tr); 
  u8g2.drawStr(27, 12, "Real time Clock"); 
  u8g2.setFont(u8g2_font_7x14B_tr); 
  u8g2.drawStr(15, 32, waktuStr); 
  u8g2.setFont(u8g2_font_6x10_tr); 
  u8g2.drawStr(15, 48, tanggalStr); 
  u8g2.sendBuffer();
}

void tampilkanTimer() {
  u8g2.clearBuffer();
  unsigned long totalMs = swElapsedTime;
  int ms = (totalMs % 1000) / 10; 
  int detik = (totalMs / 1000) % 60; 
  int menit = (totalMs / 60000) % 60; 
  int jam = (totalMs / 3600000);
  char waktuStr[20]; 
  sprintf(waktuStr, "%02d:%02d:%02d:%02d", jam, menit, detik, ms);
  u8g2.setFont(u8g2_font_5x7_tr); 
  u8g2.drawStr(40, 15, "STOPWATCH");
  u8g2.setFont(u8g2_font_7x14B_tr); 
  u8g2.drawStr(5, 36, waktuStr);
  if (!isTimerRunning && swElapsedTime == 0) { 
    u8g2.setFont(u8g2_font_5x7_tr); 
    u8g2.drawStr(20, 52, "Klik B3 to Start"); 
  }
  u8g2.sendBuffer();
}

void tampilkanMenuAlarm() {
  u8g2.clearBuffer(); 
  u8g2.setFont(u8g2_font_6x10_tr);
  if (alarmMenuState == 0) {
    u8g2.drawStr(0, 9, "=== ALARM ===");
    if (totalAlarm == 0) { 
      u8g2.drawStr(0, 25, "Belum ada alarm"); 
      u8g2.drawStr(0, 40, "Buka Web utk nambah"); 
    } else {
      int startIdx = 0; 
      if (alarmTerpilih >= 4) startIdx = alarmTerpilih - 3;
      for (int i = 0; i < 4; i++) {
        int idx = startIdx + i; 
        if (idx >= totalAlarm) break;
        char teks[25]; 
        sprintf(teks, "%02d/%02d %02d:%02d [%s]", alarmDay[idx], alarmMonth[idx], alarmHour[idx], alarmMinute[idx], alarmActive[idx] ? "ON" : "OFF");
        if (idx == alarmTerpilih) u8g2.drawStr(0, 22 + (i*11), ("> " + String(teks)).c_str());
        else u8g2.drawStr(0, 22 + (i*11), ("  " + String(teks)).c_str());
      }
    }
  } else if (alarmMenuState == 1) {
    u8g2.drawStr(0, 9, "Pilih Aksi:");
    String opsi1 = (alarmActionChoice == 0) ? "> Toggle ON/OFF" : "  Toggle ON/OFF";
    String opsi2 = (alarmActionChoice == 1) ? "> Hapus Alarm" : "  Hapus Alarm";
    u8g2.drawStr(0, 25, opsi1.c_str()); 
    u8g2.drawStr(0, 40, opsi2.c_str());
  } else if (alarmMenuState == 2) {
    u8g2.drawStr(0, 9, "Yakin Hapus?");
    String opsi1 = (alarmDeleteChoice == 0) ? "> Tidak" : "  Tidak";
    String opsi2 = (alarmDeleteChoice == 1) ? "> Ya" : "  Ya";
    u8g2.drawStr(0, 25, opsi1.c_str()); 
    u8g2.drawStr(0, 40, opsi2.c_str());
  }
  u8g2.sendBuffer();
}

String incrementLastDigit(String numStr) {
  if (numStr.length() == 0) return "0";
  char lastChar = numStr.charAt(numStr.length() - 1);
  int d = lastChar - '0';
  d = (d + 1) % 10;
  return numStr.substring(0, numStr.length() - 1) + String(d);
}

void tampilkanKalkulator() {
  u8g2.clearBuffer(); 
  u8g2.setFont(u8g2_font_5x7_tr); 
  u8g2.drawStr(0, 10, "Hold B3: Keluar");
  char line1[32], line2[32];
  if (calcState == 0) { 
    sprintf(line1, "Input Angka 1 (B1/B2)"); 
    sprintf(line2, "%s", calcNum1.c_str()); 
  } else if (calcState == 1) { 
    sprintf(line1, "Pilih Op (B1/B2)"); 
    sprintf(line2, "%s %c", calcNum1.c_str(), calcOp); 
  } else if (calcState == 2) { 
    sprintf(line1, "%s %c", calcNum1.c_str(), calcOp); 
    sprintf(line2, "%s", calcNum2.c_str()); 
  } else {
    float n1 = calcNum1.toFloat(), n2 = calcNum2.toFloat(), res = 0;
    if (calcOp == '+') res = n1 + n2;
    else if (calcOp == '-') res = n1 - n2;
    else if (calcOp == '*') res = n1 * n2;
    else if (calcOp == '/') res = (n2 != 0) ? (n1 / n2) : 0;
    sprintf(line1, "%s%c%s=", calcNum1.c_str(), calcOp, calcNum2.c_str());
    sprintf(line2, "%.2f", res);
  }
  u8g2.setFont(u8g2_font_7x14B_tr); 
  u8g2.drawStr(0, 32, line1); 
  u8g2.drawStr(0, 52, line2);
  u8g2.sendBuffer();
}

void gambarKotakGameOver(const char* judul, int scoreVal, int bestVal, bool isNight) {
  if (isNight) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, 128, 64);
    u8g2.setDrawColor(0);
  } else {
    u8g2.setDrawColor(1);
  }

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(0, 8, judul);
  char sTop[16];
  sprintf(sTop, "S:%d", scoreVal);
  int wTop = u8g2.getStrWidth(sTop);
  u8g2.drawStr(128 - wTop, 8, sTop);
  u8g2.drawHLine(0, 10, 128);

  if (isNight) {
    u8g2.setDrawColor(0);
    u8g2.drawRBox(10, 14, 108, 46, 3);
    u8g2.setDrawColor(1);
    u8g2.drawRFrame(10, 14, 108, 46, 3);
  } else {
    u8g2.setDrawColor(0);
    u8g2.drawRBox(10, 14, 108, 46, 3);
    u8g2.setDrawColor(1);
    u8g2.drawRFrame(10, 14, 108, 46, 3);
  }

  u8g2.setFont(u8g2_font_6x10_tr);
  const char* goTitle = "GAME OVER";
  int wGo = u8g2.getStrWidth(goTitle);
  u8g2.drawStr(64 - (wGo / 2), 25, goTitle);

  u8g2.setFont(u8g2_font_5x7_tr);
  char bufScore[24];
  sprintf(bufScore, "SCORE %-4d BEST %-4d", scoreVal, bestVal);
  int wSc = u8g2.getStrWidth(bufScore);
  u8g2.drawStr(64 - (wSc / 2), 37, bufScore);
  u8g2.drawStr(64 - (u8g2.getStrWidth("B1:RETRY  B2:MENU") / 2), 51, "B1:RETRY  B2:MENU");

  u8g2.setDrawColor(1);
}

void tampilkanGameDino() {
  bool isNight = ((gameScore / 100) % 2 == 1);
  u8g2.clearBuffer();

  if (isNight) {
    u8g2.drawBox(0, 0, 128, 64);
    u8g2.setDrawColor(0);
  } else {
    u8g2.setDrawColor(1);
  }

  if (gameOver) {
    gambarKotakGameOver("DINO RUN", gameScore, bestDino, isNight);
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(0, 8, "DINO RUN");
  char scTxt[16];
  sprintf(scTxt, "S:%d", gameScore);
  int wSc = u8g2.getStrWidth(scTxt);
  u8g2.drawStr(128 - wSc, 8, scTxt);
  u8g2.drawHLine(0, 10, 128);

  int groundY = 54;
  u8g2.drawHLine(0, groundY, 128);
  for (int gx = -groundOffset; gx < 128; gx += 8) {
    if (gx >= 0 && gx < 126) u8g2.drawPixel(gx, groundY + 3);
    if (gx + 4 >= 0 && gx + 4 < 126) u8g2.drawHLine(gx + 4, groundY + 6, 2);
  }

  int dx = 12;
  int dy = dinoY;
  u8g2.drawBox(dx, dy + 6, 4, 5);
  u8g2.drawBox(dx + 4, dy + 3, 7, 9);
  u8g2.drawBox(dx + 7, dy - 3, 7, 6);
  u8g2.setDrawColor(isNight ? 1 : 0);
  u8g2.drawPixel(dx + 11, dy - 1);
  u8g2.setDrawColor(isNight ? 0 : 1);
  if ((gameScore / 2) % 2 == 0 || dinoY < 38) {
    u8g2.drawVLine(dx + 5, dy + 12, 4);
    u8g2.drawVLine(dx + 9, dy + 12, 2);
  } else {
    u8g2.drawVLine(dx + 5, dy + 12, 2);
    u8g2.drawVLine(dx + 9, dy + 12, 4);
  }

  int ox = obsX;
  int oy = 42;
  u8g2.drawBox(ox + 2, oy, 4, 12);
  u8g2.drawBox(ox, oy + 3, 2, 4);
  u8g2.drawBox(ox + 6, oy + 2, 2, 5);

  u8g2.setDrawColor(1);
  u8g2.sendBuffer();
}

void tampilkanFlappyBird() {
  u8g2.clearBuffer();

  if (flappyGameOver) {
    gambarKotakGameOver("FLAPPY BIRD", flappyScore, bestFlappy, false);
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(0, 8, "FLAPPY BIRD");
  char scTxt[16];
  sprintf(scTxt, "S:%d", flappyScore);
  int wSc = u8g2.getStrWidth(scTxt);
  u8g2.drawStr(128 - wSc, 8, scTxt);
  u8g2.drawHLine(0, 10, 128);

  int groundY = 55;
  for (int gx = -flappyGroundOffset; gx < 128; gx += 4) {
    if (gx >= 0 && gx < 127) u8g2.drawHLine(gx, groundY, 2);
  }

  int pipeW = 10;
  int topBottomY = pipeGapY;
  int botTopY = pipeGapY + pipeGapH;
  if (topBottomY > 11) {
    u8g2.drawFrame(pipeX, 11, pipeW, topBottomY - 11);
    u8g2.drawBox(pipeX + 2, 11, pipeW - 4, topBottomY - 11);
  }
  if (botTopY < groundY) {
    u8g2.drawFrame(pipeX, botTopY, pipeW, groundY - botTopY);
    u8g2.drawBox(pipeX + 2, botTopY, pipeW - 4, groundY - botTopY);
  }

  int bx = 24;
  int by = (int)birdY;
  u8g2.drawBox(bx, by, 7, 5);
  u8g2.drawPixel(bx + 7, by + 2);
  u8g2.drawPixel(bx + 5, by + 1);
  if (birdVy < 0) {
    u8g2.drawPixel(bx + 2, by - 1);
  } else {
    u8g2.drawPixel(bx + 2, by + 5);
  }

  u8g2.sendBuffer();
}

void tampilkanRaycaster() {
  u8g2.clearBuffer();
  float fov = 3.14159 / 3.0; 
  for (int x = 0; x < 128; x += 2) {
    float rayAngle = pa - fov/2.0 + (x / 128.0) * fov;
    float sinA = sin(rayAngle);
    float cosA = cos(rayAngle);
    float dist = 0.0;
    bool hit = false;
    bool isFinish = false;
    
    while (!hit && dist < 40.0) {
      dist += 0.1;
      int mx = (int)(px + cosA * dist);
      int my = (int)(py + sinA * dist);
      if (mx < 0 || mx >= MAP_W || my < 0 || my >= MAP_H) { 
        hit = true; 
        break; 
      }
      if (mazeMap[my][mx] == 1) { 
        hit = true; 
      } else if (mazeMap[my][mx] == 2) { 
        hit = true; 
        isFinish = true; 
      }
    }
    
    int lineH = (int)(64.0 / dist);
    if (lineH > 64) lineH = 64;
    if (lineH < 1) lineH = 1;
    
    if (isFinish) lineH = lineH / 2;
    
    int startY = 32 - lineH / 2;
    u8g2.drawVLine(x, startY, lineH);
    u8g2.drawVLine(x+1, startY, lineH);
  }
  u8g2.sendBuffer();
}

unsigned long parseWaktu(String str) {
  int c1 = str.indexOf(':'); 
  int c2 = str.indexOf(':', c1 + 1);
  if (c1 == -1 || c2 == -1) return 0;
  unsigned long menit = str.substring(0, c1).toInt(); 
  unsigned long detik = str.substring(c1 + 1, c2).toInt();
  String msStr = str.substring(c2 + 1); 
  unsigned long ms = msStr.toInt();
  unsigned long multiplier = 1; 
  if (msStr.length() == 1) multiplier = 100; 
  else if (msStr.length() == 2) multiplier = 10;
  return (menit * 60000UL) + (detik * 1000UL) + (ms * multiplier);
}

void putarLirikFile(String namaFile) {
  totalLirik = 0; 
  File file = SD.open(namaFile); 
  if (!file) return;
  while (file.available() && totalLirik < MAX_LIRIK) {
    String baris = file.readStringUntil('\n'); 
    baris.trim();
    if (baris.length() > 0) {
      int k1 = baris.indexOf(','); 
      int k2 = baris.indexOf(',', k1 + 1);
      if (k1 > 0 && k2 > 0) {
        daftarLirik[totalLirik].startMs = parseWaktu(baris.substring(0, k1));
        daftarLirik[totalLirik].endMs = parseWaktu(baris.substring(k1 + 1, k2));
        String teks = baris.substring(k2 + 1); 
        int pipa = teks.indexOf('|');
        if (pipa > 0) { 
          daftarLirik[totalLirik].baris1 = teks.substring(0, pipa); 
          daftarLirik[totalLirik].baris2 = teks.substring(pipa + 1); 
        } else { 
          daftarLirik[totalLirik].baris1 = teks; 
          daftarLirik[totalLirik].baris2 = ""; 
        }
        totalLirik++;
      }
    }
  }
  file.close();
  if (totalLirik == 0) return;
  unsigned long startTime = millis(); 
  bool isPlaying = true;
  while (isPlaying) {
    if (wirelessEnabled) server.handleClient(); 
    unsigned long elapsed = millis() - startTime;
    u8g2.clearBuffer(); 
    u8g2.setFont(u8g2_font_6x10_tr);
    bool adaLirik = false;
    for (int i = 0; i < totalLirik; i++) {
      if (elapsed >= daftarLirik[i].startMs && elapsed <= daftarLirik[i].endMs) {
        u8g2.drawStr(0, 22, daftarLirik[i].baris1.c_str());
        if (daftarLirik[i].baris2 != "") u8g2.drawStr(0, 42, daftarLirik[i].baris2.c_str());
        adaLirik = true; 
        break;
      }
    }
    if (!adaLirik) u8g2.drawStr(0, 30, "...");
    u8g2.sendBuffer();
    if (digitalRead(BTN1) == LOW || webBack) { 
      isPlaying = false; 
      webBack = false; 
      delay(300); 
    }
    if (elapsed > daftarLirik[totalLirik - 1].endMs + 2000) { 
      isPlaying = false; 
    }
    delay(30); 
  }
}

void playAnimation(const char* filename, char mode, unsigned long durasiDetik) {
  File file = SD.open(filename, FILE_READ); 
  if (!file) return;

  putarAudioAnimasi(filename);

  unsigned long totalFrames = file.size() / 1024;
  unsigned long stepMs = 41;
  if (durasiDetik > 0 && totalFrames > 0) {
    stepMs = (durasiDetik * 1000UL) / totalFrames;
    if (stepMs < 1) stepMs = 1;
  }

  bool isPlaying = true;
  while (isPlaying) {
    unsigned long frameStart = millis();
    if (audioReady) audio.loop();
    if (wirelessEnabled) server.handleClient();
    if (file.available() < 1024) file.seek(0); 
    file.read(frameBuffer, 1024);
    u8g2.clearBuffer();
    if (mode == 'B' || mode == 'b') u8g2.drawBitmap(0, 0, 16, 64, frameBuffer); 
    else u8g2.drawXBMP(0, 0, 128, 64, frameBuffer);  
    u8g2.sendBuffer();
    if (digitalRead(BTN1) == LOW || webBack) { 
      isPlaying = false; 
      webBack = false; 
      delay(300); 
      break;
    }
    while (millis() - frameStart < stepMs) {
      if (audioReady) audio.loop();
      delay(1);
    }
  }
  hentikanAudio();
  file.close();
}

void matikanWireless() {
  wirelessEnabled = false; 
  server.stop(); 
  WiFi.disconnect(true); 
  WiFi.mode(WIFI_OFF);
  u8g2.clearBuffer(); 
  u8g2.setFont(u8g2_font_6x10_tr); 
  u8g2.drawStr(20, 32, "Wireless OFF"); 
  u8g2.sendBuffer(); 
  delay(1000);
  appState = 0; 
  gambarMenu();
}

void nyalakanWireless() {
  u8g2.clearBuffer(); 
  u8g2.setFont(u8g2_font_6x10_tr); 
  u8g2.drawStr(0, 32, "Konek WiFi..."); 
  u8g2.sendBuffer();
  WiFi.disconnect(true); 
  delay(1000); 
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);
  int timeout = 0; 
  while (WiFi.status() != WL_CONNECTED && timeout < 20) { 
    delay(500); 
    timeout++; 
  }
  if (WiFi.status() == WL_CONNECTED) {
    server.begin(); 
    wirelessEnabled = true;
    u8g2.clearBuffer(); 
    u8g2.drawStr(0, 22, "IP Address:"); 
    u8g2.drawStr(0, 42, WiFi.localIP().toString().c_str()); 
    u8g2.sendBuffer(); 
    delay(2000);
    appState = 0; 
    gambarMenu();
  } else {
    u8g2.clearBuffer(); 
    u8g2.drawStr(10, 32, "WiFi Gagal!"); 
    u8g2.sendBuffer(); 
    delay(1500); 
    appState = 0; 
    gambarMenu();
  }
}

bool cekToggleWireless() {
  static unsigned long pressStart = 0;
  bool isAllPressed = (digitalRead(BTN1) == LOW && digitalRead(BTN2) == LOW && digitalRead(BTN3) == LOW);

  if (isAllPressed) {
    if (pressStart == 0) pressStart = millis();
    if (millis() - pressStart > 1000) { 
      pressStart = 0;
      if (appState == 4 || appState == 14 || appState == 15 || appState == 17 || appState == 18) {
        WiFi.disconnect(true); 
        WiFi.mode(WIFI_OFF);
        appState = 0; 
        gambarMenu();
      } else {
        if (wirelessEnabled) matikanWireless(); 
        else nyalakanWireless();
      }
      while(digitalRead(BTN1) == LOW || digitalRead(BTN2) == LOW || digitalRead(BTN3) == LOW) { 
        delay(10); 
      }
      return true;
    }
  } else {
    pressStart = 0; 
  }
  return false;
}

void setupWebServerRoutes() {
  server.on("/", handleRoot); 
  server.on("/up", handleUp); 
  server.on("/down", handleDown); 
  server.on("/select", handleSelect); 
  server.on("/back", handleBack); 
  server.on("/addalarm", handleAddAlarm);
  server.on("/op+", handleOpPlus); 
  server.on("/op-", handleOpMinus); 
  server.on("/op*", handleOpMul); 
  server.on("/op/", handleOpDiv);
  server.begin();
}

bool konekWiFi() {
  u8g2.clearBuffer(); 
  u8g2.setFont(u8g2_font_6x10_tr); 
  u8g2.drawStr(0, 32, "Konek WiFi..."); 
  u8g2.sendBuffer();
  WiFi.disconnect(true); 
  delay(1000); 
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);
  int timeout = 0; 
  while (WiFi.status() != WL_CONNECTED && timeout < 20) { 
    delay(500); 
    timeout++; 
  }
  if (WiFi.status() == WL_CONNECTED) {
    bool ntpSynced = false; 
    int ntpAttempt = 1; 
    bool batalNtp = false;
    while (!ntpSynced && !batalNtp) {
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2, ntpServer3);
      time_t now = time(nullptr); 
      int countdown = 15; 
      unsigned long lastTick = millis();
      String teksNtp = (ntpAttempt == 1) ? "Sinkron Ntp.." : "Sinkron ntp lagi..";
      u8g2.clearBuffer(); 
      u8g2.setFont(u8g2_font_logisoso24_tr); 
      char numStr[3]; 
      sprintf(numStr, "%d", countdown); 
      u8g2.drawStr(5, 45, numStr);
      u8g2.setFont(u8g2_font_6x10_tr); 
      u8g2.drawStr(35, 20, teksNtp.c_str()); 
      u8g2.drawStr(35, 35, "Mohon Tunggu"); 
      u8g2.drawStr(35, 50, "B1: Exit"); 
      u8g2.sendBuffer();
      while (now < 1600000000 && countdown > 0) {
        if (digitalRead(BTN1) == LOW) { 
          batalNtp = true; 
          delay(300); 
          break; 
        }
        if (millis() - lastTick >= 1000) {
          countdown--; 
          lastTick = millis();
          if (countdown > 0) {
            u8g2.clearBuffer(); 
            u8g2.setFont(u8g2_font_logisoso24_tr); 
            sprintf(numStr, "%d", countdown); 
            u8g2.drawStr(5, 45, numStr);
            u8g2.setFont(u8g2_font_6x10_tr); 
            u8g2.drawStr(35, 20, teksNtp.c_str()); 
            u8g2.drawStr(35, 35, "Mohon Tunggu"); 
            u8g2.drawStr(35, 50, "B1: Exit"); 
            u8g2.sendBuffer();
          }
        }
        now = time(nullptr); 
        delay(10); 
      }
      if (batalNtp) break; 
      if (now >= 1600000000) ntpSynced = true;
      else { 
        ntpAttempt++; 
        delay(1000); 
      }
    }
    if (batalNtp) return false;
    if (ntpSynced) {
      bool pilihIP = true; 
      bool memilih = false;
      while (!memilih) {
        u8g2.clearBuffer(); 
        u8g2.setFont(u8g2_font_6x10_tr); 
        u8g2.drawStr(0, 15, "Wireless Control?");
        String ipStr = "IP: " + WiFi.localIP().toString();
        if (pilihIP) { 
          u8g2.drawStr(0, 32, ("> " + ipStr).c_str()); 
          u8g2.drawStr(0, 48, "  Skip Saja"); 
        } else { 
          u8g2.drawStr(0, 32, ("  " + ipStr).c_str()); 
          u8g2.drawStr(0, 48, "> Skip Saja"); 
        }
        u8g2.sendBuffer();
        if (digitalRead(BTN1) == LOW) { 
          pilihIP = true; 
          delay(200); 
        }
        if (digitalRead(BTN2) == LOW) { 
          pilihIP = false; 
          delay(200); 
        }
        if (digitalRead(BTN3) == LOW) { 
          memilih = true; 
          delay(200); 
        }
      }
      if (pilihIP) { 
        wirelessEnabled = true; 
        setupWebServerRoutes();
        return true; 
      } else { 
        wirelessEnabled = false; 
        WiFi.disconnect(true); 
        WiFi.mode(WIFI_OFF); 
        return true; 
      }
    }
  }
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  return false;
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN1, INPUT_PULLUP); 
  pinMode(BTN2, INPUT_PULLUP); 
  pinMode(BTN3, INPUT_PULLUP);
  Wire.begin(OLED_SDA, OLED_SCL); 
  u8g2.begin();
  randomSeed(millis());
  
  bool sdOk = false;
  while (!sdOk) {
    u8g2.clearBuffer(); 
    u8g2.setFont(u8g2_font_6x10_tr); 
    u8g2.drawStr(0, 32, "Membaca SD..."); 
    u8g2.sendBuffer();
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS)) {
      int countdown = 10; 
      bool exitCountdown = false; 
      unsigned long lastTick = millis();
      u8g2.clearBuffer(); 
      u8g2.setFont(u8g2_font_logisoso24_tr); 
      char numStr[3]; 
      sprintf(numStr, "%d", countdown); 
      u8g2.drawStr(5, 45, numStr);
      u8g2.setFont(u8g2_font_6x10_tr); 
      u8g2.drawStr(35, 20, "SD CARD FAIL!"); 
      u8g2.drawStr(35, 40, "Tekan B1 to exit"); 
      u8g2.sendBuffer();
      while (countdown > 0 && !exitCountdown) {
        if (digitalRead(BTN1) == LOW) { 
          exitCountdown = true; 
          delay(300); 
          break; 
        }
        if (millis() - lastTick >= 1000) {
          countdown--; 
          lastTick = millis();
          if (countdown > 0) {
            u8g2.clearBuffer(); 
            u8g2.setFont(u8g2_font_logisoso24_tr); 
            sprintf(numStr, "%d", countdown); 
            u8g2.drawStr(5, 45, numStr);
            u8g2.setFont(u8g2_font_6x10_tr); 
            u8g2.drawStr(35, 20, "SD CARD FAIL!"); 
            u8g2.drawStr(35, 40, "Tekan B1 to exit"); 
            u8g2.sendBuffer();
          }
        }
        delay(10); 
      }
      if (exitCountdown) { 
        delay(500); 
      } else { 
        u8g2.clearBuffer(); 
        u8g2.drawStr(10, 32, "Deep Sleep..."); 
        u8g2.sendBuffer(); 
        delay(1000); 
        esp_deep_sleep_start(); 
      }
    } else { 
      sdOk = true; 
    }
  }

  setupAudio();
  bacaMenuDariSD();
  bacaAlarmDariSD();
  bestDino = bacaBestDariSD("/best_dino.txt");
  bestFlappy = bacaBestDariSD("/best_flappy.txt");

  appState = 14;
  wifiPromptChoice = 0;
  gambarPromptWifi("Konek wifi?", "Konek", "Skip aja");
}

void loop() {
  if (audioReady) audio.loop();

  bool b1 = (digitalRead(BTN1) == LOW);
  bool b2 = (digitalRead(BTN2) == LOW);
  bool b3 = (digitalRead(BTN3) == LOW);

  if (b1 && btn1Time == 0) btn1Time = millis();
  if (!b1) btn1Time = 0;
  
  if (b2 && btn2Time == 0) btn2Time = millis();
  if (!b2) btn2Time = 0;
  
  if (b3 && btn3Time == 0) btn3Time = millis();
  if (!b3) btn3Time = 0;
        
  if (b1 && b2 && b3) {
    unsigned long maxT = btn1Time;
    if (btn2Time > maxT) maxT = btn2Time;
    if (btn3Time > maxT) maxT = btn3Time;
    
    unsigned long minT = btn1Time;
    if (btn2Time < minT) minT = btn2Time;
    if (btn3Time < minT) minT = btn3Time;     
    
    if ((maxT - minT) < MULTI_PRESS_WINDOW) {
      while(digitalRead(BTN1) == LOW || digitalRead(BTN2) == LOW || digitalRead(BTN3) == LOW) {
        delay(10);
      }
      btn1Time = btn2Time = btn3Time = 0;
      
      if (appState == 1) { 
        appState = 0;
        gambarMenu();
        return; 
      }
    }
  }

  if (appState != 1 && appState != 11 && appState != 13 && appState != 14 && appState != 15 && appState != 17 && appState != 18) {
    if (cekToggleWireless()) return; 
  }

  if (wirelessEnabled) server.handleClient();

  if (appState != 7 && appState != 14 && appState != 15 && appState != 17 && appState != 18) { 
    time_t now = time(nullptr); 
    struct tm* timeinfo = localtime(&now);
    char currentTimeStr[20];
    sprintf(currentTimeStr, "%04d%02d%02d%02d%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min);
    
    if (String(currentTimeStr) != lastAlarmTimeString) { 
      lastAlarmTimeString = String(currentTimeStr);
      for (int i = 0; i < totalAlarm; i++) {
        if (alarmActive[i] && 
            timeinfo->tm_year + 1900 == alarmYear[i] && 
            timeinfo->tm_mon + 1 == alarmMonth[i] && 
            timeinfo->tm_mday == alarmDay[i] && 
            timeinfo->tm_hour == alarmHour[i] && 
            timeinfo->tm_min == alarmMinute[i]) {
          appState = 7; 
          break;
        }
      }
    }
  }

  bool btnUpFisik = (digitalRead(BTN1) == LOW);
  bool btnDownFisik = (digitalRead(BTN2) == LOW);
  bool btnSelectFisik = (digitalRead(BTN3) == LOW);

  if (appState == 14) {
    if (btnUpFisik || btnDownFisik || webUp || webDown) {
      wifiPromptChoice = (wifiPromptChoice == 0) ? 1 : 0;
      gambarPromptWifi("Konek wifi?", "Konek", "Skip aja");
      delay(200);
      webUp = false;
      webDown = false;
    }
    if (btnSelectFisik || webSelect) {
      webSelect = false;
      delay(200);
      if (wifiPromptChoice == 0) {
        if (konekWiFi()) {
          appState = 0;
          gambarMenu();
        } else {
          appState = 17;
          wifiRetryChoice = 0;
          gambarPromptWifi("Wifi ggal", "Konek lagi", "Yaudah skip");
        }
      } else {
        appState = 0;
        gambarMenu();
      }
    }
  } else if (appState == 17) {
    if (btnUpFisik || btnDownFisik || webUp || webDown) {
      wifiRetryChoice = (wifiRetryChoice == 0) ? 1 : 0;
      gambarPromptWifi("Wifi ggal", "Konek lagi", "Yaudah skip");
      delay(200);
      webUp = false;
      webDown = false;
    }
    if (btnSelectFisik || webSelect) {
      webSelect = false;
      delay(200);
      if (wifiRetryChoice == 0) {
        if (konekWiFi()) {
          appState = 0;
          gambarMenu();
        } else {
          wifiRetryChoice = 0;
          gambarPromptWifi("Wifi ggal", "Konek lagi", "Yaudah skip");
        }
      } else {
        appState = 0;
        gambarMenu();
      }
    }
  } else if (appState == 15) {
    if (btnUpFisik || btnDownFisik || webUp || webDown) {
      wifiPromptChoice = (wifiPromptChoice == 0) ? 1 : 0;
      gambarPromptWifi("WiFi blm konek", "Konek", "Engk dulu");
      delay(200);
      webUp = false;
      webDown = false;
    }
    if (btnSelectFisik || webSelect) {
      webSelect = false;
      delay(200);
      if (wifiPromptChoice == 0) {
        if (konekWiFi()) {
          appState = 0;
          gambarMenu();
        } else {
          appState = 18;
          wifiRetryChoice = 0;
          gambarPromptWifi("Wifi ggal", "Konek lagi", "Yaudah g ush");
        }
      } else {
        appState = 0;
        gambarMenu();
      }
    }
    if (webBack) {
      webBack = false;
      appState = 0;
      gambarMenu();
      delay(200);
    }
  } else if (appState == 18) {
    if (btnUpFisik || btnDownFisik || webUp || webDown) {
      wifiRetryChoice = (wifiRetryChoice == 0) ? 1 : 0;
      gambarPromptWifi("Wifi ggal", "Konek lagi", "Yaudah g ush");
      delay(200);
      webUp = false;
      webDown = false;
    }
    if (btnSelectFisik || webSelect) {
      webSelect = false;
      delay(200);
      if (wifiRetryChoice == 0) {
        if (konekWiFi()) {
          appState = 0;
          gambarMenu();
        } else {
          wifiRetryChoice = 0;
          gambarPromptWifi("Wifi ggal", "Konek lagi", "Yaudah g ush");
        }
      } else {
        appState = 0;
        gambarMenu();
      }
    }
    if (webBack) {
      webBack = false;
      appState = 0;
      gambarMenu();
      delay(200);
    }
  } else if (appState == 16) {
    tampilkanCrypto();
    if (btnUpFisik || webUp) {
      cryptoCoinIdx = (cryptoCoinIdx + 1) % 3;
      ambilHargaCrypto();
      delay(200);
      webUp = false;
    }
    if (btnDownFisik || webDown) {
      cryptoUseIDR = !cryptoUseIDR;
      ambilHargaCrypto();
      delay(200);
      webDown = false;
    }
    if (btnSelectFisik || webSelect) {
      ambilHargaCrypto();
      delay(200);
      webSelect = false;
    }
    if (webBack) {
      webBack = false;
      appState = 0;
      gambarMenu();
      delay(200);
    }
    static unsigned long cryptoB3Start = 0;
    if (btnSelectFisik) {
      if (cryptoB3Start == 0) cryptoB3Start = millis();
      if (millis() - cryptoB3Start > 1000) {
        cryptoB3Start = 0;
        appState = 0;
        gambarMenu();
        delay(300);
      }
    } else {
      cryptoB3Start = 0;
    }
  } else if (appState == 19) {
    tampilkanSaham();
    if (btnUpFisik || webUp) {
      sahamIdx = (sahamIdx + 1) % 3;
      ambilHargaSaham();
      delay(200);
      webUp = false;
    }
    if (btnDownFisik || webDown) {
      sahamIdx = (sahamIdx + 2) % 3;
      ambilHargaSaham();
      delay(200);
      webDown = false;
    }
    if (btnSelectFisik || webSelect) {
      ambilHargaSaham();
      delay(200);
      webSelect = false;
    }
    if (webBack) {
      webBack = false;
      appState = 0;
      gambarMenu();
      delay(200);
    }
    static unsigned long sahamB3Start = 0;
    if (btnSelectFisik) {
      if (sahamB3Start == 0) sahamB3Start = millis();
      if (millis() - sahamB3Start > 1000) {
        sahamB3Start = 0;
        appState = 0;
        gambarMenu();
        delay(300);
      }
    } else {
      sahamB3Start = 0;
    }
  } else if (appState == 20) {
    tampilkanJadwalSholat();
    if (btnUpFisik || webUp || btnDownFisik || webDown) {
      sholatPage = (sholatPage == 0) ? 1 : 0;
      delay(200);
      webUp = false;
      webDown = false;
    }
    if (btnSelectFisik || webSelect) {
      ambilJadwalSholatSukabumi();
      delay(200);
      webSelect = false;
    }
    if (webBack) {
      webBack = false;
      appState = 0;
      gambarMenu();
      delay(200);
    }
    static unsigned long sholatB3Start = 0;
    if (btnSelectFisik) {
      if (sholatB3Start == 0) sholatB3Start = millis();
      if (millis() - sholatB3Start > 1000) {
        sholatB3Start = 0;
        appState = 0;
        gambarMenu();
        delay(300);
      }
    } else {
      sholatB3Start = 0;
    }
  } else if (appState == 0) {
    if (btnUpFisik || webUp) {
      geserMenu(1); 
      delay(100); 
      webUp = false;
    }
    if (btnDownFisik || webDown) {
      geserMenu(-1); 
      delay(100); 
      webDown = false;
    }
    if (btnSelectFisik || webSelect) {
      if (totalMenu > 0) {
        String cekMenu = menuNames[menuTerpilih]; 
        cekMenu.toLowerCase(); 
        if (cekMenu == "jam") { 
          appState = 3; 
        } else if (cekMenu == "timer") { 
          appState = 6; 
          isTimerRunning = false; 
          swElapsedTime = 0; 
        } else if (cekMenu == "alarm") { 
          appState = 8; 
          alarmTerpilih = 0; 
          alarmMenuState = 0; 
          tampilkanMenuAlarm(); 
        } else if (cekMenu == "kalkulator") { 
          appState = 9; 
          calcState = 0; 
          calcNum1 = "0"; 
          calcNum2 = "0"; 
          calcOp = '+'; 
        } else if (cekMenu == "crypto" || cekMenu == "check crypto" || menuSubFiles[menuTerpilih] == "/crypto") {
          if (WiFi.status() != WL_CONNECTED) {
            appState = 15;
            wifiPromptChoice = 0;
            gambarPromptWifi("WiFi blm konek", "Konek", "Engk dulu");
          } else {
            appState = 16;
            cryptoLoaded = false;
            ambilHargaCrypto();
          }
        } else if (cekMenu == "saham" || cekMenu == "check saham" || menuSubFiles[menuTerpilih] == "/saham") {
          if (WiFi.status() != WL_CONNECTED) {
            appState = 15;
            wifiPromptChoice = 0;
            gambarPromptWifi("WiFi blm konek", "Konek", "Engk dulu");
          } else {
            appState = 19;
            ambilHargaSaham();
          }
        } else if (cekMenu == "sholat" || cekMenu == "jadwal sholat" || menuSubFiles[menuTerpilih] == "/sholat") {
          if (WiFi.status() != WL_CONNECTED) {
            appState = 15;
            wifiPromptChoice = 0;
            gambarPromptWifi("WiFi blm konek", "Konek", "Engk dulu");
          } else {
            appState = 20;
            sholatPage = 0;
            ambilJadwalSholatSukabumi();
          }
        } else if (menuSubFiles[menuTerpilih] != "") {
          currentSubMenuFile = menuSubFiles[menuTerpilih];
          bacaSubMenuDariSD(currentSubMenuFile);
          if (totalSubMenu > 0) { 
            tampilkanLoadingSubMenu1Detik();
            appState = 1; 
            subMenuTerpilih = 0; 
            gambarSubMenu(); 
          }
        }
      }
      delay(200); 
      webSelect = false;
    }
    if (webBack) { 
      webBack = false; 
      if (wirelessEnabled) matikanWireless(); 
    }
  } else if (appState == 1) { 
    if (btnUpFisik || webUp) {
      geserSubMenu(1); 
      delay(100); 
      webUp = false;
    }
    if (btnDownFisik || webDown) { 
      geserSubMenu(-1); 
      delay(100); 
      webDown = false;
    }
    if (btnSelectFisik || webSelect) { 
      if (totalSubMenu > 0) {
        if (subMenuTypes[subMenuTerpilih] == 'T') {
          putarLirikFile(subMenuFiles[subMenuTerpilih]);
        } else if (subMenuTypes[subMenuTerpilih] == 'G') { 
          appState = 10; 
          gameOver = false; 
          gameScore = 0; 
          bestDino = bacaBestDariSD("/best_dino.txt");
          groundOffset = 0;
          dinoY = 38; 
          dinoVy = 0; 
          obsX = 128;
        } else if (subMenuTypes[subMenuTerpilih] == 'F') {
          appState = 21;
          flappyGameOver = false;
          flappyScore = 0;
          bestFlappy = bacaBestDariSD("/best_flappy.txt");
          flappyGroundOffset = 0;
          birdY = 30.0;
          birdVy = 0.0;
          pipeX = 128;
          pipeGapY = random(18, 34);
        } else if (subMenuTypes[subMenuTerpilih] == 'R') {
          appState = 11; 
          px = 1.5; 
          py = 1.5; 
          pa = 0.0; 
        } else {
          playAnimation(subMenuFiles[subMenuTerpilih].c_str(), subMenuTypes[subMenuTerpilih], subMenuDurations[subMenuTerpilih]);
        }
        if (appState != 10 && appState != 11 && appState != 21) { 
          gambarSubMenu(); 
          delay(200); 
        } 
      }
      webSelect = false;
    }
    if (webBack) { 
      webBack = false; 
      appState = 0; 
      gambarMenu(); 
      delay(200); 
    }
  } else if (appState == 3) { 
    tampilkanJam(); 
    if (btnUpFisik || webBack) { 
      appState = 0; 
      gambarMenu(); 
      delay(300); 
      webBack = false; 
    }
  } else if (appState == 4) { 
    if (btnUpFisik || webUp) { 
      if (wifiMenuTerpilih > 0) wifiMenuTerpilih--; 
      else wifiMenuTerpilih = 1; 
      gambarMenuWifiGagal(); 
      delay(200); 
      webUp = false; 
    }
    if (btnDownFisik || webDown) { 
      if (wifiMenuTerpilih < 1) wifiMenuTerpilih++; 
      else wifiMenuTerpilih = 0; 
      gambarMenuWifiGagal(); 
      delay(200); 
      webDown = false; 
    }
    if (btnSelectFisik || webSelect) { 
      if (wifiMenuTerpilih == 0) { 
        if (konekWiFi()) { 
          appState = 0; 
          gambarMenu(); 
        } else { 
          gambarMenuWifiGagal(); 
        } 
      } else { 
        WiFi.disconnect(true); 
        WiFi.mode(WIFI_OFF); 
        appState = 0; 
        gambarMenu(); 
      }
      delay(200); 
      webSelect = false;
    }
    if (webBack) { 
      webBack = false; 
      WiFi.disconnect(true); 
      WiFi.mode(WIFI_OFF); 
      appState = 0; 
      gambarMenu(); 
      delay(200); 
    }
  } else if (appState == 6) { 
    tampilkanTimer();
    if (digitalRead(BTN3) == LOW || webSelect) {
      if (!isTimerRunning) { 
        swStartTime = millis() - swElapsedTime; 
        isTimerRunning = true; 
      } else { 
        isTimerRunning = false; 
      }
      webSelect = false; 
      delay(300);
    }
    if (isTimerRunning) { 
      swElapsedTime = millis() - swStartTime; 
    }
    if (digitalRead(BTN1) == LOW || webBack) {
      isTimerRunning = false; 
      swElapsedTime = 0; 
      appState = 0; 
      gambarMenu(); 
      webBack = false; 
      delay(300);
    }
  } else if (appState == 7) { 
    u8g2.clearBuffer(); 
    u8g2.setFont(u8g2_font_7x14B_tr); 
    u8g2.drawStr(30, 30, "ALARM!"); 
    u8g2.drawStr(30, 48, "B1 Stop"); 
    u8g2.sendBuffer();
    delay(500); 
    u8g2.clearBuffer(); 
    u8g2.sendBuffer(); 
    delay(500); 
    if (btnUpFisik || btnDownFisik || btnSelectFisik || webSelect || webBack) {
      webSelect = false; 
      webBack = false; 
      appState = 0; 
      gambarMenu(); 
      delay(300);
    }
  } else if (appState == 8) { 
    if (alarmMenuState == 0) {
      if (btnUpFisik || webUp) { 
        if (alarmTerpilih > 0) alarmTerpilih--; 
        else alarmTerpilih = totalAlarm - 1; 
        tampilkanMenuAlarm(); 
        delay(200); 
        webUp = false; 
      }
      if (btnDownFisik || webDown) { 
        if (alarmTerpilih < totalAlarm - 1) alarmTerpilih++; 
        else alarmTerpilih = 0; 
        tampilkanMenuAlarm(); 
        delay(200); 
        webDown = false; 
      }
      if (btnSelectFisik || webSelect) {
        if (totalAlarm > 0) { 
          alarmMenuState = 1; 
          alarmActionChoice = 0; 
          tampilkanMenuAlarm(); 
        }
        delay(200); 
        webSelect = false;
      }
      if (btnUpFisik || webBack) { 
        appState = 0; 
        gambarMenu(); 
        delay(300); 
        webBack = false; 
      }
    } else if (alarmMenuState == 1) {
      if (btnUpFisik || webUp) { 
        alarmActionChoice = 0; 
        tampilkanMenuAlarm(); 
        delay(200); 
        webUp = false; 
      }
      if (btnDownFisik || webDown) { 
        alarmActionChoice = 1; 
        tampilkanMenuAlarm(); 
        delay(200); 
        webDown = false; 
      }
      if (btnSelectFisik || webSelect) {
        if (alarmActionChoice == 0) { 
          alarmActive[alarmTerpilih] = !alarmActive[alarmTerpilih]; 
          simpanAlarmKeSD(); 
          alarmMenuState = 0; 
          tampilkanMenuAlarm(); 
        } else { 
          alarmMenuState = 2; 
          alarmDeleteChoice = 0; 
          tampilkanMenuAlarm(); 
        }
        delay(200); 
        webSelect = false;
      }
      if (webBack) { 
        webBack = false; 
        alarmMenuState = 0; 
        tampilkanMenuAlarm(); 
        delay(200); 
      }
    } else if (alarmMenuState == 2) {
      if (btnUpFisik || webUp) { 
        alarmDeleteChoice = 0; 
        tampilkanMenuAlarm(); 
        delay(200); 
        webUp = false; 
      }
      if (btnDownFisik || webDown) { 
        alarmDeleteChoice = 1; 
        tampilkanMenuAlarm(); 
        delay(200); 
        webDown = false; 
      }
      if (btnSelectFisik || webSelect) {
        if (alarmDeleteChoice == 1) {
          for (int i = alarmTerpilih; i < totalAlarm - 1; i++) {
            alarmYear[i] = alarmYear[i+1];
            alarmMonth[i] = alarmMonth[i+1];
            alarmDay[i] = alarmDay[i+1];
            alarmHour[i] = alarmHour[i+1];
            alarmMinute[i] = alarmMinute[i+1];
            alarmActive[i] = alarmActive[i+1];
          }
          totalAlarm--;
          simpanAlarmKeSD();
          if (alarmTerpilih >= totalAlarm && totalAlarm > 0) alarmTerpilih = totalAlarm - 1;
          else if (totalAlarm == 0) alarmTerpilih = 0;
        }
        alarmMenuState = 0; 
        tampilkanMenuAlarm(); 
        delay(200); 
        webSelect = false;
      }
      if (webBack) { 
        webBack = false; 
        alarmMenuState = 0; 
        tampilkanMenuAlarm(); 
        delay(200); 
      }
    }
  } else if (appState == 9) { 
    tampilkanKalkulator();
    if (webOpPlus) { calcOp = '+'; webOpPlus = false; }
    if (webOpMinus) { calcOp = '-'; webOpMinus = false; }
    if (webOpMul) { calcOp = '*'; webOpMul = false; }
    if (webOpDiv) { calcOp = '/'; webOpDiv = false; }
    if (btnUpFisik || webUp) {
      if (calcState == 0) { 
        calcNum1 = incrementLastDigit(calcNum1); 
      } else if (calcState == 1) { 
        if (calcOp == '+') calcOp = '-'; 
        else if (calcOp == '-') calcOp = '*'; 
        else if (calcOp == '*') calcOp = '/'; 
        else if (calcOp == '/') calcOp = '+'; 
      } else if (calcState == 2) { 
        calcNum2 = incrementLastDigit(calcNum2); 
      } else if (calcState == 3) { 
        calcState = 0; 
        calcNum1 = "0"; 
        calcNum2 = "0"; 
        calcOp = '+'; 
      }
      delay(200); 
      webUp = false;
    }
    if (btnDownFisik || webDown) {
      if (calcState == 0) { 
        if (calcNum1 != "0" && calcNum1.length() < 5) calcNum1 += "0"; 
      } else if (calcState == 1) { 
        if (calcOp == '+') calcOp = '/'; 
        else if (calcOp == '/') calcOp = '*'; 
        else if (calcOp == '*') calcOp = '-'; 
        else if (calcOp == '-') calcOp = '+'; 
      } else if (calcState == 2) { 
        if (calcNum2 != "0" && calcNum2.length() < 5) calcNum2 += "0"; 
      }
      delay(200); 
      webDown = false;
    }
    static unsigned long btn3PressTime = 0;
    if (btnSelectFisik) {
      if (btn3PressTime == 0) btn3PressTime = millis();
      if (millis() - btn3PressTime > 1000) {
        btn3PressTime = 0; 
        appState = 0; 
        gambarMenu(); 
        delay(300); 
        return;
      }
    } else {
      if (btn3PressTime > 0 && millis() - btn3PressTime < 1000) {
        if (calcState == 0) { 
          calcState = 1; 
        } else if (calcState == 1) { 
          calcState = 2; 
          calcNum2 = "0"; 
        } else if (calcState == 2) { 
          calcState = 3; 
        } else if (calcState == 3) { 
          calcState = 0; 
          calcNum1 = "0"; 
          calcNum2 = "0"; 
          calcOp = '+'; 
        }
        delay(200); 
      }
      btn3PressTime = 0;
    }
    if (webSelect) {
      if (calcState == 0) { 
        calcState = 1; 
      } else if (calcState == 1) { 
        calcState = 2; 
        calcNum2 = "0"; 
      } else if (calcState == 2) { 
        calcState = 3; 
      } else if (calcState == 3) { 
        calcState = 0; 
        calcNum1 = "0"; 
        calcNum2 = "0"; 
        calcOp = '+'; 
      }
      webSelect = false; 
      delay(200);
    }
    if (webBack) { 
      webBack = false; 
      appState = 0; 
      gambarMenu(); 
      delay(200); 
    }
  } else if (appState == 10) { 
    if (gameOver) {
      tampilkanGameDino();
      if (btnUpFisik || webSelect) { 
        gameOver = false; 
        gameScore = 0; 
        groundOffset = 0;
        dinoY = 38; 
        dinoVy = 0; 
        obsX = 128; 
        webSelect = false; 
        delay(250);
      }
      if (btnDownFisik || webBack) { 
        appState = 1; 
        gambarSubMenu(); 
        webBack = false; 
        delay(250);
      }
    } else {
      int speedStep = 3 + (gameScore / 20);
      if (speedStep > 10) speedStep = 10;

      dinoVy += 2; 
      dinoY += dinoVy;
      if (dinoY > 38) { 
        dinoY = 38; 
        dinoVy = 0; 
      }
      
      obsX -= speedStep; 
      groundOffset = (groundOffset + speedStep) % 8;
      if (obsX < -10) { 
        obsX = 128; 
        gameScore++; 
        if (gameScore > bestDino) {
          bestDino = gameScore;
          simpanBestKeSD("/best_dino.txt", bestDino);
        }
      }
      
      if (obsX < 24 && obsX > 10 && dinoY > 30) { 
        gameOver = true; 
      }
      
      if (btnUpFisik || webUp) {
        if (dinoY >= 38) { 
          dinoVy = -12; 
        }
        webUp = false;
      }
      
      if (webBack) {
        appState = 1; 
        gambarSubMenu(); 
        webBack = false;
      }
      
      tampilkanGameDino();
      delay(45); 
    }
  } else if (appState == 21) {
    if (flappyGameOver) {
      tampilkanFlappyBird();
      if (btnUpFisik || webSelect) {
        flappyGameOver = false;
        flappyScore = 0;
        flappyGroundOffset = 0;
        birdY = 30.0;
        birdVy = 0.0;
        pipeX = 128;
        pipeGapY = random(18, 34);
        webSelect = false;
        delay(250);
      }
      if (btnDownFisik || webBack) {
        appState = 1;
        gambarSubMenu();
        webBack = false;
        delay(250);
      }
    } else {
      if (btnUpFisik || webUp) {
        birdVy = -2.8f;
        webUp = false;
      }

      birdVy += 0.45f;
      if (birdVy > 3.2f) birdVy = 3.2f;
      if (birdVy < -3.2f) birdVy = -3.2f;
      birdY += birdVy;

      int pSpeed = 3 + (flappyScore / 10);
      if (pSpeed > 6) pSpeed = 6;
      pipeX -= pSpeed;
      flappyGroundOffset = (flappyGroundOffset + pSpeed) % 4;

      if (pipeX < -10) {
        pipeX = 128;
        pipeGapY = random(18, 34);
        flappyScore++;
        if (flappyScore > bestFlappy) {
          bestFlappy = flappyScore;
          simpanBestKeSD("/best_flappy.txt", bestFlappy);
        }
      }

      if (birdY < 11) {
        birdY = 11;
        flappyGameOver = true;
      }
      if (birdY + 5 > 55) {
        birdY = 50;
        flappyGameOver = true;
      }

      if (pipeX < 31 && pipeX + 10 > 24) {
        if (birdY < pipeGapY || birdY + 5 > pipeGapY + pipeGapH) {
          flappyGameOver = true;
        }
      }

      if (webBack) {
        appState = 1;
        gambarSubMenu();
        webBack = false;
      }

      tampilkanFlappyBird();
      delay(40);
    }
  } else if (appState == 11) {
    tampilkanRaycaster();
    
    if (px >= 38.0 && px < 39.0 && py >= 38.0 && py < 39.0) {
      appState = 13; 
    } else if (digitalRead(BTN1) == LOW && digitalRead(BTN2) == LOW && digitalRead(BTN3) == LOW) {
      appState = 1;
      gambarSubMenu();
      delay(500);
      while(digitalRead(BTN1) == LOW || digitalRead(BTN2) == LOW || digitalRead(BTN3) == LOW) { 
        delay(10); 
      }
    } else if (digitalRead(BTN1) == LOW && digitalRead(BTN2) == LOW) {
      float nx = px - cos(pa) * 0.15;
      float ny = py - sin(pa) * 0.15;
      if (nx >= 0 && nx < MAP_W && ny >= 0 && ny < MAP_H) {
        if (mazeMap[(int)ny][(int)nx] == 0) {
          px = nx; 
          py = ny;
        }
      }
    } else {
      if (btnUpFisik || webUp) {
        float nx = px + cos(pa) * 0.15;
        float ny = py + sin(pa) * 0.15;
        if (nx >= 0 && nx < MAP_W && ny >= 0 && ny < MAP_H) {
          if (mazeMap[(int)ny][(int)nx] == 0) {
            px = nx; 
            py = ny;
          }
        }
      }
      if (btnDownFisik || webDown) { 
        pa -= 0.1; 
      } 
      if (btnSelectFisik || webSelect) { 
        pa += 0.1; 
      } 
      if (webBack) { 
        appState = 1; 
        gambarSubMenu(); 
        webBack = false; 
        delay(300); 
      }
    }
  } else if (appState == 13) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawStr(20, 35, "FINISH!");
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(20, 50, "B1/B3: Kembali");
    u8g2.sendBuffer();
    
    if (btnUpFisik || btnSelectFisik || webBack || webSelect) {
      appState = 1; 
      gambarSubMenu();
      delay(300);
    }
  }
}
