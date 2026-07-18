# OBM32 Disqeci

**OBM32 Disqeci**, uydu sinyal seviyelerini izlemek, frekans yönetimi yapmak ve profesyonel sinyal takibi sağlamak için geliştirilmiş, yüksek performanslı bir gömülü sistem projesidir. **Omfo Balkan Microelectronics** ekibi tarafından, hassas saha çalışmaları ve sinyal optimizasyonu için tasarlanmıştır.

## 🚀 Proje Hakkında
Bu cihaz, uydu kurulumcuları ve RF meraklıları için özel olarak geliştirilmiş, taşınabilir bir "Sinyal Takip Ünitesi"dir. Arduino Nano platformu üzerinde çalışan sistem; gerçek zamanlı görsel geri bildirim, kanal yönetimi ve kalıcı EEPROM hafıza desteği sunar.

## 🛠️ Teknik Özellikler
*   **Mikrodenetleyici:** Arduino Nano (ATmega328P)
*   **Ekran:** 128x64 SSD1306 OLED (I2C)
*   **Hafıza:** 24LC128 EEPROM (Kalıcı Kanal Kaydı)
*   **Giriş Birimi:** 4x Push Button (Up, Down, Ok, Back)
*   **Haberleşme:** Seri Port (USB) üzerinden dinamik kanal konfigürasyonu

## 🔌 Bağlantı Şeması
| Bileşen | Pin (Arduino Nano) |
| :--- | :--- |
| **OLED SDA** | A4 |
| **OLED SCL** | A5 |
| **Buton UP** | D2 |
| **Buton DOWN** | D3 |
| **Buton OK** | D6 |
| **Buton BACK** | D7 |

## 💡 Kullanım Kılavuzu
1.  **Serial Bağlantı:** Cihazı bilgisayara bağlayın ve Arduino IDE Serial Monitör'ü **9600 baud** hızında açın.
2.  **Kanal Ekleme:** Serial Monitör satırına `KanalAdı,Frekans` (Örn: `STAR,12015`) formatında veri gönderin.
3.  **Hafızaya Alma:** Menü üzerinden **"EEPROM AKTAR"** seçeneğine gelerek gönderdiğiniz kanalın cihaz hafızasına kalıcı olarak kaydedilmesini sağlayın.
4.  **Navigasyon:** Butonları kullanarak "Sinyal Tara", "Ayarlar" ve "İletişim" sekmeleri arasında gezinin.

## 👥 Geliştirici Ekip
*   **Mete Onelge:** [@meteonelge0127](https://www.instagram.com/meteonelge0127)

---
*Proje OBM (Omfo Balkan Microelectronics) tarafından geliştirilmiştir.*
