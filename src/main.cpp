#include <WiFi.h>
#include <CTBot.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>

// Ganti dengan nama WiFi dan password Anda
// const char *ssid = "Jakanet";         // Nama WiFi
// const char *password = "cicinghutan"; // Password WiFi

// Inisialisasi OLED Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Ganti dengan token bot Anda
String botToken = "7548353591:AAER0KSfivNf4wQjLDtLOP4HU8fwCMtugMk"; // Bot token dari BotFather

// Ganti dengan chat ID pribadi yang benar
int64_t chat_id = 460734701; // Chat ID dari chat pribadi Anda dengan bot

CTBot myBot;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 8 * 3600, 60000); // +7 timezone (misalnya WIB)
unsigned long previousMillis = 0;
const long intervalTime = 1000;
unsigned long previousMillisSensor = 0; // Variabel untuk melacak waktu terakhir pembacaan sensor
const long intervalSensor = 5000;       // Interval 5 detik untuk pembacaan sensor

// Pin analog untuk membaca MQ-2
const int analogPinMQ2 = 34; // Pin analog tempat MQ-2 terhubung
// Pin analog untuk membaca sensor flame KY-026
const int analogPinFlame = 35; // Pin analog tempat KY-026 terhubung
// Pin untuk buzzer
const int buzzerPin = 13; // Pin digital untuk mengontrol buzzer

// Pin untuk LED RGB
const int redPin = 14;   // Pin untuk warna merah
const int greenPin = 15; // Pin untuk warna hijau
const int bluePin = 16;  // Pin untuk warna biru

bool wifiConnected = false;
TBMessage msg;

// Deklarasi Prototipe Fungsi
void setLEDColor(int red, int green, int blue);

void displayTimeOnOLED(String currentTime, String ipAddress)
{
  display.clearDisplay();              // Bersihkan layar
  display.setTextSize(0.5);            // Set ukuran font
  display.setTextColor(SSD1306_WHITE); // Set warna teks menjadi putih
  display.setCursor(0, 0);             // Set posisi kursor (X, Y)
  display.print("IP: ");
  display.println(ipAddress); // Tampilkan IP address

  display.setTextSize(2);
  display.setCursor(10, 20);    // Set posisi kursor (X, Y)
  display.println(currentTime); // Tampilkan waktu saat ini

  display.setTextSize(2);
  display.setCursor(15, 50);
  display.println("Hello :)");
  display.display(); // Refresh layar untuk menampilkan konten
}

void displayWarningOnOLED(String warningMessage, int sensorValueFlame, int sensorValueMQ2, float voltageMQ2)
{
  display.clearDisplay();              // Bersihkan layar
  display.setTextSize(1);              // Set ukuran font lebih besar untuk pesan peringatan
  display.setTextColor(SSD1306_WHITE); // Set warna teks menjadi putih
  display.setTextSize(2);
  display.setCursor(5, 0);       // Set posisi kursor (X, Y)
  display.println("WARNING!!!"); // Tampilkan pesan peringatan
  display.setTextSize(1);
  display.setCursor(0, 30);        // Atur posisi teks selanjutnya
  display.println(warningMessage); // Tampilkan pesan peringatan spesifik (api/gas)
  String flameInfo = "Api: " + String(sensorValueFlame);
  String gasInfo = "Gas: " + String(sensorValueMQ2) + " (" + voltageMQ2 + "V)"; // Menampilkan 2 desimal

  display.setCursor(0, 20);   // Set posisi kursor untuk menampilkan nilai sensor api
  display.println(flameInfo); // Tampilkan nilai sensor api

  display.setCursor(0, 40); // Set posisi kursor untuk menampilkan nilai sensor gas
  display.println(gasInfo); // Tampilkan nilai sensor gas
  display.display();        // Refresh layar untuk menampilkan konten
}

void setup()
{
  // Inisialisasi serial monitor
  Serial.begin(115200);

  // Inisialisasi OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // I2C address 0x3C
    Serial.println(F("Gagal menginisialisasi OLED"));
    for (;;)
      ; // Loop tak terbatas jika gagal inisialisasi
  }
  display.display(); // Tampilkan konten awal
  delay(2000);       // Jeda selama 2 detik

  // Bersihkan buffer display
  display.clearDisplay();

  // Inisialisasi pin analog untuk MQ-2, sensor flame, dan buzzer
  pinMode(analogPinMQ2, INPUT);
  pinMode(analogPinFlame, INPUT);
  pinMode(buzzerPin, OUTPUT);

  // Setup PWM untuk RGB LED
  ledcSetup(0, 5000, 8); // Channel 0, 5 kHz, 8-bit resolution
  ledcSetup(1, 5000, 8); // Channel 1, 5 kHz, 8-bit resolution
  ledcSetup(2, 5000, 8); // Channel 2, 5 kHz, 8-bit resolution

  ledcAttachPin(redPin, 0);   // Sambungkan pin merah ke channel 0
  ledcAttachPin(greenPin, 1); // Sambungkan pin hijau ke channel 1
  ledcAttachPin(bluePin, 2);  // Sambungkan pin biru ke channel 2

  // Pastikan buzzer dan LED dalam keadaan mati pada awalnya
  digitalWrite(buzzerPin, LOW);
  setLEDColor(0, 0, 0); // Matikan semua warna LED

  // // Mulai koneksi ke WiFi
  WiFiManager wifiManager;

  // Coba untuk auto-connect
  if (!wifiManager.autoConnect("ESP32_AP"))
  {
    Serial.println("Gagal terhubung, rebooting...");
    delay(3000);
    ESP.restart();
  }
  // Serial.println("Menghubungkan ke WiFi...");
  // WiFi.begin(ssid, password);

  // Tunggu hingga terhubung
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000)
  {
    setLEDColor(0, 0, 255); // Warna biru saat mencoba terhubung
    delay(500);             // Kedip LED
    setLEDColor(0, 0, 0);   // Matikan LED
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    // WiFi berhasil terhubung
    wifiConnected = true;

    Serial.println("");
    Serial.println("Terhubung ke WiFi!");
    Serial.print("Alamat IP: ");
    Serial.println(WiFi.localIP());

    // Inisialisasi bot Telegram
    // Inisialisasi bot Telegram
    if (WiFi.status() == WL_CONNECTED)
    {
      myBot.wifiConnect(WiFi.SSID().c_str(), WiFi.psk().c_str()); // Menggunakan SSID dan password dari WiFi yang terhubung
    }
    else
    {
      Serial.println("WiFi tidak terhubung!");
      // Anda bisa menambahkan tindakan jika WiFi tidak terhubung
    }
    myBot.setTelegramToken(botToken);
    // myBot.wifiConnect(ssid, password); // Pastikan WiFi terhubung
    // myBot.setTelegramToken(botToken);  // Setel token bot

    // Periksa apakah terhubung ke bot Telegram
    if (myBot.testConnection())
    {
      Serial.println("Terhubung ke bot Telegram!");
    }
    else
    {
      Serial.println("Gagal terhubung ke bot Telegram.");
    }

    // Set LED hijau saat kondisi normal (terhubung ke WiFi)
    setLEDColor(0, 255, 0); // Warna hijau (normal)

    // Buat pesan yang akan dikirim, menginformasikan koneksi WiFi berhasil
    String message = "ESP32 terhubung ke WiFi: ";
    message += WiFi.SSID();
    message += "\nAlamat IP: ";
    message += WiFi.localIP().toString();

    // Kirim pesan ke Telegram
    bool success = myBot.sendMessage(chat_id, message);

    if (success)
    {
      Serial.println("Pesan berhasil dikirim!");
    }
    else
    {
      Serial.println("Gagal mengirim pesan.");
    }
  }
  else
  {
    // Gagal terhubung ke WiFi, set LED biru sebagai indikasi
    wifiConnected = false;
    Serial.println("Gagal terhubung ke WiFi. Sistem tetap berjalan tanpa koneksi.");

    // Jika tidak terhubung, atur LED biru
    setLEDColor(0, 0, 255); // LED biru ketika tidak terhubung
  }
  // Dapatkan IP address dari WiFi
  String ipAddress = WiFi.localIP().toString();
  timeClient.begin();
  timeClient.update();
  Serial.print("Alamat IP: ");
  Serial.println(ipAddress);
}

void loop()
{
  unsigned long currentMillis = millis(); // Ambil waktu saat ini

  // Perbarui waktu setiap 1 detik
  if (currentMillis - previousMillis >= intervalTime)
  {
    previousMillis = currentMillis; // Simpan waktu terakhir pembaruan

    // Perbarui waktu dari server NTP
    timeClient.update(); // Memperbarui waktu secara berkala di loop()

    // Ambil jam saat ini dalam format HH:MM:SS
    String formattedTime = timeClient.getFormattedTime();
    // Ambil IP address dari WiFi
    String ipAddress = WiFi.localIP().toString();

    // Tampilkan waktu di OLED
    displayTimeOnOLED(formattedTime, ipAddress);
  }
  // Cek status koneksi WiFi secara terus-menerus
  if (WiFi.status() != WL_CONNECTED)
  {
    if (wifiConnected)
    {
      // Jika WiFi baru saja terputus
      wifiConnected = false;
      setLEDColor(0, 0, 255); // Set LED menjadi biru
      Serial.println("WiFi terputus!");
    }
    else
    {
      wifiConnected = true;
      setLEDColor(0, 0, 255); // Set LED menjadi hijau
      Serial.println("WiFi gagal terhubung");
    }
  }

  if (currentMillis - previousMillisSensor >= intervalSensor)
  {
    previousMillisSensor = currentMillis;

    // Membaca nilai dari sensor MQ-2
    int sensorValueMQ2 = analogRead(analogPinMQ2);

    // Membaca nilai dari sensor flame KY-026 (pin analog)
    int sensorValueFlame = analogRead(analogPinFlame);

    // Konversi nilai sensor MQ-2 menjadi tegangan (0 - 3.3V)
    float voltageMQ2 = sensorValueMQ2 * (3.3 / 4095.0); // 4095 adalah resolusi ADC pada ESP32

    // Menampilkan nilai sensor di Serial Monitor
    Serial.print("Nilai sensor MQ-2: ");
    Serial.print(sensorValueMQ2);
    Serial.print(" - Tegangan: ");
    Serial.print(voltageMQ2);
    Serial.println(" V");

    // Menampilkan nilai sensor flame di Serial Monitor
    Serial.print("Nilai sensor api: ");
    Serial.println(sensorValueFlame);

    // Deteksi api (misalnya jika nilai sensor flame lebih rendah dari ambang batas tertentu)
    if (sensorValueFlame < 500)
    { // Misalnya, jika nilai analog flame kurang dari 300, api terdeteksi
      Serial.println("Api terdeteksi!");
      // digitalWrite(buzzerPin, HIGH); // Aktifkan buzzer
      unsigned long startTime = millis(); // Catat waktu mulai
      while (millis() - startTime < 10000)
      {                                // Loop selama 10 detik
        digitalWrite(buzzerPin, HIGH); // Buzzer ON
        setLEDColor(255, 0, 0);        // LED merah ON
        delay(200);                    // Tunda 200ms

        digitalWrite(buzzerPin, LOW); // Buzzer OFF
        setLEDColor(0, 0, 255);       // LED biru ON
        delay(300);                   // Tunda 300ms
      }

      // Kirim pesan peringatan ke Telegram
      String alertMessage = "Peringatan! Api terdeteksi!";
      alertMessage += "\nNilai sensor: ";
      alertMessage += String(sensorValueFlame);
      displayWarningOnOLED("Api terdeteksi", sensorValueFlame, sensorValueMQ2, (int)(voltageMQ2 * 1000)); // Tampilkan peringatan di OLED
      bool success = myBot.sendMessage(chat_id, alertMessage);

      if (success)
      {
        Serial.println("Pesan peringatan api berhasil dikirim!");
      }
      else
      {
        Serial.println("Gagal mengirim pesan peringatan api.");
      }

      // Tunggu sebentar sebelum pengecekan berikutnya
      delay(3000); // 10 detik jeda setelah peringatan dikirim
    }
    else if (sensorValueMQ2 > 2000)
    { // Jika nilai sensor gas MQ-2 melebihi ambang batas
      String alertMessage = "Peringatan! Gas terdeteksi!\n";
      alertMessage += "Nilai sensor: ";
      alertMessage += String(sensorValueMQ2);
      alertMessage += "\nTegangan: ";
      alertMessage += String(voltageMQ2) + " V";
      displayWarningOnOLED("Gas Terdeteksi", sensorValueFlame, sensorValueMQ2, (int)(voltageMQ2 * 1000)); // Ta,mpilkan peringatan di OLED

      // Aktifkan buzzer dan LED merah berkedip
      // digitalWrite(buzzerPin, HIGH);
      unsigned long startTime = millis(); // Catat waktu mulai
      while (millis() - startTime < 10000)
      {                                // Loop selama 10 detik
        digitalWrite(buzzerPin, HIGH); // Buzzer ON
        setLEDColor(255, 0, 0);        // LED merah ON
        delay(200);                    // Tunda 200ms

        digitalWrite(buzzerPin, LOW); // Buzzer OFF
        setLEDColor(0, 0, 0);         // LED mati
        delay(300);                   // Tunda 300ms
      }

      // Kirim pesan peringatan ke Telegram
      bool success = myBot.sendMessage(chat_id, alertMessage);

      if (success)
      {
        Serial.println("Pesan peringatan berhasil dikirim!");
      }
      else
      {
        Serial.println("Gagal mengirim pesan peringatan.");
      }

      // Tunggu sebentar sebelum pengecekan berikutnya
      delay(3000); // 10 detik jeda setelah peringatan dikirim
    }
    else
    {
      // Matikan buzzer dan kembali ke kondisi normal (LED hijau)
      digitalWrite(buzzerPin, LOW);
      setLEDColor(0, 255, 0); // Warna hijau (normal)
      Serial.println("Tidak ada gas atau api terdeteksi, buzzer mati.");
    }
  }
}

// Fungsi untuk mengatur warna LED RGB menggunakan PWM
void setLEDColor(int red, int green, int blue)
{
  ledcWrite(0, red);   // Atur intensitas warna merah
  ledcWrite(1, green); // Atur intensitas warna hijau
  ledcWrite(2, blue);  // Atur intensitas warna biru
}
