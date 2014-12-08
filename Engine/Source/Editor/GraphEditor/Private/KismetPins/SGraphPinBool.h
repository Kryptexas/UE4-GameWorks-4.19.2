// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphPinBool : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SGraphPinBool) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

protected:
	// Begin SGraphPin interface
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	// End SGraphPin interface

	/** Determine if the check box should be checked or not */
	ESlateCheckBoxState::Type IsDefaultValueChecked() const;

	/** Called when check box is changed */
	void OnDefaultValueCheckBoxChanged( ESlateCheckBoxState::Type InIsChecked );
};
