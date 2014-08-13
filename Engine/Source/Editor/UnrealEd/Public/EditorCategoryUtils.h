// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Wrapper for an enum detailing common editor categories. Users can reference
 * these categories in metadata using the enum value name in braces, like so:
 *
 *     UFUNCTION(Category="{Utilities}|MySubCategory")
 *
 * This gives users the ability to reference shared categories across the 
 * engine, but gives us the freedom to easily remap them as needed (also gives 
 * us the ability to easily localize these categories). Games can override these 
 * default mappings with the RegisterCategoryKey() function.
 */
struct FCommonEditorCategory
{
	enum EValue
	{
		// Function categories:
		Ai,			
		Animation,	
		Audio,		
		Development,
		Effects,
		Gameplay,
		Input,
		Math,
		Networking,
		Pawn,
		Physics,
		Rendering,
		Transformation,
		Utilities,
		FlowControl,

		// Type library categories:
		String,
		Text,
		Name,
		Enum,
		Struct,
		Macro,
		Delegates,

		Variables,
	};
};

/**
 * Set of utilities for parsing, querying, and resolving class/function/field 
 * category data.
 */
struct UNREALED_API FEditorCategoryUtils
{
	/**
	 * To facilitate simple category renaming/reordering, we offer a key 
	 * replacement system, where users can specify a key in their category 
	 * metadata that will evaluate to some fully qualified category. Use this
	 * function to register key/category mappings, or to override existing ones
	 * (like those pre-registered for all the "common" categories).
	 *
	 * In metadata, keys are denoted by braces, like {Utilities} here: 
	 *	UFUNCTION(Category="{Utilities}|MySubCategory")
	 * 
	 * @param  Key		A string key that people will use in metadata to reflect this category mapping.
	 * @param  Category	The qualified category path that you want the key expanded to.
	 */
	static void RegisterCategoryKey(FString const& Key, FText const& Category);

	/**
	 * Retrieves a qualified category path for the desired common category.
	 * 
	 * @param  CategoryId	The common category you want a path for.
	 * @return A text string, (empty if the common category was not registered)
	 */
	static FText const& GetCommonCategory(FCommonEditorCategory::EValue CategoryId);

	/**
	 * Utility function that concatenates the supplied sub-category with one 
	 * that matches the root category id.
	 * 
	 * @param  RootCategory	An id denoting the root category that you want prefixed onto the sub-category.
	 * @param  SubCategory	A sub-category that you want postfixed to the root category.
	 * @return A concatenated text string, with the two categories separated by a pipe, '|', character.
	 */
	static FText BuildCategoryString(FCommonEditorCategory::EValue RootCategory, FText const& SubCategory);

	/**
	 * Expands any keys found in the category string (any terms found in square 
	 * brackets), and sanitizes the name (spacing individual words, etc.).
	 * 
	 * @param  RootCategory	An id denoting the root category that you want prefixing the result.
	 * @param  SubCategory	A sub-category that you want postfixing the result.
	 * @return A concatenated text string, with the two categories separated by a pipe, '|', character.
	 */
	static FText GetCategoryDisplayString(FText const& UnsanitizedCategory);

	/**
	 * Expands any keys found in the category string (any terms found in square 
	 * brackets), and sanitizes the name (spacing individual words, etc.).
	 * 
	 * @param  RootCategory	An id denoting the root category that you want prefixing the result.
	 * @param  SubCategory	A sub-category that you want postfixing the result.
	 * @return A concatenated string, with the two categories separated by a pipe, '|', character.
	 */
	static FString GetCategoryDisplayString(FString const& UnsanitizedCategory);

	/**
	 * Parses out the class's "HideCategories" metadata, and returns it 
	 * segmented and sanitized.
	 * 
	 * @param  Class			The class you want to pull data from.
	 * @param  CategoriesOut	An array that will be filled with a list of hidden categories.
	 */
	static void GetClassHideCategories(UClass const* Class, TArray<FString>& CategoriesOut);

	/**
	 * Parses out the class's "ShowCategories" metadata, and returns it 
	 * segmented and sanitized.
	 * 
	 * @param  Class			The class you want to pull data from.
	 * @param  CategoriesOut	An array that will be filled with a list of shown categories.
	 */
	static void GetClassShowCategories(UClass const* Class, TArray<FString>& CategoriesOut);

	/**
	 * Checks to see if the category associated with the supplied common 
	 * category id is hidden from the specified class.
	 * 
	 * @param  Class		The class you want to query.
	 * @param  CategoryId	An id associated with a category that you want to check.
	 * @return True if the common category is hidden, false if not.
	 */
	static bool IsCategoryHiddenFromClass(UClass const* Class, FCommonEditorCategory::EValue CategoryId);

	/**
	 * Checks to see if the specified category is hidden from the supplied class.
	 * 
	 * @param  Class	The class you want to query.
	 * @param  Category A category path that you want to check.
	 * @return True if the category is hidden, false if not.
	 */
	static bool IsCategoryHiddenFromClass(UClass const* Class, FText const& Category);

	/**
	 * Checks to see if the specified category is hidden from the supplied class.
	 * 
	 * @param  Class	The class you want to query.
	 * @param  Category A category path that you want to check.
	 * @return True if the category is hidden, false if not.
	 */
	static bool IsCategoryHiddenFromClass(UClass const* Class, FString const& Category);
};