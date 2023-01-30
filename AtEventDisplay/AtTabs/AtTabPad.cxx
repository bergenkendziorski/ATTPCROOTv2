#include "AtTabPad.h"

#include "AtAuxPad.h" // for AtAuxPad
#include "AtDigiPar.h"
#include "AtEvent.h"
#include "AtPad.h"
#include "AtPadArray.h"
#include "AtPadBase.h" // for AtPadBase
#include "AtRawEvent.h"
#include "AtTabInfo.h"
#include "AtViewerManager.h"

#include <FairLogger.h>
#include <FairRun.h>
#include <FairRuntimeDb.h>

#include <TCanvas.h>
#include <TF1.h>
#include <TH1.h> // for TH1D

#include <iostream> // for operator<<, basic_ostream::operator<<
#include <memory>   // for make_unique, allocator, unique_ptr
namespace DataHandling {
class AtSubject;
}

constexpr auto cRED = "\033[1;31m";
constexpr auto cYELLOW = "\033[1;33m";
constexpr auto cNORMAL = "\033[0m";
constexpr auto cGREEN = "\033[1;32m";
constexpr auto cBLUE = "\033[1;34m";

ClassImp(AtTabPad);

AtTabPad::AtTabPad(int nRow, int nCol, TString name) : AtTabCanvas(name, nRow, nCol)
{
   if (AtViewerManager::Instance() == nullptr)
      throw "AtViewerManager must be initialized before creating tabs!";

   fPadNum = &AtViewerManager::Instance()->GetPadNum();
   fPadNum->Attach(this);
}
AtTabPad::~AtTabPad()
{
   fPadNum->Detach(this);
}

void AtTabPad::InitTab()
{
   std::cout << " =====  AtEventTabPad::Init =====" << std::endl;

   if (fTabName == "AtPad")
      fTabName = TString::Format(fTabName + " %d", fTabId);

   auto man = AtViewerManager::Instance();
   fTabInfo->AddAugment(std::make_unique<AtTabInfoFairRoot<AtEvent>>(man->GetEventName()));
   fTabInfo->AddAugment(std::make_unique<AtTabInfoFairRoot<AtRawEvent>>(man->GetRawEventName()));

   std::cout << " AtEventTabPad::Init : Initialization complete! "
             << "\n";
}

void AtTabPad::MakeTab(TEveWindowSlot *slot)
{
   AtTabCanvas::MakeTab(slot);
}

void AtTabPad::Exec()
{
   auto fRawEvent = GetFairRootInfo<AtRawEvent>();
   if (fRawEvent == nullptr) {
      LOG(debug) << "fRawEvent is nullptr for tab " << fTabId << "! Please set the raw event branch.";
      return;
   }

   // Redraw any Auxiliary channels
   for (auto &[pos, toDraw] : fDrawMap) {
      fCanvas->cd(pos + 1);
      auto hist = toDraw.second;
      if (toDraw.first == PadDrawType::kAuxPad) {
         hist->Reset();

         auto auxPad = fRawEvent->GetAuxPad(fAugNames[pos]);
         if (auxPad != nullptr)
            DrawAdc(hist, *auxPad);
         hist->Draw();
      }
   }
   UpdateCvsPad();
}

void AtTabPad::Update(DataHandling::AtSubject *sub)
{
   if (sub == fPadNum)
      DrawPad();
}

void AtTabPad::DrawPad()
{

   auto fRawEvent = GetFairRootInfo<AtRawEvent>();

   if (fRawEvent == nullptr) {
      LOG(debug) << "fRawEvent is nullptr for tab " << fTabId << "! Please set the raw event branch.";
      return;
   }

   auto fPad = fRawEvent->GetPad(fPadNum->Get());

   for (auto &[pos, toDraw] : fDrawMap) {
      if (toDraw.first == PadDrawType::kAuxPad)
         continue;

      fCanvas->cd(pos + 1);
      auto hist = toDraw.second;

      if (fPad == nullptr) {
         hist->Reset();
         hist->Draw();
         continue;
      }

      switch (toDraw.first) {
      case PadDrawType::kADC: DrawAdc(hist, *fPad); break;
      case PadDrawType::kRawADC: DrawRawAdc(hist, *fPad); break;
      case PadDrawType::kArrAug: DrawArrayAug(hist, *fPad, fAugNames[pos]); break;
      case PadDrawType::kAuxPad:
         auto auxPad = fRawEvent->GetAuxPad(fAugNames[pos]);
         if (auxPad != nullptr)
            DrawAdc(hist, *auxPad);
         break;
      }
      if (fDrawHits.find(pos) != fDrawHits.end())
         DrawHit(*fPad);
   }

   UpdateCvsPad();
}

void AtTabPad::DrawAdc(TH1D *hist, const AtPad &pad)
{
   for (int i = 0; i < 512; i++) {
      hist->SetBinContent(i + 1, pad.GetADC()[i]);
   }
   hist->Draw();
}

void AtTabPad::DrawRawAdc(TH1D *hist, const AtPad &pad)
{
   for (int i = 0; i < 512; i++) {
      hist->SetBinContent(i + 1, pad.GetRawADC()[i]);
   }
   hist->Draw();
}

void AtTabPad::DrawArrayAug(TH1D *hist, const AtPad &pad, TString augName)
{
   auto aug = dynamic_cast<const AtPadArray *>(pad.GetAugment(augName.Data()));
   if (aug == nullptr)
      return;

   for (int i = 0; i < 512; i++) {
      hist->SetBinContent(i + 1, aug->GetArray()[i]);
   }
   hist->Draw();
}

void AtTabPad::DrawHit(const AtPad &pad)
{
   // Loop through all hits and
   auto event = GetFairRootInfo<AtEvent>();
   for (auto &hit : event->GetHits())
      if (hit->GetPadNum() == pad.GetPadNum()) {

         // Draw the hit
         auto Q = hit->GetCharge();
         auto z = hit->GetPosition().Z();
         auto zSig = hit->GetPositionSigma().Z();
         if (zSig == 0)
            LOG(error) << "Hits that are points (sig_z = 0) are not supported yet!";

         // position in mm to TB.
         auto fPar = dynamic_cast<AtDigiPar *>(FairRun::Instance()->GetRuntimeDb()->getContainer("AtDigiPar"));

         if (fPar == nullptr) {
            LOG(error) << "Could not find the digipar file!";
            return;
         }

         // Get pad plane position in TB
         auto totalDriftTime = fPar->GetZPadPlane() / (fPar->GetDriftVelocity() * 10.);
         auto totalDriftTimeTB = totalDriftTime / (fPar->GetTBTime() / 1000.);
         auto padPlaneTB = fPar->GetTBEntrance() - totalDriftTimeTB;

         // Get drift time in TB
         auto driftTime = z / (fPar->GetDriftVelocity() * 10.);  // driftTime in us (z in mm, vel in cm/us)
         auto driftTB = driftTime / (fPar->GetTBTime() / 1000.); // time in us. TBTime in ns.

         // Get the TB corresponding to the hit
         auto zTB = driftTB + padPlaneTB;
         // Get the variance in TB corresponding to the hitarray
         auto zSigTB = zSig / (fPar->GetDriftVelocity() * 10.) / (fPar->GetTBTime() / 1000.);

         TF1 *hitFunc = new TF1("hit", "gaus", 0, 512);
         hitFunc->SetParameter(0, Q / (zSigTB * std::sqrt(2 * TMath::Pi())));
         hitFunc->SetParameter(1, zTB);
         hitFunc->SetParameter(2, zSigTB);

         LOG(info) << "Drawing hit function";
         hitFunc->Draw("SAME");
      }
   return;
}

void AtTabPad::SetDraw(Int_t pos, PadDrawType type)
{
   auto name = TString::Format("padHist_Tab%i_%i", fTabId, pos);
   TH1D *padHist = new TH1D(name, name, 512, 0, 512);
   fDrawMap.emplace(pos, std::make_pair(type, padHist));
}

void AtTabPad::DrawHits(int row, int col)
{
   fDrawHits.insert(row * fCols + col);
}
void AtTabPad::DrawADC(int row, int col)
{
   SetDraw(row * fCols + col, PadDrawType::kADC);
}

void AtTabPad::DrawRawADC(int row, int col)
{
   SetDraw(row * fCols + col, PadDrawType::kRawADC);
}

void AtTabPad::DrawArrayAug(TString augName, int row, int col)
{
   SetDraw(row * fCols + col, PadDrawType::kArrAug);
   fAugNames.emplace(row * fCols + col, augName);
}

void AtTabPad::DrawAuxADC(TString auxName, int row, int col)
{
   SetDraw(row * fCols + col, PadDrawType::kAuxPad);
   fAugNames.emplace(row * fCols + col, auxName);
}

void AtTabPad::UpdateCvsPad()
{
   fCanvas->Modified();
   fCanvas->Update();
}
