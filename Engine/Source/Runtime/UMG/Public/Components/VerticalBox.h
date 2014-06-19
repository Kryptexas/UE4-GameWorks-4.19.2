// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VerticalBox.generated.h"

/**
 * A vertical box widget is a layout panel allowing child widgets to be automatically laid out
 * vertically.
 */
UCLASS(meta=( Category="Panel" ), ClassGroup=UserInterface)
class UMG_API UVerticalBox : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

	/** The slots containing the widgets that are flowed vertically. */
	UPROPERTY()
	TArray<UVerticalBoxSlot*> Slots;

	//TODO UMG Add ways to make adding slots callable by blueprints.

	UVerticalBoxSlot* AddSlot(UWidget* Content);

	// UPanelWidget
	virtual int32 GetChildrenCount() const override;
	virtual UWidget* GetChildAt(int32 Index) const override;
	virtual int32 GetChildIndex(UWidget* Content) const override;
	virtual bool AddChild(UWidget* Child, FVector2D Position) override;
	virtual bool RemoveChild(UWidget* Child) override;
	virtual void ReplaceChildAt(int32 Index, UWidget* Child) override;
	virtual void InsertChildAt(int32 Index, UWidget* Child) override;
	// End UPanelWidget

#if WITH_EDITOR
	// UWidget interface
	virtual const FSlateBrush* GetEditorIcon() override;
	virtual void ConnectEditorData() override;
	// End UWidget interface
#endif

protected:

	TSharedPtr<class SVerticalBox> MyVerticalBox;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
};
