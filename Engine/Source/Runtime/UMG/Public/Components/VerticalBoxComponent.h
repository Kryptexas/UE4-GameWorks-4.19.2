// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VerticalBoxComponent.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UVerticalBoxComponent : public USlateNonLeafWidgetComponent
{
	GENERATED_UCLASS_BODY()

	/** The items placed on the canvas */
	UPROPERTY(EditAnywhere, EditInline, Category=Slots)
	TArray<UVerticalBoxSlot*> Slots;

	UVerticalBoxSlot* AddSlot(USlateWrapperComponent* Content);

	// USlateNonLeafWidgetComponent
	virtual int32 GetChildrenCount() const OVERRIDE;
	virtual USlateWrapperComponent* GetChildAt(int32 Index) const OVERRIDE;
	virtual bool AddChild(USlateWrapperComponent* Child) OVERRIDE;
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

	TWeakPtr<class SVerticalBox> MyVerticalBox;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface
};
