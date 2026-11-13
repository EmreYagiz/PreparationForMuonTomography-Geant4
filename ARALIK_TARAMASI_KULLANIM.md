# Mesafe taramasi: en fazla 30 cm

10,20,25,30 cm aktif sintilator yuzey araliklari. Ortak alan 10x10 cm.
5x5x5 cm Pb blok ortada; iki taraftaki aktif yuzey bosluklari sirasiyla
2.5,7.5,10,12.5 cm. Kaynak agirligi ve 1-100 GeV/c araligi ayni.

10 cm: alan_10x10_tekrar_1 verisi kullanilir. 20/25/30 cm: her birinde
100000 referans + 100000 bloklu olay; toplam 6 yeni kosu, 600000 olay.
50/70 cm bu betik tarafindan CALISTIRILMAZ. Eski veriler degistirilmez.

B1/build icinde kullanici baslatir:

    bash run_aralik.sh aralik_10_30_1

Bitince:

    root -l -b -q 'aralik_analiz.C("aralik_10_30_1")'

Ciktilar: aralik_10_30_1/analiz_ayri_10_20_25_30/ (varsa sayili ek).
Transmission, sigma ve tum tanilamalar ayri PNG/PDF ve ROOT olarak yazilir.
Ilk eski mesafe taramasindaki p/E kaynak agirlikli dosyalar kullanilmaz.
Yeni klasor ismi, daha once baslatilmis olabilecek farkli planlarla karismayi onler.
