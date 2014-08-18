// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SKismetInspector.h"
#include "WidgetBlueprintEditor.h"
#include "MovieScene.h"
#include "Editor/Sequencer/Public/ISequencerModule.h"
#include "Animation/UMGSequencerObjectBindingManager.h"

#include "PropertyCustomizationHelpers.h"

#include "WidgetBlueprintApplicationModes.h"
//#include "WidgetDefafaultsApplicationMode.h"
#include "WidgetDesignerApplicationMode.h"
#include "WidgetGraphApplicationMode.h"

#include "WidgetBlueprintEditorToolbar.h"

#define LOCTEXT_NAMESPACE "UMG"

FWidgetBlueprintEditor::FWidgetBlueprintEditor()
	: PreviewScene(FPreviewScene::ConstructionValues().AllowAudioPlayback(true).ShouldSimulatePhysics(true))
	, PreviewBlueprint(NULL)
{
}

FWidgetBlueprintEditor::~FWidgetBlueprintEditor()
{
	UWidgetBlueprint* Blueprint = GetWidgetBlueprintObj();
	if ( Blueprint )
	{
		Blueprint->OnChanged().RemoveAll(this);
	}

	GEditor->OnObjectsReplaced().RemoveAll(this);
	
	Sequencer.Reset();

	SequencerObjectBindingManager.Reset();
}

void FWidgetBlueprintEditor::InitWidgetBlueprintEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray<UBlueprint*>& InBlueprints, bool bShouldOpenInDefaultsMode)
{
	TSharedPtr<FWidgetBlueprintEditor> ThisPtr(SharedThis(this));
	WidgetToolbar = MakeShareable(new FWidgetBlueprintEditorToolbar(ThisPtr));

	InitBlueprintEditor(Mode, InitToolkitHost, InBlueprints, bShouldOpenInDefaultsMode);

	// register for any objects replaced
	GEditor->OnObjectsReplaced().AddSP(this, &FWidgetBlueprintEditor::OnObjectsReplaced);

	UWidgetBlueprint* Blueprint = GetWidgetBlueprintObj();

	// If this blueprint is empty, add a canvas panel as the root widget.
	if ( Blueprint->WidgetTree->RootWidget == NULL )
	{
		UWidget* RootWidget = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		RootWidget->IsDesignTime(true);
		Blueprint->WidgetTree->RootWidget = RootWidget;
	}

	UpdatePreview(GetWidgetBlueprintObj(), true);

	SequencerObjectBindingManager->InitPreviewObjects();

	WidgetCommandList = MakeShareable(new FUICommandList);

	WidgetCommandList->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &FWidgetBlueprintEditor::DeleteSelectedWidgets),
		FCanExecuteAction::CreateSP(this, &FWidgetBlueprintEditor::CanDeleteSelectedWidgets)
		);

	WidgetCommandList->MapAction(FGenericCommands::Get().Copy,
		FExecuteAction::CreateSP(this, &FWidgetBlueprintEditor::CopySelectedWidgets),
		FCanExecuteAction::CreateSP(this, &FWidgetBlueprintEditor::CanCopySelectedWidgets)
		);

	WidgetCommandList->MapAction(FGenericCommands::Get().Cut,
		FExecuteAction::CreateSP(this, &FWidgetBlueprintEditor::CutSelectedWidgets),
		FCanExecuteAction::CreateSP(this, &FWidgetBlueprintEditor::CanCutSelectedWidgets)
		);

	WidgetCommandList->MapAction(FGenericCommands::Get().Paste,
		FExecuteAction::CreateSP(this, &FWidgetBlueprintEditor::PasteWidgets),
		FCanExecuteAction::CreateSP(this, &FWidgetBlueprintEditor::CanPasteWidgets)
		);
}

void FWidgetBlueprintEditor::RegisterApplicationModes(const TArray<UBlueprint*>& InBlueprints, bool bShouldOpenInDefaultsMode)
{
	//FBlueprintEditor::RegisterApplicationModes(InBlueprints, bShouldOpenInDefaultsMode);

	if ( InBlueprints.Num() == 1 )
	{
		TSharedPtr<FWidgetBlueprintEditor> ThisPtr(SharedThis(this));

		// Create the modes and activate one (which will populate with a real layout)
		TArray< TSharedRef<FApplicationMode> > TempModeList;
		TempModeList.Add(MakeShareable(new FWidgetDesignerApplicationMode(ThisPtr)));
		TempModeList.Add(MakeShareable(new FWidgetGraphApplicationMode(ThisPtr)));

		for ( TSharedRef<FApplicationMode>& AppMode : TempModeList )
		{
			AddApplicationMode(AppMode->GetModeName(), AppMode);
		}

		SetCurrentMode(FWidgetBlueprintApplicationModes::DesignerMode);
	}
	else
	{
		//// We either have no blueprints or many, open in the defaults mode for multi-editing
		//AddApplicationMode(
		//	FBlueprintEditorApplicationModes::BlueprintDefaultsMode,
		//	MakeShareable(new FBlueprintDefaultsApplicationMode(SharedThis(this))));
		//SetCurrentMode(FBlueprintEditorApplicationModes::BlueprintDefaultsMode);
	}
}

void FWidgetBlueprintEditor::SelectWidgets(const TSet<FWidgetReference>& Widgets)
{
	TSet<FWidgetReference> TempSelection;
	for ( const FWidgetReference& Widget : Widgets )
	{
		if ( Widget.IsValid() )
		{
			TempSelection.Add(Widget);
		}
	}

	OnSelectedWidgetsChanging.Broadcast();

	// Finally change the selected widgets after we've updated the details panel 
	// to ensure values that are pending are committed on focus loss, and migrated properly
	// to the old selected widgets.
	SelectedWidgets.Empty();
	SelectedObjects.Empty();

	SelectedWidgets.Append(TempSelection);

	OnSelectedWidgetsChanged.Broadcast();
}

void FWidgetBlueprintEditor::SelectObjects(const TSet<UObject*>& Objects)
{
	OnSelectedWidgetsChanging.Broadcast();

	SelectedWidgets.Empty();
	SelectedObjects.Empty();

	for ( UObject* Obj : Objects )
	{
		SelectedObjects.Add(Obj);
	}

	OnSelectedWidgetsChanged.Broadcast();
}

void FWidgetBlueprintEditor::CleanSelection()
{
	TSet<FWidgetReference> TempSelection;

	TArray<UWidget*> WidgetsInTree;
	GetWidgetBlueprintObj()->WidgetTree->GetAllWidgets(WidgetsInTree);
	TSet<UWidget*> TreeWidgetSet(WidgetsInTree);

	for ( FWidgetReference& WidgetRef : SelectedWidgets )
	{
		if ( WidgetRef.IsValid() )
		{
			if ( TreeWidgetSet.Contains(WidgetRef.GetTemplate()) )
			{
				TempSelection.Add(WidgetRef);
			}
		}
	}

	if ( TempSelection.Num() != SelectedWidgets.Num() )
	{
		SelectWidgets(TempSelection);
	}
}

const TSet<FWidgetReference>& FWidgetBlueprintEditor::GetSelectedWidgets() const
{
	return SelectedWidgets;
}

const TSet< TWeakObjectPtr<UObject> >& FWidgetBlueprintEditor::GetSelectedObjects() const
{
	return SelectedObjects;
}

void FWidgetBlueprintEditor::OnBlueprintChanged(UBlueprint* InBlueprint)
{
	FBlueprintEditor::OnBlueprintChanged(InBlueprint);

	if ( InBlueprint )
	{
		// Rebuilding the preview can force objects to be recreated, so the selection may need to be updated.
		OnSelectedWidgetsChanging.Broadcast();

		UpdatePreview(InBlueprint, true);

		CleanSelection();

		// Fire the selection updated event to ensure everyone is watching the same widgets.
		OnSelectedWidgetsChanged.Broadcast();
	}
}

void FWidgetBlueprintEditor::OnObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap)
{
	// Objects being replaced could force a selection change
	OnSelectedWidgetsChanging.Broadcast();

	TSet<FWidgetReference> TempSelection;
	for ( const FWidgetReference& WidgetRef : SelectedWidgets )
	{
		UWidget* OldTemplate = WidgetRef.GetTemplate();

		UObject* const* NewObject = ReplacementMap.Find(OldTemplate);
		if ( NewObject )
		{
			UWidget* NewTemplate = Cast<UWidget>(*NewObject);
			TempSelection.Add(FWidgetReference::FromTemplate(SharedThis(this), NewTemplate));
		}
		else
		{
			TempSelection.Add(WidgetRef);
		}
	}

	SelectedWidgets = TempSelection;

	// Fire the selection updated event to ensure everyone is watching the same widgets.
	OnSelectedWidgetsChanged.Broadcast();
}

bool FWidgetBlueprintEditor::CanDeleteSelectedWidgets()
{
	TSet<FWidgetReference> Widgets = GetSelectedWidgets();
	return Widgets.Num() > 0;
}

void FWidgetBlueprintEditor::DeleteSelectedWidgets()
{
	TSet<FWidgetReference> Widgets = GetSelectedWidgets();
	FWidgetBlueprintEditorUtils::DeleteWidgets(GetWidgetBlueprintObj(), Widgets);

	// Clear the selection now that the widget has been deleted.
	TSet<FWidgetReference> Empty;
	SelectWidgets(Empty);
}

bool FWidgetBlueprintEditor::CanCopySelectedWidgets()
{
	TSet<FWidgetReference> Widgets = GetSelectedWidgets();
	return Widgets.Num() > 0;
}

void FWidgetBlueprintEditor::CopySelectedWidgets()
{
	TSet<FWidgetReference> Widgets = GetSelectedWidgets();
	FWidgetBlueprintEditorUtils::CopyWidgets(GetWidgetBlueprintObj(), Widgets);
}

bool FWidgetBlueprintEditor::CanCutSelectedWidgets()
{
	TSet<FWidgetReference> Widgets = GetSelectedWidgets();
	return Widgets.Num() > 0;
}

void FWidgetBlueprintEditor::CutSelectedWidgets()
{
	TSet<FWidgetReference> Widgets = GetSelectedWidgets();
	FWidgetBlueprintEditorUtils::CutWidgets(GetWidgetBlueprintObj(), Widgets);
}

const UWidgetAnimation* FWidgetBlueprintEditor::RefreshCurrentAnimation()
{
	if( !SequencerObjectBindingManager->HasValidWidgetAnimation() )
	{
		const TArray<UWidgetAnimation*>& Animations = GetWidgetBlueprintObj()->Animations;
		if( Animations.Num() > 0 )
		{
			// Ensure we are viewing a valid animation
			ChangeViewedAnimation( *Animations[0] );
			return Animations[0];
		}
		else
		{
			ChangeViewedAnimation( *UWidgetAnimation::GetNullAnimation() );
			return nullptr;
		}
	}

	return SequencerObjectBindingManager->GetWidgetAnimation();
}

bool FWidgetBlueprintEditor::CanPasteWidgets()
{
	TSet<FWidgetReference> Widgets = GetSelectedWidgets();
	if ( Widgets.Num() == 1 )
	{
		FWidgetReference Target = *Widgets.CreateIterator();
		const bool bIsPanel = Cast<UPanelWidget>(Target.GetTemplate()) != NULL;
		return bIsPanel;
	}
	else if ( Widgets.Num() == 0 )
	{
		if ( GetWidgetBlueprintObj()->WidgetTree->RootWidget == NULL )
		{
			return true;
		}
	}

	return false;
}

void FWidgetBlueprintEditor::PasteWidgets()
{
	TSet<FWidgetReference> Widgets = GetSelectedWidgets();
	FWidgetReference Target = Widgets.Num() > 0 ? *Widgets.CreateIterator() : FWidgetReference();

	FWidgetBlueprintEditorUtils::PasteWidgets(GetWidgetBlueprintObj(), Target, PasteDropLocation);

	//TODO UMG - Select the newly selected pasted widgets.
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

	// Note: The weak ptr can become stale if the actor is reinstanced due to a Blueprint change, etc. In that case we 
	//       look to see if we can find the new instance in the preview world and then update the weak ptr.
	if ( PreviewWidgetPtr.IsStale(true) )
	{
		UpdatePreview(GetWidgetBlueprintObj(), true);
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
			// Check to see if there's an edit condition property we also need to migrate.
			bool bDummyNegate = false;
			UBoolProperty* EditConditionProperty = PropertyCustomizationHelpers::GetEditConditionProperty(MemberProperty, bDummyNegate);
			if ( EditConditionProperty != NULL )
			{
				MigratePropertyValue(SourceObject, DestinationObject, EditConditionProperty);
			}

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

	UUserWidget* Preview = GetPreview();
	Collector.AddReferencedObject( Preview );
}

void FWidgetBlueprintEditor::MigrateFromChain(FEditPropertyChain* PropertyThatChanged, bool bIsModify)
{
	UWidgetBlueprint* Blueprint = GetWidgetBlueprintObj();

	UUserWidget* PreviewActor = GetPreview();
	if ( PreviewActor != NULL )
	{
		for ( FWidgetReference& WidgetRef : SelectedWidgets )
		{
			UWidget* PreviewWidget = WidgetRef.GetPreview();

			if ( PreviewWidget )
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
}

void FWidgetBlueprintEditor::PostUndo(bool bSuccessful)
{
	FBlueprintEditor::PostUndo(bSuccessful);

	OnWidgetBlueprintTransaction.Broadcast();
}

void FWidgetBlueprintEditor::PostRedo(bool bSuccessful)
{
	FBlueprintEditor::PostRedo(bSuccessful);

	OnWidgetBlueprintTransaction.Broadcast();
}

TSharedRef<SWidget> FWidgetBlueprintEditor::CreateSequencerWidget()
{
	TSharedRef<SOverlay> SequencerOverlayRef =
		SNew(SOverlay)
		.Tag(TEXT("Sequencer"))
		+ SOverlay::Slot()
		[
			GetSequencer()->GetSequencerWidget()
		];

	SequencerOverlay = SequencerOverlayRef;

	RefreshCurrentAnimation();

	return SequencerOverlayRef;
}

UWidgetBlueprint* FWidgetBlueprintEditor::GetWidgetBlueprintObj() const
{
	return Cast<UWidgetBlueprint>(GetBlueprintObj());
}

UUserWidget* FWidgetBlueprintEditor::GetPreview() const
{
	if ( PreviewWidgetPtr.IsStale(true) )
	{
		return NULL;
	}

	return PreviewWidgetPtr.Get();
}

TSharedPtr<ISequencer>& FWidgetBlueprintEditor::GetSequencer()
{
	if(!Sequencer.IsValid())
	{
		UWidgetBlueprint* Blueprint = GetWidgetBlueprintObj();

		UWidgetAnimation* WidgetAnimation = nullptr;

		if( Blueprint->Animations.Num() )
		{
			WidgetAnimation = Blueprint->Animations[0];
		}
		else
		{
			UWidgetAnimation* NewAnimation = ConstructObject<UWidgetAnimation>(UWidgetAnimation::StaticClass(), Blueprint, MakeUniqueObjectName(Blueprint, UWidgetAnimation::StaticClass(), "NewAnimation"), RF_Transactional);
			NewAnimation->MovieScene =  ConstructObject<UMovieScene>(UMovieScene::StaticClass(), NewAnimation, NAME_None, RF_Transactional);

			WidgetAnimation = NewAnimation;
		}

		TSharedRef<FUMGSequencerObjectBindingManager> ObjectBindingManager = MakeShareable(new FUMGSequencerObjectBindingManager(*this, *WidgetAnimation ) );
		check( !SequencerObjectBindingManager.IsValid() );

		SequencerObjectBindingManager = ObjectBindingManager;

		FSequencerViewParams ViewParams;
		ViewParams.InitalViewRange = TRange<float>(-0.02f, 3.2f);
		ViewParams.InitialScrubPosition = 0;

		Sequencer = FModuleManager::LoadModuleChecked< ISequencerModule >("Sequencer").CreateSequencer( WidgetAnimation->MovieScene, ViewParams, ObjectBindingManager );
	}

	return Sequencer;
}

void FWidgetBlueprintEditor::ChangeViewedAnimation( UWidgetAnimation& InAnimationToView )
{
	if( InAnimationToView.MovieScene != Sequencer->GetRootMovieScene() )
	{
		TSharedRef<FUMGSequencerObjectBindingManager> NewObjectBindingManager = MakeShareable(new FUMGSequencerObjectBindingManager(*this, InAnimationToView));

		Sequencer->ResetToNewRootMovieScene(*InAnimationToView.MovieScene, NewObjectBindingManager);

		check(SequencerObjectBindingManager.IsUnique());

		SequencerObjectBindingManager = NewObjectBindingManager;

		SequencerObjectBindingManager->InitPreviewObjects();

		TSharedPtr<SOverlay> SequencerOverlayPin = SequencerOverlay.Pin();
		if( &InAnimationToView == UWidgetAnimation::GetNullAnimation() && SequencerOverlayPin.IsValid() )
		{
			Sequencer->GetSequencerWidget()->SetEnabled(false);
			// Disable sequencer from interaction
			SequencerOverlayPin->AddSlot(1)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew( STextBlock )
				.TextStyle( FEditorStyle::Get(), "UMGEditor.NoAnimationFont" )
				.Text( LOCTEXT("NoAnimationSelected","No Animation Selected") )
			];

			SequencerOverlayPin->SetVisibility( EVisibility::HitTestInvisible );
		}
		else if( SequencerOverlayPin.IsValid() && SequencerOverlayPin->GetNumWidgets() > 1 )
		{
			Sequencer->GetSequencerWidget()->SetEnabled(true);

			SequencerOverlayPin->RemoveSlot(1);
			// Allow sequencer to be interacted with
			SequencerOverlayPin->SetVisibility( EVisibility::SelfHitTestInvisible );
		}
	}
}

void FWidgetBlueprintEditor::DestroyPreview()
{
	UUserWidget* PreviewActor = GetPreview();
	if ( PreviewActor != NULL )
	{
		check(PreviewScene.GetWorld());

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
		PreviewBlueprint = Cast<UWidgetBlueprint>(InBlueprint);

		// Update the generated class'es widget tree to match the blueprint tree.  That way the preview can update
		// without needing to do a full recompile.
		Cast<UWidgetBlueprintGeneratedClass>(PreviewBlueprint->GeneratedClass)->WidgetTree = DuplicateObject<UWidgetTree>(PreviewBlueprint->WidgetTree, PreviewBlueprint->GeneratedClass);

		PreviewActor = CreateWidget<UUserWidget>(PreviewScene.GetWorld(), PreviewBlueprint->GeneratedClass);
		PreviewActor->SetFlags(RF_Transactional);
		
		// Configure all the widgets to be set to design time.
		PreviewActor->IsDesignTime(true);
		for(UWidget* SubPreviewWidget : PreviewActor->Components)
		{
			SubPreviewWidget->IsDesignTime(true);
		}

		// Store a reference to the preview actor.
		PreviewWidgetPtr = PreviewActor;
	}

	OnWidgetPreviewUpdated.Broadcast();
}


FGraphAppearanceInfo FWidgetBlueprintEditor::GetGraphAppearance() const
{
	FGraphAppearanceInfo AppearanceInfo = FBlueprintEditor::GetGraphAppearance();

	if ( GetBlueprintObj()->IsA(UWidgetBlueprint::StaticClass()) )
	{
		AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText", "WIDGET BLUEPRINT").ToString();
	}

	return AppearanceInfo;
}

#undef LOCTEXT_NAMESPACE
