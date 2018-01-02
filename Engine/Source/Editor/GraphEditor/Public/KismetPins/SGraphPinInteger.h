// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "KismetPins/SGraphPinNum.h"

class GRAPHEDITOR_API SGraphPinInteger : public SGraphPinNum<int32>
{
public:
	SLATE_BEGIN_ARGS(SGraphPinInteger) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

protected:
	//~ Begin SGraphPinString Interface
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	//~ End SGraphPinString Interface
};
