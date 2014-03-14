// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SettingsCategory.h: Declares the FSettingsCategory class.
=============================================================================*/

#pragma once


/**
 * Implements a settings category.
 */
class FSettingsCategory
	: public TSharedFromThis<FSettingsCategory>
	, public ISettingsCategory
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InName - The category's name.
	 */
	FSettingsCategory( const FName& InName )
		: Name(InName)
	{ }

public:

	/**
	 * Adds a settings section to this settings category.
	 *
	 * If a section with the specified settings objects already exists, the existing section will be returned.
	 *
	 * @param SectionName - The name of the settings section to add.
	 * @param DisplayName - The section's localized display name.
	 * @param Description - The section's localized description text.
	 * @param SettingsObject - The object that holds the section's settings.
	 * @param Delegates - The section's optional callback delegates.
	 *
	 * @return The added settings section.
	 */
	ISettingsSectionRef AddSection( const FName& SectionName, const FText& InDisplayName, const FText& InDescription, const TWeakObjectPtr<UObject>& SettingsObject, const FSettingsSectionDelegates& Delegates )
	{
		TSharedPtr<FSettingsSection>& Section = Sections.FindOrAdd(SectionName);

		if (!Section.IsValid() || (Section->GetSettingsObject() != SettingsObject) || Section->GetCustomWidget().IsValid())
		{
			Section = MakeShareable(new FSettingsSection(AsShared(), SectionName, InDisplayName, InDescription, Delegates, SettingsObject));
		}

		return Section.ToSharedRef();
	}

	/**
	 * Adds a settings section to this settings category.
	 *
	 * If a section with the specified settings objects already exists, the existing section will be returned.
	 *
	 * @param SectionName - The name of the settings section to add.
	 * @param DisplayName - The section's localized display name.
	 * @param Description - The section's localized description text.
	 * @param CustomWidget - A custom settings widget.
	 * @param Delegates - The section's optional callback delegates.
	 *
	 * @return The added settings section.
	 */
	ISettingsSectionRef AddSection( const FName& SectionName, const FText& InDisplayName, const FText& InDescription, const TSharedRef<SWidget>& CustomWidget, const FSettingsSectionDelegates& Delegates )
	{
		TSharedPtr<FSettingsSection>& Section = Sections.FindOrAdd(SectionName);

		if (!Section.IsValid() || (Section->GetSettingsObject() != NULL) || (Section->GetCustomWidget().Pin() != CustomWidget))
		{
			Section = MakeShareable(new FSettingsSection(AsShared(), SectionName, InDisplayName, InDescription, Delegates, CustomWidget));
		}

		return Section.ToSharedRef();
	}

	/**
	 * Updates the details of this settings category.
	 *
	 * @param InDisplayName - The category's localized display name.
	 * @param InDescription - The category's localized description text.
	 * @param InIconName - The name of the category's icon.
	 *
	 * @return The category.
	 */
	void Describe( const FText& InDisplayName, const FText& InDescription, const FName& InIconName )
	{
		Description = InDescription;
		DisplayName = InDisplayName;
		IconName = InIconName;
	}

	/**
	 * Removes a settings section.
	 *
	 * @param SectionName - The name of the section to remove.
	 */
	void RemoveSection( const FName& SectionName )
	{
		Sections.Remove(SectionName);
	}

public:

	// Begin ISettingsCategory interface

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

	virtual ISettingsSectionPtr GetSection( const FName& SectionName ) const OVERRIDE
	{
		return Sections.FindRef(SectionName);
	}

	virtual int32 GetSections( TArray<ISettingsSectionPtr>& OutSections ) const OVERRIDE
	{
		OutSections.Empty(Sections.Num());

		for (TMap<FName, TSharedPtr<FSettingsSection> >::TConstIterator It(Sections); It; ++It)
		{
			OutSections.Add(It.Value());
		}

		return OutSections.Num();
	}

	// End ISettingsCategory interface

private:

	// Holds the category's description text.
	FText Description;

	// Holds the category's localized display name.
	FText DisplayName;

	// Holds the collection of setting sections.
	TMap<FName, TSharedPtr<FSettingsSection> > Sections;

	// Holds the name of the category's icon.
	FName IconName;

	// Holds the category's name.
	FName Name;
};
