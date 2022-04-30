#include "../helper.h"

TH1F *hQfft = new TH1F("qfft", "Q_FFT", 1000, 0, 4096);
TH1F *hQcal = new TH1F("qcal", "Q_cal", 1000, 0, 4096);
TH1F *hQraw = new TH1F("qRaw", "Q_raw", 1000, 0, 4096);
TH1F *hQraw2 = new TH1F("qRaw2", "Q_raw", 1000, 0, 4096);

void fillEvent(ULong64_t eventNumber)
{
   std::cout << "Loading event: " << eventNumber << std::endl;
   if (!loadEvent(eventNumber))
      return;
   hQfft->Reset();
   hQcal->Reset();
   hQraw->Reset();

   std::cout << eventPtr << " " << eventFilteredPtr << std::endl;

   for (auto &hit : eventPtr->GetHitArray()) {
      hQcal->Fill(hit.GetCharge());
   }
   for (auto &hit : eventFilteredPtr->GetHitArray())
      hQfft->Fill(hit.GetCharge());
   for (auto &pad : rawEventPtr->GetPads()) {
      auto max = *std::max_element(pad->GetADC().begin(), pad->GetADC().end());
      hQraw->Fill(max);
   }
   for (auto &pad : rawEventFilteredPtr->GetPads()) {
      auto max = *std::max_element(pad->GetADC().begin(), pad->GetADC().end());
      hQraw2->Fill(max);
   }
}

void checkCalibration(TString fileName)
{
   loadRun(fileName, "AtRawEvent", "AtRawEventCal", "AtEventCal", "AtEventFFTCal");
   fillEvent(1);
}
