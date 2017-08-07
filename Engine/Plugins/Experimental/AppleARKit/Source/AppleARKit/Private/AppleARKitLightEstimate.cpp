// AppleARKit
#include "AppleARKitLightEstimate.h"
#include "AppleARKit.h"

#if ARKIT_SUPPORT

FAppleARKitLightEstimate::FAppleARKitLightEstimate( ARLightEstimate* InARLightEstimate )
{
	bIsValid = InARLightEstimate != nullptr;
	AmbientIntensity = InARLightEstimate != nullptr ? InARLightEstimate.ambientIntensity : 0.0f;
}

#endif // #ARKIT_SUPPORT
