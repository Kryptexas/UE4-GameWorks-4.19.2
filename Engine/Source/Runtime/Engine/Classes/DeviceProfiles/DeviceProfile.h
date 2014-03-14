// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DeviceProfile.h: Declares the UDeviceProfile class.
=============================================================================*/

#pragma once

#include "DeviceProfile.generated.h"

UCLASS(config=DeviceProfiles, perobjectconfig, Blueprintable)
class ENGINE_API UDeviceProfile : public UObject
{
	GENERATED_UCLASS_BODY()

	/** The type of this profile, I.e. IOS, Windows, PS4 etc */
	UPROPERTY(VisibleAnywhere, config, Category=DeviceSettings)
	FString DeviceType;

	/** The name of the parent profile of this object */
	UPROPERTY(EditAnywhere, config, Category=DeviceSettings)
	FString BaseProfileName;

	/** The parent object of this profile, it is the object matching this DeviceType with the BaseProfileName */
	UObject* Parent;

	/** Flag used in the editor to determine whether the profile is visible in the property matrix */
	bool bVisible;


	/// Console Variables

	/** The collection of CVars which is set from this profile */
	UPROPERTY(EditAnywhere, config, Category=ConsoleVariables)
	TArray<FString> CVars;


public:

#if WITH_EDITOR
	// Begin UObject interface
	virtual void PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent ) OVERRIDE;
	// End UObject interface
#endif
};
