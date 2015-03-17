// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ContentWidget.generated.h"

/**  */
UCLASS(Abstract)
class UMG_API UContentWidget : public UPanelWidget
{
	GENERATED_BODY()
public:
	UContentWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	/**  */
	UFUNCTION(BlueprintCallable, Category="Widget|Panel")
	UPanelSlot* GetContentSlot() const;

protected:

	// UPanelWidget
	virtual UClass* GetSlotClass() const override;
	// End UPanelWidget
};
