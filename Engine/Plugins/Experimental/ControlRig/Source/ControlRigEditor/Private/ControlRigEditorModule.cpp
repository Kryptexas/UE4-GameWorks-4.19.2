// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ControlRigEditorModule.h"
#include "PropertyEditorModule.h"
#include "ControlRigVariableDetailsCustomization.h"
#include "BlueprintEditorModule.h"
#include "KismetEditorUtilities.h"
#include "ComponentVisualizers.h"
#include "ControlRig.h"
#include "K2Node_ControlRig.h"
#include "K2Node_ControlRigOutput.h"
#include "ControlRigInputOutputDetailsCustomization.h"
#include "UserLabeledFieldCustomization.h"
#include "ControlRigComponent.h"
#include "ControlRigComponentVisualizer.h"
#include "ISequencerModule.h"
#include "ControlRigTrackEditor.h"
#include "IAssetTools.h"
#include "ControlRigSequenceActions.h"
#include "ControlRigSequenceEditorStyle.h"
#include "LayoutExtender.h"
#include "LevelEditor.h"
#include "MovieSceneToolsProjectSettings.h"
#include "PropertyEditorModule.h"
#include "Styling/SlateStyle.h"
#include "WorkflowTabManager.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "ControlRigSequence.h"
#include "EditorModeRegistry.h"
#include "ControlRigEditMode.h"
#include "HumanRig.h"
#include "HumanRigDetails.h"
#include "ControlRigEditorObjectBinding.h"
#include "ControlRigEditorObjectSpawner.h"
#include "ILevelSequenceModule.h"
#include "ControlRigBindingTrackEditor.h"
#include "EditorModeManager.h"
#include "ControlRigEditMode.h"

#define LOCTEXT_NAMESPACE "ControlRigEditorModule"

void FControlRigEditorModule::StartupModule()
{
	FHumanRigNodeCommand::Register();
	FControlRigSequenceEditorStyle::Get();

	// Register Blueprint editor variable customization
	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintEditorModule.RegisterVariableCustomization(UProperty::StaticClass(), FOnGetVariableCustomizationInstance::CreateStatic(&FControlRigVariableDetailsCustomization::MakeInstance));

	// Register to fixup newly created BPs
	FKismetEditorUtilities::RegisterOnBlueprintCreatedCallback(this, UControlRig::StaticClass(), FKismetEditorUtilities::FOnBlueprintCreated::CreateRaw(this, &FControlRigEditorModule::HandleNewBlueprintCreated));

	// Register details customizations for animation controller nodes
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditorModule.RegisterCustomClassLayout(UK2Node_ControlRig::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FControlRigInputOutputDetailsCustomization::MakeInstance));
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(FUserLabeledField::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FUserLabeledFieldCustomization::MakeInstance));
	PropertyEditorModule.RegisterCustomClassLayout(UHumanRig::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FHumanRigDetails::MakeInstance));
	//PropertyEditorModule.RegisterCustomClassLayout(UControlRigComponent::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FControlRigComponentDetails::MakeInstance));

	// Register blueprint compiler
	IKismetCompilerInterface& KismetCompilerModule = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>("KismetCompiler");
	KismetCompilerModule.GetCompilers().Add(&ControlRigBlueprintCompiler);

	FComponentVisualizersModule& ComponentVisualizerModule = FModuleManager::LoadModuleChecked<FComponentVisualizersModule>("ComponentVisualizers");
	ComponentVisualizerModule.RegisterComponentVisualizer(UControlRigComponent::StaticClass()->GetFName(), MakeShareable(new FControlRigComponentVisualizer));

	// Register asset tools
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	TSharedRef<IAssetTypeActions> AssetTypeAction = MakeShareable(new FControlRigSequenceActions());
	RegisteredAssetTypeActions.Add(AssetTypeAction);
	AssetTools.RegisterAssetTypeActions(AssetTypeAction);

	// Register sequencer track editor
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");
	SequencerCreatedHandle = SequencerModule.RegisterOnSequencerCreated(FOnSequencerCreated::FDelegate::CreateRaw(this, &FControlRigEditorModule::HandleSequencerCreated));
	ControlRigTrackCreateEditorHandle = SequencerModule.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FControlRigTrackEditor::CreateTrackEditor));
	ControlRigBindingTrackCreateEditorHandle = SequencerModule.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FControlRigBindingTrackEditor::CreateTrackEditor));
	ControlRigEditorObjectBindingHandle = SequencerModule.RegisterEditorObjectBinding(FOnCreateEditorObjectBinding::CreateStatic(&FControlRigEditorObjectBinding::CreateEditorObjectBinding));

	// Register level sequence spawner
	ILevelSequenceModule& LevelSequenceModule = FModuleManager::LoadModuleChecked<ILevelSequenceModule>("LevelSequence");
	LevelSequenceSpawnerDelegateHandle = LevelSequenceModule.RegisterObjectSpawner(FOnCreateMovieSceneObjectSpawner::CreateStatic(&FControlRigEditorObjectSpawner::CreateObjectSpawner));

	FEditorModeRegistry::Get().RegisterMode<FControlRigEditMode>(
		FControlRigEditMode::ModeName,
		NSLOCTEXT("AnimationModeToolkit", "DisplayName", "Animation"),
		FSlateIcon(FControlRigSequenceEditorStyle::Get().GetStyleSetName(), "ControlRigEditMode", "ControlRigEditMode.Small"),
		true);
}

void FControlRigEditorModule::ShutdownModule()
{
	FEditorModeRegistry::Get().UnregisterMode(FControlRigEditMode::ModeName);

	ILevelSequenceModule* LevelSequenceModule = FModuleManager::GetModulePtr<ILevelSequenceModule>("LevelSequence");
	if (LevelSequenceModule)
	{
		LevelSequenceModule->UnregisterObjectSpawner(LevelSequenceSpawnerDelegateHandle);
	}

	ISequencerModule* SequencerModule = FModuleManager::GetModulePtr<ISequencerModule>("Sequencer");
	if (SequencerModule)
	{
		SequencerModule->UnregisterOnSequencerCreated(SequencerCreatedHandle);
		SequencerModule->UnRegisterTrackEditor(ControlRigTrackCreateEditorHandle);
		SequencerModule->UnRegisterTrackEditor(ControlRigBindingTrackCreateEditorHandle);
		SequencerModule->UnRegisterEditorObjectBinding(ControlRigEditorObjectBindingHandle);
	}

	FAssetToolsModule* AssetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools");
	if (AssetToolsModule)
	{
		for (TSharedRef<IAssetTypeActions> RegisteredAssetTypeAction : RegisteredAssetTypeActions)
		{
			AssetToolsModule->Get().UnregisterAssetTypeActions(RegisteredAssetTypeAction);
		}
	}

	FKismetEditorUtilities::UnregisterAutoBlueprintNodeCreation(this);

	FBlueprintEditorModule* BlueprintEditorModule = FModuleManager::GetModulePtr<FBlueprintEditorModule>("Kismet");
	if (BlueprintEditorModule)
	{
		BlueprintEditorModule->UnregisterVariableCustomization(UProperty::StaticClass());
	}

	FPropertyEditorModule* PropertyEditorModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor");
	if (PropertyEditorModule)
	{
		PropertyEditorModule->UnregisterCustomClassLayout(UK2Node_ControlRig::StaticClass()->GetFName());
		PropertyEditorModule->UnregisterCustomClassLayout(FUserLabeledField::StaticStruct()->GetFName());
		PropertyEditorModule->UnregisterCustomClassLayout(UHumanRig::StaticClass()->GetFName());
		//	PropertyEditorModule->UnregisterCustomClassLayout(UControlRigComponent::StaticClass()->GetFName());
	}

	IKismetCompilerInterface* KismetCompilerModule = FModuleManager::GetModulePtr<IKismetCompilerInterface>("KismetCompiler");
	if (KismetCompilerModule)
	{
		KismetCompilerModule->GetCompilers().Remove(&ControlRigBlueprintCompiler);
	}
}

void FControlRigEditorModule::HandleNewBlueprintCreated(UBlueprint* InBlueprint)
{
	if (ensure(InBlueprint->UbergraphPages.Num() > 0))
	{
		UEdGraph* EventGraph = InBlueprint->UbergraphPages[0];

		// Add animation output node
		UK2Node_ControlRigOutput* OutputNode = NewObject<UK2Node_ControlRigOutput>(EventGraph);
		OutputNode->CreateNewGuid();
		OutputNode->PostPlacedNewNode();
		OutputNode->SetFlags(RF_Transactional);
		OutputNode->AllocateDefaultPins();
		OutputNode->ReconstructNode();
		OutputNode->NodePosX = 0;
		OutputNode->NodePosY = 0;
		UEdGraphSchema_K2::SetNodeMetaData(OutputNode, FNodeMetadata::DefaultGraphNode);
		OutputNode->DisableNode();
		OutputNode->NodeComment = LOCTEXT("AnimationOutputComment", "This node acts as the output for this animation controller.\nTo add or remove an output pin, enable or disable the \"Animation Output\" checkbox for a variable.").ToString();
		OutputNode->bCommentBubbleVisible = true;
		OutputNode->bCommentBubblePinned = true;

		EventGraph->AddNode(OutputNode);
	}
}

void FControlRigEditorModule::HandleSequencerCreated(TSharedRef<ISequencer> InSequencer)
{
	TWeakPtr<ISequencer> WeakSequencer = InSequencer;

	// We want to be informed of sequence activations (subsequences or not)
	auto HandleActivateSequence = [WeakSequencer](FMovieSceneSequenceIDRef Ref)
	{
		if (WeakSequencer.IsValid())
		{
			TSharedRef<ISequencer> Sequencer = WeakSequencer.Pin().ToSharedRef();
			UMovieSceneSequence* Sequence = Sequencer->GetFocusedMovieSceneSequence();
			if (UControlRigSequence* ControlRigSequence = ExactCast<UControlRigSequence>(Sequence))
			{
				GLevelEditorModeTools().ActivateMode(FControlRigEditMode::ModeName);

				if (FControlRigEditMode* ControlRigEditMode = static_cast<FControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FControlRigEditMode::ModeName)))
				{
					ControlRigEditMode->SetSequencer(Sequencer);
				}

				// auto-select the first spawnable we have
				UMovieScene* MovieScene = ControlRigSequence->GetMovieScene();
				if (MovieScene->GetSpawnableCount() > 0)
				{
					Sequencer->SelectObject(MovieScene->GetSpawnable(0).GetGuid());
				}
			}
			else
			{
				if (FControlRigEditMode* ControlRigEditMode = static_cast<FControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FControlRigEditMode::ModeName)))
				{
					ControlRigEditMode->SetSequencer(Sequencer);
					TArray<TWeakObjectPtr<>> SelectedObjects;
					TArray<FGuid> ObjectBindings;
					ControlRigEditMode->SetObjects(SelectedObjects, ObjectBindings);
				}
			}
		}
	};

	InSequencer->OnActivateSequence().AddLambda(HandleActivateSequence);

	// Call into activation callback to handle initial activation
	FMovieSceneSequenceID SequenceID = MovieSceneSequenceID::Root;
	HandleActivateSequence(SequenceID);

	InSequencer->GetSelectionChangedObjectGuids().AddLambda([WeakSequencer](TArray<FGuid> InObjectBindings)
	{
		if (WeakSequencer.IsValid())
		{
			TSharedRef<ISequencer> Sequencer = WeakSequencer.Pin().ToSharedRef();
			UMovieSceneSequence* Sequence = Sequencer->GetFocusedMovieSceneSequence();
			if (UControlRigSequence* ControlRigSequence = ExactCast<UControlRigSequence>(Sequence))
			{
				TArray<TWeakObjectPtr<>> SelectedObjects;

				for (const FGuid& Guid : InObjectBindings)
				{
					TArrayView<TWeakObjectPtr<>> BoundObjects = Sequencer->FindBoundObjects(Guid, Sequencer->GetFocusedTemplateID());
					SelectedObjects.Append(BoundObjects.GetData(), BoundObjects.Num());
				}

				// @TODO: allow binding to sub-editor mode tools for use in animation editors
				if (SelectedObjects.Num() > 0)
				{
					GLevelEditorModeTools().ActivateMode(FControlRigEditMode::ModeName);
					if (FControlRigEditMode* ControlRigEditMode = static_cast<FControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FControlRigEditMode::ModeName)))
					{
						ControlRigEditMode->SetSequencer(Sequencer);
						ControlRigEditMode->SetObjects(SelectedObjects, InObjectBindings);
					}
				}
			}
		}
	});
}

void FControlRigEditorModule::OnInitializeSequence(UControlRigSequence* Sequence)
{
	auto* ProjectSettings = GetDefault<UMovieSceneToolsProjectSettings>();
	Sequence->GetMovieScene()->SetPlaybackRange(ProjectSettings->DefaultStartTime, ProjectSettings->DefaultStartTime + ProjectSettings->DefaultDuration);
}

IMPLEMENT_MODULE(FControlRigEditorModule, ControlRigEditor)

#undef LOCTEXT_NAMESPACE
