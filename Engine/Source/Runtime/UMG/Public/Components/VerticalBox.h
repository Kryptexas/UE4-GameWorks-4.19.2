// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VerticalBox.generated.h"

/**
 * A vertical box widget is a layout panel allowing child widgets to be automatically laid out
 * vertically.
 */
UCLASS(meta=( BlueprintSpawnableComponent, Category="Panel" ), ClassGroup=UserInterface)
class UMG_API UVerticalBox : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

	/** The slots containing the widgets that are flowed vertically. */
	UPROPERTY(EditDefaultsOnly, EditInline, Category=Slots)
	TArray<UVerticalBoxSlot*> Slots;

	//TODO UMG Add ways to make adding slots callable by blueprints.

	UVerticalBoxSlot* AddSlot(UWidget* Content);

	// UPanelWidget
	virtual int32 GetChildrenCount() const OVERRIDE;
	virtual UWidget* GetChildAt(int32 Index) const OVERRIDE;
	virtual int32 GetChildIndex(UWidget* Content) const OVERRIDE;
	virtual bool AddChild(UWidget* Child, FVector2D Position) OVERRIDE;
	virtual bool RemoveChild(UWidget* Child) OVERRIDE;
	virtual void ReplaceChildAt(int32 Index, UWidget* Child) OVERRIDE;
	virtual void InsertChildAt(int32 Index, UWidget* Child) OVERRIDE;
	// End UPanelWidget

#if WITH_EDITOR
	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End of UObject interface

	// UWidget interface
	virtual void ConnectEditorData() OVERRIDE;
	// End UWidget interface
#endif

protected:

	TWeakPtr<class SVerticalBox> MyVerticalBox;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of UWidget interface
};
