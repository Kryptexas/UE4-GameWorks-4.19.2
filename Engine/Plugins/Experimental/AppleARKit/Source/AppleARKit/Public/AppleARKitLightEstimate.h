#pragma once

// UE4
#include "Math/SHMath.h"

// ARKit
#if ARKIT_SUPPORT
#import <ARKit/ARKit.h>
#endif // ARKIT_SUPPORT

// AppleARKit
#include "AppleARKitLightEstimate.generated.h"

/**
 * A light estimate represented as spherical harmonics
 */
USTRUCT( BlueprintType )
struct FAppleARKitLightEstimate
{
	GENERATED_BODY()

	// Default constructor
	FAppleARKitLightEstimate() {};

#if ARKIT_SUPPORT

	/** 
	 * This is a conversion copy-constructor that takes a raw ARLightEstimate and fills this structs 
	 * members with the UE4-ified versions of ARLightEstimate's properties.
	 */ 
	FAppleARKitLightEstimate( ARLightEstimate* InARLightEstimate );

#endif // #ARKIT_SUPPORT

	/** True if light estimation was enabled for the session and light estimation was successful */
	UPROPERTY( BlueprintReadOnly, Category="AppleARKit|Light Estimate" )
	bool bIsValid;

	/**
	 * Ambient intensity of the lighting.
	 * 
	 * In a well lit environment, this value is close to 1000. It typically ranges from 0 
	 * (very dark) to around 2000 (very bright).
	 */
	UPROPERTY( BlueprintReadOnly, Category="AppleARKit|Light Estimate" )
	float AmbientIntensity;
};
