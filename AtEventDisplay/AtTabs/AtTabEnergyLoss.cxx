#include "AtTabEnergyLoss.h"

#include "AtEvent.h"
#include "AtPatternEvent.h"
#include "AtPatternLine.h"
#include "AtRawEvent.h"
#include "AtTrack.h"
#include "AtViewerManager.h"

#include <TCanvas.h>
#include <TH1.h>
#include <THStack.h>

using XYZVector = ROOT::Math::XYZVector;
using XYZPoint = ROOT::Math::XYZPoint;

AtTabEnergyLoss::AtTabEnergyLoss()
   : AtTabCanvas("dE/dx", 2, 1), fRawEvent(AtViewerManager::Instance()->GetRawEventName()),
     fEvent(AtViewerManager::Instance()->GetEventName()),
     fPatternEvent(AtViewerManager::Instance()->GetPatternEventName()),
     fEntry(AtViewerManager::Instance()->GetCurrentEntry()), fBinWidth(100)
{

   double maxLength = TMath::Sqrt(TMath::Sq(1000) + TMath::Sq(25));
   int nBins = TMath::CeilNint(maxLength / fBinWidth.Get());

   dEdx[0] = new TH1F("dEdx1", "dEdX Frag 1", nBins, 0, maxLength);
   dEdx[0]->SetMarkerStyle(21);
   dEdx[0]->SetMarkerColor(9);
   dEdx[1] = new TH1F("dEdx2", "dEdX Frag 2", nBins, 0, maxLength);
   dEdx[1]->SetMarkerStyle(21);
   dEdx[1]->SetMarkerColor(31);

   dEdxStack->Add(dEdx[0]);
   dEdxStack->Add(dEdx[1]);

   dEdxZ[0] = new TH1F("dEdxZ1", "dEdX Frag 1", nBins, 0, maxLength);
   dEdxZ[0]->SetMarkerStyle(21);
   dEdxZ[0]->SetMarkerColor(9);
   dEdxZ[1] = new TH1F("dEdxZ2", "dEdX Frag 2", nBins, 0, maxLength);
   dEdxZ[1]->SetMarkerStyle(21);
   dEdxZ[1]->SetMarkerColor(31);

   dEdxStackZ->Add(dEdxZ[0]);
   dEdxStackZ->Add(dEdxZ[1]);

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
   for (auto hist : dEdx)
      hist->Reset();
   for (auto hist : dEdxZ)
      hist->Reset();

   if (fPatternEvent.GetInfo() == nullptr)
      return;
   if (fPatternEvent.GetInfo()->GetTrackCand().size() != 2) {
      LOG(info) << "Skipping dEdx in event";
      UpdateCanvas();
      return;
   }

   setAngleAndVertex();
   setdEdX();

   fCanvas->cd(1);
   dEdxStack->Draw("nostack,X0,ep1");
   fCanvas->cd(2);
   dEdxStackZ->Draw("nostack,X0,ep1");

   UpdateCanvas();
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
