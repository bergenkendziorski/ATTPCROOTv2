// Include ROOT headers
#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>

void plot_com_angle() {
    // Step 1: Open the ROOT file
    TFile* file = TFile::Open("/mnt/analysis/e12014/bergen/ATTPCROOTv2/simCode/data/digi_Bi200/output_digi00.root"); //alter this line with desired root file
    if (!file || file->IsZombie()) {
        std::cerr << "Error opening file" << std::endl;
        return;
    }

    // Step 2: Retrieve the cbmsim tree
    TTree* cbmsim = (TTree*)file->Get("cbmsim");
    if (!cbmsim) {
        std::cerr << "Error: cbmsim tree not found" << std::endl;
        return;
    }

    // Step 3: Draw the histogram using TTree::Draw
    TCanvas* canvas = new TCanvas("canvas", "Center of Mass Angle", 800, 600);
    //cbmsim->Draw("SimInfo.ang>>angHist(100, 0, 180)", "", "hist");
    //auto ang = simInfo->GetBinContent(simInfo->GetXaxis()->FindBin("ang"))

    // Optionally retrieve the histogram for further customization
    TH1F* angHist = (TH1F*)gDirectory->Get("angHist");
    if (angHist) {
        angHist->SetTitle("Center of Mass Angle between Fragments");
        angHist->GetXaxis()->SetTitle("Angle (degrees)");
        angHist->GetYaxis()->SetTitle("Counts");
    }

    // update canvas
    canvas->Update();
}
