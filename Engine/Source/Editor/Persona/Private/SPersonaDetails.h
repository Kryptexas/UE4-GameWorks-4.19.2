// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

class SPersonaDetails : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPersonaDetails) {}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	TSharedPtr<class IDetailsView> DetailsView;
};