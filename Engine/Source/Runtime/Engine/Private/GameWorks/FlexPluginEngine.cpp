#include "GameWorks/IFlexPluginBridge.h"
#include "GameWorks/FlexPluginCommon.h"

IFlexPluginBridge* GFlexPluginBridge = nullptr;

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
