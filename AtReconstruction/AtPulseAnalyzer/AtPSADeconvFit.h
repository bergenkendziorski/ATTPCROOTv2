#ifndef ATPSADECONVFIT_H
#define ATPSADECONVFIT_H
#include "AtPSADeconv.h"

class AtPSADeconvFit : public AtPSADeconv {

public:
protected:
   HitData getZandQ(const AtPad::trace &charge) override;
};

#endif // #ifndef ATPSADECONVFIT_H
