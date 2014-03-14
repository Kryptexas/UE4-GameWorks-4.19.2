// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

struct FParentClassItem;

/**
 * A dialog to choose a new class parent and name
 */
class SNewClassDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SNewClassDialog )
		: _Class(NULL)
	{}

	/** The class we want to build our new class from. If this is not specified then the wizard will display classes to the user. */
	SLATE_ARGUMENT(UClass*, Class)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs );

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;

private:

	/** Creates a row in the parent class list */
	TSharedRef<ITableRow> MakeParentClassListViewWidget(TSharedPtr<FParentClassItem> ParentClassItem, const TSharedRef<STableViewBase>& OwnerTable);

	/** Gets the currently selected parent class name */
	FString GetSelectedParentClassName() const;

	/** Handler for when a parent class item is double clicked */
	void OnParentClassItemDoubleClicked( TSharedPtr<FParentClassItem> TemplateItem );

	/** Handler for when a class is selected in the parent class list */
	void OnClassSelected(TSharedPtr<FParentClassItem> Item, ESelectInfo::Type SelectInfo);

	/** Handler for when a class was picked in the full class tree */
	void OnAdvancedClassSelected(UClass* Class);

	/** Gets the check box state for the full class list */
	ESlateCheckBoxState::Type IsFullClassTreeChecked() const;

	/** Gets the check box state for the full class list */
	void OnFullClassTreeChanged(ESlateCheckBoxState::Type NewCheckedState);

	/** Gets the visibility of the basic class list */
	EVisibility GetBasicParentClassVisibility() const;

	/** Gets the visibility of the full class list */
	EVisibility GetAdvancedParentClassVisibility() const;

	/** Gets the visibility of the name error label */
	EVisibility GetNameErrorLabelVisibility() const;

	/** Gets the text to display in the name error label */
	FString GetNameErrorLabelText() const;

	/** Gets the visibility of the global error label */
	EVisibility GetGlobalErrorLabelVisibility() const;

	/** Gets the visibility of the global error label IDE Link */
	EVisibility GetGlobalErrorLabelIDELinkVisibility() const;

	/** Gets the text to display in the global error label */
	FString GetGlobalErrorLabelText() const;

	/** Handler for when the user enters the "name class" page */
	void OnNamePageEntered();

	/** Returns the title text for the "name class" page */
	FString GetNameClassTitle() const;

	/** Returns the text in the class name edit box */
	FText OnGetClassNameText() const;

	/** Handler for when the text in the class name edit box has changed */
	void OnClassNameTextChanged(const FText& NewText);

	/** Handler for when cancel is clicked */
	void CancelClicked();

	/** Returns true if Finish is allowed */
	bool CanFinish() const;

	/** Handler for when finish is clicked */
	void FinishClicked();

	/** Handler for when the error label IDE hyperlink was clicked */
	void OnDownloadIDEClicked(FString URL);

private:
	/** Checks the current class name for validity and updates cached values accordingly */
	void UpdateClassNameValidity();

	/** Gets the currently selected parent class */
	const UClass* GetSelectedParentClass() const;

	/** Adds parent classes to the ParentClassListView source */
	void SetupParentClassItems();

	/** Closes the window that contains this widget */
	void CloseContainingWindow();

private:

	/** The wizard widget */
	TSharedPtr<SWizard> MainWizard;
	
	/** ParentClass items */
	TSharedPtr< SListView< TSharedPtr<FParentClassItem> > > ParentClassListView;
	TArray< TSharedPtr<FParentClassItem> > ParentClassItemsSource;

	/** A pointer to a class viewer **/
	TSharedPtr<class SClassViewer> ClassViewer;

	/** The editable text box to enter the current name */
	TSharedPtr<SEditableTextBox> ClassNameEditBox;

	/** The name of the class being created */
	FString NewClassName;

	/** The selected parent class */
	TWeakObjectPtr<UClass> ParentClass;

	/** The fixed width of the dialog */
	int32 DialogFixedWidth;

	/** If true, the full class tree will be shown in the parent class selection */
	bool bShowFullClassTree;

	/** The last time that the class name was checked for validity. This is used to throttle I/O requests to a reasonable frequency */
	double LastNameValidityCheckTime;

	/** The frequency in seconds for validity checks while the dialog is idle. Changes to the path immediately update the validity. */
	double NameValidityCheckFrequency;

	/** Periodic checks for validity will not occur while this flag is true. Used to prevent a frame of "this project already exists" while exiting after a successful creation. */
	bool bPreventPeriodicValidityChecksUntilNextChange;

	/** The error text from the last validity check */
	FText LastNameValidityErrorText;

	/** True if the last validity check returned that the class name is valid for creation */
	bool bLastNameValidityCheckSuccessful;
};
