// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "InterpTrackInstColorScale.generated.h"

UCLASS()
class UInterpTrackInstColorScale : public UInterpTrackInst
{
	GENERATED_BODY()
public:
	UInterpTrackInstColorScale(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UInterpTrackInst Instance
	virtual void TermTrackInst(UInterpTrack* Track) override;
	// End UInterpTrackInst Instance
};

