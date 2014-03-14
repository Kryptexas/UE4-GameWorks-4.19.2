// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#ifndef __ObjectEditorUtils_h__
#define __ObjectEditorUtils_h__

#pragma once

#if WITH_EDITOR
class UProperty;

namespace FObjectEditorUtils
{

	/**
	 * Gets the category this property belongs to.
	 *
	 * @param	InProperty	Property we want the category name of.
	 * @return	Category name of the given property.
	 */
	ENGINE_API FString GetCategory( const class UProperty* InProperty );	

	/**
	 * Gets the category this property belongs to.
	 *
	 * @param	InProperty	Property we want the category name of.
	 * @return	Category name of the given property.
	 */
	ENGINE_API FName GetCategoryFName( const class UProperty* InProperty );


	/**
	 * Query if a function is flagged as hidden from the given class either by category or by function name
	 *
	 * @param	InFunction	Function to check
	 * @param	InClass		Class to check hidden if the function is hidden in
	 *
	 * @return	true if the function is hidden in the specified class.
	 */
	ENGINE_API bool IsFunctionHiddenFromClass(  const UFunction* InFunction, const UClass* Class );

	/**
	 * Query if a the category a variable belongs to is flagged as hidden from the given class.
	 *
	 * @param	InVariable	Property to check
	 * @param	InClass		Class to check hidden if the function is hidden in
	 *
	 * @return	true if the functions category is hidden in the specified class.
	 */
	ENGINE_API bool IsVariableCategoryHiddenFromClass(  const UProperty* InVariable, const UClass* Class );
};
#endif // WITH_EDITOR

#endif// __ObjectEditorUtils_h__
