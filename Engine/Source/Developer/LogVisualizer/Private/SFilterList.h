// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SFilter;

/**
* A list of filters currently applied to an asset view.
*/
class SFilterList : public SCompoundWidget
{
public:
	/** Delegate for when filters have changed */
	DECLARE_DELEGATE(FOnFilterChanged);

	DECLARE_DELEGATE_RetVal(TSharedPtr<SWidget>, FOnGetContextMenu);

	SLATE_BEGIN_ARGS(SFilterList){}

		/** Called when an asset is right clicked */
		SLATE_EVENT(FOnGetContextMenu, OnGetContextMenu)

		/** Delegate for when filters have changed */
		SLATE_EVENT(FOnFilterChanged, OnFilterChanged)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void AddFilter(const FString& InFilterName, FLinearColor ColorCategory);

	bool IsFilterEnabled(const FString& InFilterName, TEnumAsByte<ELogVerbosity::Type> Verbosity = ELogVerbosity::All);

	void SomeFilterGetChanged();

	TArray<TSharedRef<SFilter> > Filters;

	/** The horizontal box which contains all the filters */
	TSharedPtr<SWrapBox> FilterBox;

	/** Delegate for getting the context menu. */
	FOnGetContextMenu OnGetContextMenu;

	/** Delegate for when filters have changed */
	FOnFilterChanged OnFilterChanged;
};