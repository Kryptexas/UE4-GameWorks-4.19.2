// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateNonLeafWidgetComponent.generated.h"

UCLASS(Abstract)
class UMG_API USlateNonLeafWidgetComponent : public USlateWrapperComponent
{
	GENERATED_UCLASS_BODY()

	virtual int32 GetChildrenCount() const { return 0; }
	virtual USlateWrapperComponent* GetChildAt(int32 Index) const { return NULL; }

	virtual bool AddChild(USlateWrapperComponent* Child) { return false; }
};
