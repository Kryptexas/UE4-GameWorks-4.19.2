// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"
#include "Persona.h"
#include "SPersonaToolbar.h"
#include "PersonaModule.h"
#include "AnimGraphDefinitions.h"

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
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Runtime/AssetRegistry/Public/AssetRegistryModule.h"

#include "Editor/Persona/Private/AnimationEditorViewportClient.h"

#include "ComponentAssetBroker.h"

#define LOCTEXT_NAMESPACE "FPersona"

/////////////////////////////////////////////////////
// FLocalCharEditorCallbacks

struct FLocalCharEditorCallbacks
{
	static FText GetObjectName(UObject* Object)
	{
		return FText::FromString( Object->GetName() );
	}
};

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// Create Animation dialog for recording animation
/////////////////////////////////////////////////////

class SCreateAnimationDlg : public SWindow
{
public:
	SLATE_BEGIN_ARGS(SCreateAnimationDlg)
		: _Title(LOCTEXT("Title", "Create New Animation Object"))
	{
	}
	SLATE_ARGUMENT(FText, Title)
		SLATE_ARGUMENT(FText, DefaultAssetPath)
		SLATE_END_ARGS()

		SCreateAnimationDlg()
		: UserResponse(EAppReturnType::Cancel)
	{
	}

	void Construct(const FArguments& InArgs);

public:
	/** Displays the dialog in a blocking fashion */
	EAppReturnType::Type ShowModal();

	/** Gets the resulting asset path */
	FString GetAssetPath();

	/** Gets the resulting asset name */
	FString GetAssetName();

	/** Gets the resulting full asset path (path+'/'+name) */
	FString GetFullAssetPath();

	/** Get Duration **/
	float GetDuration();

protected:
	void OnPathChange(const FString& NewPath);
	void OnNameChange(const FText& NewName, ETextCommit::Type CommitInfo);
	void OnDurationChange(const FText& NewName, ETextCommit::Type CommitInfo);
	FReply OnButtonClick(EAppReturnType::Type ButtonID);

	bool ValidatePackage();

	EAppReturnType::Type UserResponse;
	FText AssetPath;
	FText AssetName;
	FText Duration;
};

void SCreateAnimationDlg::Construct(const FArguments& InArgs)
{
	AssetPath = FText::FromString(FPackageName::GetLongPackagePath(InArgs._DefaultAssetPath.ToString()));
	AssetName = FText::FromString(FPackageName::GetLongPackageAssetName(InArgs._DefaultAssetPath.ToString()));

	FPathPickerConfig PathPickerConfig;
	PathPickerConfig.DefaultPath = AssetPath.ToString();
	PathPickerConfig.OnPathSelected = FOnPathSelected::CreateSP(this, &SCreateAnimationDlg::OnPathChange);
	PathPickerConfig.bAddDefaultPath = true;

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	SWindow::Construct(SWindow::FArguments()
		.Title(InArgs._Title)
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		//.SizingRule( ESizingRule::Autosized )
		.ClientSize(FVector2D(450,450))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot() // Add user input block
			.Padding(2)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SVerticalBox)

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SelectPath", "Select Path to create animation"))
						.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 14 ) )
					]

					+SVerticalBox::Slot()
					.FillHeight(1)
					.Padding(3)
					[
						ContentBrowserModule.Get().CreatePathPicker(PathPickerConfig)
					]

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SSeparator)
					]

					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(3)
					[
						SNew(SHorizontalBox)

						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0, 0, 10, 0)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("AnimationName", "Animation Name"))
						]

						+SHorizontalBox::Slot()
						[
							SNew(SEditableTextBox)
							.Text(AssetName)
							.OnTextCommitted(this, &SCreateAnimationDlg::OnNameChange)
							.MinDesiredWidth(250)
						]
					]

					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(3)
					[
						SNew(SHorizontalBox)

						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0, 0, 10, 0)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("DurationInSec", "Duration (in sec) "))
						]

						+SHorizontalBox::Slot()
						[
							SNew(SEditableTextBox)
							.Text(Duration)
							.OnTextCommitted(this, &SCreateAnimationDlg::OnDurationChange)
							.MinDesiredWidth(100)
						]
					]
				]
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(5)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FEditorStyle::GetMargin("StandardDialog.SlotPadding"))
				.MinDesiredSlotWidth(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
				.MinDesiredSlotHeight(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
				+SUniformGridPanel::Slot(0,0)
				[
					SNew(SButton) 
					.HAlign(HAlign_Center)
					.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
					.Text(LOCTEXT("OK", "OK"))
					.OnClicked(this, &SCreateAnimationDlg::OnButtonClick, EAppReturnType::Ok)
				]
				+SUniformGridPanel::Slot(1,0)
					[
						SNew(SButton) 
						.HAlign(HAlign_Center)
						.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
						.Text(LOCTEXT("Cancel", "Cancel"))
						.OnClicked(this, &SCreateAnimationDlg::OnButtonClick, EAppReturnType::Cancel)
					]
			]
		]);
}

void SCreateAnimationDlg::OnDurationChange(const FText& InDuration, ETextCommit::Type CommitInfo)
{
	Duration = InDuration;
}

void SCreateAnimationDlg::OnNameChange(const FText& NewName, ETextCommit::Type CommitInfo)
{
	AssetName = NewName;
}

void SCreateAnimationDlg::OnPathChange(const FString& NewPath)
{
	AssetPath = FText::FromString(NewPath);
}

FReply SCreateAnimationDlg::OnButtonClick(EAppReturnType::Type ButtonID)
{
	UserResponse = ButtonID;

	if (ButtonID != EAppReturnType::Cancel)
	{
		if (!ValidatePackage())
		{
			// reject the request
			return FReply::Handled();
		}
	}

	RequestDestroyWindow();

	return FReply::Handled();
}

/** Ensures supplied package name information is valid */
bool SCreateAnimationDlg::ValidatePackage()
{
	FText Reason;
	FString FullPath = GetFullAssetPath();

	if(!FPackageName::IsValidLongPackageName(FullPath, false, &Reason)
		|| !FName(*AssetName.ToString()).IsValidObjectName(Reason))
	{
		FMessageDialog::Open(EAppMsgType::Ok, Reason);
		return false;
	}

	if (GetDuration() <= 0.0f)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("InvalidDuration", "Invalid duration"));
		return false;
	}

	return true;
}

EAppReturnType::Type SCreateAnimationDlg::ShowModal()
{
	GEditor->EditorAddModalWindow(SharedThis(this));
	return UserResponse;
}

FString SCreateAnimationDlg::GetAssetPath()
{
	return AssetPath.ToString();
}

FString SCreateAnimationDlg::GetAssetName()
{
	return AssetName.ToString();
}

FString SCreateAnimationDlg::GetFullAssetPath()
{
	return AssetPath.ToString() + "/" + AssetName.ToString();
}

float SCreateAnimationDlg::GetDuration()
{
	return FCString::Atof(*Duration.ToString());
}

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

		TabLayout = FTabManager::NewLayout( "Persona_SkeletonEditMode_Layout_v2" )
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
						->SetSizeCoefficient(0.75f)
						->Split
						(
							FTabManager::NewStack()
							->SetHideTabWell(true)
							->AddTab( FPersonaTabs::PreviewViewportID, ETabState::OpenedTab )
						)
					)
					->Split
					(
						// Right 1/3rd - Details panel and quick browser
						FTabManager::NewSplitter()
						->SetOrientation(Orient_Vertical)
						->Split
						(
							FTabManager::NewStack()
							->AddTab( FBlueprintEditorTabs::DetailsID, ETabState::OpenedTab )	//@TODO: FPersonaTabs::AnimPropertiesID
						)
						->Split
						(
							FTabManager::NewStack()
							->AddTab( FPersonaTabs::AssetBrowserID, ETabState::OpenedTab )
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
	: TargetSkeleton(NULL)
	, PreviewComponent(NULL)
	, PreviewScene(FPreviewScene::ConstructionValues().AllowAudioPlayback(true).ShouldSimulatePhysics(true))
{
	// Register to be notified when properties are edited
	OnPropertyChangedHandle = FCoreDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(this, &FPersona::OnPropertyChanged);
	FCoreDelegates::OnObjectPropertyChanged.Add(OnPropertyChangedHandle);
}

FPersona::~FPersona()
{
	FAssetEditorToolkit::OnPostReimport().RemoveAll(this);

	FPersonaModule* PersonaModule = &FModuleManager::LoadModuleChecked<FPersonaModule>( "Persona" );
	PersonaModule->GetMenuExtensibilityManager()->RemoveExtender(PersonaMenuExtender);

	if(Viewport.IsValid())
	{
		Viewport.Pin()->CleanupPersonaReferences();
	}

	FCoreDelegates::OnObjectPropertyChanged.Remove(OnPropertyChangedHandle);

	if(PreviewComponent)
	{
		PreviewComponent->RemoveFromRoot();
	}
	
	// NOTE: Any tabs that we still have hanging out when destroyed will be cleaned up by FBaseToolkit's destructor
}

TSharedPtr<SWidget> FPersona::CreateEditorWidgetForAnimDocument(UObject* InAnimAsset)
{
	TSharedPtr<SWidget> Result;
	if (InAnimAsset)
	{
		if (UAnimSequence* Sequence = Cast<UAnimSequence>(InAnimAsset))
		{
			Result = SNew(SSequenceEditor)
				.Persona(SharedThis(this))
				.Sequence(Sequence);
		}
		else if (UAnimComposite* Composite = Cast<UAnimComposite>(InAnimAsset))
		{
			Result = SNew(SAnimCompositeEditor)
				.Persona(SharedThis(this))
				.Composite(Composite);
		}
		else if (UAnimMontage* Montage = Cast<UAnimMontage>(InAnimAsset))
		{
			Result = SNew(SMontageEditor)
				.Persona(SharedThis(this))
				.Montage(Montage);
		}
		else if (UBlendSpace* BlendSpace = Cast<UBlendSpace>(InAnimAsset))
		{
			Result = SNew(SBlendSpaceEditor)
				.Persona(SharedThis(this))
				.BlendSpace(BlendSpace);
		}
		else if (UBlendSpace1D* BlendSpace1D = Cast<UBlendSpace1D>(InAnimAsset))
		{
			Result = SNew(SBlendSpaceEditor1D)
				.Persona(SharedThis(this))
				.BlendSpace1D(BlendSpace1D);
		}
	}

	if (Result.IsValid())
	{
		// Wrap the document editor with a potential preview mismatch warning
		Result = SNew(SOverlay)
			+SOverlay::Slot()
			[
				Result.ToSharedRef()
			]
			+SOverlay::Slot()
			[
				SNew(SAnimDifferentAssetBeingPreviewedWarning, SharedThis(this))
			];

		InAnimAsset->SetFlags(RF_Transactional);
	}

	return Result;
}

TSharedPtr<SDockTab> FPersona::OpenNewAnimationDocumentTab(UObject* InAnimAsset)
{
	TSharedPtr<SDockTab> OpenedTab;

	if (InAnimAsset != NULL)
	{
		TSharedPtr<SWidget> TabContents = CreateEditorWidgetForAnimDocument(InAnimAsset);
		if (TabContents.IsValid())
		{
			if ( SharedAnimAssetBeingEdited.IsValid() )
			{
				RemoveEditingObject(SharedAnimAssetBeingEdited.Get());
			}

			if ( InAnimAsset != NULL )
			{
				AddEditingObject(InAnimAsset);
			}

			SharedAnimAssetBeingEdited = InAnimAsset;
			TAttribute<FText> NameAttribute = TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateStatic(&FLocalCharEditorCallbacks::GetObjectName, (UObject*)InAnimAsset));

			if (SharedAnimDocumentTab.IsValid())
			{
				OpenedTab = SharedAnimDocumentTab.Pin();
				OpenedTab->SetContent(TabContents.ToSharedRef());
				OpenedTab->ActivateInParent(ETabActivationCause::SetDirectly);
				OpenedTab->SetLabel(NameAttribute);
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
			
				TabManager->InsertNewDocumentTab("Document", FTabManager::ESearchPreference::RequireClosedTab, OpenedTab.ToSharedRef());

				SharedAnimDocumentTab = OpenedTab;
			}
		}
	}

	return OpenedTab;
}

TSharedPtr<SDockTab> FPersona::OpenNewDocumentTab(class UAnimationAsset* InAnimAsset)
{
	TSharedPtr<SDockTab> NewTab;
	if (InAnimAsset)
	{
		// Are we allowed to open animation documents right now?
		//@TODO: Super-hacky check
		FName CurrentMode = GetCurrentMode();
		if (CurrentMode == FPersonaModes::AnimationEditMode)
		{
			NewTab = OpenNewAnimationDocumentTab(InAnimAsset);
		}

		SetPreviewAnimationAsset(InAnimAsset);
	}

	return NewTab;
}

void FPersona::SetViewport(TWeakPtr<class SAnimationEditorViewportTabBody> NewViewport)
{
	Viewport = NewViewport;
	if(Viewport.IsValid())
	{
		Viewport.Pin()->SetPreviewComponent(PreviewComponent);
	}
	OnViewportCreated.Broadcast(Viewport);
}

void FPersona::RefreshViewport()
{
	if (Viewport.IsValid())
	{
		Viewport.Pin()->RefreshViewport();
	}
}

USkeleton* FPersona::GetSkeleton() const
{
	return TargetSkeleton;
}

USkeletalMesh* FPersona::GetMesh() const
{
	return PreviewComponent->SkeletalMesh;
}

UPhysicsAsset* FPersona::GetPhysicsAsset() const
{
	return NULL;
}

UAnimBlueprint* FPersona::GetAnimBlueprint() const
{
	return Cast<UAnimBlueprint>(GetBlueprintObj());
}

UObject* FPersona::GetMeshAsObject() const
{
	return GetMesh();
}

UObject* FPersona::GetSkeletonAsObject() const
{
	return GetSkeleton();
}

UObject* FPersona::GetPhysicsAssetAsObject() const
{
	return GetPhysicsAsset();
}

UObject* FPersona::GetAnimBlueprintAsObject() const
{
	return GetAnimBlueprint();
}

void FPersona::InitPersona(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, USkeleton* InitSkeleton, UAnimBlueprint* InitAnimBlueprint, UAnimationAsset* InitAnimationAsset, class USkeletalMesh * InitMesh)
{
	FAssetEditorToolkit::OnPostReimport().AddRaw(this, &FPersona::OnPostReimport);

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

	// Determine the initial mode the app will open up in
	FName InitialMode = NAME_None;
	if (InitAnimBlueprint != NULL)
	{
		InitialMode = FPersonaModes::AnimBlueprintEditMode;
		TargetSkeleton = InitAnimBlueprint->TargetSkeleton;
	}
	else if (InitAnimationAsset != NULL)
	{
		InitialMode = FPersonaModes::AnimationEditMode;
		TargetSkeleton = InitSkeleton;
	}
	else if (InitMesh != NULL)
	{
		InitialMode = FPersonaModes::MeshEditMode;
		TargetSkeleton = InitSkeleton;
	}
	else
	{
		InitialMode = FPersonaModes::SkeletonDisplayMode;
		TargetSkeleton = InitSkeleton;
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

	ObjectsBeingEdited.Add(TargetSkeleton);
	check(TargetSkeleton);

	// We could modify the skeleton within Persona (add/remove sockets), so we need to enable undo/redo on it
	TargetSkeleton->SetFlags( RF_Transactional );

	// Validate the skeletons attached objects and display a notification to the user if any were broken
	int32 NumBrokenAssets = TargetSkeleton->ValidatePreviewAttachedObjects();
	if(NumBrokenAssets > 0)
	{
		// Tell the user that there were assets that could not be loaded
		FFormatNamedArguments Args;
		Args.Add( TEXT("NumBrokenAssets"), NumBrokenAssets );
		FNotificationInfo Info( FText::Format( LOCTEXT( "MissingPreviewAttachedAssets", "{NumBrokenAssets} attached assets could not be found on loading and were removed" ), Args ) );

		Info.bUseLargeFont = false;
		Info.ExpireDuration = 5.0f;

		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification( Info );
		if ( Notification.IsValid() )
		{
			Notification->SetCompletionState( SNotificationItem::CS_Fail );
		}
	}
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

	FPersonaModule* PersonaModule = &FModuleManager::LoadModuleChecked<FPersonaModule>( "Persona" );

	// Add additional persona editor menus
	struct Local
	{
		static void AddPersonaFileMenu( FMenuBuilder& MenuBuilder )
		{
			// View
			MenuBuilder.BeginSection("Persona", LOCTEXT("PersonaEditorMenu", "Animation" ) );
			{
				MenuBuilder.AddMenuEntry( FPersonaCommands::Get().RecordAnimation );
			}
			MenuBuilder.EndSection();
		}

		static void AddPersonaAssetMenu( FMenuBuilder& MenuBuilder )
		{
			// View
			MenuBuilder.BeginSection("Persona", LOCTEXT("PersonaAssetMenuMenu", "Skeleton" ) );
			{
				MenuBuilder.AddMenuEntry( FPersonaCommands::Get().ChangeSkeletonPreviewMesh );
			}
			MenuBuilder.EndSection();
		}
	};

	PersonaMenuExtender = MakeShareable(new FExtender);

	UAnimBlueprint* AnimBlueprint = GetAnimBlueprint();
	if (AnimBlueprint)
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender);
		FKismet2Menu::SetupBlueprintEditorMenu( MenuExtender, *this );
		AddMenuExtender(MenuExtender);

		PersonaMenuExtender->AddMenuExtension(
			"FileLoadAndSave", 
			EExtensionHook::After, 
			GetToolkitCommands(), 
			FMenuExtensionDelegate::CreateStatic(&Local::AddPersonaFileMenu));
	}

	PersonaMenuExtender->AddMenuExtension(
			"AssetEditorActions",
			EExtensionHook::After,
			GetToolkitCommands(),
			FMenuExtensionDelegate::CreateStatic( &Local::AddPersonaAssetMenu )
			);
					
	PersonaModule->GetMenuExtensibilityManager()->AddExtender(PersonaMenuExtender);					

	AddMenuExtender(PersonaModule->GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

	check(PreviewComponent == NULL);

	// Create the preview component
	PreviewComponent = ConstructObject<UDebugSkelMeshComponent>(UDebugSkelMeshComponent::StaticClass(), GetTransientPackage());

	// note: we add to root here (rather than using RF_Standalone) as all standalone objects will be cleaned up when we switch
	// preview worlds (in UWorld::CleanupWorld), but we want this to stick around while Persona exists
	PreviewComponent->AddToRoot();

	// Set the mesh
	if (InitMesh != NULL)
	{
		SetPreviewMesh(InitMesh);
	}
	else if (TargetSkeleton)
	{
		//If no preview mesh set, just find the first mesh that uses this skeleton
		USkeletalMesh * PreviewMesh = TargetSkeleton->GetPreviewMesh();
		if(!PreviewMesh)
		{
			FARFilter Filter;
			Filter.ClassNames.Add(USkeletalMesh::StaticClass()->GetFName());

			FString SkeletonString = FAssetData(TargetSkeleton).GetExportTextName();
			Filter.TagsAndValues.Add(GET_MEMBER_NAME_CHECKED(USkeletalMesh, Skeleton), SkeletonString);

			TArray<FAssetData> AssetList;
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			AssetRegistryModule.Get().GetAssets(Filter, AssetList);

			if(AssetList.Num() > 0)
			{
				TargetSkeleton->SetPreviewMesh( Cast<USkeletalMesh>(AssetList[0].GetAsset()), false );
			}
		}

		if (TargetSkeleton->GetPreviewMesh() != NULL)
		{
			SetPreviewMesh(TargetSkeleton->GetPreviewMesh());
		}
	}

	PreviewComponent->SetAnimClass(AnimBlueprint ? AnimBlueprint->GeneratedClass : NULL);

	// We always want a preview instance unless we are using blueprints so that bone manipulation works
	if (AnimBlueprint == NULL)
	{
		PreviewComponent->EnablePreview(true, NULL, NULL);
	}
	else
	{
		// Make sure the object being debugged is the preview instance
		AnimBlueprint->SetObjectBeingDebugged(PreviewComponent->AnimScriptInstance);
	}

	ExtendDefaultPersonaToolbar();

	RegenerateMenusAndToolbars();

	// Activate the initial mode (which will populate with a real layout)
	SetCurrentMode(InitialMode);
	check(CurrentAppModePtr.IsValid());

	// Post-layout initialization
	PostLayoutBlueprintEditorInitialization();

	//@TODO: Push somewhere else?
	if (InitAnimationAsset != NULL)
	{
		OpenNewDocumentTab( InitAnimationAsset );
	}
}


void FPersona::ExtendDefaultPersonaToolbar()
{
	TSharedPtr<FExtender> Extender = MakeShareable(new FExtender);

	PersonaToolbar->SetupPersonaToolbar(Extender, SharedThis(this));

	AddToolbarExtender(Extender);
}

UBlueprint* FPersona::GetBlueprintObj() const
{
	auto EditingObjects = GetEditingObjects();
	for (int32 i = 0; i < EditingObjects.Num(); ++i)
	{
		if (EditingObjects[i]->IsA<UAnimBlueprint>()) {return (UBlueprint*)EditingObjects[i];}
	}
	return NULL;
}

UObject* FPersona::GetPreviewAnimationAsset() const
{
	if (Viewport.IsValid())
	{
		if (PreviewComponent)
		{
			// if same, do not overwrite. It will reset time and everything
			if (PreviewComponent->PreviewInstance != NULL)
			{
				return PreviewComponent->PreviewInstance->CurrentAsset;
			}
		}
	}

	return NULL;
}

UObject* FPersona::GetAnimationAssetBeingEdited() const
{
	if (!SharedAnimDocumentTab.IsValid())
	{
		SharedAnimAssetBeingEdited = NULL;
	}

	return SharedAnimAssetBeingEdited.Get();
}

void FPersona::SetPreviewAnimationAsset(UAnimationAsset* AnimAsset, bool bEnablePreview)
{
	if (Viewport.IsValid() && !Viewport.Pin()->bPreviewLockModeOn)
	{
		if (PreviewComponent)
		{
			RemoveAttachedComponent(false);

			if (AnimAsset != NULL)
			{
				// Early out if the new preview asset is the same as the current one, to avoid replaying from the beginning, etc...
				if (AnimAsset == GetPreviewAnimationAsset() && PreviewComponent->IsPreviewOn())
				{
					return;
				}

				// Treat it as invalid if it's got a bogus skeleton pointer
				if (AnimAsset->GetSkeleton() != TargetSkeleton)
				{
					return;
				}
			}

			PreviewComponent->EnablePreview(bEnablePreview, AnimAsset, NULL);
		}

		OnAnimChanged.Broadcast(AnimAsset);
	}
}

void FPersona::SetPreviewVertexAnim(UVertexAnimation* VertexAnim)
{
	if (Viewport.IsValid() && !Viewport.Pin()->bPreviewLockModeOn)
	{
		if (PreviewComponent)
		{
			// if same, do not overwrite. It will reset time and everything
			if( VertexAnim && 
				PreviewComponent->PreviewInstance && 
				VertexAnim == PreviewComponent->PreviewInstance->CurrentVertexAnim )
			{
				return;
			}

			PreviewComponent->EnablePreview(true, NULL, VertexAnim);
		}
	}
}

void FPersona::UpdateSelectionDetails(UObject* Object, const FString& ForcedTitle)
{
	Inspector->ShowDetailsForSingleObject(Object, SKismetInspector::FShowDetailsOptions(ForcedTitle));
}

void FPersona::SetDetailObject(UObject* Obj)
{
	FString ForcedTitle = (Obj != NULL) ? *Obj->GetName() : FString();
	UpdateSelectionDetails(Obj, ForcedTitle);
}

UDebugSkelMeshComponent* FPersona::GetPreviewMeshComponent()
{
	return PreviewComponent;
}

void FPersona::OnPostReimport(UObject* InObject, bool bSuccess)
{
	// Ignore if this is regarding a different object
	if ( InObject != TargetSkeleton && InObject != TargetSkeleton->GetPreviewMesh() && InObject != GetAnimationAssetBeingEdited() )
	{
		return;
	}

	RefreshViewport();
}

/** Called when graph editor focus is changed */
void FPersona::OnGraphEditorFocused(const TSharedRef<class SGraphEditor>& InGraphEditor)
{
	// in the future, depending on which graph editor is this will act different
	FBlueprintEditor::OnGraphEditorFocused(InGraphEditor);
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

	// record animation
	ToolkitCommands->MapAction( FPersonaCommands::Get().RecordAnimation,
		FExecuteAction::CreateSP( this, &FPersona::RecordAnimation ),
		FCanExecuteAction::CreateSP( this, &FPersona::CanRecordAnimation ),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP( this, &FPersona::IsRecordAvailable )
		);

	ToolkitCommands->MapAction( FPersonaCommands::Get().ChangeSkeletonPreviewMesh,
		FExecuteAction::CreateSP( this, &FPersona::ChangeSkeletonPreviewMesh ),
		FCanExecuteAction::CreateSP( this, &FPersona::CanChangeSkeletonPreviewMesh )
		);

	// Generic deletion
	ToolkitCommands->MapAction(FGenericCommands::Get().Delete, FExecuteAction::CreateSP(this, &FPersona::OnCommandGenericDelete));

}

void FPersona::StartEditingDefaults(bool bAutoFocus, bool bForceRefresh)
{
	FBlueprintEditor::StartEditingDefaults(bAutoFocus, bForceRefresh);
	if (PreviewComponent != NULL)
	{
		if (PreviewComponent->AnimScriptInstance != NULL)
		{
			PreviewEditor->ShowDetailsForSingleObject(PreviewComponent->AnimScriptInstance, SKismetInspector::FShowDetailsOptions(TEXT("Preview settings")));
		}
	}
	if (bAutoFocus)
	{
		TabManager->InvokeTab(FPersonaTabs::AnimBlueprintDefaultsEditorID);
	}
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
			UAnimGraphNode_SequencePlayer * OldNode = Cast<UAnimGraphNode_SequencePlayer>(*NodeIter);

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
			UAnimGraphNode_SequenceEvaluator * OldNode = Cast<UAnimGraphNode_SequenceEvaluator>(*NodeIter);

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
			UAnimGraphNode_BlendSpacePlayer * OldNode = Cast<UAnimGraphNode_BlendSpacePlayer>(*NodeIter);

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
			UAnimGraphNode_BlendSpaceEvaluator * OldNode = Cast<UAnimGraphNode_BlendSpaceEvaluator>(*NodeIter);

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
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetAnimBlueprint());
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

void FPersona::Compile()
{
	// Note if we were debugging the preview
	UObject* CurrentDebugObject = GetBlueprintObj()->GetObjectBeingDebugged();
	const bool bIsDebuggingPreview = (PreviewComponent != NULL) && PreviewComponent->IsAnimBlueprintInstanced() && (PreviewComponent->AnimScriptInstance == CurrentDebugObject);

	// Compile the blueprint
	FBlueprintEditor::Compile();

	if (PreviewComponent != NULL)
	{
		if (PreviewComponent->AnimScriptInstance == NULL)
		{
			// try reinitialize animation if it doesn't exist
			PreviewComponent->InitAnim(true);
		}

		if (bIsDebuggingPreview)
		{
			GetBlueprintObj()->SetObjectBeingDebugged(PreviewComponent->AnimScriptInstance);
		}
	}
}

FString FPersona::GetDefaultEditorTitle()
{
	return NSLOCTEXT("Kismet", "PreviewParamatersTabTitle", "Preview Parameters").ToString();
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
		check(TargetSkeleton != NULL);

		const bool bDirtyState = TargetSkeleton->GetOutermost()->IsDirty();

		Args.Add( TEXT("SkeletonName"), FText::FromString( TargetSkeleton->GetName() ) );
		Args.Add( TEXT("DirtyState"), bDirtyState ? FText::FromString( TEXT( "*" ) ) : FText::GetEmpty() );
		return FText::Format( LOCTEXT("KismetToolkitName_SingleSkeleton", "{SkeletonName}{DirtyState}"), Args );
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

void FPersona::SaveAsset_Execute()
{
	if( ensure( GetEditingObjects().Num() > 0 ) )
	{
		const FName CurrentMode = GetCurrentMode();

		TArray<UObject*> EditorObjects = GetEditorObjectsForMode(CurrentMode);

		TArray< UPackage* > PackagesToSave;
		for(auto Iter = EditorObjects.CreateIterator(); Iter; ++Iter)
		{
			PackagesToSave.Add((*Iter)->GetOutermost());
		}

		FEditorFileUtils::PromptForCheckoutAndSave( PackagesToSave, /*bCheckDirty=*/ false, /*bPromptToSave=*/ false );
	}
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
		if (UAnimationAsset* SharedAsset = Cast<UAnimationAsset>(SharedAnimAssetBeingEdited.Get()))
		{
			SetPreviewAnimationAsset(SharedAsset);
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

// Sets the current preview mesh
void FPersona::SetPreviewMesh(USkeletalMesh* NewPreviewMesh)
{
	if(!TargetSkeleton->IsCompatibleMesh(NewPreviewMesh))
	{
		// Send a notification that the skeletal mesh cannot work with the skeleton
		FFormatNamedArguments Args;
		Args.Add( TEXT("PreviewMeshName"), FText::FromString( NewPreviewMesh->GetName() ) );
		Args.Add( TEXT("TargetSkeletonName"), FText::FromString( TargetSkeleton->GetName() ) );
		FNotificationInfo Info( FText::Format( LOCTEXT("SkeletalMeshIncompatible", "Skeletal Mesh \"{PreviewMeshName}\" incompatible with Skeleton \"{TargetSkeletonName}\"" ), Args ) );
		Info.ExpireDuration = 3.0f;
		Info.bUseLargeFont = false;
		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if ( Notification.IsValid() )
		{
			Notification->SetCompletionState( SNotificationItem::CS_Fail );
		}
	}
	else
	{
		if (NewPreviewMesh != PreviewComponent->SkeletalMesh)
		{
			if ( PreviewComponent->SkeletalMesh != NULL )
			{
				RemoveEditingObject(PreviewComponent->SkeletalMesh);
			}

			if ( NewPreviewMesh != NULL )
			{
				AddEditingObject(NewPreviewMesh);
			}

			PreviewComponent->SetSkeletalMesh(NewPreviewMesh);
		}
		else
		{
			PreviewComponent->InitAnim(true);
		}

		if (NewPreviewMesh != NULL)
		{
			PreviewScene.AddComponent(PreviewComponent, FTransform::Identity);
			for (auto Iter = AdditionalMeshes.CreateIterator(); Iter; ++Iter)
			{
				PreviewScene.AddComponent((*Iter), FTransform::Identity);
			}

			// Set up the mesh for transactions
			NewPreviewMesh->SetFlags(RF_Transactional);

			AddPreviewAttachedObjects();

			if(Viewport.IsValid())
			{
				Viewport.Pin()->SetPreviewComponent(PreviewComponent);
			}
		}

		for(auto Iter = AdditionalMeshes.CreateIterator(); Iter; ++Iter)
		{
			(*Iter)->SetMasterPoseComponent(PreviewComponent);
			(*Iter)->UpdateMasterBoneMap();
		}

		OnPreviewMeshChanged.Broadcast(NewPreviewMesh);
	}
}

void FPersona::OnPropertyChanged(UObject* ObjectBeingModified)
{
	// Re-initialize the preview when a skeletal control is being edited
	//@TODO: Should we still do this?
}

void FPersona::PostUndo(bool bSuccess)
{
	DocumentManager->RefreshAllTabs();

	FBlueprintEditor::PostUndo(bSuccess);

	// PostUndo broadcast
	OnPostUndo.Broadcast();	

	// clear up preview anim notify states
	// animnotify states are saved in AnimInstance
	// if those are undoed or redoed, they have to be 
	// cleared up, otherwise, they might have invalid data
	ClearupPreviewMeshAnimNotifyStates();
}

void FPersona::ClearupPreviewMeshAnimNotifyStates()
{
	USkeletalMeshComponent * PreviewMeshComponent = GetPreviewMeshComponent();
	if ( PreviewMeshComponent )
	{
		UAnimInstance * AnimInstantace = PreviewMeshComponent->GetAnimInstance();

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
	if (PreviewComponent->IsAnimBlueprintInstanced())
	{
		new (DebugList) FCustomDebugObject(PreviewComponent->AnimScriptInstance, LOCTEXT("PreviewObjectLabel", "Preview Instance").ToString());
	}
}

void FPersona::CreateDefaultTabContents(const TArray<UBlueprint*>& InBlueprints)
{
	FBlueprintEditor::CreateDefaultTabContents(InBlueprints);

	UBlueprint* InBlueprint = InBlueprints.Num() == 1 ? InBlueprints[0] : NULL;

	// Cache off whether or not this is an interface, since it is used to govern multiple widget's behavior
	const bool bIsInterface = (InBlueprint && InBlueprint->BlueprintType == BPTYPE_Interface);

	const bool bShowPublicView = false;
	const bool bHideNameArea = true;

	PreviewEditor = 
		SNew(SKismetInspector)
		. Kismet2(SharedThis(this))
		. ViewIdentifier(FName("PersonaPreview"))
		. SetNotifyHook(false)
		. IsEnabled(!bIsInterface)
		. ShowPublicViewControl(bShowPublicView)
		. ShowTitleArea(false)
		. HideNameArea(bHideNameArea)
		. IsPropertyEditingEnabledDelegate( FIsPropertyEditingEnabled::CreateSP(this, &FPersona::IsPropertyEditingEnabled) )
		. OnFinishedChangingProperties( FOnFinishedChangingProperties::FDelegate::CreateSP( this, &FPersona::OnFinishedChangingProperties ) );
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

void FPersona::ClearSelectedBones()
{
	if (UDebugSkelMeshComponent* Preview = GetPreviewMeshComponent())
	{
		Preview->BonesOfInterest.Empty();
	}
}

void FPersona::SetSelectedBone(USkeleton* InTargetSkeleton, const FName& BoneName, bool bRebroadcast /* = true */ )
{
	const int32 BoneIndex = InTargetSkeleton->GetReferenceSkeleton().FindBoneIndex(BoneName);
	if (BoneIndex != INDEX_NONE)
	{
		if (UDebugSkelMeshComponent* Preview = GetPreviewMeshComponent())
		{
			// need to get mesh bone base since BonesOfInterest is saved in SkeletalMeshComponent
			// and it is used by renderer. It is not Skeleton base
			const int32 MeshBoneIndex = Preview->GetBoneIndex(BoneName);

			if (MeshBoneIndex != INDEX_NONE)
			{
				Preview->BonesOfInterest.Empty();
				ClearSelectedSocket();

				if (Preview->PreviewInstance != NULL)
				{
					// Add in bone of interest only if we have a preview instance set-up
					Preview->BonesOfInterest.Add(MeshBoneIndex);

					if ( bRebroadcast )
					{
						// Broadcast that a bone has been selected
						OnBoneSelected.Broadcast( BoneName );
					}

					FAnimNode_ModifyBone& SkelControl = Preview->PreviewInstance->SingleBoneController;
					
					// Zero out the skelcontrol since we're switching bones
					SkelControl.Rotation = FRotator::ZeroRotator;
					SkelControl.Translation = FVector::ZeroVector;

					// Update who this skelcontrol is operating on
					SkelControl.BoneToModify.BoneName = BoneName;

					if( Viewport.IsValid() )
					{
						Viewport.Pin()->GetLevelViewportClient().Invalidate();
					}
				}
			}
		}
	}
}

void FPersona::SetSelectedSocket( const FSelectedSocketInfo& SocketInfo, bool bRebroadcast /* = true */ )
{
	if ( UDebugSkelMeshComponent* Preview = GetPreviewMeshComponent() )
	{
		Preview->SocketsOfInterest.Empty();
		Preview->BonesOfInterest.Empty();

		Preview->SocketsOfInterest.Add( SocketInfo );

		if ( bRebroadcast )
		{
			OnSocketSelected.Broadcast( SocketInfo );
		}

		// This populates the details panel with the information for the socket
		SetDetailObject( SocketInfo.Socket );

		if( Viewport.IsValid() )
		{
			Viewport.Pin()->GetLevelViewportClient().Invalidate();
		}
	}
}

void FPersona::ClearSelectedSocket()
{
	if (UDebugSkelMeshComponent* Preview = GetPreviewMeshComponent())
	{
		Preview->SocketsOfInterest.Empty();
		Preview->BonesOfInterest.Empty();

		SetDetailObject( NULL );
	}
}

void FPersona::ClearSelectedWindActor()
{
	Viewport.Pin()->GetAnimationViewportClient()->ClearSelectedWindActor();
}

void FPersona::RenameSocket( USkeletalMeshSocket* Socket, const FName& NewSocketName )
{
	Socket->SocketName = NewSocketName;

	// Broadcast that the skeleton tree has changed due to the new name
	OnSkeletonTreeChanged.Broadcast();

}

void FPersona::ChangeSocketParent( USkeletalMeshSocket* Socket, const FName& NewSocketParent )
{
	// Update the bone name.
	Socket->BoneName = NewSocketParent;

	// Broadcast that the skeleton tree has changed due to the new parent
	OnSkeletonTreeChanged.Broadcast();
}

void FPersona::DuplicateAndSelectSocket( const FSelectedSocketInfo& SocketInfoToDuplicate, const FName& NewParentBoneName /*= FName()*/ )
{
	check( SocketInfoToDuplicate.Socket );

	USkeletalMesh* Mesh = GetMesh();

	const FScopedTransaction Transaction( LOCTEXT( "CopySocket", "Copy Socket" ) );

	USkeletalMeshSocket* NewSocket;

	bool bModifiedSkeleton = false;

	if ( SocketInfoToDuplicate.bSocketIsOnSkeleton )
	{
		TargetSkeleton->Modify();
		bModifiedSkeleton = true;

		NewSocket = ConstructObject<USkeletalMeshSocket>( USkeletalMeshSocket::StaticClass(), TargetSkeleton );
		check(NewSocket);
	}
	else if ( Mesh )
	{
		Mesh->Modify();

		NewSocket = ConstructObject<USkeletalMeshSocket>( USkeletalMeshSocket::StaticClass(), Mesh );
		check(NewSocket);
	}
	else 
	{
		// Original socket was on the mesh, but we have no mesh. Huh?
		check( 0 );
		return;
	}

	NewSocket->SocketName = GenerateUniqueSocketName( SocketInfoToDuplicate.Socket->SocketName );
	NewSocket->BoneName = NewParentBoneName != "" ? NewParentBoneName : SocketInfoToDuplicate.Socket->BoneName;
	NewSocket->RelativeLocation = SocketInfoToDuplicate.Socket->RelativeLocation;
	NewSocket->RelativeRotation = SocketInfoToDuplicate.Socket->RelativeRotation;
	NewSocket->RelativeScale = SocketInfoToDuplicate.Socket->RelativeScale;

	if ( SocketInfoToDuplicate.bSocketIsOnSkeleton )
	{
		TargetSkeleton->Sockets.Add( NewSocket );
	}
	else if ( Mesh )
	{
		Mesh->GetMeshOnlySocketList().Add( NewSocket );
	}

	// Duplicated attached assets
	int32 NumExistingAttachedObjects = TargetSkeleton->PreviewAttachedAssetContainer.Num();
	for(int AttachedObjectIndex = 0; AttachedObjectIndex < NumExistingAttachedObjects; ++AttachedObjectIndex)
	{
		FPreviewAttachedObjectPair& Pair = TargetSkeleton->PreviewAttachedAssetContainer[AttachedObjectIndex];
		if(Pair.AttachedTo == SocketInfoToDuplicate.Socket->SocketName)
		{
			if(!bModifiedSkeleton)
			{
				bModifiedSkeleton = true;
				TargetSkeleton->Modify();
			}
			FPreviewAttachedObjectPair NewPair = Pair;
			NewPair.AttachedTo = NewSocket->SocketName;
			AttachObjectToPreviewComponent( NewPair.Object, NewPair.AttachedTo, &TargetSkeleton->PreviewAttachedAssetContainer );
		}
	}

	// Broadcast that the skeleton tree has changed due to the new socket
	OnSkeletonTreeChanged.Broadcast();

	SetSelectedSocket( FSelectedSocketInfo( NewSocket, SocketInfoToDuplicate.bSocketIsOnSkeleton ) );
}

FName FPersona::GenerateUniqueSocketName( FName InName )
{
	USkeletalMesh* Mesh = GetMesh();

	EName OriginalNameIndex = (EName)InName.GetIndex();
	int32 NewNumber = InName.GetNumber();

	// Increment NewNumber until we have a unique name (potential infinite loop if *all* int32 values are used!)
	while ( DoesSocketAlreadyExist( NULL, FText::FromName( FName( OriginalNameIndex, NewNumber ) ), TargetSkeleton->Sockets ) ||
		( Mesh ? DoesSocketAlreadyExist( NULL, FText::FromName( FName( OriginalNameIndex, NewNumber ) ), Mesh->GetMeshOnlySocketList() ) : false ) )
	{
		++NewNumber;
	}

	return FName( OriginalNameIndex, NewNumber );
}

bool FPersona::DoesSocketAlreadyExist( const USkeletalMeshSocket* InSocket, const FText& InSocketName, const TArray< USkeletalMeshSocket* >& InSocketArray ) const
{
	for ( auto SocketIt = InSocketArray.CreateConstIterator(); SocketIt; ++SocketIt )
	{
		USkeletalMeshSocket* Socket = *( SocketIt );

		if ( InSocket != Socket && Socket->SocketName.ToString() == InSocketName.ToString() )
		{
			return true;
		}
	}

	return false;
}

USkeletalMeshComponent*	FPersona::CreateNewSkeletalMeshComponent()
{
	USkeletalMeshComponent* NewComp = ConstructObject<USkeletalMeshComponent>(USkeletalMeshComponent::StaticClass());
	AdditionalMeshes.Add(NewComp);
	NewComp->SetMasterPoseComponent(GetPreviewMeshComponent());
	return NewComp;
}

void FPersona::RemoveAdditionalMesh(USkeletalMeshComponent* MeshToRemove)
{
	PreviewScene.RemoveComponent(MeshToRemove);
	AdditionalMeshes.Remove(MeshToRemove);
}

void FPersona::ClearAllAdditionalMeshes()
{
	for(auto Iter = AdditionalMeshes.CreateIterator(); Iter; ++Iter)
	{
		PreviewScene.RemoveComponent( *Iter );
	}

	AdditionalMeshes.Empty();
}

void FPersona::AddPreviewAttachedObjects()
{
	// Load up mesh attachments...
	USkeletalMesh* Mesh = GetMesh();

	if ( Mesh )
	{
		for(int32 i = 0; i < Mesh->PreviewAttachedAssetContainer.Num(); i++)
		{
			FPreviewAttachedObjectPair& PreviewAttachedObject = Mesh->PreviewAttachedAssetContainer[i];

			AttachObjectToPreviewComponent(PreviewAttachedObject.Object, PreviewAttachedObject.AttachedTo);
		}
	}

	// ...and then skeleton attachments
	for(int32 i = 0; i < TargetSkeleton->PreviewAttachedAssetContainer.Num(); i++)
	{
		FPreviewAttachedObjectPair& PreviewAttachedObject = TargetSkeleton->PreviewAttachedAssetContainer[i];

		AttachObjectToPreviewComponent(PreviewAttachedObject.Object, PreviewAttachedObject.AttachedTo);
	}
}

bool FPersona::AttachObjectToPreviewComponent( UObject* Object, FName AttachTo, FPreviewAssetAttachContainer* PreviewAssetContainer /* = NULL */ )
{
	if(GetComponentForAttachedObject(Object, AttachTo))
	{
		return false; // Already have this asset attached at this location
	}

	TSubclassOf<UActorComponent> ComponentClass = FComponentAssetBrokerage::GetPrimaryComponentForAsset(Object->GetClass());
	if(PreviewComponent && *ComponentClass && ComponentClass->IsChildOf(USceneComponent::StaticClass()))
	{
		if ( PreviewAssetContainer )
		{
			// Set up the attached object for transactions
			Object->SetFlags(RF_Transactional);
			PreviewAssetContainer->AddAttachedObject( Object, AttachTo );
		}

		//set up world info for undo
		AWorldSettings* WorldSettings = PreviewScene.GetWorld()->GetWorldSettings();
		WorldSettings->SetFlags(RF_Transactional);
		WorldSettings->Modify();

		USceneComponent* SceneComponent = ConstructObject<USceneComponent>(ComponentClass, WorldSettings);
		SceneComponent->SetFlags(RF_Transactional);

		FComponentAssetBrokerage::AssignAssetToComponent(SceneComponent, Object);

		if(UParticleSystemComponent* NewPSysComp = Cast<UParticleSystemComponent>(SceneComponent))
		{
			//maybe this should happen in FComponentAssetBrokerage::AssignAssetToComponent?
			NewPSysComp->SetTickGroup(TG_PostUpdateWork);
		}

		//set up preview component for undo
		PreviewComponent->SetFlags(RF_Transactional);
		PreviewComponent->Modify();

		// Attach component to the preview component
		SceneComponent->AttachTo(PreviewComponent, AttachTo);
		SceneComponent->RegisterComponent();
		return true;
	}
	return false;
}

void FPersona::RemoveAttachedObjectFromPreviewComponent(UObject* Object, FName AttachedTo)
{
	// clean up components	
	if (PreviewComponent)
	{
		AWorldSettings* WorldSettings = PreviewScene.GetWorld()->GetWorldSettings();
		WorldSettings->SetFlags(RF_Transactional);
		WorldSettings->Modify();

		//set up preview component for undo
		PreviewComponent->SetFlags(RF_Transactional);
		PreviewComponent->Modify();

		for (int32 I=PreviewComponent->AttachChildren.Num()-1; I >= 0; --I) // Iterate backwards because Cleancomponent will remove from AttachChildren
		{
			USceneComponent* ChildComponent = PreviewComponent->AttachChildren[I];
			UObject* Asset = FComponentAssetBrokerage::GetAssetFromComponent(ChildComponent);

			if( Asset == Object && ChildComponent->AttachSocketName == AttachedTo)
			{
				// PreviewComponet will be cleaned up by PreviewScene, 
				// but if anything is attached, it won't be cleaned up, 
				// so we'll need to clean them up manually
				CleanupComponent(PreviewComponent->AttachChildren[I]);
				break;
			}
		}
	}
}

USceneComponent* FPersona::GetComponentForAttachedObject(UObject* Object, FName AttachedTo)
{
	if (PreviewComponent)
	{
		for (int32 I=0; I < PreviewComponent->AttachChildren.Num(); ++I)
		{
			USceneComponent* ChildComponent = PreviewComponent->AttachChildren[I];
			UObject* Asset = FComponentAssetBrokerage::GetAssetFromComponent(ChildComponent);

			if( Asset == Object && ChildComponent->AttachSocketName == AttachedTo)
			{
				return ChildComponent;
			}
		}
	}
	return NULL;
}

void FPersona::RemoveAttachedComponent( bool bRemovePreviewAttached /* = true */ )
{
	TMap<UObject*, TArray<FName>> PreviewAttachedObjects;

	if( !bRemovePreviewAttached )
	{
		for(auto Iter = TargetSkeleton->PreviewAttachedAssetContainer.CreateConstIterator(); Iter; ++Iter)
		{
			PreviewAttachedObjects.FindOrAdd(Iter->Object).Add(Iter->AttachedTo);
		}

		if ( USkeletalMesh* PreviewMesh = GetMesh() )
		{
			for(auto Iter = PreviewMesh->PreviewAttachedAssetContainer.CreateConstIterator(); Iter; ++Iter)
			{
				PreviewAttachedObjects.FindOrAdd(Iter->Object).Add(Iter->AttachedTo);
			}
		}
	}

	// clean up components	
	if (PreviewComponent)
	{
		for (int32 I=PreviewComponent->AttachChildren.Num()-1; I >= 0; --I) // Iterate backwards because Cleancomponent will remove from AttachChildren
		{
			USceneComponent* ChildComponent = PreviewComponent->AttachChildren[I];
			UObject* Asset = FComponentAssetBrokerage::GetAssetFromComponent(ChildComponent);

			bool bRemove = true;

			//are we supposed to leave assets that came from the skeleton
			if(	!bRemovePreviewAttached )
			{
				//could this asset have come from the skeleton
				if(PreviewAttachedObjects.Contains(Asset))
				{
					if(PreviewAttachedObjects.Find(Asset)->Contains(ChildComponent->AttachSocketName))
					{
						bRemove = false;
					}
				}
			}

			if(bRemove)
			{
				// PreviewComponet will be cleaned up by PreviewScene, 
				// but if anything is attached, it won't be cleaned up, 
				// so we'll need to clean them up manually
				CleanupComponent(PreviewComponent->AttachChildren[I]);
			}
		}

		if( bRemovePreviewAttached )
		{
			PreviewComponent->AttachChildren.Empty();
		}
	}
}

void FPersona::CleanupComponent(USceneComponent* Component)
{
	if (Component)
	{
		for (int32 I=0; I<Component->AttachChildren.Num(); ++I)
		{
			CleanupComponent(Component->AttachChildren[I]);
		}

		Component->AttachChildren.Empty();
		Component->DestroyComponent();
	}
}

void FPersona::DeselectAll()
{
	ClearSelectedBones();
	ClearSelectedSocket();
	ClearSelectedWindActor();

	OnAllDeselected.Broadcast();
}

FReply FPersona::OnClickEditMesh()
{
	USkeletalMesh* PreviewMesh = TargetSkeleton->GetPreviewMesh();
	if(PreviewMesh)
	{
		UpdateSelectionDetails(PreviewMesh, *PreviewMesh->GetName());
	}
	return FReply::Handled();
}


void FPersona::PostRedo(bool bSuccess)
{
	DocumentManager->RefreshAllTabs();

	FBlueprintEditor::PostRedo(bSuccess);

	// PostUndo broadcast, OnPostRedo
	OnPostUndo.Broadcast();

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
	GEditor->SyncBrowserToObjects( ObjectsToSyncTo );
}

void FPersona::OnCommandGenericDelete()
{
	OnGenericDelete.Broadcast();
}

void FPersona::UndoAction()
{
	GEditor->UndoTransaction();
}

void FPersona::RedoAction()
{
	GEditor->RedoTransaction();
}


// get record configuration
bool GetRecordConfig( FString & AssetPath, FString & AssetName, float & Duration )
{
	TSharedRef<SCreateAnimationDlg> NewAnimDlg = 
		SNew(SCreateAnimationDlg);

	if (NewAnimDlg->ShowModal() != EAppReturnType::Cancel)
	{
		AssetPath = NewAnimDlg->GetFullAssetPath();
		AssetName = NewAnimDlg->GetAssetName();
		Duration = NewAnimDlg->GetDuration();
		return true;
	}

	return false;
}

void FPersona::RecordAnimation()
{
	// options
	// - whether restart anim tree?
	// - sample rate for now is 30

	if (!PreviewComponent || !PreviewComponent->SkeletalMesh)
	{
		// error
		return;
	}

	// ask for path
	FString AssetPath;
	FString AssetName;
	float Duration = 0.f;

	if ( GetRecordConfig(AssetPath, AssetName, Duration) )
	{
		// create the asset
		UObject * 	Parent = CreatePackage(NULL, *AssetPath);
		UObject * Object = LoadObject<UObject>(Parent, *AssetName, NULL, LOAD_None, NULL);
		// if object with same name exists, warn user
		if (Object)
		{
			FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "Error_AssetExist", "Asset with same name exists. Can't overwrite another asset") ) ;
			return; // Move on to next sequence...
		}

		// If not, create new one now.
		UAnimSequence * NewSeq = ConstructObject<UAnimSequence>( UAnimSequence::StaticClass(), Parent, *AssetName, RF_Public|RF_Standalone );
		if (NewSeq)
		{
			// restart anim tree
			//PreviewComponent->InitAnim(true);

			// set skeleton
			NewSeq->SetSkeleton ((PreviewComponent->SkeletalMesh)? PreviewComponent->SkeletalMesh->Skeleton : TargetSkeleton);
			// Notify the asset registry
			FAssetRegistryModule::AssetCreated(NewSeq);
			Recorder.StartRecord(PreviewComponent, NewSeq, Duration);
		}
	}
}

void FPersona::ChangeSkeletonPreviewMesh()
{
	// Menu option cannot be called unless this is valid
	if(TargetSkeleton->GetPreviewMesh() != PreviewComponent->SkeletalMesh)
	{
		const FScopedTransaction Transaction( LOCTEXT("ChangeSkeletonPreviewMesh", "Change Skeleton Preview Mesh") );
		TargetSkeleton->SetPreviewMesh(PreviewComponent->SkeletalMesh);

		FFormatNamedArguments Args;
		Args.Add( TEXT("PreviewMeshName"), FText::FromString( PreviewComponent->SkeletalMesh->GetName() ) );
		Args.Add( TEXT("TargetSkeletonName"), FText::FromString( TargetSkeleton->GetName() ) );
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

bool FPersona::IsAnimationBeingRecorded() const
{
	return (Recorder.GetAnimationObject()!=NULL);
}

void FPersona::Tick(float DeltaTime)
{
	FBlueprintEditor::Tick(DeltaTime);

	if (IsAnimationBeingRecorded())
	{
		// make sure you don't allow switch previewcomponent
		if (Recorder.UpdateRecord(PreviewComponent, DeltaTime) == false)
		{
			// before stop record, save the animation, it will get cleared after stop recording
			const UAnimSequence * NewAnim = Recorder.GetAnimationObject();
			Recorder.StopRecord();

			// notify to user
			if (NewAnim)
			{
				const FText NotificationText = FText::Format( LOCTEXT("RecordAnimation", "'{0}' has been successfully recorded" ), FText::FromString( NewAnim->GetName() ) );

				//This is not showing well in the Persona, so opening dialog first. 
				//right now it will crash if you don't wait until end of the record, so it is important for users to know
				//this is done
/*				
				FNotificationInfo Info(NotificationText);
				Info.ExpireDuration = 3.0f;
				Info.bUseLargeFont = false;
				TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
				if ( Notification.IsValid() )
				{
					Notification->SetCompletionState( SNotificationItem::CS_Success );
				}*/

				FMessageDialog::Open(EAppMsgType::Ok, NotificationText);
			}
		}
	}
}

bool FPersona::CanRecordAnimation() const
{
	return (!IsAnimationBeingRecorded());
}

bool FPersona::CanChangeSkeletonPreviewMesh() const
{
	return PreviewComponent && PreviewComponent->SkeletalMesh;
}

bool FPersona::IsRecordAvailable() const
{
	return (GetCurrentMode() == FPersonaModes::AnimBlueprintEditMode);
}

/////////////////////////////////////////////////////

FAnimationRecorder::FAnimationRecorder()
	: SampleRate(30)
	, AnimationObject(NULL)
{

}

FAnimationRecorder::~FAnimationRecorder()
{
	StopRecord();
}

void FAnimationRecorder::StartRecord(USkeletalMeshComponent * Component, UAnimSequence * InAnimationObject, float InDuration)
{
	Duration = InDuration;
	check (Duration > 0.f);
	TimePassed = 0.f;
	AnimationObject = InAnimationObject;

	AnimationObject->RawAnimationData.Empty();
	AnimationObject->TrackToSkeletonMapTable.Empty();
	AnimationObject->AnimationTrackNames.Empty();

	PreviousSpacesBases = Component->SpaceBases;

	check (SampleRate!=0.f);
	const float IntervalTime = 1.f/SampleRate;
	// prepare to record
	// how many frames we need?
	int32 MaxFrame = Duration/IntervalTime+1;
	int32 NumBones = PreviousSpacesBases.Num();
	check (MaxFrame > 0);
	check (Component->SkeletalMesh);

	LastFrame = 0;
	AnimationObject->SequenceLength = Duration;
	AnimationObject->NumFrames = MaxFrame;

	USkeleton * AnimSkeleton = AnimationObject->GetSkeleton();
	// add all frames
	for (int32 BoneIndex=0; BoneIndex <PreviousSpacesBases.Num(); ++BoneIndex)
	{
		// verify if this bone exists in skeleton
		int32 BoneTreeIndex = AnimSkeleton->GetSkeletonBoneIndexFromMeshBoneIndex(Component->SkeletalMesh, BoneIndex);
		if (BoneTreeIndex != INDEX_NONE)
		{
			FName BoneTreeName = AnimSkeleton->GetReferenceSkeleton().GetBoneName(BoneTreeIndex);
			int32 NewTrack = AnimationObject->RawAnimationData.AddZeroed(1);
			AnimationObject->AnimationTrackNames.Add(BoneTreeName);

			FRawAnimSequenceTrack& RawTrack = AnimationObject->RawAnimationData[NewTrack];
			// add mapping to skeleton bone track
			AnimationObject->TrackToSkeletonMapTable.Add(FTrackToSkeletonMap(BoneTreeIndex));
			RawTrack.PosKeys.AddZeroed(MaxFrame);
			RawTrack.RotKeys.AddZeroed(MaxFrame);
			RawTrack.ScaleKeys.AddZeroed(MaxFrame);
		}
	}

	// record the first frame;
	Record(Component, PreviousSpacesBases, 0);
}

void FAnimationRecorder::StopRecord()
{
	if (AnimationObject)
	{
		// if LastFrame doesn't match with NumFrames, we need to adjust data
		if ( LastFrame + 1 < AnimationObject->NumFrames )
		{
			int32 NewMaxFrame = LastFrame + 1;
			int32 OldMaxFrame = AnimationObject->NumFrames;
			// Set New Frames/Time
			AnimationObject->NumFrames = NewMaxFrame;
			AnimationObject->SequenceLength = (float)(NewMaxFrame)/SampleRate;

			for (int32 TrackId=0; TrackId <AnimationObject->RawAnimationData.Num(); ++TrackId)
			{
				FRawAnimSequenceTrack& RawTrack = AnimationObject->RawAnimationData[TrackId];
				// add mapping to skeleton bone track
				RawTrack.PosKeys.RemoveAt(NewMaxFrame, OldMaxFrame-NewMaxFrame);
				RawTrack.RotKeys.RemoveAt(NewMaxFrame, OldMaxFrame-NewMaxFrame);
				RawTrack.ScaleKeys.RemoveAt(NewMaxFrame, OldMaxFrame-NewMaxFrame);
			}
		}

		AnimationObject->PostProcessSequence();
		AnimationObject->MarkPackageDirty();
		AnimationObject = NULL;

		PreviousSpacesBases.Empty();
	}
}

// return false if nothing to update
// return true if it has properly updated
bool FAnimationRecorder::UpdateRecord(USkeletalMeshComponent * Component, float DeltaTime)
{
	const float MaxDelta = 1.f/10.f;
	float UseDelta = FMath::Min<float>(DeltaTime, MaxDelta);

	float PreviousTimePassed = TimePassed;
	TimePassed += UseDelta;

	check (SampleRate!=0.f);
	const float IntervalTime = 1.f/SampleRate;

	// time passed has been updated
	// now find what frames we need to update
	int32 MaxFrame = Duration/IntervalTime;
	int32 FramesRecorded = (PreviousTimePassed / IntervalTime);
	int32 FramesToRecord = FMath::Min<int32>(TimePassed / IntervalTime, MaxFrame);

	TArray<FTransform> & SpaceBases = Component->SpaceBases;

	check (SpaceBases.Num() == PreviousSpacesBases.Num());

	TArray<FTransform> BlendedSpaceBases;
	BlendedSpaceBases.AddZeroed(SpaceBases.Num());

	// if we need to record frame
	while (FramesToRecord > FramesRecorded)
	{
		// find what frames we need to record
		// convert to time
		float NewTime = (FramesRecorded + 1) * IntervalTime;

		float PercentageOfNew = (NewTime - PreviousTimePassed)/UseDelta;
		//float PercentageOfOld = (TimePassed-NewTime)/UseDelta;

		// make sure this is almost 1 = PercentageOfNew
		//ensure ( FMath::IsNearlyEqual(PercentageOfNew + PercentageOfOld, 1.f, KINDA_SMALL_NUMBER));

		// for now we just concern component space, not skeleton space
		for (int32 BoneIndex=0; BoneIndex<SpaceBases.Num(); ++BoneIndex)
		{
			BlendedSpaceBases[BoneIndex].Blend(PreviousSpacesBases[BoneIndex], SpaceBases[BoneIndex], PercentageOfNew);
		}

		Record(Component, BlendedSpaceBases, FramesRecorded + 1);
		++FramesRecorded;
	}

	//save to current transform
	PreviousSpacesBases = Component->SpaceBases;

	// if we reached MaxFrame, return false;
	return (MaxFrame > FramesRecorded);
}

void FAnimationRecorder::Record( USkeletalMeshComponent * Component, TArray<FTransform> SpacesBases, int32 FrameToAdd )
{
	if (ensure (AnimationObject))
	{
		USkeleton * AnimSkeleton = AnimationObject->GetSkeleton();
		for (int32 TrackIndex=0; TrackIndex <AnimationObject->RawAnimationData.Num(); ++TrackIndex)
		{
			// verify if this bone exists in skeleton
			int32 BoneTreeIndex = AnimationObject->GetSkeletonIndexFromTrackIndex(TrackIndex);
			if (BoneTreeIndex != INDEX_NONE)
			{
				int32 BoneIndex = AnimSkeleton->GetMeshBoneIndexFromSkeletonBoneIndex(Component->SkeletalMesh, BoneTreeIndex);
				int32 ParentIndex = Component->SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);
				FTransform LocalTransform = SpacesBases[BoneIndex];
				if ( ParentIndex != INDEX_NONE )
				{
					LocalTransform.SetToRelativeTransform(SpacesBases[ParentIndex]);
				}

				FRawAnimSequenceTrack& RawTrack = AnimationObject->RawAnimationData[TrackIndex];
				RawTrack.PosKeys[FrameToAdd] = LocalTransform.GetTranslation();
				RawTrack.RotKeys[FrameToAdd] = LocalTransform.GetRotation();
				RawTrack.ScaleKeys[FrameToAdd] = LocalTransform.GetScale3D();
			}
		}

		LastFrame = FrameToAdd;
	}
}

#undef LOCTEXT_NAMESPACE

