// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphPinMaterialInput : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SGraphPinMaterialInput) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

protected:
	// Begin SGraphPin interface
	virtual FSlateColor GetPinColor() const OVERRIDE;
	// End SGraphPin interface

private:
	/** Cached Pin Color */
	FSlateColor CachedPinColor;
};
