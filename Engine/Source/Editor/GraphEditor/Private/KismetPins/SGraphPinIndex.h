// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphPinIndex : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SGraphPinIndex) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);
	
	FEdGraphPinType OnGetPinType() const;
	void OnTypeChanged(const FEdGraphPinType& PinType);

protected:
	// Begin SGraphPin interface
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() OVERRIDE;
	// End SGraphPin interface
};
