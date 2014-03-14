// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SGraphPinObject.h"

/////////////////////////////////////////////////////
// SGraphPinClass

class SGraphPinClass : public SGraphPinObject
{
public:
	SLATE_BEGIN_ARGS(SGraphPinClass) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);
protected:

	// Called when a new class was picked via the asset picker
	void OnPickedNewClass(UClass* ChosenClass);

	// Begin SGraphPinObject interface
	virtual FReply OnClickUse() OVERRIDE;
	virtual bool AllowSelfPinWidget() const OVERRIDE { return false; }
	virtual TSharedRef<SWidget> GenerateAssetPicker() OVERRIDE;
	virtual FText GetDefaultComboText() const OVERRIDE;
	virtual FOnClicked GetOnUseButtonDelegate() OVERRIDE;
	// End SGraphPinObject interface

};
