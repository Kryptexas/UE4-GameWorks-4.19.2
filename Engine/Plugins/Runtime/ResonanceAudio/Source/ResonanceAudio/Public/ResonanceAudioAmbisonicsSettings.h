//
// Copyright (C) Google Inc. 2017. All rights reserved.
//

#pragma once

#include "ResonanceAudioCommon.h"
#include "IAmbisonicsMixer.h"
#include "ResonanceAudioAmbisonicsSettings.generated.h"

UENUM()
enum class EAmbisonicsOrder : uint8
{
	FirstOrder,
	SecondOrder,
	ThirdOrder
};

UCLASS()
class RESONANCEAUDIO_API UResonanceAudioAmbisonicsSettings : public UAmbisonicsSubmixSettingsBase
{
	GENERATED_BODY()

public:
	//Which order of ambisonics to use for this submix.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Ambisonics)
		EAmbisonicsOrder AmbisonicsOrder;
};