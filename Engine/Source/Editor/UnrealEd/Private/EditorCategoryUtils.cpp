// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "EditorCategoryUtils.h"

#define LOCTEXT_NAMESPACE "EditorCategoryUtils"

/*******************************************************************************
 * Static FEditorCategoryUtils Helpers
 ******************************************************************************/

namespace FEditorCategoryUtilsImpl
{
	/**
	 * Gets the table that tracks mappings from string keys to qualified 
	 * category paths. Inits the structure if it hadn't been before (adds 
	 * default mappings for all FCommonEditorCategory values)
	 * 
	 * @return A map from string category keys, to fully qualified category paths.
	 */
	static FFormatNamedArguments& GetCategoryTable();

	/**
	 * Performs a lookup into the category key table, retrieving a fully 
	 * qualified category path for the specified key.
	 * 
	 * @param  Key	The key you want a category path for.
	 * @return The category display string associated with the specified key (an empty string if an entry wasn't found).
	 */
	static FText const& GetCategory(FString const& Key);


	/** Metadata tags */
	static const FName ClassHideCategoriesMetaKey(TEXT("HideCategories"));
	static const FName ClassShowCategoriesMetaKey(TEXT("ShowCategories"));
}

#define ENUM_STRING(EnumVal) FString(#EnumVal)

//------------------------------------------------------------------------------
static FFormatNamedArguments& FEditorCategoryUtilsImpl::GetCategoryTable()
{
	static FFormatNamedArguments CategoryLookup;

	static bool bInitialized = false;
	// this function is reentrant, so we have to guard against recursion
	if (!bInitialized)
	{
		bInitialized = true;

#define ASSIGN_ROOT_CATEGORY(EnumVal, Text) FCommonEditorCategory::EnumVal; \
		FEditorCategoryUtils::RegisterCategoryKey(ENUM_STRING(EnumVal), LOCTEXT(#EnumVal L"Category", Text))
#define REGISTER_ROOT_CATEGORY(EnumVal) ASSIGN_ROOT_CATEGORY(EnumVal, #EnumVal)

		REGISTER_ROOT_CATEGORY(Ai);
		REGISTER_ROOT_CATEGORY(Animation);
		REGISTER_ROOT_CATEGORY(Audio);
		REGISTER_ROOT_CATEGORY(Development);
		REGISTER_ROOT_CATEGORY(Effects);
		ASSIGN_ROOT_CATEGORY(Gameplay, "Game");
		REGISTER_ROOT_CATEGORY(Input);
		REGISTER_ROOT_CATEGORY(Math);
		REGISTER_ROOT_CATEGORY(Networking);
		REGISTER_ROOT_CATEGORY(Pawn);
		REGISTER_ROOT_CATEGORY(Rendering);
		REGISTER_ROOT_CATEGORY(Utilities);

#undef REGISTER_ROOT_CATEGORY
#undef ASSIGN_ROOT_CATEGORY

		// NOTE: the root category has to be registered prior to this
#define REGISTER_SUB_CATEGORY(RootId, EnumVal) FCommonEditorCategory::EnumVal; \
		FText EnumVal##_Category = FEditorCategoryUtils::BuildCategoryString(FCommonEditorCategory::RootId, LOCTEXT(#EnumVal L"Category", #EnumVal)); \
		FEditorCategoryUtils::RegisterCategoryKey(ENUM_STRING(EnumVal), EnumVal##_Category)

		REGISTER_SUB_CATEGORY(Utilities, Transformation);
		REGISTER_SUB_CATEGORY(Utilities, String);
		REGISTER_SUB_CATEGORY(Utilities, Text);
		REGISTER_SUB_CATEGORY(Utilities, Name);
		REGISTER_SUB_CATEGORY(Utilities, Enum);
		REGISTER_SUB_CATEGORY(Utilities, Struct);
		REGISTER_SUB_CATEGORY(Utilities, Macro);

#undef REGISTER_SUB_CATEGORY
	}

	return CategoryLookup;
}

//------------------------------------------------------------------------------
static FText const& FEditorCategoryUtilsImpl::GetCategory(FString const& Key)
{
	FFormatNamedArguments const& CategoryLookup = GetCategoryTable();
	if (FFormatArgumentValue const* FoundCategory = CategoryLookup.Find(Key.ToUpper()))
	{
		return *FoundCategory->TextValue;
	}
	return FText::GetEmpty();
}

/*******************************************************************************
 * FEditorCategoryUtils
 ******************************************************************************/

//------------------------------------------------------------------------------
void FEditorCategoryUtils::RegisterCategoryKey(FString const& Key, FText const& Category)
{
	FEditorCategoryUtilsImpl::GetCategoryTable().Add(Key.ToUpper(), FEditorCategoryUtils::GetCategoryDisplayString(Category));
}

//------------------------------------------------------------------------------
FText const& FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::EValue CategoryId)
{
	static TMap<FCommonEditorCategory::EValue, FString> CommonCategoryKeys;
	if (CommonCategoryKeys.Num() == 0)
	{
#define REGISTER_COMMON_CATEGORY(EnumVal) \
		CommonCategoryKeys.Add(FCommonEditorCategory::EnumVal, ENUM_STRING(EnumVal));

		REGISTER_COMMON_CATEGORY(Ai);
		REGISTER_COMMON_CATEGORY(Animation);
		REGISTER_COMMON_CATEGORY(Audio);
		REGISTER_COMMON_CATEGORY(Development);
		REGISTER_COMMON_CATEGORY(Effects);
		REGISTER_COMMON_CATEGORY(Gameplay);
		REGISTER_COMMON_CATEGORY(Input);
		REGISTER_COMMON_CATEGORY(Math);
		REGISTER_COMMON_CATEGORY(Networking);
		REGISTER_COMMON_CATEGORY(Pawn);
		REGISTER_COMMON_CATEGORY(Rendering);
		REGISTER_COMMON_CATEGORY(Utilities);

		REGISTER_COMMON_CATEGORY(Transformation);
		REGISTER_COMMON_CATEGORY(String);
		REGISTER_COMMON_CATEGORY(Text);
		REGISTER_COMMON_CATEGORY(Name);
		REGISTER_COMMON_CATEGORY(Enum);
		REGISTER_COMMON_CATEGORY(Struct);
		REGISTER_COMMON_CATEGORY(Macro);

#undef REGISTER_COMMON_CATEGORY
	}

	if (FString* CategoryKey = CommonCategoryKeys.Find(CategoryId))
	{
		return FEditorCategoryUtilsImpl::GetCategory(*CategoryKey);
	}
	return FText::GetEmpty();
}

//------------------------------------------------------------------------------
FText FEditorCategoryUtils::BuildCategoryString(FCommonEditorCategory::EValue RootId, FText const& SubCategory)
{
	FText ConstructedCategory;

	FText const& RootCategory = GetCommonCategory(RootId);
	if (RootCategory.IsEmpty())
	{
		ConstructedCategory = SubCategory;
	}
	else if (SubCategory.IsEmpty())
	{
		ConstructedCategory = RootCategory;
	}
	else
	{
		ConstructedCategory = FText::Format(LOCTEXT("ConcatedCategory", "{0}|{1}"), RootCategory, SubCategory);
	}
	return ConstructedCategory;
}

//------------------------------------------------------------------------------
FText FEditorCategoryUtils::GetCategoryDisplayString(FText const& UnsanitizedCategory)
{
	return FText::FromString(GetCategoryDisplayString(UnsanitizedCategory.ToString()));
}

//------------------------------------------------------------------------------
FString FEditorCategoryUtils::GetCategoryDisplayString(FString const& UnsanitizedCategory)
{
	FString DisplayString = UnsanitizedCategory;

	int32 KeyIndex = INDEX_NONE;
	do
	{
		KeyIndex = DisplayString.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, KeyIndex);
		if (KeyIndex != INDEX_NONE)
		{
			int32 EndIndex = DisplayString.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, KeyIndex);
			if (EndIndex != INDEX_NONE)
			{
				FString ToReplaceStr(EndIndex+1 - KeyIndex, *DisplayString + KeyIndex);
				FString ReplacementStr;
				
				int32 KeyLen = EndIndex - (KeyIndex + 1);
				if (KeyLen > 0)
				{
					FString Key(KeyLen, *DisplayString + KeyIndex+1);
					ReplacementStr = FEditorCategoryUtilsImpl::GetCategory(Key).ToString();
				}
				DisplayString.ReplaceInline(*ToReplaceStr, *ReplacementStr);
			}
			KeyIndex = EndIndex;
		}

	} while (KeyIndex != INDEX_NONE);

	DisplayString = FName::NameToDisplayString(DisplayString, /*bIsBool =*/false);
	DisplayString.ReplaceInline(TEXT("| "), TEXT("|"));

	return DisplayString;
}

//------------------------------------------------------------------------------
void FEditorCategoryUtils::GetClassHideCategories(UClass const* Class, TArray<FString>& CategoriesOut)
{
	CategoriesOut.Empty();

	using namespace FEditorCategoryUtilsImpl;
	if (Class->HasMetaData(ClassHideCategoriesMetaKey))
	{
		FString const& HideCategories = Class->GetMetaData(ClassHideCategoriesMetaKey);

		HideCategories.ParseIntoArray(&CategoriesOut, TEXT(" "), /*InCullEmpty =*/true);
		
		for (FString& Category : CategoriesOut)
		{
			Category = GetCategoryDisplayString(FText::FromString(Category)).ToString();
		}
	}
}

//------------------------------------------------------------------------------
void  FEditorCategoryUtils::GetClassShowCategories(UClass const* Class, TArray<FString>& CategoriesOut)
{
	CategoriesOut.Empty();

	using namespace FEditorCategoryUtilsImpl;
	if (Class->HasMetaData(ClassShowCategoriesMetaKey))
	{
		FString const& ShowCategories = Class->GetMetaData(ClassShowCategoriesMetaKey);
		ShowCategories.ParseIntoArray(&CategoriesOut, TEXT(" "), /*InCullEmpty =*/true);

		for (FString& Category : CategoriesOut)
		{
			Category = GetCategoryDisplayString(FText::FromString(Category)).ToString();
		}
	}
}

//------------------------------------------------------------------------------
bool FEditorCategoryUtils::IsCategoryHiddenFromClass(UClass const* Class, FCommonEditorCategory::EValue CategoryId)
{
	return IsCategoryHiddenFromClass(Class, GetCommonCategory(CategoryId));
}

//------------------------------------------------------------------------------
bool FEditorCategoryUtils::IsCategoryHiddenFromClass(UClass const* Class, FText const& Category)
{
	return IsCategoryHiddenFromClass(Class, Category.ToString());
}

//------------------------------------------------------------------------------
bool FEditorCategoryUtils::IsCategoryHiddenFromClass(UClass const* Class, FString const& Category)
{
	bool bIsHidden = false;

	TArray<FString> ClassHideCategories;
	GetClassHideCategories(Class, ClassHideCategories);

	// run the category through sanitization so we can ensure compares will hit
	FString const DisplayCategory = GetCategoryDisplayString(Category);

	for (FString& HideCategory : ClassHideCategories)
	{
		bIsHidden = (HideCategory == DisplayCategory);
		if (bIsHidden)
		{
			TArray<FString> ClassShowCategories;
			GetClassShowCategories(Class, ClassShowCategories);
			// if they hid it, and showed it... favor showing (could be a shown in a sub-class, and hid in a super)
			bIsHidden = (ClassShowCategories.Find(DisplayCategory) == INDEX_NONE);
		}
		else // see if the category's root is hidden
		{
			TArray<FString> SubCategoryList;
			DisplayCategory.ParseIntoArray(&SubCategoryList, TEXT("|"), /*InCullEmpty =*/true);

			FString FullSubCategoryPath;
			for (FString const& SubCategory : SubCategoryList)
			{
				FullSubCategoryPath += SubCategory;
				if ((HideCategory == SubCategory) || (HideCategory == FullSubCategoryPath))
				{
					TArray<FString> ClassShowCategories;
					GetClassShowCategories(Class, ClassShowCategories);
					// if they hid it, and showed it... favor showing (could be a shown in a sub-class, and hid in a super)
					bIsHidden = (ClassShowCategories.Find(DisplayCategory) == INDEX_NONE);
				}
				FullSubCategoryPath += "|";
			}
		}

		if (bIsHidden)
		{
			break;
		}
	}

	return bIsHidden;
}

#undef ENUM_STRING
#undef LOCTEXT_NAMESPACE
