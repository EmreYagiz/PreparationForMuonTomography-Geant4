# Bagimsiz tekrar: 10x10 cm ortak alan, 10 cm aralik

Iki makro /setup/overlapWidth 10 cm ve /setup/gap 10 cm secer.
Ust merkez X=0, Y=4.3 cm; alt merkez X=-4.3, Y=0 cm.
Ortak izdusum [-5,5] x [-5,5] cm, alan 100 cm2.
Merkezdeki kursun blok 5x5x5 cm; ust/alt aktif yuzeylere 2.5 cm.
Her kosu 100000 olay; bagimsiz yeni tohumlar 14930001/2 ve 14930011/12.
Kullanici istegiyle geri alinan p*flux*cos(theta) kaynak agirligi korunur.
Ilk 1.8 sigma veren eski dosyada ek p/E carpani vardi; kaynak birebir ayni
degildir. Yeni rastgele orneklemde ayni sigma cikmasi da beklenmez.

B1/build icinde:

    bash run_alan_10x10.sh alan_10x10_tekrar_1

Iki kosu tamamlaninca:

    root -l -b -q 'analiz_alan.C("alan_10x10_tekrar_1",10)'

Eski 10x7 analizi ayni komutla calismaya devam eder (varsayilan genislik 7).
Eski veriler ve sonuc klasorleri korunur. Yeni sonuclar bu yeni klasorun
analiz/ altina yazilir; var olan analiz klasoru numarali ekle korunur.
