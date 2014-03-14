// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetViewSortManager
{
public:
	/** Constructor */
	FAssetViewSortManager();

	/** Sorts a list using the current ColumnId and Mode. Supply a MajorityAssetType to help discover sorting type (numerical vs alphabetical) */
	void SortList(TArray<TSharedPtr<struct FAssetViewItem>>& AssetItems, FName MajorityAssetType) const;

	/** Sets the sort mode based on the column that was clicked */
	void SetOrToggleSortColumn(FName ColumnId);

	/**	Sets the column to sort */
	void SetSortColumnId( const FName& ColumnId );

	/**	Sets the current sort mode */
	void SetSortMode( const EColumnSortMode::Type InSortMode );

	/** Gets the current sort mode */
	EColumnSortMode::Type GetSortMode() const { return SortMode; }

	/** Gets the current sort column id */
	FName GetSortColumnId() const { return SortColumnId; }

public:
	/** The names of non-type specific columns in the columns view. */
	static const FName NameColumnId;
	static const FName ClassColumnId;
	static const FName PathColumnId;

private:
	/** The name of the column that is currently used for sorting. */
	FName SortColumnId;

	/** Whether the sort is ascending or descending. */
	EColumnSortMode::Type SortMode;
};