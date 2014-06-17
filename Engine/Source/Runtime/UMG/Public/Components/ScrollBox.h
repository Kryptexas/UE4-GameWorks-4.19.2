// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ScrollBox.generated.h"

/** An arbitrary scrollable collection of widgets.  Great for presenting 10-100 widgets in a list.  Doesn't support virtualization. */
UCLASS(meta=( Category="Panel" ), ClassGroup=UserInterface)
class UMG_API UScrollBox : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

	/** The slots containing the widgets that are scrollable. */
	UPROPERTY()
	TArray<UScrollBoxSlot*> Slots;

	//TODO UMG Add ways to make adding slots callable by blueprints.

	UScrollBoxSlot* AddSlot(UWidget* Content);

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
	virtual void ConnectEditorData() override;
	// End UWidget interface
#endif

protected:

	TSharedPtr<class SScrollBox> MyScrollBox;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
};
