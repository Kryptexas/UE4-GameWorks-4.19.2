// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Implements details panel customizations for UConfigPropertyHelper fields.
 */
class FConfigPropertyHelperDetails
	: public IDetailCustomization
{
	//////////////////////////////////////////////////////////////////////////
	/// Detail Customization
public:

	/**
	 * Makes a new instance of this config editor detail layout class.
	 *
	 * @return The created instance.
	 */
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FConfigPropertyHelperDetails);
	}

	// IDetailCustomization interface
	virtual void CustomizeDetails( IDetailLayoutBuilder& InDetailBuilder ) override;


	//////////////////////////////////////////////////////////////////////////
	/// Property Table functionality
private:

	/**
	 * Create a property table of the Config Files vs Property we are editing
	 *
	 * @return The created instance of the property table.
	 */
	TSharedRef<SWidget> ConstructPropertyTable(IDetailLayoutBuilder& DetailBuilder);

	/**
	 * Populate the Property table with entries for the provided config files.
	 */
	void RepopulatePropertyTable(IDetailLayoutBuilder& DetailBuilder);

	/**
	 * Event which is triggered through changes in our editor.
	 *
	 * @param Object				- The object whose property has changed.
	 * @param PropertyChangedEvent	- The event information of the change.
	 */
	void OnPropertyValueChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);

private:
	
	// The table which holds our editable properties.
	TSharedPtr<IPropertyTable> PropertyTable;


	//////////////////////////////////////////////////////////////////////////
	/// Misc operations...
private:
	void AddEditablePropertyForConfig(IDetailLayoutBuilder& DetailBuilder, const class UPropertyConfigFileDisplayRow* ConfigFileProperty);

	void OnConfigFileListChanged();

	//////////////////////////////////////////////////////////////////////////
	/// Misc properties...
private:

	// Keep track of the property handle for the config files.
	TSharedPtr<IPropertyHandle> ConfigFilesHandle;

	// The property we wish to edit on a per config basis.
	UProperty* Property;

	// Coupling of Config files and their editable objects.
	TMap<FString, UObject*> AssociatedConfigFileAndObjectPairings;

	TMap<FString, UObject*> ConfigFileAndPropertySourcePairings;

	TMap<FString, TSharedPtr<SWidget>> ExternalPropertyValueWidgetAndConfigPairings;
};
