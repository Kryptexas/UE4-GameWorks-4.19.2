// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SettingsSection.h: Declares the FSettingsSection class.
=============================================================================*/

#pragma once


/**
 * Implements a project settings section.
 */
class FSettingsSection
	: public ISettingsSection
{
public:

	/**
	 * Creates and initializes a new settings section from the given settings object.
	 *
	 * @param InCategory - The settings category that owns this section.
	 * @param InName - The setting section's name.
	 * @param InDisplayName - The section's localized display name.
	 * @param InDescription - The section's localized description text.
	 * @param InDelegates - The section's optional callback delegates.
	 * @param InSettingsObject - The object that holds the settings for this section.
	 */
	FSettingsSection( const ISettingsCategoryRef& InCategory, const FName& InName, const FText& InDisplayName, const FText& InDescription, const FSettingsSectionDelegates& InDelegates, const TWeakObjectPtr<UObject>& InSettingsObject )
		: Category(InCategory)
		, Description(InDescription)
		, DisplayName(InDisplayName)
		, Name(InName)
		, SettingsObject(InSettingsObject)
		, CanEditDelegate(InDelegates.CanEditDelegate)
		, ExportDelegate(InDelegates.ExportDelegate)
		, ImportDelegate(InDelegates.ImportDelegate)
		, ModifiedDelegate(InDelegates.ModifiedDelegate)
		, ResetDefaultsDelegate(InDelegates.ResetDefaultsDelegate)
		, SaveDefaultsDelegate(InDelegates.SaveDefaultsDelegate)
		, SaveDelegate(InDelegates.SaveDelegate)
		, StatusDelegate(InDelegates.StatusDelegate)
	{ }

	/**
	 * Creates and initializes a new settings section from the given custom settings widget.
	 *
	 * @param InCategory - The settings category that owns this section.
	 * @param InName - The setting section's name.
	 * @param InDisplayName - The section's localized display name.
	 * @param InDescription - The section's localized description text.
	 * @param InDelegates - The section's optional callback delegates.
	 * @param InCustomWidget - A custom settings widget.
	 */
	FSettingsSection( const ISettingsCategoryRef& InCategory, const FName& InName, const FText& InDisplayName, const FText& InDescription, const FSettingsSectionDelegates& InDelegates, const TSharedRef<SWidget>& InCustomWidget )
		: Category(InCategory)
		, CustomWidget(InCustomWidget)
		, Description(InDescription)
		, DisplayName(InDisplayName)
		, Name(InName)
		, CanEditDelegate(InDelegates.CanEditDelegate)
		, ExportDelegate(InDelegates.ExportDelegate)
		, ImportDelegate(InDelegates.ImportDelegate)
		, ModifiedDelegate(InDelegates.ModifiedDelegate)
		, ResetDefaultsDelegate(InDelegates.ResetDefaultsDelegate)
		, SaveDefaultsDelegate(InDelegates.SaveDefaultsDelegate)
		, SaveDelegate(InDelegates.SaveDelegate)
		, StatusDelegate(InDelegates.StatusDelegate)
	{ }

public:

	// Begin ISettingsSection interface

	virtual bool CanEdit( ) const OVERRIDE
	{
		if (CanEditDelegate.IsBound())
		{
			return CanEditDelegate.Execute();
		}

		return true;
	}

	virtual bool CanExport( ) const OVERRIDE
	{
		return (ExportDelegate.IsBound() || (SettingsObject.IsValid() && SettingsObject->GetClass()->HasAnyClassFlags(CLASS_Config)));
	}

	virtual bool CanImport( ) const OVERRIDE
	{
		return (ImportDelegate.IsBound() || (SettingsObject.IsValid() && SettingsObject->GetClass()->HasAnyClassFlags(CLASS_Config)));
	}

	virtual bool CanResetDefaults( ) const OVERRIDE
	{
		return (ResetDefaultsDelegate.IsBound() || (SettingsObject.IsValid() && SettingsObject->GetClass()->HasAnyClassFlags(CLASS_Config) && !SettingsObject->GetClass()->HasAnyClassFlags(CLASS_DefaultConfig)));
	}

	virtual bool CanSaveDefaults( ) const OVERRIDE
	{
		if (SaveDefaultsDelegate.IsBound())
		{
			return true;
		}

		return (SettingsObject.IsValid() && SettingsObject->GetClass()->HasAnyClassFlags(CLASS_Config));
	}

	virtual bool Export( const FString& Filename ) OVERRIDE
	{
		if (ExportDelegate.IsBound())
		{
			return ExportDelegate.Execute(Filename);
		}

		if (SettingsObject.IsValid())
		{
			SettingsObject->SaveConfig(CPF_Config, *Filename);

			return true;
		}

		return false;
	}

	virtual ISettingsCategoryWeakPtr GetCategory( ) OVERRIDE
	{
		return Category;
	}

	virtual TWeakPtr<SWidget> GetCustomWidget( ) const OVERRIDE
	{
		return CustomWidget;
	}

	virtual const FText& GetDescription( ) const OVERRIDE
	{
		return Description;
	}

	virtual const FText& GetDisplayName( ) const OVERRIDE
	{
		return DisplayName;
	}

	virtual const FName& GetName( ) const OVERRIDE
	{
		return Name;
	}

	virtual TWeakObjectPtr<UObject> GetSettingsObject( ) const OVERRIDE
	{
		return SettingsObject;
	}

	virtual FText GetStatus( ) const OVERRIDE
	{
		if (StatusDelegate.IsBound())
		{
			return StatusDelegate.Execute();
		}

		return FText::GetEmpty();
	}

	virtual bool HasDefaultSettingsObject( ) OVERRIDE
	{
		if (!SettingsObject.IsValid())
		{
			return false;
		}

		return SettingsObject->GetClass()->HasAnyClassFlags(CLASS_DefaultConfig);
	}

	virtual bool Import( const FString& Filename ) OVERRIDE
	{
		if (ImportDelegate.IsBound())
		{
			return ImportDelegate.Execute(Filename);
		}

		if (SettingsObject.IsValid())
		{
			SettingsObject->LoadConfig(SettingsObject->GetClass(), *Filename, UE4::LCPF_PropagateToInstances);

			return true;
		}

		return false;	
	}

	virtual bool ResetDefaults( ) OVERRIDE
	{
		if (ResetDefaultsDelegate.IsBound())
		{
			return ResetDefaultsDelegate.Execute();
		}

		if (SettingsObject.IsValid() && SettingsObject->GetClass()->HasAnyClassFlags(CLASS_Config) && !SettingsObject->GetClass()->HasAnyClassFlags(CLASS_DefaultConfig))
		{
			FString ConfigName = SettingsObject->GetClass()->GetConfigName();

			GConfig->EmptySection(*SettingsObject->GetClass()->GetPathName(), ConfigName);
			GConfig->Flush(false);

			FConfigCacheIni::LoadGlobalIniFile(ConfigName, *FPaths::GetBaseFilename(ConfigName), NULL, NULL, true);

			SettingsObject->ReloadConfig(NULL, NULL, UE4::LCPF_PropagateToInstances|UE4::LCPF_PropagateToChildDefaultObjects);

			return true;
		}

		return false;
	}

	virtual bool Save( ) OVERRIDE
	{
		if (ModifiedDelegate.IsBound() && !ModifiedDelegate.Execute())
		{
			return false;
		}

		if (SaveDelegate.IsBound())
		{
			return SaveDelegate.Execute();
		}

		if (!SettingsObject.IsValid())
		{
			return false;
		}

		SettingsObject->SaveConfig();

		return true;
	}

	virtual bool SaveDefaults( ) OVERRIDE
	{
		if (SaveDefaultsDelegate.IsBound())
		{
			return SaveDefaultsDelegate.Execute();
		}

		if (!SettingsObject.IsValid())
		{
			return false;
		}

		SettingsObject->UpdateDefaultConfigFile();
		SettingsObject->ReloadConfig(NULL, NULL, UE4::LCPF_PropagateToInstances);

		return true;
	}

	// End ISettingsSection interface

private:

	// Holds a pointer to the settings category that owns this section.
	ISettingsCategoryWeakPtr Category;

	// Holds the section's custom editor widget.
	TWeakPtr<SWidget> CustomWidget;

	// Holds the section's description text.
	FText Description;

	// Holds the section's localized display name.
	FText DisplayName;

	// Holds the section's name.
	FName Name;

	// Holds the settings object.
	TWeakObjectPtr<UObject> SettingsObject;

private:

	// Holds a delegate that is executed to check whether a settings section can be edited.
	FOnSettingsSectionCanEdit CanEditDelegate;

	// Holds a delegate that is executed when the settings section should be exported to a file.
	FOnSettingsSectionExport ExportDelegate;

	// Holds a delegate that is executed when the settings section should be imported from a file.
	FOnSettingsSectionImport ImportDelegate;

	// Holds a delegate that is executed after the settings section has been modified.
	FOnSettingsSectionModified ModifiedDelegate;

	// Holds a delegate that is executed when the settings section should have its values reset to default.
	FOnSettingsSectionResetDefaults ResetDefaultsDelegate;

	// Holds a delegate that is executed when a settings section should have its values saved as default.
	FOnSettingsSectionSaveDefaults SaveDefaultsDelegate;

	// Holds a delegate that is executed when a settings section should have its values saved.
	FOnSettingsSectionSave SaveDelegate;

	// Holds a delegate that is executed to retrieve a status message for a settings section.
	FOnSettingsSectionStatus StatusDelegate;
};
