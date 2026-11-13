#include "mesafe_analiz.C"
void ust20_blok_analiz(const char* folder="Pb8_ust_20cm_1") {
    try {
        for(const char* name:{"reference","block"}) {
            const std::string path=std::string(folder)+"/"+name+"_20cm.root";
            TFile f(path.c_str(),"READ");Mesafe::Need(!f.IsZombie(),"Dosya acilamadi: "+path);
            auto* info=dynamic_cast<TTree*>(f.Get("RunInfo"));
            Mesafe::Need(info&&info->GetEntry(0)>0,"RunInfo okunamadi");
            for(auto p:std::vector<std::pair<const char*,double>>{
                    {"Gap_cm",20},{"BlockWidth_cm",8},{"BlockHeight_cm",8},
                    {"BlockThickness_cm",5},{"BlockCenterX_cm",0},
                    {"BlockCenterY_cm",0},{"BlockCenterZ_cm",6.5},{"OverlapWidth_cm",10}})
                Mesafe::Need(Mesafe::Equal(Mesafe::Value(info,p.first),p.second),std::string("Beklenmeyen ayar: ")+p.first);
        }
        mesafe_analiz(folder,"","","","20");
    } catch(const std::exception& e) {std::cerr<<"ANALIZ HATASI: "<<e.what()<<'\n';}
}
