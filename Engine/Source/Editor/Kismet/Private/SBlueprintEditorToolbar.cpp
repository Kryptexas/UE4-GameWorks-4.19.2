// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintEditorPrivatePCH.h"
#include "BlueprintEditorCommands.h"
#include "BlueprintEditor.h"
#include "SBlueprintEditorToolbar.h"
#include "Editor/UnrealEd/Public/Kismet2/DebuggerCommands.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "SSCSEditor.h"
#include "SSCSEditorViewport.h"
#include "GraphEditorActions.h"
#include "ISourceControlModule.h"
#include "ISourceControlRevision.h"
#include "AssetToolsModule.h"
#include "BlueprintEditorModes.h"
#include "WorkflowOrientedApp/SModeWidget.h"
#include "Editor/PropertyEditor/Public/PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"
#include "IDocumentation.h"
#include "SLevelOfDetailBranchNode.h"
#include "STutorialWrapper.h"
#include "SBlueprintEditorSelectedDebugObjectWidget.h"

#define LOCTEXT_NAMESPACE "KismetToolbar"

//////////////////////////////////////////////////////////////////////////
// SBlueprintModeSeparator

class SBlueprintModeSeparator : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SBlueprintModeSeparator) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArg)
	{
		SBorder::Construct(
			SBorder::FArguments()
			.BorderImage(FEditorStyle::GetBrush("BlueprintEditor.PipelineSeparator"))
			.Padding(0.0f)
			);
	}

	// SWidget interface
	virtual FVector2D ComputeDesiredSize() const OVERRIDE
	{
		const float Height = 20.0f;
		const float Thickness = 16.0f;
		return FVector2D(Thickness, Height);
	}
	// End of SWidget interface
};


//////////////////////////////////////////////////////////////////////////
// FKismet2Menu

void FKismet2Menu::FillFileMenuBlueprintSection( FMenuBuilder& MenuBuilder, FBlueprintEditor& Kismet )
{
	MenuBuilder.BeginSection("FileBlueprint", LOCTEXT("BlueprintHeading", "Blueprint"));
	{
		MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().CompileBlueprint );
		MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().RefreshAllNodes );
		MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().ReparentBlueprint );
		MenuBuilder.AddWrapperSubMenu(
			LOCTEXT("Diff", "Diff"),
			LOCTEXT("BlueprintEditorDiffToolTip", "Diff against previous revisions"),
			FOnGetContent::CreateStatic< FBlueprintEditor& >( &FKismet2Menu::MakeDiffMenu, Kismet),
			FSlateIcon());
	}
	MenuBuilder.EndSection();

	// Only show the developer menu on machines with the solution (assuming they can build it)
	if (FModuleManager::Get().IsSolutionFilePresent())
	{
		MenuBuilder.BeginSection("FileDeveloper");
		{
			MenuBuilder.AddSubMenu( 
				LOCTEXT("DeveloperMenu", "Developer"),
				LOCTEXT("DeveloperMenu_ToolTip", "Open the developer menu"),
				FNewMenuDelegate::CreateStatic( &FKismet2Menu::FillDeveloperMenu ),
				"DeveloperMenu");
		}
		MenuBuilder.EndSection();
	}
}

void FKismet2Menu::FillDeveloperMenu( FMenuBuilder& MenuBuilder )
{
	MenuBuilder.BeginSection("FileDeveloperCompilerSettings", LOCTEXT("CompileOptionsHeading", "Compiler Settings"));
	{
		MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().SaveIntermediateBuildProducts );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("FileDeveloperModuleIteration", LOCTEXT("ModuleIterationHeading", "Module Iteration"));
	{
		MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().RecompileGraphEditor );
		MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().RecompileKismetCompiler );
		MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().RecompileBlueprintEditor );
		MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().RecompilePersona );
	}
	MenuBuilder.EndSection();

	if (false)
	{
		MenuBuilder.BeginSection("FileDeveloperFindReferences");
		{
			MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().FindReferencesFromClass );
			MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().FindReferencesFromBlueprint );
			MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().FindReferencesFromBlueprint );
		}
		MenuBuilder.EndSection();
	}
}

void FKismet2Menu::FillEditMenu( FMenuBuilder& MenuBuilder )
{
	MenuBuilder.BeginSection("EditSearch", LOCTEXT("EditMenu_SearchHeading", "Search") );
	{
		MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().FindInBlueprint );
		MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().FindInBlueprints );
		MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().DeleteUnusedVariables );
	}
	MenuBuilder.EndSection();
}

void FKismet2Menu::FillViewMenu( FMenuBuilder& MenuBuilder )
{
	MenuBuilder.BeginSection("ViewPinVisibility", LOCTEXT("ViewMenu_PinVisibilityHeading", "Pin Visibility") );
	{
		MenuBuilder.AddMenuEntry(FGraphEditorCommands::Get().ShowAllPins);
		MenuBuilder.AddMenuEntry(FGraphEditorCommands::Get().HideNoConnectionNoDefaultPins);
		MenuBuilder.AddMenuEntry(FGraphEditorCommands::Get().HideNoConnectionPins);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("ViewZoom", LOCTEXT("ViewMenu_ZoomHeading", "Zoom") );
	{
		MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().ZoomToWindow );
		MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().ZoomToSelection );
	}
	MenuBuilder.EndSection();
}

void FKismet2Menu::FillDebugMenu( FMenuBuilder& MenuBuilder )
{
	MenuBuilder.BeginSection("DebugBreakpoints", LOCTEXT("DebugMenu_BreakpointHeading", "Breakpoints") );
	{
		MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().DisableAllBreakpoints );
		MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().EnableAllBreakpoints );
		MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().ClearAllBreakpoints );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("DebugWatches", LOCTEXT("DebugMenu_WatchHeading", "Watches") );
	{
		MenuBuilder.AddMenuEntry( FBlueprintEditorCommands::Get().ClearAllWatches );
	}
	MenuBuilder.EndSection();
}

void FKismet2Menu::SetupBlueprintEditorMenu( TSharedPtr< FExtender > Extender, FBlueprintEditor& BlueprintEditor)
{
	// Extend the File menu with asset actions
	Extender->AddMenuExtension(
		"FileLoadAndSave",
		EExtensionHook::After,
		BlueprintEditor.GetToolkitCommands(),
		FMenuExtensionDelegate::CreateStatic< FBlueprintEditor& >( &FKismet2Menu::FillFileMenuBlueprintSection, BlueprintEditor ) );

	// Extend the Edit menu
	Extender->AddMenuExtension(
		"EditHistory",
		EExtensionHook::After,
		BlueprintEditor.GetToolkitCommands(),
		FMenuExtensionDelegate::CreateStatic( &FKismet2Menu::FillEditMenu ) );

	// Add additional blueprint editor menus
	{
		struct Local
		{
			static void AddBlueprintEditorMenus( FMenuBarBuilder& MenuBarBuilder )
			{
				// View
				MenuBarBuilder.AddPullDownMenu( 
					LOCTEXT("ViewMenu", "View"),
					LOCTEXT("ViewMenu_ToolTip", "Open the View menu"),
					FNewMenuDelegate::CreateStatic( &FKismet2Menu::FillViewMenu ),
					"View");

				// Debug
				MenuBarBuilder.AddPullDownMenu( 
					LOCTEXT("DebugMenu", "Debug"),
					LOCTEXT("DebugMenu_ToolTip", "Open the debug menu"),
					FNewMenuDelegate::CreateStatic( &FKismet2Menu::FillDebugMenu ),
					"Debug");
			}
		};

		Extender->AddMenuBarExtension(
			"Edit",
			EExtensionHook::After,
			BlueprintEditor.GetToolkitCommands(), 
			FMenuBarExtensionDelegate::CreateStatic( &Local::AddBlueprintEditorMenus ) );
	}
}

namespace EBlueprintDiffMenuState
{
	enum Type
	{
		NotQueried,
		QueryInProgress,
		Queried,
	};
}

/**
 * Helper widget for managing & displaying source control history retrieval status
 */
class SBlueprintDiffMenu : public SCompoundWidget
{
	SLATE_BEGIN_ARGS( SBlueprintDiffMenu ){}

	SLATE_ARGUMENT(UBlueprint*, BlueprintObj)

	SLATE_END_ARGS()

	~SBlueprintDiffMenu()
	{
		// cancel any operation if this widget is destroyed while in progress
		if(State == EBlueprintDiffMenuState::QueryInProgress)
		{
			ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
			if(UpdateStatusOperation.IsValid() && SourceControlProvider.CanCancelOperation(UpdateStatusOperation.ToSharedRef()))
			{
				SourceControlProvider.CancelOperation(UpdateStatusOperation.ToSharedRef());
			}
		}
	}

	void Construct(const FArguments& InArgs)
	{
		State = EBlueprintDiffMenuState::NotQueried;
		BlueprintObj = InArgs._BlueprintObj;
		bIsLevelScriptBlueprint = false;

		ChildSlot	
		[
			SAssignNew(VerticalBox, SVerticalBox)
			+SVerticalBox::Slot()
			[
				SNew(SBorder)
				.Visibility(this, &SBlueprintDiffMenu::GetInProgressVisibility)
				.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
				.Content()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SThrobber)
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(2.0f, 0.0f, 4.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("DiffMenuOperationInProgress", "Updating history..."))
					]
					+SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.Visibility(this, &SBlueprintDiffMenu::GetCancelButtonVisibility)
						.OnClicked(this, &SBlueprintDiffMenu::OnCancelButtonClicked)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.Content()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("DiffMenuCancelButton", "Cancel"))
						]
					]
				]
			]
		];

		if(BlueprintObj.IsValid())
		{
			bIsLevelScriptBlueprint = FBlueprintEditorUtils::IsLevelScriptBlueprint(BlueprintObj.Get());
			Filename = SourceControlHelpers::PackageFilename(bIsLevelScriptBlueprint ? BlueprintObj.Get()->GetOuter()->GetPathName() : BlueprintObj.Get()->GetPathName());

			// make sure the history info is up to date
			UpdateStatusOperation = ISourceControlOperation::Create<FUpdateStatus>();
			UpdateStatusOperation->SetUpdateHistory(true);
			ISourceControlModule::Get().GetProvider().Execute(UpdateStatusOperation.ToSharedRef(), Filename, EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateSP(this, &SBlueprintDiffMenu::HandleSourceControlOperationComplete));

			State = EBlueprintDiffMenuState::QueryInProgress;
		}
	}

	/** Callback for when the source control operation is complete */
	void HandleSourceControlOperationComplete( const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult )
	{
		check(UpdateStatusOperation == InOperation);

		if(InResult == ECommandResult::Succeeded)
		{
			// get the cached state
			ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
			FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(Filename, EStateCacheUsage::Use);

			//Add pop-out menu for each revision
			FMenuBuilder MenuBuilder(true, NULL);

			MenuBuilder.BeginSection("AddDiffRevision", LOCTEXT("Revisions", "Revisions") );

			if (SourceControlState.IsValid() && SourceControlState->GetHistorySize() > 0)
			{
				// Figure out the highest revision # (so we can label it "Depot")
				int32 LatestRevision = 0;
				for(int32 HistoryIndex = 0; HistoryIndex < SourceControlState->GetHistorySize(); HistoryIndex++)
				{
					TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> Revision = SourceControlState->GetHistoryItem(HistoryIndex);
					if(Revision.IsValid() && Revision->GetRevisionNumber() > LatestRevision)
					{
						LatestRevision = Revision->GetRevisionNumber();
					}
				}

				for(int32 HistoryIndex = 0; HistoryIndex < SourceControlState->GetHistorySize(); HistoryIndex++)
				{
					TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> Revision = SourceControlState->GetHistoryItem(HistoryIndex);
					if(Revision.IsValid())
					{
						FText Label = FText::Format( LOCTEXT("RevisionNumber", "Revision {0}"), FText::AsNumber( Revision->GetRevisionNumber(), NULL, FInternationalization::GetInvariantCulture() ) );

						FFormatNamedArguments Args;
						Args.Add( TEXT("CheckInNumber"), FText::AsNumber( Revision->GetCheckInIdentifier(), NULL, FInternationalization::GetInvariantCulture() ) );
						Args.Add( TEXT("UserName"), FText::FromString( Revision->GetUserName() ) );
						Args.Add( TEXT("DateTime"), FText::AsDate( Revision->GetDate().ToDate() ) );
						Args.Add( TEXT("ChanglistDescription"), FText::FromString( Revision->GetDescription() ) );
						const FText ToolTip = FText::Format( LOCTEXT("RevisionToolTip", "CL #{CheckInNumber} {UserName} \n{DateTime} \n{ChanglistDescription}"), Args );
						
						if(LatestRevision == Revision->GetRevisionNumber())
						{
							Label = LOCTEXT("Depo", "Depot");
						}

 						MenuBuilder.AddMenuEntry(TAttribute<FText>(Label), ToolTip, FSlateIcon(), 
 									FUIAction(FExecuteAction::CreateSP(this, &SBlueprintDiffMenu::DiffAgainstRevision, Revision->GetRevisionNumber())));
					}
				}
			}
			else
			{
				// Show 'empty' item in toolbar
				MenuBuilder.AddMenuEntry( LOCTEXT("NoRevisonHistory", "No revisions found"), 
					FText(), FSlateIcon(), FUIAction() );
			}

			MenuBuilder.EndSection();

			VerticalBox->AddSlot()
			[
				MenuBuilder.MakeWidget()
			];
		}

		UpdateStatusOperation.Reset();
		State = EBlueprintDiffMenuState::Queried;
	}

	/** Delegate called to diff a specific revision with the current */
	void DiffAgainstRevision( int32 InOldRevision )
	{
		if(BlueprintObj.IsValid())
		{
			ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();

			// Get the SCC state
			FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(Filename, EStateCacheUsage::Use);
			if(SourceControlState.IsValid())
			{
				for ( int32 HistoryIndex = 0; HistoryIndex < SourceControlState->GetHistorySize(); HistoryIndex++ )
				{
					TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> Revision = SourceControlState->GetHistoryItem(HistoryIndex);
					check(Revision.IsValid());
					if(Revision->GetRevisionNumber() == InOldRevision)
					{
						// Get the revision of this package from source control
						FString PreviousTempPkgName;
						if(Revision->Get(PreviousTempPkgName))
						{
							// Forcibly disable compile on load in case we are loading old blueprints that might try to update/compile
							TGuardValue<bool> DisableCompileOnLoad(GForceDisableBlueprintCompileOnLoad, true);

							// Try and load that package
							UPackage* PreviousTempPkg = LoadPackage(NULL, *PreviousTempPkgName, LOAD_None);

							if(PreviousTempPkg != NULL)
							{
								UObject* PreviousAsset = NULL;

								// If its a levelscript blueprint, find the previous levelscript blueprint in the map
								if (bIsLevelScriptBlueprint)
								{
									TArray<UObject *> ObjectsInOuter;
									GetObjectsWithOuter(PreviousTempPkg, ObjectsInOuter);
						
									// Look for the level script blueprint for this package
									for( int32 Index = 0; Index < ObjectsInOuter.Num(); Index++ )
									{
										UObject* Obj = ObjectsInOuter[Index];
										if (ULevelScriptBlueprint* ObjAsBlueprint = Cast<ULevelScriptBlueprint>(Obj))
										{
											PreviousAsset = ObjAsBlueprint;
											break;
										}
									}
								}
								// otherwise its a normal Blueprint
								else
								{
									FString PreviousAssetName = FPaths::GetBaseFilename(Filename, true);
									PreviousAsset = FindObject<UObject>(PreviousTempPkg, *PreviousAssetName);
								}

								if(PreviousAsset != NULL)
								{
									FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
									FRevisionInfo OldRevision = {Revision->GetRevisionNumber(), Revision->GetCheckInIdentifier(), Revision->GetDate()};
									FRevisionInfo CurrentRevision = {-1, Revision->GetCheckInIdentifier(), Revision->GetDate()};
									AssetToolsModule.Get().DiffAssets(PreviousAsset, BlueprintObj.Get(), OldRevision, CurrentRevision );
								}
							}
							else
							{
								FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("SourceControl.HistoryWindow", "UnableToLoadAssets", "Unable to load assets to diff. Content may no longer be supported?"));
							}
						}
						break;
					}
				}
			}
		}
	}

	/** Delegate used to cancel a source control operation in progress */
	FReply OnCancelButtonClicked() const
	{
		if(UpdateStatusOperation.IsValid())
		{
			ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
			SourceControlProvider.CancelOperation(UpdateStatusOperation.ToSharedRef());
		}

		return FReply::Handled();
	}

	/** Delegate used to determine the visibility of the cancel button */
	EVisibility GetCancelButtonVisibility() const
	{
		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		return UpdateStatusOperation.IsValid() && SourceControlProvider.CanCancelOperation(UpdateStatusOperation.ToSharedRef()) ? EVisibility::Visible : EVisibility::Collapsed;
	}

	/** Delegate used to determine the visibility 'in progress' widgets */
	EVisibility GetInProgressVisibility() const
	{
		return State == EBlueprintDiffMenuState::QueryInProgress ? EVisibility::Visible : EVisibility::Collapsed;
	}

private:

	/** The source control operation in progress */
	TSharedPtr<FUpdateStatus, ESPMode::ThreadSafe> UpdateStatusOperation;

	/** The box we are using to display our menu */
	TSharedPtr<SVerticalBox> VerticalBox;

	/** The state of the SCC query */
	EBlueprintDiffMenuState::Type State;

	/** The Blueprint we are interested in */
	TWeakObjectPtr<UBlueprint> BlueprintObj;

	/** The name of the file we want to diff */
	FString Filename;

	/** We need to handle level script blueprints differently, so we cache this here */
	bool bIsLevelScriptBlueprint;
};

TSharedRef<SWidget> FKismet2Menu::MakeDiffMenu(FBlueprintEditor& Kismet)
{
	if (ISourceControlModule::Get().IsEnabled() && ISourceControlModule::Get().GetProvider().IsAvailable())
	{
		UBlueprint* BlueprintObj = Kismet.GetBlueprintObj();
		if(BlueprintObj)
		{
			// Add our async SCC task widget
			return SNew(SBlueprintDiffMenu).BlueprintObj(BlueprintObj);
		}
		else
		{
			// if BlueprintObj is null then this means that multiple blueprints are selected
			FMenuBuilder MenuBuilder(true, NULL);
			MenuBuilder.AddMenuEntry( LOCTEXT("NoRevisionsForMultipleBlueprints", "Multiple blueprints selected"), 
				FText(), FSlateIcon(), FUIAction() );
			return MenuBuilder.MakeWidget();
		}
	}

	FMenuBuilder MenuBuilder(true, NULL);
	MenuBuilder.AddMenuEntry( LOCTEXT("SourceControlDisabled", "Source control is disabled"), 
		FText(), FSlateIcon(), FUIAction() );
	return MenuBuilder.MakeWidget();
}



//////////////////////////////////////////////////////////////////////////
// FFullBlueprintEditorCommands

void FFullBlueprintEditorCommands::RegisterCommands() 
{
	UI_COMMAND(Compile, "Compile", "Compile the blueprint", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(SwitchToScriptingMode, "Graph", "Switches to Graph Editing Mode", EUserInterfaceActionType::ToggleButton, FInputGesture());
	UI_COMMAND(SwitchToBlueprintDefaultsMode, "Defaults", "Switches to Blueprint Defaults Mode", EUserInterfaceActionType::ToggleButton, FInputGesture());
	UI_COMMAND(SwitchToComponentsMode, "Components", "Switches to Components Mode", EUserInterfaceActionType::ToggleButton, FInputGesture());
	UI_COMMAND(EditGlobalOptions, "Blueprint Props", "Edit Blueprint Options", EUserInterfaceActionType::Button, FInputGesture());
}


//////////////////////////////////////////////////////////////////////////
// FBlueprintEditorToolbar

void FBlueprintEditorToolbar::AddBlueprintEditorModesToolbar(TSharedPtr<FExtender> Extender)
{
	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = BlueprintEditor.Pin();

	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		BlueprintEditorPtr->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP( this, &FBlueprintEditorToolbar::FillBlueprintEditorModesToolbar ) );
}

void FBlueprintEditorToolbar::AddBlueprintGlobalOptionsToolbar(TSharedPtr<FExtender> Extender)
{
	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = BlueprintEditor.Pin();

	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		BlueprintEditorPtr->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP( this, &FBlueprintEditorToolbar::FillBlueprintGlobalOptionsToolbar ) );
}

void FBlueprintEditorToolbar::AddCompileToolbar(TSharedPtr<FExtender> Extender)
{
	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = BlueprintEditor.Pin();

	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::Before,
		BlueprintEditorPtr->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP( this, &FBlueprintEditorToolbar::FillCompileToolbar ) );
}

void FBlueprintEditorToolbar::AddNewToolbar(TSharedPtr<FExtender> Extender)
{
	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = BlueprintEditor.Pin();

	Extender->AddToolBarExtension(
		"MyBlueprint",
		EExtensionHook::After,
		BlueprintEditorPtr->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP( this, &FBlueprintEditorToolbar::FillNewToolbar ) );
}

void FBlueprintEditorToolbar::AddScriptingToolbar(TSharedPtr<FExtender> Extender)
{
	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = BlueprintEditor.Pin();

	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		BlueprintEditorPtr->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP( this, &FBlueprintEditorToolbar::FillScriptingToolbar ) );
}

void FBlueprintEditorToolbar::AddDebuggingToolbar(TSharedPtr<FExtender> Extender)
{
	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = BlueprintEditor.Pin();

	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		BlueprintEditorPtr->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP( this, &FBlueprintEditorToolbar::FillDebuggingToolbar ) );
}

void FBlueprintEditorToolbar::AddComponentsToolbar(TSharedPtr<FExtender> Extender)
{
	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = BlueprintEditor.Pin();

	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		BlueprintEditorPtr->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP( this, &FBlueprintEditorToolbar::FillComponentsToolbar ) );
}


void FBlueprintEditorToolbar::FillBlueprintEditorModesToolbar(FToolBarBuilder& ToolbarBuilder)
{
	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = BlueprintEditor.Pin();
	UBlueprint* BlueprintObj = BlueprintEditorPtr->GetBlueprintObj();

	if( !BlueprintObj ||
		(!FBlueprintEditorUtils::IsLevelScriptBlueprint(BlueprintObj) 
		&& !FBlueprintEditorUtils::IsInterfaceBlueprint(BlueprintObj)
		&& !BlueprintObj->bIsNewlyCreated)
		)
	{
		TAttribute<FName> GetActiveMode(BlueprintEditorPtr.ToSharedRef(), &FBlueprintEditor::GetCurrentMode);
		FOnModeChangeRequested SetActiveMode = FOnModeChangeRequested::CreateSP(BlueprintEditorPtr.ToSharedRef(), &FBlueprintEditor::SetCurrentMode);

		// Left side padding
		BlueprintEditorPtr->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(4.0f, 1.0f)));

		BlueprintEditorPtr->AddToolbarWidget(
			SNew(STutorialWrapper)
			.Name(TEXT("DefaultsMode"))
			.Content()
			[
				SNew(SModeWidget, FBlueprintEditorApplicationModes::GetLocalizedMode( FBlueprintEditorApplicationModes::BlueprintDefaultsMode ), FBlueprintEditorApplicationModes::BlueprintDefaultsMode)
				.OnGetActiveMode(GetActiveMode)
				.OnSetActiveMode(SetActiveMode)
				.ToolTip(IDocumentation::Get()->CreateToolTip(
					LOCTEXT("BlueprintDefaultsModeButtonTooltip", "Switch to Blueprint Defaults Mode"),
					NULL,
					TEXT("Shared/Editors/BlueprintEditor"),
					TEXT("DefaultsMode")))
				.IconImage(FEditorStyle::GetBrush("FullBlueprintEditor.SwitchToBlueprintDefaultsMode"))
				.SmallIconImage(FEditorStyle::GetBrush("FullBlueprintEditor.SwitchToBlueprintDefaultsMode.Small"))
			]
		);

		BlueprintEditorPtr->AddToolbarWidget(SNew(SBlueprintModeSeparator));

		BlueprintEditorPtr->AddToolbarWidget(
			SNew(STutorialWrapper)
			.Name(TEXT("ComponentsMode"))
			.Content()
			[
				SNew(SModeWidget, FBlueprintEditorApplicationModes::GetLocalizedMode( FBlueprintEditorApplicationModes::BlueprintComponentsMode ), FBlueprintEditorApplicationModes::BlueprintComponentsMode)
				.OnGetActiveMode(GetActiveMode)
				.OnSetActiveMode(SetActiveMode)
				.CanBeSelected(BlueprintEditorPtr.Get(), &FBlueprintEditor::CanAccessComponentsMode)
				.ToolTip(IDocumentation::Get()->CreateToolTip(
					LOCTEXT("ComponentsModeButtonTooltip", "Switch to Components Mode"),
					NULL,
					TEXT("Shared/Editors/BlueprintEditor"),
					TEXT("ComponentsMode")))
				.IconImage(FEditorStyle::GetBrush("FullBlueprintEditor.SwitchToComponentsMode"))
				.SmallIconImage(FEditorStyle::GetBrush("FullBlueprintEditor.SwitchToComponentsMode.Small"))
			]
		);

		BlueprintEditorPtr->AddToolbarWidget(SNew(SBlueprintModeSeparator));

		BlueprintEditorPtr->AddToolbarWidget(
			SNew(STutorialWrapper)
			.Name(TEXT("GraphMode"))
			.Content()
			[
				SNew(SModeWidget, FBlueprintEditorApplicationModes::GetLocalizedMode( FBlueprintEditorApplicationModes::StandardBlueprintEditorMode ), FBlueprintEditorApplicationModes::StandardBlueprintEditorMode)
				.OnGetActiveMode(GetActiveMode)
				.OnSetActiveMode(SetActiveMode)
				.CanBeSelected(BlueprintEditorPtr.Get(), &FBlueprintEditor::IsEditingSingleBlueprint)
				.ToolTip(IDocumentation::Get()->CreateToolTip(
					LOCTEXT("GraphModeButtonTooltip", "Switch to Graph Editing Mode"),
					NULL,
					TEXT("Shared/Editors/BlueprintEditor"),
					TEXT("GraphMode")))
				.ToolTipText(LOCTEXT("GraphModeButtonTooltip", "Switch to Graph Editing Mode"))
				.IconImage(FEditorStyle::GetBrush("FullBlueprintEditor.SwitchToScriptingMode"))
				.SmallIconImage(FEditorStyle::GetBrush("FullBlueprintEditor.SwitchToScriptingMode.Small"))
			]
		);
		
		// Right side padding
		BlueprintEditorPtr->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(4.0f, 1.0f)));
	}
}

void FBlueprintEditorToolbar::FillBlueprintGlobalOptionsToolbar(FToolBarBuilder& ToolbarBuilder)
{
	const FFullBlueprintEditorCommands& Commands = FFullBlueprintEditorCommands::Get();
	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = BlueprintEditor.Pin();
	UBlueprint* BlueprintObj = BlueprintEditorPtr->GetBlueprintObj();
	
	ToolbarBuilder.BeginSection("Settings");
	
	if(BlueprintObj != NULL)
	{
		ToolbarBuilder.AddToolBarButton(Commands.EditGlobalOptions);
	}
	
	ToolbarBuilder.EndSection();
}

void FBlueprintEditorToolbar::FillCompileToolbar(FToolBarBuilder& ToolbarBuilder)
{
	const FFullBlueprintEditorCommands& Commands = FFullBlueprintEditorCommands::Get();
	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = BlueprintEditor.Pin();
	UBlueprint* BlueprintObj = BlueprintEditorPtr->GetBlueprintObj();

	ToolbarBuilder.BeginSection("Compile");
	if (BlueprintObj != NULL)
	{
		ToolbarBuilder.AddToolBarButton( Commands.Compile,
									 NAME_None, 
									 TAttribute<FText>(),
									 TAttribute<FText>(this, &FBlueprintEditorToolbar::GetStatusTooltip),
									 TAttribute<FSlateIcon>(this, &FBlueprintEditorToolbar::GetStatusImage),
									 FName(TEXT("CompileBlueprint")));
	}
	ToolbarBuilder.EndSection();
}

void FBlueprintEditorToolbar::FillNewToolbar(FToolBarBuilder& ToolbarBuilder)
{
	const FBlueprintEditorCommands& Commands = FBlueprintEditorCommands::Get();

	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = BlueprintEditor.Pin();
	UBlueprint* BlueprintObj = BlueprintEditorPtr->GetBlueprintObj();

	ToolbarBuilder.BeginSection("AddNew");
	if (BlueprintObj != NULL)
	{
		ToolbarBuilder.AddToolBarButton(Commands.AddNewVariable, NAME_None, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), FName(TEXT("BPEAddNewVariable")));
		ToolbarBuilder.AddToolBarButton(Commands.AddNewFunction, NAME_None, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), FName(TEXT("BPEAddNewFunction")));
		ToolbarBuilder.AddToolBarButton(Commands.AddNewMacroDeclaration, NAME_None, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), FName(TEXT("BPEAddNewMacro")));
		// Add New Animation Graph isn't supported right now.
		ToolbarBuilder.AddToolBarButton(Commands.AddNewEventGraph, NAME_None, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), FName(TEXT("BPEAddNewEventGraph")));
		ToolbarBuilder.AddToolBarButton(Commands.AddNewDelegate, NAME_None, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), FName(TEXT("BPEAddNewDelegate")));
	}
	ToolbarBuilder.EndSection();
}

void FBlueprintEditorToolbar::FillScriptingToolbar(FToolBarBuilder& ToolbarBuilder)
{
	const FBlueprintEditorCommands& Commands = FBlueprintEditorCommands::Get();

	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = BlueprintEditor.Pin();
	UBlueprint* BlueprintObj = BlueprintEditorPtr->GetBlueprintObj();

	ToolbarBuilder.BeginSection("Script");

	ToolbarBuilder.AddToolBarButton(FBlueprintEditorCommands::Get().FindInBlueprint);

	ToolbarBuilder.EndSection();
}

void FBlueprintEditorToolbar::FillDebuggingToolbar(FToolBarBuilder& ToolbarBuilder)
{
	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = BlueprintEditor.Pin();
	UBlueprint* BlueprintObj = BlueprintEditorPtr->GetBlueprintObj();

	ToolbarBuilder.BeginSection("Debugging");
	if (BlueprintObj)
	{
		FPlayWorldCommands::BuildToolbar(ToolbarBuilder);

		if (BlueprintObj->BlueprintType != BPTYPE_MacroLibrary)
		{
			// Selected debug actor button
			ToolbarBuilder.AddWidget(SNew(SBlueprintEditorSelectedDebugObjectWidget, BlueprintEditorPtr));
		}
	}
	ToolbarBuilder.EndSection();
}

void FBlueprintEditorToolbar::FillComponentsToolbar(FToolBarBuilder& ToolbarBuilder)
{
	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = BlueprintEditor.Pin();
	UBlueprint* BlueprintObj = BlueprintEditorPtr->GetBlueprintObj();

#if 0 // restore this if we ever need the ability to toggle component editing on/off
	ToolbarBuilder.BeginSection("Components");
		ToolbarBuilder.AddToolBarButton(FSCSCommands::Get().ToggleComponentEditing);
	ToolbarBuilder.EndSection();
#endif

	ToolbarBuilder.BeginSection("ComponentsViewport");
		ToolbarBuilder.AddToolBarButton(FBlueprintEditorCommands::Get().EnableSimulation);
	ToolbarBuilder.EndSection();
}

FSlateIcon FBlueprintEditorToolbar::GetStatusImage() const
{
	UBlueprint* BlueprintObj = BlueprintEditor.Pin()->GetBlueprintObj();
	EBlueprintStatus Status = BlueprintObj->Status;

	// For macro types, always show as up-to-date, since we don't compile them
	if ((BlueprintObj->BlueprintType == BPTYPE_MacroLibrary) || FBlueprintEditorUtils::IsDataOnlyBlueprint(BlueprintObj))
	{
		Status = BS_UpToDate;
	}

	switch (Status)
	{
	default:
	case BS_Unknown:
	case BS_Dirty:
		return FSlateIcon(FEditorStyle::GetStyleSetName(), "Kismet.Status.Unknown");
	case BS_Error:
		return FSlateIcon(FEditorStyle::GetStyleSetName(), "Kismet.Status.Error");
	case BS_UpToDate:
		return FSlateIcon(FEditorStyle::GetStyleSetName(), "Kismet.Status.Good");
	case BS_UpToDateWithWarnings:
		return FSlateIcon(FEditorStyle::GetStyleSetName(), "Kismet.Status.Warning");
	}
}

FText FBlueprintEditorToolbar::GetStatusTooltip() const
{
	UBlueprint* BlueprintObj = BlueprintEditor.Pin()->GetBlueprintObj();
	EBlueprintStatus Status = BlueprintObj->Status;

	// For macro types, always show as up-to-date, since we don't compile them
	if ((BlueprintObj->BlueprintType == BPTYPE_MacroLibrary) || FBlueprintEditorUtils::IsDataOnlyBlueprint(BlueprintObj))
	{
		Status = BS_UpToDate;
	}

	switch (Status)
	{
	default:
	case BS_Unknown:
		return LOCTEXT("Recompile_Status", "Unknown status; should recompile");
	case BS_Dirty:
		return LOCTEXT("Dirty_Status", "Dirty; needs to be recompiled");
	case BS_Error:
		return LOCTEXT("CompileError_Status", "There was an error during compilation, see the log for details");
	case BS_UpToDate:
		return LOCTEXT("GoodToGo_Status", "Good to go");
	case BS_UpToDateWithWarnings:
		return LOCTEXT("GoodToGoWarning_Status", "There was a warning during compilation, see the log for details");
	}
}


#undef LOCTEXT_NAMESPACE
