// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

class SFacialAnimationBulkImporter : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFacialAnimationBulkImporter)
	{}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	bool IsImportButtonEnabled() const;

	FReply HandleImportClicked();
};