// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class UClass;
class SToolTip;

namespace FEditorClassUtils
{

	/**
	 * Gets the tooltip to display for a given class
	 *
	 * @param	InClass		Class we want to build a tooltip for
	 * @return				Shared reference to the constructed tooltip
	 */
	UNREALED_API TSharedRef<SToolTip> GetTooltip(const UClass* Class);

	/**
	 * Returns the link path to the documentation for a given class
	 *
	 * @param	Class		Class we want to build a link for
	 * @return				The path to the documentation for the class
	 */
	UNREALED_API FString GetDocumentationLink(const UClass* Class);

	/**
	 * Creates a link widget to the documentation for a given class
	 *
	 * @param	Class		Class we want to build a link for
	 * @return				Shared pointer to the constructed tooltip
	 */
	UNREALED_API TSharedRef<SWidget> GetDocumentationLinkWidget(const UClass* Class);

	/**
	 * Creates a link to the source code or blueprint for a given class
	 *
	 * @param	Class			Class we want to build a link for
	 * @param	ObjectWeakPtr	Optional object to set blueprint debugging to in the case we are choosing a blueprint
	 * @return					Shared pointer to the constructed tooltip
	 */
	UNREALED_API TSharedRef<SWidget> GetSourceLink(const UClass* Class, const TWeakObjectPtr<UObject> ObjectWeakPtr);
};