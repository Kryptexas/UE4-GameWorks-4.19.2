// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "HorizontalBoxComponent.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UHorizontalBoxComponent : public USlateNonLeafWidgetComponent
{
	GENERATED_UCLASS_BODY()

	/** The items placed on the canvas */
	UPROPERTY(EditAnywhere, EditInline, Category=Slots)
	TArray<UHorizontalBoxSlot*> Slots;

	UHorizontalBoxSlot* AddSlot(USlateWrapperComponent* Content);

#if WITH_EDITOR
	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End of UObject interface
#endif

protected:

	TWeakPtr<class SHorizontalBox> MyHorizontalBox;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface

	// USlateNonLeafWidgetComponent interface
	virtual void OnKnownChildrenChanged() OVERRIDE;
	// End of USlateNonLeafWidgetComponent
};
