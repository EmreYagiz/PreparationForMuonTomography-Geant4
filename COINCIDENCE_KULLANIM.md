# B1 — sabit kurşun blok, ROOT coincidence kaydı

Bu sürüm yalnızca batch çalışır: pencere, görüntü, tarama veya otomatik analiz üretmez.
Eski scan_H.py / plot_H.py dosyaları bu ölçümün parçası değildir.

## Düzenek

- Orijinal B1 sintilatörleri: 18.6 cm toplam uzunluk, 10 cm geniş gövde,
  4 cm dar uç, 1.4 cm kalınlık. Kaplama ve PMT modeli korunmuştur.
- Üst merkez (0,4.3,+15.7) cm; alt merkez (-4.3,0,-15.7) cm.
- Alt modül z ekseni etrafında tam 90° döner. Panel, kaplama ve PMT birlikte döner.
- İki panel yataydır. Aktif yüzeyler arası açıklık 30 cm.
- Geniş uç köşeleri üstten bakışta (5,-5) cm'de çakışır.
- Dikey izdüşüm ortak alanı 10×10 cm, merkezdeki kurşun blok 5×5×5 cm'dir.
  Bu en-boy önceki varsayımdır. Blok, bütün kabul edilen müon yollarını kapatmaz.

## Kaynak ve hız

Referans projenin kaynak aralıkları korunur: modified-Gaisser, p=1–100 GeV/c,
zenit 0–70°, 20×20 cm düzlem z=60 cm. Düzlem merkezi üst panele uyar:
(x,y)=(0,4.3) cm. Yakın kaynak düzlemine geçilmedi. Log-momentum
örneklemesine dE/dp=p/E Jacobian düzeltmesi uygulandı.

Bu alt momentum kesimi yaklaşık 900 MeV kinetik enerjiye karşılık gelir;
daha düşük enerjili müonlar üretilmez. Model tam düşük-enerjili açık gökyüzü
akısı değildir ve başlangıç olayı sayısı doğrudan deney süresi sayılmaz.

Optik foton taşınımı, ışık verimi ve QE aynı kalır. Sadece coincidence için
gerekmeyen çok örnekli dalga biçimi hesaplaması ve Oscilloscope ntuple'ı
çıkarılmıştır. Elektronik darbe yüksekliği ve gerçek yük bu sürümde üretilmez.
Fotoelektron sayısı sinyal büyüklüğü için model verisidir; deneysel yük kalibrasyonu değildir.

## Çalıştırma (Ubuntu/WSL)

```bash
source ~/Geant4/geant4-v11.3.2-install/bin/geant4.sh
cd ~/Geant4/geant4-v11.3.2/examples/basic/B1
cmake -S . -B build -DGeant4_DIR=$HOME/Geant4/geant4-v11.3.2-install/lib/cmake/Geant4 -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
cd build
./exampleB1 reference.mac
./exampleB1 run_block.mac
```

İlk komut blok olmadan 10.000, ikincisi blok varken 10.000 olay üretir.
Her ikisinde 8 worker vardır. Sayılar ve dosya adları ilgili .mac dosyasından
değiştirilir. `/run/beamOn 10000` başlangıç olayı sayısıdır.

Sonuçlar `build/reference.root` ve `build/block.root` olarak kaydedilir.
Ntuple merging ile bütün worker olayları aynı ROOT dosyasına yazılır.
Dosya yalnızca koşu sonunda tamamen yazılıp kapatılmış olur.
Var olan ROOT dosyasının üstüne yazılması reddedilir: yeni ölçümde
`/analysis/setFileName` değerini değiştirin veya yeni bir çıktı klasöründen çalıştırın.

`run1.mac` bloklu, `run2.mac` referans makrosunun kısa yoludur.
Çalıştırıcıdan ayrı bir Python/plot komutu çağrılmaz.

## Coincidence tanımı

Her PMT'de en az 1 PE ve ilk PE varışları için |t2-t1| ≤15 ns.
`/setup/peThreshold` ve `/setup/timeWindow` ile initialize öncesi değiştirilebilir.
PE eşiği toplam PE sayısına uygulanır; zaman hâlâ ilk PE zamanıdır.
Bu, gerçek analog diskriminatörün eşik geçiş modeli değildir.
Üst/alt tekli sayımlar, zaman şartı ve nihai coincidence ayrı alanlardır.
Hiçbir hit yokken dahi olay satırı yazılır; analizde sıfırlar kaybolmaz.

## ROOT içeriği

### MuonTelescope — her başlangıç olayı için bir satır

- Kimlik: RunID, EventID, WorkerID, BlockEnabled.
- Ölçüm/model sinyali: PEPMT1/2, PMT1Hit/2Hit, Coincidence, TimeCoincidence,
  PEThreshold, TimeWindow_ns, DeltaT_ns.
- PE zamanları: ilk, son, ortalama, RMS; üst/alt ayrı.
- Işık: PMT'ye ulaşan foton sayısı, Scintillation/Cerenkov/Other ayrımı,
  ilk ve ortalama foton transit zamanı.
- Enerji bırakma: EdepTop_MeV, EdepBottom_MeV, EdepBlock_MeV.
  Bunlar o hacimde tüm parçacıkların toplam enerji bırakmasıdır.
- Kaynak: kinetik MuonEnergy_GeV, momentum, yük, zenit, azimut,
  PrimaryStart konum ve yön bileşenleri.
- Tanılama: PrimaryVisitedTop/Bottom/Both, PrimaryEnteredBlock,
  PrimaryStoppedInBlock, giriş/çıkış sayıları, ilk giriş/ilk çıkış kinetik enerjileri,
  PrimaryEdep ve PrimaryPath (üst, alt, blok için ayrı).

Primary* alanları ve başlangıç müon enerjisi simülasyon bilgisidir; deneysel
PMT ölçümü gibi kullanılamaz. Bu bayraklar coincidence seçiminde kullanılmaz.
Hacmi ziyaret etmek tam kalınlığı geçmek veya belirli bir zaman sırasını garanti etmez.
PrimaryStoppedInBlock, blok içinde kinetik enerjinin sıfıra indiğini gösterir.
PE/foton yoksa ilgili zamanlar -1; enerji bırakma ve sayılar 0'dır.
Giriş/çıkış yoksa ilgili kinetik enerji -1'dir.

### RunInfo — her worker ve run için bir satır

Events, TopHits, BottomHits, Coincidences worker yerel sayılarıdır; aynı RunID
için worker satırları toplanır. Geometri, blok açık/kapalı, boyutlar, panel merkezleri,
açıklık, 90° dönüş, eşikler, kaynak sınırları ve optik takip sınırları kaydedilir.
Başlangıç rastgele üreteç durumu ve hesaplama WallTime_s da kaydedilir.
WallTime_s deneysel maruziyet süresi değildir. Çok iş parçacıklı bit düzeyinde
tekrar üretim yalnızca bu alanla garanti edilmez; makroları ve Geant4 sürümünü de koruyun.

Olay satırları worker sırasına göre karışık olabilir; (dosya, RunID, EventID)
ile eşleştirilir. RunInfo worker sayıları ile MuonTelescope toplamının eşleşmesi
sonraki analiz kodunda kontrol edilmelidir.

Bu aşamada analiz kodu veya fizik sonucu üretilmedi. Kod derlendi; parçacık
koşularını kullanıcı başlatacaktır.
