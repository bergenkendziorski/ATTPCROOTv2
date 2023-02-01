#ifndef ATTABENERGYLOSS_H
#define ATTABENERGYLOSS_H

#include "AtEvent.h"
#include "AtPatternEvent.h"
#include "AtRawEvent.h"
#include "AtTabCanvas.h"
#include "AtTrack.h"

#include <Math/Point3D.h>
#include <Math/Vector3D.h>
#include <THStack.h>
class AtHit;
class AtRawEvent;
class AtEvent;
class AtPatternEvent;
namespace DataHandling {
class AtSubject;
}
class THStack;
class TH1F;

/**
 * @brief Class to calculate dE/dx for fission fragments (but possibly generally).
 *
 */
class AtTabEnergyLoss : public AtTabCanvas, public DataHandling::AtObserver {
protected:
   using XYZVector = ROOT::Math::XYZVector;
   using XYZPoint = ROOT::Math::XYZPoint;
   using TH1Ptr = std::unique_ptr<TH1F>;

   AtTabInfoFairRoot<AtRawEvent> fRawEvent;         //< Points to selected RawEventBranch
   AtTabInfoFairRoot<AtEvent> fEvent;               //< Points to selected EventBranch
   AtTabInfoFairRoot<AtPatternEvent> fPatternEvent; //< Points to selected Pattern Event Branch
   DataHandling::AtTreeEntry &fEntry;               //< Tracks current entry
   DataHandling::AtSimpleType<float> fBinWidth;     //< Width of binning in mm

   /// Number of std dev to go after the mean of the first hit before calcualting the ratio of the
   /// histograms.
   DataHandling::AtSimpleType<float> fSigmaFromHit;
   DataHandling::AtSimpleType<int> fTBtoAvg; //< Number of TB from track start to average for ratio.

   // Data for current entry
   double fAngle;
   XYZPoint fVertex;
   std::vector<std::vector<double>> fCharge;
   std::vector<AtTrack> fTracks;

   // Helpful things
   const std::array<Color_t, 2> fHistColors = {9, 31};

   THStack dEdxStack{"hs", "Stacked dE/dx curves"};
   std::array<TH1Ptr, 2> dEdx;

   THStack dEdxStackZ{"hsz", "Stacked dE/dx curves bin in Z"};
   std::array<TH1Ptr, 2> dEdxZ;

   // Histograms to fill with charge sum
   THStack dEdxStackSum{"hsSum", "dQ/dZ (summing charge)"};
   std::array<TH1Ptr, 2> fSumQ;

   THStack dEdxStackFit{"hsFit", "dQ/dZ (summing gaussian fits)"};
   std::array<TH1Ptr, 2> fSumFit;

   TH1Ptr fRatioQ;   //<Ratio of max(fSumQ)/min(fSumQ)
   TH1Ptr fRatioFit; //<Ratio of max(fSumFit)/min(fSumFit)
   TH1Ptr fProxy;    //<Proxy for Z in an event
   TH1Ptr fZHist;    //<Z in an event

   std::array<float, 2> fTrackStart;
   std::array<AtHit *, 2> fFirstHit{nullptr, nullptr}; //< First hit calculated according to the gaussian fits

   std::unique_ptr<TF1> fRatioFunc;
   std::unique_ptr<TF1> fProxyFunc;
   std::unique_ptr<TF1> fZFunc;

   std::vector<AtPadReference> fVetoPads;

public:
   AtTabEnergyLoss();
   ~AtTabEnergyLoss();
   void InitTab() override;
   void Exec() override{};
   void Update(DataHandling::AtSubject *sub) override;

   static float GetZ(int Zcn, float proxy);

private:
   void SetStyle(std::array<TH1Ptr, 2> &hists, THStack &stack);
   void Update();
   void setAngleAndVertex();
   void setdEdX();
   double getHitDistanceFromVertex(const AtHit &hit);
   double getHitDistanceFromVertexAlongZ(const AtHit &hit);
   XYZPoint calcualteVetrex(const std::vector<XYZVector> &lineStart, const std::vector<XYZVector> &lineStep);

   void FillSums(float threshold = 15);

   void FillChargeSum(TH1F *hist, const std::vector<AtHit> &hits, int threshold);
   void FillFitSum(TH1F *hist, const AtHit &hit, int threshold);

   void FillRatio();
   bool isGoodHit(const AtHit &hit);
};

#endif //#ifndef ATTABENERGYLOSS_H
