// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SKismetInspector.h"
#include "WidgetBlueprintEditor.h"
#include "MovieScene.h"
#include "Editor/Sequencer/Public/ISequencerModule.h"

#define LOCTEXT_NAMESPACE "UMG"

FWidgetBlueprintEditor::FWidgetBlueprintEditor()
	: PreviewScene(FPreviewScene::ConstructionValues().AllowAudioPlayback(true).ShouldSimulatePhysics(true))
	, PreviewBlueprint(NULL)
	, DefaultMovieScene(NULL)
{
}

FWidgetBlueprintEditor::~FWidgetBlueprintEditor()
{
	UWidgetBlueprint* Blueprint = GetWidgetBlueprintObj();
	if ( Blueprint )
	{
		Blueprint->OnChanged().RemoveAll(this);
	}
}

void FWidgetBlueprintEditor::SelectWidgets(TArray<UWidget*> Widgets)
{
	// Convert the selection set to an array of UObject* pointers
	FString InspectorTitle;
	TArray<UObject*> InspectorObjects;
	InspectorObjects.Empty(Widgets.Num());
	for ( UWidget* Widget : Widgets )
	{
		//if ( NodePtr->CanEditDefaults() )
		{
			InspectorTitle = "Widget";// Widget->GetDisplayString();
			InspectorObjects.Add(Widget);
		}
	}

	UWidgetBlueprint* Blueprint = GetWidgetBlueprintObj();

	// Update the details panel
	SKismetInspector::FShowDetailsOptions Options(InspectorTitle, true);
	GetInspector()->ShowDetailsForObjects(InspectorObjects, Options);

	SelectedPreviewWidgets = Widgets;
}

void FWidgetBlueprintEditor::OnBlueprintChanged(UBlueprint* InBlueprint)
{
	if ( InBlueprint )
	{
		UpdatePreview(InBlueprint, true);
	}
}

void FWidgetBlueprintEditor::InitWidgetBlueprintEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray<UBlueprint*>& InBlueprints, bool bShouldOpenInDefaultsMode)
{
	InitBlueprintEditor(Mode, InitToolkitHost, InBlueprints, bShouldOpenInDefaultsMode);

	UpdatePreview(GetWidgetBlueprintObj(), true);
}

void FWidgetBlueprintEditor::Tick(float DeltaTime)
{
	FBlueprintEditor::Tick(DeltaTime);

	// Tick the preview scene world.
	if ( !GIntraFrameDebuggingGameThread )
	{
		bool bIsSimulateEnabled = true;
		bool bIsRealTime = true;

		// Allow full tick only if preview simulation is enabled and we're not currently in an active SIE or PIE session
		if ( bIsSimulateEnabled && GEditor->PlayWorld == NULL && !GEditor->bIsSimulatingInEditor )
		{
			PreviewScene.GetWorld()->Tick(bIsRealTime ? LEVELTICK_All : LEVELTICK_TimeOnly, DeltaTime);
		}
		else
		{
			PreviewScene.GetWorld()->Tick(bIsRealTime ? LEVELTICK_ViewportsOnly : LEVELTICK_TimeOnly, DeltaTime);
		}
	}
}

static bool MigratePropertyValue(UObject* SourceObject, UObject* DestinationObject, UProperty* MemberProperty)
{
	FString SourceValue;

	// Get the property addresses for the source and destination objects.
	uint8* SourceAddr = MemberProperty->ContainerPtrToValuePtr<uint8>(SourceObject);
	uint8* DestionationAddr = MemberProperty->ContainerPtrToValuePtr<uint8>(DestinationObject);

	if ( SourceAddr == NULL || DestionationAddr == NULL )
	{
		return false;
	}

	// Get the current value from the source object.
	MemberProperty->ExportText_Direct(SourceValue, SourceAddr, SourceAddr, NULL, PPF_Localized);

	const bool bNotifyObjectOfChange = true;

	if ( !DestinationObject->HasAnyFlags(RF_ClassDefaultObject) && bNotifyObjectOfChange )
	{
		DestinationObject->PreEditChange(MemberProperty);
	}

	// Set the value on the destination object.
	MemberProperty->ImportText(*SourceValue, DestionationAddr, 0, DestinationObject);

	if ( !DestinationObject->HasAnyFlags(RF_ClassDefaultObject) && bNotifyObjectOfChange )
	{
		FPropertyChangedEvent PropertyEvent(MemberProperty);
		DestinationObject->PostEditChangeProperty(PropertyEvent);
	}

	return true;
}

static bool MigratePropertyValue(UObject* SourceObject, UObject* DestinationObject, FEditPropertyChain::TDoubleLinkedListNode* PropertyChainNode, UProperty* MemberProperty, bool bIsModify)
{
	UProperty* CurrentProperty = PropertyChainNode->GetValue();

	if ( PropertyChainNode->GetNextNode() == NULL )
	{
		if ( bIsModify )
		{
			DestinationObject->Modify();
			return true;
		}
		else
		{
			return MigratePropertyValue(SourceObject, DestinationObject, MemberProperty);
		}
	}
	
	if ( UObjectProperty* CurrentObjectProperty = Cast<UObjectProperty>(CurrentProperty) )
	{
		// Get the property addresses for the source and destination objects.
		UObject* SourceObjectProperty = CurrentObjectProperty->GetObjectPropertyValue(CurrentObjectProperty->ContainerPtrToValuePtr<void>(SourceObject));
		UObject* DestionationObjectProperty = CurrentObjectProperty->GetObjectPropertyValue(CurrentObjectProperty->ContainerPtrToValuePtr<void>(DestinationObject));

		return MigratePropertyValue(SourceObjectProperty, DestionationObjectProperty, PropertyChainNode->GetNextNode(), PropertyChainNode->GetNextNode()->GetValue(), bIsModify);
	}
	else if ( UArrayProperty* CurrentArrayProperty = Cast<UArrayProperty>(CurrentProperty) )
	{
		// Arrays!
	}

	return MigratePropertyValue(SourceObject, DestinationObject, PropertyChainNode->GetNextNode(), MemberProperty, bIsModify);
}

void FWidgetBlueprintEditor::AddReferencedObjects( FReferenceCollector& Collector )
{
	FBlueprintEditor::AddReferencedObjects( Collector );
	if( DefaultMovieScene != nullptr )
	{
		Collector.AddReferencedObject( DefaultMovieScene );
	}
}

void FWidgetBlueprintEditor::NotifyPreChange(FEditPropertyChain* PropertyAboutToChange)
{
	MigrateFromChain(PropertyAboutToChange, true);
}

void FWidgetBlueprintEditor::MigrateFromChain(FEditPropertyChain* PropertyThatChanged, bool bIsModify)
{
	UWidgetBlueprint* Blueprint = GetWidgetBlueprintObj();

	UUserWidget* PreviewActor = GetPreview();
	if ( PreviewActor != NULL )
	{
		for ( UWidget* PreviewWidget : SelectedPreviewWidgets )
		{
			FString PreviewWidgetName = PreviewWidget->GetName();
			UWidget* TemplateWidget = Blueprint->WidgetTree->FindWidget(PreviewWidgetName);

			if ( TemplateWidget )
			{
				FEditPropertyChain::TDoubleLinkedListNode* PropertyChainNode = PropertyThatChanged->GetHead();
				MigratePropertyValue(PreviewWidget, TemplateWidget, PropertyChainNode, PropertyChainNode->GetValue(), bIsModify);
			}
		}
	}
}

void FWidgetBlueprintEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FEditPropertyChain* PropertyThatChanged)
{
	//Super::NotifyPostChange(PropertyChangedEvent, PropertyThatChanged);
	
	if ( PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive )
	{
		MigrateFromChain(PropertyThatChanged, false);
	}
}

UWidgetBlueprint* FWidgetBlueprintEditor::GetWidgetBlueprintObj() const
{
	return Cast<UWidgetBlueprint>(GetBlueprintObj());
}

UUserWidget* FWidgetBlueprintEditor::GetPreview() const
{
	//// Note: The weak ptr can become stale if the actor is reinstanced due to a Blueprint change, etc. In that case we look to see if we can find the new instance in the preview world and then update the weak ptr.
	//if ( PreviewWidgetActorPtr.IsStale(true) )
	//{
	//	UWorld* PreviewWorld = PreviewWidgetActorPtr->GetWorld();
	//	for ( TActorIterator<AActor> It(PreviewWorld); It; ++It )
	//	{
	//		AActor* Actor = *It;
	//		if ( !Actor->IsPendingKillPending()
	//			&& Actor->GetClass()->ClassGeneratedBy == PreviewBlueprint )
	//		{
	//			PreviewActorPtr = Actor;
	//			break;
	//		}
	//	}
	//}

	return PreviewWidgetActorPtr.Get();
}

TSharedPtr<ISequencer>& FWidgetBlueprintEditor::GetSequencer()
{
	if(!Sequencer.IsValid())
	{
		UWidgetBlueprint* Blueprint = GetWidgetBlueprintObj();

		UMovieScene* MovieScene = Blueprint->AnimationData.Num() ? Blueprint->AnimationData[0] : GetDefaultMovieScene();
		Sequencer = FModuleManager::LoadModuleChecked< ISequencerModule >("Sequencer").CreateSequencer(MovieScene);
	}

	return Sequencer;
}

void FWidgetBlueprintEditor::DestroyPreview()
{
	UUserWidget* PreviewActor = GetPreview();
	if ( PreviewActor != NULL )
	{
		check(PreviewScene.GetWorld());

		PreviewActor->ClearFlags(RF_Standalone);
		PreviewActor->MarkPendingKill();
	}

	//if ( PreviewBlueprint != NULL )
	//{
	//	if ( PreviewBlueprint->SimpleConstructionScript != NULL
	//		&& PreviewActor == PreviewBlueprint->SimpleConstructionScript->GetComponentEditorActorInstance() )
	//	{
	//		// Ensure that all editable component references are cleared
	//		PreviewBlueprint->SimpleConstructionScript->ClearEditorComponentReferences();

	//		// Clear the reference to the preview actor instance
	//		PreviewBlueprint->SimpleConstructionScript->SetComponentEditorActorInstance(NULL);
	//	}

	//	PreviewBlueprint = NULL;
	//}
}

void FWidgetBlueprintEditor::UpdatePreview(UBlueprint* InBlueprint, bool bInForceFullUpdate)
{
	UUserWidget* PreviewActor = GetPreview();

	// Signal that we're going to be constructing editor components
	if ( InBlueprint != NULL && InBlueprint->SimpleConstructionScript != NULL )
	{
		InBlueprint->SimpleConstructionScript->BeginEditorComponentConstruction();
	}

	// If the Blueprint is changing
	if ( InBlueprint != PreviewBlueprint || bInForceFullUpdate )
	{
		// Destroy the previous actor instance
		DestroyPreview();

		// Save the Blueprint we're creating a preview for
		PreviewBlueprint = InBlueprint;

		PreviewActor = ConstructObject<UUserWidget>(PreviewBlueprint->GeneratedClass, PreviewScene.GetWorld()->GetCurrentLevel());
		PreviewActor->SetFlags(RF_Standalone | RF_Transactional);
        
        // Configure all the widgets to be set to design time.
        PreviewActor->IsDesignTime(true);
        for(UWidget* SubPreviewWidget : PreviewActor->Components)
        {
            SubPreviewWidget->IsDesignTime(true);
        }

		PreviewWidgetActorPtr = PreviewActor;
	}
}

UMovieScene* FWidgetBlueprintEditor::GetDefaultMovieScene()
{
	if( !DefaultMovieScene )
	{
		DefaultMovieScene = ConstructObject<UMovieScene>( UMovieScene::StaticClass(), GetTransientPackage(), "DefaultAnimationData" );
	}

	return DefaultMovieScene;
}

#undef LOCTEXT_NAMESPACE
