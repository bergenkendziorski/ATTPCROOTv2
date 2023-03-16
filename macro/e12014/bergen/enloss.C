#ifndef ENLOSS_C
#define ENLOSS_C

//#include "/mnt/analysis/e12014/bergen/ATTPCROOTv2/macro/e12014/simulation/eventGenerator/fissionMC.cpp"

#ifndef __CLING__
#include "/mnt/analysis/e12014/bergen/ATTPCROOTv2/AtGenerators/AtTPCFissionGeneratorV3.h"
#endif

void enloss()
{
   int num_points = 10;
   int eloss = 0;
   double energy1 = 500;
   double energy2 = 500;
   double charge1 = 20;
   double charge2 = 20;
   double beta = 0.2271732;
   for (int i = 0; i <= num_points; i++) {
      eloss = 5;
      energy1 -= eloss;
   }

   auto gen = new AtTPCFissionGeneratorV3();
}

#endif
