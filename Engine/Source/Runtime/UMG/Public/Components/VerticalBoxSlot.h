// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateWrapperTypes.h"

#include "VerticalBoxSlot.generated.h"

/** The Slot for the UVerticalBox, contains the widget that is flowed vertically */
UCLASS()
class UMG_API UVerticalBoxSlot : public UPanelSlot
{
	GENERATED_UCLASS_BODY()
	
	/** The padding area between the slot and the content it contains. */
	UPROPERTY(EditDefaultsOnly, Category=Layout)
	FMargin Padding;

	/** How much space this slot should occupy in the direction of the panel. */
	UPROPERTY(EditDefaultsOnly, Category=Layout)
	FSlateChildSize Size;

	/** The alignment of the object horizontally. */
	UPROPERTY(EditDefaultsOnly, Category=Layout)
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/** The alignment of the object vertically. */
	UPROPERTY(EditDefaultsOnly, Category=Layout)
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;

	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetPadding(FMargin InPadding);

	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetSize(FSlateChildSize InSize);

	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetHorizontalAlignment(EHorizontalAlignment InHorizontalAlignment);

	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetVerticalAlignment(EVerticalAlignment InVerticalAlignment);

	// UPanelSlot interface
	virtual void SyncronizeProperties() override;
	// End of UPanelSlot interface

	/** Builds the underlying FSlot for the Slate layout panel. */
	void BuildSlot(TSharedRef<SVerticalBox> InVerticalBox);

private:
	//TODO UMG Slots should hold weak or shared refs to slots.

	/** A raw pointer to the slot to allow us to adjust the size, padding...etc at runtime. */
	SVerticalBox::FSlot* Slot;
};
