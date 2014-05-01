// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ContentWidget.generated.h"

UCLASS(Abstract)
class UMG_API UContentWidget : public USlateNonLeafWidgetComponent
{
	GENERATED_UCLASS_BODY()

	UFUNCTION()
	virtual USlateWrapperComponent* GetContent();
	
	UFUNCTION()
	virtual void SetContent(USlateWrapperComponent* InContent);

	// USlateNonLeafWidgetComponent
	virtual int32 GetChildrenCount() const OVERRIDE;
	virtual USlateWrapperComponent* GetChildAt(int32 Index) const OVERRIDE;
	virtual bool AddChild(USlateWrapperComponent* Child, FVector2D Position) OVERRIDE;
	virtual bool RemoveChild(USlateWrapperComponent* Child) OVERRIDE;
	// End USlateNonLeafWidgetComponent

protected:

	UPROPERTY()
	USlateWrapperComponent* Content;
};
