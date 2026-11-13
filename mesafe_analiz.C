#include <TFile.h>
#include <TH1D.h>
#include <TTree.h>
#include <TLeaf.h>
#include <TLeafC.h>
#include <TEfficiency.h>
#include <TGraphAsymmErrors.h>
#include <TGraph.h>
#include <TAxis.h>
#include <limits>
#include <TCanvas.h>
#include <TLegend.h>
#include <TLine.h>
#include <TSystem.h>
#include <TROOT.h>
#include <TStyle.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Mesafe {
// Diagnostic truth is never used to select the measured coincidence.
struct Diagnostics {
    Long64_t both=0, cBoth=0, cNoBoth=0, bothNoC=0;
    Long64_t entered=0, enteredC=0, enteredTop=0, enteredTopBoth=0;
    Long64_t enteredTopC=0, enteredTopBothC=0, enteredTopCNoBoth=0;
    Long64_t stopped=0, stoppedTop=0, noExit=0, repeated=0;
    Long64_t ray=0, rayC=0, validLoss=0;
    double lossSum=0, lossMax=0;
    std::vector<double> losses;
};

// Intersection of the INITIAL, unscattered forward ray with the nominal box.
// Defined identically with and without the actual block; NOT a measured track.
bool NominalRay(TTree* t,const std::map<std::string,double>& config) {
    const char* pos[]={"PrimaryStartX_cm","PrimaryStartY_cm","PrimaryStartZ_cm"};
    const char* dir[]={"PrimaryStartDirX","PrimaryStartDirY","PrimaryStartDirZ"};
    const double half[]={config.at("BlockWidth_cm")/2,config.at("BlockHeight_cm")/2,config.at("BlockThickness_cm")/2};
    const double center[]={config.at("BlockCenterX_cm"),config.at("BlockCenterY_cm"),config.at("BlockCenterZ_cm")};
    double near=0,far=std::numeric_limits<double>::infinity();
    for(int axis=0;axis<3;++axis) {
        const double x=t->GetLeaf(pos[axis])->GetValue()-center[axis],v=t->GetLeaf(dir[axis])->GetValue();
        if(std::abs(v)<1e-14){if(std::abs(x)>half[axis])return false;continue;}
        double a=(-half[axis]-x)/v,b=(half[axis]-x)/v;
        if(a>b)std::swap(a,b);near=std::max(near,a);far=std::min(far,b);
        if(far<near)return false;
    }
    return far>=near;
}

struct Data {
    Diagnostics diag;
    Long64_t n=0, c=0, top=0, bottom=0;
    std::map<std::string,double> config;
    std::string material, source;
};
void Need(bool ok,const std::string& message) {
    if(!ok) throw std::runtime_error(message);
}
double Value(TTree* t,const char* key) {
    auto* leaf=t->GetLeaf(key);
    Need(leaf!=nullptr,std::string("Eksik alan: ")+key);
    return leaf->GetValue();
}
bool Equal(double a,double b) {
    return std::isfinite(a)&&std::isfinite(b)&&
           std::abs(a-b)<=1e-8*std::max({1.,std::abs(a),std::abs(b)});
}
Data Read(const std::string& path,double gap,int enabled) {
    std::unique_ptr<TFile> f(TFile::Open(path.c_str(),"READ"));
    Need(f&&!f->IsZombie(),"Dosya acilamadi: "+path);
    auto* t=dynamic_cast<TTree*>(f->Get("MuonTelescope"));
    auto* info=dynamic_cast<TTree*>(f->Get("RunInfo"));
    Need(t&&info,"MuonTelescope veya RunInfo yok: "+path);
    Need(t->GetEntries()>0&&info->GetEntries()>0,"Bos veri: "+path);
    Data d;
    const std::vector<std::string> keys={
        "SchemaVersion","BlockWidth_cm","BlockHeight_cm","BlockThickness_cm",
        "Gap_cm","UpperCenterX_cm","UpperCenterY_cm","UpperCenterZ_cm",
        "LowerCenterX_cm","LowerCenterY_cm","LowerCenterZ_cm","LowerRotation_deg",
        "PEThreshold","TimeWindow_ns","SourcePMin_GeV_c","SourcePMax_GeV_c",
        "SourceZenithMax_deg","SourceHalfSize_cm","SourceZ_cm",
        "SourceCenterX_cm","SourceCenterY_cm","OpticalMaxTime_ns","OpticalMaxSteps"};
    // Geant4/g4tools writes string ntuple columns as ROOT /C leaves,
    // not std::string object branches. Read ROOT-managed buffers directly.
    auto* materialLeaf=dynamic_cast<TLeafC*>(info->GetLeaf("BlockMaterial"));
    auto* sourceLeaf=dynamic_cast<TLeafC*>(info->GetLeaf("SourceModel"));
    Need(materialLeaf&&sourceLeaf,"Geant4 metin metaverisi eksik veya tipi yanlis: "+path);
    Long64_t expected=0, expectedC=0, expectedTop=0, expectedBottom=0;
    std::set<std::pair<int,int>> workers;
    for(Long64_t i=0;i<info->GetEntries();++i) {
        Need(info->GetEntry(i)>0,"RunInfo okunamadi");
        Need(workers.insert({int(Value(info,"RunID")),int(Value(info,"WorkerID"))}).second,
             "Tekrarlanan worker/run metaverisi: "+path);
        Need(int(Value(info,"RunID"))==0,"Her dosyada tek ilk run bekleniyor");
        Need(int(Value(info,"BlockEnabled"))==enabled,"Blok durumu yanlis: "+path);
        for(const auto& key:keys) {
            const double v=Value(info,key.c_str());
            if(i==0)d.config[key]=v;
            Need(Equal(d.config.at(key),v),"Dosya icinde ayar degisiyor: "+key);
        }
        // Schema 1 wrote only centered blocks; schema 2 records the center.
        for(const char* name:{"BlockCenterX_cm","BlockCenterY_cm","BlockCenterZ_cm"}) {
            const bool present=info->GetLeaf(name)!=nullptr;
            Need(present||d.config.at("SchemaVersion")==1,"Eksik blok konumu metaverisi");
            const double v=present?Value(info,name):0.;
            if(i==0)d.config[name]=v;
            Need(Equal(d.config.at(name),v),"Dosya icinde blok merkezi degisiyor");
        }
        const std::string material=materialLeaf->GetValueString();
        const std::string source=sourceLeaf->GetValueString();
        Need(!material.empty()&&!source.empty(),"Bos kaynak/malzeme bilgisi");
        if(i==0){d.material=material;d.source=source;}
        Need(d.material==material&&d.source==source,"Tutarsiz metaveri");
        expected+=Long64_t(Value(info,"Events"));
        expectedC+=Long64_t(Value(info,"Coincidences"));
        expectedTop+=Long64_t(Value(info,"TopHits"));
        expectedBottom+=Long64_t(Value(info,"BottomHits"));
    }
    info->ResetBranchAddresses();
    Need(Equal(d.config.at("Gap_cm"),gap),"Dosya mesafesi yanlis: "+path);
    const double thick=d.config.at("BlockThickness_cm");
    const double z=0.5*(gap+1.4);
    Need(Equal(d.config.at("UpperCenterZ_cm"),z)&&
         Equal(d.config.at("LowerCenterZ_cm"),-z),"Sintilator z konumlari yanlis");
    Need(gap>thick,"Blok mesafeye sigmiyor");
    Need(std::abs(d.config.at("BlockCenterZ_cm"))+thick/2<gap/2-.1,"Blok konumu sintilator kaplamasiyla cakisir");
    for(const char* key:{"PrimaryVisitedBoth","PrimaryVisitedTop","PrimaryVisitedBottom",
          "PrimaryEnteredBlock","PrimaryStoppedInBlock","PrimaryBlockEntries","PrimaryBlockExits",
          "PrimaryBlockEntryKinetic_MeV","PrimaryBlockExitKinetic_MeV",
          "PrimaryStartX_cm","PrimaryStartY_cm","PrimaryStartZ_cm",
          "PrimaryStartDirX","PrimaryStartDirY","PrimaryStartDirZ"})
        Need(t->GetLeaf(key)!=nullptr,std::string("Eksik tanilama alani: ")+key);
    std::set<std::pair<int,int>> ids;
    for(Long64_t i=0;i<t->GetEntries();++i) {
        Need(t->GetEntry(i)>0,"Olay okunamadi");
        Need(int(Value(t,"BlockEnabled"))==enabled,"Olay blok durumu yanlis");
        Need(ids.insert({int(Value(t,"RunID")),int(Value(t,"EventID"))}).second,
             "Tekrarlanan olay kimligi: "+path);
        Need(Equal(Value(t,"PEThreshold"),d.config.at("PEThreshold"))&&
             Equal(Value(t,"TimeWindow_ns"),d.config.at("TimeWindow_ns")),
             "Olay esik/zaman ayari uyusmuyor");
        const int c=int(Value(t,"Coincidence"));
        const double t1=Value(t,"FirstPETimePMT1_ns"),t2=Value(t,"FirstPETimePMT2_ns");
        const bool calculated=Value(t,"PEPMT1")>=d.config.at("PEThreshold")&&
            Value(t,"PEPMT2")>=d.config.at("PEThreshold")&&t1>=0&&t2>=0&&
            std::isfinite(t1)&&std::isfinite(t2)&&
            std::abs(t2-t1)<=d.config.at("TimeWindow_ns");
        Need((c==0||c==1)&&calculated==(c==1),"Coincidence kaydi tutarsiz");
        // Diagnostic truth only: do not modify c or the measured totals.
        auto& q=d.diag;
        const bool both=Value(t,"PrimaryVisitedBoth")==1;
        const bool top=Value(t,"PrimaryVisitedTop")==1;
        const bool bottom=Value(t,"PrimaryVisitedBottom")==1;
        const bool entered=Value(t,"PrimaryEnteredBlock")==1;
        const bool stopped=Value(t,"PrimaryStoppedInBlock")==1;
        const int entries=int(Value(t,"PrimaryBlockEntries"));
        const int exits=int(Value(t,"PrimaryBlockExits"));
        Need(both==(top&&bottom),"PrimaryVisitedBoth tutarsiz");
        Need(!stopped||entered,"Blokta durma/giris kaydi tutarsiz");
        Need(enabled||(!entered&&!stopped&&entries==0&&exits==0),"Referansta blok gecisi var");
        q.both+=both;q.cBoth+=(c&&both);q.cNoBoth+=(c&&!both);q.bothNoC+=(both&&!c);
        q.entered+=entered;q.enteredC+=(entered&&c);q.enteredTop+=(entered&&top);
        q.enteredTopBoth+=(entered&&top&&both);q.enteredTopC+=(entered&&top&&c);
        q.enteredTopBothC+=(entered&&top&&both&&c);q.enteredTopCNoBoth+=(entered&&top&&c&&!both);
        q.stopped+=stopped;q.stoppedTop+=(stopped&&top);
        q.noExit+=(entered&&exits==0);q.repeated+=(entries>1||exits>1);
        const bool ray=NominalRay(t,d.config);q.ray+=ray;q.rayC+=(ray&&c);
        const double entryE=Value(t,"PrimaryBlockEntryKinetic_MeV");
        const double exitE=Value(t,"PrimaryBlockExitKinetic_MeV");
        if(entered&&!stopped&&entries==1&&exits==1&&entryE>=0&&exitE>=0&&
           std::isfinite(entryE)&&std::isfinite(exitE)) {
            const double loss=entryE-exitE;
            ++q.validLoss;q.lossSum+=loss;q.lossMax=std::max(q.lossMax,loss);q.losses.push_back(loss);
        }

        ++d.n; d.c+=c; d.top+=int(Value(t,"PMT1Hit"))==1;
        d.bottom+=int(Value(t,"PMT2Hit"))==1;
    }
    Need(d.n==expected&&d.c==expectedC&&d.top==expectedTop&&d.bottom==expectedBottom,
         "RunInfo toplamlarina ulasilmadi; dosya eksik olabilir: "+path);
    return d;
}
void Compatible(const Data& a,const Data& b,bool varyingGap) {
    for(const auto& p:a.config) {
        if(varyingGap&&(p.first=="Gap_cm"||p.first=="UpperCenterZ_cm"||
                       p.first=="LowerCenterZ_cm"))continue;
        Need(Equal(p.second,b.config.at(p.first)),"Karsilastirilan ayar farkli: "+p.first);
    }
    Need(a.material==b.material&&a.source==b.source,"Malzeme/kaynak modeli farkli");
}
std::pair<double,double> CI(const Data& d,double level) {
    return {TEfficiency::ClopperPearson(d.n,d.c,level,false),
            TEfficiency::ClopperPearson(d.n,d.c,level,true)};
}
void Point(TGraphAsymmErrors& g,int i,double x,double y,double lo,double hi) {
    g.SetPoint(i,x,y);g.SetPointError(i,0,0,y-lo,hi-y);
}
}

namespace Mesafe {
struct Comparison {
    double transmission, low, high, familyLow, familyHigh;
    double z, poissonZ, pDeficit, adjustedP;
};
long double LogChoose(Long64_t n,Long64_t k) {
    if(k<0||k>n)return -std::numeric_limits<long double>::infinity();
    return std::lgamma((long double)n+1)-std::lgamma((long double)k+1)
           -std::lgamma((long double)(n-k)+1);
}
// One-sided Fisher exact test: fewer block coincidences, independent runs.
double FisherDeficit(const Data& r,const Data& b) {
    const Long64_t total=r.c+b.c;
    const Long64_t first=std::max(Long64_t(0),total-r.n);
    const long double denominator=LogChoose(r.n+b.n,total);
    long double logSum=-std::numeric_limits<long double>::infinity();
    for(Long64_t k=first;k<=b.c;++k) {
        const long double v=LogChoose(b.n,k)+LogChoose(r.n,total-k)-denominator;
        if(!std::isfinite(logSum))logSum=v;
        else {const long double m=std::max(logSum,v);logSum=m+std::log(std::exp(logSum-m)+std::exp(v-m));}
    }
    return std::min(1.,double(std::exp(logSum)));
}
Comparison Compare(const Data& r,const Data& b,int distances) {
    Need(r.n>0&&b.n>0&&r.c>0,"Referans coincidence sifir veya veri bos");
    const double pr=double(r.c)/r.n,pb=double(b.c)/b.n;
    const auto ri=CI(r,.975),bi=CI(b,.975);
    // Union bound over BOTH runs at EVERY included distance.
    const double level=1.-.05/(2*distances);
    const auto rf=CI(r,level),bf=CI(b,level);
    const double pooled=double(r.c+b.c)/(double(r.n)+b.n);
    const double se=std::sqrt(pooled*(1-pooled)*(1./r.n+1./b.n));
    const double poissonSE=std::sqrt(double(r.c)/(double(r.n)*r.n)+double(b.c)/(double(b.n)*b.n));
    const double p=FisherDeficit(r,b);
    return {pb/pr,bi.first/ri.second,bi.second/ri.first,
            bf.first/rf.second,bf.second/rf.first,
            se>0?(pr-pb)/se:0,poissonSE>0?(pr-pb)/poissonSE:0,
            p,std::min(1.,p*distances)};
}
void SetRange(TGraphAsymmErrors& g,double include,double minWidth) {
    double lo=include,hi=include;
    for(int i=0;i<g.GetN();++i) {
        lo=std::min(lo,g.GetY()[i]-g.GetErrorYlow(i));
        hi=std::max(hi,g.GetY()[i]+g.GetErrorYhigh(i));
    }
    const double margin=.15*std::max(minWidth,hi-lo);
    g.SetMinimum(lo-margin);g.SetMaximum(hi+margin);
}
void Save(TCanvas& c,TFile& f,const std::string& out,const char* name) {
    c.Modified();c.Update();
    c.SaveAs((out+"/"+name+".png").c_str());
    c.SaveAs((out+"/"+name+".pdf").c_str());
    f.cd();c.Write(name);
}
}

namespace Mesafe {
void DiagnosticRate(TGraphAsymmErrors& g,double gap,Long64_t k,Long64_t n) {
    if(n==0)return; // Undefined conditional probability: omit, never draw zero.
    const double p=double(k)/n;
    Point(g,g.GetN(),gap,100*p,
          100*TEfficiency::ClopperPearson(n,k,.95,false),
          100*TEfficiency::ClopperPearson(n,k,.95,true));
}
void DiagnosticDelta(TGraphAsymmErrors& g,double gap,Long64_t kr,Long64_t nr,
                     Long64_t kb,Long64_t nb,int sign=1) {
    const double delta=double(kr)/nr-double(kb)/nb;
    const double lo=TEfficiency::ClopperPearson(nr,kr,.975,false)-TEfficiency::ClopperPearson(nb,kb,.975,true);
    const double hi=TEfficiency::ClopperPearson(nr,kr,.975,true)-TEfficiency::ClopperPearson(nb,kb,.975,false);
    Point(g,g.GetN(),gap,100*sign*delta,100*(sign>0?lo:-hi),100*(sign>0?hi:-lo));
}
void DiagnosticPlot(std::vector<TGraphAsymmErrors*> graphs,const std::vector<const char*>& labels,
                    const char* title,const char* name,const std::vector<double>& gaps,
                    TFile& root,const std::string& out,bool difference=false) {
    TCanvas canvas((std::string(name)+"_canvas").c_str(),title,1050,760);
    canvas.SetLeftMargin(.14);canvas.SetTopMargin(.25);canvas.SetGrid();
    TGraphAsymmErrors* first=nullptr;double lo=0,hi=difference?0:1;
    for(auto* g:graphs)for(int i=0;i<g->GetN();++i) {
        lo=std::min(lo,g->GetY()[i]-g->GetErrorYlow(i));
        hi=std::max(hi,g->GetY()[i]+g->GetErrorYhigh(i));
    }
    const int colors[]={kBlue+1,kRed+1,kGreen+2,kMagenta+1};
    TLegend legend(.15,.77,.88,.93);legend.SetTextSize(.025);legend.SetFillStyle(0);legend.SetBorderSize(0);
    for(size_t i=0;i<graphs.size();++i) {
        auto* g=graphs[i];if(!g->GetN())continue;
        g->SetMarkerStyle(20+i);g->SetMarkerColor(colors[i%4]);g->SetLineColor(colors[i%4]);g->SetMarkerSize(1.2);
        if(!first) {
            first=g;g->SetTitle(title);
            const double span=std::max(hi-lo,.1);
            g->SetMinimum(difference?lo-.15*span:0);
            g->SetMaximum(difference?hi+.15*span:std::min(105.,hi+.1*span));
            g->Draw("AP");g->GetXaxis()->SetLimits(gaps.front()-1,gaps.back()+1);
        } else g->Draw("P SAME");
        legend.AddEntry(g,labels[i],"pe");
    }
    if(!first)return;
    legend.Draw();TLine zero(gaps.front()-1,0,gaps.back()+1,0);
    if(difference){zero.SetLineStyle(2);zero.Draw();}
    Save(canvas,root,out,name);
    root.cd();for(auto* g:graphs)g->Write();
}
void RenderDiagnostics(const std::vector<Data>& refs,const std::vector<Data>& blocks,
                       const std::vector<double>& gaps,TFile& root,const std::string& out) {
    std::ofstream csv(out+"/diagnostic_counts.csv"),txt(out+"/diagnostic_summary.txt"),
                  components(out+"/diagnostic_decomposition.csv");
    Need(csv.good()&&txt.good()&&components.good(),"Tanilama ozetleri acilamadi");
    csv<<"gap_cm,condition,events,coincidence,primary_both,C_and_both,C_without_both,both_without_C,entered,entered_and_C,entered_and_top,entered_top_and_both,entered_top_and_C,entered_top_both_and_C,entered_top_C_without_both,stopped_in_block,stopped_and_top,entered_without_exit,multiple_entries_or_exits,nominal_initial_ray_intersects_box,ray_and_C,valid_single_entry_exit_energy_pairs,mean_first_entry_exit_loss_MeV\n";
    components<<"gap_cm,total_deficit_probability,primary_both_deficit,missed_PMT_correction,C_without_primary_both_correction,identity_residual\n";
    csv<<std::setprecision(12);components<<std::setprecision(12);txt<<std::fixed<<std::setprecision(6);
    TGraphAsymmErrors enteredFraction,detectedEntered,bothAfterEntry,stoppedAfterEntry,
        measuredAfterEntry,effRef,effBlock,rayRef,rayBlock,outsideRef,outsideBlock,
        primaryDelta,missedCorrection,otherCorrection,totalDelta;
    enteredFraction.SetName("truth_entered_per_generated");detectedEntered.SetName("truth_entered_given_C");
    bothAfterEntry.SetName("truth_both_given_entered_and_top");stoppedAfterEntry.SetName("truth_stopped_given_entered_and_top");
    measuredAfterEntry.SetName("truth_C_given_entered_and_top");
    effRef.SetName("truth_C_given_both_reference");effBlock.SetName("truth_C_given_both_block");
    rayRef.SetName("truth_C_given_initial_ray_box_reference");rayBlock.SetName("truth_C_given_initial_ray_box_block");
    outsideRef.SetName("truth_C_given_initial_ray_misses_box_reference");outsideBlock.SetName("truth_C_given_initial_ray_misses_box_block");
    primaryDelta.SetName("truth_primary_both_deficit");missedCorrection.SetName("truth_missed_PMT_correction");
    otherCorrection.SetName("truth_C_without_both_correction");totalDelta.SetName("measured_total_deficit_probability");
    for(size_t i=0;i<gaps.size();++i) {
        const auto& r=refs[i];const auto& b=blocks[i];const auto& a=r.diag;const auto& q=b.diag;
        for(int which=0;which<2;++which) {
            const auto& d=which?b:r;const auto& v=d.diag;
            Need(d.c==v.cBoth+v.cNoBoth&&v.both==v.cBoth+v.bothNoC,"Tanilama sayim ozdesligi tutmuyor");
            csv<<gaps[i]<<','<<(which?"block":"reference")<<','<<d.n<<','<<d.c<<','<<v.both<<','
               <<v.cBoth<<','<<v.cNoBoth<<','<<v.bothNoC<<','<<v.entered<<','<<v.enteredC<<','
               <<v.enteredTop<<','<<v.enteredTopBoth<<','<<v.enteredTopC<<','<<v.enteredTopBothC<<','
               <<v.enteredTopCNoBoth<<','<<v.stopped<<','<<v.stoppedTop<<','<<v.noExit<<','<<v.repeated<<','
               <<v.ray<<','<<v.rayC<<','<<v.validLoss<<',';
            if(v.validLoss)csv<<v.lossSum/v.validLoss;else csv<<"nan";csv<<'\n';
        }
        DiagnosticRate(enteredFraction,gaps[i],q.entered,b.n);
        DiagnosticRate(detectedEntered,gaps[i],q.enteredC,b.c);
        DiagnosticRate(bothAfterEntry,gaps[i],q.enteredTopBoth,q.enteredTop);
        DiagnosticRate(stoppedAfterEntry,gaps[i],q.stoppedTop,q.enteredTop);
        DiagnosticRate(measuredAfterEntry,gaps[i],q.enteredTopC,q.enteredTop);
        DiagnosticRate(effRef,gaps[i],a.cBoth,a.both);DiagnosticRate(effBlock,gaps[i],q.cBoth,q.both);
        DiagnosticRate(rayRef,gaps[i],a.rayC,a.ray);DiagnosticRate(rayBlock,gaps[i],q.rayC,q.ray);
        DiagnosticRate(outsideRef,gaps[i],r.c-a.rayC,r.n-a.ray);
        DiagnosticRate(outsideBlock,gaps[i],b.c-q.rayC,b.n-q.ray);
        DiagnosticDelta(primaryDelta,gaps[i],a.both,r.n,q.both,b.n);
        DiagnosticDelta(missedCorrection,gaps[i],a.bothNoC,r.n,q.bothNoC,b.n,-1);
        DiagnosticDelta(otherCorrection,gaps[i],a.cNoBoth,r.n,q.cNoBoth,b.n);
        DiagnosticDelta(totalDelta,gaps[i],r.c,r.n,b.c,b.n);
        const double total=double(r.c)/r.n-double(b.c)/b.n;
        const double primary=double(a.both)/r.n-double(q.both)/b.n;
        const double missed=-(double(a.bothNoC)/r.n-double(q.bothNoC)/b.n);
        const double other=double(a.cNoBoth)/r.n-double(q.cNoBoth)/b.n;
        Need(std::abs(total-primary-missed-other)<1e-12,"Sayim farki ayrisimi tutmuyor");
        components<<gaps[i]<<','<<total<<','<<primary<<','<<missed<<','<<other<<','<<total-primary-missed-other<<'\n';
        txt<<"GAP "<<gaps[i]<<" cm\n"
           <<"Block: entered="<<q.entered<<" / "<<b.n<<"; entered among measured C="<<q.enteredC<<" / "<<b.c<<"\n"
           <<"Block: primary entered AND visited top="<<q.enteredTop<<"; of these visited both="<<q.enteredTopBoth
           <<", did not visit bottom="<<q.enteredTop-q.enteredTopBoth<<", measured C="<<q.enteredTopC
           <<", no measured C="<<q.enteredTop-q.enteredTopC<<"\n"
           <<"Block: primary stopped="<<q.stopped<<"; stopped AND visited top="<<q.stoppedTop
           <<"; entered without recorded exit="<<q.noExit<<"; repeated entry/exit="<<q.repeated<<"\n"
           <<"Reference C without primary-both="<<a.cNoBoth<<"; block C without primary-both="<<q.cNoBoth<<"\n"
           <<"Reference primary-both without C="<<a.bothNoC<<"; block primary-both without C="<<q.bothNoC<<"\n"
           <<"Initial nominal box rays: reference="<<a.ray<<" (C="<<a.rayC<<"); block="<<q.ray<<" (C="<<q.rayC<<")\n"
           <<"Primary single-entry/exit energy pairs="<<q.validLoss;
        if(q.validLoss)txt<<"; mean kinetic loss="<<q.lossSum/q.validLoss<<" MeV; maximum="<<q.lossMax<<" MeV";
        txt<<"\n\n";
        const std::string name="diagnostic_energy_loss_"+std::to_string(int(gaps[i]))+"cm";
        TH1D hist(name.c_str(),"SIMULATION: single entry/exit primary kinetic loss;Entry kinetic - exit kinetic (MeV);Primary muons",100,0,1000);
        hist.SetDirectory(nullptr);
        for(double loss:q.losses)hist.Fill(loss);
        root.cd();hist.Write();
        if(q.validLoss) {
            TCanvas c((name+"_canvas").c_str(),"Energy loss",1050,760);c.SetLeftMargin(.13);c.SetGrid();
            hist.Draw("HIST");Save(c,root,out,name.c_str());
        }
        txt<<"Energy histogram "<<gaps[i]<<" cm: underflow="<<hist.GetBinContent(0)
           <<"; overflow >=1000 MeV="<<hist.GetBinContent(hist.GetNbinsX()+1)<<" (included in mean).\n\n";
    }
    DiagnosticPlot({&enteredFraction,&detectedEntered},{"Primary entered / all generated","Primary entered / measured coincidences"},
        "SIMULATION: block intersection fractions;Scintillator face gap (cm);Fraction (%)","diagnostic_block_coverage",gaps,root,out);
    DiagnosticPlot({&bothAfterEntry,&measuredAfterEntry,&stoppedAfterEntry},
        {"Visited both / entered and visited top","Measured C / entered and visited top","Stopped in block / entered and visited top"},
        "SIMULATION: outcomes after block entry;Scintillator face gap (cm);Conditional fraction (%)","diagnostic_entry_outcomes",gaps,root,out);
    DiagnosticPlot({&effRef,&effBlock},{"Reference: C / primary visited both","Block: C / primary visited both"},
        "SIMULATION: PMT acceptance conditional on primary visits;Scintillator face gap (cm);Conditional fraction (%)","diagnostic_detection",gaps,root,out);
    DiagnosticPlot({&rayRef,&rayBlock},{"Reference: C / nominal initial box ray","Block: C / nominal initial box ray"},
        "SIMULATION: initial straight-ray box subset;Scintillator face gap (cm);Coincidence fraction (%)","diagnostic_initial_ray",gaps,root,out);
    DiagnosticPlot({&outsideRef,&outsideBlock},{"Reference: C / initial ray misses box","Block: C / initial ray misses box"},
        "SIMULATION: initial straight-ray control subset;Scintillator face gap (cm);Coincidence fraction (%)","diagnostic_initial_ray_control",gaps,root,out);
    DiagnosticPlot({&totalDelta,&primaryDelta,&missedCorrection,&otherCorrection},
        {"Measured total deficit","Primary-both deficit","Missed-PMT correction","C without primary-both correction"},
        "SIMULATION: algebraic count-difference decomposition;Scintillator face gap (cm);Probability difference (percentage points)",
        "diagnostic_count_decomposition",gaps,root,out,true);
    txt<<"INTERPRETATION LIMITS\n"
       <<"Diagnostics use simulation truth; measured coincidence graphs are unchanged. Conditional intervals are descriptive per-point binomial 95%.\n"
       <<"Entry+top is an ANY-order condition, not a verified top -> block -> bottom trajectory. A visit is not a full traversal.\n"
       <<"No block exit is not by itself proof of stopping. Stopped means recorded kinetic energy reached zero in block.\n"
       <<"Failure to visit bottom includes geometry, scattering, stopping, decay and other transport effects; it is not a measured scattering angle.\n"
       <<"No entry/exit directions were saved: exact scattering-angle distributions cannot be recovered from these ROOT files.\n"
       <<"C without primary-both is a flag for inspection; it is NOT proof of a false coincidence or a uniquely secondary-caused event.\n"
       <<"Initial ray subsets use a hypothetical unscattered ray and nominal box at its recorded center, identically in both runs; not actual intersections or measured tracks.\n"
       <<"First entry/exit kinetic loss is plotted only for exactly one entry and one exit and non-stopped primaries. It is not local dE/dx or scintillator energy deposit.\n"
       <<"Count decomposition is an exact algebraic identity, NOT a causal allocation. Components share events and are correlated.\n"
       <<"Independent reference/block runs do not provide a causal event-by-event lost/gained list. No such pairing has been attempted.\n";
    txt.close();csv.close();components.close();
    std::ifstream summary(out+"/diagnostic_summary.txt");std::cout<<summary.rdbuf();
}
}

// Optional folders add 8 and/or 11 cm WITHOUT copying previous ROOT data.
void mesafe_analiz(const char* folder="mesafe_100k_1", const char* extra8="",
                   const char* extra11="", const char* outputFolder="", const char* gapList="") {
    using namespace Mesafe;
    try {
        const std::string dir=folder;
        std::map<int,std::string> inputs={{10,dir},{15,dir},{20,dir}};
        if(std::string(extra8).size())inputs[8]=extra8;
        if(std::string(extra11).size())inputs[11]=extra11;
        if(std::string(gapList).size()) {
            Need(std::string(extra8).empty()&&std::string(extra11).empty(),"Mesafe listesi ile ek klasorleri birlikte kullanmayin");
            inputs.clear();std::istringstream list(gapList);int gap=0;
            while(list>>gap) {
                Need(gap>=6&&gap<=100,"Mesafe listesi 6-100 cm tam sayilar icermeli");
                Need(inputs.emplace(gap,dir).second,"Mesafe listesinde tekrar var");
            }
            Need(list.eof()&&!inputs.empty(),"Gecersiz mesafe listesi");
        }
        std::vector<double> gaps;
        std::vector<Data> refs,blocks;
        std::vector<Comparison> metrics;
        // Validate all actual inputs BEFORE opening any analysis output.
        for(const auto& item:inputs) {
            const int gap=item.first;const std::string tag=std::to_string(gap)+"cm.root";
            std::cout<<"Reading "<<gap<<" cm..."<<std::endl;
            gaps.push_back(gap);
            refs.push_back(Read(item.second+"/reference_"+tag,gap,0));
            blocks.push_back(Read(item.second+"/block_"+tag,gap,1));
            Compatible(refs.back(),blocks.back(),false);
            Compatible(refs.front(),refs.back(),true);
            metrics.push_back(Compare(refs.back(),blocks.back(),inputs.size()));
        }
        std::string suffix="/analiz_ayri";
        for(const auto& item:inputs)suffix+="_"+std::to_string(item.first);
        const std::string requested=std::string(outputFolder).empty()?dir+suffix:outputFolder;
        std::string out=requested;
        for(int k=2;!gSystem->AccessPathName(out.c_str());++k)out=requested+"_"+std::to_string(k);
        Need(gSystem->mkdir(out.c_str(),true)==0,"Analiz klasoru olusturulamadi");
        std::ofstream csv(out+"/mesafe_ozet.csv"),txt(out+"/ozet.txt");
        Need(csv.good()&&txt.good(),"Ozet dosyalari acilamadi");
        TGraphAsymmErrors reference,block,transmission,family,deficit;
        TGraph sigma;
        reference.SetName("reference_efficiency");block.SetName("block_efficiency");
        transmission.SetName("transmission");family.SetName("transmission_family95");
        deficit.SetName("deficit_percent");sigma.SetName("signed_score_Z");
        csv<<std::setprecision(12)
           <<"gap_cm,block_top_active_clearance_cm,block_bottom_active_clearance_cm,reference_events,reference_top,reference_bottom,reference_coincidence,block_events,block_top,block_bottom,block_coincidence,reference_efficiency,block_efficiency,transmission,transmission_low95,transmission_high95,transmission_family_low95,transmission_family_high95,deficit_percent,deficit_low95,deficit_high95,signed_score_Z,approx_poisson_Z,p_deficit_fisher_one_sided,p_deficit_bonferroni\n";
        txt<<std::fixed<<std::setprecision(6);
        for(int i=0;i<int(gaps.size());++i) {
            const auto& r=refs[i];const auto& b=blocks[i];const auto& m=metrics[i];
            const double pr=double(r.c)/r.n,pb=double(b.c)/b.n;
            const auto rc=CI(r,.95),bc=CI(b,.95);
            const double centerZ=r.config.at("BlockCenterZ_cm");
            const double clear=(gaps[i]-r.config.at("BlockThickness_cm"))/2-centerZ;
            const double bottomClear=(gaps[i]-r.config.at("BlockThickness_cm"))/2+centerZ;
            Point(reference,i,gaps[i],100*pr,100*rc.first,100*rc.second);
            Point(block,i,gaps[i],100*pb,100*bc.first,100*bc.second);
            Point(transmission,i,gaps[i],m.transmission,m.low,m.high);
            Point(family,i,gaps[i],m.transmission,m.familyLow,m.familyHigh);
            Point(deficit,i,gaps[i],100*(1-m.transmission),100*(1-m.high),100*(1-m.low));
            sigma.SetPoint(i,gaps[i],m.z);
            csv<<gaps[i]<<','<<clear<<','<<bottomClear<<','<<r.n<<','<<r.top<<','<<r.bottom<<','<<r.c<<','
               <<b.n<<','<<b.top<<','<<b.bottom<<','<<b.c<<','<<pr<<','<<pb<<','
               <<m.transmission<<','<<m.low<<','<<m.high<<','<<m.familyLow<<','<<m.familyHigh<<','
               <<100*(1-m.transmission)<<','<<100*(1-m.high)<<','<<100*(1-m.low)<<','
               <<m.z<<','<<m.poissonZ<<','<<m.pDeficit<<','<<m.adjustedP<<'\n';
            txt<<"Gap "<<gaps[i]<<" cm; block center z="<<centerZ<<" cm; upper active-face clearance="<<clear<<" cm; lower clearance="<<bottomClear<<" cm.\n"
               <<"Reference: "<<r.c<<"/"<<r.n<<"; block: "<<b.c<<"/"<<b.n
               <<"; T="<<m.transmission<<" ["<<m.low<<", "<<m.high<<"]\n"
               <<"Signed score Z="<<m.z<<"; approximate Poisson Z="<<m.poissonZ
               <<"; exact one-sided deficit p="<<m.pDeficit
               <<"; Bonferroni p="<<m.adjustedP<<"\n"
               <<"Input: "<<inputs.at(int(gaps[i]))<<"\n\n";
        }
        txt<<"Independent runs; probabilities per generated muon, NOT Hz or minutes.\n"
           <<"Efficiency bars: exact binomial 95%. Transmission/deficit bars: conservative per-distance 95% envelopes.\n"
           <<"transmission_family95: conservative simultaneous 95% envelopes for a fixed set of included distances.\n"
           <<"Z=(p_ref-p_block)/sqrt(p_pool*(1-p_pool)*(1/N_ref+1/N_block)); approximate null score statistic.\n"
           <<"Positive Z: fewer block coincidences; negative Z: more block coincidences. Z is NOT a probability or an error bar.\n"
           <<"Fisher p uses the one-sided alternative p_block<p_ref. Z is not obtained from that exact p.\n"
           <<"Multiple-distance p adjustment: Bonferroni over included distances. Adaptive selection of new distances is not fully accounted for.\n"
           <<"No interpolation, peak fit, or automatic optimal-distance selection is performed.\n"
           <<"No primary truth filter. Missing coincidence does not prove absorption.\n"
           <<"Source z=60 cm; momentum range 1-100 GeV/c; finite source plane retained.\n"
           <<"Material: "<<refs[0].material<<"; block dimensions cm: "
           <<refs[0].config.at("BlockWidth_cm")<<" x "<<refs[0].config.at("BlockHeight_cm")
           <<" x "<<refs[0].config.at("BlockThickness_cm")<<"\n";
        txt.close();csv.close();
        gROOT->SetBatch(true);gStyle->SetOptStat(0);
        TFile result((out+"/mesafe_analiz.root").c_str(),"CREATE");
        Need(!result.IsZombie(),"Analiz ROOT dosyasi yazilamadi");
        reference.SetMarkerStyle(20);reference.SetMarkerColor(kBlue+1);reference.SetLineColor(kBlue+1);
        block.SetMarkerStyle(24);block.SetMarkerColor(kRed+1);block.SetLineColor(kRed+1);
        for(auto* g:{&transmission,&family,&deficit}){g->SetMarkerStyle(20);g->SetMarkerSize(1.3);}
        sigma.SetMarkerStyle(20);sigma.SetMarkerSize(1.4);sigma.SetMarkerColor(kBlue+1);
        const double xmin=gaps.front()-1,xmax=gaps.back()+1;
        {
            TCanvas c("probability_canvas","Coincidence",950,700);c.SetLeftMargin(.14);c.SetGrid();
            double ymax=0;
            for(int i=0;i<reference.GetN();++i){
                ymax=std::max(ymax,reference.GetY()[i]+reference.GetErrorYhigh(i));
                ymax=std::max(ymax,block.GetY()[i]+block.GetErrorYhigh(i));
            }
            reference.SetTitle("Coincidence probability;Scintillator face gap (cm);Coincidences / generated muons (%)");
            reference.SetMinimum(0);reference.SetMaximum(1.25*ymax);
            reference.Draw("AP");reference.GetXaxis()->SetLimits(xmin,xmax);block.Draw("P SAME");
            TLegend legend(.64,.74,.89,.88);legend.AddEntry(&reference,"No block","pe");
            legend.AddEntry(&block,"Lead block","pe");legend.Draw();Save(c,result,out,"coincidence");
        }
        for(int j=0;j<2;++j) {
            auto& g=j?family:transmission;
            TCanvas c(j?"transmission_family_canvas":"transmission_canvas","Transmission",950,700);
            c.SetLeftMargin(.14);c.SetGrid();
            g.SetTitle(j?"Transmission: fixed-family simultaneous 95% envelope;Scintillator face gap (cm);Block / reference probability"
                        :"Transmission: per-distance 95% envelope;Scintillator face gap (cm);Block / reference probability");
            SetRange(g,1,.1);g.Draw("AP");g.GetXaxis()->SetLimits(xmin,xmax);
            TLine line(xmin,1,xmax,1);line.SetLineStyle(2);line.SetLineWidth(2);line.Draw();
            Save(c,result,out,j?"transmission_family95":"transmission");
        }
        {
            TCanvas c("deficit_canvas","Reduction",950,700);c.SetLeftMargin(.14);c.SetGrid();
            deficit.SetTitle("Coincidence reduction: per-distance 95% envelope;Scintillator face gap (cm);100 (1 - transmission) (%)");
            SetRange(deficit,0,5);deficit.Draw("AP");deficit.GetXaxis()->SetLimits(xmin,xmax);
            TLine line(xmin,0,xmax,0);line.SetLineStyle(2);line.Draw();Save(c,result,out,"azalma");
        }
        {
            TCanvas c("sigma_canvas","Signed Z",950,700);c.SetLeftMargin(.14);c.SetGrid();
            double extent=3.5;
            for(int i=0;i<sigma.GetN();++i)extent=std::max(extent,std::abs(sigma.GetY()[i])+.5);
            sigma.SetTitle("Signed score Z: local, approximate;Scintillator face gap (cm);Z (positive = coincidence deficit)");
            sigma.SetMinimum(-extent);sigma.SetMaximum(extent);sigma.Draw("AP");sigma.GetXaxis()->SetLimits(xmin,xmax);
            std::vector<std::unique_ptr<TLine>> lines;
            for(double y:{-3.,-2.,0.,2.,3.}) {
                auto l=std::unique_ptr<TLine>(new TLine(xmin,y,xmax,y));
                l->SetLineStyle(y==0?1:2);l->SetLineColor(y==0?kBlack:kGray+1);l->Draw();lines.push_back(std::move(l));
            }
            Save(c,result,out,"sigma");
        }
        result.cd();reference.Write();block.Write();transmission.Write();family.Write();deficit.Write();sigma.Write();
        RenderDiagnostics(refs,blocks,gaps,result,out);
        result.Close();
        std::ifstream report(out+"/ozet.txt");std::cout<<report.rdbuf();
        std::cout<<"\nSonuclar: "<<out<<"\n";
    } catch(const std::exception& e) {
        std::cerr<<"ANALIZ HATASI: "<<e.what()<<'\n';
    }
}
