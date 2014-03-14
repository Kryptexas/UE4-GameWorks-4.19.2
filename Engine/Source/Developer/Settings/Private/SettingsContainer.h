// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SettingsContainer.h: Declares the FSettingsContainer class.
=============================================================================*/

#pragma once


/**
 * Implements a settings container.
 */
class FSettingsContainer
	: public ISettingsContainer
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InName - The container's name.
	 */
	FSettingsContainer( const FName& InName )
		: Name(InName)
	{ }

public:

	/**
	 * Adds a settings section to the specified settings category (using a settings object).
	 *
	 * If a section with the specified settings objects already exists, the existing section will be replaced.
	 *
	 * @param CategoryName - The name of the category to add the section to.
	 * @param SectionName - The name of the settings section to add.
	 * @param DisplayName - The section's localized display name.
	 * @param Description - The section's localized description text.
	 * @param SettingsObject - The object that holds the section's settings.
	 * @param Delegates - The section's optional callback delegates.
	 *
	 * @return The added settings section, or NULL if the category does not exist.
	 */
	ISettingsSectionPtr AddSection( const FName& CategoryName, const FName& SectionName, const FText& InDisplayName, const FText& InDescription, const TWeakObjectPtr<UObject>& SettingsObject, const FSettingsSectionDelegates& Delegates )
	{
		TSharedPtr<FSettingsCategory> Category = Categories.FindRef(CategoryName);

		if (!Category.IsValid())
		{
			return NULL;
		}

		ISettingsSectionRef Section = Category->AddSection(SectionName, InDisplayName, InDescription, SettingsObject, Delegates);
		CategoryModifiedDelegate.Broadcast(CategoryName);

		return Section;
	}

	/**
	 * Adds a settings section to the specified category (using a custom settings widget).
	 *
	 * If a section with the specified settings objects already exists, the existing section will be replaced.
	 *
	 * @param CategoryName - The name of the category to add the section to.
	 * @param SectionName - The name of the settings section to add.
	 * @param DisplayName - The section's localized display name.
	 * @param Description - The section's localized description text.
	 * @param CustomWidget - A custom settings widget.
	 * @param Delegates - The section's optional callback delegates.
	 *
	 * @return The added settings section, or NULL if the category does not exist.
	 */
	ISettingsSectionPtr AddSection( const FName& CategoryName, const FName& SectionName, const FText& InDisplayName, const FText& InDescription, const TSharedRef<SWidget>& CustomWidget, const FSettingsSectionDelegates& Delegates )
	{
		TSharedPtr<FSettingsCategory> Category = Categories.FindRef(CategoryName);

		if (!Category.IsValid())
		{
			return NULL;
		}

		ISettingsSectionRef Section = Category->AddSection(SectionName, InDisplayName, InDescription, CustomWidget, Delegates);
		CategoryModifiedDelegate.Broadcast(CategoryName);

		return Section;
	}

	/**
	 * Removes a settings section.
	 *
	 * @param CategoryName - The name of the category that contains the section.
	 * @param SectionName - The name of the section to remove.
	 */
	void RemoveSection( const FName& CategoryName, const FName& SectionName )
	{
		TSharedPtr<FSettingsCategory> Category = Categories.FindRef(CategoryName);

		if (Category.IsValid())
		{
			ISettingsSectionPtr Section = Category->GetSection(SectionName);

			if (Section.IsValid())
			{
				Category->RemoveSection(SectionName);
				SectionRemovedDelegate.Broadcast(Section.ToSharedRef());
				CategoryModifiedDelegate.Broadcast(CategoryName);
			}
		}
	}

public:

	// Begin ISettingsContainer interface

	virtual void Describe( const FText& InDisplayName, const FText& InDescription, const FName& InIconName ) OVERRIDE
	{
		Description = InDescription;
		DisplayName = InDisplayName;
		IconName = InIconName;
	}

	virtual void DescribeCategory( const FName& CategoryName, const FText& InDisplayName, const FText& InDescription, const FName& InIconName ) OVERRIDE
	{
		TSharedPtr<FSettingsCategory>& Category = Categories.FindOrAdd(CategoryName);

		if (!Category.IsValid())
		{
			Category = MakeShareable(new FSettingsCategory(CategoryName));
		}

		Category->Describe(InDisplayName, InDescription, InIconName);
		CategoryModifiedDelegate.Broadcast(CategoryName);
	}

	virtual int32 GetCategories( TArray<ISettingsCategoryPtr>& OutCategories ) const OVERRIDE
	{
		OutCategories.Empty(Categories.Num());

		for (TMap<FName, TSharedPtr<FSettingsCategory> >::TConstIterator It(Categories); It; ++It)
		{
			OutCategories.Add(It.Value());
		}

		return OutCategories.Num();
	}

	virtual ISettingsCategoryPtr GetCategory( const FName& CategoryName ) const OVERRIDE
	{
		return Categories.FindRef(CategoryName);
	}

	virtual const FText& GetDescription( ) const OVERRIDE
	{
		return Description;
	}

	virtual const FText& GetDisplayName( ) const OVERRIDE
	{
		return DisplayName;
	}

	virtual const FName& GetIconName( ) const OVERRIDE
	{
		return IconName;
	}

	virtual const FName& GetName( ) const OVERRIDE
	{
		return Name;
	}

	virtual FOnSettingsContainerCategoryModified& OnCategoryModified( ) OVERRIDE
	{
		return CategoryModifiedDelegate;
	}

	virtual FOnSettingsContainerSectionRemoved& OnSectionRemoved( ) OVERRIDE
	{
		return SectionRemovedDelegate;
	}

	// End ISettingsContainer interface

private:

	// Holds the collection of setting categories
	TMap<FName, TSharedPtr<FSettingsCategory> > Categories;

	// Holds the container's description text.
	FText Description;

	// Holds the container's localized display name.
	FText DisplayName;

	// Holds the name of the container's icon.
	FName IconName;

	// Holds the container's name.
	FName Name;

private:

	// Holds a delegate that is executed when a settings category has been added or modified.
	FOnSettingsContainerCategoryModified CategoryModifiedDelegate;

	// Holds a delegate that is executed when a settings section has been removed.
	FOnSettingsContainerSectionRemoved SectionRemovedDelegate;
};
