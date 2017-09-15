#include "GameWorks/FlexPluginCommon.h"

FFlexInertialScale::FFlexInertialScale()
{
	LinearInertialScale = 0.35f;
	AngularInertialScale = 0.75f;
}

FFlexPhase::FFlexPhase()
{
	AutoAssignGroup = true;
	Group = 0;
	SelfCollide = false;
	IgnoreRestCollisions = false;
	Fluid = false;
}
