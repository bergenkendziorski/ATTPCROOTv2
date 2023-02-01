#include "AtTabEnergyLoss.h"

#include "AtCSVReader.h"
#include "AtDataManip.h"
#include "AtEvent.h"
#include "AtMap.h"
#include "AtPadArray.h"
#include "AtPatternEvent.h"
#include "AtPatternLine.h"
#include "AtRawEvent.h"
#include "AtTrack.h"
#include "AtViewerManager.h"

#include <TCanvas.h>
#include <TF1.h>
#include <TH1.h>
#include <THStack.h>

using XYZVector = ROOT::Math::XYZVector;
using XYZPoint = ROOT::Math::XYZPoint;

void AtTabEnergyLoss::SetStyle(std::array<TH1Ptr, 2> &hists, THStack &stack)
{
   for (int i = 0; i < hists.size(); ++i) {
      hists[i]->SetLineColor(fHistColors[i]);
      hists[i]->SetMarkerColor(fHistColors[i]);
      hists[i]->SetMarkerStyle(21);
      hists[i]->SetDirectory(nullptr);
      stack.Add(hists[i].get());
   }
}

AtTabEnergyLoss::AtTabEnergyLoss()
   : AtTabCanvas("dE/dx", 2, 2), fRawEvent(AtViewerManager::Instance()->GetRawEventName()),
     fEvent(AtViewerManager::Instance()->GetEventName()),
     fPatternEvent(AtViewerManager::Instance()->GetPatternEventName()),
     fEntry(AtViewerManager::Instance()->GetCurrentEntry()), fBinWidth(100), fSigmaFromHit(1), fTBtoAvg(10),
     fRatioFunc(std::make_unique<TF1>("ratioFunc", "pol0", 0, 1, TF1::EAddToList::kNo))
{

   double maxLength = TMath::Sqrt(TMath::Sq(1000) + TMath::Sq(25));
   int nBins = TMath::CeilNint(maxLength / fBinWidth.Get());

   dEdx[0] = std::make_unique<TH1F>("dEdx1", "dEdX Frag 1", nBins, 0, maxLength);
   dEdx[1] = std::make_unique<TH1F>("dEdx2", "dEdX Frag 2", nBins, 0, maxLength);
   SetStyle(dEdx, dEdxStack);

   dEdxZ[0] = std::make_unique<TH1F>("dEdxZ1", "dEdX Frag 1", nBins, 0, maxLength);
   dEdxZ[1] = std::make_unique<TH1F>("dEdxZ2", "dEdX Frag 2", nBins, 0, maxLength);
   SetStyle(dEdxZ, dEdxStackZ);

   fSumQ[0] = std::make_unique<TH1F>("qSum_0", "Q Sum Frag 1", 512, 0, 512);
   fSumQ[1] = std::make_unique<TH1F>("qSum_1", "Q Sum Frag 2", 512, 0, 512);
   SetStyle(fSumQ, dEdxStackSum);

   fSumFit[0] = std::make_unique<TH1F>("fitSum_0", "Fit Sum Frag 1", 512, 0, 512);
   fSumFit[1] = std::make_unique<TH1F>("fitSum_1", "Fit Sum Frag 2", 512, 0, 512);
   SetStyle(fSumFit, dEdxStackFit);

   fRatioQ = std::make_unique<TH1F>("ratioQ", "Ratio of Q Sum", 512, 0, 512);
   fRatioFit = std::make_unique<TH1F>("ratioFit", "Ratio of Fit Sum", 512, 0, 512);

   fVetoPads = {{0, 1, 1, 6},  {0, 1, 1, 7},  {0, 1, 1, 9},  {0, 1, 1, 10}, {0, 1, 1, 12}, {0, 1, 1, 39},
                {0, 1, 1, 40}, {0, 1, 1, 41}, {0, 1, 1, 44}, {0, 1, 1, 43}, {0, 1, 1, 46}, {0, 1, 3, 13}};

   std::ifstream file("/mnt/projects/hira/e12014/tpcSharedInfo/e12014_zap.csv");
   if (!file.is_open())
      LOG(fatal) << "File not open";

   std::string header;
   std::getline(file, header);

   for (auto &row : CSVRange<int>(file)) {
      fVetoPads.push_back({row[0], row[1], row[2], row[3]});
   }

   fEntry.Attach(this);
}

AtTabEnergyLoss::~AtTabEnergyLoss()
{
   fEntry.Detach(this);
}

void AtTabEnergyLoss::InitTab() {}

void AtTabEnergyLoss::Update(DataHandling::AtSubject *sub)
{
   if (sub == &fEntry) {
      Update();
      UpdateCanvas();
   }
}

void AtTabEnergyLoss::Update()
{
   for (auto &hist : dEdx)
      hist->Reset();
   for (auto &hist : dEdxZ)
      hist->Reset();
   for (auto &hist : fSumQ)
      hist->Reset();
   for (auto &hist : fSumFit)
      hist->Reset();
   fRatioQ->Reset();
   fRatioFit->Reset();

   if (fPatternEvent.GetInfo() == nullptr) {
      return;
   }
   if (fPatternEvent.GetInfo()->GetTrackCand().size() != 2) {
      LOG(info) << "Skipping dEdx in event. Not exactly two tracks.";
      return;
   }

   setAngleAndVertex();
   setdEdX();

   // Fill fSumQ and fSumFit
   FillSums();
   FillRatio();

   fCanvas->cd(1);
   dEdxStackSum.Draw("nostack;hist");
   fCanvas->cd(2);
   dEdxStackFit.Draw("nostack;hist");

   // dEdxStack.Draw("nostack,X0,ep1");
   // dEdxStackZ.Draw("nostack,X0,ep1");

   fCanvas->cd(3);
   fRatioQ->Draw("hist");
   fRatioFunc->Draw("SAME");

   fCanvas->cd(4);
   fRatioFit->Draw("hist");
   fRatioFunc->Draw("SAME");
}

void AtTabEnergyLoss::FillRatio()
{
   // Get the hit to use as the start of the ratio filling. We want the index that is closest to
   // the pad plane (location is closest to zero)
   int index = (fTrackStart[0] < fTrackStart[1]) ? 0 : 1;
   int tbIni = AtTools::GetTB(fTrackStart[index]);
   LOG(info) << "Starting ratio from " << fTrackStart[index] << "(mm) " << AtTools::GetTB(fTrackStart[index]) << "(TB)";

   // Get the track index that has the higher charge. This should be the numberator in the division
   int trackInd = fSumQ[0]->GetBinContent(tbIni + 1) > fSumQ[1]->GetBinContent(tbIni + 1) ? 0 : 1;

   fRatioQ->Divide(fSumQ[trackInd].get(), fSumQ[(trackInd + 1) % 2].get());
   fRatioFit->Divide(fSumFit[trackInd].get(), fSumFit[(trackInd + 1) % 2].get());

   // Get the average of the ratio
   double avg = 0;

   for (int i = 0; i < fTBtoAvg.Get(); ++i) {
      avg += fRatioFit->GetBinContent(tbIni - i + 1);
   }
   avg /= fTBtoAvg.Get();
   LOG(info) << "Average ratio: " << avg;
   fRatioFunc->SetParameter(0, avg);
   fRatioFunc->SetRange(tbIni - fTBtoAvg.Get(), tbIni);
}

void AtTabEnergyLoss::FillFitSum(TH1F *hist, const AtHit &hit, int threshold)
{
   auto func = AtTools::GetHitFunctionTB(hit);
   if (func == nullptr)
      return;

   for (int tb = 0; tb < 512; ++tb) {
      auto val = func->Eval(tb);
      if (val > threshold)
         hist->Fill(tb, val);
   }
}

bool AtTabEnergyLoss::isGoodHit(const AtHit &hit)
{
   auto padRef = AtViewerManager::Instance()->GetMap()->GetPadRef(hit.GetPadNum());
   if (padRef.cobo == 2 && padRef.asad == 3)
      return false;

   for (auto ref : fVetoPads)
      if (ref == padRef)
         return false;

   return true;
}

void AtTabEnergyLoss::FillChargeSum(TH1F *hist, const std::vector<AtHit> &hits, int threshold)
{
   auto rawEvent = fRawEvent.GetInfo();
   if (rawEvent == nullptr) {
      std::cout << "Raw event is null!" << std::endl;
      return;
   }

   std::set<int> usedPads;
   for (auto &hit : hits) {

      if (usedPads.count(hit.GetPadNum()) != 0 || !isGoodHit(hit))
         continue;
      usedPads.insert(hit.GetPadNum());

      auto pad = fRawEvent.Get()->GetPad(hit.GetPadNum());
      if (pad == nullptr)
         continue;

      const auto charge = pad->GetAugment<AtPadArray>("Qreco");
      if (charge == nullptr)
         continue;

      // If we pass all the checks, add the charge to the histogram
      for (int tb = 20; tb < 500; ++tb)
         if (charge->GetArray(tb) > threshold)
            hist->Fill(tb + 0.5, charge->GetArray(tb));

   } // end loop over hits
}
void AtTabEnergyLoss::FillSums(float threshold)
{
   for (int i = 0; i < 2; ++i) {
      fFirstHit[i] = nullptr;
      fTrackStart[i] = 0;

      // Fill fSumQ
      FillChargeSum(fSumQ[i].get(), fPatternEvent.Get()->GetTrackCand()[i].GetHitArray(), threshold);

      // Fill fSumFit
      for (auto &hit : fPatternEvent.GetInfo()->GetTrackCand()[i].GetHitArray()) {
         if (isGoodHit(hit)) {
            FillFitSum(fSumFit[i].get(), hit, threshold);

            // Update the first hit (want highest TB)
            if (fFirstHit[i] == nullptr) {
               fFirstHit[i] = &hit;
            } else if (hit.GetPosition().Z() - fSigmaFromHit.Get() * hit.GetPositionSigma().Z() <
                       fFirstHit[i]->GetPosition().Z() - fSigmaFromHit.Get() * fFirstHit[i]->GetPositionSigma().Z()) {
               fFirstHit[i] = &hit;
            }

            // Update hit location
            auto hitLocation = hit.GetPosition().Z() - fSigmaFromHit.Get() * hit.GetPositionSigma().Z();
            if (fTrackStart[i] < hitLocation) {
               LOG(info) << "Setting start of " << i << " to " << hit.GetPosition() << " at " << hit.GetPadNum();
               fTrackStart[i] = hitLocation;
            }
         }
      }
   }
}
void AtTabEnergyLoss::setdEdX()
{
   for (int i = 0; i < 2; ++i) {

      for (auto &hit : fPatternEvent.GetInfo()->GetTrackCand()[i].GetHitArray())
         if (hit.GetPosition().z() > 0 && hit.GetPosition().Z() > fVertex.Z()) {
            dEdx[i]->Fill(getHitDistanceFromVertex(hit), hit.GetCharge());
            dEdxZ[i]->Fill(getHitDistanceFromVertexAlongZ(hit), hit.GetCharge());
         }

      for (int bin = 0; bin < dEdx[i]->GetNbinsX(); ++bin) {
         dEdx[i]->SetBinError(bin, TMath::Sqrt(dEdx[i]->GetBinContent(bin)));
         dEdxZ[i]->SetBinError(bin, TMath::Sqrt(dEdxZ[i]->GetBinContent(bin)));
      }
   }
}

void AtTabEnergyLoss::setAngleAndVertex()
{
   std::vector<XYZVector> lineStart;
   std::vector<XYZVector> lineStep; // each step is 1 mm in z
   if (fTracks.size() != 2)
      return;

   for (auto &track : fTracks) {
      auto line = dynamic_cast<const AtPatterns::AtPatternLine *>(track.GetPattern());
      if (line == nullptr)
         return;
      lineStart.emplace_back(line->GetPoint());
      lineStep.emplace_back(line->GetDirection());
   }

   fVertex = calcualteVetrex(lineStart, lineStep);
   LOG(info) << "Vertex: " << fVertex;

   auto dot = lineStep[0].Unit().Dot(lineStep[1].Unit());
   fAngle = TMath::ACos(dot) * TMath::RadToDeg();
   LOG(info) << "Angle: " << fAngle;
}

XYZPoint
AtTabEnergyLoss::calcualteVetrex(const std::vector<XYZVector> &lineStart, const std::vector<XYZVector> &lineStep)
{
   auto n = lineStep[0].Cross(lineStep[1]);
   auto n0 = lineStep[0].Cross(n);
   auto n1 = lineStep[1].Cross(n);

   auto c0 = lineStart[0] + (lineStart[1] - lineStart[0]).Dot(n1) / lineStep[0].Dot(n1) * lineStep[0];
   auto c1 = lineStart[1] + (lineStart[0] - lineStart[1]).Dot(n0) / lineStep[1].Dot(n0) * lineStep[1];

   return XYZPoint((c0 + c1) / 2.);
}

double AtTabEnergyLoss::getHitDistanceFromVertex(const AtHit &hit)
{
   auto p = hit.GetPosition();
   auto position = XYZPoint(p.x(), p.y(), p.z());
   auto diff = position - fVertex;
   return TMath::Sqrt(diff.Mag2());
}

double AtTabEnergyLoss::getHitDistanceFromVertexAlongZ(const AtHit &hit)
{
   auto p = hit.GetPosition();
   auto diff = p.z() - fVertex.z();
   return diff;
}
