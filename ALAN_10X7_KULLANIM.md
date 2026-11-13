# 10 x 7 cm ortak alan

Ortak dikey izdusum X=[-3.5,3.5], Y=[-5,5] cm: tam 70 cm2.
Ust merkez X=+1.5, Y=+4.3 cm; alt merkez X=-5.8, Y=0 cm.
Her sintilator kendi Teflonu ve PMTsiyle birlikte kayar. Fiziksel sekil,
optik ozellikler ve yatay duzlemdeki alt modul 90 derece donusu aynidir.
5x5x5 cm Pb blok (0,0,0) merkezinde. Gap=10 cm; aktif yuzeylere 2.5 cm.
Kaynak global X=0, Y=4.3, Z=60 cm, eski kaynak agirligi ve 1-100 GeV/c.

Eski makrolar icin varsayilan overlapWidth=10 cm korunur. Yeni iki makro
/setup/overlapWidth 7 cm secimini acikca yapar. RunInfo'ya ortak alan
olculeri ve gercek ust merkez X konumu kaydedilir.

B1/build icinden kullanici baslatir:

    bash run_alan_10x7.sh alan_10x7_1

Iki kosu bitince:

    root -l -b -q 'analiz_alan.C("alan_10x7_1")'

Her kosu 100000 olay; iki kosuda 200000. Eski dosyalar degistirilmez.
Sonuclar alan_10x7_1/analiz/ altina (varsa numarali yeni klasore) yazilir.
Transmission/sigma ve onceki tanilama grafikleri birlikte olusur.
Eski mesafe verilerinde ek p/E kaynak carpani vardi. Bu denemede kullanici
istegiyle eski p*flux*cos(theta) agirligi vardir; ikisi arasindaki farki
yalnizca ortak alan degisikligine baglamak dogru degildir. Gerekirse ayni
agirlikla yeni 10x10 kontrol kosusu ayrica alinmalidir.
