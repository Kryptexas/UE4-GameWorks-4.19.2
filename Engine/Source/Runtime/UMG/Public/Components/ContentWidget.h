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
	virtual int32 GetChildrenCount() const OVERRIDE;
	virtual UWidget* GetChildAt(int32 Index) const OVERRIDE;
	virtual bool AddChild(UWidget* Child, FVector2D Position) OVERRIDE;
	virtual bool RemoveChild(UWidget* Child) OVERRIDE;
	virtual void ReplaceChildAt(int32 Index, UWidget* Child) OVERRIDE;
	// End UPanelWidget

protected:

	UPROPERTY()
	UWidget* Content;
};
