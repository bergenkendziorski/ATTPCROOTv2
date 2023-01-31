#include "AtTabEnergyLoss.h"

#include "AtDataManip.h"
#include "AtEvent.h"
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
     fEntry(AtViewerManager::Instance()->GetCurrentEntry()), fBinWidth(100)
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

   fEntry.Attach(this);
}

AtTabEnergyLoss::~AtTabEnergyLoss()
{
   fEntry.Detach(this);
}

void AtTabEnergyLoss::InitTab() {}

void AtTabEnergyLoss::Update(DataHandling::AtSubject *sub)
{
   if (sub == &fEntry)
      Update();
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

   if (fPatternEvent.GetInfo() == nullptr) {
      UpdateCanvas();
      return;
   }
   if (fPatternEvent.GetInfo()->GetTrackCand().size() != 2) {
      LOG(info) << "Skipping dEdx in event";
      UpdateCanvas();
      return;
   }

   setAngleAndVertex();
   setdEdX();
   FillSums();

   fCanvas->cd(1);
   dEdxStack.Draw("nostack,X0,ep1");
   fCanvas->cd(3);
   dEdxStackZ.Draw("nostack,X0,ep1");

   fCanvas->cd(2);
   dEdxStackSum.Draw("nostack;hist");
   fCanvas->cd(4);
   dEdxStackFit.Draw("nostack;hist");

   UpdateCanvas();
}

void AtTabEnergyLoss::FillFitSum(TH1F *hist, const AtHit &hit)
{
   auto func = AtTools::GetHitFunctionTB(hit);
   if (func == nullptr)
      return;

   for (int tb = 0; tb < 512; ++tb)
      hist->Fill(tb, func->Eval(tb));
}
/// Assumes there is a non-null rawEvent
void AtTabEnergyLoss::FillChargeSum(TH1F *hist, const AtPad &pad, int threshold)
{
   const auto charge = pad.GetAugment<AtPadArray>("Qreco");
   if (charge == nullptr) {
      LOG(error) << "Could not find charge augment for pad " << pad.GetPadNum() << " in raw event!";
      return;
   }

   for (int tb = 20; tb < 500; ++tb)
      if (charge->GetArray(tb) > threshold)
         hist->Fill(tb + 0.5, charge->GetArray(tb));
}

void AtTabEnergyLoss::FillSums(TH1F *hist, const std::vector<AtHit> &hits, int threshold)
{
   auto rawEvent = fRawEvent.GetInfo();
   if (rawEvent == nullptr) {
      std::cout << "Raw event is null!" << std::endl;
      return;
   }

   std::set<int> usedPads;
   for (auto &hit : hits) {

      if (usedPads.count(hit.GetPadNum()) != 0)
         continue;
      usedPads.insert(hit.GetPadNum());

      auto pad = fRawEvent.Get()->GetPad(hit.GetPadNum());
      if (pad == nullptr)
         continue;
      FillChargeSum(hist, *pad, threshold);
   }
}
void AtTabEnergyLoss::FillSums(float threshold)
{

   for (int i = 0; i < 2; ++i) {
      FillSums(fSumQ[i].get(), fPatternEvent.GetInfo()->GetTrackCand()[i].GetHitArray(), threshold);
      for (auto &hit : fPatternEvent.GetInfo()->GetTrackCand()[i].GetHitArray())
         FillFitSum(fSumFit[i].get(), hit);
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
