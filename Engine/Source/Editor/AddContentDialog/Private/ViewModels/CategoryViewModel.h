// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** A view model for displaying a content source category in the UI. */
class FCategoryViewModel
{
public:
	FCategoryViewModel();

	FCategoryViewModel(EContentSourceCategory InCategory);

	/** Gets the display name of the category. */
	FText GetText() const;

	/** Gets the brush which should be used to draw the icon for the category. */
	const FSlateBrush* GetIconBrush() const;

	inline bool operator==(const FCategoryViewModel& Other) const;

	uint32 GetTypeHash() const;

private:
	void Initialize();

private:
	EContentSourceCategory Category;
	FText Text;
	const FSlateBrush* IconBrush;
};

inline uint32 GetTypeHash(const FCategoryViewModel& CategoryViewModel)
{
	return CategoryViewModel.GetTypeHash();
}