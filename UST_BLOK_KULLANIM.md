# Genis blok, ust sintilatorun hemen altinda

Gap 30 cm, ortak izdusum 10x10 cm. Kursun blok 8x8x5 cm, merkezi
(0,0,11.5) cm. Blok Z araligi [9,14] cm. Ust aktif yuzey Z=15,
Teflon alt yuzeyi Z=14.9 cm: bosluklar 1.0 ve 0.9 cm. Blok alti ile
alt aktif yuzey arasinda 24 cm var. Ust/alt merkezleri +/-15.7 cm.
Blok XY izi ortak alana sigar. Sintilator, PMT, optik ve kaynak ayarlari
korunur; kaynak agirligi eski p*flux*cos(theta), momentum 1-100 GeV/c.

Her kosu 500000 olay, toplam 1000000. Daha fazla olay fiziksel etkiyi
buyutmez; istatistiksel belirsizligi azaltir. %5-6 azalma garanti edilmez.
Iki kosu sirayla, 8 worker ile calisir. Kullanici B1/build icinden baslatir:

    bash run_ust_blok.sh Pb8_ust_30cm_1

Bitince:

    root -l -b -q 'ust_blok_analiz.C("Pb8_ust_30cm_1")'

Ciktilar Pb8_ust_30cm_1/analiz_ayri_30/ altina, varsa numarali yeni klasore.
Eski veriler korunur. Yeni, ayni geometri metaverisine sahip bos referans
alinir; eski merkezdeki 5 cm blok referansiyla karistirilmaz.

/setup/blockZ varsayilan 0 cm oldugu icin eski makrolar merkezi konumlarini
korur. RunInfo SchemaVersion=2, BlockCenterX/Y/Z_cm eklenmistir.
Guncel analiz eski schema 1 dosyalarinda merkezi (0,0,0) kabul eder;
yeni dosyalarda kaydedilen merkezi okur ve nominal isin hesabinda kullanir.
Blok-ust ve blok-alt aktif yuzey acikliklari CSV/txt'de ayri yazilir.
Tanilama truth alanlari deneyde olculen coincidence secimine eklenmez.
