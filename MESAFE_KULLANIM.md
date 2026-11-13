# Mesafe karsilastirmasi

Mesafe iki aktif sintilatorun birbirine bakan yuzeyleri arasindadir.
5 x 5 x 5 cm kursun blok z=0 merkezinde kalir; iki modul simetrik hareket eder.
Mevcut XY hizasi ve alt modulun yatay duzlemde 90 derece donusu korunur.

| Sintilator yuzeyleri arasi | Blok ile her aktif yuzey arasi | Blok ile her Teflon dis yuzeyi arasi |
|---|---|---|
| 10 cm | 2.5 cm | 2.4 cm |
| 15 cm | 5 cm | 4.9 cm |
| 20 cm | 7.5 cm | 7.4 cm |

Her mesafede 100000 referans ve 100000 bloklu olay: toplam 600000.
6 ayri Geant4 sureci; her biri 8 thread kullanir. Ayni anda tek kosu calisir.
Kaynak z=60 cm'de sabit kalir. Momentum araligi 1-100 GeV/c, tam optik aciktir.
Olay sayisi fiziksel bekleme suresine donusturulmez. Dedektor kabulunun
mesafeye bagli degisimi icin her mesafenin kendi referansi kullanilir.
Sonuclar bu sonlu kaynak duzlemine aittir; kaynak-duzlemi boyutu icin bir
yakinsama sinamasi bu calismaya dahil degildir.

## Derle ve kendin baslat

```bash
source ~/Geant4/geant4-v11.3.2-install/bin/geant4.sh
cd ~/Geant4/geant4-v11.3.2/examples/basic/B1
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DGeant4_DIR=~/Geant4/geant4-v11.3.2-install/lib/cmake/Geant4
cmake --build build -j8
cd build
bash run_mesafe.sh mesafe_100k_1
```

Geant4 ortamindaki Geant4_DIR cache'te zaten varsa tekrar vermek gerekmez.
Var olan cikti klasorune yazilmaz. Yeniden kosuda mesafe_100k_2 gibi yeni ad ver.
Ilerleme terminalde ve her kosunun .log dosyasinda gorulur. Tamamlanan kosu
COMPLETED yazisini, script en sonda ALTI KOSU TAMAMLANDI yazisini verir.
Script devam ettirme araci degildir: yarida kesilirse biten dosyalar korunur;
kalan .mac dosyalari ayni cikti klasorunden tek tek calistirilabilir.
Olay sayisi degistirilecekse ilgili alti .mac dosyasindaki beamOn satirini
degistir; scriptteki 100000 metni yalniz bilgilendirme mesajidir.

## Alti kosudan sonra C++ ROOT analizi

```bash
root -l -b -q 'mesafe_analiz.C("mesafe_100k_1")'
```

Analiz tek dosya basina olaylari ve RunInfo worker toplamlarini dogrular;
blok durumu, kaynak, geometri, esik ve zaman ayarlarinin uyumunu denetler.
Yalniz PMT coincidence kullanir, primary truth filtresi uygulamaz.

Ciktilar mesafe_100k_1/analiz/ icinde:

- mesafe_karsilastirma.png ve .pdf: uc panel.
- mesafe_analiz.root: grafikler ve canvas, ROOT'ta tekrar acilabilir.
- mesafe_ozet.csv: olay, hit, coincidence, normalize oran ve belirsizlikler.
- ozet.txt: sayisal ozet, ayarlar ve yorumlama notlari.

Birinci panel uretilen muon basina coincidence olasiligini gosterir;
saniyedeki sayim degildir. Hata cubuklari tam binom %95 araliklardir.
Ikinci panel T=(blok coincidence/blok olay)/(referans coincidence/referans olay).
Ucuncu panel %100*(1-T). Negatif azalma sayim artisi anlamina gelir.
T ve azalma araliklari, her kosu icin %97.5 Clopper-Pearson araliklarinin
oran zarfindan gelir: mesafe basina en az %95 kapsama, uc mesafe icin ortak
%95 garanti degildir. Sadece nokta degerine gore en iyi mesafe secilmez.
Kayip coincidence tek basina muonun kursunda durdugunu kanitlamaz.

Mevcut analiz klasorunun uzerine yazilmaz; tekrar analizden once klasoru
yeniden adlandir. Eski olcumler, reference.mac ve run_block.mac degistirilmez.
