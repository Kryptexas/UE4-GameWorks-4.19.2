// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "ARTrackable.h"

#include "AppleARKitSettings.generated.h"

UCLASS(Config=Engine)
class APPLEARKIT_API UAppleARKitSettings : public UObject
{
	GENERATED_BODY()

public:
	UAppleARKitSettings()
			: bEnableLiveLinkForFaceTracking(false)
			, LiveLinkPublishingPort(11111)
			, DefaultFaceTrackingLiveLinkSubjectName(FName("iPhoneXFaceAR"))
			, DefaultFaceTrackingDirection(EARFaceTrackingDirection::FaceRelative)
	{
	}

	/** Whether to publish face blend shapes to LiveLink or not */
	UPROPERTY(Config, EditAnywhere, Category="AR Settings")
	bool bEnableLiveLinkForFaceTracking;

	/** The port to use when listening/sending LiveLink face blend shapes via the network */
	UPROPERTY(Config, EditAnywhere, Category="AR Settings")
	int32 LiveLinkPublishingPort;

	/** The default name to use when publishing face tracking name */
	UPROPERTY(Config, EditAnywhere, Category="AR Settings")
	FName DefaultFaceTrackingLiveLinkSubjectName;

	/** The default tracking to use when tracking face blend shapes (face relative or mirrored). Defaults to face relative */
	UPROPERTY(Config, EditAnywhere, Category="AR Settings")
	EARFaceTrackingDirection DefaultFaceTrackingDirection;
};
