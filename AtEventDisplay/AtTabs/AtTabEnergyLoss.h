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

   // Data for current entry
   double fAngle;
   XYZPoint fVertex;
   std::vector<std::vector<double>> fCharge;
   std::vector<AtTrack> fTracks;

   // Histograms we are filling
   THStack *dEdxStack{new THStack("hs", "Stacked dE/dx curves")};
   std::array<TH1F *, 2> dEdx;
   THStack *dEdxStackZ{new THStack("hsz", "Stacked dE/dx curves bin in Z")};
   std::array<TH1F *, 2> dEdxZ;

   // Histograms to fill with charge sum
   THStack *dEdxStackSum{new THStack("hsSum", "Stacked dE/dx curves")};
   std::array<TH1Ptr, 2> fSumQ;

public:
   AtTabEnergyLoss();
   ~AtTabEnergyLoss();
   void InitTab() override;
   void Exec() override{};
   void Update(DataHandling::AtSubject *sub) override;

private:
   void Update();
   void setAngleAndVertex();
   void setdEdX();
   double getHitDistanceFromVertex(const AtHit &hit);
   double getHitDistanceFromVertexAlongZ(const AtHit &hit);
   XYZPoint calcualteVetrex(const std::vector<XYZVector> &lineStart, const std::vector<XYZVector> &lineStep);

   void FillChargeSum(float threshold = 15);
   void FillChargeSum(TH1F *hist, const AtPad &pad, int threshold);
   void FillChargeSum(TH1F *hist, const std::vector<AtHit> &hits, int threshold);
};

#endif //#ifndef ATTABENERGYLOSS_H
