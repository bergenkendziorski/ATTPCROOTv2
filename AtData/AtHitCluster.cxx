#include "AtHitCluster.h"

#include <Rtypes.h>

ClassImp(AtHitCluster);

AtHitCluster::AtHitCluster() : AtHit(-1, -1, {0, 0, -1000}, 0)
{
   fCovMatrix.ResizeTo(3, 3);
   for (Int_t iElem = 0; iElem < 9; iElem++)
      fCovMatrix(iElem / 3, iElem % 3) = 0;
}

/**
 * @brief Sets the cov[i,j] = cov[j,i] = val.
 */
void AtHitCluster::SetCovMatrix(int i, int j, double val)
{
   fCovMatrix(i, j) = val;
   fCovMatrix(j, i) = val;
}

/**
 * @brief Add hit to cluster.
 *
 * Adds a hit, updating the clusters position and charge. As well as the covariance
 * matrix. The variance is updated using the charge-weighted variance of the passed hit.
 * The off diagonal elements of the covariance matrix are updated according to the "standard" textbook definition of the
 * covariance matrix \f$ COV_{x,y} = \frac{\Sigma_i^N (x_i - \hat{x})(y_i - \hat{y})}{N-1}\f$.
 *
 */
void AtHitCluster::AddHit(const AtHit &hit)
{
   // Update the weights
   auto wRel = byElementInv(hit.GetPositionVariance());
   auto wRel2 = byElementMult(wRel, wRel);
   auto w = hit.GetCharge() * wRel;
   auto w2 = hit.GetCharge() * wRel2;
   fTotalWeight += w;
   fTotalWeight2 += w2;

   // Update the position based on new weights
   // Pos = pos_old + w/wTot * ( hitPos - pos_old)
   fCharge += hit.GetCharge();
   auto wRatio = byElementDiv(w, fTotalWeight);
   auto posOld = fPosition;
   fPosition += byElementMult(wRatio, hit.GetPosition() - fPosition);

   // Update on diagonal elements of the covariance matrix
   auto a = byElementMult(hit.GetPosition() - fPosition, hit.GetPosition() - posOld);
   auto update = byElementMult(a, w);
   fCovNum(0, 0) += update.X();
   fCovNum(1, 1) += update.Y();
   fCovNum(2, 2) += update.Z();
   // TODO: Use thid value to update covariance matrix according to eqn 24

   // Update off diagonal elements of covariance matrix
}

AtHitCluster::XYZVector AtHitCluster::byElementMult(const XYZVector &a, const XYZVector &b)
{
   return {a.X() * b.X(), a.Y() * b.Y(), a.Z() * b.Z()};
}
AtHitCluster::XYZVector AtHitCluster::byElementDiv(const XYZVector &a, const XYZVector &b)
{
   return byElementMult(a, byElementInv(b));
}
AtHitCluster::XYZVector AtHitCluster::byElementInv(const XYZVector &a)
{
   return {1 / a.X(), 1 / a.Y(), 1 / a.Z()};
}
