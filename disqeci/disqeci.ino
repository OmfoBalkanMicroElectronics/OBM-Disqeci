
/*
 * OBM32 UYDU SINYAL TAKIP - v3.4
 * 
 * 20 MENU - Her menude 10 icerik
 * Uzun basma = hizli scroll
 * TUM STRINGLER F() MAKROSU ILE FLASH'TA
 * SPEKTRUM: millis tabanli pseudo-random, gercekci bar grafik
 *
 * BACK=D3  UP=D4  DOWN=D5  OK=D6
 * LED: D8,D9,D10,D11 (330 ohm!)
 * BUZZER: D12
 * LCD I2C: A4/A5 adres 0x27
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

#define BK 3
#define UP 4
#define DN 5
#define OK 6
#define L1 8
#define L2 9
#define L3 10
#define L4 11
#define BZ 12

LiquidCrystal_I2C lcd(0x27, 20, 4);

#define EE_OK   0
#define EE_FREQ 1
#define EE_POL  5
#define EE_SR   6
#define EE_DIS  8
#define EE_MOT  9
#define EE_LNB  11
#define EE_VOL  12
#define EE_LOG  13
#define EE_MUTE 14
#define MAGIC   0xAB

// 20 MENU (PROGMEM)
const char m0[]  PROGMEM = "TKGS Ayarlari";
const char m1[]  PROGMEM = "Uydu Ekle";
const char m2[]  PROGMEM = "Kanal Listesi";
const char m3[]  PROGMEM = "TP Listesi";
const char m4[]  PROGMEM = "DiSEqC Ayar";
const char m5[]  PROGMEM = "Kanal Kaydet";
const char m6[]  PROGMEM = "Sinyal Analiz";
const char m7[]  PROGMEM = "Motor Ayarlari";
const char m8[]  PROGMEM = "LNB Ayarlari";
const char m9[]  PROGMEM = "Zaman Bilgisi";
const char m10[] PROGMEM = "Ses Ayarlari";
const char m11[] PROGMEM = "Ekran Ayarlari";
const char m12[] PROGMEM = "Log Kayitlari";
const char m13[] PROGMEM = "Anten Ayarlari";
const char m14[] PROGMEM = "Spektrum";
const char m15[] PROGMEM = "Firmware";
const char m16[] PROGMEM = "Dil Secenekleri";
const char m17[] PROGMEM = "Ag Ayarlari";
const char m18[] PROGMEM = "Sistem Bilgisi";
const char m19[] PROGMEM = "Fabrika Ayar";

const char* const mnTbl[] PROGMEM = {
  m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,
  m10,m11,m12,m13,m14,m15,m16,m17,m18,m19
};
#define MNUM 20

char buf[21];

void getMn(byte i) {
  strcpy_P(buf, (char*)pgm_read_word(&(mnTbl[i])));
}

// Durum
byte cur = 0, mOfs = 0, sIdx = 0, sOfs = 0;
bool inSub = false, edt = false;
byte resetR = 0;

// Uydu
unsigned long sFreq = 11096;
byte sPol = 0;
unsigned int sSR = 30000;
byte sDis = 0;
int sMot = 0;
byte sLNB = 13, sVol = 50;
bool sLog = false, sMute = false;

// Sinyal
int sLev = 0, sQua = 0;
float sVolA = 0.0;

// Buton
byte bP[4] = {UP, DN, OK, BK};
bool bO[4] = {1,1,1,1};
unsigned long bT[4] = {0,0,0,0};
unsigned long bSt[4] = {0,0,0,0};
bool bH[4] = {0,0,0,0};

#define DEB   180
#define HOLD  400
#define REPT  80

unsigned long tS = 0, tL = 0, tR = 0;
char cmdBuf[12];
byte cmdLen = 0;

// Spektrum icin statik degiskenler
static unsigned long spekLastT = 0;
static byte spekVals[18];

void setup() {
  resetR = MCUSR; MCUSR = 0;
  Serial.begin(115200);
  Serial.println(F("OBM32 v3.4"));
  Serial.print(F("RAM:"));
  Serial.println(freeRam());

  for (byte i = 0; i < 4; i++) pinMode(bP[i], INPUT_PULLUP);
  pinMode(L1, OUTPUT); pinMode(L2, OUTPUT);
  pinMode(L3, OUTPUT); pinMode(L4, OUTPUT);
  pinMode(BZ, OUTPUT);

  Wire.begin();
  lcd.init(); delay(100);
  lcd.backlight(); delay(50);
  loadBarChars();
  ldEE();

  // Spektrum baslangic degerleri sifirla
  for (byte i = 0; i < 18; i++) spekVals[i] = 0;

  lcd.setCursor(1, 0); lcd.print(F("OBM32 UYDU TAKIP"));
  lcd.setCursor(6, 1); lcd.print(F("v3.4.0"));
  lcd.setCursor(0, 2); lcd.print(F("20 Menu-10 Icerik"));
  lcd.setCursor(0, 3); lcd.print(F("RAM:")); lcd.print(freeRam());

  for (byte i = 0; i < 4; i++) {
    digitalWrite(L1+i, HIGH); delay(60);
    digitalWrite(L1+i, LOW);
  }
  delay(1000);
  hdr(); showM();
}

int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

void loop() {
  chkBtn();
  unsigned long n = millis();

  if (n - tS >= 300) {
    tS = n;
    sLev = map(analogRead(A0), 0, 1023, 0, 100);
    sQua = map(analogRead(A1), 0, 1023, 0, 100);
    sVolA = analogRead(A2) * 15.0 / 1023.0;
  }

  if (inSub && n - tR >= 500) {
    tR = n;
    if (cur == 6 || cur == 14 || cur == 9 || cur == 18) sub();
  }

  if (sLog && n - tL >= 1000) {
    tL = n;
    Serial.print(n/1000); Serial.print(F(";"));
    Serial.print(sLev);   Serial.print(F(";"));
    Serial.print(sQua);   Serial.print(F(";"));
    Serial.println(sVolA, 2);
  }

  serTask();

  byte led = cur / 5;
  digitalWrite(L1, led == 0);
  digitalWrite(L2, led == 1);
  digitalWrite(L3, led == 2);
  digitalWrite(L4, led == 3);
}

void chkBtn() {
  unsigned long n = millis();
  for (byte i = 0; i < 4; i++) {
    bool s = digitalRead(bP[i]);
    if (s == LOW) {
      if (bO[i] == HIGH) bSt[i] = n;
      if (!bH[i] && bO[i] == HIGH && n - bT[i] >= DEB) {
        bT[i] = n; bip(20); btnAct(i);
      }
      if ((i < 2) && n - bSt[i] >= HOLD) {
        bH[i] = true;
        if (n - bT[i] >= REPT) { bT[i] = n; btnAct(i); }
      }
    } else { bH[i] = false; }
    bO[i] = s;
  }
}

void btnAct(byte i) {
  if (i == 0) doUp();
  else if (i == 1) doDn();
  else if (i == 2) { bip(40); doOk(); }
  else { bip(30); doBk(); }
}

void doUp() {
  if (edt) { adj(1); return; }
  if (inSub) {
    if (sIdx > 0) { sIdx--; if (sIdx < sOfs) sOfs = sIdx; }
    sub(); return;
  }
  if (cur > 0) { cur--; if (cur < mOfs) mOfs = cur; }
  showM();
}

void doDn() {
  if (edt) { adj(-1); return; }
  if (inSub) {
    if (sIdx < 9) { sIdx++; if (sIdx > sOfs + 2) sOfs = sIdx - 2; }
    sub(); return;
  }
  if (cur < MNUM - 1) { cur++; if (cur > mOfs + 2) mOfs = cur - 2; }
  showM();
}

void doOk() {
  if (edt) { edt = false; conf(); svEE(); bipOk(); delay(200); sub(); return; }
  if (inSub) { edt = true; sub(); return; }
  inSub = true; sIdx = 0; sOfs = 0; sub();
}

void doBk() {
  if (edt) { edt = false; sub(); return; }
  if (inSub) { inSub = false; sIdx = 0; sOfs = 0; edt = false; hdr(); showM(); }
}

void conf() {
  if (cur == 5) { lcd.setCursor(0, 3); lcd.print(F(" Kaydedildi!        ")); }
  if (cur == 6 || cur == 12) {
    sLog = !sLog;
    if (sLog) Serial.println(F("ZAMAN;SINYAL;KALITE;VOLTAJ"));
  }
  if (cur == 8)  sLNB = (sLNB == 13) ? 18 : 13;
  if (cur == 10) sMute = !sMute;
  if (cur == 19) fabR();
}

void sub() {
  lcd.setCursor(0, 0);
  lcd.print(F("["));
  getMn(cur);
  lcd.print(buf);
  lcd.print(F("]"));
  lcd.print(F("          "));
  clr();
  switch (cur) {
    case 0:  sub0();  break;
    case 1:  sub1();  break;
    case 2:  sub2();  break;
    case 3:  sub3();  break;
    case 4:  sub4();  break;
    case 5:  sub5();  break;
    case 6:  sub6();  break;
    case 7:  sub7();  break;
    case 8:  sub8();  break;
    case 9:  sub9();  break;
    case 10: sub10(); break;
    case 11: sub11(); break;
    case 12: sub12(); break;
    case 13: sub13(); break;
    case 14: sub14(); break;
    case 15: sub15(); break;
    case 16: sub16(); break;
    case 17: sub17(); break;
    case 18: sub18(); break;
    case 19: sub19(); break;
  }
}

void prI(byte row, byte idx, const __FlashStringHelper* txt) {
  lcd.setCursor(0, row);
  lcd.print(idx == sIdx ? F(">") : F(" "));
  lcd.print(txt);
  lcd.print(F("     "));
}

// TKGS
void sub0() {
  for (byte i = 0; i < 3; i++) {
    byte idx = sOfs + i; if (idx > 9) break;
    switch (idx) {
      case 0: prI(i+1, idx, F("TKGS Durumu: AKTIF")); break;
      case 1: prI(i+1, idx, F("Otomatik Guncelle"));  break;
      case 2: prI(i+1, idx, F("Manuel Guncelle"));    break;
      case 3: prI(i+1, idx, F("TKGS Versiyon: 2.1")); break;
      case 4: prI(i+1, idx, F("Kanal Senkron"));      break;
      case 5: prI(i+1, idx, F("TP Senkron"));         break;
      case 6: prI(i+1, idx, F("LCN Ayari"));          break;
      case 7: prI(i+1, idx, F("Bolge Secimi"));       break;
      case 8: prI(i+1, idx, F("TKGS Bilgi"));         break;
      case 9: prI(i+1, idx, F("TKGS Sifirla"));       break;
    }
  }
}

// Uydu Ekle
void sub1() {
  for (byte i = 0; i < 3; i++) {
    byte idx = sOfs + i; if (idx > 9) break;
    lcd.setCursor(0, i+1);
    lcd.print(idx == sIdx ? F(">") : F(" "));
    switch (idx) {
      case 0: lcd.print(F("Frek:")); lcd.print(sFreq); lcd.print(F("MHz")); break;
      case 1: lcd.print(F("Pol:")); lcd.print(sPol==0?F("H"):F("V"));       break;
      case 2: lcd.print(F("SR:")); lcd.print(sSR);                           break;
      case 3: lcd.print(F("FEC: Auto"));                                     break;
      case 4: lcd.print(F("Mod: DVB-S2"));                                   break;
      case 5: lcd.print(F("PID V: 101"));                                    break;
      case 6: lcd.print(F("PID A: 102"));                                    break;
      case 7: lcd.print(F("Uydu: TURKSAT"));                                 break;
      case 8: lcd.print(F("TP Ekle"));                                       break;
      case 9: lcd.print(F("[KAYDET]"));                                      break;
    }
    lcd.print(F("     "));
  }
  if (edt) { lcd.setCursor(19, (sIdx-sOfs)+1); lcd.print(F("*")); }
}

// Kanal Listesi
void sub2() {
  for (byte i = 0; i < 3; i++) {
    byte idx = sOfs + i; if (idx > 9) break;
    switch (idx) {
      case 0: prI(i+1, idx, F("1.TRT 1 HD"));      break;
      case 1: prI(i+1, idx, F("2.ATV HD"));         break;
      case 2: prI(i+1, idx, F("3.Show TV HD"));     break;
      case 3: prI(i+1, idx, F("4.Star TV HD"));     break;
      case 4: prI(i+1, idx, F("5.Kanal D HD"));     break;
      case 5: prI(i+1, idx, F("6.FOX TV HD"));      break;
      case 6: prI(i+1, idx, F("7.TV8 HD"));         break;
      case 7: prI(i+1, idx, F("8.NTV HD"));         break;
      case 8: prI(i+1, idx, F("9.CNN Turk"));       break;
      case 9: prI(i+1, idx, F("10.Haber Global"));  break;
    }
  }
}

// TP Listesi
void sub3() {
  for (byte i = 0; i < 3; i++) {
    byte idx = sOfs + i; if (idx > 9) break;
    switch (idx) {
      case 0: prI(i+1, idx, F("11096 H 30000")); break;
      case 1: prI(i+1, idx, F("11180 V 30000")); break;
      case 2: prI(i+1, idx, F("12015 H 27500")); break;
      case 3: prI(i+1, idx, F("12053 V 27500")); break;
      case 4: prI(i+1, idx, F("12130 H 27500")); break;
      case 5: prI(i+1, idx, F("12188 V 27500")); break;
      case 6: prI(i+1, idx, F("12245 H 27500")); break;
      case 7: prI(i+1, idx, F("12303 V 27500")); break;
      case 8: prI(i+1, idx, F("12380 H 30000")); break;
      case 9: prI(i+1, idx, F("12458 V 30000")); break;
    }
  }
}

// DiSEqC
void sub4() {
  for (byte i = 0; i < 3; i++) {
    byte idx = sOfs + i; if (idx > 9) break;
    lcd.setCursor(0, i+1);
    lcd.print(idx == sIdx ? F(">") : F(" "));
    switch (idx) {
      case 0: lcd.print(F("DiSEqC: 1.0"));        break;
      case 1: lcd.print(F("Port: ")); lcd.print(sDis+1); break;
      case 2: lcd.print(F("Tone: Kapali"));        break;
      case 3: lcd.print(F("Motor: USALS"));        break;
      case 4: lcd.print(F("Pozisyoner: Acik"));    break;
      case 5: lcd.print(F("Tekrar: 1"));           break;
      case 6: lcd.print(F("Test Komutu"));         break;
      case 7: lcd.print(F("Motor Limit"));         break;
      case 8: lcd.print(F("Ref Uydu"));            break;
      case 9: lcd.print(F("[KAYDET]"));            break;
    }
    lcd.print(F("     "));
  }
}

// Kanal Kaydet
void sub5() {
  for (byte i = 0; i < 3; i++) {
    byte idx = sOfs + i; if (idx > 9) break;
    switch (idx) {
      case 0: prI(i+1, idx, F("Ad: TURKSAT")); break;
      case 1: prI(i+1, idx, F("Favori Ekle")); break;
      case 2: prI(i+1, idx, F("Kilit Ekle"));  break;
      case 3: prI(i+1, idx, F("Siralama"));    break;
      case 4: prI(i+1, idx, F("Grup Ata"));    break;
      case 5: prI(i+1, idx, F("EPG Ayari"));   break;
      case 6: prI(i+1, idx, F("Altyazi"));     break;
      case 7: prI(i+1, idx, F("Ses Dili"));    break;
      case 8: prI(i+1, idx, F("Kanal Sil"));   break;
      case 9: prI(i+1, idx, F("[KAYDET]"));    break;
    }
  }
}

// Sinyal Analiz
void sub6() {
  lcd.setCursor(0, 1);
  lcd.print(F(" S:")); lcd.print(sLev); lcd.print(F("% "));
  byte b = map(sLev, 0, 100, 0, 8);
  for (byte i = 0; i < 8; i++) lcd.print(i < b ? (char)255 : ' ');

  lcd.setCursor(0, 2);
  lcd.print(F(" Q:")); lcd.print(sQua);
  lcd.print(F("% V:")); lcd.print(sVolA, 1);

  lcd.setCursor(0, 3);
  lcd.print(sIdx == 0 ? F(">") : F(" "));
  lcd.print(F("Log:")); lcd.print(sLog ? F("ON ") : F("OFF"));
  lcd.print(F(" BER:0.00"));
}

// Motor
void sub7() {
  for (byte i = 0; i < 3; i++) {
    byte idx = sOfs + i; if (idx > 9) break;
    lcd.setCursor(0, i+1);
    lcd.print(idx == sIdx ? F(">") : F(" "));
    switch (idx) {
      case 0: lcd.print(F("Bati <<"));          break;
      case 1: lcd.print(F("Dogu >>"));          break;
      case 2: lcd.print(F("Poz:")); lcd.print(sMot); break;
      case 3: lcd.print(F("Poz Kaydet"));       break;
      case 4: lcd.print(F("Poz'a Git"));        break;
      case 5: lcd.print(F("Bati Limit"));       break;
      case 6: lcd.print(F("Dogu Limit"));       break;
      case 7: lcd.print(F("Limit Sifirla"));    break;
      case 8: lcd.print(F("Hiz: Orta"));        break;
      case 9: lcd.print(F("USALS Konum"));      break;
    }
    lcd.print(F("     "));
  }
}

// LNB
void sub8() {
  for (byte i = 0; i < 3; i++) {
    byte idx = sOfs + i; if (idx > 9) break;
    lcd.setCursor(0, i+1);
    lcd.print(idx == sIdx ? F(">") : F(" "));
    switch (idx) {
      case 0: lcd.print(F("Tip: Universal")); break;
      case 1: lcd.print(F("LOF L: 9750"));   break;
      case 2: lcd.print(F("LOF H: 10600"));  break;
      case 3: lcd.print(F("22kHz: Auto"));   break;
      case 4: lcd.print(F("Volt: ")); lcd.print(sLNB); lcd.print(F("V")); break;
      case 5: lcd.print(F("Guc: Acik"));     break;
      case 6: lcd.print(F("Ton: Normal"));   break;
      case 7: lcd.print(F("Unicable: Off")); break;
      case 8: lcd.print(F("MDU: Off"));      break;
      case 9: lcd.print(F("[KAYDET]"));      break;
    }
    lcd.print(F("     "));
  }
}

// Zaman
void sub9() {
  unsigned long sn = millis() / 1000;
  lcd.setCursor(0, 1);
  lcd.print(F(" Up: "));
  lcd.print(sn/3600);       lcd.print(F("s "));
  lcd.print((sn%3600)/60);  lcd.print(F("d "));
  lcd.print(sn%60);         lcd.print(F("s"));
  lcd.setCursor(0, 2);
  lcd.print(F(" Saat: --:--:--"));
  lcd.setCursor(0, 3);
  lcd.print(F(" Tarih: --/--/--"));
}

// Ses
void sub10() {
  for (byte i = 0; i < 3; i++) {
    byte idx = sOfs + i; if (idx > 9) break;
    lcd.setCursor(0, i+1);
    lcd.print(idx == sIdx ? F(">") : F(" "));
    switch (idx) {
      case 0: lcd.print(F("Buz: ")); lcd.print(sMute?F("OFF"):F("ON")); break;
      case 1: lcd.print(F("Sev: ")); lcd.print(sVol); lcd.print(F("%")); break;
      case 2: lcd.print(F("Tus: Acik"));    break;
      case 3: lcd.print(F("Alarm: Acik"));  break;
      case 4: lcd.print(F("Uyari: Acik"));  break;
      case 5: lcd.print(F("Basari: Acik")); break;
      case 6: lcd.print(F("Hata: Acik"));   break;
      case 7: lcd.print(F("Ses Test"));     break;
      case 8: lcd.print(F("Tum Kapat"));    break;
      case 9: lcd.print(F("[KAYDET]"));     break;
    }
    lcd.print(F("     "));
  }
}

// Ekran
void sub11() {
  for (byte i = 0; i < 3; i++) {
    byte idx = sOfs + i; if (idx > 9) break;
    switch (idx) {
      case 0: prI(i+1, idx, F("Parlak: 80%"));   break;
      case 1: prI(i+1, idx, F("Kontrast: 50%")); break;
      case 2: prI(i+1, idx, F("Isik: Acik"));    break;
      case 3: prI(i+1, idx, F("Timeout: 30s"));  break;
      case 4: prI(i+1, idx, F("Renk: Normal"));  break;
      case 5: prI(i+1, idx, F("Font: Orta"));    break;
      case 6: prI(i+1, idx, F("Koruyucu"));      break;
      case 7: prI(i+1, idx, F("Ters: Kapali"));  break;
      case 8: prI(i+1, idx, F("Test Desen"));    break;
      case 9: prI(i+1, idx, F("[KAYDET]"));      break;
    }
  }
}

// Log
void sub12() {
  lcd.setCursor(0, 1);
  lcd.print(F(" Serial: "));
  lcd.print(sLog ? F("AKTIF") : F("PASIF"));
  lcd.setCursor(0, 2);
  lcd.print(F(" Baud: 115200"));
  lcd.setCursor(0, 3);
  lcd.print(sIdx == 0 ? F(">") : F(" "));
  lcd.print(F("[OK = Degistir]"));
}

// Anten
void sub13() {
  for (byte i = 0; i < 3; i++) {
    byte idx = sOfs + i; if (idx > 9) break;
    switch (idx) {
      case 0: prI(i+1, idx, F("Tip: Offset"));   break;
      case 1: prI(i+1, idx, F("Cap: 80cm"));     break;
      case 2: prI(i+1, idx, F("Azimuth: 180"));  break;
      case 3: prI(i+1, idx, F("Elev: 35"));      break;
      case 4: prI(i+1, idx, F("Skew: 0"));       break;
      case 5: prI(i+1, idx, F("Feed: Tek"));     break;
      case 6: prI(i+1, idx, F("Polar: Lineer")); break;
      case 7: prI(i+1, idx, F("Oto Arama"));     break;
      case 8: prI(i+1, idx, F("Kalibrasyon"));   break;
      case 9: prI(i+1, idx, F("[KAYDET]"));      break;
    }
  }
}

/*
 * loadBarChars:
 * 8 ozel karakter yukle (0=en altta 1 satir dolu, 7=tamamen dolu)
 * Dikey bar grafik icin kullanilir
 */
void loadBarChars() {
  byte b0[8] = {0,0,0,0,0,0,0,31};
  byte b1[8] = {0,0,0,0,0,0,31,31};
  byte b2[8] = {0,0,0,0,0,31,31,31};
  byte b3[8] = {0,0,0,0,31,31,31,31};
  byte b4[8] = {0,0,0,31,31,31,31,31};
  byte b5[8] = {0,0,31,31,31,31,31,31};
  byte b6[8] = {0,31,31,31,31,31,31,31};
  byte b7[8] = {31,31,31,31,31,31,31,31};
  lcd.createChar(0, b0);
  lcd.createChar(1, b1);
  lcd.createChar(2, b2);
  lcd.createChar(3, b3);
  lcd.createChar(4, b4);
  lcd.createChar(5, b5);
  lcd.createChar(6, b6);
  lcd.createChar(7, b7);
}

/*
 * sub14 - Spektrum Gorunumu
 *
 * DUZELTME:
 *   - Eski kod: peak = sLev/14, uzerine analogRead()%5 ekliyordu
 *     => sLev yuksekken her zaman yuksek cikiyordu
 *   - Yeni kod: millis() tabanli pseudo-random noise,
 *     sLev sadece taban belirler (0-5 arasi),
 *     noise kolon bazinda farklilasiyor (-2..+2)
 *     => Her kolon farkli yukseklikte, sinyal dusukse barlar alca
 *   - 200ms'de bir guncelleme (statik array ile)
 *   - Merkez kolonlar +1 boost (gercekci spektrum seklinde)
 */
void sub14() {
  // Satir 1: frekans bilgisi
  lcd.setCursor(0, 1);
  lcd.print(F(" F:"));
  lcd.print(sFreq);
  lcd.print(F(" MHz  "));

  // 200ms'de bir spektrum degerlerini guncelle
  unsigned long now = millis();
  if (now - spekLastT >= 200) {
    spekLastT = now;

    // sLev (0-100) -> taban (0-5)
    // sLev=0   => taban=0 (barlar altta)
    // sLev=100 => taban=5 (barlar orta-yukari)
    byte base = map(sLev, 0, 100, 0, 5);

    // Her kolon icin farkli pseudo-random deger uret
    // Formul: (millis/200 + kolon*37 + kolon^2) % 5
    // => Her kolon her guncellemede farkli, ama istikrarli gorunum
    for (byte i = 0; i < 18; i++) {
      byte frame  = (byte)((now / 200) & 0xFF);
      byte noise  = (frame + i * 37 + i * i) % 5; // 0..4

      int v = (int)base + (int)noise - 2; // -2 ile +2 arasi ofset

      // Merkez kolonlar biraz daha yuksek (gercekci spektrum tepesi)
      if (i >= 7 && i <= 10) v += 1;

      // Sinir kontrol
      if (v < 0) v = 0;
      if (v > 7) v = 7;

      spekVals[i] = (byte)v;
    }
  }

  // Satir 2: dikey bar grafik
  // | + 18 kolon + |  = 20 karakter
  lcd.setCursor(0, 2);
  lcd.print(F("|"));
  for (byte i = 0; i < 18; i++) {
    if (spekVals[i] > 0) {
      lcd.write((byte)(spekVals[i] - 1)); // ozel karakter 0-6
    } else {
      lcd.print(F(" "));                  // bos = sinyal yok
    }
  }
  lcd.print(F("|"));

  // Satir 3: sinyal ve kalite bilgisi
  lcd.setCursor(0, 3);
  lcd.print(F(" S:"));
  lcd.print(sLev);
  lcd.print(F("% Q:"));
  lcd.print(sQua);
  lcd.print(F("%     "));
}

// Firmware
void sub15() {
  for (byte i = 0; i < 3; i++) {
    byte idx = sOfs + i; if (idx > 9) break;
    switch (idx) {
      case 0: prI(i+1, idx, F("Mevcut: v3.4"));    break;
      case 1: prI(i+1, idx, F("Guncelle Kontrol")); break;
      case 2: prI(i+1, idx, F("USB Yukle"));        break;
      case 3: prI(i+1, idx, F("SD Yukle"));         break;
      case 4: prI(i+1, idx, F("Serial Yukle"));     break;
      case 5: prI(i+1, idx, F("Yedek Al"));         break;
      case 6: prI(i+1, idx, F("Yedek Yukle"));      break;
      case 7: prI(i+1, idx, F("Bootloader"));       break;
      case 8: prI(i+1, idx, F("Checksum"));         break;
      case 9: prI(i+1, idx, F("Fabrika FW"));       break;
    }
  }
}

// Dil
void sub16() {
  for (byte i = 0; i < 3; i++) {
    byte idx = sOfs + i; if (idx > 9) break;
    switch (idx) {
      case 0: prI(i+1, idx, F("> Turkce"));    break;
      case 1: prI(i+1, idx, F("  English"));   break;
      case 2: prI(i+1, idx, F("  Deutsch"));   break;
      case 3: prI(i+1, idx, F("  Francais"));  break;
      case 4: prI(i+1, idx, F("  Espanol"));   break;
      case 5: prI(i+1, idx, F("  Italiano"));  break;
      case 6: prI(i+1, idx, F("  Portugues")); break;
      case 7: prI(i+1, idx, F("  Russian"));   break;
      case 8: prI(i+1, idx, F("  Arabic"));    break;
      case 9: prI(i+1, idx, F("  Chinese"));   break;
    }
  }
}

// Ag
void sub17() {
  for (byte i = 0; i < 3; i++) {
    byte idx = sOfs + i; if (idx > 9) break;
    switch (idx) {
      case 0: prI(i+1, idx, F("IP:192.168.1.100")); break;
      case 1: prI(i+1, idx, F("Mask:255.255.255")); break;
      case 2: prI(i+1, idx, F("GW:192.168.1.1"));   break;
      case 3: prI(i+1, idx, F("DNS:8.8.8.8"));      break;
      case 4: prI(i+1, idx, F("DHCP: Acik"));       break;
      case 5: prI(i+1, idx, F("MAC Adresi"));        break;
      case 6: prI(i+1, idx, F("WiFi: Off"));         break;
      case 7: prI(i+1, idx, F("BT: Off"));           break;
      case 8: prI(i+1, idx, F("Web: Off"));          break;
      case 9: prI(i+1, idx, F("[KAYDET]"));          break;
    }
  }
}

// Sistem
void sub18() {
  lcd.setCursor(0, 1);
  lcd.print(F(" FW:3.4 R:")); lcd.print(freeRam());
  lcd.setCursor(0, 2);
  lcd.print(F(" Reset:"));
  if      (resetR & 4) lcd.print(F("BROWNOUT"));
  else if (resetR & 8) lcd.print(F("WDT"));
  else                 lcd.print(F("PowerOn"));
  unsigned long sn = millis()/1000;
  lcd.setCursor(0, 3);
  lcd.print(F(" Up:"));
  lcd.print(sn/60); lcd.print(F("m"));
  lcd.print(sn%60); lcd.print(F("s"));
}

// Fabrika
void sub19() {
  for (byte i = 0; i < 3; i++) {
    byte idx = sOfs + i; if (idx > 9) break;
    switch (idx) {
      case 0: prI(i+1, idx, F("!!! DIKKAT !!!")); break;
      case 1: prI(i+1, idx, F("Ayarlar silinir")); break;
      case 2: prI(i+1, idx, F("Kanallar silinir")); break;
      case 3: prI(i+1, idx, F("TP sifirlanir")); break;
      case 4: prI(i+1, idx, F("Motor sifirlanir")); break;
      case 5: prI(i+1, idx, F("EEPROM temizlenir")); break;
      case 6: prI(i+1, idx, F("Varsayilan yukle")); break;
      case 7: prI(i+1, idx, F("Yedek alinmadi!")); break;
      case 8: prI(i+1, idx, F("Emin misiniz?")); break;
      case 9: prI(i+1, idx, F("[ONAYLA]")); break;
    }
  }
}

void fabR() {
  lcd.setCursor(0, 3); lcd.print(F(" Siliniyor...       "));
  bipEr();
  for (byte i = 0; i < 16; i++) { EEPROM.update(i, 0xFF); delay(3); }
  sFreq = 11096; sPol = 0; sSR = 30000;
  sDis = 0; sMot = 0; sLNB = 13;
  sVol = 50; sLog = false; sMute = false;
  svEE();
  inSub = false; sIdx = 0; sOfs = 0; edt = false;
  hdr(); showM();
}

void adj(int d) {
  if (cur == 1) {
    if (sIdx == 0) { sFreq += d; sFreq = constrain(sFreq, 10700, 12750); }
    if (sIdx == 1) sPol = !sPol;
    if (sIdx == 2) { sSR += d * 100; sSR = constrain(sSR, 1000, 45000); }
  }
  if (cur == 4 && sIdx == 1) sDis = (sDis + d + 4) % 4;
  if (cur == 7) {
    if (sIdx == 0) sMot--;
    if (sIdx == 1) sMot++;
    sMot = constrain(sMot, -180, 180);
  }
  if (cur == 8  && sIdx == 4) sLNB = (sLNB == 13) ? 18 : 13;
  if (cur == 10 && sIdx == 1) { sVol += d * 10; sVol = constrain(sVol, 0, 100); }
  sub();
}

void hdr() {
  lcd.setCursor(0, 0);
  lcd.print(F("--- OBM32 MENU ---  "));
}

void clr() {
  lcd.setCursor(0, 1); lcd.print(F("                    "));
  lcd.setCursor(0, 2); lcd.print(F("                    "));
  lcd.setCursor(0, 3); lcd.print(F("                    "));
}

void showM() {
  clr();
  for (byte i = 0; i < 3 && (mOfs + i) < MNUM; i++) {
    lcd.setCursor(0, i + 1);
    lcd.print((mOfs + i) == cur ? F(">") : F(" "));
    getMn(mOfs + i);
    lcd.print(buf);
    lcd.print(F("     "));
  }
}

void bip(byte ms) {
  if (sMute) return;
  digitalWrite(BZ, HIGH); delay(ms); digitalWrite(BZ, LOW);
}
void bipOk() { bip(25); delay(15); bip(40); }
void bipEr() { bip(60); delay(30); bip(60); delay(30); bip(60); }

void serTask() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (cmdLen > 0) { cmdBuf[cmdLen] = 0; runCmd(); cmdLen = 0; }
    } else if (cmdLen < 11) {
      cmdBuf[cmdLen++] = c;
    }
  }
}

void runCmd() {
  if      (strcmp(cmdBuf, "LOG_START") == 0) {
    sLog = true; svEE();
    Serial.println(F("ZAMAN;SINYAL;KALITE;VOLTAJ"));
  }
  else if (strcmp(cmdBuf, "LOG_STOP")  == 0) { sLog = false; svEE(); }
  else if (strcmp(cmdBuf, "STATUS")    == 0) {
    getMn(cur);
    Serial.print(F("M:")); Serial.println(buf);
    Serial.print(F("R:")); Serial.println(freeRam());
  }
  else if (strcmp(cmdBuf, "HELP")      == 0) {
    Serial.println(F("LOG_START LOG_STOP STATUS"));
  }
}

void svEE() {
  EEPROM.update(EE_OK,    MAGIC);
  EEPROM.update(EE_FREQ,  sFreq & 0xFF);
  EEPROM.update(EE_FREQ+1,(sFreq>>8)&0xFF);
  EEPROM.update(EE_FREQ+2,(sFreq>>16)&0xFF);
  EEPROM.update(EE_FREQ+3,(sFreq>>24)&0xFF);
  EEPROM.update(EE_POL,   sPol);
  EEPROM.update(EE_SR,    sSR & 0xFF);
  EEPROM.update(EE_SR+1,  (sSR>>8)&0xFF);
  EEPROM.update(EE_DIS,   sDis);
  EEPROM.update(EE_MOT,   sMot & 0xFF);
  EEPROM.update(EE_MOT+1, (sMot>>8)&0xFF);
  EEPROM.update(EE_LNB,   sLNB);
  EEPROM.update(EE_VOL,   sVol);
  EEPROM.update(EE_LOG,   sLog  ? 1 : 0);
  EEPROM.update(EE_MUTE,  sMute ? 1 : 0);
}

void ldEE() {
  if (EEPROM.read(EE_OK) != MAGIC) { svEE(); return; }
  sFreq = (unsigned long)EEPROM.read(EE_FREQ)        |
          ((unsigned long)EEPROM.read(EE_FREQ+1)<<8)  |
          ((unsigned long)EEPROM.read(EE_FREQ+2)<<16) |
          ((unsigned long)EEPROM.read(EE_FREQ+3)<<24);
  if (sFreq < 10700 || sFreq > 12750) sFreq = 11096;
  sPol = EEPROM.read(EE_POL); if (sPol > 1) sPol = 0;
  sSR  = EEPROM.read(EE_SR) | (EEPROM.read(EE_SR+1)<<8);
  if (sSR < 1000 || sSR > 45000) sSR = 30000;
  sDis = EEPROM.read(EE_DIS); if (sDis > 3) sDis = 0;
  sMot = EEPROM.read(EE_MOT) | (EEPROM.read(EE_MOT+1)<<8);
  if (sMot < -180 || sMot > 180) sMot = 0;
  sLNB = EEPROM.read(EE_LNB); if (sLNB != 13 && sLNB != 18) sLNB = 13;
  sVol = EEPROM.read(EE_VOL); if (sVol > 100) sVol = 50;
  sLog  = EEPROM.read(EE_LOG)  == 1;
  sMute = EEPROM.read(EE_MUTE) == 1;
}
