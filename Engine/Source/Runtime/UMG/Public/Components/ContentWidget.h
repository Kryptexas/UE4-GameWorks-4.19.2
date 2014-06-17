// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ContentWidget.generated.h"

UCLASS(Abstract)
class UMG_API UContentWidget : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

	UFUNCTION()
	virtual UWidget* GetContent();
	
	UFUNCTION()
	virtual void SetContent(UWidget* InContent);

	// UPanelWidget
	virtual int32 GetChildrenCount() const override;
	virtual UWidget* GetChildAt(int32 Index) const override;
	virtual bool AddChild(UWidget* Child, FVector2D Position) override;
	virtual bool RemoveChild(UWidget* Child) override;
	virtual void ReplaceChildAt(int32 Index, UWidget* Child) override;
	// End UPanelWidget

protected:

	UPROPERTY()
	UPanelSlot* ContentSlot;
};
