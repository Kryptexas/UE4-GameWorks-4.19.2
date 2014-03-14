// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// A sticky note.  Level designers can place these in the level and then
// view them as a batch in the error/warnings window.
//=============================================================================

#pragma once
#include "Note.generated.h"

UCLASS(MinimalAPI, hidecategories = (Input))
class ANote : public AActor
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Note)
	FString Text;

	// Reference to sprite visualization component
	UPROPERTY()
	TSubobjectPtr<class UBillboardComponent> SpriteComponent;

	// Reference to arrow visualization component
	UPROPERTY()
	TSubobjectPtr<class UArrowComponent> ArrowComponent;

#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
	// Begin AActor Interface
	virtual void CheckForErrors();
	// End AActor Interface
#endif
};



