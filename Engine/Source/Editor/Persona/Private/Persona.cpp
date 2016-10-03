// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"
#include "Persona.h"
#include "SPersonaToolbar.h"
#include "PersonaModule.h"
#include "AnimGraphDefinitions.h"
#include "IDetailsView.h"

#include "Toolkits/IToolkitHost.h"

// this one needed?
#include "SAnimationEditorViewport.h"
// end of questionable

#include "SSequenceEditor.h"
#include "SMontageEditor.h"
#include "SAnimCompositeEditor.h"
#include "SAnimationBlendSpace.h"
#include "SAnimationBlendSpace1D.h"
#include "SKismetInspector.h"
#include "SSkeletonWidget.h"
#include "SPoseEditor.h"

#include "Editor/Kismet/Public/BlueprintEditorTabs.h"
#include "Editor/Kismet/Public/BlueprintEditorModes.h"

#include "ScopedTransaction.h"
#include "Editor/UnrealEd/Public/EdGraphUtilities.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "Editor/UnrealEd/Public/Kismet2/DebuggerCommands.h"

//#include "SkeletonMode/SkeletonMode.h"
#include "MeshMode/MeshMode.h"
#include "PhysicsMode/PhysicsMode.h"
#include "AnimationMode/AnimationMode.h"
#include "BlueprintMode/AnimBlueprintMode.h"
#include "Runtime/AssetRegistry/Public/AssetRegistryModule.h"

#include "Editor/Persona/Private/AnimationEditorViewportClient.h"

#include "ComponentAssetBroker.h"
#include "AnimGraphNode_BlendListByInt.h"
#include "AnimGraphNode_BlendSpaceEvaluator.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_SequenceEvaluator.h"
#include "AnimGraphNode_PoseByName.h"
#include "AnimGraphNode_PoseBlendNode.h"
#include "AnimGraphNode_Slot.h"
#include "Customization/AnimGraphNodeSlotDetails.h"
#include "AnimGraphNode_AimOffsetLookAt.h"
#include "AnimGraphNode_RotationOffsetBlendSpace.h"

#include "AnimPreviewInstance.h"

#include "Particles/ParticleSystemComponent.h"

#include "SAnimationDlgs.h"
#include "AssetToolsModule.h"
#include "FbxMeshUtils.h"
#include "AnimationEditorUtils.h"
#include "SAnimationSequenceBrowser.h"
#include "SDockTab.h"
#include "GenericCommands.h"
#include "SNotificationList.h"
#include "NotificationManager.h"

#include "Animation/AimOffsetBlendSpace.h"
#include "Animation/AimOffsetBlendSpace1D.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Animation/PoseAsset.h"

#include "MessageLog.h"
#include "SAdvancedPreviewDetailsTab.h"
#include "PhysicsEngine/PhysicsAsset.h"

#include "Editor/AnimGraph/Public/AnimGraphCommands.h"
#include "PersonaUtils.h"
#include "IPersonaToolkit.h"
#include "ISkeletonEditorModule.h"
#include "SMorphTargetViewer.h"
#include "SSkeletonAnimNotifies.h"
#include "IEditableSkeleton.h"
#include "SBlueprintEditorToolbar.h"
#include "IPersonaEditorModeManager.h"
#include "AnimationGraph.h"

#define LOCTEXT_NAMESPACE "FPersona"

const FName FPersona::PreviewSceneSettingsTabId(TEXT("Persona_PreviewScene"));

/////////////////////////////////////////////////////
// FLocalCharEditorCallbacks

struct FLocalCharEditorCallbacks
{
	static FText GetObjectName(UObject* Object)
	{
		return FText::FromString( Object->GetName() );
	}
};

//////////////////////////////////////////////////////
/////////////////////////////////////////////////////
// FSkeletonEditAppMode

class FSkeletonEditAppMode : public FPersonaAppMode
{
public:
	FSkeletonEditAppMode(TSharedPtr<FPersona> InPersona)
		: FPersonaAppMode(InPersona, FPersonaModes::SkeletonDisplayMode)
	{
		PersonaTabFactories.RegisterFactory(MakeShareable(new FSelectionDetailsSummoner(InPersona)));

		TabLayout = FTabManager::NewLayout( "Persona_SkeletonEditMode_Layout_v3" )
			->AddArea
			(
				FTabManager::NewPrimaryArea() ->SetOrientation(Orient_Vertical)
				->Split
				(
					// Top toolbar area
					FTabManager::NewStack()
					->SetSizeCoefficient(0.186721f)
					->SetHideTabWell(true)
					->AddTab( InPersona->GetToolbarTabId(), ETabState::OpenedTab )
				)
				->Split
				(
					// Rest of screen
					FTabManager::NewSplitter() ->SetOrientation(Orient_Horizontal)
					->Split
					(
						// Left 1/3rd - Skeleton tree and mesh panel
						FTabManager::NewSplitter()
						->SetOrientation(Orient_Vertical)
						->SetSizeCoefficient(0.3f)
						->Split
						(
							FTabManager::NewStack()
							->AddTab( FPersonaTabs::SkeletonTreeViewID, ETabState::OpenedTab )
						)
					)
					->Split
					(
						// Middle 1/3rd - Viewport
						FTabManager::NewSplitter()
						->SetOrientation(Orient_Vertical)
						->SetSizeCoefficient(0.5f)
						->Split
						(
							FTabManager::NewStack()
							->SetHideTabWell(true)
							->AddTab( FPersonaTabs::PreviewViewportID, ETabState::OpenedTab )
						)
					)
					->Split
					(
						// Right 1/3rd - Details panel 
						FTabManager::NewSplitter()
						->SetOrientation(Orient_Vertical)
						->SetSizeCoefficient(0.2f)
						->Split
						(
							FTabManager::NewStack()
							->AddTab(FPersonaTabs::AdvancedPreviewSceneSettingsID, ETabState::OpenedTab)
							->AddTab( FBlueprintEditorTabs::DetailsID, ETabState::OpenedTab )	//@TODO: FPersonaTabs::AnimPropertiesID
						)
					)
				)
			);

	}
};


/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

TArray<UObject*> GetEditorObjectsOfClass( const TArray< UObject* >& Objects, const UClass* ClassToFind )
{
	TArray<UObject*> ObjectsToReturn;
	for( auto ObjectIter = Objects.CreateConstIterator(); ObjectIter; ++ObjectIter )
	{
		// Don't allow user to perform certain actions on objects that aren't actually assets (e.g. Level Script blueprint objects)
		const auto EditingObject = *ObjectIter;
		if( EditingObject != NULL && EditingObject->IsAsset() && EditingObject->GetClass()->IsChildOf( ClassToFind ) )
		{
			ObjectsToReturn.Add(EditingObject);
		}
	}
	return ObjectsToReturn;
}

/////////////////////////////////////////////////////
// FPersona

FPersona::FPersona()
	: PersonaMeshDetailLayout(NULL)
{
	// Register to be notified when properties are edited
	OnPropertyChangedHandle = FCoreUObjectDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(this, &FPersona::OnPropertyChanged);
	OnPropertyChangedHandleDelegateHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.Add(OnPropertyChangedHandle);

	GEditor->OnBlueprintPreCompile().AddRaw(this, &FPersona::OnBlueprintPreCompile);
}

FPersona::~FPersona()
{
	GEditor->OnBlueprintPreCompile().RemoveAll(this);

	FEditorDelegates::OnAssetPostImport.RemoveAll(this);
	FReimportManager::Instance()->OnPostReimport().RemoveAll(this);

	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(OnPropertyChangedHandleDelegateHandle);

	// NOTE: Any tabs that we still have hanging out when destroyed will be cleaned up by FBaseToolkit's destructor
}

void FPersona::ReinitMode()
{
	FName CurrentMode = GetCurrentMode();

	if (CurrentMode == FPersonaModes::AnimationEditMode)
	{
		// if opening animation edit mode, open the animation editor
		OpenNewAnimationDocumentTab(GetPreviewScene()->GetPreviewAnimationAsset());
	}
}

TSharedPtr<SDockTab> FPersona::OpenNewAnimationDocumentTab(UAnimationAsset* InAnimAsset)
{
	TSharedPtr<SDockTab> OpenedTab;

	if (InAnimAsset != NULL)
	{
		FString	DocumentLink;

		FAnimDocumentArgs Args(GetPreviewScene(), GetPersonaToolkit(), GetSkeletonTree()->GetEditableSkeleton(), OnPostUndo, OnCurvesChanged, OnAnimNotifiesChanged, OnSectionsChanged);
		Args.OnDespatchObjectsSelected = FOnObjectsSelected::CreateSP(this, &FPersona::HandleObjectsSelected);
		Args.OnDespatchAnimNotifiesChanged = FSimpleDelegate::CreateSP(this, &FPersona::HandleAnimNotifiesChanged);
		Args.OnDespatchInvokeTab = FOnInvokeTab::CreateSP(this, &FAssetEditorToolkit::InvokeTab);
		Args.OnDespatchCurvesChanged = FSimpleDelegate::CreateSP(this, &FPersona::HandleCurvesChanged);
		Args.OnDespatchSectionsChanged = FSimpleDelegate::CreateSP(this, &FPersona::HandleSectionsChanged);

		FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");
		TSharedPtr<SWidget> TabContents = PersonaModule.CreateEditorWidgetForAnimDocument(SharedThis(this), InAnimAsset, Args, DocumentLink);
		if (TabContents.IsValid())
		{
			if ( GetPersonaToolkit()->GetAnimationAsset() )
			{
				RemoveEditingObject(GetPersonaToolkit()->GetAnimationAsset());
			}

			if ( InAnimAsset != NULL )
			{
				AddEditingObject(InAnimAsset);
			}

			GetPersonaToolkit()->SetAnimationAsset(InAnimAsset);
			TAttribute<FText> NameAttribute = TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateStatic(&FLocalCharEditorCallbacks::GetObjectName, (UObject*)InAnimAsset));

			if (SharedAnimDocumentTab.IsValid())
			{
				OpenedTab = SharedAnimDocumentTab.Pin();
				OpenedTab->SetContent(TabContents.ToSharedRef());
				OpenedTab->ActivateInParent(ETabActivationCause::SetDirectly);
				OpenedTab->SetLabel(NameAttribute);
				OpenedTab->SetLeftContent(IDocumentation::Get()->CreateAnchor(DocumentLink));
			}
			else
			{
				OpenedTab = SNew(SDockTab)
					.Label(NameAttribute)
					.OnTabClosed(this, &FPersona::OnEditTabClosed)
					.TabRole(ETabRole::DocumentTab)
					.TabColorScale(GetTabColorScale())
					[
						TabContents.ToSharedRef()
					];

				OpenedTab->SetLeftContent(IDocumentation::Get()->CreateAnchor(DocumentLink));
			
				TabManager->InsertNewDocumentTab("Document", FTabManager::ESearchPreference::RequireClosedTab, OpenedTab.ToSharedRef());

				SharedAnimDocumentTab = OpenedTab;
			}
		}

		if(SequenceBrowser.IsValid())
		{
			SequenceBrowser.Pin()->SelectAsset(InAnimAsset);
		}
	}

	return OpenedTab;
}

TSharedPtr<SDockTab> FPersona::OpenNewDocumentTab(class UAnimationAsset* InAnimAsset, bool bAddToHistory/* = true*/)
{
	/// before opening new asset, clear the currently selected object
	SetDetailObject(NULL);

	TSharedPtr<SDockTab> NewTab;
	if (InAnimAsset)
	{
		// add to history
		if (bAddToHistory && SequenceBrowser.IsValid())
		{
			SequenceBrowser.Pin()->AddToHistory(InAnimAsset);
		}

		// Are we allowed to open animation documents right now?
		//@TODO: Super-hacky check
		FName CurrentMode = GetCurrentMode();
		if (CurrentMode == FPersonaModes::AnimationEditMode)
		{
			NewTab = OpenNewAnimationDocumentTab(InAnimAsset);
		}

		GetPreviewScene()->SetPreviewAnimationAsset(InAnimAsset);
	}

	return NewTab;
}

void FPersona::SetViewport(TWeakPtr<class SAnimationEditorViewportTabBody> NewViewport)
{
	Viewport = NewViewport;
	OnViewportCreated.Broadcast(Viewport);
}

void FPersona::RefreshViewport()
{
	if (Viewport.IsValid())
	{
		Viewport.Pin()->RefreshViewport();
	}
}

UAnimBlueprint* FPersona::GetAnimBlueprint() const
{
	return Cast<UAnimBlueprint>(GetBlueprintObj());
}

void FPersona::ExtendMenu()
{
	// Add additional persona editor menus
	struct Local
	{
		static void AddPersonaSaveLoadMenu(FMenuBuilder& MenuBuilder)
		{
			MenuBuilder.AddMenuEntry(FPersonaCommands::Get().SaveAnimationAssets);
			MenuBuilder.AddMenuSeparator();
		}
		
		static void AddPersonaFileMenu(FMenuBuilder& MenuBuilder)
		{
			// View
			MenuBuilder.BeginSection("Persona", LOCTEXT("PersonaEditorMenu_File", "Blueprint"));
			{
			}
			MenuBuilder.EndSection();
		}

		static void AddPersonaAssetMenu(FMenuBuilder& MenuBuilder, FPersona* PersonaPtr)
		{
			// View
			MenuBuilder.BeginSection("Persona", LOCTEXT("PersonaAssetMenuMenu_Skeleton", "Skeleton"));
			{
				MenuBuilder.AddMenuEntry(FPersonaCommands::Get().ChangeSkeletonPreviewMesh);
				MenuBuilder.AddMenuEntry(FPersonaCommands::Get().RemoveUnusedBones);
				MenuBuilder.AddMenuEntry(FPersonaCommands::Get().UpdateSkeletonRefPose);
				MenuBuilder.AddMenuEntry(FPersonaCommands::Get().TestSkeletonCurveNamesForUse);
			}
			MenuBuilder.EndSection();

			//  Animation menu
			MenuBuilder.BeginSection("Persona", LOCTEXT("PersonaAssetMenuMenu_Animation", "Animation"));
			{
				MenuBuilder.AddMenuEntry(FPersonaCommands::Get().ApplyCompression);
				MenuBuilder.AddMenuEntry(FPersonaCommands::Get().ExportToFBX);
				MenuBuilder.AddMenuEntry(FPersonaCommands::Get().AddLoopingInterpolation);
			}
			MenuBuilder.EndSection();
		}
	};

	if(MenuExtender.IsValid())
	{
		RemoveMenuExtender(MenuExtender);
		MenuExtender.Reset();
	}

	MenuExtender = MakeShareable(new FExtender);

	UAnimBlueprint* AnimBlueprint = GetAnimBlueprint();
	if(AnimBlueprint)
	{
		TSharedPtr<FExtender> AnimBPMenuExtender = MakeShareable(new FExtender);
		FKismet2Menu::SetupBlueprintEditorMenu(AnimBPMenuExtender, *this);
		AddMenuExtender(AnimBPMenuExtender);

		AnimBPMenuExtender->AddMenuExtension(
			"FileLoadAndSave",
			EExtensionHook::After,
			GetToolkitCommands(),
			FMenuExtensionDelegate::CreateStatic(&Local::AddPersonaFileMenu));
	}

	MenuExtender->AddMenuExtension(
		"FileLoadAndSave",
		EExtensionHook::First,
		GetToolkitCommands(),
		FMenuExtensionDelegate::CreateStatic(&Local::AddPersonaSaveLoadMenu));

	MenuExtender->AddMenuExtension(
			"AssetEditorActions",
			EExtensionHook::After,
			GetToolkitCommands(),
			FMenuExtensionDelegate::CreateStatic(&Local::AddPersonaAssetMenu, this)
			);

	AddMenuExtender(MenuExtender);

	// add extensible menu if exists
	FPersonaModule* PersonaModule = &FModuleManager::LoadModuleChecked<FPersonaModule>("Persona");
	AddMenuExtender(PersonaModule->GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
}

void FPersona::InitPersona(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, USkeleton* InitSkeleton, UAnimBlueprint* InitAnimBlueprint, UAnimationAsset* InitAnimationAsset, class USkeletalMesh* InitMesh)
{
	FReimportManager::Instance()->OnPostReimport().AddRaw(this, &FPersona::OnPostReimport);

	AssetDirtyBrush = FEditorStyle::GetBrush("ContentBrowser.ContentDirty");

	if (!Toolbar.IsValid())
	{
		Toolbar = MakeShareable(new FBlueprintEditorToolbar(SharedThis(this)));
	}
	if (!PersonaToolbar.IsValid())
	{
		PersonaToolbar = MakeShareable(new FPersonaToolbar);
	}

	GetToolkitCommands()->Append(FPlayWorldCommands::GlobalPlayWorldActions.ToSharedRef());

	FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");

	// Determine the initial mode the app will open up in
	FName InitialMode = NAME_None;
	if (InitAnimBlueprint != NULL)
	{
		InitialMode = FPersonaModes::AnimBlueprintEditMode;
		PersonaToolkit = PersonaModule.CreatePersonaToolkit(InitAnimBlueprint);
	}
	else if (InitAnimationAsset != NULL)
	{
		InitialMode = FPersonaModes::AnimationEditMode;
		PersonaToolkit = PersonaModule.CreatePersonaToolkit(InitAnimationAsset);
	}
	else if (InitMesh != NULL)
	{
		InitialMode = FPersonaModes::MeshEditMode;
		PersonaToolkit = PersonaModule.CreatePersonaToolkit(InitMesh);
	}
	else
	{
		InitialMode = FPersonaModes::SkeletonDisplayMode;
		PersonaToolkit = PersonaModule.CreatePersonaToolkit(InitSkeleton);
	}

	// create the skeleton tree
	FSkeletonTreeArgs SkeletonTreeArgs(OnPostUndo);
	SkeletonTreeArgs.OnObjectSelected = FOnObjectSelected::CreateSP(this, &FPersona::HandleObjectSelected);
	SkeletonTreeArgs.PreviewScene = GetPreviewScene();

	ISkeletonEditorModule& SkeletonEditorModule = FModuleManager::LoadModuleChecked<ISkeletonEditorModule>("SkeletonEditor");
	SkeletonTree = SkeletonEditorModule.CreateSkeletonTree(PersonaToolkit->GetSkeleton(), PersonaToolkit->GetMesh(), SkeletonTreeArgs);

	// Create a list of available modes
	{
		TSharedPtr<FPersona> ThisPtr(SharedThis(this));

		TArray< TSharedRef<FApplicationMode> > TempModeList;
		TempModeList.Add(MakeShareable(new FSkeletonEditAppMode(ThisPtr)));
		TempModeList.Add(MakeShareable(new FMeshEditAppMode(ThisPtr)));
		//TempModeList.Add(MakeShareable(new FPhysicsEditAppMode(ThisPtr)));
		TempModeList.Add(MakeShareable(new FAnimEditAppMode(ThisPtr)));
		TempModeList.Add(MakeShareable(new FAnimBlueprintEditAppMode(ThisPtr)));

		for (auto ModeListIt = TempModeList.CreateIterator(); ModeListIt; ++ModeListIt)
		{
			TSharedRef<FApplicationMode> ApplicationMode = *ModeListIt;
			AddApplicationMode(ApplicationMode->GetModeName(), ApplicationMode);
		}
	}

	// Build up a list of objects being edited in this asset editor
	TArray<UObject*> ObjectsBeingEdited;

	if (InitAnimBlueprint != NULL)
	{
		ObjectsBeingEdited.Add(InitAnimBlueprint);
	} 

	// Purposefully skipping adding InitAnimationAsset here because it will get added when opening a document tab for it
	//if (InitAnimationAsset != NULL)
	//{
	//	ObjectsBeingEdited.Add(InitAnimationAsset);
	//}

	// Purposefully skipping adding InitMesh here because it will get added when setting the preview mesh
	//if (InitMesh != NULL)
	//{
	//	ObjectsBeingEdited.Add(InitMesh);
	//}

	USkeleton* Skeleton = PersonaToolkit->GetSkeleton();
	check(Skeleton);
	ObjectsBeingEdited.Add(Skeleton);

	Skeleton->CollectAnimationNotifies();

	// We could modify the skeleton within Persona (add/remove sockets), so we need to enable undo/redo on it
	Skeleton->SetFlags( RF_Transactional );

	// Initialize the asset editor and spawn tabs
	const TSharedRef<FTabManager::FLayout> DummyLayout = FTabManager::NewLayout("NullLayout")->AddArea(FTabManager::NewPrimaryArea());
	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	InitAssetEditor(Mode, InitToolkitHost, PersonaAppName, DummyLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, ObjectsBeingEdited);
	
	TArray<UBlueprint*> AnimBlueprints;
	if (InitAnimBlueprint)
	{
		AnimBlueprints.Add(InitAnimBlueprint);
	}
	CommonInitialization(AnimBlueprints);

	bool bSetMesh = false;

	// Set the mesh
	if (InitMesh != NULL)
	{
		SetPreviewMesh(InitMesh);
		bSetMesh = true;

		if (!Skeleton->GetPreviewMesh())
		{
			Skeleton->SetPreviewMesh(InitMesh, false);
		}
	}
	else if (InitAnimationAsset != NULL)
	{
		USkeletalMesh* AssetMesh = InitAnimationAsset->GetPreviewMesh();
		if (AssetMesh)
		{
			SetPreviewMesh(AssetMesh);
			bSetMesh = true;
		}
	}

	if (!bSetMesh && Skeleton)
	{
		//If no preview mesh set, just find the first mesh that uses this skeleton
		USkeletalMesh* PreviewMesh = Skeleton->GetPreviewMesh(true);
		if ( PreviewMesh )
		{
			SetPreviewMesh( PreviewMesh );
		}
	}

	// Force validation of preview attached assets (catch case of never doing it if we dont have a valid preview mesh)
	GetPreviewScene()->ValidatePreviewAttachedAssets(nullptr);

	UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
	UAnimBlueprint* AnimBlueprint = PersonaToolkit->GetAnimBlueprint();
	PreviewMeshComponent->SetAnimInstanceClass(AnimBlueprint ? AnimBlueprint->GeneratedClass : NULL);

	// We always want a preview instance unless we are using blueprints so that bone manipulation works
	if (AnimBlueprint == NULL)
	{
		PreviewMeshComponent->EnablePreview(true, nullptr);
	}
	else
	{
		// Make sure the object being debugged is the preview instance
		AnimBlueprint->SetObjectBeingDebugged(PreviewMeshComponent->GetAnimInstance());
	}

	ExtendMenu();
	ExtendDefaultPersonaToolbar();
	RegenerateMenusAndToolbars();

	// Activate the initial mode (which will populate with a real layout)
	SetCurrentMode(InitialMode);
	check(CurrentAppModePtr.IsValid());

	// set up our editor mode
	check(AssetEditorModeManager);
	AssetEditorModeManager->SetDefaultMode(FPersonaEditModes::SkeletonSelection);

	// Post-layout initialization
	PostLayoutBlueprintEditorInitialization();

	//@TODO: Push somewhere else?
	if (InitAnimationAsset != NULL)
	{
		OpenNewDocumentTab( InitAnimationAsset );
	}

	// register customization of Slot node for this Persona
	// this is so that you can open the manage window per Persona
	PersonaModule.CustomizeSlotNodeDetails(Inspector->GetPropertyView().ToSharedRef(), FOnInvokeTab::CreateSP(this, &FAssetEditorToolkit::InvokeTab));

	// Register post import callback to catch animation imports when we have the asset open (we need to reinit)
	FEditorDelegates::OnAssetPostImport.AddRaw(this, &FPersona::OnPostImport);

	
}
	
void FPersona::CreateAnimation(const TArray<UObject*> NewAssets, int32 Option) 
{
	bool bResult = true;
	if (NewAssets.Num() > 0)
	{
		USkeletalMeshComponent* MeshComponent = PersonaToolkit->GetPreviewMeshComponent();
		UAnimSequence* Sequence = Cast<UAnimSequence> (GetPreviewScene()->GetPreviewAnimationAsset());

		for (auto NewAsset : NewAssets)
		{
			UAnimSequence* NewAnimSequence = Cast<UAnimSequence>(NewAsset);
			if (NewAnimSequence)
			{
				switch (Option)
				{
				case 0:
					bResult &= NewAnimSequence->CreateAnimation(MeshComponent->SkeletalMesh);
					break;
				case 1:
					bResult &= NewAnimSequence->CreateAnimation(MeshComponent);
					break;
				case 2:
					bResult &= NewAnimSequence->CreateAnimation(Sequence);
					break;
				}
			}
		}

		// if it contains error, warn them
		if (bResult)
		{
			OnAssetCreated(NewAssets);

			// if it created based on current mesh component, 
			if (Option == 1)
			{
				UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
				if (PreviewMeshComponent && PreviewMeshComponent->PreviewInstance)
				{
					PreviewMeshComponent->PreviewInstance->ResetModifiedBone();
				}
			}
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("FailedToCreateAsset", "Failed to create asset"));
		}
	}
}

void FPersona::CreatePoseAsset(const TArray<UObject*> NewAssets, int32 Option)
{
	bool bResult = false;
	if (NewAssets.Num() > 0)
	{
		UDebugSkelMeshComponent* PreviewComponent = GetPreviewScene()->GetPreviewMeshComponent();
		UAnimSequence* Sequence = Cast<UAnimSequence>(GetPreviewScene()->GetPreviewAnimationAsset());

		for (auto NewAsset : NewAssets)
		{
			UPoseAsset* NewPoseAsset = Cast<UPoseAsset>(NewAsset);
			if (NewPoseAsset)
			{
				switch (Option)
				{
				case 0:
					NewPoseAsset->AddOrUpdatePoseWithUniqueName(PreviewComponent);
					break;
				case 1:
					NewPoseAsset->CreatePoseFromAnimation(Sequence);
					break;
				}
				
				bResult = true;
			}
		}

		// if it contains error, warn them
		if (bResult)
		{
			OnAssetCreated(NewAssets);
			
			// if it created based on current mesh component, 
			if (Option == 0)
			{
				PreviewComponent->PreviewInstance->ResetModifiedBone();
			}
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("FailedToCreateAsset", "Failed to create asset"));
		}
	}
}

void FPersona::FillCreateAnimationMenu(FMenuBuilder& MenuBuilder) const
{
	TArray<TWeakObjectPtr<USkeleton>> Skeletons;

	Skeletons.Add(PersonaToolkit->GetSkeleton());

	// create rig
	MenuBuilder.BeginSection("CreateAnimationSubMenu", LOCTEXT("CreateAnimationSubMenuHeading", "Create Animation"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("CreateAnimation_RefPose", "From Reference Pose"),
			LOCTEXT("CreateAnimation_RefPose_Tooltip", "Create Animation from reference pose."),
			FSlateIcon(),
			FUIAction(
			FExecuteAction::CreateStatic(&AnimationEditorUtils::ExecuteNewAnimAsset<UAnimSequenceFactory, UAnimSequence>, Skeletons, FString("_Sequence"), FAnimAssetCreated::CreateSP(this, &FPersona::CreateAnimation, 0), false),
			FCanExecuteAction()
			)
			);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("CreateAnimation_CurrentPose", "From Current Pose"),
			LOCTEXT("CreateAnimation_CurrentPose_Tooltip", "Create Animation from current pose."),
			FSlateIcon(),
			FUIAction(
			FExecuteAction::CreateStatic(&AnimationEditorUtils::ExecuteNewAnimAsset<UAnimSequenceFactory, UAnimSequence>, Skeletons, FString("_Sequence"), FAnimAssetCreated::CreateSP(this, &FPersona::CreateAnimation, 1), false),
			FCanExecuteAction()
			)
			);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("CreateAnimation_CurrentAnimation", "From Current Animation"),
			LOCTEXT("CreateAnimation_CurrentAnimation_Tooltip", "Create Animation from current animation."),
			FSlateIcon(),
			FUIAction(
			FExecuteAction::CreateStatic(&AnimationEditorUtils::ExecuteNewAnimAsset<UAnimSequenceFactory, UAnimSequence>, Skeletons, FString("_Sequence"), FAnimAssetCreated::CreateSP(this, &FPersona::CreateAnimation, 2), false),
			FCanExecuteAction::CreateSP(this, &FPersona::HasValidAnimationSequencePlaying)
			)
			);
	}
	MenuBuilder.EndSection();
}

void FPersona::FillCreatePoseAssetMenu(FMenuBuilder& MenuBuilder) const
{
	TArray<TWeakObjectPtr<USkeleton>> Skeletons;

	Skeletons.Add(TargetSkeleton);

	// create pose asset
	MenuBuilder.BeginSection("CreatePoseAssetSubMenu", LOCTEXT("CreatePoseAssetSubMenuHeading", "Create PoseAsset"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("CreatePoseAsset_CurrentPose", "From Current Pose"),
			LOCTEXT("CreatePoseAsset_CurrentPose_Tooltip", "Create PoseAsset from current pose."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&AnimationEditorUtils::ExecuteNewAnimAsset<UPoseAssetFactory, UPoseAsset>, Skeletons, FString("_PoseAsset"), FAnimAssetCreated::CreateSP(this, &FPersona::CreatePoseAsset, 0), false),
				FCanExecuteAction()
				)
			);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("CreatePoseAsset_CurrentAnimation", "From Current Animation"),
			LOCTEXT("CreatePoseAsset_CurrentAnimation_Tooltip", "Create Animation from current animation."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&AnimationEditorUtils::ExecuteNewAnimAsset<UPoseAssetFactory, UPoseAsset>, Skeletons, FString("_PoseAsset"), FAnimAssetCreated::CreateSP(this, &FPersona::CreatePoseAsset, 1), false),
				FCanExecuteAction::CreateSP(this, &FPersona::HasValidAnimationSequencePlaying)
				)
			);
	}
	MenuBuilder.EndSection();

	// insert pose to pose asset
	MenuBuilder.BeginSection("InsertPoseSubMenuSection", LOCTEXT("InsertPoseSubMenuSubMenuHeading", "Insert Pose"));
	{
		MenuBuilder.AddSubMenu(
			LOCTEXT("InsertPoseSubmenu", "Insert Pose"),
			LOCTEXT("InsertPoseSubmenu_ToolTip", "Insert current pose to selected PoseAsset"),
			FNewMenuDelegate::CreateSP(this, &FPersona::FillInsertPoseMenu),
			false,
			FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.PoseAsset")
		);
	}
	MenuBuilder.EndSection();
}

void FPersona::FillInsertPoseMenu(FMenuBuilder& MenuBuilder) const
{
	FAssetPickerConfig AssetPickerConfig;
	
	USkeleton* Skeleton = GetPersonaToolkit()->GetSkeleton();

	/** The asset picker will only show skeletons */
	AssetPickerConfig.Filter.ClassNames.Add(*UPoseAsset::StaticClass()->GetName());
	AssetPickerConfig.Filter.bRecursiveClasses = false;
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.Filter.TagsAndValues.Add(TEXT("Skeleton"), FAssetData(Skeleton).GetExportTextName());

	/** The delegate that fires when an asset was selected */
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateRaw(this, &FPersona::InsertCurrentPoseToAsset);

	/** The default view mode should be a list view */
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	MenuBuilder.AddWidget(
		ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig),
		LOCTEXT("Select_Label", "")
	);
}

void FPersona::InsertCurrentPoseToAsset(const FAssetData& NewPoseAssetData)
{
	UPoseAsset* PoseAsset = Cast<UPoseAsset>(NewPoseAssetData.GetAsset());
	FScopedTransaction ScopedTransaction(LOCTEXT("InsertPose", "Insert Pose"));

	if (PoseAsset)
	{
		PoseAsset->Modify();

		UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
		if (PreviewMeshComponent)
		{
			FSmartName NewPoseName;

			bool bSuccess =  PoseAsset->AddOrUpdatePoseWithUniqueName(PreviewMeshComponent, &NewPoseName);

			if (bSuccess)
			{
				FFormatNamedArguments Args;
				Args.Add(TEXT("PoseAsset"), FText::FromString(PoseAsset->GetName()));
				Args.Add(TEXT("PoseName"), FText::FromName(NewPoseName.DisplayName));
				FNotificationInfo Info(FText::Format(LOCTEXT("InsertPoseSucceeded", "The current pose has inserted to {PoseAsset} with {PoseName}"), Args));
				Info.ExpireDuration = 7.0f;
				Info.bUseLargeFont = false;
				TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
				if (Notification.IsValid())
				{
					Notification->SetCompletionState(SNotificationItem::CS_Success);
				}
			}
			else
			{
				FFormatNamedArguments Args;
				Args.Add(TEXT("PoseAsset"), FText::FromString(PoseAsset->GetName()));
				FNotificationInfo Info(FText::Format(LOCTEXT("InsertPoseFailed", "Inserting pose to asset {PoseAsset} has failed"), Args));
				Info.ExpireDuration = 7.0f;
				Info.bUseLargeFont = false;
				TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
				if (Notification.IsValid())
				{
					Notification->SetCompletionState(SNotificationItem::CS_Fail);
				}
			}
		}
	}

	// it doesn't work well if I leave the window open. The delegate goes weired or it stop showing the popups. 
	FSlateApplication::Get().DismissAllMenus();
}

TSharedRef< SWidget > FPersona::GenerateCreateAssetMenu( USkeleton* Skeleton ) const
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, NULL);

	// Create Animation menu
	MenuBuilder.BeginSection("CreateAnimation", LOCTEXT("CreateAnimationMenuHeading", "Animation"));
	{
		// create menu
		MenuBuilder.AddSubMenu(
				LOCTEXT("CreateAnimationSubmenu", "Create Animation"),
				LOCTEXT("CreateAnimationSubmenu_ToolTip", "Create Animation for this skeleton"),
				FNewMenuDelegate::CreateSP(this, &FPersona::FillCreateAnimationMenu),
				false,
				FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.AnimSequence")
				);

		MenuBuilder.AddSubMenu(
			LOCTEXT("CreatePoseAssetSubmenu", "Create PoseAsset"),
			LOCTEXT("CreatePoseAsssetSubmenu_ToolTip", "Create PoseAsset for this skeleton"),
			FNewMenuDelegate::CreateSP(this, &FPersona::FillCreatePoseAssetMenu),
			false,
			FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.PoseAsset")
			);
	}
	MenuBuilder.EndSection();

	TArray<TWeakObjectPtr<USkeleton>> Skeletons;

	Skeletons.Add(Skeleton);

	AnimationEditorUtils::FillCreateAssetMenu(MenuBuilder, Skeletons, FAnimAssetCreated::CreateSP(this, &FPersona::OnAssetCreated), false);

	return MenuBuilder.MakeWidget();
}

void FPersona::OnAssetCreated(const TArray<UObject*> NewAssets) 
{
	if ( NewAssets.Num() > 0 )
	{
		FAssetRegistryModule::AssetCreated(NewAssets[0]);
		OpenNewDocumentTab(CastChecked<UAnimationAsset>(NewAssets[0]));
	}
}

void FPersona::ExtendDefaultPersonaToolbar()
{
	// If the ToolbarExtender is valid, remove it before rebuilding it
	if(ToolbarExtender.IsValid())
	{
		RemoveToolbarExtender(ToolbarExtender);
		ToolbarExtender.Reset();
	}

	ToolbarExtender = MakeShareable(new FExtender);

	PersonaToolbar->SetupPersonaToolbar(ToolbarExtender, SharedThis(this));

	// extend extra menu/toolbars
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder, USkeleton* Skeleton, FPersona* PersonaPtr)
		{
			ToolbarBuilder.BeginSection("Skeleton");
			{
				ToolbarBuilder.AddToolBarButton(FPersonaCommands::Get().TogglePreviewAsset, NAME_None, LOCTEXT("Toolbar_PreviewAsset", "Preview"), TAttribute<FText>(PersonaPtr, &FPersona::GetPreviewAssetTooltip));
				ToolbarBuilder.AddToolBarButton(FPersonaCommands::Get().ToggleReferencePose, NAME_None, LOCTEXT("Toolbar_ToggleReferencePose", "Ref Pose"), LOCTEXT("Toolbar_ToggleReferencePoseTooltip", "Show Reference Pose"));
				ToolbarBuilder.AddSeparator();
				ToolbarBuilder.AddToolBarButton(FPersonaCommands::Get().AnimNotifyWindow);
				ToolbarBuilder.AddToolBarButton(FPersonaCommands::Get().RetargetManager, NAME_None, LOCTEXT("Toolbar_RetargetManager", "Retarget Manager"));
				ToolbarBuilder.AddToolBarButton(FPersonaCommands::Get().ImportMesh);
				ToolbarBuilder.AddToolBarButton(FPersonaCommands::Get().ReimportMesh);

				// animation import menu
				ToolbarBuilder.AddToolBarButton(FPersonaCommands::Get().ImportAnimation, NAME_None, LOCTEXT("Toolbar_ImportAnimation", "Import"));
				ToolbarBuilder.AddToolBarButton(FPersonaCommands::Get().ReimportAnimation, NAME_None, LOCTEXT("Toolbar_ReimportAnimation", "Reimport"));
				ToolbarBuilder.AddToolBarButton(FPersonaCommands::Get().ExportToFBX, NAME_None, LOCTEXT("Toolbar_ExportToFBX", "Export"));
			}
			ToolbarBuilder.EndSection();

			ToolbarBuilder.BeginSection("Animation");
			{
				// create button
				{
					ToolbarBuilder.AddComboButton(
						FUIAction(
							FExecuteAction(),
							FCanExecuteAction(),
							FIsActionChecked(),
							FIsActionButtonVisible::CreateSP(PersonaPtr, &FPersona::IsInPersonaMode, FPersonaModes::AnimationEditMode)
							),
						FOnGetContent::CreateSP(PersonaPtr, &FPersona::GenerateCreateAssetMenu, Skeleton),
						LOCTEXT("OpenBlueprint_Label", "Create Asset"),
						LOCTEXT("OpenBlueprint_ToolTip", "Create Assets for this skeleton."),
						FSlateIcon(FEditorStyle::GetStyleSetName(), "Persona.CreateAsset")
						);
				}

				ToolbarBuilder.AddToolBarButton(FPersonaCommands::Get().ApplyCompression, NAME_None, LOCTEXT("Toolbar_ApplyCompression", "Compression"));
			}
			ToolbarBuilder.EndSection();

			ToolbarBuilder.BeginSection("Editing");
			{
				ToolbarBuilder.AddToolBarButton(FPersonaCommands::Get().SetKey, NAME_None, LOCTEXT("Toolbar_SetKey", "Key"));
				ToolbarBuilder.AddToolBarButton(FPersonaCommands::Get().ApplyAnimation, NAME_None, LOCTEXT("Toolbar_ApplyAnimation", "Apply"));
			}
			ToolbarBuilder.EndSection();
		}
	};


	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar, PersonaToolkit->GetSkeleton(), this)
		);

	AddToolbarExtender(ToolbarExtender);

	FPersonaModule* PersonaModule = &FModuleManager::LoadModuleChecked<FPersonaModule>("Persona");
	AddToolbarExtender(PersonaModule->GetToolBarExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
}

UBlueprint* FPersona::GetBlueprintObj() const
{
	const TArray<UObject*>& EditingObjs = GetEditingObjects();
	for (int32 i = 0; i < EditingObjs.Num(); ++i)
	{
		if (EditingObjs[i]->IsA<UAnimBlueprint>()) {return (UBlueprint*)EditingObjs[i];}
	}
	return nullptr;
}

UObject* FPersona::GetPreviewAnimationAsset() const
{
	return GetPreviewScene()->GetPreviewAnimationAsset();
}

UObject* FPersona::GetAnimationAssetBeingEdited() const
{
	return GetPersonaToolkit()->GetAnimationAsset();
}

void FPersona::SetPreviewAnimationAsset(UAnimationAsset* AnimAsset, bool bEnablePreview)
{
	return GetPreviewScene()->SetPreviewAnimationAsset(AnimAsset, bEnablePreview);
			}

void FPersona::SetDetailObjects(const TArray<UObject*>& InObjects)
{
	Inspector->ShowDetailsForObjects(InObjects);
}

void FPersona::SetDetailObject(UObject* Obj)
{
	TArray<UObject*> Objects;
	Objects.Add(Obj);
	SetDetailObjects(Objects);
}

void FPersona::OnPostReimport(UObject* InObject, bool bSuccess)
{
	if (bSuccess)
	{
		ConditionalRefreshEditor(InObject);
	}
}

void FPersona::OnPostImport(UFactory* InFactory, UObject* InObject)
{
	ConditionalRefreshEditor(InObject);
}

void FPersona::ConditionalRefreshEditor(UObject* InObject)
{
	bool bInterestingAsset = true;
	USkeleton* Skeleton = GetPersonaToolkit()->GetSkeleton();
	// Ignore if this is regarding a different object
	if(InObject != Skeleton && InObject != Skeleton->GetPreviewMesh() && InObject != GetAnimationAssetBeingEdited())
	{
		bInterestingAsset = false;
	}

	// Check that we aren't a montage that uses an incoming animation
	if(UAnimMontage* Montage = Cast<UAnimMontage>(GetAnimationAssetBeingEdited()))
	{
		for(FSlotAnimationTrack& Slot : Montage->SlotAnimTracks)
		{
			if(bInterestingAsset)
			{
				break;
			}

			for(FAnimSegment& Segment : Slot.AnimTrack.AnimSegments)
			{
				if(Segment.AnimReference == InObject)
				{
					bInterestingAsset = true;
					break;
				}
			}
		}
	}

	if(bInterestingAsset)
	{
		RefreshViewport();
		ReinitMode();
	}
}

/** Called when graph editor focus is changed */
void FPersona::OnGraphEditorFocused(const TSharedRef<class SGraphEditor>& InGraphEditor)
{
	// in the future, depending on which graph editor is this will act different
	FBlueprintEditor::OnGraphEditorFocused(InGraphEditor);

	// install callback to allow us to propagate pin default changes live to the preview
	UAnimationGraph* AnimationGraph = Cast<UAnimationGraph>(InGraphEditor->GetCurrentGraph());
	if (AnimationGraph)
	{
		OnPinDefaultValueChangedHandle = AnimationGraph->OnPinDefaultValueChanged.Add(FOnPinDefaultValueChanged::FDelegate::CreateSP(this, &FPersona::HandlePinDefaultValueChanged));
	}
}

void FPersona::OnGraphEditorBackgrounded(const TSharedRef<SGraphEditor>& InGraphEditor)
{
	FBlueprintEditor::OnGraphEditorBackgrounded(InGraphEditor);

	UAnimationGraph* AnimationGraph = Cast<UAnimationGraph>(InGraphEditor->GetCurrentGraph());
	if (AnimationGraph)
	{
		AnimationGraph->OnPinDefaultValueChanged.Remove(OnPinDefaultValueChangedHandle);
	}
}

/** Create Default Tabs **/
void FPersona::CreateDefaultCommands() 
{
	if (GetBlueprintObj())
	{
		FBlueprintEditor::CreateDefaultCommands();
	}
	else
	{
		ToolkitCommands->MapAction( FGenericCommands::Get().Undo, 
			FExecuteAction::CreateSP( this, &FPersona::UndoAction ));
		ToolkitCommands->MapAction( FGenericCommands::Get().Redo, 
			FExecuteAction::CreateSP( this, &FPersona::RedoAction ));
	}

	// now add default commands
	FPersonaCommands::Register();

	// save all animation assets
	ToolkitCommands->MapAction(FPersonaCommands::Get().SaveAnimationAssets,
		FExecuteAction::CreateSP(this, &FPersona::SaveAnimationAssets_Execute),
		FCanExecuteAction::CreateSP(this, &FPersona::CanSaveAnimationAssets)
		);

	ToolkitCommands->MapAction(FPersonaCommands::Get().ApplyCompression,
		FExecuteAction::CreateSP(this, &FPersona::OnApplyCompression),
		FCanExecuteAction::CreateSP(this, &FPersona::HasValidAnimationSequencePlaying),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FPersona::IsInPersonaMode, FPersonaModes::AnimationEditMode)
		);

	ToolkitCommands->MapAction(FPersonaCommands::Get().SetKey,
		FExecuteAction::CreateSP(this, &FPersona::OnSetKey),
		FCanExecuteAction::CreateSP(this, &FPersona::CanSetKey),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FPersona::IsInPersonaMode, FPersonaModes::AnimationEditMode)
		);

	ToolkitCommands->MapAction(FPersonaCommands::Get().ApplyAnimation,
			FExecuteAction::CreateSP(this, &FPersona::OnApplyRawAnimChanges),
			FCanExecuteAction::CreateSP(this, &FPersona::CanApplyRawAnimChanges),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP(this, &FPersona::IsInPersonaMode, FPersonaModes::AnimationEditMode)
			);

	ToolkitCommands->MapAction(FPersonaCommands::Get().ExportToFBX,
		FExecuteAction::CreateSP(this, &FPersona::OnExportToFBX),
		FCanExecuteAction::CreateSP(this, &FPersona::HasValidAnimationSequencePlaying),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FPersona::IsInPersonaMode, FPersonaModes::AnimationEditMode)
		);

	ToolkitCommands->MapAction(FPersonaCommands::Get().AddLoopingInterpolation,
		FExecuteAction::CreateSP(this, &FPersona::OnAddLoopingInterpolation),
		FCanExecuteAction::CreateSP(this, &FPersona::HasValidAnimationSequencePlaying),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FPersona::IsInPersonaMode, FPersonaModes::AnimationEditMode)
		);

	ToolkitCommands->MapAction( FPersonaCommands::Get().ChangeSkeletonPreviewMesh,
		FExecuteAction::CreateSP( this, &FPersona::ChangeSkeletonPreviewMesh ),
		FCanExecuteAction::CreateSP( this, &FPersona::CanChangeSkeletonPreviewMesh )
		);

	ToolkitCommands->MapAction(FPersonaCommands::Get().ToggleReferencePose,
	FExecuteAction::CreateSP(this, &FPersona::ShowReferencePose, true),
	FCanExecuteAction(),
	FIsActionChecked::CreateSP(this, &FPersona::IsShowReferencePoseEnabled)
	);

	ToolkitCommands->MapAction(FPersonaCommands::Get().TogglePreviewAsset,
	FExecuteAction::CreateSP(this, &FPersona::ShowReferencePose, false),
	FCanExecuteAction(),
	FIsActionChecked::CreateSP(this, &FPersona::IsPreviewAssetEnabled)
	);

	ToolkitCommands->MapAction(FPersonaCommands::Get().TogglePlay,
		FExecuteAction::CreateSP(this, &FPersona::TogglePlayback)
	);

	ToolkitCommands->MapAction( FPersonaCommands::Get().RemoveUnusedBones,
		FExecuteAction::CreateSP( this, &FPersona::RemoveUnusedBones ),
		FCanExecuteAction::CreateSP( this, &FPersona::CanRemoveBones )
		);
	ToolkitCommands->MapAction(FPersonaCommands::Get().TestSkeletonCurveNamesForUse,
		FExecuteAction::CreateSP(this, &FPersona::TestSkeletonCurveNamesForUse),
		FCanExecuteAction()
		);
	ToolkitCommands->MapAction(FPersonaCommands::Get().UpdateSkeletonRefPose,
		FExecuteAction::CreateSP(this, &FPersona::UpdateSkeletonRefPose)
		);

	ToolkitCommands->MapAction(FPersonaCommands::Get().AnimNotifyWindow,
		FExecuteAction::CreateSP(this, &FPersona::OnAnimNotifyWindow),
		FCanExecuteAction(),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FPersona::IsInPersonaMode, FPersonaModes::SkeletonDisplayMode)
		);

	ToolkitCommands->MapAction(FPersonaCommands::Get().RetargetManager,
		FExecuteAction::CreateSP(this, &FPersona::OnRetargetManager),
		FCanExecuteAction(),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FPersona::IsInPersonaMode, FPersonaModes::SkeletonDisplayMode)
		);

	ToolkitCommands->MapAction(FPersonaCommands::Get().ImportMesh,
		FExecuteAction::CreateSP(this, &FPersona::OnImportAsset, FBXIT_SkeletalMesh),
		FCanExecuteAction(),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FPersona::IsInPersonaMode, FPersonaModes::SkeletonDisplayMode)
		);

	ToolkitCommands->MapAction(FPersonaCommands::Get().ReimportMesh,
		FExecuteAction::CreateSP(this, &FPersona::OnReimportMesh),
		FCanExecuteAction(),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FPersona::IsInPersonaMode, FPersonaModes::MeshEditMode)
		);

// 	ToolkitCommands->MapAction(FPersonaCommands::Get().ImportLODs,
// 		FExecuteAction::CreateSP(this, &FPersona::OnImportLODs),
// 		FCanExecuteAction(),
// 		FIsActionChecked(),
// 		FIsActionButtonVisible::CreateSP(this, &FPersona::IsInPersonaMode, FPersonaModes::MeshEditMode)
// 		);

// 	ToolkitCommands->MapAction(FPersonaCommands::Get().AddBodyPart,
// 		FExecuteAction::CreateSP(this, &FPersona::OnAddBodyPart),
// 		FCanExecuteAction(),
// 		FIsActionChecked(),
// 		FIsActionButtonVisible::CreateSP(this, &FPersona::IsInPersonaMode, FPersonaModes::MeshEditMode)
// 		);


	// animation menu options
	// import animation
	ToolkitCommands->MapAction(FPersonaCommands::Get().ImportAnimation,
		FExecuteAction::CreateSP(this, &FPersona::OnImportAsset, FBXIT_Animation),
		FCanExecuteAction(),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FPersona::IsInPersonaMode, FPersonaModes::AnimationEditMode)
		);

	ToolkitCommands->MapAction(FPersonaCommands::Get().ReimportAnimation,
		FExecuteAction::CreateSP(this, &FPersona::OnReimportAnimation),
		FCanExecuteAction::CreateSP(this, &FPersona::HasValidAnimationSequencePlaying),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FPersona::IsInPersonaMode, FPersonaModes::AnimationEditMode)
		);
}

void FPersona::OnCreateGraphEditorCommands(TSharedPtr<FUICommandList> GraphEditorCommandsList)
{
	GraphEditorCommandsList->MapAction(FAnimGraphCommands::Get().TogglePoseWatch,
		FExecuteAction::CreateSP(this, &FPersona::OnTogglePoseWatch));
}

bool FPersona::CanSelectBone() const
{
	return true;
}

void FPersona::OnSelectBone()
{
	//@TODO: A2REMOVAL: This doesn't do anything 
}

void FPersona::OnAddPosePin()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	if (SelectedNodes.Num() == 1)
	{
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			UObject* Node = *NodeIt;

			if (UAnimGraphNode_BlendListByInt* BlendNode = Cast<UAnimGraphNode_BlendListByInt>(Node))
			{
				BlendNode->AddPinToBlendList();
				break;
			}
			else if (UAnimGraphNode_LayeredBoneBlend* FilterNode = Cast<UAnimGraphNode_LayeredBoneBlend>(Node))
			{
				FilterNode->AddPinToBlendByFilter();
				break;
			}
		}
	}
}

bool FPersona::CanAddPosePin() const
{
	return true;
}

void FPersona::OnRemovePosePin()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	UAnimGraphNode_BlendListByInt* BlendListIntNode = NULL;
	UAnimGraphNode_LayeredBoneBlend* BlendByFilterNode = NULL;

	if (SelectedNodes.Num() == 1)
	{
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			if (UAnimGraphNode_BlendListByInt* BlendNode = Cast<UAnimGraphNode_BlendListByInt>(*NodeIt))
			{
				BlendListIntNode = BlendNode;
				break;
			}
			else if (UAnimGraphNode_LayeredBoneBlend* LayeredBlendNode = Cast<UAnimGraphNode_LayeredBoneBlend>(*NodeIt))
			{
				BlendByFilterNode = LayeredBlendNode;
				break;
			}		
		}
	}


	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (FocusedGraphEd.IsValid())
	{
		// @fixme: I think we can make blendlistbase to have common functionality
		// and each can implement the common function, but for now, we separate them
		// each implement their menu, so we still can use listbase as the root
		if (BlendListIntNode)
		{
			// make sure we at least have BlendListNode selected
			UEdGraphPin* SelectedPin = FocusedGraphEd->GetGraphPinForMenu();

			BlendListIntNode->RemovePinFromBlendList(SelectedPin);

			// Update the graph so that the node will be refreshed
			FocusedGraphEd->NotifyGraphChanged();
		}

		if (BlendByFilterNode)
		{
			// make sure we at least have BlendListNode selected
			UEdGraphPin* SelectedPin = FocusedGraphEd->GetGraphPinForMenu();

			BlendByFilterNode->RemovePinFromBlendByFilter(SelectedPin);

			// Update the graph so that the node will be refreshed
			FocusedGraphEd->NotifyGraphChanged();
		}
	}
}

void FPersona::OnConvertToSequenceEvaluator()
{
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	if (SelectedNodes.Num() > 0)
	{
		for (auto NodeIter = SelectedNodes.CreateIterator(); NodeIter; ++NodeIter)
		{
			UAnimGraphNode_SequencePlayer* OldNode = Cast<UAnimGraphNode_SequencePlayer>(*NodeIter);

			// see if sequence player
			if ( OldNode && OldNode->Node.Sequence )
			{
				//const FScopedTransaction Transaction( LOCTEXT("ConvertToSequenceEvaluator", "Convert to Single Frame Animation") );

				// convert to sequence evaluator
				UEdGraph* TargetGraph = OldNode->GetGraph();
				// create new evaluator
				FGraphNodeCreator<UAnimGraphNode_SequenceEvaluator> NodeCreator(*TargetGraph);
				UAnimGraphNode_SequenceEvaluator* NewNode = NodeCreator.CreateNode();
				NewNode->Node.Sequence = OldNode->Node.Sequence;
				NodeCreator.Finalize();

				// get default data from old node to new node
				FEdGraphUtilities::CopyCommonState(OldNode, NewNode);

				UEdGraphPin* OldPosePin = OldNode->FindPin(TEXT("Pose"));
				UEdGraphPin* NewPosePin = NewNode->FindPin(TEXT("Pose"));

				if (ensure(OldPosePin && NewPosePin))
				{
					NewPosePin->CopyPersistentDataFromOldPin(*OldPosePin);
				}

				// remove from selection and from graph
				NodeIter.RemoveCurrent();
				TargetGraph->RemoveNode(OldNode);

				NewNode->Modify();
			}
		}

		// @todo fixme: below code doesn't work
		// because of SetAndCenterObject kicks in after new node is added
		// will need to disable that first
		TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();

		// Update the graph so that the node will be refreshed
		FocusedGraphEd->NotifyGraphChanged();
		// It's possible to leave invalid objects in the selection set if they get GC'd, so clear it out
		FocusedGraphEd->ClearSelectionSet();

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetAnimBlueprint());
	}
}

void FPersona::OnConvertToSequencePlayer()
{
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	if (SelectedNodes.Num() > 0)
	{
		for (auto NodeIter = SelectedNodes.CreateIterator(); NodeIter; ++NodeIter)
		{
			UAnimGraphNode_SequenceEvaluator* OldNode = Cast<UAnimGraphNode_SequenceEvaluator>(*NodeIter);

			// see if sequence player
			if ( OldNode && OldNode->Node.Sequence )
			{
				//const FScopedTransaction Transaction( LOCTEXT("ConvertToSequenceEvaluator", "Convert to Single Frame Animation") );
				// convert to sequence player
				UEdGraph* TargetGraph = OldNode->GetGraph();
				// create new player
				FGraphNodeCreator<UAnimGraphNode_SequencePlayer> NodeCreator(*TargetGraph);
				UAnimGraphNode_SequencePlayer* NewNode = NodeCreator.CreateNode();
				NewNode->Node.Sequence = OldNode->Node.Sequence;
				NodeCreator.Finalize();

				// get default data from old node to new node
				FEdGraphUtilities::CopyCommonState(OldNode, NewNode);

				UEdGraphPin* OldPosePin = OldNode->FindPin(TEXT("Pose"));
				UEdGraphPin* NewPosePin = NewNode->FindPin(TEXT("Pose"));

				if (ensure(OldPosePin && NewPosePin))
				{
					NewPosePin->CopyPersistentDataFromOldPin(*OldPosePin);
				}

				// remove from selection and from graph
				NodeIter.RemoveCurrent();
				TargetGraph->RemoveNode(OldNode);

				NewNode->Modify();
			}
		}

		// @todo fixme: below code doesn't work
		// because of SetAndCenterObject kicks in after new node is added
		// will need to disable that first
		TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();

		// Update the graph so that the node will be refreshed
		FocusedGraphEd->NotifyGraphChanged();
		// It's possible to leave invalid objects in the selection set if they get GC'd, so clear it out
		FocusedGraphEd->ClearSelectionSet();

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetAnimBlueprint());
	}
}

void FPersona::OnConvertToBlendSpaceEvaluator() 
{
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	if (SelectedNodes.Num() > 0)
	{
		for (auto NodeIter = SelectedNodes.CreateIterator(); NodeIter; ++NodeIter)
		{
			UAnimGraphNode_BlendSpacePlayer* OldNode = Cast<UAnimGraphNode_BlendSpacePlayer>(*NodeIter);

			// see if sequence player
			if ( OldNode && OldNode->Node.BlendSpace )
			{
				//const FScopedTransaction Transaction( LOCTEXT("ConvertToSequenceEvaluator", "Convert to Single Frame Animation") );

				// convert to sequence evaluator
				UEdGraph* TargetGraph = OldNode->GetGraph();
				// create new evaluator
				FGraphNodeCreator<UAnimGraphNode_BlendSpaceEvaluator> NodeCreator(*TargetGraph);
				UAnimGraphNode_BlendSpaceEvaluator* NewNode = NodeCreator.CreateNode();
				NewNode->Node.BlendSpace = OldNode->Node.BlendSpace;
				NodeCreator.Finalize();

				// get default data from old node to new node
				FEdGraphUtilities::CopyCommonState(OldNode, NewNode);

				UEdGraphPin* OldPosePin = OldNode->FindPin(TEXT("X"));
				UEdGraphPin* NewPosePin = NewNode->FindPin(TEXT("X"));

				if (ensure(OldPosePin && NewPosePin))
				{
					NewPosePin->CopyPersistentDataFromOldPin(*OldPosePin);
				}

				OldPosePin = OldNode->FindPin(TEXT("Y"));
				NewPosePin = NewNode->FindPin(TEXT("Y"));

				if (ensure(OldPosePin && NewPosePin))
				{
					NewPosePin->CopyPersistentDataFromOldPin(*OldPosePin);
				}


				OldPosePin = OldNode->FindPin(TEXT("Pose"));
				NewPosePin = NewNode->FindPin(TEXT("Pose"));

				if (ensure(OldPosePin && NewPosePin))
				{
					NewPosePin->CopyPersistentDataFromOldPin(*OldPosePin);
				}

				// remove from selection and from graph
				NodeIter.RemoveCurrent();
				TargetGraph->RemoveNode(OldNode);

				NewNode->Modify();
			}
		}

		// @todo fixme: below code doesn't work
		// because of SetAndCenterObject kicks in after new node is added
		// will need to disable that first
		TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();

		// Update the graph so that the node will be refreshed
		FocusedGraphEd->NotifyGraphChanged();
		// It's possible to leave invalid objects in the selection set if they get GC'd, so clear it out
		FocusedGraphEd->ClearSelectionSet();

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetAnimBlueprint());
	}
}
void FPersona::OnConvertToBlendSpacePlayer() 
{
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	if (SelectedNodes.Num() > 0)
	{
		for (auto NodeIter = SelectedNodes.CreateIterator(); NodeIter; ++NodeIter)
		{
			UAnimGraphNode_BlendSpaceEvaluator* OldNode = Cast<UAnimGraphNode_BlendSpaceEvaluator>(*NodeIter);

			// see if sequence player
			if ( OldNode && OldNode->Node.BlendSpace )
			{
				//const FScopedTransaction Transaction( LOCTEXT("ConvertToSequenceEvaluator", "Convert to Single Frame Animation") );
				// convert to sequence player
				UEdGraph* TargetGraph = OldNode->GetGraph();
				// create new player
				FGraphNodeCreator<UAnimGraphNode_BlendSpacePlayer> NodeCreator(*TargetGraph);
				UAnimGraphNode_BlendSpacePlayer* NewNode = NodeCreator.CreateNode();
				NewNode->Node.BlendSpace = OldNode->Node.BlendSpace;
				NodeCreator.Finalize();

				// get default data from old node to new node
				FEdGraphUtilities::CopyCommonState(OldNode, NewNode);

				UEdGraphPin* OldPosePin = OldNode->FindPin(TEXT("X"));
				UEdGraphPin* NewPosePin = NewNode->FindPin(TEXT("X"));

				if (ensure(OldPosePin && NewPosePin))
				{
					NewPosePin->CopyPersistentDataFromOldPin(*OldPosePin);
				}

				OldPosePin = OldNode->FindPin(TEXT("Y"));
				NewPosePin = NewNode->FindPin(TEXT("Y"));

				if (ensure(OldPosePin && NewPosePin))
				{
					NewPosePin->CopyPersistentDataFromOldPin(*OldPosePin);
				}


				OldPosePin = OldNode->FindPin(TEXT("Pose"));
				NewPosePin = NewNode->FindPin(TEXT("Pose"));

				if (ensure(OldPosePin && NewPosePin))
				{
					NewPosePin->CopyPersistentDataFromOldPin(*OldPosePin);
				}

				// remove from selection and from graph
				NodeIter.RemoveCurrent();
				TargetGraph->RemoveNode(OldNode);

				NewNode->Modify();
			}
		}

		// @todo fixme: below code doesn't work
		// because of SetAndCenterObject kicks in after new node is added
		// will need to disable that first
		TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
		// Update the graph so that the node will be refreshed
		FocusedGraphEd->NotifyGraphChanged();
		// It's possible to leave invalid objects in the selection set if they get GC'd, so clear it out
		FocusedGraphEd->ClearSelectionSet();

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetAnimBlueprint());
	}
}

void FPersona::OnConvertToAimOffsetLookAt()
{
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	if (SelectedNodes.Num() > 0)
	{
		for (auto NodeIter = SelectedNodes.CreateIterator(); NodeIter; ++NodeIter)
		{
			UAnimGraphNode_RotationOffsetBlendSpace* OldNode = Cast<UAnimGraphNode_RotationOffsetBlendSpace>(*NodeIter);

			// see if sequence player
			if (OldNode && OldNode->Node.BlendSpace)
			{
				//const FScopedTransaction Transaction( LOCTEXT("ConvertToSequenceEvaluator", "Convert to Single Frame Animation") );

				// convert to sequence evaluator
				UEdGraph* TargetGraph = OldNode->GetGraph();
				// create new evaluator
				FGraphNodeCreator<UAnimGraphNode_AimOffsetLookAt> NodeCreator(*TargetGraph);
				UAnimGraphNode_AimOffsetLookAt* NewNode = NodeCreator.CreateNode();
				NewNode->Node.BlendSpace = OldNode->Node.BlendSpace;
				NodeCreator.Finalize();

				// get default data from old node to new node
				FEdGraphUtilities::CopyCommonState(OldNode, NewNode);

				UEdGraphPin* OldPosePin = OldNode->FindPin(TEXT("Pose"));
				UEdGraphPin* NewPosePin = NewNode->FindPin(TEXT("Pose"));

				if (ensure(OldPosePin && NewPosePin))
				{
					NewPosePin->CopyPersistentDataFromOldPin(*OldPosePin);
				}

				OldPosePin = OldNode->FindPin(TEXT("BasePose"));
				NewPosePin = NewNode->FindPin(TEXT("BasePose"));

				if (ensure(OldPosePin && NewPosePin))
				{
					NewPosePin->CopyPersistentDataFromOldPin(*OldPosePin);
				}

				// remove from selection and from graph
				NodeIter.RemoveCurrent();
				TargetGraph->RemoveNode(OldNode);

				NewNode->Modify();
			}
		}

		// @todo fixme: below code doesn't work
		// because of SetAndCenterObject kicks in after new node is added
		// will need to disable that first
		TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();

		// Update the graph so that the node will be refreshed
		FocusedGraphEd->NotifyGraphChanged();
		// It's possible to leave invalid objects in the selection set if they get GC'd, so clear it out
		FocusedGraphEd->ClearSelectionSet();

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetAnimBlueprint());
	}
}

void FPersona::OnConvertToAimOffsetSimple()
{
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	if (SelectedNodes.Num() > 0)
	{
		for (auto NodeIter = SelectedNodes.CreateIterator(); NodeIter; ++NodeIter)
		{
			UAnimGraphNode_AimOffsetLookAt* OldNode = Cast<UAnimGraphNode_AimOffsetLookAt>(*NodeIter);

			// see if sequence player
			if (OldNode && OldNode->Node.BlendSpace)
			{
				//const FScopedTransaction Transaction( LOCTEXT("ConvertToSequenceEvaluator", "Convert to Single Frame Animation") );
				// convert to sequence player
				UEdGraph* TargetGraph = OldNode->GetGraph();
				// create new player
				FGraphNodeCreator<UAnimGraphNode_RotationOffsetBlendSpace> NodeCreator(*TargetGraph);
				UAnimGraphNode_RotationOffsetBlendSpace* NewNode = NodeCreator.CreateNode();
				NewNode->Node.BlendSpace = OldNode->Node.BlendSpace;
				NodeCreator.Finalize();

				// get default data from old node to new node
				FEdGraphUtilities::CopyCommonState(OldNode, NewNode);

				UEdGraphPin* OldPosePin = OldNode->FindPin(TEXT("Pose"));
				UEdGraphPin* NewPosePin = NewNode->FindPin(TEXT("Pose"));

				if (ensure(OldPosePin && NewPosePin))
				{
					NewPosePin->CopyPersistentDataFromOldPin(*OldPosePin);
				}

				OldPosePin = OldNode->FindPin(TEXT("BasePose"));
				NewPosePin = NewNode->FindPin(TEXT("BasePose"));

				if (ensure(OldPosePin && NewPosePin))
				{
					NewPosePin->CopyPersistentDataFromOldPin(*OldPosePin);
				}

				// remove from selection and from graph
				NodeIter.RemoveCurrent();
				TargetGraph->RemoveNode(OldNode);

				NewNode->Modify();
			}
		}

		// @todo fixme: below code doesn't work
		// because of SetAndCenterObject kicks in after new node is added
		// will need to disable that first
		TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
		// Update the graph so that the node will be refreshed
		FocusedGraphEd->NotifyGraphChanged();
		// It's possible to leave invalid objects in the selection set if they get GC'd, so clear it out
		FocusedGraphEd->ClearSelectionSet();

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetAnimBlueprint());
	}
}

void FPersona::OnConvertToPoseBlender()
{
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	if (SelectedNodes.Num() > 0)
	{
		for (auto NodeIter = SelectedNodes.CreateIterator(); NodeIter; ++NodeIter)
		{
			UAnimGraphNode_PoseByName* OldNode = Cast<UAnimGraphNode_PoseByName>(*NodeIter);

			// see if sequence player
			if (OldNode && OldNode->Node.PoseAsset)
			{
				// convert to sequence player
				UEdGraph* TargetGraph = OldNode->GetGraph();
				// create new player
				FGraphNodeCreator<UAnimGraphNode_PoseBlendNode> NodeCreator(*TargetGraph);
				UAnimGraphNode_PoseBlendNode* NewNode = NodeCreator.CreateNode();
				NewNode->Node.PoseAsset = OldNode->Node.PoseAsset;
				NodeCreator.Finalize();

				// get default data from old node to new node
				FEdGraphUtilities::CopyCommonState(OldNode, NewNode);

				UEdGraphPin* OldPosePin = OldNode->FindPin(TEXT("Pose"));
				UEdGraphPin* NewPosePin = NewNode->FindPin(TEXT("Pose"));

				if (ensure(OldPosePin && NewPosePin))
				{
					NewPosePin->CopyPersistentDataFromOldPin(*OldPosePin);
				}

				// remove from selection and from graph
				NodeIter.RemoveCurrent();
				TargetGraph->RemoveNode(OldNode);

				NewNode->Modify();
			}
		}

		// @todo fixme: below code doesn't work
		// because of SetAndCenterObject kicks in after new node is added
		// will need to disable that first
		TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();

		// Update the graph so that the node will be refreshed
		FocusedGraphEd->NotifyGraphChanged();
		// It's possible to leave invalid objects in the selection set if they get GC'd, so clear it out
		FocusedGraphEd->ClearSelectionSet();

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetAnimBlueprint());
	}
}

void FPersona::OnConvertToPoseByName()
{
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	if (SelectedNodes.Num() > 0)
	{
		for (auto NodeIter = SelectedNodes.CreateIterator(); NodeIter; ++NodeIter)
		{
			UAnimGraphNode_PoseBlendNode* OldNode = Cast<UAnimGraphNode_PoseBlendNode>(*NodeIter);

			// see if sequence player
			if (OldNode && OldNode->Node.PoseAsset)
			{
				//const FScopedTransaction Transaction( LOCTEXT("ConvertToSequenceEvaluator", "Convert to Single Frame Animation") );
				// convert to sequence player
				UEdGraph* TargetGraph = OldNode->GetGraph();
				// create new player
				FGraphNodeCreator<UAnimGraphNode_PoseByName> NodeCreator(*TargetGraph);
				UAnimGraphNode_PoseByName* NewNode = NodeCreator.CreateNode();
				NewNode->Node.PoseAsset = OldNode->Node.PoseAsset;
				NodeCreator.Finalize();

				// get default data from old node to new node
				FEdGraphUtilities::CopyCommonState(OldNode, NewNode);

				UEdGraphPin* OldPosePin = OldNode->FindPin(TEXT("Pose"));
				UEdGraphPin* NewPosePin = NewNode->FindPin(TEXT("Pose"));

				if (ensure(OldPosePin && NewPosePin))
				{
					NewPosePin->CopyPersistentDataFromOldPin(*OldPosePin);
				}

				// remove from selection and from graph
				NodeIter.RemoveCurrent();
				TargetGraph->RemoveNode(OldNode);

				NewNode->Modify();
			}
		}

		// @todo fixme: below code doesn't work
		// because of SetAndCenterObject kicks in after new node is added
		// will need to disable that first
		TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();

		// Update the graph so that the node will be refreshed
		FocusedGraphEd->NotifyGraphChanged();
		// It's possible to leave invalid objects in the selection set if they get GC'd, so clear it out
		FocusedGraphEd->ClearSelectionSet();

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetAnimBlueprint());
	}
}

void FPersona::OnTogglePoseWatch()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	UAnimBlueprint* AnimBP = GetAnimBlueprint();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UAnimGraphNode_Base* SelectedNode = Cast<UAnimGraphNode_Base>(*NodeIt))
		{
			UPoseWatch* PoseWatch = AnimationEditorUtils::FindPoseWatchForNode(SelectedNode, AnimBP);
			if (PoseWatch)
			{
				AnimationEditorUtils::RemovePoseWatch(PoseWatch, AnimBP);
			}
			else
			{
				AnimationEditorUtils::MakePoseWatchForNode(AnimBP, SelectedNode, FColor::Red);
			}
		}
	}
}

void FPersona::OnOpenRelatedAsset()
{
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	
	EToolkitMode::Type Mode = EToolkitMode::Standalone;
	FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>( "Persona" );

	if (SelectedNodes.Num() > 0)
	{
		for (auto NodeIter = SelectedNodes.CreateIterator(); NodeIter; ++NodeIter)
		{
			if(UAnimGraphNode_Base* Node = Cast<UAnimGraphNode_Base>(*NodeIter))
			{
				UAnimationAsset* AnimAsset = Node->GetAnimationAsset();
				
				if(AnimAsset)
				{
					PersonaModule.CreatePersona( Mode, TSharedPtr<IToolkitHost>(), AnimAsset->GetSkeleton(), NULL, AnimAsset, NULL );
				}
			}
		}
	}
}

bool FPersona::CanRemovePosePin() const
{
	return true;
}

void FPersona::RecompileAnimBlueprintIfDirty()
{
	if (UBlueprint* Blueprint = GetBlueprintObj())
	{
		if (!Blueprint->IsUpToDate())
		{
			Compile();
		}
	}
}

void FPersona::Compile()
{
	// Note if we were debugging the preview
	UObject* CurrentDebugObject = GetBlueprintObj()->GetObjectBeingDebugged();
	UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
	const bool bIsDebuggingPreview = (PreviewMeshComponent != NULL) && PreviewMeshComponent->IsAnimBlueprintInstanced() && (PreviewMeshComponent->GetAnimInstance() == CurrentDebugObject);

	if (PreviewMeshComponent != NULL)
	{
		// Force close any asset editors that are using the AnimScriptInstance (such as the Property Matrix), the class will be garbage collected
		FAssetEditorManager::Get().CloseOtherEditors(PreviewMeshComponent->GetAnimInstance(), nullptr);
	}

	// Compile the blueprint
	FBlueprintEditor::Compile();

	if (PreviewMeshComponent != NULL)
	{
		if (PreviewMeshComponent->GetAnimInstance() == NULL)
		{
			// try reinitialize animation if it doesn't exist
			PreviewMeshComponent->InitAnim(true);
		}

		if (bIsDebuggingPreview)
		{
			GetBlueprintObj()->SetObjectBeingDebugged(PreviewMeshComponent->GetAnimInstance());
		}
	}

	OnPostCompile();
}

FName FPersona::GetToolkitContextFName() const
{
	return GetToolkitFName();
}

FName FPersona::GetToolkitFName() const
{
	return FName("Persona");
}

FText FPersona::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Persona");
}

FText FPersona::GetToolkitName() const
{
	FFormatNamedArguments Args;

	if (IsEditingSingleBlueprint())
	{
		const bool bDirtyState = GetBlueprintObj()->GetOutermost()->IsDirty();

		Args.Add( TEXT("BlueprintName"), FText::FromString( GetBlueprintObj()->GetName() ) );
		Args.Add( TEXT("DirtyState"), bDirtyState ? FText::FromString( TEXT( "*" ) ) : FText::GetEmpty() );
		return FText::Format( LOCTEXT("KismetToolkitName_SingleBlueprint", "{BlueprintName}{DirtyState}"), Args );
	}
	else
	{
		USkeleton* Skeleton = GetPersonaToolkit()->GetSkeleton();
		const bool bDirtyState = Skeleton->GetOutermost()->IsDirty();

		Args.Add( TEXT("SkeletonName"), FText::FromString(Skeleton->GetName() ) );
		Args.Add( TEXT("DirtyState"), bDirtyState ? FText::FromString( TEXT( "*" ) ) : FText::GetEmpty() );
		return FText::Format( LOCTEXT("KismetToolkitName_SingleSkeleton", "{SkeletonName}{DirtyState}"), Args );
	}
}

FText FPersona::GetToolkitToolTipText() const
{
	if (IsEditingSingleBlueprint())
	{
		return FAssetEditorToolkit::GetToolTipTextForObject(GetBlueprintObj());
	}
	else
	{
		USkeleton* Skeleton = GetPersonaToolkit()->GetSkeleton();
		return FAssetEditorToolkit::GetToolTipTextForObject(Skeleton);
	}
}

FString FPersona::GetWorldCentricTabPrefix() const
{
	return NSLOCTEXT("Persona", "WorldCentricTabPrefix", "Persona ").ToString();
}


FLinearColor FPersona::GetWorldCentricTabColorScale() const
{
	return FLinearColor( 0.5f, 0.25f, 0.35f, 0.5f );
}

void FPersona::SaveAnimationAssets_Execute()
{
	// only save animation assets related to skeletons
	TArray<UClass*> SaveClasses;
	SaveClasses.Add(UAnimationAsset::StaticClass());
	SaveClasses.Add(UAnimBlueprint::StaticClass());
	SaveClasses.Add(USkeletalMesh::StaticClass());
	SaveClasses.Add(USkeleton::StaticClass());

	const bool bPromptUserToSave = true;
	const bool bFastSave = false;
	FEditorFileUtils::SaveDirtyContentPackages(SaveClasses, bPromptUserToSave, bFastSave);
}

bool FPersona::CanSaveAnimationAssets() const
{
	return true;
}

void FPersona::OnActiveTabChanged( TSharedPtr<SDockTab> PreviouslyActive, TSharedPtr<SDockTab> NewlyActivated )
{
	if (!NewlyActivated.IsValid())
	{
		TArray<UObject*> ObjArray;
		Inspector->ShowDetailsForObjects(ObjArray);
	}
	else if (SharedAnimDocumentTab.IsValid() && NewlyActivated.Get() == SharedAnimDocumentTab.Pin().Get())
	{
		if (GetPersonaToolkit()->GetAnimationAsset())
		{
			GetPersonaToolkit()->GetPreviewScene()->SetPreviewAnimationAsset(GetPersonaToolkit()->GetAnimationAsset());
		}
	}
	else if (SharedAnimDocumentTab.IsValid() && PreviouslyActive.Get() == SharedAnimDocumentTab.Pin().Get())
	{
		//@TODO: The flash focus makes it impossible to do this and still edit montages
		//SetDetailObject(NULL);
	}
	else 
	{
		FBlueprintEditor::OnActiveTabChanged(PreviouslyActive, NewlyActivated);
	}
}

void FPersona::SetPreviewMesh(USkeletalMesh* NewPreviewMesh)
{
	GetSkeletonTree()->SetSkeletalMesh(NewPreviewMesh);
}

void FPersona::OnPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent)
{
	// Re-initialize the preview when a skeletal control is being edited
	//@TODO: Should we still do this?
}

void FPersona::RefreshPreviewInstanceTrackCurves()
{
	// need to refresh the preview mesh
	UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
	if(PreviewMeshComponent->PreviewInstance)
	{
		PreviewMeshComponent->PreviewInstance->RefreshCurveBoneControllers();
	}
}

void FPersona::PostUndo(bool bSuccess)
{
	DocumentManager->CleanInvalidTabs();
	DocumentManager->RefreshAllTabs();

	FBlueprintEditor::PostUndo(bSuccess);

	// If we undid a node creation that caused us to clean up a tab/graph we need to refresh the UI state
	RefreshEditors();

	// PostUndo broadcast
	OnPostUndo.Broadcast();	

	// Re-init preview instance
	UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
	if (PreviewMeshComponent && PreviewMeshComponent->AnimScriptInstance)
	{
		PreviewMeshComponent->AnimScriptInstance->InitializeAnimation();
	}

	RefreshPreviewInstanceTrackCurves();

	// clear up preview anim notify states
	// animnotify states are saved in AnimInstance
	// if those are undoed or redoed, they have to be 
	// cleared up, otherwise, they might have invalid data
	ClearupPreviewMeshAnimNotifyStates();

	OnPostCompile();
}

void FPersona::ClearupPreviewMeshAnimNotifyStates()
{
	UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
	if ( PreviewMeshComponent )
	{
		UAnimInstance* AnimInstantace = PreviewMeshComponent->GetAnimInstance();

		if (AnimInstantace)
		{
			// empty this because otherwise, it can have corrupted data
			// this will cause state to be interrupted, but that is better
			// than crashing
			AnimInstantace->ActiveAnimNotifyState.Empty();
		}
	}
}

bool FPersona::IsInAScriptingMode() const
{
	return IsModeCurrent(FPersonaModes::AnimBlueprintEditMode);
}

void FPersona::GetCustomDebugObjects(TArray<FCustomDebugObject>& DebugList) const
{
	UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
	if (PreviewMeshComponent->IsAnimBlueprintInstanced())
	{
		new (DebugList) FCustomDebugObject(PreviewMeshComponent->GetAnimInstance(), LOCTEXT("PreviewObjectLabel", "Preview Instance").ToString());
	}
}

void FPersona::CreateDefaultTabContents(const TArray<UBlueprint*>& InBlueprints)
{
	FBlueprintEditor::CreateDefaultTabContents(InBlueprints);

	PreviewEditor = SNew(SPersonaPreviewPropertyEditor, GetPreviewScene());
}

FGraphAppearanceInfo FPersona::GetGraphAppearance(UEdGraph* InGraph) const
{
	FGraphAppearanceInfo AppearanceInfo = FBlueprintEditor::GetGraphAppearance(InGraph);

	if ( GetBlueprintObj()->IsA(UAnimBlueprint::StaticClass()) )
	{
		AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_Animation", "ANIMATION");
	}

	return AppearanceInfo;
}

void FPersona::FocusWindow(UObject* ObjectToFocusOn)
{
	FBlueprintEditor::FocusWindow(ObjectToFocusOn);
	if (ObjectToFocusOn != NULL)
	{
		// we should be already in our desired window, now just update mode
		if (ObjectToFocusOn->IsA(UAnimBlueprint::StaticClass()))
		{
			SetCurrentMode(FPersonaModes::AnimBlueprintEditMode);
		}
		else if (UAnimationAsset* AnimationAssetToFocus = Cast<UAnimationAsset>(ObjectToFocusOn))
		{
			SetCurrentMode(FPersonaModes::AnimationEditMode);
			OpenNewDocumentTab(AnimationAssetToFocus);
		}
		else if (USkeletalMesh* SkeletalMeshToFocus = Cast<USkeletalMesh>(ObjectToFocusOn))
		{
			SetCurrentMode(FPersonaModes::MeshEditMode);
			SetPreviewMesh(SkeletalMeshToFocus);
		}
		else if (ObjectToFocusOn->IsA(USkeleton::StaticClass()))
		{
			SetCurrentMode(FPersonaModes::SkeletonDisplayMode);
		}
	}
}

void FPersona::ClearSelectedWindActor()
{
	GetPreviewScene()->ClearSelectedActor();
}

void FPersona::ClearSelectedAnimGraphNode()
{
	SelectedAnimGraphNode.Reset();
}

void FPersona::DeselectAll()
{
	GetSkeletonTree()->DeselectAll();
	ClearSelectedWindActor();
	ClearSelectedAnimGraphNode();
}

FReply FPersona::OnClickEditMesh()
{
	USkeleton* Skeleton = PersonaToolkit->GetSkeleton();
	USkeletalMesh* PreviewMesh = Skeleton->GetPreviewMesh();
	if(PreviewMesh)
	{
		SetDetailObject(PreviewMesh);
	}
	return FReply::Handled();
}


void FPersona::PostRedo(bool bSuccess)
{
	DocumentManager->RefreshAllTabs();

	FBlueprintEditor::PostRedo(bSuccess);

	// PostUndo broadcast, OnPostRedo
	OnPostUndo.Broadcast();

	UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
	if (PreviewMeshComponent && PreviewMeshComponent->AnimScriptInstance)
	{
		PreviewMeshComponent->AnimScriptInstance->InitializeAnimation();
	}

	// clear up preview anim notify states
	// animnotify states are saved in AnimInstance
	// if those are undoed or redoed, they have to be 
	// cleared up, otherwise, they might have invalid data
	ClearupPreviewMeshAnimNotifyStates();
}

TArray<UObject*> FPersona::GetEditorObjectsForMode(FName Mode) const
{
	if( Mode == FPersonaModes::AnimationEditMode )
	{
		return GetEditorObjectsOfClass( GetEditingObjects(), UAnimationAsset::StaticClass() );
	}
	else if( Mode == FPersonaModes::AnimBlueprintEditMode )
	{
		return GetEditorObjectsOfClass( GetEditingObjects(), UAnimBlueprint::StaticClass() );
	}
	else if( Mode == FPersonaModes::MeshEditMode )
	{
		return GetEditorObjectsOfClass( GetEditingObjects(), USkeletalMesh::StaticClass() );
	}
	else if( Mode == FPersonaModes::PhysicsEditMode )
	{
		return GetEditorObjectsOfClass( GetEditingObjects(), UPhysicsAsset::StaticClass() );
	}
	else if( Mode == FPersonaModes::SkeletonDisplayMode )
	{
		return GetEditorObjectsOfClass( GetEditingObjects(), USkeleton::StaticClass() );
	}
	return TArray<UObject*>();
}

void FPersona::GetSaveableObjects(TArray<UObject*>& OutObjects) const
{
	OutObjects = GetEditorObjectsForMode(GetCurrentMode());
}

const FSlateBrush* FPersona::GetDirtyImageForMode(FName Mode) const
{
	TArray<UObject*> EditorObjects = GetEditorObjectsForMode(Mode);
	for(auto Iter = EditorObjects.CreateIterator(); Iter; ++Iter)
	{
		if(UPackage* Package = (*Iter)->GetOutermost())
		{
			if(Package->IsDirty())
			{
				return AssetDirtyBrush;
			}
		}
	}
	return NULL;
}

void FPersona::FindInContentBrowser_Execute()
{
	FName CurrentMode = GetCurrentMode();
	TArray<UObject*> ObjectsToSyncTo = GetEditorObjectsForMode(CurrentMode);
	if (ObjectsToSyncTo.Num() > 0)
	{
		GEditor->SyncBrowserToObjects( ObjectsToSyncTo );
	}
}

void FPersona::SaveAsset_Execute()
{
	//Clean the unused Material entry before saving
	if (USkeletalMesh* Mesh = PersonaToolkit->GetMesh())
	{
		for (int32 MaterialIndex = 0; MaterialIndex < Mesh->Materials.Num(); ++MaterialIndex)
		{
			bool MaterialIsUsed = false;
			FSkeletalMeshResource* ImportedResource = Mesh->GetImportedResource();
			check(ImportedResource);
			for (int32 LODIndex = 0; !MaterialIsUsed && LODIndex < ImportedResource->LODModels.Num(); ++LODIndex)
			{
				FSkeletalMeshLODInfo& Info = Mesh->LODInfo[LODIndex];
				if (LODIndex == 0 || Mesh->LODInfo[LODIndex].LODMaterialMap.Num() == 0)
				{
					for (int32 SectionIndex = 0; SectionIndex < ImportedResource->LODModels[LODIndex].Sections.Num(); ++SectionIndex)
					{
						if (ImportedResource->LODModels[LODIndex].Sections[SectionIndex].MaterialIndex == MaterialIndex)
						{
							MaterialIsUsed = true;
							break;
						}
					}
				}
				else
				{
					for (int32 SectionIndex = 0; SectionIndex < Mesh->LODInfo[LODIndex].LODMaterialMap.Num(); ++SectionIndex)
					{
						if (Info.LODMaterialMap[SectionIndex] == MaterialIndex)
						{
							MaterialIsUsed = true;
							break;
						}
					}
				}
			}
			if (!MaterialIsUsed)
			{
				Mesh->Materials[MaterialIndex].MaterialInterface = nullptr;
			}
		}
	}
	FAssetEditorToolkit::SaveAsset_Execute();
}

void FPersona::UndoAction()
{
	GEditor->UndoTransaction();
}

void FPersona::RedoAction()
{
	GEditor->RedoTransaction();
}

void FPersona::OnSetKeyCompleted()
{
	OnTrackCurvesChanged.Broadcast();
}

bool FPersona::CanSetKey() const
{
	UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
	return ( HasValidAnimationSequencePlaying() && PreviewMeshComponent->BonesOfInterest.Num() > 0);
}

void FPersona::OnSetKey()
{
	UAnimSequence* AnimSequence = Cast<UAnimSequence> (GetAnimationAssetBeingEdited());
	if (AnimSequence)
	{
		UDebugSkelMeshComponent* Component = PersonaToolkit->GetPreviewMeshComponent();
		Component->PreviewInstance->SetKey(FSimpleDelegate::CreateSP(this, &FPersona::OnSetKeyCompleted));
	}
}

bool FPersona::CanApplyRawAnimChanges() const
{
	UAnimSequence* AnimSequence = Cast<UAnimSequence> (GetAnimationAssetBeingEdited());
	// ideally would be great if we can only show if something changed
	return (AnimSequence && (AnimSequence->DoesNeedRebake() || AnimSequence->DoesNeedRecompress()));
}

void FPersona::OnApplyRawAnimChanges()
{
	UAnimSequence* AnimSequence = Cast<UAnimSequence>(GetAnimationAssetBeingEdited());
	if(AnimSequence)
	{
		UDebugSkelMeshComponent* Component = PersonaToolkit->GetPreviewMeshComponent();
		// now bake
		if (AnimSequence->DoesNeedRebake())
		{
			FScopedTransaction ScopedTransaction(LOCTEXT("BakeAnimation", "Bake Animation"));
			AnimSequence->Modify(true);
			AnimSequence->BakeTrackCurvesToRawAnimation();
		}

		if (AnimSequence->DoesNeedRecompress())
		{
			FScopedTransaction ScopedTransaction(LOCTEXT("BakeAnimation", "Bake Animation"));
			AnimSequence->Modify(true);
			AnimSequence->RequestSyncAnimRecompression(false);
		}
	}
}

void FPersona::OnApplyCompression()
{
	UAnimSequence* AnimSequence = Cast<UAnimSequence> (GetPreviewScene()->GetPreviewAnimationAsset());

	if (AnimSequence)
	{
		TArray<TWeakObjectPtr<UAnimSequence>> AnimSequences;
		AnimSequences.Add(AnimSequence);
		FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");
		PersonaModule.ApplyCompression(AnimSequences);
	}
}

void FPersona::OnExportToFBX()
{
	UAnimSequence* AnimSequence = Cast<UAnimSequence>(GetPreviewScene()->GetPreviewAnimationAsset());

	if(AnimSequence)
	{
		TArray<TWeakObjectPtr<UAnimSequence>> AnimSequences;
		AnimSequences.Add(AnimSequence);
		FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");
		PersonaModule.ExportToFBX(AnimSequences, GetPreviewScene()->GetPreviewMeshComponent()->SkeletalMesh);
	}
}

void FPersona::OnAddLoopingInterpolation()
{
	UAnimSequence* AnimSequence = Cast<UAnimSequence>(GetPreviewScene()->GetPreviewAnimationAsset());

	if(AnimSequence)
	{
		TArray<TWeakObjectPtr<UAnimSequence>> AnimSequences;
		AnimSequences.Add(AnimSequence);
		FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");
		PersonaModule.AddLoopingInterpolation(AnimSequences);
	}
}

bool FPersona::HasValidAnimationSequencePlaying() const
{
	UAnimSequence* AnimSequence = Cast<UAnimSequence> (GetAnimationAssetBeingEdited());
	
	return (AnimSequence != NULL);
}

bool FPersona::IsInPersonaMode(const FName InPersonaMode) const
{
	return (GetCurrentMode() == InPersonaMode);
}

void FPersona::ChangeSkeletonPreviewMesh()
{
	UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
	const USkeleton& Skeleton = GetSkeletonTree()->GetEditableSkeleton()->GetSkeleton();

	// Menu option cannot be called unless this is valid
	if(Skeleton.GetPreviewMesh() != PreviewMeshComponent->SkeletalMesh)
	{
		GetSkeletonTree()->GetEditableSkeleton()->SetPreviewMesh(PreviewMeshComponent->SkeletalMesh);

		FFormatNamedArguments Args;
		Args.Add( TEXT("PreviewMeshName"), FText::FromString( PreviewMeshComponent->SkeletalMesh->GetName() ) );
		Args.Add( TEXT("TargetSkeletonName"), FText::FromString( Skeleton.GetName() ) );
		FNotificationInfo Info( FText::Format( LOCTEXT("SaveSkeletonWarning", "Please save Skeleton {TargetSkeletonName} if you'd like to keep {PreviewMeshName} as default preview mesh" ), Args ) );
		Info.ExpireDuration = 5.0f;
		Info.bUseLargeFont = false;
		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if ( Notification.IsValid() )
		{
			Notification->SetCompletionState( SNotificationItem::CS_Fail );
		}
	}
}

void FPersona::UpdateSkeletonRefPose()
{
	GetSkeletonTree()->GetEditableSkeleton()->UpdateSkeletonReferencePose(PersonaToolkit->GetPreviewMeshComponent()->SkeletalMesh);
}

void FPersona::RemoveUnusedBones()
{
	GetSkeletonTree()->GetEditableSkeleton()->RemoveUnusedBones();
}

void FPersona::Tick(float DeltaTime)
{
	FBlueprintEditor::Tick(DeltaTime);

	if (Viewport.IsValid())
	{
		Viewport.Pin()->RefreshViewport();
	}
}

bool FPersona::CanChangeSkeletonPreviewMesh() const
{
	UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
	return PreviewMeshComponent && PreviewMeshComponent->SkeletalMesh;
}

bool FPersona::CanRemoveBones() const
{
	UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
	return PreviewMeshComponent && PreviewMeshComponent->SkeletalMesh;
}

bool FPersona::IsEditable(UEdGraph* InGraph) const
{
	bool bEditable = FBlueprintEditor::IsEditable(InGraph);
	bEditable &= IsGraphInCurrentBlueprint(InGraph);

	return bEditable;
}

FText FPersona::GetGraphDecorationString(UEdGraph* InGraph) const
{
	if (!IsGraphInCurrentBlueprint(InGraph))
	{
		return LOCTEXT("PersonaExternalGraphDecoration", " Parent Graph Preview");
	}
	return FText::GetEmpty();
}



void FPersona::OnAnimNotifyWindow()
{
	TabManager->InvokeTab(FPersonaTabs::SkeletonAnimNotifiesID);
}

void FPersona::OnRetargetManager()
{
	TabManager->InvokeTab(FPersonaTabs::RetargetManagerID);
}

void FPersona::OnImportAsset(enum EFBXImportType DefaultImportType)
{
	FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");
	PersonaModule.ImportNewAsset(PersonaToolkit->GetSkeleton(), DefaultImportType);
}

void FPersona::OnReimportMesh()
{
	// Reimport the asset
	UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
	if(PreviewMeshComponent && PreviewMeshComponent->SkeletalMesh)
	{
		FReimportManager::Instance()->Reimport(PreviewMeshComponent->SkeletalMesh, true);
	}
}

void FPersona::OnReimportAnimation()
{
	// Reimport the asset
	UAnimSequence* AnimSequence = Cast<UAnimSequence> (GetAnimationAssetBeingEdited());
	if (AnimSequence)
	{
		FReimportManager::Instance()->Reimport(AnimSequence, true);
	}
}

TStatId FPersona::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FPersona, STATGROUP_Tickables);
}

void FPersona::OnBlueprintPreCompile(UBlueprint* BlueprintToCompile)
{
	UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
	if(PreviewMeshComponent && PreviewMeshComponent->PreviewInstance)
	{
		// If we are compiling an anim notify state the class will soon be sanitized and 
		// if an anim instance is running a state when that happens it will likely
		// crash, so we end any states that are about to compile.
		UAnimPreviewInstance* Instance = PreviewMeshComponent->PreviewInstance;
		USkeletalMeshComponent* SkelMeshComp = Instance->GetSkelMeshComponent();

		for(int32 Idx = Instance->ActiveAnimNotifyState.Num() - 1 ; Idx >= 0 ; --Idx)
		{
			FAnimNotifyEvent& Event = Instance->ActiveAnimNotifyState[Idx];
			if(Event.NotifyStateClass->GetClass() == BlueprintToCompile->GeneratedClass)
			{
				Event.NotifyStateClass->NotifyEnd(SkelMeshComp, Cast<UAnimSequenceBase>(Event.NotifyStateClass->GetOuter()));
				Instance->ActiveAnimNotifyState.RemoveAt(Idx);
			}
		}
	}
}

void FPersona::OnBlueprintChangedImpl(UBlueprint* InBlueprint, bool bIsJustBeingCompiled /*= false*/)
{
	FBlueprintEditor::OnBlueprintChangedImpl(InBlueprint, bIsJustBeingCompiled);

	UObject* CurrentDebugObject = GetBlueprintObj()->GetObjectBeingDebugged();
	UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();

	if(PreviewMeshComponent != NULL)
	{
		// Reinitialize the animation, anything we reference could have changed triggering
		// the blueprint change
		PreviewMeshComponent->InitAnim(true);

		const bool bIsDebuggingPreview = PreviewMeshComponent->IsAnimBlueprintInstanced() && (PreviewMeshComponent->GetAnimInstance() == CurrentDebugObject);
		if(bIsDebuggingPreview)
		{
			GetBlueprintObj()->SetObjectBeingDebugged(PreviewMeshComponent->GetAnimInstance());
		}
	}

	// calls PostCompile to copy proper values between anim nodes
	OnPostCompile();
}

void FPersona::ShowReferencePose(bool bReferencePose)
{
	GetPreviewScene()->ShowReferencePose(bReferencePose);
}

void FPersona::TestSkeletonCurveNamesForUse() const
	{
	FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");
	PersonaModule.TestSkeletonCurveNamesForUse(GetSkeletonTree()->GetEditableSkeleton());
}

bool FPersona::IsShowReferencePoseEnabled() const
				{
	return GetPreviewScene()->IsShowReferencePoseEnabled() == true;
				}

bool FPersona::IsPreviewAssetEnabled() const
			{
	return GetPreviewScene()->IsShowReferencePoseEnabled() == false;
			}

FText FPersona::GetPreviewAssetTooltip() const
			{
	return GetPreviewScene()->GetPreviewAssetTooltip(IsInPersonaMode(FPersonaModes::AnimBlueprintEditMode));
			}
			
void FPersona::SetSelectedBlendProfile(UBlendProfile* InBlendProfile)
{
	OnBlendProfileSelected.Broadcast(InBlendProfile);
}

TSharedRef<FAnimationEditorPreviewScene> FPersona::GetPreviewScene() const
	{
	check(PersonaToolkit.IsValid()); 
	return StaticCastSharedRef<FAnimationEditorPreviewScene>(PersonaToolkit->GetPreviewScene()); 
	}

void FPersona::HandleCurvesChanged()
{
	OnCurvesChanged.Broadcast();
}

void FPersona::HandleSectionsChanged()
{
	OnSectionsChanged.Broadcast();
}

void FPersona::HandleObjectsSelected(const TArray<UObject*>& InObjects)
{
	SetDetailObjects(InObjects);
}
			
void FPersona::HandleObjectSelected(UObject* InObject)
			{
	SetDetailObject(InObject);
}

void FPersona::HandleAnimNotifiesChanged()
					{
	OnAnimNotifiesChanged.Broadcast();
				}

void FPersona::HandleViewportCreated(const TSharedRef<IPersonaViewport>& InViewport)
				{
	// mode switch data sharing: We save data from previous viewports and restore after they are switched
	bool bRestoreData = Viewport.IsValid();
	if (bRestoreData)
					{
		ViewportState = InViewport->SaveState();
	}

	SetViewport(StaticCastSharedRef<SAnimationEditorViewportTabBody>(InViewport));

	if (bRestoreData)
						{
		InViewport->RestoreState(ViewportState.ToSharedRef());
						}
							}

void FPersona::HandleOpenNewAsset(UObject* InNewAsset)
							{
	if (UAnimationAsset* NewAnimationAsset = Cast<UAnimationAsset>(InNewAsset))
								{
		OpenNewDocumentTab(NewAnimationAsset);
		SetPreviewAnimationAsset(NewAnimationAsset);
							}
						}

UObject* FPersona::HandleGetObject()
				{
	return GetEditingObject();
}

void FPersona::OnPostCompile()
					{
	// act as if we have re-selected, so internal pointers are updated
	OnSelectedNodesChangedImpl(GetSelectedNodes());

	// if the user manipulated Pin values directly from the node, then should copy updated values to the internal node to retain data consistency
	UEdGraph* FocusedGraph = GetFocusedGraph();
	if (FocusedGraph)
						{
		// find UAnimGraphNode_Base
		for (UEdGraphNode* Node : FocusedGraph->Nodes)
							{
			UAnimGraphNode_Base* AnimGraphNode = Cast<UAnimGraphNode_Base>(Node);
			if (AnimGraphNode)
								{
				FAnimNode_Base* AnimNode = FindAnimNode(AnimGraphNode);
				if (AnimNode)
				{
					AnimGraphNode->CopyNodeDataToPreviewNode(AnimNode);
				}
			}
		}
	}
}

FAnimNode_Base* FPersona::FindAnimNode(UAnimGraphNode_Base* AnimGraphNode) const
{
	FAnimNode_Base* AnimNode = nullptr;
	if (AnimGraphNode)
{
		UDebugSkelMeshComponent* PreviewMeshComponent = GetPreviewScene()->GetPreviewMeshComponent();
		if (PreviewMeshComponent != nullptr && PreviewMeshComponent->GetAnimInstance() != nullptr)
	{
			AnimNode = AnimGraphNode->FindDebugAnimNode(PreviewMeshComponent);
	}
}

	return AnimNode;
}

void FPersona::OnSelectedNodesChangedImpl(const TSet<class UObject*>& NewSelection)
{
	FBlueprintEditor::OnSelectedNodesChangedImpl(NewSelection);

	if (SelectedAnimGraphNode.IsValid())
{
		// copy values from the preview node to ensure data consistency
		FAnimNode_Base* PreviewNode = FindAnimNode(SelectedAnimGraphNode.Get());

		SelectedAnimGraphNode->OnNodeSelected(false, *GetAssetEditorModeManager(), PreviewNode);
		SelectedAnimGraphNode.Reset();
			}

	// if we only have one node selected, let it know
	UAnimGraphNode_Base* NewSelectedAnimGraphNode = nullptr;
	if (NewSelection.Num() == 1)
		{
		NewSelectedAnimGraphNode = Cast<UAnimGraphNode_SkeletalControlBase>(*NewSelection.CreateConstIterator());
		if (NewSelectedAnimGraphNode != nullptr)
			{
			SelectedAnimGraphNode = NewSelectedAnimGraphNode;

			// copy values to the preview node to ensure data consistency
			FAnimNode_Base* PreviewNode = FindAnimNode(SelectedAnimGraphNode.Get());

			SelectedAnimGraphNode->OnNodeSelected(true, *GetAssetEditorModeManager(), PreviewNode);
			}
		}
}

void FPersona::HandlePinDefaultValueChanged(UEdGraphPin* InPinThatChanged)
{
	UAnimGraphNode_Base* AnimGraphNode = Cast<UAnimGraphNode_Base>(InPinThatChanged->GetOwningNode());
	if (AnimGraphNode)
{
		FAnimNode_Base* AnimNode = FindAnimNode(AnimGraphNode);
		if (AnimNode)
{
			AnimGraphNode->CopyNodeDataToPreviewNode(AnimNode);
}
}
}

void FPersona::TogglePlayback()
{
	UDebugSkelMeshComponent* PreviewComponent = GetPreviewScene()->GetPreviewMeshComponent();
	if (PreviewComponent && PreviewComponent->PreviewInstance)
	{
		PreviewComponent->PreviewInstance->SetPlaying(!PreviewComponent->PreviewInstance->IsPlaying());
	}
}

void FPersona::HandleAnimationSequenceBrowserCreated(const TSharedRef<IAnimationSequenceBrowser>& InSequenceBrowser)
		{
	SequenceBrowser = InSequenceBrowser;
	}

#undef LOCTEXT_NAMESPACE

