// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphPinNum : public SGraphPinString
{
public:
	SLATE_BEGIN_ARGS(SGraphPinNum) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

protected:
	// Begin SGraphPinText interface
	virtual void SetTypeInValue(const FText& NewTypeInValue, ETextCommit::Type CommitInfo) OVERRIDE;
	// End SGraphPinText interface
};
