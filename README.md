# Av Ses Teybi (ESP32 MP3/WAV Çalar)

Av sırasında kuş ve hayvan seslerini çalmak için yapılmış, 12 V ile çalışan bir ses teybi. Sesler micro SD karttan okunur. ESP32 sesi I2S üzerinden PCM5102A DAC'a gönderir, TPA3116D2 D sınıfı amfi de hoparlörü sürer. Seçilen ses kesintisiz döngüde çalar. Cihaz kapatılıp açıldığında en son seçilen parçadan devam eder.

```
 micro SD ──SPI──▶ ESP32 ──I2S──▶ PCM5102A DAC ──analog L/R──▶ TPA3116D2 ──▶ Hoparlör(ler)
                     ▲
        NEXT / PAUSE / BACK butonları
```

## Proje içeriği

```
teyip/
├── code/
│   └── kod.ino                 # ESP32 Arduino yazılımı
└── schema/
    └── teyip_devre.pdsprj      # Proteus projesi (ISIS şeması + ARES PCB yerleşimi)
```

---

## Yazılım özellikleri

| Özellik | Açıklama |
|---|---|
| **Desteklenen formatlar** | `.mp3` ve `.wav` (`.MP3` / `.WAV` de olur). WAV dosyaları PCM olmalıdır. |
| **Otomatik parça listesi** | Açılışta SD kartın **kök dizini** taranır ve bulunan ses dosyaları listeye eklenir. |
| **Sürekli tekrar (loop)** | Çalan parça bitince **aynı parça** baştan başlar. Bir av sesi saatlerce kendiliğinden tekrar eder. |
| **Kaldığı yerden devam** | Seçilen parçanın sırası ESP32'nin kalıcı hafızasına (NVS, `Preferences`) yazılır. Cihaz yeniden açıldığında o parça çalmaya başlar. |
| **Güvenli geri yükleme** | SD karttaki dosya sayısı azalmışsa ve kayıtlı sıra artık geçersizse ilk parçaya dönülür. |
| **Buton kontrolü** | Sonraki / önceki parça ve duraklat / devam. Liste başa ve sona döner (son parçadan sonra ilk parçaya geçer). |
| **Buton parazit önleme** | Basışlar arasında 400 ms bekleme süresi var. Butonu basılı tutarsanız parça her 400 ms'de bir değişir. |
| **Ses seviyesi** | Yazılımda sabittir: `out->SetGain(0.85)`. |
| **Seri port günlüğü** | 115200 baud hızında durum mesajları yazar (aşağıya bakın). |

### Butonlar

| Buton | GPIO | Görevi |
|---|---|---|
| **NEXT** | 17 | Sonraki parça |
| **BACK** | 4 | Önceki parça |
| **PAUSE** | 16 | Duraklat / devam ettir |

Butonların bir ucu GPIO'ya, diğer ucu GND'ye bağlıdır. ESP32'nin dahili pull-up dirençleri kullanıldığı için harici direnç gerekmez. Butona basıldığında pin LOW olur.

### Seri port mesajları (115200 baud)

```
Listeye eklendi: /keklik.mp3
Caliniyor: /keklik.mp3
Cihaz acildi, 0. parcadan devam ediliyor.
SD Hatasi!            ← SD kart takılı değil veya okunamıyor
```

---

## Donanım özellikleri

Bilgiler Proteus şemasından (`schema/teyip_devre.pdsprj`) ve PCB net listesinden çıkarıldı.

### Malzeme listesi

| Ref | Parça | Görevi |
|---|---|---|
| **U1** | ESP32 geliştirme kartı (38 pin, ESP32-WROOM-32) | Ana işlemci: SD okuma, MP3/WAV çözme, I2S ses çıkışı |
| **P1** | PCM5102A I2S DAC modülü (şemada `PCA2`) | Dijital sesi stereo analog sinyale çevirir |
| **U2** | TPA3116D2 stereo D sınıfı amfi modülü | Hoparlörü sürer, doğrudan +12 V ile beslenir |
| **L1** | Micro SD kart modülü (SPI) | Ses dosyalarının bulunduğu kart |
| **M1** | MINI360 DC-DC düşürücü (buck) modül | 12 V → 5 V (ESP32 beslemesi) |
| **D1** | 1N5822 Schottky diyot (3 A) | Negatif hatta seri bağlı, **ters polarite koruması** |
| **C1** | 1000 µF elektrolitik kondansatör | +12 V hattında filtre / tampon (amfinin ani akım çekişleri için) |
| **J1** | 2'li klemens | 12 V DC güç girişi |
| **B1, B2, B3** | Push buton | NEXT, PAUSE, BACK |

### Güç dağıtımı

```
J1 (+) ─────────────────────────────── +12V ─┬── C1 1000µF ── GND
                                             ├── U2 TPA3116D2  VCC
                                             └── M1 MINI360 VIN+
                                                     │
                                                     └─ VOUT+ ── +5V ── U1 ESP32 "5V" pini
                                                                           │
                                                                ESP32 "3V3" pini ── +3.3V ─┬── L1 SD modülü (3.3V pini)
                                                                                           └── P1 PCM5102A VIN
J1 (–) ── D1 1N5822 ── GND
```

- Cihaz **12 V DC** ile çalışır, örneğin 12 V akü.
- D1, girişin negatif ucuna seri bağlıdır. Güç kablosu ters takılırsa akım geçmez ve devre zarar görmez.
- 3,3 V hattı ESP32 kartının kendi regülatöründen gelir. SD modülü ve DAC bu hattan beslenir. SD modülünün `5v` pini boş bırakılmıştır.

### ESP32 pin bağlantıları

| ESP32 pini | Bağlandığı yer | Koddaki tanım |
|---|---|---|
| GPIO 27 | SD modülü **CS** | `SD_CS` |
| GPIO 13 | SD modülü **MOSI** | `SPI_MOSI` |
| GPIO 19 | SD modülü **MISO** | `SPI_MISO` |
| GPIO 14 | SD modülü **SCK** | `SPI_SCK` |
| GPIO 25 | PCM5102A **DIN** | `I2S_DOUT` |
| GPIO 26 | PCM5102A **BCK** | `I2S_BCLK` |
| GPIO 22 | PCM5102A **LCK** (LRCK) | `I2S_LRC` |
| GPIO 17 | NEXT butonu | `BTN_NEXT` |
| GPIO 4 | BACK butonu | `BTN_PREV` |
| GPIO 16 | PAUSE butonu | `BTN_PP` |
| 5V | MINI360 çıkışı (+5V) | – |
| 3V3 | +3.3V hattı (SD + DAC) | – |
| GND (×3) | Ortak toprak | – |

### PCM5102A DAC ayar pinleri

| Pin | Bağlantı | Anlamı |
|---|---|---|
| SCK | GND | Harici sistem saati yok, DAC saati BCK'den kendi üretir (dahili PLL) |
| FMT | GND | Veri formatı: standart I2S |
| FLT | GND | Normal gecikmeli dijital filtre |
| DEMP | GND | De-emphasis kapalı |
| XSMT | A3V3 (HIGH) | Yumuşak susturma kapalı, ses çıkışı açık |
| LOUT → | U2 **INL** | Sol kanal |
| ROUT → | U2 **INR** | Sağ kanal |
| AGND → | U2 **GND1** | Ses sinyali toprağı (`GND1` adlı ayrı net) |

### TPA3116D2 amfi

- **VCC = +12 V**. Amfi MINI360'tan değil, doğrudan giriş hattından beslenir. Bu sayede yüksek güç verebilir.
- Girişleri (INL / INR / GND1) DAC çıkışına bağlıdır.
- Hoparlör çıkışları (SPL± / SPR±) şemada bağlanmamıştır. Hoparlörler doğrudan amfi modülünün kendi klemenslerine bağlanır.

---

## Kurulum

### 1. Yazılımı yükleme

1. **Arduino IDE**'ye ESP32 kart paketini kurun (*Boards Manager → "esp32" by Espressif*). **2.x veya üzeri** bir sürüm kullanın. Eski 1.0.x sürümlerinde `entry.name()` tam yol döndürür ve kodun oluşturduğu dosya yolları bozulur.
2. *Library Manager*'dan **ESP8266Audio** (Earle F. Philhower) kütüphanesini kurun. `SD`, `SPI` ve `Preferences` ESP32 paketiyle birlikte gelir.
3. Kart olarak **ESP32 Dev Module**'ü seçin, `code/kod.ino` dosyasını açıp yükleyin.

### 2. SD kartı hazırlama

- Kartı **FAT32** olarak biçimlendirin.
- `.mp3` / `.wav` dosyalarını **kök dizine** kopyalayın. Alt klasörler taranmaz.
- **Çalma sırası alfabetik değildir.** Dosyalar karta kopyalandıkları sırayla listelenir. Sıra önemliyse kartı boşaltıp dosyaları istediğiniz sırayla tek tek kopyalayın.
- Mac ile hazırlanan kartlarda oluşan `._dosya.mp3` gibi gizli dosyaları silin. Bunlar da listeye eklenir ama çalmaz.

### 3. İlk çalıştırma

1. ESP32'yi takmadan önce **MINI360 çıkışını trimpot ile 5,0 V'a ayarlayın.** Ayarsız bir modül ESP32'ye fazla gerilim verebilir.
2. 12 V girişin kutuplarını kontrol edip gücü verin.
3. Seri port ekranında (115200 baud) dosyaların listelendiğini ve çalmanın başladığını görün.

---

## Bilinen sınırlamalar

- **SD kart takılı değilse veya kartta ses dosyası yoksa** cihaz ses çıkarmaz. Bu durumda bir butona basılırsa ESP32 çöker ve yeniden başlar: parça sayısı 0 olduğu için sıfıra bölme oluşur ve `out` nesnesi oluşturulmamış olur.
- Açılışta parçanın **en başından** çalınır. Parça içindeki konum kaydedilmez.
- Uzantılar yalnızca `.mp3`, `.MP3`, `.wav`, `.WAV` biçiminde tanınır. `.Mp3` gibi karışık yazımlar atlanır.
- Bozuk ya da desteklenmeyen bir dosya seçilirse çalma durur. Buton ile başka parçaya geçmek gerekir.
- Proteus şemasındaki özel parçaların (ESP32, PCM5102A, TPA3116D2, MINI360, SD modülü) simülasyon modeli yoktur. Şema simülasyon için değil, **bağlantı ve PCB tasarımı** için hazırlanmıştır. Özel parçalar proje dosyasının içinde kayıtlıdır (kütüphane kategorisi: `TEYİP`).

---

> **Not:** Türkiye dahil birçok ülkenin av mevzuatı, elektronik ses cihazlarıyla avlanmayı yasaklayabilir veya kısıtlayabilir. Kullanmadan önce bulunduğunuz bölgedeki kuralları kontrol edin.
