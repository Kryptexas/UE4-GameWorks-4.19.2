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

	// USlateNonLeafWidgetComponent
	virtual int32 GetChildrenCount() const OVERRIDE;
	virtual USlateWrapperComponent* GetChildAt(int32 Index) const OVERRIDE;
	virtual bool UHorizontalBoxComponent::AddChild(USlateWrapperComponent* Child) OVERRIDE;
	// End USlateNonLeafWidgetComponent

#if WITH_EDITOR
	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End of UObject interface

	// USlateWrapperComponent interface
	virtual void ConnectEditorData() OVERRIDE;
	// End USlateWrapperComponent interface
#endif

protected:

	TWeakPtr<class SHorizontalBox> MyHorizontalBox;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface
};
