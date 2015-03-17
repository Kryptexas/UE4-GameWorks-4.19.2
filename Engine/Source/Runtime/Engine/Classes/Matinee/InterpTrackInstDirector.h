// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Matinee/InterpTrackInst.h"
#include "InterpTrackInstDirector.generated.h"

UCLASS(MinimalAPI)
class UInterpTrackInstDirector : public UInterpTrackInst
{
	GENERATED_BODY()
public:
	ENGINE_API UInterpTrackInstDirector(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY()
	class AActor* OldViewTarget;


	// Begin UInterpTrackInst Instance
	virtual void TermTrackInst(class UInterpTrack* Track) override;
	// End UInterpTrackInst Instance
};

