// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ISettingsCategory.h: Declares the ISettingsCategory interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of ISettingsCategory.
 */
typedef TSharedPtr<class ISettingsCategory> ISettingsCategoryPtr;

/**
 * Type definition for shared references to instances of ISettingsCategory.
 */
typedef TSharedRef<class ISettingsCategory> ISettingsCategoryRef;


/**
 * Interface for setting categories.
 *
 * A setting category is a collection of setting sections that logically belong together
 * (i.e. all project settings or all platform settings). Each section then contains the
 * actual settings in the form of UObject properties.
 */
class ISettingsCategory
{
public:

	/**
	 * Gets the category's localized description text.
	 *
	 * @return Description text.
	 */
	virtual const FText& GetDescription( ) const = 0;

	/**
	 * Gets the category's localized display name.
	 *
	 * @return Display name.
	 */
	virtual const FText& GetDisplayName( ) const = 0;

	/**
	 * Gets the settings section with the specified name.
	 *
	 * @param SectionName - The name of the section to get.
	 *
	 * @return The settings section, or NULL if it doesn't exist.
	 */
	virtual ISettingsSectionPtr GetSection( const FName& SectionName ) const = 0;

	/**
	 * Gets the setting sections contained in this category.
	 *
	 * @param OutSections - Will hold the collection of sections.
	 *
	 * @return The number of sections returned.
	 */
	virtual int32 GetSections( TArray<ISettingsSectionPtr>& OutSections ) const = 0;

	/**
	 * Gets the name of the category's icon.
	 *
	 * @return Icon image name.
	 */
	virtual const FName& GetIconName( ) const = 0;

	/**
	 * Gets the category's name.
	 *
	 * @return Category name.
	 */
	virtual const FName& GetName( ) const = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~ISettingsCategory( ) { }
};
