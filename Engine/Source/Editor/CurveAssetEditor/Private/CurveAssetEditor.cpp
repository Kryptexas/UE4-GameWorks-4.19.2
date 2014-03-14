// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CurveAssetEditorPrivatePCH.h"

#include "CurveAssetEditor.h"
#include "SCurveEditor.h"
//#include "Toolkits/IToolkitHost.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "CurveAssetEditor"

const FName FCurveAssetEditor::CurveTabId( TEXT( "CurveAssetEditor_Curve" ) );

void FCurveAssetEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(TabManager);

	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	TabManager->RegisterTabSpawner( CurveTabId, FOnSpawnTab::CreateSP(this, &FCurveAssetEditor::SpawnTab_CurveAsset) )
		.SetDisplayName( LOCTEXT("CurveTab", "Curve") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );
}

void FCurveAssetEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	TabManager->UnregisterTabSpawner( CurveTabId );
}

void FCurveAssetEditor::InitCurveAssetEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UCurveBase* CurveToEdit )
{	
	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout( "Standalone_CurveAssetEditor_Layout" )
	->AddArea
	(
		FTabManager::NewPrimaryArea()
		->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.1f)
			->SetHideTabWell( true )
			->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
		) 
		->Split
		(
			FTabManager::NewStack()
			->SetHideTabWell( true )
			->AddTab( CurveTabId, ETabState::OpenedTab )
		)
	);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor( Mode, InitToolkitHost, FCurveAssetEditorModule::CurveAssetEditorAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, CurveToEdit );
	
	FCurveAssetEditorModule& CurveAssetEditorModule = FModuleManager::LoadModuleChecked<FCurveAssetEditorModule>( "CurveAssetEditor" );
	AddMenuExtender(CurveAssetEditorModule.GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

	// @todo toolkit world centric editing
	/*// Setup our tool's layout
	if( IsWorldCentricAssetEditor() )
	{
		const FString TabInitializationPayload(TEXT(""));		// NOTE: Payload not currently used for table properties
		SpawnToolkitTab( CurveTabId, TabInitializationPayload, EToolkitTabSpot::Details );
	}*/

	// NOTE: Could fill in asset editor commands here!
}

FName FCurveAssetEditor::GetToolkitFName() const
{
	return FName("CurveAssetEditor");
}

FText FCurveAssetEditor::GetBaseToolkitName() const
{
	return LOCTEXT( "AppLabel", "Curve Asset Editor" );
}

FString FCurveAssetEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "CurveAsset ").ToString();
}

FLinearColor FCurveAssetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor( 0.0f, 0.0f, 0.2f, 0.5f );
}

TSharedRef<SDockTab> FCurveAssetEditor::SpawnTab_CurveAsset( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId().TabType == CurveTabId );

	ViewMinInput=0.f;
	ViewMaxInput=5.f;

	TSharedRef<SDockTab> NewDockTab = SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("CurveAssetEditor.Tabs.Properties") )
		.Label( FText::Format(LOCTEXT("CurveAssetEditorTitle", "{0} Curve Asset"), FText::FromString(GetTabPrefix())))
		.TabColorScale( GetTabColorScale() )
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
			.Padding(0.0f)
			[
				SAssignNew(TrackWidget, SCurveEditor)
				.ViewMinInput(this, &FCurveAssetEditor::GetViewMinInput)
				.ViewMaxInput(this, &FCurveAssetEditor::GetViewMaxInput)
				.TimelineLength(this, &FCurveAssetEditor::GetTimelineLength)
				.OnSetInputViewRange(this, &FCurveAssetEditor::SetInputViewRange)
				.HideUI(false)
				.AlwaysDisplayColorCurves(true)
			]
		];

	UCurveBase* Curve = Cast<UCurveBase>(GetEditingObject());
	UCurveFloat* FloatCurve = Cast<UCurveFloat>(Curve);
	UCurveVector* VectorCurve = Cast<UCurveVector>(Curve);
	UCurveLinearColor* LinearColorCurve = Cast<UCurveLinearColor>(Curve);

	FCurveOwnerInterface* CurveOwner = NULL;
	if(FloatCurve != NULL)
	{
		CurveOwner = FloatCurve;
	}
	else if(LinearColorCurve != NULL)
	{
		CurveOwner = LinearColorCurve;
	}
	else
	{
		CurveOwner = VectorCurve;
	}

	if (CurveOwner != NULL)
	{
		check(TrackWidget.IsValid());
		// Set this curve as the SCurveEditor's selected curve
		TrackWidget->SetCurveOwner(CurveOwner);
	}
	
	return NewDockTab;
}

float FCurveAssetEditor::GetTimelineLength() const
{
	return 0.f;
}

void FCurveAssetEditor::SetInputViewRange(float InViewMinInput, float InViewMaxInput)
{
	ViewMaxInput = InViewMaxInput;
	ViewMinInput = InViewMinInput;
}

#undef LOCTEXT_NAMESPACE