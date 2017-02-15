// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "AvfMediaSettings.generated.h"


UCLASS(config=Engine)
class AVFMEDIAFACTORY_API UAvfMediaSettings
	: public UObject
{
	GENERATED_BODY()

public:
	 
	/** Default constructor. */
	UAvfMediaSettings();

public:

	/** Play audio tracks via the operating system's native sound mixer. */
	UPROPERTY(config, EditAnywhere, Category=Debug)
	bool NativeAudioOut;
};
