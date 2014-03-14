// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "InteractiveTutorials.h"
#include "InteractivityData.h"
#include "K2Node_TutorialExcerptComplete.h"
#include "IntroTutorials.generated.inl"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Editor/UnrealEd/Public/Toolkits/AssetEditorManager.h"
#include "Editor/PlacementMode/Public/IPlacementModeModule.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"

#include "Editor.h"

#define LOCTEXT_NAMESPACE "IntroTutorials"



FInteractiveTutorials::FInteractiveTutorials(FSimpleDelegate SectionCompleted)
	: ExcerptConditionsCompleted(SectionCompleted)
{
}

void FInteractiveTutorials::SetupEditorHooks()
{
	// Set up editor wide hooks first
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>( "LevelEditor" );
	LevelEditorModule.OnActorSelectionChanged().AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FActorSelectionChangedTrigger, FBaseTriggerDelegate::ActorSelectionChanged>);

	USelection::SelectionChangedEvent.AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnSelectionChanged, FBaseTriggerDelegate::OnSelectionChanged>);
	USelection::SelectObjectEvent.AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnSelectObject, FBaseTriggerDelegate::OnSelectObject>);

	FCoreDelegates::OnObjectPropertyChanged.Add( FCoreDelegates::FOnObjectPropertyChanged::FDelegate::CreateSP( this, &FInteractiveTutorials::OnDelegateTriggered<FObjectPropertyChangedTrigger, FBaseTriggerDelegate::ObjectPropertyChanged> ) );

	FAssetEditorManager::Get().OnAssetEditorRequestedOpen().AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FAssetEditorRequestedOpenTrigger, FBaseTriggerDelegate::AssetEditorRequestedOpen>);

	GEditorModeTools().OnEditorModeChanged().AddSP( this, &FInteractiveTutorials::OnDelegateTriggered<FEditorModeChangedTrigger, FBaseTriggerDelegate::EditorModeChanged> );
	GEditorModeTools().OnWidgetModeChanged().AddSP( this, &FInteractiveTutorials::OnDelegateTriggered<FOnWidgetModeChanged, FBaseTriggerDelegate::OnWidgetModeChanged> );

	GEditor->OnEndObjectMovement().AddSP( this, &FInteractiveTutorials::OnDelegateTriggered<FOnEndTransformObject, FBaseTriggerDelegate::OnEndTransformObject> );

	FEditorDelegates::BeginPIE.AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnBeginPIETrigger, FBaseTriggerDelegate::OnBeginPIE>);
	FEditorDelegates::EndPIE.AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnEndPIETrigger, FBaseTriggerDelegate::OnEndPIE>);
	FEditorDelegates::OnConfigureNewAssetProperties.AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnNewAssetCreated, FBaseTriggerDelegate::OnNewAssetBeginConfiguration>);
	FEditorDelegates::OnNewAssetCreated.AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnNewAssetCreated, FBaseTriggerDelegate::OnNewAssetCreated>);
	FEditorDelegates::OnFinishPickingBlueprintClass.AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnNewBlueprintClassPicked, FBaseTriggerDelegate::OnNewBlueprintClassPicked>);
	FEditorDelegates::OnNewActorsDropped.AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnNewActorsDropped, FBaseTriggerDelegate::OnNewActorsDropped>);
	FEditorDelegates::OnApplyObjectToActor.AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnDropAssetOnActor, FBaseTriggerDelegate::OnDropAssetOnActor>);
	FEditorDelegates::OnGridSnappingChanged.AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnGridSnappingChanged, FBaseTriggerDelegate::OnGridSnappingChanged>);
	FEditorDelegates::OnLightingBuildStarted.AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnLightingBuildStarted, FBaseTriggerDelegate::OnLightingBuildStarted>);
	FEditorDelegates::OnLightingBuildKept.AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnLightingBuildKept, FBaseTriggerDelegate::OnLightingBuildKept>);
	FEditorDelegates::OnFocusViewportOnActors.AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnFocusViewportOnActors, FBaseTriggerDelegate::OnFocusViewportOnActors>);
	FEditorDelegates::OnMapOpened.AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnMapOpened, FBaseTriggerDelegate::OnMapOpened>);
	FEditorDelegates::OnEditorCameraMoved.AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnEditorCameraMoved, FBaseTriggerDelegate::EditorCameraMoved>);
	FEditorDelegates::OnDollyPerspectiveCamera.AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnEditorCameraZoom, FBaseTriggerDelegate::EditorCameraZoomed>);

	FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>( TEXT("ContentBrowser") );
	ContentBrowserModule.GetOnFilterChanged().AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnCBFilterChanged, FBaseTriggerDelegate::OnCBFilterChanged>);
	ContentBrowserModule.GetOnSearchBoxChanged().AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnCBSearchBoxChanged, FBaseTriggerDelegate::OnCBSearchBoxChanged>);
	ContentBrowserModule.GetOnAssetSelectionChanged().AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnCBAssetSelectionChanged, FBaseTriggerDelegate::OnCBAssetSelectionChanged>);
	ContentBrowserModule.GetOnSourcesViewChanged().AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnCBSourcesViewChanged, FBaseTriggerDelegate::OnCBSourcesViewChanged>);
	ContentBrowserModule.GetOnAssetPathChanged().AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnCBAssetPathChanged, FBaseTriggerDelegate::OnCBAssetPathChanged>);


	TSharedRef<IPlacementMode> PlacementEditMode = IPlacementModeModule::Get().GetPlacementMode();
	PlacementEditMode->OnStartedPlacing().AddSP(this, &FInteractiveTutorials::OnDelegateTriggered<FOnStartedPlacing, FBaseTriggerDelegate::OnStartedPlacing>);

	UK2Node_TutorialExcerptComplete::OnTutorialExcerptComplete.AddSP(this, &FInteractiveTutorials::CompleteExcerpt);


	// Hook us up to the tutorial system to allow us to highlight particular widgets
	STutorialWrapper::OnWidgetHighlight().BindSP(this, &FInteractiveTutorials::IsWidgetHighlighted);


	// Then make sure to collect all our actual tutorial data
	PopulateInteractivityData(SharedThis(this));
}

void FInteractiveTutorials::BindInteractivityData(const FString& InUDNPath, FBindInteractivityData InBindInteractivityData)
{
	FString UDNPath = InUDNPath;
	FPaths::NormalizeDirectoryName(UDNPath);

	TSharedRef<FInteractiveTutorial> Tutorial = MakeShareable(new FInteractiveTutorial());
	Tutorials.Add(UDNPath, Tutorial);

	if(InBindInteractivityData.IsBound())
	{
		InBindInteractivityData.Execute(UDNPath, Tutorial);
	}
}

void FInteractiveTutorials::SetCurrentTutorial(const FString& InUDNPath)
{
	FString UDNPath = InUDNPath;
	FPaths::NormalizeDirectoryName(UDNPath);

	// do we have interactivity data for this tutorial?
	TSharedPtr<FInteractiveTutorial>* Tutorial = Tutorials.Find(UDNPath);
	if(Tutorial != NULL)
	{
		CurrentTutorial = *Tutorial;
	}
}

ETutorialStyle::Type FInteractiveTutorials::GetCurrentTutorialStyle()
{
	if(CurrentTutorial.IsValid())
	{
		return CurrentTutorial->GetTutorialStyle();
	}
	else
	{
		return ETutorialStyle::Default;
	}
}


void FInteractiveTutorials::SetCurrentExcerpt(const FString& NewExcerpt)
{
	if(CurrentTutorial.IsValid())
	{
		CurrentTutorial->SetCurrentExcerpt(NewExcerpt);
	}
}

bool FInteractiveTutorials::IsExcerptInteractive(const FString& ExcerptName)
{
	if(CurrentTutorial.IsValid())
	{
		return CurrentTutorial->IsExcerptInteractive(ExcerptName);
	}

	return false;
}

bool FInteractiveTutorials::CanManuallyAdvanceExcerpt(const FString& ExcerptName)
{
	if (CurrentTutorial.IsValid())
	{
		return CurrentTutorial->CanManuallyAdvanceExcerpt(ExcerptName);
	}

	// allow by default
	return true;
}
bool FInteractiveTutorials::CanManuallyReverseExcerpt(const FString& ExcerptName)
{
	if (CurrentTutorial.IsValid())
	{
		return CurrentTutorial->CanManuallyReverseExcerpt(ExcerptName);
	}

	// allow by default
	return true;
}

bool FInteractiveTutorials::ShouldSkipExcerpt(const FString& ExcerptName)
{
	bool bShouldSkip = false;

	if(CurrentTutorial.IsValid())
	{
		const FShouldSkipExcerptDelegate* SkipDelegate = CurrentTutorial->FindSkipDelegate(ExcerptName);
		if(SkipDelegate != NULL && SkipDelegate->IsBound())
		{
			bShouldSkip = SkipDelegate->Execute();
			if (bShouldSkip)
			{
				CurrentTutorial->OnExcerptCompleted(ExcerptName);
			}
		}
	}

	return bShouldSkip;
}

UDialogueWave* FInteractiveTutorials::GetDialogueForExcerpt(const FString& ExcerptName)
{
	if(CurrentTutorial.IsValid())
	{
		return CurrentTutorial->FindDialogueAudio(ExcerptName);
	}

	return NULL;
}


void FInteractiveTutorials::OnExcerptCompleted(const FString& ExcerptName)
{
	if (CurrentTutorial.IsValid())
	{
		return CurrentTutorial->OnExcerptCompleted(ExcerptName);
	}
}

void FInteractiveTutorials::CompleteExcerpt(const FString& InExcerpt)
{
 	if(CurrentTutorial.IsValid())
 	{
 		if (InExcerpt == CurrentTutorial->GetCurrentExcerpt())
 		{
 			ExcerptConditionsCompleted.ExecuteIfBound();
 		}
 	}
}

bool FInteractiveTutorials::IsWidgetHighlighted(const FName& InWidgetName) const
{
	if(CurrentTutorial.IsValid())
	{
		return CurrentTutorial->IsWidgetHighlighted(InWidgetName);
	}

	return false;
}

template<typename DelegateSignature>
DelegateSignature FInteractiveTutorials::GetTriggerDelegate(FBaseTriggerDelegate::Type DelegateType)
{
	if(CurrentTutorial.IsValid())
	{
		TSharedPtr<FBaseTriggerDelegate> Trigger = CurrentTutorial->FindCurrentTrigger();

		if (Trigger.IsValid() && Trigger->DelegateType == DelegateType)
		{
			return StaticCastSharedPtr<FDerivedTriggerDelegate<DelegateSignature>>(Trigger)->Delegate;
		}
	}

	return DelegateSignature();
}

void FInteractiveTutorials::ExecuteCompletion(bool bSuccessfulCompletion)
{
	if (bSuccessfulCompletion)
	{
		ExcerptConditionsCompleted.ExecuteIfBound();
	}
}

template<typename DelegateType, FBaseTriggerDelegate::Type DelegateEnumType>
void FInteractiveTutorials::OnDelegateTriggered()
{
	auto Delegate = GetTriggerDelegate<DelegateType>(DelegateEnumType);
	if (Delegate.IsBound())
	{
		ExecuteCompletion(Delegate.Execute());
	}
}

template<typename DelegateType, FBaseTriggerDelegate::Type DelegateEnumType, typename Param1Type>
void FInteractiveTutorials::OnDelegateTriggered(Param1Type Param1)
{
	auto Delegate = GetTriggerDelegate<DelegateType>(DelegateEnumType);
	if (Delegate.IsBound())
	{
		ExecuteCompletion(Delegate.Execute(Param1));
	}
}

template<typename DelegateType, FBaseTriggerDelegate::Type DelegateEnumType, typename Param1Type, typename Param2Type>
void FInteractiveTutorials::OnDelegateTriggered(Param1Type Param1, Param2Type Param2)
{
	auto Delegate = GetTriggerDelegate<DelegateType>(DelegateEnumType);
	if (Delegate.IsBound())
	{
		ExecuteCompletion(Delegate.Execute(Param1, Param2));
	}
}

template<typename DelegateType, FBaseTriggerDelegate::Type DelegateEnumType, typename Param1Type, typename Param2Type, typename Param3Type, typename Param4Type>
void FInteractiveTutorials::OnDelegateTriggered(Param1Type Param1, Param2Type Param2, Param3Type Param3, Param4Type Param4)
{
	auto Delegate = GetTriggerDelegate<DelegateType>(DelegateEnumType);
	if (Delegate.IsBound())
	{
		ExecuteCompletion(Delegate.Execute(Param1, Param2, Param3, Param4));
	}
}



#undef LOCTEXT_NAMESPACE
