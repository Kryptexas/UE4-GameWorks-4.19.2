// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

DECLARE_LOG_CATEGORY_EXTERN(LogPluginWizard, Log, All);

/**
 * Description of a plugin template
 */
struct FPluginTemplateDescription
{
	/** Name of this template in the GUI */
	FText Name;

	/** Description of this template in the GUI */
	FText Description;

	/** Name of the directory containing template files */
	FString OnDiskPath;

	/** Whether this template needs the UI files generated (commands, etc...) */
	bool bIncludeUI;

	/** Whether this template creates an editor mode */
	bool bMakeEditorMode;

	/** Whether this template creates a blueprint function library */
	bool bAddBPLibrary;

	/** Brush resource for the image that is dynamically loaded */
	TSharedPtr< FSlateDynamicImageBrush > PluginIconDynamicImageBrush;

	/** Constructor */
	FPluginTemplateDescription(FText InName, FText InDescription, FString InOnDiskPath, bool bInIncludeUI, bool bInMakeEditorMode, bool bInAddBPLibrary)
		: Name(InName)
		, Description(InDescription)
		, OnDiskPath(InOnDiskPath)
		, bIncludeUI(bInIncludeUI)
		, bMakeEditorMode(bInMakeEditorMode)
		, bAddBPLibrary(bInAddBPLibrary)
	{
	}
};

/**
 * A wizard to create a new plugin
 */
class SNewPluginWizard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNewPluginWizard){}
	SLATE_END_ARGS()

	/** Constructor */
	SNewPluginWizard();

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, TSharedPtr<SDockTab> InOwnerTab);

private:
	/**
	 * Called to generate a widget for the specified list item
	 * @param Item The template information for this row
	 * @param OwnerTable The table that owns these rows
	 * @return The widget for this template
	 */
	TSharedRef<ITableRow> OnGenerateTemplateRow(TSharedRef<FPluginTemplateDescription> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/**
	 * Called when selected template changes
	 * @param InItem The selected template
	 * @param SelectInfo How the item was selected
	 */
	void OnTemplateSelectionChanged( TSharedPtr<FPluginTemplateDescription> InItem, ESelectInfo::Type SelectInfo );

	/**
	 * Called when Plugin Name textbox changes value
	 * @param InText The new Plugin name text
	 */
	void OnPluginNameTextChanged(const FText& InText);

	/**
	 * Whether we are currently able to create a plugin
	 */
	bool CanCreatePlugin() const;

	/**
	 * Get the path where we will create a plugin
	 */
	FText GetPluginDestinationPath() const;

	/**
	 * Whether we will create a plugin in the engine directory
	 */
	ECheckBoxState IsEnginePlugin() const;

	/**
	 * Called when state of Engine plugin checkbox changes
	 * @param NewCheckedState New state of the checkbox
	 */
	void OnEnginePluginCheckboxChanged(ECheckBoxState NewCheckedState);

	/**
	 * This is where all the magic happens.
	 * Create actual plugin using parameters collected from other widgets
	 */
	FReply OnCreatePluginClicked();

	/**
	 * Helper function to create directory under given path
	 * @param InPath - path to a directory that you want to create
	 */
	bool MakeDirectory(const FString& InPath);

	/**
	 * Copies a file and adds to a list of created files
	 * @param DestinationFile Where the file will be copied
	 * @param SourceFile Original file to copy
	 * @param InOutCreatedFiles Array of created files to add to
	 * @return Whether the file was copied successfully
	 */
	bool CopyFile(const FString& DestinationFile, const FString& SourceFile, TArray<FString>& InOutCreatedFiles);

	/**
	 * Writes a plugin descriptor file to disk
	 * @param PluginModuleName Name of the plugin and its module
	 * @param UPluginFilePath Path where the descriptor file should be written
	 * @return Whether the files was written successfully
	 */
	bool WritePluginDescriptor(const FString& PluginModuleName, const FString& UPluginFilePath);

	/**
	 * Displays an editor pop up error notification
	 * @param ErrorMessage Message to display in the pop up
	 */
	void PopErrorNotification(const FText& ErrorMessage);

	/**
	 * Helper function to delete given directory
	 * @param InPath - full path to directory that you want to remove
	 */
	void DeletePluginDirectory(const FString& InPath);

private:
	/** List of known templates */
	TArray<TSharedRef<FPluginTemplateDescription>> Templates;

	/** Currently selected template */
	TSharedPtr<FPluginTemplateDescription> CurrentTemplate;

	/** Name of the plugin you want to create*/
	FText PluginNameText;

	/** Textbox widget that user will put plugin name in */
	TSharedPtr<SEditableTextBox> PluginNameTextBox;

	/** Whether the name of the plugin entered is currently valid */
	bool bIsPluginNameValid;

	/** Whether we want to create a plugin in the engine folder */
	bool bIsEnginePlugin;

	/** Tab that owns this wizard so that we can ask to close after completion */
	TWeakPtr<SDockTab> OwnerTab;
};
