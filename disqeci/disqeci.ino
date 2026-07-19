
/*
 * OBM32 v4.3 - OPTIMIZED
 * 30720 byte limiti icin optimize edildi
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

#define BK 3
#define UP 4
#define DN 5
#define OK 6
#define BZ 12
#define BL 9

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
#define EE_BRT  15
#define EE_LANG 16
#define EE_TKGS 17
#define EE_TAUT 18
#define EE_LCN  19
#define EE_BOLG 20
#define EE_FEC  21
#define EE_DMOD 22
#define EE_22K  23
#define EE_TONE 24
#define EE_UCAB 25
#define EE_MSPD 26
#define EE_ATYP 27
#define EE_ACAP 28
#define EE_AZIM 30
#define EE_ELEV 32
#define EE_SKEW 34
#define EE_FEED 36
#define EE_APOL 37
#define EE_DHCP 38
#define EE_SCRN 39
#define EE_TOUT 40
#define EE_FONT 41
#define EE_TUSB 44
#define EE_SLPT 49
#define EE_BFRQ 50
#define MAGIC   0xAB

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
void getMn(byte i){strcpy_P(buf,(char*)pgm_read_word(&(mnTbl[i])));}

const byte gaussProf[18] PROGMEM={1,2,3,4,5,6,7,8,8,8,8,7,6,5,4,3,2,1};

// Line buffer
char lb[21];
byte lp;
void lC(){memset(lb,' ',20);lb[20]=0;lp=0;}
void lA(char c){if(lp<20)lb[lp++]=c;}
void lS(const char*s){while(*s&&lp<20)lb[lp++]=*s++;}
void lF(const __FlashStringHelper*f){PGM_P p=reinterpret_cast<PGM_P>(f);char c;while((c=pgm_read_byte(p++))&&lp<20)lb[lp++]=c;}
void lN(long n){char nb[12];ltoa(n,nb,10);lS(nb);}
void lW(byte r){lb[20]=0;lcd.setCursor(0,r);lcd.print(lb);}
void wF(byte r,const __FlashStringHelper*t){strncpy_P(lb,(const char*)t,20);lb[20]=0;byte l=strlen(lb);for(byte i=l;i<20;i++)lb[i]=' ';lcd.setCursor(0,r);lcd.print(lb);}

// Vars
byte cur=0,mOfs=0,sIdx=0,sOfs=0;
bool inSub=false,edt=false;
byte resetR=0;

unsigned long lastAct=0;
bool sleeping=false;
byte sleepTime=10;
unsigned long sleepClkT=0;

bool tkgsOn=1,tkgsAuto=1,tkgsLCN=1;
byte tkgsBolge=0;

unsigned long sFreq=11096;
byte sPol=0;
unsigned int sSR=30000;
byte sFEC=0,sDmod=0;
unsigned int sPIDV=101,sPIDA=102;

byte sDis=0,sDisVer=0;
bool sTone=false;

int sMot=0;
byte sMotSpd=1;
bool sMotLimB=0,sMotLimD=0;

byte sLNB=13,sLNBtyp=0;
bool s22kHz=1,sLNBpwr=1,sToneB=0,sUnicab=0,sMDU=0;

byte sVol=50;
bool sMute=0,sTusBip=1;
unsigned int sBuzFreq=2000;

byte sBrt=80,sKont=50;
bool sIsik=1;
byte sTout=30,sFont=1;

byte sATyp=0,sACap=80;
int sAzim=180,sElev=35,sSkew=0;
byte sFeed=0,sAPol=0;

bool sDHCP=1;
byte sLang=0;
bool sLog=0;

int sLev=0,sQua=0,sLevF=0,sQuaF=0;
float sVolA=0.0;
int sLevMin=100,sLevMax=0;
unsigned long sSigCnt=0;

bool chFav[10],chLock[10];

byte bP[4]={UP,DN,OK,BK};
bool bO[4]={1,1,1,1};
unsigned long bT[4]={0,0,0,0};
unsigned long bSt[4]={0,0,0,0};
bool bH[4]={0,0,0,0};
#define DEB 180
#define HOLD 400
#define REPT 80

unsigned long tS=0,tL=0,tR=0,spekLastT=0;
byte spekVals[18],spekPeak[18];
byte lfoPhase=0;

char cmdBuf[32];
byte cmdLen=0;
unsigned long msgT=0;
bool msgAct=false;

unsigned int rndSeed=12345;
byte pRnd(byte mx){rndSeed^=(rndSeed<<5);rndSeed^=(rndSeed>>3);rndSeed^=(rndSeed<<7);return(byte)(rndSeed%(mx+1));}

int freeRam(){extern int __heap_start,*__brkval;int v;return(int)&v-(__brkval==0?(int)&__heap_start:(int)__brkval);}

// Brightness
void setBrt(byte p){analogWrite(BL,map(p,0,100,0,255));}

// Sound
void bip(byte ms){
  if(sMute||!sTusBip)return;
  byte a=(unsigned int)ms*sVol/100;if(a<5)a=5;
  tone(BZ,sBuzFreq,a);delay(a);noTone(BZ);
}
void bipOk(){if(sMute)return;tone(BZ,2500,25);delay(30);tone(BZ,3000,40);delay(45);noTone(BZ);}
void bipEr(){if(sMute)return;tone(BZ,800,60);delay(80);tone(BZ,600,60);delay(80);tone(BZ,400,80);delay(90);noTone(BZ);}

// Animation
void animProg(const __FlashStringHelper*txt,unsigned int ms){
  lC();lA(' ');lF(txt);lW(2);
  wF(3,F("[                  ]"));
  for(byte i=0;i<18;i++){lcd.setCursor(i+1,3);lcd.write((byte)7);delay(ms/18);}
}
void animOK(){wF(3,F("   >>> TAMAM! <<<   "));bipOk();delay(300);}
void animERR(){wF(3,F("    !!! HATA !!!    "));bipEr();}

void showMsg(const __FlashStringHelper*msg){
  lC();lA(' ');lF(msg);lW(3);msgT=millis();msgAct=true;
}

// Sleep
void enterSleep(){
  if(sleeping)return;sleeping=true;
  byte c=map(sBrt,0,100,0,255);
  for(int i=c;i>=0;i-=5){analogWrite(BL,i);delay(3);}
  analogWrite(BL,0);
  for(byte r=0;r<4;r++)wF(r,F("                    "));
  lcd.noBacklight();sleepClkT=0;
}

void showSleepClock(){
  unsigned long sn=millis()/1000;
  unsigned long now=millis();
  if(now-sleepClkT<1000)return;
  sleepClkT=now;
  lcd.backlight();analogWrite(BL,15);
  wF(0,F("     OBM32 v4.3     "));
  lC();lF(F("      "));
  if(sn/3600<10)lA('0');lN(sn/3600);lA(':');
  if((sn%3600)/60<10)lA('0');lN((sn%3600)/60);lA(':');
  if(sn%60<10)lA('0');lN(sn%60);
  lW(1);
  lC();lF(F("   S:"));lN(sLev);lF(F("% Q:"));lN(sQua);lA('%');lW(2);
  wF(3,F("  Herhangi tusa bas "));
}

void wakeUp(){
  if(!sleeping)return;sleeping=false;lastAct=millis();
  lcd.backlight();
  byte t=map(sBrt,0,100,0,255);
  for(int i=0;i<=t;i+=5){analogWrite(BL,i);delay(2);}
  analogWrite(BL,t);
  if(inSub)sub();else{hdr();showM();}
  bip(30);
}

void resetAct(){lastAct=millis();if(sleeping)wakeUp();}

// Setup
void setup(){
  resetR=MCUSR;MCUSR=0;
  Serial.begin(115200);
  Serial.println(F("OBM32 v4.3"));

  for(byte i=0;i<4;i++)pinMode(bP[i],INPUT_PULLUP);
  pinMode(BZ,OUTPUT);pinMode(BL,OUTPUT);

  Wire.begin();lcd.init();delay(100);
  lcd.backlight();delay(50);
  loadBarChars();ldEE();setBrt(sBrt);

  for(byte i=0;i<18;i++){spekVals[i]=0;spekPeak[i]=0;}
  for(byte i=0;i<10;i++){chFav[i]=0;chLock[i]=0;}

  wF(0,F("===================="));
  wF(1,F("   OBM32 DiSEqCi   "));
  wF(2,F("    v4.3 - FULL     "));
  wF(3,F("[                  ]"));

  for(byte i=0;i<18;i++){
    lcd.setCursor(i+1,3);lcd.write((byte)7);
    if(i%4==0){tone(BZ,500+i*100,15);}
    delay(35);
  }
  noTone(BZ);delay(200);

  lC();lF(F(" RAM:"));lN(freeRam());lF(F(" Uyku:"));lN(sleepTime);lA('s');lW(3);
  bipOk();delay(500);
  lastAct=millis();hdr();showM();
}

// Loop
void loop(){
  chkBtn();
  unsigned long n=millis();

  if(n-tS>=300){
    tS=n;
    int rL=map(analogRead(A0),0,1023,0,100);
    int rQ=map(analogRead(A1),0,1023,0,100);
    sVolA=analogRead(A2)*15.0/1023.0;
    sLevF=((sLevF*3)+rL)/4;sQuaF=((sQuaF*3)+rQ)/4;
    sLev=sLevF;sQua=sQuaF;
    if(sLev<sLevMin)sLevMin=sLev;
    if(sLev>sLevMax)sLevMax=sLev;
    sSigCnt++;
  }

  if(!sleeping&&inSub&&!msgAct&&n-tR>=500){
    tR=n;
    if(cur==6||cur==14||cur==9||cur==18)sub();
  }

  if(msgAct&&n-msgT>=2000){msgAct=false;if(inSub&&!sleeping)sub();}

  if(!sleeping&&(n-lastAct>=(unsigned long)sleepTime*1000))enterSleep();
  if(sleeping)showSleepClock();

  if(sLog&&n-tL>=1000){
    tL=n;Serial.print(n/1000);Serial.print(';');
    Serial.print(sLev);Serial.print(';');
    Serial.print(sQua);Serial.print(';');
    Serial.println(sVolA,2);
  }

  serTask();
  static unsigned long lfoT=0;
  if(n-lfoT>=400){lfoT=n;lfoPhase++;}
}

// Button
void chkBtn(){
  unsigned long n=millis();
  for(byte i=0;i<4;i++){
    bool s=digitalRead(bP[i]);
    if(s==LOW){
      if(bO[i]==HIGH)bSt[i]=n;
      if(sleeping){wakeUp();bO[i]=s;return;}
      if(!bH[i]&&bO[i]==HIGH&&n-bT[i]>=DEB){bT[i]=n;bip(20);btnAct(i);resetAct();}
      if((i<2)&&n-bSt[i]>=HOLD){bH[i]=true;if(n-bT[i]>=REPT){bT[i]=n;btnAct(i);resetAct();}}
    }else{bH[i]=false;}
    bO[i]=s;
  }
}

void btnAct(byte i){
  if(i==0)doUp();else if(i==1)doDn();
  else if(i==2){bip(40);doOk();}else{bip(30);doBk();}
}

void doUp(){
  if(edt){adj(1);return;}
  if(inSub){if(sIdx>0){sIdx--;if(sIdx<sOfs)sOfs=sIdx;}sub();return;}
  if(cur>0){cur--;if(cur<mOfs)mOfs=cur;}showM();
}

void doDn(){
  if(edt){adj(-1);return;}
  if(inSub){if(sIdx<9){sIdx++;if(sIdx>sOfs+2)sOfs=sIdx-2;}sub();return;}
  if(cur<MNUM-1){cur++;if(cur>mOfs+2)mOfs=cur-2;}showM();
}

void doOk(){
  if(edt){edt=false;svEE();animOK();delay(200);sub();return;}
  if(inSub){doAction();return;}
  inSub=true;sIdx=0;sOfs=0;sub();
}

void doBk(){
  if(edt){edt=false;sub();return;}
  if(inSub){inSub=false;sIdx=0;sOfs=0;edt=false;hdr();showM();}
}

// Actions
void doAction(){
  switch(cur){
    case 0:act0();break;case 1:act1();break;case 2:act2();break;
    case 3:act3();break;case 4:act4();break;case 5:act5();break;
    case 6:act6();break;case 7:act7();break;case 8:act8();break;
    case 9:showMsg(F("Salt okunur"));break;
    case 10:act10();break;case 11:act11();break;
    case 12:act12();break;case 13:act13();break;
    case 14:showMsg(F("Tarama devam"));break;
    case 15:act15();break;case 16:act16();break;
    case 17:act17();break;case 18:showMsg(F("Salt okunur"));break;
    case 19:act19();break;
  }
}

void act0(){
  switch(sIdx){
    case 0:tkgsOn=!tkgsOn;svEE();showMsg(tkgsOn?F("TKGS:AKTIF"):F("TKGS:PASIF"));bipOk();break;
    case 1:tkgsAuto=!tkgsAuto;svEE();showMsg(tkgsAuto?F("OtoGnc:ACIK"):F("OtoGnc:KAPALI"));bipOk();break;
    case 2:clr();animProg(F("TKGS Guncelle"),1500);animOK();break;
    case 3:showMsg(F("TKGS v2.1"));break;
    case 4:clr();animProg(F("Kanal Senkron"),1000);animOK();break;
    case 5:clr();animProg(F("TP Senkron"),1000);animOK();break;
    case 6:tkgsLCN=!tkgsLCN;svEE();showMsg(tkgsLCN?F("LCN:ACIK"):F("LCN:KAPALI"));bipOk();break;
    case 7:tkgsBolge=(tkgsBolge+1)%7;svEE();bipOk();break;
    case 8:showMsg(F("TKGS bilgi yuklu"));break;
    case 9:tkgsOn=1;tkgsAuto=1;tkgsLCN=1;tkgsBolge=0;svEE();showMsg(F("Sifirlandi!"));bipEr();break;
  }
  sub();
}

void act1(){
  switch(sIdx){
    case 0:case 1:case 2:case 5:case 6:edt=true;sub();return;
    case 3:sFEC=(sFEC+1)%6;bipOk();break;
    case 4:sDmod=!sDmod;bipOk();break;
    case 7:showMsg(F("TURKSAT 42E"));break;
    case 8:clr();animProg(F("TP Ekleniyor"),800);animOK();break;
    case 9:clr();animProg(F("Kaydediliyor"),1000);svEE();animOK();break;
  }
  sub();
}

void act2(){showMsg(F("Kanal secildi!"));bipOk();}

void act3(){
  const unsigned int tpF[]={11096,11180,12015,12053,12130,12188,12245,12303,12380,12458};
  const byte tpP[]={0,1,0,1,0,1,0,1,0,1};
  const unsigned int tpS[]={30000,30000,27500,27500,27500,27500,27500,27500,30000,30000};
  if(sIdx<10){sFreq=tpF[sIdx];sPol=tpP[sIdx];sSR=tpS[sIdx];svEE();
    clr();animProg(F("TP Yukleniyor"),600);animOK();}
  sub();
}

void act4(){
  switch(sIdx){
    case 0:sDisVer=(sDisVer+1)%3;bipOk();break;
    case 1:sDis=(sDis+1)%4;bipOk();break;
    case 2:sTone=!sTone;bipOk();break;
    case 6:showMsg(F("Test OK!"));bipOk();break;
    case 9:svEE();showMsg(F("Kaydedildi!"));bipOk();break;
    default:showMsg(F("OK"));break;
  }
  svEE();sub();
}

void act5(){
  switch(sIdx){
    case 1:chFav[0]=!chFav[0];showMsg(chFav[0]?F("Favori+"):F("Favori-"));bipOk();break;
    case 2:chLock[0]=!chLock[0];showMsg(chLock[0]?F("Kilit+"):F("Kilit-"));bipOk();break;
    case 8:showMsg(F("Silindi!"));bipEr();break;
    case 9:svEE();showMsg(F("Kaydedildi!"));bipOk();break;
    default:showMsg(F("OK"));bipOk();break;
  }
  sub();
}

void act6(){
  if(sIdx==0){sLog=!sLog;svEE();showMsg(sLog?F("Log:BASLADI"):F("Log:DURDU"));
    if(sLog)Serial.println(F("T;S;Q;V"));bipOk();}
  else if(sIdx==1){sLevMin=100;sLevMax=0;showMsg(F("MinMax sifir"));bipOk();}
}

void act7(){
  switch(sIdx){
    case 0:sMot--;sMot=constrain(sMot,-180,180);showMsg(F("BATI <<"));bip(30);break;
    case 1:sMot++;sMot=constrain(sMot,-180,180);showMsg(F("DOGU >>"));bip(30);break;
    case 2:edt=true;sub();return;
    case 3:svEE();showMsg(F("Poz kaydedildi"));bipOk();break;
    case 5:sMotLimB=1;showMsg(F("Bati lim SET"));bipOk();break;
    case 6:sMotLimD=1;showMsg(F("Dogu lim SET"));bipOk();break;
    case 7:sMotLimB=0;sMotLimD=0;showMsg(F("Limitler sifir"));bipEr();break;
    case 8:sMotSpd=(sMotSpd+1)%3;bipOk();break;
    default:showMsg(F("OK"));break;
  }
  svEE();sub();
}

void act8(){
  switch(sIdx){
    case 0:sLNBtyp=(sLNBtyp+1)%3;bipOk();break;
    case 3:s22kHz=!s22kHz;bipOk();break;
    case 4:sLNB=(sLNB==13)?18:13;bipOk();break;
    case 5:sLNBpwr=!sLNBpwr;bipOk();break;
    case 6:sToneB=!sToneB;bipOk();break;
    case 7:sUnicab=!sUnicab;bipOk();break;
    case 8:sMDU=!sMDU;bipOk();break;
    case 9:svEE();showMsg(F("Kaydedildi!"));bipOk();break;
    default:showMsg(F("OK"));break;
  }
  svEE();sub();
}

void act10(){
  switch(sIdx){
    case 0:sMute=!sMute;svEE();if(!sMute){tone(BZ,2000,30);delay(35);noTone(BZ);}break;
    case 1:case 9:edt=true;sub();return;
    case 2:sTusBip=!sTusBip;bip(20);break;
    case 7:{unsigned int n[]={262,330,392,523,392,330,262};
      for(byte i=0;i<7;i++){byte d=(unsigned int)60*sVol/100;if(d<10)d=10;
        tone(BZ,n[i],d);delay(d+15);}noTone(BZ);showMsg(F("Melodi OK!"));}break;
    case 8:sMute=1;sTusBip=0;showMsg(F("Sesler KAPALI"));break;
    default:bipOk();break;
  }
  svEE();sub();
}

void act11(){
  switch(sIdx){
    case 0:case 1:edt=true;sub();return;
    case 2:sIsik=!sIsik;if(sIsik){lcd.backlight();setBrt(sBrt);}else{lcd.noBacklight();analogWrite(BL,0);}bipOk();break;
    case 3:sTout=(sTout>=120)?10:sTout+10;bipOk();break;
    case 5:sFont=(sFont+1)%3;bipOk();break;
    case 8:for(byte p=0;p<2;p++){for(byte r=1;r<=3;r++){lcd.setCursor(0,r);for(byte i=0;i<20;i++)lcd.write((byte)((i+p*3)%8));}delay(400);}break;
    case 9:svEE();showMsg(F("Kaydedildi!"));bipOk();break;
    default:bipOk();break;
  }
  svEE();sub();
}

void act12(){
  sLog=!sLog;svEE();
  if(sLog){sLevMin=100;sLevMax=0;sSigCnt=0;Serial.println(F("T;S;Q;V"));
    clr();animProg(F("Log Basliyor"),600);animOK();}
  else{Serial.print(F("#"));Serial.println(sSigCnt);showMsg(F("Log DURDU!"));bipOk();}
  delay(150);sub();
}

void act13(){
  switch(sIdx){
    case 0:sATyp=(sATyp+1)%3;bipOk();break;
    case 1:case 2:case 3:case 4:edt=true;sub();return;
    case 5:sFeed=(sFeed+1)%3;bipOk();break;
    case 6:sAPol=!sAPol;bipOk();break;
    case 7:clr();animProg(F("Oto Arama"),2000);showMsg(F("3 TP bulundu!"));bipOk();break;
    case 8:clr();animProg(F("Kalibrasyon"),1200);animOK();break;
    case 9:svEE();showMsg(F("Kaydedildi!"));bipOk();break;
    default:bipOk();break;
  }
  svEE();sub();
}

void act15(){
  switch(sIdx){
    case 0:showMsg(F("FW:v4.3 Guncel"));break;
    case 1:showMsg(F("Guncelleme yok"));break;
    case 2:case 3:case 6:animERR();break;
    case 5:Serial.println(F("=YEDEK="));
      Serial.print(F("F:"));Serial.println(sFreq);
      Serial.print(F("P:"));Serial.println(sPol);
      Serial.print(F("S:"));Serial.println(sSR);
      Serial.println(F("=BITTI="));
      showMsg(F("Yedek alindi!"));bipOk();break;
    case 8:showMsg(F("CRC:OK"));bipOk();break;
    default:showMsg(F("OK"));break;
  }
  sub();
}

void act16(){sLang=sIdx;svEE();showMsg(F("Dil secildi!"));bipOk();sub();}

void act17(){
  switch(sIdx){
    case 4:sDHCP=!sDHCP;svEE();showMsg(sDHCP?F("DHCP:ACIK"):F("DHCP:KPLI"));bipOk();break;
    case 6:case 7:case 8:animERR();break;
    case 9:svEE();showMsg(F("Kaydedildi!"));bipOk();break;
    default:showMsg(F("OK"));break;
  }
  sub();
}

void act19(){
  if(sIdx==9){
    wF(3,F(" OK=Onayla BK=Iptal "));bipEr();
    unsigned long t=millis();
    while(millis()-t<5000){
      if(digitalRead(bP[2])==LOW){delay(200);fabR();return;}
      if(digitalRead(bP[3])==LOW){showMsg(F("Iptal!"));sub();return;}
    }
    showMsg(F("Zaman asimi!"));
  }else showMsg(F("[ONAYLA]icin 9.ya"));
}

// Sub menu display
void sub(){
  lC();lA('[');getMn(cur);lS(buf);lA(']');lW(0);
  clr();
  switch(cur){
    case 0:sub0();break;case 1:sub1();break;case 2:sub2();break;
    case 3:sub3();break;case 4:sub4();break;case 5:sub5();break;
    case 6:sub6();break;case 7:sub7();break;case 8:sub8();break;
    case 9:sub9();break;case 10:sub10();break;case 11:sub11();break;
    case 12:sub12();break;case 13:sub13();break;case 14:sub14();break;
    case 15:sub15();break;case 16:sub16();break;case 17:sub17();break;
    case 18:sub18();break;case 19:sub19();break;
  }
}

#define SR(i,idx) lC();lA(idx==sIdx?'>':' ')
#define SE(i) lW(i+1)

void sub0(){
  const char*bl[]={"Marmara","Ege","Akdeniz","IcAnadl","K.Deniz","D.Anadl","G.DAnad"};
  for(byte i=0;i<3;i++){byte idx=sOfs+i;if(idx>9)break;SR(i,idx);
    switch(idx){
      case 0:lF(F("TKGS:"));lF(tkgsOn?F("AKTIF"):F("PASIF"));break;
      case 1:lF(F("OtoGnc:"));lF(tkgsAuto?F("ACIK"):F("KPLI"));break;
      case 2:lF(F("Manuel Guncelle"));break;
      case 3:lF(F("Versiyon: 2.1"));break;
      case 4:lF(F("Kanal Senkron"));break;
      case 5:lF(F("TP Senkron"));break;
      case 6:lF(F("LCN:"));lF(tkgsLCN?F("ACIK"):F("KPLI"));break;
      case 7:lF(F("Bolge:"));lS(bl[tkgsBolge]);break;
      case 8:lF(F("TKGS Bilgi"));break;
      case 9:lF(F("[SIFIRLA]"));break;
    }SE(i);}
}

void sub1(){
  const char*fc[]={"Auto","1/2","2/3","3/4","5/6","7/8"};
  for(byte i=0;i<3;i++){byte idx=sOfs+i;if(idx>9)break;SR(i,idx);
    switch(idx){
      case 0:lF(F("Frk:"));lN(sFreq);break;
      case 1:lF(F("Pol:"));lA(sPol==0?'H':'V');break;
      case 2:lF(F("SR:"));lN(sSR);break;
      case 3:lF(F("FEC:"));lS(fc[sFEC]);break;
      case 4:lF(F("Mod:"));lF(sDmod==0?F("DVB-S"):F("DVB-S2"));break;
      case 5:lF(F("PIDV:"));lN(sPIDV);break;
      case 6:lF(F("PIDA:"));lN(sPIDA);break;
      case 7:lF(F("Uydu:TURKSAT"));break;
      case 8:lF(F("TP Ekle"));break;
      case 9:lF(F("[KAYDET]"));break;
    }
    if(edt&&idx==sIdx){while(lp<19)lA(' ');lA('*');}
    SE(i);}
}

void sub2(){
  for(byte i=0;i<3;i++){byte idx=sOfs+i;if(idx>9)break;SR(i,idx);
    lA(chFav[idx]?'*':' ');lA(chLock[idx]?'K':' ');
    switch(idx){
      case 0:lF(F("TRT 1 HD"));break;case 1:lF(F("ATV HD"));break;
      case 2:lF(F("Show TV"));break;case 3:lF(F("Star TV"));break;
      case 4:lF(F("Kanal D"));break;case 5:lF(F("FOX TV"));break;
      case 6:lF(F("TV8 HD"));break;case 7:lF(F("NTV HD"));break;
      case 8:lF(F("CNN Turk"));break;case 9:lF(F("HaberGlob"));break;
    }SE(i);}
}

void sub3(){
  const char*tp[]={"11096 H 30000","11180 V 30000","12015 H 27500",
    "12053 V 27500","12130 H 27500","12188 V 27500",
    "12245 H 27500","12303 V 27500","12380 H 30000","12458 V 30000"};
  for(byte i=0;i<3;i++){byte idx=sOfs+i;if(idx>9)break;SR(i,idx);lS(tp[idx]);SE(i);}
}

void sub4(){
  const char*dv[]={"1.0","1.1","1.2"};
  for(byte i=0;i<3;i++){byte idx=sOfs+i;if(idx>9)break;SR(i,idx);
    switch(idx){
      case 0:lF(F("Ver:"));lS(dv[sDisVer]);break;
      case 1:lF(F("Port:"));lN(sDis+1);break;
      case 2:lF(F("Tone:"));lF(sTone?F("ACIK"):F("KPLI"));break;
      case 3:lF(F("Motor:USALS"));break;
      case 4:lF(F("Pozisyoner"));break;
      case 5:lF(F("Tekrar: 1"));break;
      case 6:lF(F("Test Komutu"));break;
      case 7:lF(F("Motor Limit"));break;
      case 8:lF(F("Ref Uydu"));break;
      case 9:lF(F("[KAYDET]"));break;
    }SE(i);}
}

void sub5(){
  for(byte i=0;i<3;i++){byte idx=sOfs+i;if(idx>9)break;SR(i,idx);
    switch(idx){
      case 0:lF(F("Ad: TURKSAT"));break;
      case 1:lF(F("Favori:"));lF(chFav[0]?F("ACIK"):F("KPLI"));break;
      case 2:lF(F("Kilit:"));lF(chLock[0]?F("ACIK"):F("KPLI"));break;
      case 3:lF(F("Siralama"));break;case 4:lF(F("Grup Ata"));break;
      case 5:lF(F("EPG Ayari"));break;case 6:lF(F("Altyazi"));break;
      case 7:lF(F("Ses Dili"));break;case 8:lF(F("Kanal Sil"));break;
      case 9:lF(F("[KAYDET]"));break;
    }SE(i);}
}

void sub6(){
  lC();lF(F(" S:"));if(sLev<10)lA(' ');lN(sLev);lF(F("% "));
  byte b=map(sLev,0,100,0,10);
  for(byte i=0;i<10;i++)lA(i<b?(char)255:'-');lW(1);

  lC();lF(F(" Q:"));if(sQua<10)lA(' ');lN(sQua);
  lF(F("% V:"));int vI=(int)sVolA;int vD=(int)((sVolA-vI)*10);
  lN(vI);lA('.');lN(vD);lA('V');lW(2);

  lC();lA(sIdx==0?'>':' ');lF(F("L:"));lF(sLog?F("ON"):F("--"));
  lF(F(" Mn:"));lN(sLevMin);lF(F(" Mx:"));lN(sLevMax);lW(3);
}

void sub7(){
  const char*sp[]={"Yavas","Orta","Hizli"};
  for(byte i=0;i<3;i++){byte idx=sOfs+i;if(idx>9)break;SR(i,idx);
    switch(idx){
      case 0:lF(F("< Bati"));break;case 1:lF(F("> Dogu"));break;
      case 2:lF(F("Poz:"));lN(sMot);lA((char)223);break;
      case 3:lF(F("Poz Kaydet"));break;case 4:lF(F("Poz'a Git"));break;
      case 5:lF(F("BatiLim:"));lF(sMotLimB?F("SET"):F("--"));break;
      case 6:lF(F("DoguLim:"));lF(sMotLimD?F("SET"):F("--"));break;
      case 7:lF(F("Limit Sifirla"));break;
      case 8:lF(F("Hiz:"));lS(sp[sMotSpd]);break;
      case 9:lF(F("USALS Konum"));break;
    }SE(i);}
}

void sub8(){
  const char*lt[]={"Univrsl","Standrd","Widebnd"};
  for(byte i=0;i<3;i++){byte idx=sOfs+i;if(idx>9)break;SR(i,idx);
    switch(idx){
      case 0:lF(F("Tip:"));lS(lt[sLNBtyp]);break;
      case 1:lF(F("LOF L:9750"));break;case 2:lF(F("LOF H:10600"));break;
      case 3:lF(F("22kHz:"));lF(s22kHz?F("Auto"):F("Kpli"));break;
      case 4:lF(F("Volt:"));lN(sLNB);lA('V');break;
      case 5:lF(F("Guc:"));lF(sLNBpwr?F("ACIK"):F("KPLI"));break;
      case 6:lF(F("ToneBrst:"));lF(sToneB?F("AC"):F("KP"));break;
      case 7:lF(F("Unicab:"));lF(sUnicab?F("AC"):F("KP"));break;
      case 8:lF(F("MDU:"));lF(sMDU?F("ACIK"):F("KPLI"));break;
      case 9:lF(F("[KAYDET]"));break;
    }SE(i);}
}

void sub9(){
  unsigned long sn=millis()/1000;
  lC();lF(F(" Up: "));
  if(sn/3600<10)lA('0');lN(sn/3600);lA(':');
  if((sn%3600)/60<10)lA('0');lN((sn%3600)/60);lA(':');
  if(sn%60<10)lA('0');lN(sn%60);lW(1);
  lC();lF(F(" Olcum:#"));lN(sSigCnt);lW(2);
  lC();lF(F(" Uyku:"));lN(sleepTime);lF(F("s Frq:"));lN(sFreq);lW(3);
}

void sub10(){
  for(byte i=0;i<3;i++){byte idx=sOfs+i;if(idx>9)break;SR(i,idx);
    switch(idx){
      case 0:lF(F("Buzzer:"));lF(sMute?F("KAPALI"):F("ACIK"));break;
      case 1:lF(F("Seviye:"));lN(sVol);lA('%');break;
      case 2:lF(F("TusSesi:"));lF(sTusBip?F("AC"):F("KP"));break;
      case 3:case 4:case 5:case 6:lF(F("Ses Ayari"));break;
      case 7:lF(F("Ses Test"));break;
      case 8:lF(F("Tum Kapat"));break;
      case 9:lF(F("Frek:"));lN(sBuzFreq);lF(F("Hz"));break;
    }SE(i);}
}

void sub11(){
  const char*fn[]={"Kucuk","Orta","Buyuk"};
  for(byte i=0;i<3;i++){byte idx=sOfs+i;if(idx>9)break;SR(i,idx);
    switch(idx){
      case 0:lF(F("Parlak:"));lN(sBrt);lA('%');break;
      case 1:lF(F("Kontrst:"));lN(sKont);lA('%');break;
      case 2:lF(F("Isik:"));lF(sIsik?F("ACIK"):F("KPLI"));break;
      case 3:lF(F("Timeout:"));lN(sTout);lA('s');break;
      case 4:lF(F("Renk:Normal"));break;
      case 5:lF(F("Font:"));lS(fn[sFont]);break;
      case 6:case 7:lF(F("Ekran Ayari"));break;
      case 8:lF(F("Test Desen"));break;
      case 9:lF(F("[KAYDET]"));break;
    }SE(i);}
}

void sub12(){
  lC();lF(F(" Log:"));lF(sLog?F("AKTIF"):F("PASIF"));lF(F(" #"));lN(sSigCnt);lW(1);
  wF(2,F(" Baud:115200        "));
  lC();lA(sIdx==0?'>':' ');lF(F("[OK=Basla/Dur]"));lW(3);
}

void sub13(){
  const char*at[]={"Offset","Prime","Flat"};
  const char*fd[]={"Tek","Cift","Quad"};
  for(byte i=0;i<3;i++){byte idx=sOfs+i;if(idx>9)break;SR(i,idx);
    switch(idx){
      case 0:lF(F("Tip:"));lS(at[sATyp]);break;
      case 1:lF(F("Cap:"));lN(sACap);lF(F("cm"));break;
      case 2:lF(F("Azimut:"));lN(sAzim);lA((char)223);break;
      case 3:lF(F("Elev:"));lN(sElev);lA((char)223);break;
      case 4:lF(F("Skew:"));lN(sSkew);lA((char)223);break;
      case 5:lF(F("Feed:"));lS(fd[sFeed]);break;
      case 6:lF(F("Polar:"));lF(sAPol==0?F("Lineer"):F("Circul"));break;
      case 7:lF(F("Oto Arama"));break;
      case 8:lF(F("Kalibrasyon"));break;
      case 9:lF(F("[KAYDET]"));break;
    }SE(i);}
}

void loadBarChars(){
  byte b[8][8]={{0,0,0,0,0,0,0,31},{0,0,0,0,0,0,31,31},{0,0,0,0,0,31,31,31},
    {0,0,0,0,31,31,31,31},{0,0,0,31,31,31,31,31},{0,0,31,31,31,31,31,31},
    {0,31,31,31,31,31,31,31},{31,31,31,31,31,31,31,31}};
  for(byte i=0;i<8;i++)lcd.createChar(i,b[i]);
}

void sub14(){
  lC();lA(' ');lN(sFreq);lA(sPol==0?'H':'V');lF(F(" SR:"));lN(sSR);lW(1);

  unsigned long now=millis();
  if(now-spekLastT>=250){spekLastT=now;
    byte sc=(byte)((unsigned int)sLev*8/100);
    rndSeed^=(unsigned int)(now&0xFFFF);
    for(byte i=0;i<18;i++){
      byte pr=pgm_read_byte(&gaussProf[i]);
      int v=((int)pr*(int)sc)/8;
      byte lf=((lfoPhase+i*3)%6);if(lf>3)lf=6-lf;
      v+=(lf-1);v+=(int)pRnd(2)-1;
      if(v<0)v=0;if(v>7)v=7;
      spekVals[i]=(byte)v;
      if(spekVals[i]>spekPeak[i])spekPeak[i]=spekVals[i];
      else if(spekPeak[i]>0)spekPeak[i]--;
    }
  }

  lcd.setCursor(0,2);lcd.print('|');
  for(byte i=0;i<18;i++){
    byte val=spekVals[i];if(spekPeak[i]>val)val=spekPeak[i];
    if(val>0)lcd.write((byte)(val-1));else lcd.print(' ');
  }
  lcd.print('|');

  lC();lF(F(" S:"));lN(sLev);lF(F("% Q:"));lN(sQua);lF(F("% "));
  if(sLev>=70)lF(F("OK"));else if(sLev>=40)lF(F("--"));else lF(F("!!"));
  lW(3);
}

void sub15(){
  for(byte i=0;i<3;i++){byte idx=sOfs+i;if(idx>9)break;SR(i,idx);
    switch(idx){
      case 0:lF(F("FW: v4.3"));break;case 1:lF(F("Guncelle Kontrol"));break;
      case 2:lF(F("USB Yukle"));break;case 3:lF(F("SD Yukle"));break;
      case 4:lF(F("Serial Yukle"));break;case 5:lF(F("Yedek Al"));break;
      case 6:lF(F("Yedek Yukle"));break;case 7:lF(F("Bootloader"));break;
      case 8:lF(F("Checksum"));break;case 9:lF(F("Fabrika FW"));break;
    }SE(i);}
}

void sub16(){
  const char*dl[]={"Turkce","English","Deutsch","Francais","Espanol",
    "Italiano","Portugues","Russian","Arabic","Chinese"};
  for(byte i=0;i<3;i++){byte idx=sOfs+i;if(idx>9)break;SR(i,idx);
    lA(idx==sLang?'*':' ');lS(dl[idx]);SE(i);}
}

void sub17(){
  for(byte i=0;i<3;i++){byte idx=sOfs+i;if(idx>9)break;SR(i,idx);
    switch(idx){
      case 0:lF(F("IP:192.168.1.100"));break;
      case 1:lF(F("Mask:255.255.255"));break;
      case 2:lF(F("GW:192.168.1.1"));break;
      case 3:lF(F("DNS:8.8.8.8"));break;
      case 4:lF(F("DHCP:"));lF(sDHCP?F("ACIK"):F("KPLI"));break;
      case 5:lF(F("MAC Adresi"));break;
      case 6:lF(F("WiFi:N/A"));break;case 7:lF(F("BT:N/A"));break;
      case 8:lF(F("Web:N/A"));break;case 9:lF(F("[KAYDET]"));break;
    }SE(i);}
}

void sub18(){
  lC();lF(F(" FW:4.3 RAM:"));lN(freeRam());lW(1);
  lC();lF(F(" EE:"));lF(EEPROM.read(EE_OK)==MAGIC?F("OK"):F("--"));
  lF(F(" Brt:"));lN(sBrt);lA('%');lW(2);
  unsigned long sn=millis()/1000;
  lC();lF(F(" Up:"));lN(sn/3600);lA('h');lN((sn%3600)/60);lA('m');lN(sn%60);lA('s');lW(3);
}

void sub19(){
  for(byte i=0;i<3;i++){byte idx=sOfs+i;if(idx>9)break;SR(i,idx);
    switch(idx){
      case 0:lF(F("!!! DIKKAT !!!"));break;
      case 1:lF(F("Tum ayar silinir"));break;
      case 2:lF(F("Kanallar silinir"));break;
      case 3:lF(F("TP sifirlanir"));break;
      case 4:lF(F("Motor sifirlanir"));break;
      case 5:lF(F("EEPROM temizlenir"));break;
      case 6:lF(F("Varsayilan yukle"));break;
      case 7:lF(F("Yedek alinmadi!"));break;
      case 8:lF(F("Emin misiniz?"));break;
      case 9:lF(F("[ONAYLA]"));break;
    }SE(i);}
}

// Factory
void fabR(){
  clr();animProg(F("Fabrika Sifirla"),2000);
  for(byte i=0;i<52;i++){EEPROM.update(i,0xFF);delay(1);}
  sFreq=11096;sPol=0;sSR=30000;sFEC=0;sDmod=0;sPIDV=101;sPIDA=102;
  sDis=0;sDisVer=0;sTone=0;sMot=0;sMotSpd=1;sMotLimB=0;sMotLimD=0;
  sLNB=13;sLNBtyp=0;s22kHz=1;sLNBpwr=1;sToneB=0;sUnicab=0;sMDU=0;
  sVol=50;sMute=0;sTusBip=1;sBuzFreq=2000;
  sBrt=80;sKont=50;sIsik=1;sTout=30;sFont=1;
  sATyp=0;sACap=80;sAzim=180;sElev=35;sSkew=0;sFeed=0;sAPol=0;
  sDHCP=1;sLang=0;sLog=0;sleepTime=10;
  tkgsOn=1;tkgsAuto=1;tkgsLCN=1;tkgsBolge=0;
  sLevMin=100;sLevMax=0;sSigCnt=0;
  for(byte i=0;i<10;i++){chFav[i]=0;chLock[i]=0;}
  lcd.backlight();setBrt(sBrt);svEE();animOK();delay(300);
  inSub=0;sIdx=0;sOfs=0;edt=0;hdr();showM();
}

// Adj
void adj(int d){
  switch(cur){
    case 1:
      if(sIdx==0){sFreq+=d;sFreq=constrain(sFreq,10700,12750);}
      if(sIdx==1)sPol=!sPol;
      if(sIdx==2){sSR+=d*100;sSR=constrain(sSR,1000,45000);}
      if(sIdx==5){sPIDV+=d;sPIDV=constrain(sPIDV,0,8191);}
      if(sIdx==6){sPIDA+=d;sPIDA=constrain(sPIDA,0,8191);}
      break;
    case 7:if(sIdx==2){sMot+=d;sMot=constrain(sMot,-180,180);}break;
    case 10:
      if(sIdx==1){sVol+=d*5;sVol=constrain(sVol,0,100);
        if(!sMute){tone(BZ,sBuzFreq,20);delay(25);noTone(BZ);}}
      if(sIdx==9){sBuzFreq+=d*100;sBuzFreq=constrain(sBuzFreq,200,5000);
        if(!sMute){tone(BZ,sBuzFreq,40);delay(45);noTone(BZ);}}
      break;
    case 11:
      if(sIdx==0){sBrt+=d*10;sBrt=constrain(sBrt,0,100);setBrt(sBrt);}
      if(sIdx==1){sKont+=d*10;sKont=constrain(sKont,10,100);}
      break;
    case 13:
      if(sIdx==1){sACap+=d*10;sACap=constrain(sACap,30,300);}
      if(sIdx==2){sAzim+=d;sAzim=constrain(sAzim,0,360);}
      if(sIdx==3){sElev+=d;sElev=constrain(sElev,0,90);}
      if(sIdx==4){sSkew+=d;sSkew=constrain(sSkew,-90,90);}
      break;
  }
  sub();
}

// LCD
void hdr(){wF(0,F("--- OBM32 MENU ---  "));}
void clr(){wF(1,F("                    "));wF(2,F("                    "));wF(3,F("                    "));}

void showM(){
  clr();
  for(byte i=0;i<3&&(mOfs+i)<MNUM;i++){
    byte mi=mOfs+i;lC();lA(mi==cur?'>':' ');
    if(mi<10)lA(' ');lN(mi);lA('.');getMn(mi);lS(buf);lW(i+1);
  }
}

// Serial
void serTask(){
  while(Serial.available()){
    char c=Serial.read();
    if(c=='\n'||c=='\r'){if(cmdLen>0){cmdBuf[cmdLen]=0;runCmd();cmdLen=0;}}
    else if(cmdLen<30){cmdBuf[cmdLen++]=c;}
  }
}

long pVal(){char*p=strchr(cmdBuf,' ');if(p)return atol(p+1);return-1;}

void runCmd(){
  if(strcmp(cmdBuf,"STATUS")==0){
    getMn(cur);Serial.print(F("M:"));Serial.println(buf);
    Serial.print(F("R:"));Serial.print(freeRam());
    Serial.print(F(" S:"));Serial.print(sLev);
    Serial.print(F(" Q:"));Serial.print(sQua);
    Serial.print(F(" F:"));Serial.print(sFreq);
    Serial.print(sPol==0?'H':'V');
    Serial.print(F(" SR:"));Serial.println(sSR);
    Serial.print(F("Brt:"));Serial.print(sBrt);
    Serial.print(F(" Vol:"));Serial.print(sVol);
    Serial.print(F(" Slp:"));Serial.println(sleepTime);
  }
  else if(strcmp(cmdBuf,"LOG_START")==0){sLog=1;sLevMin=100;sLevMax=0;sSigCnt=0;svEE();Serial.println(F("T;S;Q;V"));}
  else if(strcmp(cmdBuf,"LOG_STOP")==0){sLog=0;svEE();Serial.print(F("#"));Serial.println(sSigCnt);}
  else if(strcmp(cmdBuf,"WAKE")==0){if(sleeping)wakeUp();Serial.println(F("OK"));}
  else if(strcmp(cmdBuf,"SLEEP")==0){if(!sleeping)enterSleep();Serial.println(F("OK"));}
  else if(strcmp(cmdBuf,"MUTE")==0){sMute=!sMute;svEE();Serial.println(sMute?F("ON"):F("OFF"));}
  else if(strcmp(cmdBuf,"BIP")==0){tone(BZ,sBuzFreq,200);delay(210);noTone(BZ);}
  else if(strcmp(cmdBuf,"FACTORY")==0){fabR();}
  else if(strcmp(cmdBuf,"DUMP")==0){for(byte i=0;i<52;i++){Serial.print(i);Serial.print(':');Serial.println(EEPROM.read(i));}}
  // SET komutlari
  else if(strncmp(cmdBuf,"SF ",3)==0){long v=pVal();if(v>=10700&&v<=12750){sFreq=v;svEE();Serial.println(sFreq);}else Serial.println(F("ERR"));}
  else if(strncmp(cmdBuf,"SP ",3)==0){sPol=(cmdBuf[3]=='V'||cmdBuf[3]=='v')?1:0;svEE();Serial.println(sPol?'V':'H');}
  else if(strncmp(cmdBuf,"SS ",3)==0){long v=pVal();if(v>=1000&&v<=45000){sSR=v;svEE();Serial.println(sSR);}else Serial.println(F("ERR"));}
  else if(strncmp(cmdBuf,"SB ",3)==0){long v=pVal();if(v>=0&&v<=100){sBrt=v;setBrt(sBrt);svEE();Serial.println(sBrt);}else Serial.println(F("ERR"));}
  else if(strncmp(cmdBuf,"SV ",3)==0){long v=pVal();if(v>=0&&v<=100){sVol=v;svEE();Serial.println(sVol);}else Serial.println(F("ERR"));}
  else if(strncmp(cmdBuf,"SL ",3)==0){long v=pVal();if(v==13||v==18){sLNB=v;svEE();Serial.println(sLNB);}else Serial.println(F("ERR"));}
  else if(strncmp(cmdBuf,"SM ",3)==0){long v=pVal();if(v>=-180&&v<=180){sMot=v;svEE();Serial.println(sMot);}else Serial.println(F("ERR"));}
  else if(strncmp(cmdBuf,"SD ",3)==0){long v=pVal();if(v>=1&&v<=4){sDis=v-1;svEE();Serial.println(sDis+1);}else Serial.println(F("ERR"));}
  else if(strncmp(cmdBuf,"ST ",3)==0){long v=pVal();if(v>=5&&v<=120){sleepTime=v;svEE();Serial.println(sleepTime);}else Serial.println(F("ERR"));}
  else if(strcmp(cmdBuf,"HELP")==0){
    Serial.println(F("STATUS LOG_START LOG_STOP"));
    Serial.println(F("WAKE SLEEP MUTE BIP FACTORY DUMP"));
    Serial.println(F("SF n=Freq SP H/V SS n=SR"));
    Serial.println(F("SB n=Brt SV n=Vol SL 13/18"));
    Serial.println(F("SM n=Mot SD n=DiS ST n=Sleep"));
  }
  else{Serial.print(F("?:"));Serial.println(cmdBuf);}

  if(!sleeping&&inSub)sub();
}

// EEPROM
void svEE(){
  EEPROM.update(EE_OK,MAGIC);
  EEPROM.update(EE_FREQ,sFreq&0xFF);EEPROM.update(EE_FREQ+1,(sFreq>>8)&0xFF);
  EEPROM.update(EE_FREQ+2,(sFreq>>16)&0xFF);EEPROM.update(EE_FREQ+3,(sFreq>>24)&0xFF);
  EEPROM.update(EE_POL,sPol);
  EEPROM.update(EE_SR,sSR&0xFF);EEPROM.update(EE_SR+1,(sSR>>8)&0xFF);
  EEPROM.update(EE_DIS,sDis);
  EEPROM.update(EE_MOT,sMot&0xFF);EEPROM.update(EE_MOT+1,(sMot>>8)&0xFF);
  EEPROM.update(EE_LNB,sLNB);EEPROM.update(EE_VOL,sVol);
  EEPROM.update(EE_LOG,sLog?1:0);EEPROM.update(EE_MUTE,sMute?1:0);
  EEPROM.update(EE_BRT,sBrt);EEPROM.update(EE_LANG,sLang);
  EEPROM.update(EE_TKGS,tkgsOn?1:0);EEPROM.update(EE_TAUT,tkgsAuto?1:0);
  EEPROM.update(EE_LCN,tkgsLCN?1:0);EEPROM.update(EE_BOLG,tkgsBolge);
  EEPROM.update(EE_FEC,sFEC);EEPROM.update(EE_DMOD,sDmod);
  EEPROM.update(EE_22K,s22kHz?1:0);EEPROM.update(EE_TONE,sTone?1:0);
  EEPROM.update(EE_UCAB,sUnicab?1:0);EEPROM.update(EE_MSPD,sMotSpd);
  EEPROM.update(EE_ATYP,sATyp);
  EEPROM.update(EE_ACAP,sACap&0xFF);EEPROM.update(EE_ACAP+1,(sACap>>8)&0xFF);
  EEPROM.update(EE_AZIM,sAzim&0xFF);EEPROM.update(EE_AZIM+1,(sAzim>>8)&0xFF);
  EEPROM.update(EE_ELEV,sElev&0xFF);EEPROM.update(EE_ELEV+1,(sElev>>8)&0xFF);
  EEPROM.update(EE_SKEW,sSkew&0xFF);EEPROM.update(EE_SKEW+1,(sSkew>>8)&0xFF);
  EEPROM.update(EE_FEED,sFeed);EEPROM.update(EE_APOL,sAPol);
  EEPROM.update(EE_DHCP,sDHCP?1:0);EEPROM.update(EE_SCRN,sKont);
  EEPROM.update(EE_TOUT,sTout);EEPROM.update(EE_FONT,sFont);
  EEPROM.update(EE_TUSB,sTusBip?1:0);EEPROM.update(EE_SLPT,sleepTime);
  EEPROM.update(EE_BFRQ,sBuzFreq/100);
}

void ldEE(){
  if(EEPROM.read(EE_OK)!=MAGIC){svEE();return;}
  sFreq=(unsigned long)EEPROM.read(EE_FREQ)|((unsigned long)EEPROM.read(EE_FREQ+1)<<8)|
    ((unsigned long)EEPROM.read(EE_FREQ+2)<<16)|((unsigned long)EEPROM.read(EE_FREQ+3)<<24);
  if(sFreq<10700||sFreq>12750)sFreq=11096;
  sPol=EEPROM.read(EE_POL);if(sPol>1)sPol=0;
  sSR=EEPROM.read(EE_SR)|((unsigned int)EEPROM.read(EE_SR+1)<<8);
  if(sSR<1000||sSR>45000)sSR=30000;
  sDis=EEPROM.read(EE_DIS);if(sDis>3)sDis=0;
  sMot=(int)(EEPROM.read(EE_MOT)|((int)EEPROM.read(EE_MOT+1)<<8));
  if(sMot<-180||sMot>180)sMot=0;
  sLNB=EEPROM.read(EE_LNB);if(sLNB!=13&&sLNB!=18)sLNB=13;
  sVol=EEPROM.read(EE_VOL);if(sVol>100)sVol=50;
  sLog=EEPROM.read(EE_LOG)==1;sMute=EEPROM.read(EE_MUTE)==1;
  sBrt=EEPROM.read(EE_BRT);if(sBrt>100)sBrt=80;
  sLang=EEPROM.read(EE_LANG);if(sLang>9)sLang=0;
  tkgsOn=EEPROM.read(EE_TKGS)==1;tkgsAuto=EEPROM.read(EE_TAUT)==1;
  tkgsLCN=EEPROM.read(EE_LCN)==1;
  tkgsBolge=EEPROM.read(EE_BOLG);if(tkgsBolge>6)tkgsBolge=0;
  sFEC=EEPROM.read(EE_FEC);if(sFEC>5)sFEC=0;
  sDmod=EEPROM.read(EE_DMOD);if(sDmod>1)sDmod=0;
  s22kHz=EEPROM.read(EE_22K)==1;sTone=EEPROM.read(EE_TONE)==1;
  sUnicab=EEPROM.read(EE_UCAB)==1;
  sMotSpd=EEPROM.read(EE_MSPD);if(sMotSpd>2)sMotSpd=1;
  sATyp=EEPROM.read(EE_ATYP);if(sATyp>2)sATyp=0;
  sACap=EEPROM.read(EE_ACAP)|((int)EEPROM.read(EE_ACAP+1)<<8);
  if(sACap<30||sACap>300)sACap=80;
  sAzim=EEPROM.read(EE_AZIM)|((int)EEPROM.read(EE_AZIM+1)<<8);
  if(sAzim<0||sAzim>360)sAzim=180;
  sElev=EEPROM.read(EE_ELEV)|((int)EEPROM.read(EE_ELEV+1)<<8);
  if(sElev<0||sElev>90)sElev=35;
  sSkew=EEPROM.read(EE_SKEW)|((int)EEPROM.read(EE_SKEW+1)<<8);
  if(sSkew<-90||sSkew>90)sSkew=0;
  sFeed=EEPROM.read(EE_FEED);if(sFeed>2)sFeed=0;
  sAPol=EEPROM.read(EE_APOL);if(sAPol>1)sAPol=0;
  sDHCP=EEPROM.read(EE_DHCP)==1;
  sKont=EEPROM.read(EE_SCRN);if(sKont<10||sKont>100)sKont=50;
  sTout=EEPROM.read(EE_TOUT);if(sTout<10||sTout>120)sTout=30;
  sFont=EEPROM.read(EE_FONT);if(sFont>2)sFont=1;
  sTusBip=EEPROM.read(EE_TUSB)==1;
  sleepTime=EEPROM.read(EE_SLPT);if(sleepTime<5||sleepTime>120)sleepTime=10;
  byte bf=EEPROM.read(EE_BFRQ);sBuzFreq=(unsigned int)bf*100;
  if(sBuzFreq<200||sBuzFreq>5000)sBuzFreq=2000;
  sIsik=1;
}
