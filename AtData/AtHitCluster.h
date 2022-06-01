#ifndef ATHITCLUSTER_HH
#define ATHITCLUSTER_HH

#include "AtHit.h"

#include <Rtypes.h>
#include <TMatrixDSym.h>
#include <TMatrixDfwd.h>
#include <TMatrixT.h>

class TBuffer;
class TClass;
class TMemberInspector;

class AtHitCluster : public AtHit {
protected:
   // Off diagonal elements are calcualed using eqn 18. Diagonal from eqn 24
   TMatrixD fCovMatrix; //< Cluster covariance matrix

   // Corresponds to equations 15 for off diagonal and 20 for on diagonal
   TMatrixDSym fCovNum; //< Numerator for updating

   XYZVector fTotalWeight{0, 0, 0};  //< Sum of 1/var*q .
   XYZVector fTotalWeight2{0, 0, 0}; //< Sum of 1/var^2*q

   Double_t fLength{-999};
   Int_t fClusterID{-1};

public:
   AtHitCluster();
   AtHitCluster(const AtHitCluster &cluster) = default;
   virtual ~AtHitCluster() = default;

   void SetCovMatrix(TMatrixD matrix) { fCovMatrix = std::move(matrix); }
   void SetCovMatrix(int i, int j, double val);
   void SetLength(Double_t length) { fLength = length; }
   void SetClusterID(Int_t id) { fClusterID = id; }
   void AddHit(const AtHit &hit);

   Double_t GetLength() const { return fLength; }
   const TMatrixD &GetCovMatrix() const { return fCovMatrix; }
   Int_t GetClusterID() const { return fClusterID; }

protected:
   XYZVector byElementMult(const XYZVector &a, const XYZVector &b);
   XYZVector byElementDiv(const XYZVector &a, const XYZVector &b);
   XYZVector byElementInv(const XYZVector &a);

   ClassDef(AtHitCluster, 3);
};

#endif
