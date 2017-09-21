// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Features/IModularFeature.h"
#include "ARHitTestingSupport.generated.h"

/**
 * A result of an intersection found during a hit-test.
 */
USTRUCT( BlueprintType, Category="AR", meta=(Experimental))
struct FARHitTestResult
{
	GENERATED_BODY();
	
	// Default constructor
	FARHitTestResult() {};
	
	/**
	 * The type of the hit-test result.
	 */
	// UPROPERTY( BlueprintReadOnly, Category = "ARHitTestResult" )
	// EARKitHitTestResultType Type;
	
	/**
	 * The distance from the camera to the intersection in meters.
	 */
	UPROPERTY( BlueprintReadOnly, Category = "ARHitTestResult")
	float Distance;
	
	/**
	 * The transformation matrix that defines the intersection's rotation, translation and scale
	 * relative to the world.
	 */
	UPROPERTY( BlueprintReadOnly, Category = "ARHitTestResult")
	FTransform Transform;
	
	/**
	 * The anchor that the hit-test intersected.
	 *
	 * An anchor will only be provided for existing plane result types.
	 */
	// UPROPERTY( BlueprintReadOnly, Category = "ARHitTestResult")
	// class UARAnchor* Anchor = nullptr;
};


class IARHitTestingSupport : public IModularFeature
{
public:
	//
	// MODULAR FEATURE SUPPORT
	//

	/**
	* Part of the pattern for supporting modular features.
	*
	* @return the name of the feature.
	*/
	static FName GetModularFeatureName()
	{
		static const FName ModularFeatureName = FName(TEXT("ARHitTesting"));
		return ModularFeatureName;
	}

	virtual bool ARLineTraceFromScreenPoint(const FVector2D ScreenPosition, TArray<FARHitTestResult>& OutHitResults) = 0;
};
