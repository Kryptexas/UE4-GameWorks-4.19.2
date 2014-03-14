// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


struct FTemplateItem;


/**
 * A wizard to create a new game project
 */
class SNewProjectWizard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SNewProjectWizard ){}

		SLATE_EVENT( FOnClicked, OnBackRequested )

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs );

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;

private:

	/** Construct the folder diagram */
	TSharedRef<SWidget> ConstructDiagram();

	/** Gets the image to display for the specified template */
	const FSlateBrush* GetTemplateItemImage(TWeakPtr<FTemplateItem> TemplateItem) const;

	/** Accessor for the currently selected template item */
	TSharedPtr<FTemplateItem> GetSelectedTemplateItem() const;

	/** Accessor for the project name text */
	FText GetCurrentProjectFileName() const;

	/** Accessor for the project name string */
	FString GetCurrentProjectFileNameString() const;

	/** Accessor for the project filename string with the extension */
	FString GetCurrentProjectFileNameStringWithExtension() const;

	/** Handler for when the project name string was changed */
	void OnCurrentProjectFileNameChanged(const FText& InValue);

	/** Accessor for the project path text */
	FText GetCurrentProjectFilePath() const;

	/** Accessor for the project ParentFolder string */
	FString GetCurrentProjectFileParentFolder() const;

	/** Handler for when the project path string was changed */
	void OnCurrentProjectFilePathChanged(const FText& InValue);

	/** Accessor for the label to preview the current filename with path */
	FString GetProjectFilenameWithPathLabelText() const;

	/** Accessor for the label to show the currently selected template */
	FText GetSelectedTemplateName() const;

	/** Gets the assembled project filename with path */
	FString GetProjectFilenameWithPath() const;

	/** Opens a web browser to the specified IDE url */
	void OnDownloadIDEClicked(FString URL);

	/** Returns true if the user is allowed to specify a project with the supplied name and path */
	bool IsCreateProjectEnabled() const;

	// Handles checking whether the specified page can be shown.
	bool HandlePageCanShow( FName PageName ) const;

	/** Fired when the page changes in the wizard */
	void OnPageVisited(FName NewPageName);

	/** Gets the visibility of the error label */
	EVisibility GetGlobalErrorLabelVisibility() const;

	/** Gets the visibility of the error label close button. Only visible when displaying a persistent error */
	EVisibility GetGlobalErrorLabelCloseButtonVisibility() const;

	/** Gets the visibility of the IDE link in the error label */
	EVisibility GetGlobalErrorLabelIDELinkVisibility() const;

	/** Gets the text to display in the error label */
	FText GetGlobalErrorLabelText() const;

	/** Handler for when the close button on the error label is clicked */
	FReply OnCloseGlobalErrorLabelClicked();

	/** Gets the visibility of the error label */
	EVisibility GetNameAndLocationErrorLabelVisibility() const;

	/** Gets the text to display in the error label */
	FText GetNameAndLocationErrorLabelText() const;

	/** Handler for when the "copy starter content" checkbox changes state */
	void OnCopyStarterContentChanged(ESlateCheckBoxState::Type InNewState);

	/** Returns true if the "copy starter content" checkbox should be checked. */
	ESlateCheckBoxState::Type IsCopyStarterContentChecked() const;

	/** Gets the visibility of the copy starter content option */
	EVisibility GetCopyStarterContentCheckboxVisibility() const;

private:

	/** Populates TemplateItemsSource with templates found on disk */
	void FindTemplateProjects();

	/** Sets the default project name and path */
	void SetDefaultProjectLocation();

	/** Checks the current project path an name for validity and updates cached values accordingly */
	void UpdateProjectFileValidity();

	/** Returns true if we have a code template selected */
	bool IsCompilerRequired() const;

	/** Begins the creation process for the configured project */
	void CreateAndOpenProject();

	/** Opens the specified project file */
	bool OpenProject(const FString& ProjectFile, bool bPromptForConfirmation);

	/** Opens the solution for the specified project */
	bool OpenCodeIDE(const FString& ProjectFile, bool bPromptForConfirmation);

	/** Creates a project with the supplied project filename */
	bool CreateProject(const FString& ProjectFile);

	/** Closes the containing window, but only if summoned via the editor so the non-game version doesn't just close to desktop. */
	void CloseWindowIfAppropriate( bool ForceClose = false );

	/** Displays an error to the user */
	void DisplayError(const FText& ErrorText);

private:

	/** Handler for when the Browse button is clicked */
	FReply HandleBrowseButtonClicked( );

	/** Checks whether the 'Create Project' wizard can finish. */
	bool HandleCreateProjectWizardCanFinish( ) const;

	/** Handler for when the Create button is clicked */
	void HandleCreateProjectWizardFinished( );

	/** Creates a row in the template list */
	TSharedRef<ITableRow> HandleTemplateListViewGenerateRow( TSharedPtr<FTemplateItem> TemplateItem, const TSharedRef<STableViewBase>& OwnerTable );

	/** Called when a user double-clicks a template item in the template project list. */
	void HandleTemplateListViewDoubleClick( TSharedPtr<FTemplateItem> TemplateItem );

	/** Handler for when the selection changes in the template list */
	void HandleTemplateListViewSelectionChanged( TSharedPtr<FTemplateItem> TemplateItem, ESelectInfo::Type SelectInfo );

private:
	/* @return Whether the expanded items are currently visible, based on the current expander state */
	EVisibility GetExpandedItemsVisibility() const;

	/* @return The scale to apply to the expanded items, based on the rollout curve value */
	FVector2D GetExpandedItemsScale() const;

	/* @return The brush to use for the expander icon, based on the current expander state */
	const FSlateBrush* GetExpanderIcon() const;

	/* Handle the expander button being clicked (toggles the expansion state) */
	FReply OnExpanderClicked();

private:

	/** The wizard widget */
	TSharedPtr<SWizard> MainWizard;

	FString LastBrowsePath;
	FString CurrentProjectFileName;
	FString CurrentProjectFilePath;
	FText PersistentGlobalErrorLabelText;

	/** The last time that the selected project file path was checked for validity. This is used to throttle I/O requests to a reasonable frequency */
	double LastValidityCheckTime;

	/** The frequency in seconds for validity checks while the dialog is idle. Changes to the path immediately update the validity. */
	double ValidityCheckFrequency;

	/** Periodic checks for validity will not occur while this flag is true. Used to prevent a frame of "this project already exists" while exiting after a successful creation. */
	bool bPreventPeriodicValidityChecksUntilNextChange;

	/** The global error text from the last validity check */
	FText LastGlobalValidityErrorText;

	/** The global error text from the last validity check */
	FText LastNameAndLocationValidityErrorText;

	/** True if the last global validity check returned that the project path is valid for creation */
	bool bLastGlobalValidityCheckSuccessful;

	/** True if the last NameAndLocation validity check returned that the project path is valid for creation */
	bool bLastNameAndLocationValidityCheckSuccessful;

	/** True if the items are currently expanded */
	bool bItemsAreExpanded;

	/** Curved used to simulate a rollout of the section */
	FCurveSequence ExpanderRolloutCurve;

	/** The name of the page that is currently in view */
	FName CurrentPageName;

	/** True if user has selected to copy starter content. */
	bool bCopyStarterContent;

	/** True if starter content is available to be copied into new projects. */
	bool bStarterContentIsAvailable;

	/** Template items */
	TArray<TSharedPtr<FTemplateItem> > TemplateList;
	TSharedPtr<SListView<TSharedPtr<FTemplateItem> > > TemplateListView;

	/** Names for pages */
	static FName TemplatePageName;
	static FName NameAndLocationPageName;
};
