// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "HorizontalBox.generated.h"

UCLASS(meta=( Category="Panel" ), ClassGroup=UserInterface)
class UMG_API UHorizontalBox : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

	/** The items placed on the canvas */
	UPROPERTY()
	TArray<UHorizontalBoxSlot*> Slots;

	UHorizontalBoxSlot* AddSlot(UWidget* Content);

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

	TSharedPtr<class SHorizontalBox> MyHorizontalBox;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
};
