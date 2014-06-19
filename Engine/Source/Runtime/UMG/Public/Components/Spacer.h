// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Spacer.generated.h"

/** A spacer widget; it does not have a visual representation, and just provides padding between other widgets. */
UCLASS(meta=( Category="Misc" ), ClassGroup=UserInterface)
class UMG_API USpacer : public UWidget
{
	GENERATED_UCLASS_BODY()

	/** The size of the spacer */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FVector2D Size;

	/** Sets the size of the spacer */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void SetSize(FVector2D InSize);
	
	// UWidget interface
	virtual void SyncronizeProperties() override;
	// End of UWidget interface

#if WITH_EDITOR
	virtual const FSlateBrush* GetEditorIcon() override;
#endif

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

protected:
	TSharedPtr<SSpacer> MySpacer;
};
