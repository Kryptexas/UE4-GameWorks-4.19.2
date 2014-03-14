// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphPinExec : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SGraphPinExec)	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);

protected:
	// Begin SGraphPin interface
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() OVERRIDE;
	virtual const FSlateBrush* GetPinIcon() const OVERRIDE;
	// End SGraphPin interface

	void CachePinBrushes(bool bForceCache = false) const;

protected:
	mutable bool bWasEventPin;
};
