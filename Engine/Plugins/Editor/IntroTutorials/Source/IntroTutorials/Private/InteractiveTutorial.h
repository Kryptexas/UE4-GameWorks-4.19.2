// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

DECLARE_DELEGATE( FOnBeginExcerptDelegate );
DECLARE_DELEGATE_RetVal(bool, FShouldSkipExcerptDelegate);

DECLARE_DELEGATE_RetVal_OneParam(bool, FActorSelectionChangedTrigger, const TArray<UObject*>&);
DECLARE_DELEGATE_RetVal_OneParam(bool, FObjectPropertyChangedTrigger, UObject*);
DECLARE_DELEGATE_RetVal_OneParam(bool, FAssetEditorRequestedOpenTrigger, UObject*);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FEditorModeChangedTrigger, FEdMode*, bool);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FEditorModeChangedTrigger, FEdMode*, bool);
DECLARE_DELEGATE_RetVal_FourParams(bool, FOnEditorCameraMoved, const FVector&, const FRotator&, ELevelViewportType, int32 );
DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnEditorCameraZoom, const FVector&, int32 );
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnBeginPIETrigger, bool);
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnEndPIETrigger, bool);
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnNewAssetCreated, UFactory*);
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnNewBlueprintClassPicked, UClass*);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnNewActorsDropped, const TArray<UObject*>&, const TArray<AActor*>&);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnGridSnappingChanged, bool, float);
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnEndTransformObject, UObject&);
DECLARE_DELEGATE_RetVal(bool, FOnLightingBuildStarted);
DECLARE_DELEGATE_RetVal(bool, FOnLightingBuildKept);
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnStartedPlacing, const TArray<UObject*>&);
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnSelectionChanged, UObject*);
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnSelectObject, UObject*);
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnWidgetModeChanged, FWidget::EWidgetMode);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnDropAssetOnActor, UObject*, AActor*);
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnFocusViewportOnActors, const TArray<AActor*>&);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnMapOpened, const FString&, bool);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnCBFilterChanged, const struct FARFilter&, bool);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnCBSearchBoxChanged, const FText&, bool);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnCBAssetSelectionChanged, const TArray<class FAssetData>&, bool);
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnCBSourcesViewChanged, bool);
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnCBAssetPathChanged, const FString&);


/** Struct for handling all the various different kinds of delegate events */
struct FBaseTriggerDelegate
{
	enum Type
	{
		ActorSelectionChanged,
		ObjectPropertyChanged,
		AssetEditorRequestedOpen,
		EditorModeChanged,
		EditorCameraMoved,
		EditorCameraZoomed,
		OnBeginPIE,
		OnEndPIE,
		OnNewAssetBeginConfiguration,
		OnNewAssetCreated,
		OnNewBlueprintClassPicked,
		OnNewActorsDropped,
		OnGridSnappingChanged,
		OnEndTransformObject,
		OnLightingBuildStarted,
		OnLightingBuildKept,
		OnStartedPlacing,
		OnSelectionChanged,
		OnSelectObject,
		OnWidgetModeChanged,
		OnDropAssetOnActor,
		OnFocusViewportOnActors,
		OnMapOpened,
		OnCBFilterChanged,
		OnCBSearchBoxChanged,
		OnCBAssetSelectionChanged,
		OnCBSourcesViewChanged,
		OnCBAssetPathChanged,
	};
	FBaseTriggerDelegate::Type DelegateType;

	FBaseTriggerDelegate(FBaseTriggerDelegate::Type InDelegateType)
		: DelegateType(InDelegateType) {}
};

/** Derived classes for all the different kinds of delegate events */
template<typename DelegateSignature>
struct FDerivedTriggerDelegate : public FBaseTriggerDelegate
{
	FDerivedTriggerDelegate(FBaseTriggerDelegate::Type InDelegateType, DelegateSignature InDelegate)
		: FBaseTriggerDelegate(InDelegateType)
		, Delegate(InDelegate) {}

	DelegateSignature Delegate;
};

/** Enum to define visual style */
namespace ETutorialStyle
{
	enum Type
	{
		Default,
		Home		
	};
}

/**
 * A class encapsulating the interactivity data for a tutorial
 */
class FInteractiveTutorial : public TSharedFromThis<FInteractiveTutorial>
{
public:
	FInteractiveTutorial()
		: TutorialStyle(ETutorialStyle::Default)
	{}

	/** Sets the currently viewed excerpt  */
	void SetCurrentExcerpt(const FString& NewExcerpt);

	/** Gets the currently viewed excerpt  */
	const FString& GetCurrentExcerpt() const;

	/** Determines if the passed in excerpt is interactive or not */
	bool IsExcerptInteractive(const FString& ExcerptName);
	
	/** Decides if the "next" button should be active on the given except */
	bool CanManuallyAdvanceExcerpt(const FString& ExcerptName) const;

	/** Decides if the "back" button should be active on the given except */
	bool CanManuallyReverseExcerpt(const FString& ExcerptName) const;

	/** Call to disable the "next" button until after the excerpt auto-advances. */
	void DisableManualAdvanceUntilTriggered(const FString& ExcerptName)
	{
		ExcerptsWithDisabledAdvancement.AddUnique(ExcerptName);
	}

	/** Adds in a trigger delegate to watch out for in the tutorial */
	template<typename DelegateSignature>
	void AddTriggerDelegate(const FString& ExcerptName, FBaseTriggerDelegate::Type DelegateType, DelegateSignature InDelegate)
	{
		TriggerExcerpts.Add(ExcerptName, MakeShareable(new FDerivedTriggerDelegate<DelegateSignature>(DelegateType, InDelegate)));
	}

	/** Adds some dialogue audio to an excerpt */
	void AddDialogueAudio(const FString& ExcerptName, const FString& AudioAssetPathName)
	{
		ExcerptDialogue.Add(ExcerptName, AudioAssetPathName);
	}

	/** Find (possibly load) dialogue for this excerpt */
	UDialogueWave* FindDialogueAudio(const FString& ExcerptName);

	/** Set visual style to use */
	void SetTutorialStyle(ETutorialStyle::Type InStyle)
	{
		TutorialStyle = InStyle;
	}

	/** Get visual style used */
	ETutorialStyle::Type GetTutorialStyle()
	{
		return TutorialStyle;
	}

	/** FInd the trigger delegate for the current excerpt */
	TSharedPtr<FBaseTriggerDelegate> FindCurrentTrigger() const;

	/** Adds in a widget to highlight during the tutorial */
	void AddHighlightWidget(const FString& InExcerptName, const FName& InWidgetName);

	/** Check whether a particular widget should be highlighted */
	bool IsWidgetHighlighted(const FName& InWidgetName) const;

	/** Add a delegate to an excerpt to see if we should skip it */
	void AddSkipDelegate(const FString& InExcerptName, FShouldSkipExcerptDelegate InDelegate)
	{
		ExcerptSkipDelegates.Add(InExcerptName, InDelegate);
	}

	/** Add an "OnBegin" delegate to an excerpt */
	void AddBeginExcerptDelegate(const FString& InExcerptName, FOnBeginExcerptDelegate InDelegate)
	{
		BeginExcerptDelegates.Add(InExcerptName, InDelegate);
	}

	/** Find the skip delegate for the current excerpt */
	const FShouldSkipExcerptDelegate* FindSkipDelegate(const FString& InExcerptName) const;

	void OnExcerptCompleted(const FString& InExcerpt);


private:

	/** List of excerpts whose comletion triggers have fired successfully */
	TArray<FString> CompletedExcerpts;

	/** Maps of excerpt names to trigger delegates */
	TMap<FString, TSharedPtr<FBaseTriggerDelegate>> TriggerExcerpts;

	/** Map excerpt names to highlighted widget names */
	TMultiMap<FString, FName> ExcerptWidgets;

	/** Map excerpt name to 'skip' delegate */
	TMap<FString, FShouldSkipExcerptDelegate> ExcerptSkipDelegates;

	/** Map excerpt name to 'OnBegin' delegate */
	TMap<FString, FOnBeginExcerptDelegate> BeginExcerptDelegates;

	/** Map excerpt name to dialogue to play */
	TMap<FString, FString> ExcerptDialogue;

	/** List of excerpts with Next button disabled until auto-advance. */
	TArray<FString> ExcerptsWithDisabledAdvancement;
	
	/** The current section's excerpt that we will check against */
	FString CurrentExcerpt;

	/** The example map for this tutorial */
	FString ExampleMap;

	/** Style to use for this tutorial */
	ETutorialStyle::Type TutorialStyle;
};
