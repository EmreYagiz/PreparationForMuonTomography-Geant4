# 8 cm ek kosu

Sintilator aktif yuzeyleri arasi 8 cm, blok 5 cm kalinliginda ortadadir.
Blok ile iki aktif yuzey arasi 1.5 cm (Teflon dis yuzeyine 1.4 cm).
Kaynak, XY hizasi, 90 derece alt modul donusu ve PMT ayarlari korunur.
100000 bloksuz ve 100000 bloklu olay, bagimsiz yeni rastgele tohumlar.

B1/build icinden kullanici baslatir:

    bash run_8cm.sh mesafe_8cm_100k_1

Iki kosu bitince ortak C++ analizi:

    root -l -b -q 'mesafe_analiz.C("mesafe_100k_1","mesafe_8cm_100k_1")'

Eski 10/15/20 cm ROOT dosyalari ilk klasorden, yeni 8 cm dosyalari ikinci
klasorden okunur. Kopyalama veya eski dosyalarin uzerine yazma yapilmaz.
Ortak sonuclar mesafe_100k_1/analiz_8_20/ klasorune kaydedilir.
Ilk uc mesafenin analiz/ klasoru korunur.
