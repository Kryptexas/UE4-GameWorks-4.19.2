// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "StaticMeshEditorModule.h"
#include "AssetRegistryModule.h"

#include "StaticMeshEditor.h"
#include "SStaticMeshEditorViewport.h"
#include "StaticMeshEditorViewportClient.h"
#include "StaticMeshEditorTools.h"
#include "StaticMeshEditorActions.h"

#include "UnrealEd.h"
#include "ISocketManager.h"
#include "LinkedObjDrawUtils.h"
#include "PreviewScene.h"
#include "ScopedTransaction.h"
#include "BusyCursor.h"
#include "FbxMeshUtils.h"
#include "../Private/GeomFitUtils.h"
#include "EditorViewportCommands.h"
#include "Editor/UnrealEd/Private/ConvexDecompTool.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "StaticMeshEditor"

DEFINE_LOG_CATEGORY_STATIC(LogStaticMeshEditor, Log, All);

const FName FStaticMeshEditor::ViewportTabId( TEXT( "StaticMeshEditor_Viewport" ) );
const FName FStaticMeshEditor::PropertiesTabId( TEXT( "StaticMeshEditor_Properties" ) );
const FName FStaticMeshEditor::SocketManagerTabId( TEXT( "StaticMeshEditor_SocketManager" ) );
const FName FStaticMeshEditor::CollisionTabId( TEXT( "StaticMeshEditor_Collision" ) );
const FName FStaticMeshEditor::GenerateUniqueUVsTabId( TEXT( "StaticMeshEditor_GenerateUniqueUVs" ) );


void FStaticMeshEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(TabManager);

	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	TabManager->RegisterTabSpawner( ViewportTabId, FOnSpawnTab::CreateSP(this, &FStaticMeshEditor::SpawnTab_Viewport) )
		.SetDisplayName( LOCTEXT("ViewportTab", "Viewport") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );
	
	TabManager->RegisterTabSpawner( PropertiesTabId, FOnSpawnTab::CreateSP(this, &FStaticMeshEditor::SpawnTab_Properties) )
		.SetDisplayName( LOCTEXT("PropertiesTab", "Details") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );
	
	TabManager->RegisterTabSpawner( SocketManagerTabId, FOnSpawnTab::CreateSP(this, &FStaticMeshEditor::SpawnTab_SocketManager) )
		.SetDisplayName( LOCTEXT("SocketManagerTab", "Socket Manager") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );
	
	TabManager->RegisterTabSpawner( CollisionTabId, FOnSpawnTab::CreateSP(this, &FStaticMeshEditor::SpawnTab_Collision) )
		.SetDisplayName( LOCTEXT("CollisionTab", "Convex Decomposition") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );
	
	TabManager->RegisterTabSpawner( GenerateUniqueUVsTabId, FOnSpawnTab::CreateSP(this, &FStaticMeshEditor::SpawnTab_GenerateUniqueUVs) )
		.SetDisplayName( LOCTEXT("GenerateUniqueUVsTab", "Generate Unique UVs") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );
}

void FStaticMeshEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(TabManager);

	TabManager->UnregisterTabSpawner( ViewportTabId );
	TabManager->UnregisterTabSpawner( PropertiesTabId );
	TabManager->UnregisterTabSpawner( SocketManagerTabId );
	TabManager->UnregisterTabSpawner( CollisionTabId );
	TabManager->UnregisterTabSpawner( GenerateUniqueUVsTabId );
}


FStaticMeshEditor::~FStaticMeshEditor()
{
	FAssetEditorToolkit::OnPostReimport().RemoveAll(this);

	GEditor->UnregisterForUndo( this );
	GEditor->OnObjectReimported().RemoveAll(this);
}

void FStaticMeshEditor::InitStaticMeshEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UStaticMesh* ObjectToEdit )
{
	FAssetEditorToolkit::OnPostReimport().AddRaw(this, &FStaticMeshEditor::OnPostReimport);

	// Support undo/redo
	ObjectToEdit->SetFlags( RF_Transactional );

	GEditor->RegisterForUndo( this );

	// Register our commands. This will only register them if not previously registered
	FStaticMeshEditorCommands::Register();

	// Register to be notified when an object is reimported.
	GEditor->OnObjectReimported().AddSP(this, &FStaticMeshEditor::OnObjectReimported);

	BindCommands();

	Viewport = SNew(SStaticMeshEditorViewport)
		.StaticMeshEditor(SharedThis(this))
		.ObjectToEdit(ObjectToEdit);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bHideActorNameArea = true;
	DetailsViewArgs.bObjectsUseNameArea = false;
	DetailsViewArgs.NotifyHook = this;

	StaticMeshDetailsView = PropertyEditorModule.CreateDetailView( DetailsViewArgs );

	FOnGetDetailCustomizationInstance LayoutCustomStaticMeshProperties = FOnGetDetailCustomizationInstance::CreateSP( this, &FStaticMeshEditor::MakeStaticMeshDetails );
	StaticMeshDetailsView->RegisterInstancedCustomPropertyLayout( UStaticMesh::StaticClass(), LayoutCustomStaticMeshProperties );

	SetEditorMesh(ObjectToEdit);

	BuildSubTools();

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout( "Standalone_StaticMeshEditor_Layout_v3" )
	->AddArea
	(
		FTabManager::NewPrimaryArea() ->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.1f)
			->SetHideTabWell( true )
			->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
		)
		->Split
		(
			FTabManager::NewSplitter() ->SetOrientation(Orient_Horizontal) ->SetSizeCoefficient(0.9f)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.6f)
				->AddTab(ViewportTabId, ETabState::OpenedTab) ->SetHideTabWell( true )
			)
			->Split
			(
				FTabManager::NewSplitter() ->SetOrientation(Orient_Vertical) ->SetSizeCoefficient(0.2f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.5f)
					->AddTab(PropertiesTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.5f)
					->AddTab(SocketManagerTabId, ETabState::ClosedTab)
					->AddTab(CollisionTabId, ETabState::ClosedTab)
					->AddTab(GenerateUniqueUVsTabId, ETabState::ClosedTab)
				)
			)
		)
	);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor( Mode, InitToolkitHost, StaticMeshEditorAppIdentifier, StandaloneDefaultLayout, bCreateDefaultToolbar, bCreateDefaultStandaloneMenu, ObjectToEdit );
	
	ExtendMenu();
	ExtendToolBar();
	RegenerateMenusAndToolbars();
}

TSharedRef<IDetailCustomization> FStaticMeshEditor::MakeStaticMeshDetails()
{
	TSharedRef<FStaticMeshDetails> NewDetails = MakeShareable( new FStaticMeshDetails( *this ) );
	StaticMeshDetails = NewDetails;
	return NewDetails;
}

void FStaticMeshEditor::ExtendMenu()
{
	struct Local
	{
		static void FillEditMenu( FMenuBuilder& InMenuBuilder )
		{
			InMenuBuilder.BeginSection("Sockets", LOCTEXT("EditStaticMeshSockets", "Sockets"));
			{
				InMenuBuilder.AddMenuEntry( FGenericCommands::Get().Delete, "DeleteSocket", LOCTEXT("DeleteSocket", "Delete Socket"), LOCTEXT("DeleteSocketToolTip", "Deletes the selected socket from the mesh.") );
				InMenuBuilder.AddMenuEntry( FGenericCommands::Get().Duplicate, "DuplicateSocket", LOCTEXT("DuplicateSocket", "Duplicate Socket"), LOCTEXT("DuplicateSocketToolTip", "Duplicates the selected socket.") );
			}
			InMenuBuilder.EndSection();
		}

		static void FillMeshMenu( FMenuBuilder& InMenuBuilder )
		{
			// @todo mainframe: These menus, and indeed all menus like them, should be updated with extension points, plus expose public module
			// access to extending the menus.  They may also need to extend the command list, or be able to PUSH a command list of their own.
			// If we decide to only allow PUSHING, then nothing else should be needed (happens by extender automatically).  But if we want to
			// augment the asset editor's existing command list, then we need to think about how to expose support for that.

			InMenuBuilder.BeginSection("MeshFindSource");
			{
				InMenuBuilder.AddMenuEntry(FStaticMeshEditorCommands::Get().FindSource);
			}
			InMenuBuilder.EndSection();
			
			InMenuBuilder.BeginSection("MeshChange");
			{
				InMenuBuilder.AddMenuEntry(FStaticMeshEditorCommands::Get().ChangeMesh);
			}
			InMenuBuilder.EndSection();
		}

		static void FillCollisionMenu( FMenuBuilder& InMenuBuilder )
		{
			InMenuBuilder.BeginSection("CollisionEditCollision");
			{
				InMenuBuilder.AddMenuEntry(FStaticMeshEditorCommands::Get().CreateDOP6);
				InMenuBuilder.AddMenuEntry(FStaticMeshEditorCommands::Get().CreateDOP10X);
				InMenuBuilder.AddMenuEntry(FStaticMeshEditorCommands::Get().CreateDOP10Y);
				InMenuBuilder.AddMenuEntry(FStaticMeshEditorCommands::Get().CreateDOP10Z);
				InMenuBuilder.AddMenuEntry(FStaticMeshEditorCommands::Get().CreateDOP18);
				InMenuBuilder.AddMenuEntry(FStaticMeshEditorCommands::Get().CreateDOP26);	

				InMenuBuilder.AddMenuEntry(FStaticMeshEditorCommands::Get().CreateSphereCollision);
				InMenuBuilder.AddMenuEntry(FStaticMeshEditorCommands::Get().RemoveCollision);
				InMenuBuilder.AddMenuEntry(FStaticMeshEditorCommands::Get().ConvertBoxesToConvex);
			}
			InMenuBuilder.EndSection();

			InMenuBuilder.BeginSection("CollisionAutoConvexCollision");
			{
				InMenuBuilder.AddMenuEntry(FStaticMeshEditorCommands::Get().CreateAutoConvexCollision);
			}
			InMenuBuilder.EndSection();

			InMenuBuilder.BeginSection("CollisionCopy");
			{
				InMenuBuilder.AddMenuEntry(FStaticMeshEditorCommands::Get().CopyCollisionFromSelectedMesh);
			}
			InMenuBuilder.EndSection();
		}

		static void GenerateMeshAndCollisionMenuBars( FMenuBarBuilder& InMenuBarBuilder)
		{
			InMenuBarBuilder.AddPullDownMenu(
				LOCTEXT("StaticMeshEditorMeshMenu", "Mesh"),
				LOCTEXT("StaticMeshEditorMeshMenu_ToolTip", "Opens a menu with commands for altering this mesh"),
				FNewMenuDelegate::CreateStatic(&Local::FillMeshMenu),
				"Mesh");

			InMenuBarBuilder.AddPullDownMenu(
				LOCTEXT("StaticMeshEditorCollisionMenu", "Collision"),
				LOCTEXT("StaticMeshEditorCollisionMenu_ToolTip", "Opens a menu with commands for editing this mesh's collision"),
				FNewMenuDelegate::CreateStatic(&Local::FillCollisionMenu),
				"Collision");
		}
	};
	
	TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender);

	MenuExtender->AddMenuExtension(
		"EditHistory",
		EExtensionHook::After,
		GetToolkitCommands(),
		FMenuExtensionDelegate::CreateStatic( &Local::FillEditMenu ) );

	MenuExtender->AddMenuBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FMenuBarExtensionDelegate::CreateStatic( &Local::GenerateMeshAndCollisionMenuBars )
		);
	
	AddMenuExtender(MenuExtender);
	
	IStaticMeshEditorModule* StaticMeshEditorModule = &FModuleManager::LoadModuleChecked<IStaticMeshEditorModule>( "StaticMeshEditor" );
	AddMenuExtender(StaticMeshEditorModule->GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
}

void FStaticMeshEditor::AddReferencedObjects( FReferenceCollector& Collector )
{
	Collector.AddReferencedObject( StaticMesh );
}

TSharedRef<SDockTab> FStaticMeshEditor::SpawnTab_Viewport( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId() == ViewportTabId );
	
	TSharedRef<SDockTab> SpawnedTab =
	 SNew(SDockTab)
		.Label( LOCTEXT("StaticMeshViewport_TabTitle", "Viewport") )
		[
			Viewport.ToSharedRef()
		];

	Viewport->SetParentTab( SpawnedTab );

	return SpawnedTab;
}

TSharedRef<SDockTab> FStaticMeshEditor::SpawnTab_Properties( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId() == PropertiesTabId );

	return SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("StaticMeshEditor.Tabs.Properties") )
		.Label( LOCTEXT("StaticMeshProperties_TabTitle", "Details") )
		[
			StaticMeshDetailsView.ToSharedRef()
		];
}

TSharedRef<SDockTab> FStaticMeshEditor::SpawnTab_SocketManager( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId() == SocketManagerTabId );

	return SNew(SDockTab)
		.Label( LOCTEXT("StaticMeshSocketManager_TabTitle", "Socket Manager") )
		[
			SocketManager.ToSharedRef()
		];
}

TSharedRef<SDockTab> FStaticMeshEditor::SpawnTab_Collision( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId() == CollisionTabId );

	return SNew(SDockTab)
		.Label( LOCTEXT("StaticMeshConvexDecomp_TabTitle", "Convex Decomposition") )
		[
			ConvexDecomposition.ToSharedRef()
		];
}

TSharedRef<SDockTab> FStaticMeshEditor::SpawnTab_GenerateUniqueUVs( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId() == GenerateUniqueUVsTabId );
	
#if !PLATFORM_WINDOWS
	FText ErrorMessage = FText::Format( LOCTEXT("StaticMeshGenerateUniqueUVs_Unsupported", "Generate Unique UVs not yet implemented for {0}."), FText::FromString( FPlatformProperties::IniPlatformName() ) );
	OpenMsgDlgInt( EAppMsgType::Ok, ErrorMessage, LOCTEXT("StaticMeshGenerateUniqueUVs_UnsupportedErrorCaption", "Error") );
#endif
	
	return SNew(SDockTab)
		.Label( LOCTEXT("StaticMeshGenerateUniqueUVs_TabTitle", "Generate Unique UVs") )
		[
#if PLATFORM_WINDOWS
			GenerateUniqueUVs.ToSharedRef()
#else
			SNew(STextBlock)
			.Text( ErrorMessage )
#endif
		];
}

void FStaticMeshEditor::BindCommands()
{
	const FStaticMeshEditorCommands& Commands = FStaticMeshEditorCommands::Get();

	const TSharedRef<FUICommandList>& UICommandList = GetToolkitCommands();

	UICommandList->MapAction( FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP( this, &FStaticMeshEditor::DeleteSelectedSockets ),
		FCanExecuteAction::CreateSP( this, &FStaticMeshEditor::HasSelectedSockets ));

	UICommandList->MapAction( FGenericCommands::Get().Undo, 
		FExecuteAction::CreateSP( this, &FStaticMeshEditor::UndoAction ) );

	UICommandList->MapAction( FGenericCommands::Get().Redo, 
		FExecuteAction::CreateSP( this, &FStaticMeshEditor::RedoAction ) );

	UICommandList->MapAction(
		FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateSP(this, &FStaticMeshEditor::DuplicateSelectedSocket),
		FCanExecuteAction::CreateSP(this, &FStaticMeshEditor::HasSelectedSockets));

	UICommandList->MapAction(
		FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &FStaticMeshEditor::RequestRenameSelectedSocket),
		FCanExecuteAction::CreateSP(this, &FStaticMeshEditor::HasSelectedSockets));

	UICommandList->MapAction(
		Commands.CreateDOP6,
		FExecuteAction::CreateSP(this, &FStaticMeshEditor::GenerateKDop, KDopDir6, (uint32)6));

	UICommandList->MapAction(
		Commands.CreateDOP10X,
		FExecuteAction::CreateSP(this, &FStaticMeshEditor::GenerateKDop, KDopDir10X, (uint32)10));

	UICommandList->MapAction(
		Commands.CreateDOP10Y,
		FExecuteAction::CreateSP(this, &FStaticMeshEditor::GenerateKDop, KDopDir10Y, (uint32)10));

	UICommandList->MapAction(
		Commands.CreateDOP10Z,
		FExecuteAction::CreateSP(this, &FStaticMeshEditor::GenerateKDop, KDopDir10Z, (uint32)10));

	UICommandList->MapAction(
		Commands.CreateDOP18,
		FExecuteAction::CreateSP(this, &FStaticMeshEditor::GenerateKDop, KDopDir18, (uint32)18));

	UICommandList->MapAction(
		Commands.CreateDOP26,
		FExecuteAction::CreateSP(this, &FStaticMeshEditor::GenerateKDop, KDopDir26, (uint32)26));

	UICommandList->MapAction(
		Commands.CreateSphereCollision,
		FExecuteAction::CreateSP(this, &FStaticMeshEditor::OnCollisionSphere));

	UICommandList->MapAction(
		Commands.RemoveCollision,
		FExecuteAction::CreateSP(this, &FStaticMeshEditor::OnRemoveCollision));

	UICommandList->MapAction(
		Commands.ConvertBoxesToConvex,
		FExecuteAction::CreateSP(this, &FStaticMeshEditor::OnConvertBoxToConvexCollision));

	UICommandList->MapAction(
		Commands.CopyCollisionFromSelectedMesh,
		FExecuteAction::CreateSP(this, &FStaticMeshEditor::OnCopyCollisionFromSelectedStaticMesh));

	// Mesh menu
	UICommandList->MapAction(
		Commands.FindSource,
		FExecuteAction::CreateSP(this, &FStaticMeshEditor::ExecuteFindInExplorer),
		FCanExecuteAction::CreateSP(this, &FStaticMeshEditor::CanExecuteSourceCommands));

	UICommandList->MapAction(
		Commands.ChangeMesh,
		FExecuteAction::CreateSP(this, &FStaticMeshEditor::OnChangeMesh));

	// Collision Menu
	UICommandList->MapAction(
		Commands.CreateAutoConvexCollision,
		FExecuteAction::CreateSP(this, &FStaticMeshEditor::OnConvexDecomposition));
}

void FStaticMeshEditor::ExtendToolBar()
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder, TSharedPtr< class STextComboBox > UVChannelCombo, TSharedPtr< class STextComboBox > LODLevelCombo)
		{
			ToolbarBuilder.BeginSection("Realtime");
			{
				ToolbarBuilder.AddToolBarButton(FEditorViewportCommands::Get().ToggleRealTime);
			}
			ToolbarBuilder.EndSection();
	
			ToolbarBuilder.BeginSection("Command");
			{
				ToolbarBuilder.AddToolBarButton(FStaticMeshEditorCommands::Get().SetShowSockets);
				ToolbarBuilder.AddToolBarButton(FStaticMeshEditorCommands::Get().SetShowWireframe);
				ToolbarBuilder.AddToolBarButton(FStaticMeshEditorCommands::Get().SetShowVertexColor);
				ToolbarBuilder.AddToolBarButton(FStaticMeshEditorCommands::Get().SetShowGrid);
				ToolbarBuilder.AddToolBarButton(FStaticMeshEditorCommands::Get().SetShowBounds);
				ToolbarBuilder.AddToolBarButton(FStaticMeshEditorCommands::Get().SetShowCollision);
				ToolbarBuilder.AddToolBarButton(FStaticMeshEditorCommands::Get().SetShowPivot);
				ToolbarBuilder.AddToolBarButton(FStaticMeshEditorCommands::Get().SetShowNormals);
				ToolbarBuilder.AddToolBarButton(FStaticMeshEditorCommands::Get().SetShowTangents);
				ToolbarBuilder.AddToolBarButton(FStaticMeshEditorCommands::Get().SetShowBinormals);
				ToolbarBuilder.AddToolBarButton(FStaticMeshEditorCommands::Get().SetDrawUVs);
				ToolbarBuilder.AddToolBarButton(FStaticMeshEditorCommands::Get().SetDrawAdditionalData);
			}
			ToolbarBuilder.EndSection();
	
			ToolbarBuilder.BeginSection("UV");
			{
				ToolbarBuilder.AddWidget(UVChannelCombo.ToSharedRef());
			}
			ToolbarBuilder.EndSection();
	
			ToolbarBuilder.BeginSection("Camera");
			{
				ToolbarBuilder.AddToolBarButton(FStaticMeshEditorCommands::Get().ResetCamera);
			}
			ToolbarBuilder.EndSection();
	
			ToolbarBuilder.BeginSection("LOD");
			{
				ToolbarBuilder.AddWidget(LODLevelCombo.ToSharedRef());
			}
			ToolbarBuilder.EndSection();
		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		Viewport->GetCommandList(),
		FToolBarExtensionDelegate::CreateStatic( &Local::FillToolbar, UVChannelCombo, LODLevelCombo )
		);
	
	AddToolbarExtender(ToolbarExtender);
	
	IStaticMeshEditorModule* StaticMeshEditorModule = &FModuleManager::LoadModuleChecked<IStaticMeshEditorModule>( "StaticMeshEditor" );
	AddToolbarExtender(StaticMeshEditorModule->GetToolBarExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
}

void FStaticMeshEditor::BuildSubTools()
{
	FSimpleDelegate OnSocketSelectionChanged = FSimpleDelegate::CreateSP( SharedThis(this), &FStaticMeshEditor::OnSocketSelectionChanged );

	SocketManager = ISocketManager::CreateSocketManager( SharedThis(this) , OnSocketSelectionChanged );

	SAssignNew( ConvexDecomposition, SConvexDecomposition )
		.StaticMeshEditorPtr(SharedThis(this));

	SAssignNew(GenerateUniqueUVs, SGenerateUniqueUVs)
		.StaticMeshEditorPtr(SharedThis(this));

	// Build toolbar widgets
	UVChannelCombo = SNew(STextComboBox)
		.OptionsSource(&UVChannels)
		.OnSelectionChanged(this, &FStaticMeshEditor::ComboBoxSelectionChanged)
		.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() );

	if(UVChannels.IsValidIndex(0))
	{
		UVChannelCombo->SetSelectedItem(UVChannels[0]);
	}

	LODLevelCombo = SNew(STextComboBox)
		.OptionsSource(&LODLevels)
		.OnSelectionChanged(this, &FStaticMeshEditor::LODLevelsSelectionChanged)
		.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() );

	if(LODLevels.IsValidIndex(0))
	{
		LODLevelCombo->SetSelectedItem(LODLevels[0]);
	}
}

FName FStaticMeshEditor::GetToolkitFName() const
{
	return FName("StaticMeshEditor");
}

FText FStaticMeshEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "StaticMesh Editor");
}

FString FStaticMeshEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "StaticMesh ").ToString();
}

FLinearColor FStaticMeshEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor( 0.3f, 0.2f, 0.5f, 0.5f );
}

UStaticMeshComponent* FStaticMeshEditor::GetStaticMeshComponent() const
{
	return Viewport->GetStaticMeshComponent();
}

void FStaticMeshEditor::SetSelectedSocket(UStaticMeshSocket* InSelectedSocket)
{
	SocketManager->SetSelectedSocket(InSelectedSocket);
}

UStaticMeshSocket* FStaticMeshEditor::GetSelectedSocket() const
{
	check(SocketManager.IsValid());

	return SocketManager->GetSelectedSocket();
}

void FStaticMeshEditor::DuplicateSelectedSocket()
{
	SocketManager->DuplicateSelectedSocket();
}

void FStaticMeshEditor::RequestRenameSelectedSocket()
{
	SocketManager->RequestRenameSelectedSocket();
}

void FStaticMeshEditor::RefreshTool()
{
	int32 NumLODs = StaticMesh->GetNumLODs();
	for (int32 LODIndex = 0; LODIndex < NumLODs; ++LODIndex)
	{
		UpdateLODStats(LODIndex);
	}

	bool bForceRefresh = true;
	StaticMeshDetailsView->SetObject( StaticMesh, bForceRefresh );

	RegenerateLODComboList();
	RegenerateUVChannelComboList();
	RefreshViewport();
}

void FStaticMeshEditor::RefreshViewport()
{
	Viewport->RefreshViewport();
}

void FStaticMeshEditor::RegenerateLODComboList()
{
	if( StaticMesh->RenderData )
	{
		int32 OldLOD = GetCurrentLODLevel();

		NumLODLevels = StaticMesh->RenderData->LODResources.Num();

		// Fill out the LOD level combo.
		LODLevels.Empty();
		LODLevels.Add( MakeShareable( new FString( LOCTEXT("AutoLOD", "Auto LOD").ToString() ) ) );
		LODLevels.Add( MakeShareable( new FString( LOCTEXT("BaseLOD", "Base LOD").ToString() ) ) );
		for(int32 LODLevelID = 1; LODLevelID < NumLODLevels; ++LODLevelID)
		{
			LODLevels.Add( MakeShareable( new FString( FString::Printf(*LOCTEXT("LODLevel_ID", "LOD Level %d").ToString(), LODLevelID ) ) ) );
		}

		if( LODLevelCombo.IsValid() )
		{
			LODLevelCombo->RefreshOptions();

			if( OldLOD < LODLevels.Num() )
			{
				LODLevelCombo->SetSelectedItem(LODLevels[OldLOD]);
			}
			else
			{
				LODLevelCombo->SetSelectedItem(LODLevels[0]);
			}

		}
	}
	else
	{
		NumLODLevels = 0;
		LODLevels.Empty();
		LODLevels.Add( MakeShareable( new FString( LOCTEXT("AutoLOD", "Auto LOD").ToString() ) ) );
	}
}

void FStaticMeshEditor::RegenerateUVChannelComboList()
{
	int32 OldUVChannel = GetCurrentUVChannel();

	// Fill out the UV channels combo.
	UVChannels.Empty();
	int32 MaxUVChannels = FMath::Max<int32>(GetNumUVChannels(),1);
	for(int32 UVChannelID = 0; UVChannelID < MaxUVChannels; ++UVChannelID)
	{
		UVChannels.Add( MakeShareable( new FString( FString::Printf(*LOCTEXT("UVChannel_ID", "UV Channel %d").ToString(), UVChannelID ) ) ) );
	}

	if(UVChannelCombo.IsValid())
	{
		UVChannelCombo->RefreshOptions();
		if( OldUVChannel >= 0 && OldUVChannel < GetNumUVChannels() )
		{
			UVChannelCombo->SetSelectedItem(UVChannels[OldUVChannel]);
		}
		else
		{
			UVChannelCombo->SetSelectedItem(UVChannels[0]);
		}
	}
}


void FStaticMeshEditor::UpdateLODStats(int32 CurrentLOD) 
{
	NumTriangles[CurrentLOD] = 0;
	NumVertices[CurrentLOD] = 0;
	NumUVChannels[CurrentLOD] = 0;
	NumLODLevels = 0;

	if( StaticMesh->RenderData )
	{
		NumLODLevels = StaticMesh->RenderData->LODResources.Num();
		if (CurrentLOD >= 0 && CurrentLOD < NumLODLevels)
		{
			FStaticMeshLODResources& LODModel = StaticMesh->RenderData->LODResources[CurrentLOD];
			NumTriangles[CurrentLOD] = LODModel.GetNumTriangles();
			NumVertices[CurrentLOD] = LODModel.GetNumVertices();
			NumUVChannels[CurrentLOD] = LODModel.VertexBuffer.GetNumTexCoords();
		}
	}
}

void FStaticMeshEditor::ComboBoxSelectionChanged( TSharedPtr<FString> NewSelection, ESelectInfo::Type /*SelectInfo*/ )
{
	Viewport->RefreshViewport();
}

void FStaticMeshEditor::LODLevelsSelectionChanged( TSharedPtr<FString> NewSelection, ESelectInfo::Type /*SelectInfo*/ )
{
	int32 CurrentLOD = GetCurrentLODLevel();

	UpdateLODStats( CurrentLOD > 0? CurrentLOD - 1 : 0 );

	Viewport->ForceLODLevel(CurrentLOD);
}

int32 FStaticMeshEditor::GetCurrentUVChannel()
{
	int32 Index = 0;
	UVChannels.Find(UVChannelCombo->GetSelectedItem(), Index);

	return Index;
}

int32 FStaticMeshEditor::GetCurrentLODLevel()
{
	int32 Index = 0;
	LODLevels.Find(LODLevelCombo->GetSelectedItem(), Index);

	return Index;
}

int32 FStaticMeshEditor::GetCurrentLODIndex()
{
	int32 Index = LODLevels.Find(LODLevelCombo->GetSelectedItem());

	return Index == 0? 0 : Index - 1;
}

void FStaticMeshEditor::GenerateKDop(const FVector* Directions, uint32 NumDirections)
{
	TArray<FVector>	DirArray;

	for(uint32 DirectionIndex = 0;DirectionIndex < NumDirections;DirectionIndex++)
	{
		DirArray.Add(Directions[DirectionIndex]);
	}
	GenerateKDopAsSimpleCollision( StaticMesh, DirArray );

	Viewport->RefreshViewport();
}

void FStaticMeshEditor::OnCollisionSphere()
{
	GenerateSphereAsSimpleCollision( StaticMesh );

	Viewport->RefreshViewport();
}

void FStaticMeshEditor::OnRemoveCollision(void)
{
	// If we have a collision model for this staticmesh, ask if we want to replace it.
	UBodySetup* BS = StaticMesh->BodySetup;
	if (BS != NULL && (BS->AggGeom.GetElementCount() > 0))
	{
		int32 ShouldReplace = FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("RemoveCollisionPrompt", "Are you sure you want to remove the collision mesh?"));
		if (ShouldReplace == EAppReturnType::Yes)
		{
			// Make sure rendering is done - so we are not changing data being used by collision drawing.
			FlushRenderingCommands();

			StaticMesh->BodySetup->RemoveSimpleCollision();

			// refresh collision change back to staticmesh components
			RefreshCollisionChange(StaticMesh);

			// Mark staticmesh as dirty, to help make sure it gets saved.
			StaticMesh->MarkPackageDirty();

			// Update views/property windows
			Viewport->RefreshViewport();
		}
	}
}

/** Util for adding vertex to an array if it is not already present. */
static void AddVertexIfNotPresent(TArray<FVector>& Vertices, const FVector& NewVertex)
{
	bool bIsPresent = false;

	for(int32 i=0; i<Vertices.Num(); i++)
	{
		float diffSqr = (NewVertex - Vertices[i]).SizeSquared();
		if(diffSqr < 0.01f * 0.01f)
		{
			bIsPresent = 1;
			break;
		}
	}

	if(!bIsPresent)
	{
		Vertices.Add(NewVertex);
	}
}

void FStaticMeshEditor::CreateBoxVertsFromBoxCollision(const FKBoxElem& BoxElem, TArray<FVector>& Verts, float Scale)
{
	FVector	B[2], P, Q, Radii;

	// X,Y,Z member variables are LENGTH not RADIUS
	Radii.X = Scale*0.5f*BoxElem.X;
	Radii.Y = Scale*0.5f*BoxElem.Y;
	Radii.Z = Scale*0.5f*BoxElem.Z;

	B[0] = Radii; // max
	B[1] = -1.0f * Radii; // min

	FTransform BoxElemTM = BoxElem.GetTransform();

	for( int32 i=0; i<2; i++ )
	{
		for( int32 j=0; j<2; j++ )
		{
			P.X=B[i].X; Q.X=B[i].X;
			P.Y=B[j].Y; Q.Y=B[j].Y;
			P.Z=B[0].Z; Q.Z=B[1].Z;
			AddVertexIfNotPresent(Verts, BoxElemTM.TransformPosition(P));
			AddVertexIfNotPresent(Verts, BoxElemTM.TransformPosition(Q));

			P.Y=B[i].Y; Q.Y=B[i].Y;
			P.Z=B[j].Z; Q.Z=B[j].Z;
			P.X=B[0].X; Q.X=B[1].X;
			AddVertexIfNotPresent(Verts, BoxElemTM.TransformPosition(P));
			AddVertexIfNotPresent(Verts, BoxElemTM.TransformPosition(Q));

			P.Z=B[i].Z; Q.Z=B[i].Z;
			P.X=B[j].X; Q.X=B[j].X;
			P.Y=B[0].Y; Q.Y=B[1].Y;
			AddVertexIfNotPresent(Verts, BoxElemTM.TransformPosition(P));
			AddVertexIfNotPresent(Verts, BoxElemTM.TransformPosition(Q));
		}
	}
}

void FStaticMeshEditor::OnConvertBoxToConvexCollision()
{
	// If we have a collision model for this staticmesh, ask if we want to replace it.
	if (StaticMesh->BodySetup != NULL)
	{
		int32 ShouldReplace = FMessageDialog::Open( EAppMsgType::YesNo, LOCTEXT("ConvertBoxCollisionPrompt", "Are you sure you want to convert all box collision?") );
		if (ShouldReplace == EAppReturnType::Yes)
		{
			UBodySetup* BodySetup = StaticMesh->BodySetup;

			int32 NumBoxElems = BodySetup->AggGeom.BoxElems.Num();
			if (NumBoxElems > 0)
			{
				// Make sure rendering is done - so we are not changing data being used by collision drawing.
				FlushRenderingCommands();

				FKConvexElem* NewConvexColl = NULL;

				//For each box elem, calculate the new convex collision representation
				//Stored in a temp array so we can undo on failure.
				TArray<FKConvexElem> TempArray;

				for (int32 i=0; i<NumBoxElems; i++)
				{
					const FKBoxElem& BoxColl = BodySetup->AggGeom.BoxElems[i];

					//Create a new convex collision element
					NewConvexColl = new(TempArray) FKConvexElem();
					NewConvexColl->Reset();

					//Fill the convex verts from the box elem collision and generate the convex hull
					CreateBoxVertsFromBoxCollision(BoxColl, NewConvexColl->VertexData, 1.0f);
					NewConvexColl->UpdateElemBox();
				}

				//Clear the cache (PIE may have created some data), create new GUID
				BodySetup->InvalidatePhysicsData();

				//Copy the new data into the static mesh
				BodySetup->AggGeom.ConvexElems.Append(TempArray);

				//Clear out what we just replaced
				BodySetup->AggGeom.BoxElems.Empty();

				BodySetup->CreatePhysicsMeshes();

				// Mark static mesh as dirty, to help make sure it gets saved.
				StaticMesh->MarkPackageDirty();

				// Update views/property windows
				Viewport->RefreshViewport();
			}
		}
	}
}

void FStaticMeshEditor::OnCopyCollisionFromSelectedStaticMesh()
{
	// Find currently selected mesh from content browser
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssetData;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssetData);
	UStaticMesh * SelectedMesh = NULL;
	if ( SelectedAssetData.Num() > 0 )
	{
		// find the first staticmesh we can find
		for ( auto AssetIt = SelectedAssetData.CreateConstIterator(); AssetIt; ++AssetIt )
		{
			const FAssetData & Asset = (*AssetIt);
			// if staticmesh
			if ( Asset.GetClass() == UStaticMesh::StaticClass() )
			{
				SelectedMesh = Cast<UStaticMesh>(Asset.GetAsset());
			}
		}
	}

	// If we have a collision model for this staticmesh, ask if we want to replace it.
	if (SelectedMesh && SelectedMesh != StaticMesh && 
		SelectedMesh->BodySetup != NULL)
	{
		int32 ShouldReplace = FMessageDialog::Open( EAppMsgType::YesNo, FText::Format( LOCTEXT("CopyCollisionFromMeshPrompt", "Are you sure you want to copy collision from {0}?"), FText::FromString( SelectedMesh->GetName() ) ) );
		if (ShouldReplace == EAppReturnType::Yes)
		{
			UBodySetup* BodySetup = StaticMesh->BodySetup;

			// Make sure rendering is done - so we are not changing data being used by collision drawing.
			FlushRenderingCommands();

			// copy body properties from
			BodySetup->CopyBodyPropertiesFrom(SelectedMesh->BodySetup);

			//Invalidate physics data
			BodySetup->InvalidatePhysicsData();

			// Mark static mesh as dirty, to help make sure it gets saved.
			StaticMesh->MarkPackageDirty();

			// Redraw level editor viewports, in case the asset's collision is visible in a viewport and the viewport isn't set to realtime.
			// Note: This could be more intelligent and only trigger a redraw if the asset is referenced in the world.
			GUnrealEd->RedrawLevelEditingViewports();

			// Update views/property windows
			Viewport->RefreshViewport();
		}
	}
	else
	{
		FNotificationInfo Info( LOCTEXT("InvalidMeshSelectionCollisionCopyFromPrompt", "Invalid asset to copy collision from. Select valid StaticMesh in Content Browser.") );
		Info.ExpireDuration = 2.0f;
		FSlateNotificationManager::Get().AddNotification( Info );
	}
}

void FStaticMeshEditor::SetEditorMesh(UStaticMesh* InStaticMesh)
{
	StaticMesh = InStaticMesh;

	//Init stat arrays. A static mesh can have up to three level of details beyond the base mesh.
	const int32 ArraySize = 4;
	NumVertices.Empty(ArraySize);
	NumVertices.AddZeroed(ArraySize);
	NumTriangles.Empty(ArraySize);
	NumTriangles.AddZeroed(ArraySize);
	NumUVChannels.Empty(ArraySize);
	NumUVChannels.AddZeroed(ArraySize);

	// Always default the LOD to 0 when setting the mesh.
	UpdateLODStats(0);

	// Fill out the LOD level combo.
	LODLevels.Empty();
	LODLevels.Add( MakeShareable( new FString( LOCTEXT("AutoLOD", "Auto LOD").ToString() ) ) );
	LODLevels.Add( MakeShareable( new FString( LOCTEXT("BaseLOD", "Base LOD").ToString() ) ) );
	for(int32 LODLevelID = 1; LODLevelID < NumLODLevels; ++LODLevelID)
	{
		LODLevels.Add( MakeShareable( new FString( FString::Printf(*LOCTEXT("LODLevel_ID", "LOD Level %d").ToString(), LODLevelID ) ) ) );

		//Update LOD stats for each level
		UpdateLODStats(LODLevelID);
	}

	// Fill out the UV channels combo.
	UVChannels.Empty();
	for(int32 UVChannelID = 0; UVChannelID < GetNumUVChannels(0); ++UVChannelID)
	{
		UVChannels.Add( MakeShareable( new FString( FString::Printf(*LOCTEXT("UVChannel_ID", "UV Channel %d").ToString(), UVChannelID ) ) ) );
	}

	if( UVChannelCombo.IsValid() )
	{
		UVChannelCombo->RefreshOptions();

		if(UVChannels.Num())
		{
			UVChannelCombo->SetSelectedItem(UVChannels[0]);
		}
	}

	if( LODLevelCombo.IsValid() )
	{
		LODLevelCombo->RefreshOptions();

		if(LODLevels.Num())
		{
			LODLevelCombo->SetSelectedItem(LODLevels[0]);
		}
	}

	// Set the details view.
	StaticMeshDetailsView->SetObject(StaticMesh);

	Viewport->UpdatePreviewMesh(StaticMesh);
	Viewport->RefreshViewport();
}

void FStaticMeshEditor::OnChangeMesh()
{
	FEditorDelegates::LoadSelectedAssetsIfNeeded.Broadcast();
	UStaticMesh* SelectedMesh = GEditor->GetSelectedObjects()->GetTop<UStaticMesh>();
	if(SelectedMesh && SelectedMesh != StaticMesh)
	{
		RemoveEditingObject(StaticMesh);
		AddEditingObject(SelectedMesh);

		SetEditorMesh(SelectedMesh);

		// Refresh the tool so it will update it's LOD list.
		if(GenerateUniqueUVs.IsValid())
		{
			GenerateUniqueUVs->RefreshTool();
		}

		if(SocketManager.IsValid())
		{
			SocketManager->UpdateStaticMesh();
		}
	}
}

void FStaticMeshEditor::DoDecomp(int32 InMaxHullCount, int32 InMaxHullVerts)
{
	// Check we have a selected StaticMesh
	if(StaticMesh && StaticMesh->RenderData)
	{
		FStaticMeshLODResources& LODModel = StaticMesh->RenderData->LODResources[0];

		// Start a busy cursor so the user has feedback while waiting
		const FScopedBusyCursor BusyCursor;

		// Make vertex buffer
		int32 NumVerts = LODModel.VertexBuffer.GetNumVertices();
		TArray<FVector> Verts;
		for(int32 i=0; i<NumVerts; i++)
		{
			FVector Vert = LODModel.PositionVertexBuffer.VertexPosition(i);
			Verts.Add(Vert);
		}

		// Make index buffer
		TArray<uint32> Indices;
		LODModel.IndexBuffer.GetCopy(Indices);

		// Make sure rendering is done - so we are not changing data being used by collision drawing.
		FlushRenderingCommands();

		// Get the BodySetup we are going to put the collision into
		UBodySetup* bs = StaticMesh->BodySetup;
		if(bs)
		{
			bs->RemoveSimpleCollision();
		}
		else
		{
			// Otherwise, create one here.
			StaticMesh->CreateBodySetup();
			bs = StaticMesh->BodySetup;
		}

		// Run actual util to do the work
		DecomposeMeshToHulls(bs, Verts, Indices, InMaxHullCount, InMaxHullVerts);		

		// refresh collision change back to staticmesh components
		RefreshCollisionChange(StaticMesh);

		// Mark mesh as dirty
		StaticMesh->MarkPackageDirty();

		// Update screen.
		Viewport->RefreshViewport();
	}
}

void FStaticMeshEditor::OnExportLightmapMesh( bool IsFBX )
{
	check(StaticMesh);		//must have a mesh to perform this operation

	TArray <UStaticMesh*> StaticMeshArray;
	StaticMeshArray.Add(StaticMesh);

	FString Directory;
	if (PromptUserForDirectory(Directory, *LOCTEXT("StaticMeshEditor_ExportToPromptTitle", "Export to...").ToString(), *FEditorDirectories::Get().GetLastDirectory(ELastDirectory::MESH_IMPORT_EXPORT)))
	{
		FEditorDirectories::Get().SetLastDirectory(ELastDirectory::MESH_IMPORT_EXPORT, Directory); // Save path as default for next time.

		bool bAnyErrorOccurred = FbxMeshUtils::ExportAllLightmapModels(StaticMeshArray, Directory, IsFBX);
		if (bAnyErrorOccurred)
		{
			FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("StaticMeshEditor_LightmapExportFailure", "Some static meshes failed to export or provided warnings.  Please check the Log Window for details."));
		}
	}
}

void FStaticMeshEditor::OnImportLightmapMesh( bool IsFBX )
{
	check(StaticMesh);		//must have a mesh to perform this operation

	TArray <UStaticMesh*> StaticMeshArray;
	StaticMeshArray.Add(StaticMesh);

	FString Directory;
	if (PromptUserForDirectory(Directory, *LOCTEXT("ImportFrom", "Import from...").ToString(), *FEditorDirectories::Get().GetLastDirectory(ELastDirectory::MESH_IMPORT_EXPORT)))
	{
		FEditorDirectories::Get().SetLastDirectory(ELastDirectory::MESH_IMPORT_EXPORT, Directory); // Save path as default for next time.

		bool bAnyErrorOccurred = FbxMeshUtils::ImportAllLightmapModels(StaticMeshArray, Directory, IsFBX);
		if (bAnyErrorOccurred)
		{
			FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("StaticMeshEditor_LightmapImportFailure", "Some static meshes failed to import or provided warnings.  Please check the Log Window for details."));
		}
		Viewport->RefreshViewport();
	}
}

TSet< int32 >& FStaticMeshEditor::GetSelectedEdges()
{
	return Viewport->GetSelectedEdges();
}

int32 FStaticMeshEditor::GetNumTriangles( int32 LODLevel ) const
{
	return NumTriangles.IsValidIndex(LODLevel) ? NumTriangles[LODLevel] : 0;
}

int32 FStaticMeshEditor::GetNumVertices( int32 LODLevel ) const
{
	return NumVertices.IsValidIndex(LODLevel) ? NumVertices[LODLevel] : 0;
}

int32 FStaticMeshEditor::GetNumUVChannels( int32 LODLevel ) const
{
	return NumUVChannels.IsValidIndex(LODLevel) ? NumUVChannels[LODLevel] : 0;
}

void FStaticMeshEditor::DeleteSelectedSockets()
{
	check(SocketManager.IsValid());

	SocketManager->DeleteSelectedSocket();
}

bool FStaticMeshEditor::HasSelectedSockets() const
{
	return ( GetSelectedSocket() != NULL );
}

void FStaticMeshEditor::ExecuteFindInExplorer()
{
	if ( ensure(StaticMesh->AssetImportData) )
	{
		const FString SourceFilePath = FReimportManager::ResolveImportFilename(StaticMesh->AssetImportData->SourceFilePath, StaticMesh);
		if ( SourceFilePath.Len() && IFileManager::Get().FileSize( *SourceFilePath ) != INDEX_NONE )
		{
			FPlatformProcess::ExploreFolder( *FPaths::GetPath(SourceFilePath) );
		}
	}
}

bool FStaticMeshEditor::CanExecuteSourceCommands() const
{
	if ( !StaticMesh->AssetImportData )
	{
		return false;
	}

	const FString& SourceFilePath = FReimportManager::ResolveImportFilename(StaticMesh->AssetImportData->SourceFilePath, StaticMesh);

	return SourceFilePath.Len() && IFileManager::Get().FileSize(*SourceFilePath) != INDEX_NONE;
}

void FStaticMeshEditor::OnObjectReimported(UObject* InObject)
{
	// Make sure we are using the object that is being reimported, otherwise a lot of needless work could occur.
	if(StaticMesh == InObject)
	{
		SetEditorMesh(Cast<UStaticMesh>(InObject));
	}
}

void FStaticMeshEditor::OnConvexDecomposition()
{
	TabManager->InvokeTab(CollisionTabId);
}

bool FStaticMeshEditor::OnRequestClose()
{
	bool bAllowClose = true;
	if (StaticMeshDetails.IsValid() && StaticMeshDetails.Pin()->IsApplyNeeded())
	{
		// find out the user wants to do with this dirty material
		EAppReturnType::Type YesNoCancelReply = FMessageDialog::Open(
			EAppMsgType::YesNoCancel,
			FText::Format( LOCTEXT("ShouldApplyLODChanges", "Would you like to apply level of detail changes to {0}?\n\n(No will lose all changes!)"), FText::FromString( StaticMesh->GetName() ) )
		);

		switch (YesNoCancelReply)
		{
		case EAppReturnType::Yes:
			StaticMeshDetails.Pin()->ApplyChanges();
			bAllowClose = true;
			break;
		case EAppReturnType::No:
			// Do nothing, changes will be abandoned.
			bAllowClose = true;
			break;
		case EAppReturnType::Cancel:
			// Don't exit.
			bAllowClose = false;
			break;
		}
	}

	return bAllowClose;
}

void FStaticMeshEditor::RegisterOnPostUndo( const FOnPostUndo& Delegate )
{
	OnPostUndo.Add( Delegate );
}

void FStaticMeshEditor::UnregisterOnPostUndo( SWidget* Widget )
{
	OnPostUndo.RemoveAll( Widget );
}

void FStaticMeshEditor::NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged )
{
	if(StaticMesh && StaticMesh->BodySetup)
	{
		StaticMesh->BodySetup->CreatePhysicsMeshes();
	}
}

void FStaticMeshEditor::UndoAction()
{
	GEditor->UndoTransaction();
}

void FStaticMeshEditor::RedoAction()
{
	GEditor->RedoTransaction();
}

void FStaticMeshEditor::PostUndo( bool bSuccess )
{
	OnPostUndo.Broadcast();
}

void FStaticMeshEditor::PostRedo( bool bSuccess )
{
	OnPostUndo.Broadcast();
}

void FStaticMeshEditor::OnSocketSelectionChanged()
{
	UStaticMeshSocket* SelectedSocket = GetSelectedSocket();
	Viewport->GetViewportClient().OnSocketSelectionChanged( SelectedSocket );
}

void FStaticMeshEditor::OnPostReimport(UObject* InObject, bool bSuccess)
{
	// Ignore if this is regarding a different object
	if ( InObject != StaticMesh )
	{
		return;
	}

	RefreshTool();
}

#undef LOCTEXT_NAMESPACE
