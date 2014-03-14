// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// SCreateBlueprintFromActorDialog

class SCreateBlueprintFromActorDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SCreateBlueprintFromActorDialog )
	{}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs, TSharedPtr< SWindow > InParentWindow, bool bInHarvest );

	/** 
	 * Static function to access constructing this window.
	 *
	 * @param bInHarvest		true if the components of the selected actors should be harvested for the blueprint.
	 */
	static KISMETWIDGETS_API void OpenDialog(bool bInHarvest);
private:

	/** 
	 * Will create the blueprint
	 *
	 * @param bInHarvest		true if the components of the selected actors should be harvested for the blueprint.
	 */
	FReply OnCreateBlueprintFromActorClicked( bool bInHarvest);

	/** Callback when the selected blueprint path has changed. */
	void OnSelectBlueprintPath( const FString& Path );

	/** Destroys the window when the operation is cancelled. */
	FReply OnCancelCreateBlueprintFromActor();

	/** Callback when level selection has changed, will destroy the window. */
	void OnLevelSelectionChanged(UObject* InObjectSelected);

	/** Callback when the user changes the filename for the Blueprint */
	void OnFilenameChanged(const FText& InNewName);

	/** Callback to see if creating a blueprint is enabled */
	bool IsCreateBlueprintEnabled() const;
private:
	/** The window this widget is nested in */
	TSharedPtr< SWindow > ParentWindow;

	/** The selected path to create the blueprint */
	FString PathForActorBlueprint;

	/** The resultant actor instance label, based on the original actor labels */
	FString ActorInstanceLabel;

	/** Filename textbox widget */
	TWeakPtr< SEditableTextBox > FileNameWidget;

	/** True if an error is currently being reported */
	bool bIsReportingError;
};
