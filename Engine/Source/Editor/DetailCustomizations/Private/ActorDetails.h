// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FActorDetails : public IDetailCustomization
{
public:
	virtual ~FActorDetails();

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailLayout ) OVERRIDE;
private:
	/** Delegate that returns an icon */
	DECLARE_DELEGATE_RetVal(const FSlateBrush*, FGetBrushImage);

	/** Delegate that returns a tooltip or other text field */
	DECLARE_DELEGATE_RetVal(FText, FGetText);

	/**
	 * Returns the conversion root class of the passed in class
	 *
	 * @param InCurrentClass		The class to find the conversion root of
	 *
	 * @return						Going up the class hierarchy, the first class found to have "IsConversionRoot" metadata (NULL if none is found)
	 */
	UClass* GetConversionRoot( UClass* InCurrentClass ) const;

	/**
	 * Callback from the Class Picker when a class is picked.
	 *
	 * @param ChosenClass		The class chosen to convert to
	 */
	void OnConvertActor(UClass* ChosenClass);

	/** 
	 * Creates the filters for displaying valid classes to convert to
	 *
	 * @param ConvertActor			The actor being converted
	 * @param ClassPickerOptions	Options for the Class Picker being generated, to be given a newly created filter
	 */
	void CreateClassPickerConvertActorFilter(const TWeakObjectPtr<AActor> ConvertActor, class FClassViewerInitializationOptions* ClassPickerOptions);

	/** Retrieves the content for the Convert combo button */
	TSharedRef<SWidget> OnGetConvertContent();

	/** Returns the visibility of the Convert menu */
	EVisibility GetConvertMenuVisibility() const;

	/** Create the Convert combo button */
	TSharedRef<SWidget> MakeConvertMenu( const FSelectedActorInfo& SelectedActorInfo );

	void OnNarrowSelectionSetToSpecificLevel( TWeakObjectPtr<ULevel> LevelToNarrowInto );

	bool IsActorValidForLevelScript() const;

	FReply FindSelectedActorsInLevelScript();

	bool AreAnySelectedActorsInLevelScript() const;

	/** Util to create a menu for events we can add for the selected actor */
	TSharedRef<SWidget> MakeEventOptionsWidgetFromSelection();

	void AddActorCategory( IDetailLayoutBuilder& DetailBuilder, const TMap<ULevel*, int32>& ActorsPerLevelCount );

	void AddBlueprintCategory( IDetailLayoutBuilder& DetailBuilder, const TMap<UBlueprint*, UObject*>& UniqueBlueprints );

	void AddCodeViewCategory( IDetailLayoutBuilder& DetailBuilder );

	void AddComponentsCategory( IDetailLayoutBuilder& DetailBuilder );

	void AddLayersCategory( IDetailLayoutBuilder& DetailBuilder );

	void AddTransformCategory( IDetailLayoutBuilder& DetailBuilder );

	/**
	 * Creates a category for displaying materials used by the selected actors
	 */
	void AddMaterialCategory( IDetailLayoutBuilder& DetailBuilder );

	const TArray< TWeakObjectPtr<AActor> >& GetSelectedActors() const;

	/** Creates an area meant to be displayed once for any number of selected blueprints that displays helper buttons. */
	void AddUtilityBlueprintRows( IDetailCategoryBuilder& BlueprintCategory );

	// Creates a row for a single blueprint in the selection details view   
	void AddSingleBlueprintRow( IDetailCategoryBuilder& BlueprintCategory, UBlueprint* InBlueprint, UObject* InExampleActor);

	FText GetBlueprintStatusTooltip( TWeakObjectPtr<class UBlueprint> Asset ) const;

	const FSlateBrush* GetBlueprintStatusIcon(TWeakObjectPtr<class UBlueprint> Asset) const;

	const FSlateBrush* GetBlueprintBreakpointStatusIcon(TWeakObjectPtr<class UBlueprint> Asset) const;

	FReply OnCompileBlueprintClicked(TWeakObjectPtr<class UBlueprint> Asset);

	FReply OnToggleBreakpointsClicked(TWeakObjectPtr<class UBlueprint> Asset);

	FReply OnEditBlueprintClicked( TWeakObjectPtr<UBlueprint> InBlueprint, TWeakObjectPtr<UObject> InAsset );

	bool PushToBlueprintDefaults_IsEnabled( TWeakObjectPtr<UBlueprint> InBlueprint ) const;
	FReply PushToBlueprintDefaults_OnClicked( TWeakObjectPtr<UBlueprint> InBlueprint );
	FText PushToBlueprintDefaults_ToolTipText( TWeakObjectPtr<UBlueprint> InBlueprint ) const;

	bool ResetToBlueprintDefaults_IsEnabled( TWeakObjectPtr<UBlueprint> InBlueprint ) const;
	FReply ResetToBlueprintDefaults_OnClicked( TWeakObjectPtr<UBlueprint> InBlueprint );
	FText ResetToBlueprintDefaults_ToolTipText( TWeakObjectPtr<UBlueprint> InBlueprint ) const;
private:

	/** Bring up the menu for user to select the path to create blueprint at */
	FReply OnPickBlueprintPathClicked(bool bHavest);

	/** Bring up the menu for user to select the path to create blueprint at */
	void   OnSelectBlueprintPath(const FString& );

	/** The path the user has selected to create a blueprint at*/
	FString PathForActorBlueprint;

	TArray< TWeakObjectPtr<AActor> > SelectedActors;

	TSharedPtr< IDetailCustomization > LayersLayoutDetails;

	/** The material category that displays used materials */
	TSharedPtr< class FActorMaterialCategory > MaterialCategory;
};
