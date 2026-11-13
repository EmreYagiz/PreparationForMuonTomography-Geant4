#include "mesafe_analiz.C"

// A single geometry's independent reference/block pair. No old-run mixing.
void analiz_alan(const char* folder="alan_10x7_1", double overlapWidth=7.) {
    using namespace Mesafe;
    try {
        const std::string dir=folder;
        Need(std::isfinite(overlapWidth)&&overlapWidth>0&&overlapWidth<=10,"Gecersiz ortak alan genisligi");
        const double shift=(10-overlapWidth)/2,area=10*overlapWidth;
        std::ostringstream label;label<<"10x"<<overlapWidth<<" cm overlap";
        auto r=Read(dir+"/reference.root",10,0);
        auto b=Read(dir+"/block.root",10,1);
        Compatible(r,b,false);
        Need(Equal(r.config.at("UpperCenterX_cm"),shift)&&
             Equal(r.config.at("LowerCenterX_cm"),-4.3-shift),"Ortak alan konumlari beklenenden farkli");
        const auto m=Compare(r,b,1);
        std::string out=dir+"/analiz";
        for(int n=2;!gSystem->AccessPathName(out.c_str());++n)out=dir+"/analiz_"+std::to_string(n);
        Need(gSystem->mkdir(out.c_str(),true)==0,"Analiz klasoru olusturulamadi");
        std::ofstream report(out+"/ozet.txt");report<<std::setprecision(8)
            <<"Projected overlap: "<<overlapWidth<<" cm X x 10 cm Y = "<<area<<" cm2; centered at (0,0).\n"
            <<"Gap: 10 cm; centered Pb block: 5x5x5 cm. Face clearance: 2.5 cm each side.\n"
            <<"Source: "<<r.source<<"\n"
            <<"Reference coincidence: "<<r.c<<" / "<<r.n<<"\n"
            <<"Block coincidence: "<<b.c<<" / "<<b.n<<"\n"
            <<"Transmission: "<<m.transmission<<" ["<<m.low<<", "<<m.high<<"]\n"
            <<"Reduction percent: "<<100*(1-m.transmission)<<"\n"
            <<"Signed approximate score Z: "<<m.z<<"\n"
            <<"One-sided exact Fisher deficit p: "<<m.pDeficit<<"\n"
            <<"No peak or optimal-geometry conclusion from a single pair.\n"
            <<"Earlier distance runs used a different source weighting. Do not attribute differences to overlap alone.\n";
        report.close();gROOT->SetBatch(true);gStyle->SetOptStat(0);
        TFile result((out+"/alan_analiz.root").c_str(),"CREATE");Need(!result.IsZombie(),"ROOT cikti hatasi");
        TGraphAsymmErrors t;t.SetName("transmission");t.SetMarkerStyle(20);
        Point(t,0,area,m.transmission,m.low,m.high);
        {
            TCanvas c("overlap_transmission","Transmission",950,700);c.SetLeftMargin(.14);c.SetGrid();
            t.SetTitle((label.str()+", 10 cm gap;Projected common area (cm^{2});Block / reference probability").c_str());
            SetRange(t,1,.1);t.Draw("AP");t.GetXaxis()->SetLimits(area-10,area+10);
            TLine line(area-10,1,area+10,1);line.SetLineStyle(2);line.Draw();Save(c,result,out,"transmission");
        }
        TGraph z;z.SetName("signed_score_Z");z.SetPoint(0,area,m.z);z.SetMarkerStyle(20);
        {
            TCanvas c("overlap_sigma","Signed Z",950,700);c.SetLeftMargin(.14);c.SetGrid();
            z.SetTitle((label.str()+": local approximate Z;Projected common area (cm^{2});Signed Z (positive = deficit)").c_str());
            const double extent=std::max(3.5,std::abs(m.z)+.5);z.SetMinimum(-extent);z.SetMaximum(extent);
            z.Draw("AP");z.GetXaxis()->SetLimits(area-10,area+10);
            TLine line(area-10,0,area+10,0);line.Draw();Save(c,result,out,"sigma");
        }
        result.cd();t.Write();z.Write();RenderDiagnostics({r},{b},{10},result,out);result.Close();
        std::ifstream text(out+"/ozet.txt");std::cout<<text.rdbuf()<<"\nSonuclar: "<<out<<'\n';
    } catch(const std::exception& e) {std::cerr<<"ANALIZ HATASI: "<<e.what()<<'\n';}
}
