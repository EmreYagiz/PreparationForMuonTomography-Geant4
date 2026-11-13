# 11 cm ve ayri grafikler

Aktif sintilator yuzeyleri arasi 11 cm; ortada 5x5x5 cm Pb blok.
Blok ile her aktif yuzey arasi 3 cm; Teflon dis yuzeyine 2.9 cm.
Kaynak, PMT, XY hizasi, optik ve esikler aynidir. Iki yeni bagimsiz kosu,
her biri 100000 olay. B1/build icinden kullanici baslatir:

    bash run_11cm.sh mesafe_11cm_100k_1

Bitince bes mesafeyi birlikte analiz et:

    root -l -b -q 'mesafe_analiz.C("mesafe_100k_1","mesafe_8cm_100k_1","mesafe_11cm_100k_1")'

Eski dort mesafeyi yeni grafiklerle analiz etmek de mumkun:

    root -l -b -q 'mesafe_analiz.C("mesafe_100k_1","mesafe_8cm_100k_1")'

Yeni ciktilar ana klasorde analiz_ayri_8_10_11_15_20/ altina yazilir.
11 cm verilmezse klasor adinda 11 bulunmaz. Var olan klasor korunur;
tekrar analizde _2, _3 gibi ek kullanilir. Son klasor terminale yazilir.

Her biri ayri PNG ve PDF: coincidence, transmission, transmission_family95,
azalma, sigma. Grafikler ayrica mesafe_analiz.root'a yazilir. CSV/txt ozeti
tum sayimlari, oranlari, araliklari, isaretli sigma ve Fisher p degerlerini icerir.

T=blok olasiligi/referans olasiligi. Transmission grafikleri yakinlastirilmis
Y ekseni kullanir, T=1 cizgisi ve tum hata cubuklari gorunur.
Ilk transmission grafigi mesafe basina %95, family95 sabit dahil edilen
mesafe grubu icin korumaci ortak %95 aralik verir. Araliklar tam binom
Clopper-Pearson araliklarinin Bonferroni oran zarfindan hesaplanir.

Sigma: bagimsiz iki binom oraninin sifir fark hipotezine gore havuzlanmis
standart hatayla hesaplanan yaklasik isaretli Z degeridir.
Pozitif Z azalma, negatif Z artis demektir. Onceki Poisson yaklasimi CSV'de
ayri tutulur. Tek yonlu Fisher tam p degeri ve dahil edilen mesafe sayisina
gore Bonferroni duzeltmesi de verilir; sigma bu tam p'den turetilmez.
Grafikte 0, +/-2 ve +/-3 yardimci cizgileri vardir; bunlar yerel yaklasik
degerlerdir. Z'ye yapay hata cubugu eklenmez.

Mesafeler onceki sonuclara bakilarak secildiginden, bu duzeltmeler tum
uyarlamali secim surecini kapsamaz. Grafik tepesine egri uydurma veya
otomatik en iyi mesafe karari verilmez. Uretilen olay sayisi dakika degildir.
