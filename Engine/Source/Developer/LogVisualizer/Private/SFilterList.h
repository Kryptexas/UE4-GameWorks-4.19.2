// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SLogFilter;

struct FSimpleGraphFilter
{
	FSimpleGraphFilter(FName InName) : Name(InName) {}
	FSimpleGraphFilter(FName InName, bool InEnabled) : Name(InName), bEnabled(InEnabled) {}

	FName Name;
	uint32 bEnabled : 1;
	TEnumAsByte<ELogVerbosity::Type> Verbosity;
};

FORCEINLINE bool operator==(const FSimpleGraphFilter& Left, const FSimpleGraphFilter& Right)
{
	return Left.Name == Right.Name;
}

/**
* A list of filters currently applied to an asset view.
*/
class SLogFilterList : public SCompoundWidget
{
public:
	/** Delegate for when filters have changed */
	DECLARE_DELEGATE(FOnFilterChanged);

	DECLARE_DELEGATE_RetVal(TSharedPtr<SWidget>, FOnGetContextMenu);

	SLATE_BEGIN_ARGS(SLogFilterList){}

		/** Called when an asset is right clicked */
		SLATE_EVENT(FOnGetContextMenu, OnGetContextMenu)

		/** Delegate for when filters have changed */
		SLATE_EVENT(FOnFilterChanged, OnFilterChanged)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void AddFilter(const FString& InFilterName, FLinearColor ColorCategory);
	
	void AddGraphFilter(const FString& InGraphName, const FString& InDataName, FLinearColor ColorCategory);

	bool IsFilterEnabled(const FString& InFilterName, TEnumAsByte<ELogVerbosity::Type> Verbosity = ELogVerbosity::All);
	bool IsFilterEnabled(const FString& InGraphName, const FString& InDataName, TEnumAsByte<ELogVerbosity::Type> Verbosity = ELogVerbosity::All);

	void SomeFilterGetChanged();

	void CreateFiltersMenuCategoryForGraph(FMenuBuilder& MenuBuilder, FName MenuCategory) const;
	void GraphFilterCategoryClicked(FName MenuCategory);
	bool IsGraphFilterCategoryInUse(FName MenuCategory) const;
	void FilterByTypeClicked(FName InGraphName, FName InDataName);
	bool IsAssetTypeActionsInUse(FName InGraphName, FName InDataName) const;

	TSharedRef<SWidget> MakeGraphsFilterMenu();

	TArray<TSharedRef<SLogFilter> > Filters;

	TMap<FName, TArray<FSimpleGraphFilter> > GraphFilters;

	/** The horizontal box which contains all the filters */
	TSharedPtr<SWrapBox> FilterBox;

	/** Delegate for getting the context menu. */
	FOnGetContextMenu OnGetContextMenu;

	/** Delegate for when filters have changed */
	FOnFilterChanged OnFilterChanged;
};