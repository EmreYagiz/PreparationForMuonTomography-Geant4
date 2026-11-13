# Ek tanilama analizi

Yeni Geant4 kosusu gerekmiyor. Mevcut komut ayni:

    root -l -b -q 'mesafe_analiz.C("mesafe_100k_1","mesafe_8cm_100k_1","mesafe_11cm_100k_1~")'

Var olan cikti klasoru korunur; yeni analiz _2, _3 gibi ekli klasore gider.
Asil coincidence/transmission/azalma/sigma hesaplari korunur. Diagnostic
dosyalari SIMULATION etiketi tasir; deneyin dogrudan olcumleri degildir.

Ek ciktilar (her grafik ayri PNG, PDF; ROOT icinde de saklanir):

- diagnostic_block_coverage: uretilenlerde ve algilananlarda birincilin bloga giris oranlari.
- diagnostic_entry_outcomes: bloga giren ve ust sintilatoru ziyaret edenlerde
  iki sintilatore ulasma, coincidence ve blokta durma oranlari.
- diagnostic_detection: iki sintilatoru ziyaret eden birincile kosullu PMT coincidence orani.
- diagnostic_initial_ray: baslangictaki duz isini nominal blok kutusuna
  carpan grupta, referans ve bloklu coincidence oranlari.
- diagnostic_initial_ray_control: duz isini kutuyu iskalar gruptaki kontrol.
- diagnostic_count_decomposition: olculen sayim farkinin birincil-iki-ziyaret,
  PMT kacirma ve birincili iki sintilatore ulasmayan coincidence terimlerine
  cebirsel ayrisimi. Olaylarin neden kayboldugunun nedensel ayrisimi degildir.
- diagnostic_energy_loss_Ncm: tek giris/cikisli, durmamis birincillerin ilk
  giris-cikis kinetik enerji farki. 0-1000 MeV gorunur aralik disindaki
  degerler histogram alt/ust tasma hucrelerinde ve ozet istatistiginde korunur.
- diagnostic_counts.csv, diagnostic_decomposition.csv, diagnostic_summary.txt.

Kosullu oran paydasi sifirsa nokta cizilmez; sifir oran gibi gosterilmez.
Ziyaret kayitlari siralama veya tam gecis kaniti degildir. Cikis olmamasi
tek basina durma degildir. PrimaryStoppedInBlock kinetik enerjinin sifira
indigi kayittir. Kesin sacilma acisi kaydedilmediginden hesaplanamaz.
Bagimsiz referans/blok olaylari kayboldu/kazanildi seklinde eslestirilmez.
Referans/blok ortak initial-ray altgruplari sanal baslangic geometrisidir,
gercek blok gecisi veya deneyde olculmus iz degildir.
