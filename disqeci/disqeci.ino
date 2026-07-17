#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

const int btnUp = 2, btnDown = 3, btnOk = 6, btnBack = 7;
int menuIndex = 0;
const int menuMax = 4;
String menuItems[] = {"Sinyal Tara", "Ayarlar", "Bilgi", "ILETISIM", "EEPROM AKTAR"};

struct Kanal {
  char ad[16];
  long frekans;
};
Kanal sonKayit = {"Bos", 0};

void setup() {
  Serial.begin(9600);
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(btnUp, INPUT_PULLUP); pinMode(btnDown, INPUT_PULLUP);
  pinMode(btnOk, INPUT_PULLUP); pinMode(btnBack, INPUT_PULLUP);

  showSplash();
  drawMenu();
}

void showSplash() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  // Merkezlenmiş Başlangıç
  display.setCursor(38, 20); display.print("OBM32");
  display.setCursor(20, 35); display.print("OmfoBalkan Elec.");
  display.display();
  delay(2000);
}

void drawMenu() {
  display.clearDisplay();
  display.setCursor(25, 0); display.print("OBM32 Disqeci");
  display.drawFastHLine(0, 10, 128, WHITE);
  for (int i = 0; i <= menuMax; i++) {
    display.setCursor(25, 20 + (i * 9));
    if (i == menuIndex) display.print("> ");
    display.print(menuItems[i]);
  }
  display.display();
}

void loop() {
  // Serialden KanalAdi,Frekans verisi alma
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    int commaIndex = data.indexOf(',');
    if (commaIndex != -1) {
      data.substring(0, commaIndex).toCharArray(sonKayit.ad, 16);
      sonKayit.frekans = data.substring(commaIndex + 1).toInt();
    }
  }

  // Menü Gezinme
  if (digitalRead(btnUp) == LOW) { menuIndex = (menuIndex == 0) ? menuMax : menuIndex - 1; drawMenu(); delay(200); }
  if (digitalRead(btnDown) == LOW) { menuIndex = (menuIndex == menuMax) ? 0 : menuIndex + 1; drawMenu(); delay(200); }
  
  if (digitalRead(btnOk) == LOW) {
    while(true) {
      if (menuIndex == 0) signalScan();
      else if (menuIndex == 1) settingsMenu();
      else if (menuIndex == 2) infoMenu();
      else if (menuIndex == 3) iletisimMenu();
      else if (menuIndex == 4) eepromAktar();
      
      if (digitalRead(btnBack) == LOW) { drawMenu(); break; }
      delay(50);
    }
  }
}

// --- Fonksiyonlar ---

void signalScan() {
  display.clearDisplay();
  display.setCursor(22, 0); display.print("Sinyal Seviyesi");
  int signal = random(20, 100); 
  display.drawRect(0, 20, 128, 20, WHITE);
  display.fillRect(2, 22, (signal * 124 / 100), 16, WHITE);
  display.setCursor(55, 45); display.print(signal); display.print("%");
  display.display();
}

void settingsMenu() {
  display.clearDisplay();
  display.setCursor(20, 0); display.print("Kanal Ayarlari");
  display.setCursor(0, 20); display.print("Kayit: " + String(sonKayit.ad));
  display.setCursor(0, 35); display.print("Freq: " + String(sonKayit.frekans));
  display.display();
}

void infoMenu() {
  display.clearDisplay();
  display.setCursor(45, 15); display.print("OBM32");
  display.setCursor(48, 30); display.print("V1.0");
  display.display();
}

void iletisimMenu() {
  display.clearDisplay();
  display.setCursor(35, 0); display.print("ILETISIM");
  display.setCursor(15, 25); display.print("meteonelge0127");
  display.setCursor(28, 40); display.print("barbaror4");
  display.display();
}

void eepromAktar() {
  display.clearDisplay();
  display.setCursor(10, 20); display.print("EEPROM'A KAYDEDILDI");
  
  // I2C 24LC128 EEPROM Yazma
  Wire.beginTransmission(0x50);
  Wire.write(0x00); Wire.write(0x00);
  Wire.write((byte*)&sonKayit, sizeof(sonKayit));
  Wire.endTransmission();
  
  display.display();
  delay(1500);
}