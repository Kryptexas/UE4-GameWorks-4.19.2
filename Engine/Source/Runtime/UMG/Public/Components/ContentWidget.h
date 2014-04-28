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

	virtual int32 GetChildrenCount() const OVERRIDE;
	virtual USlateWrapperComponent* GetChildAt(int32 Index) const OVERRIDE;
	virtual bool AddChild(USlateWrapperComponent* InContent, FVector2D Position) OVERRIDE;

protected:

	UPROPERTY()
	USlateWrapperComponent* Content;
};
