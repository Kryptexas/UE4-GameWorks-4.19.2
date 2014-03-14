// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ContentBrowserPCH.h"
#include "AssetViewTypes.h"

static bool CompareFolderFirst(const TSharedPtr<FAssetViewItem>& A, const TSharedPtr<FAssetViewItem>& B, bool bAscending, bool& bComparisonResult)
{
	if(A->GetType() == EAssetItemType::Folder && B->GetType() == EAssetItemType::Folder)
	{
		if(bAscending)
		{
			bComparisonResult = StaticCastSharedPtr<FAssetViewFolder>(A)->FolderName.CompareTo(StaticCastSharedPtr<FAssetViewFolder>(B)->FolderName) < 0;
		}
		else
		{
			bComparisonResult = StaticCastSharedPtr<FAssetViewFolder>(A)->FolderName.CompareTo(StaticCastSharedPtr<FAssetViewFolder>(B)->FolderName) > 0;
		}
		return true;
	}
	else if(A->GetType() == EAssetItemType::Folder)
	{
		bComparisonResult = bAscending;
		return true;
	}
	else if(B->GetType() == EAssetItemType::Folder)
	{
		bComparisonResult = !bAscending;
		return true;
	}

	return false;
}

struct FCompareFAssetItemByNameAscending
{
	FORCEINLINE bool operator()( const TSharedPtr<FAssetViewItem>& A, const TSharedPtr<FAssetViewItem>& B ) const
	{
		bool bFolderComparisonResult = false;
		if(CompareFolderFirst(A, B, true, bFolderComparisonResult))
		{
			return bFolderComparisonResult;
		}

		return StaticCastSharedPtr<FAssetViewAsset>(A)->Data.AssetName.Compare(StaticCastSharedPtr<FAssetViewAsset>(B)->Data.AssetName) < 0;
	}
};

struct FCompareFAssetItemByNameDescending
{
	FORCEINLINE bool operator()( const TSharedPtr<FAssetViewItem>& A, const TSharedPtr<FAssetViewItem>& B ) const
	{
		bool bFolderComparisonResult = false;
		if(CompareFolderFirst(A, B, false, bFolderComparisonResult))
		{
			return bFolderComparisonResult;
		}

		return StaticCastSharedPtr<FAssetViewAsset>(A)->Data.AssetName.Compare(StaticCastSharedPtr<FAssetViewAsset>(B)->Data.AssetName) > 0;
	}
};

struct FCompareFAssetItemByClassAscending
{
	FORCEINLINE bool operator()( const TSharedPtr<FAssetViewItem>& A, const TSharedPtr<FAssetViewItem>& B ) const
	{
		bool bFolderComparisonResult = false;
		if(CompareFolderFirst(A, B, true, bFolderComparisonResult))
		{
			return bFolderComparisonResult;
		}

		return StaticCastSharedPtr<FAssetViewAsset>(A)->Data.AssetClass.Compare(StaticCastSharedPtr<FAssetViewAsset>(B)->Data.AssetClass) < 0;
	}
};

struct FCompareFAssetItemByClassDescending
{
	FORCEINLINE bool operator()( const TSharedPtr<FAssetViewItem>& A, const TSharedPtr<FAssetViewItem>& B ) const
	{
		bool bFolderComparisonResult = false;
		if(CompareFolderFirst(A, B, false, bFolderComparisonResult))
		{
			return bFolderComparisonResult;
		}

		return StaticCastSharedPtr<FAssetViewAsset>(A)->Data.AssetClass.Compare(StaticCastSharedPtr<FAssetViewAsset>(B)->Data.AssetClass) > 0;
	}
};

struct FCompareFAssetItemByPathAscending
{
	FORCEINLINE bool operator()( const TSharedPtr<FAssetViewItem>& A, const TSharedPtr<FAssetViewItem>& B ) const
	{
		bool bFolderComparisonResult = false;
		if(CompareFolderFirst(A, B, true, bFolderComparisonResult))
		{
			return bFolderComparisonResult;
		}

		return StaticCastSharedPtr<FAssetViewAsset>(A)->Data.PackagePath.Compare(StaticCastSharedPtr<FAssetViewAsset>(B)->Data.PackagePath) < 0;
	}
};

struct FCompareFAssetItemByPathDescending
{
	FORCEINLINE bool operator()( const TSharedPtr<FAssetViewItem>& A, const TSharedPtr<FAssetViewItem>& B ) const
	{
		bool bFolderComparisonResult = false;
		if(CompareFolderFirst(A, B, false, bFolderComparisonResult))
		{
			return bFolderComparisonResult;
		}

		return StaticCastSharedPtr<FAssetViewAsset>(A)->Data.PackagePath.Compare(StaticCastSharedPtr<FAssetViewAsset>(B)->Data.PackagePath) > 0;
	}
};

struct FCompareFAssetItemByTagAscending
{
	FCompareFAssetItemByTagAscending(FName InTag) : Tag(InTag) {}

	FORCEINLINE bool operator()( const TSharedPtr<FAssetViewItem>& A, const TSharedPtr<FAssetViewItem>& B ) const
	{
		bool bFolderComparisonResult = false;
		if(CompareFolderFirst(A, B, true, bFolderComparisonResult))
		{
			return bFolderComparisonResult;
		}

		const FString* AValue = StaticCastSharedPtr<FAssetViewAsset>(A)->Data.TagsAndValues.Find(Tag);
		if ( AValue )
		{
			const FString* BValue = StaticCastSharedPtr<FAssetViewAsset>(B)->Data.TagsAndValues.Find(Tag);
			if ( BValue )
			{
				return *AValue < *BValue;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return true;
		}
	}

	FName Tag;
};

struct FCompareFAssetItemByTagDescending
{
	FCompareFAssetItemByTagDescending(FName InTag) : Tag(InTag) {}

	FORCEINLINE bool operator()( const TSharedPtr<FAssetViewItem>& A, const TSharedPtr<FAssetViewItem>& B ) const
	{
		bool bFolderComparisonResult = false;
		if(CompareFolderFirst(A, B, false, bFolderComparisonResult))
		{
			return bFolderComparisonResult;
		}

		const FString* AValue = StaticCastSharedPtr<FAssetViewAsset>(A)->Data.TagsAndValues.Find(Tag);
		if ( AValue )
		{
			const FString* BValue = StaticCastSharedPtr<FAssetViewAsset>(B)->Data.TagsAndValues.Find(Tag);
			if ( BValue )
			{
				return *AValue > *BValue;
			}
			else
			{
				return true;
			}
		}
		else
		{
			return false;
		}
	}

	FName Tag;
};

struct FCompareFAssetItemByTagNumericalAscending
{
	FCompareFAssetItemByTagNumericalAscending(FName InTag) : Tag(InTag) {}

	FORCEINLINE bool operator()( const TSharedPtr<FAssetViewItem>& A, const TSharedPtr<FAssetViewItem>& B ) const
	{
		bool bFolderComparisonResult = false;
		if(CompareFolderFirst(A, B, true, bFolderComparisonResult))
		{
			return bFolderComparisonResult;
		}

		const FString* AValue = StaticCastSharedPtr<FAssetViewAsset>(A)->Data.TagsAndValues.Find(Tag);
		if ( AValue )
		{
			const FString* BValue = StaticCastSharedPtr<FAssetViewAsset>(A)->Data.TagsAndValues.Find(Tag);
			if ( BValue )
			{
				return FCString::Atof(**AValue) < FCString::Atof(**BValue);
			}
			else
			{
				return false;
			}
		}
		else
		{
			return true;
		}
	}

	FName Tag;
};

struct FCompareFAssetItemByTagNumericalDescending
{
	FCompareFAssetItemByTagNumericalDescending(FName InTag) : Tag(InTag) {}

	FORCEINLINE bool operator()( const TSharedPtr<FAssetViewItem>& A, const TSharedPtr<FAssetViewItem>& B ) const
	{
		bool bFolderComparisonResult = false;
		if(CompareFolderFirst(A, B, false, bFolderComparisonResult))
		{
			return bFolderComparisonResult;
		}

		const FString* AValue = StaticCastSharedPtr<FAssetViewAsset>(A)->Data.TagsAndValues.Find(Tag);
		if ( AValue )
		{
			const FString* BValue = StaticCastSharedPtr<FAssetViewAsset>(B)->Data.TagsAndValues.Find(Tag);
			if ( BValue )
			{
				return FCString::Atof(**AValue) > FCString::Atof(**BValue);
			}
			else
			{
				return true;
			}
		}
		else
		{
			return false;
		}
	}

	FName Tag;
};

struct FCompareFAssetItemByTagDimensionalAscending
{
	FCompareFAssetItemByTagDimensionalAscending(FName InTag) : Tag(InTag) {}

	FORCEINLINE bool operator()( const TSharedPtr<FAssetViewItem>& A, const TSharedPtr<FAssetViewItem>& B ) const
	{
		bool bFolderComparisonResult = false;
		if(CompareFolderFirst(A, B, true, bFolderComparisonResult))
		{
			return bFolderComparisonResult;
		}

		const FString* AValue = StaticCastSharedPtr<FAssetViewAsset>(A)->Data.TagsAndValues.Find(Tag);
		if ( AValue )
		{
			const FString* BValue = StaticCastSharedPtr<FAssetViewAsset>(B)->Data.TagsAndValues.Find(Tag);
			if ( BValue )
			{
				TArray<FString> ATokens;
				(*AValue).ParseIntoArray(&ATokens, TEXT("x"), true);
				float ATotal = 1.f;
				for ( auto ATokenIt = ATokens.CreateConstIterator(); ATokenIt; ++ATokenIt )
				{
					ATotal *= FCString::Atof(**ATokenIt);
				}

				TArray<FString> BTokens;
				(*BValue).ParseIntoArray(&BTokens, TEXT("x"), true);
				float BTotal = 1.f;
				for ( auto BTokenIt = BTokens.CreateConstIterator(); BTokenIt; ++BTokenIt )
				{
					BTotal *= FCString::Atof(**BTokenIt);
				}

				return ATotal < BTotal;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return true;
		}
	}

	FName Tag;
};

struct FCompareFAssetItemByTagDimensionalDescending
{
	FCompareFAssetItemByTagDimensionalDescending(FName InTag) : Tag(InTag) {}

	FORCEINLINE bool operator()( const TSharedPtr<FAssetViewItem>& A, const TSharedPtr<FAssetViewItem>& B ) const
	{
		bool bFolderComparisonResult = false;
		if(CompareFolderFirst(A, B, false, bFolderComparisonResult))
		{
			return bFolderComparisonResult;
		}

		const FString* AValue = StaticCastSharedPtr<FAssetViewAsset>(A)->Data.TagsAndValues.Find(Tag);
		if ( AValue )
		{
			const FString* BValue = StaticCastSharedPtr<FAssetViewAsset>(B)->Data.TagsAndValues.Find(Tag);
			if ( BValue )
			{
				TArray<FString> ATokens;
				(*AValue).ParseIntoArray(&ATokens, TEXT("x"), true);
				float ATotal = 1.f;
				for ( auto ATokenIt = ATokens.CreateConstIterator(); ATokenIt; ++ATokenIt )
				{
					ATotal *= FCString::Atof(**ATokenIt);
				}

				TArray<FString> BTokens;
				(*BValue).ParseIntoArray(&BTokens, TEXT("x"), true);
				float BTotal = 1.f;
				for ( auto BTokenIt = BTokens.CreateConstIterator(); BTokenIt; ++BTokenIt )
				{
					BTotal *= FCString::Atof(**BTokenIt);
				}

				return ATotal > BTotal;
			}
			else
			{
				return true;
			}
		}
		else
		{
			return false;
		}
	}

	FName Tag;
};

const FName FAssetViewSortManager::NameColumnId = "Name";
const FName FAssetViewSortManager::ClassColumnId = "Class";
const FName FAssetViewSortManager::PathColumnId = "Path";

FAssetViewSortManager::FAssetViewSortManager()
{
	SortColumnId = NameColumnId;
	SortMode = EColumnSortMode::Ascending;
}

void FAssetViewSortManager::SortList(TArray<TSharedPtr<FAssetViewItem>>& AssetItems, FName MajorityAssetType) const
{
	if ( SortColumnId == NameColumnId )
	{
		if ( SortMode == EColumnSortMode::Ascending )
		{
			AssetItems.Sort( FCompareFAssetItemByNameAscending() );
		}
		else if ( SortMode == EColumnSortMode::Descending )
		{
			AssetItems.Sort( FCompareFAssetItemByNameDescending() );
		}
	}
	else if ( SortColumnId == ClassColumnId )
	{
		if ( SortMode == EColumnSortMode::Ascending )
		{
			AssetItems.Sort( FCompareFAssetItemByClassAscending() );
		}
		else if ( SortMode == EColumnSortMode::Descending )
		{
			AssetItems.Sort( FCompareFAssetItemByClassDescending() );
		}
	}
	else if ( SortColumnId == PathColumnId )
	{
		if ( SortMode == EColumnSortMode::Ascending )
		{
			AssetItems.Sort( FCompareFAssetItemByPathAscending() );
		}
		else if ( SortMode == EColumnSortMode::Descending )
		{
			AssetItems.Sort( FCompareFAssetItemByPathDescending() );
		}
	}
	else
	{
		// Since this ColumnId is not one of preset columns, sort by asset registry tag
		UObject::FAssetRegistryTag::ETagType TagType = UObject::FAssetRegistryTag::TT_Alphabetical;
		if ( MajorityAssetType != NAME_None )
		{
			UClass* Class = FindObject<UClass>(ANY_PACKAGE, *MajorityAssetType.ToString());
			if ( Class )
			{
				UObject* CDO = Class->GetDefaultObject();

				if ( CDO )
				{
					TArray<UObject::FAssetRegistryTag> TagList;
					CDO->GetAssetRegistryTags(TagList);

					for ( auto TagIt = TagList.CreateConstIterator(); TagIt; ++TagIt )
					{
						if ( TagIt->Name == SortColumnId )
						{
							TagType = TagIt->Type;
							break;
						}
					}
				}
			}
		}
		
		if ( TagType == UObject::FAssetRegistryTag::TT_Numerical )
		{
			// The property is a number, compare using atof
			if ( SortMode == EColumnSortMode::Ascending )
			{
				AssetItems.Sort( FCompareFAssetItemByTagNumericalAscending(SortColumnId) );
			}
			else if ( SortMode == EColumnSortMode::Descending )
			{
				AssetItems.Sort( FCompareFAssetItemByTagNumericalDescending(SortColumnId) );
			}
		}
 		else if ( TagType == UObject::FAssetRegistryTag::TT_Dimensional )
 		{
 			// The property is a series of numbers representing dimensions, compare by using atof for each number, delimited by an "x"
 			if ( SortMode == EColumnSortMode::Ascending )
 			{
 				AssetItems.Sort( FCompareFAssetItemByTagDimensionalAscending(SortColumnId) );
 			}
 			else if ( SortMode == EColumnSortMode::Descending )
 			{
 				AssetItems.Sort( FCompareFAssetItemByTagDimensionalDescending(SortColumnId) );
 			}
 		}
		else
		{
			// Unknown or alphabetical, sort alphabetically either way
			if ( SortMode == EColumnSortMode::Ascending )
			{
				AssetItems.Sort( FCompareFAssetItemByTagAscending(SortColumnId) );
			}
			else if ( SortMode == EColumnSortMode::Descending )
			{
				AssetItems.Sort( FCompareFAssetItemByTagDescending(SortColumnId) );
			}
		}
	}
}

void FAssetViewSortManager::SetSortColumnId( const FName& ColumnId )
{
	SortColumnId = ColumnId;
}

void FAssetViewSortManager::SetSortMode( const EColumnSortMode::Type InSortMode )
{
	SortMode = InSortMode;
}

void FAssetViewSortManager::SetOrToggleSortColumn(FName ColumnId)
{
	if ( SortColumnId != ColumnId )
	{
		// Clicked a new column, default to ascending
		SortColumnId = ColumnId;
		SortMode = EColumnSortMode::Ascending;
	}
	else
	{
		// Clicked the current column, toggle sort mode
		if ( SortMode == EColumnSortMode::Ascending )
		{
			SortMode = EColumnSortMode::Descending;
		}
		else
		{
			SortMode = EColumnSortMode::Ascending;
		}
	}
}