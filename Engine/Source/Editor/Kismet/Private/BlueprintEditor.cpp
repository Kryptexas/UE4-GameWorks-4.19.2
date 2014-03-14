// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "ScopedTransaction.h"
#include "Editor/UnrealEd/Public/Kismet2/KismetDebugUtilities.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "EngineLevelScriptClasses.h"
#include "GraphEditor.h"
#include "BlueprintUtilities.h"
#include "Editor/KismetCompiler/Public/KismetCompilerModule.h"
#include "Editor/ClassViewer/Public/ClassViewerModule.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"
#include "Runtime/Engine/Public/EngineUtils.h"
#include "EdGraphUtilities.h"
#include "Toolkits/IToolkitHost.h"
#include "Developer/MessageLog/Public/MessageLogModule.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "Kismet/GameplayStatics.h"
#include "Editor/BlueprintGraph/Public/K2ActionMenuBuilder.h"
#include "BlueprintEditorCommands.h"
#include "BlueprintEditor.h"
#include "BlueprintEditorModule.h"
#include "GraphEditorActions.h"
#include "SNodePanel.h"

#include "SBlueprintEditorToolbar.h"
#include "FindInBlueprints.h"
#include "SGraphTitleBar.h"
#include "Editor/UnrealEd/Public/Kismet2/Kismet2NameValidators.h"
#include "Editor/UnrealEd/Public/Kismet2/DebuggerCommands.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"

#include "BlueprintEditorTabs.h"
#include "StructureEditorUtils.h"

#include "ClassIconFinder.h"

// Core kismet tabs
#include "SSCSEditor.h"
#include "SSCSEditorViewport.h"
#include "STimelineEditor.h"
#include "SKismetInspector.h"
#include "SBlueprintPalette.h"
#include "SBlueprintActionMenu.h"
#include "SMyBlueprint.h"
#include "FindInBlueprints.h"
#include "SUserDefinedStructureEditor.h"
// End of core kismet tabs

// Debugging
#include "Debugging/SKismetDebuggingView.h"
#include "Debugging/KismetDebugCommands.h"
// End of debugging

// Misc diagnostics
#include "ObjectTools.h"
// End of misc diagnostics

#include "AssetRegistryModule.h"
#include "BlueprintEditorTabFactories.h"
#include "SPinTypeSelector.h"
#include "AnimGraphDefinitions.h"
#include "BlueprintEditorModes.h"

#include "EngineAnalytics.h"
#include "IAnalyticsProvider.h"
#include "ISCSEditorCustomization.h"
#include "Editor/UnrealEd/Public/SourceCodeNavigation.h"

#include "Kismet.generated.inl"

#define LOCTEXT_NAMESPACE "BlueprintEditor"

/////////////////////////////////////////////////////
// FSelectionDetailsSummoner

FSelectionDetailsSummoner::FSelectionDetailsSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
	: FWorkflowTabFactory(FBlueprintEditorTabs::DetailsID, InHostingApp)
{
	TabLabel = LOCTEXT("DetailsView_TabTitle", "Details");
	TabIcon = FEditorStyle::GetBrush("LevelEditor.Tabs.Details");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("DetailsView_MenuTitle", "Details");
	ViewMenuTooltip = LOCTEXT("DetailsView_ToolTip", "Shows the details view");
}

TSharedRef<SWidget> FSelectionDetailsSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedPtr<FBlueprintEditor> BlueprintEditorPtr = StaticCastSharedPtr<FBlueprintEditor>(HostingApp.Pin());

	return BlueprintEditorPtr->GetInspector();
}

/////////////////////////////////////////////////////
// SChildGraphPicker

class SChildGraphPicker : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SChildGraphPicker) {}
	SLATE_END_ARGS()
public:
	void Construct(const FArguments& InArgs, UEdGraph* ParentGraph)
	{
		this->ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
			.Padding(5)
			.ToolTipText(LOCTEXT("ChildGraphPickerTooltip", "Pick the graph to enter"))
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ChildGraphPickerDesc", "Navigate to graph"))
					.Font( FEditorStyle::GetFontStyle(TEXT("Kismet.GraphPicker.Title.Font")) )
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SListView<UEdGraph*>)
					.ItemHeight(20)
					.ListItemsSource(&(ParentGraph->SubGraphs))
					.OnGenerateRow( this, &SChildGraphPicker::GenerateMenuItemRow )
				]
			]
		];	
	}
private:
	/** Generate a row for the InItem in the combo box's list (passed in as OwnerTable). Do this by calling the user-specified OnGenerateWidget */
	TSharedRef<ITableRow> GenerateMenuItemRow(UEdGraph* InItem, const TSharedRef<STableViewBase>& OwnerTable)
	{
		check(InItem != NULL);

		const FText DisplayName = FLocalKismetCallbacks::GetGraphDisplayName(InItem);

		return SNew( STableRow<UEdGraph*>, OwnerTable )
			[
				SNew(SHyperlink)
					.Text(DisplayName)
					.Style(FEditorStyle::Get(), "HoverOnlyHyperlink")
					.OnNavigate(this, &SChildGraphPicker::NavigateToGraph, InItem)
			]
		;
	}

	void NavigateToGraph(UEdGraph* ChildGraph)
	{
		FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(ChildGraph);

		FSlateApplication::Get().DismissAllMenus();
	}
};

/////////////////////////////////////////////////////
// FBlueprintEditor

bool FBlueprintEditor::IsASubGraph( const UEdGraph* GraphPtr )
{
	if( GraphPtr && GraphPtr->GetOuter() )
	{
		//Check whether the outer is a composite node
		if( GraphPtr->GetOuter()->IsA( UK2Node_Composite::StaticClass() ) )
		{
			return true;
		}
	}
	return false;
}

/** Util for finding a glyph for a graph */
const FSlateBrush* FBlueprintEditor::GetGlyphForGraph(const UEdGraph* Graph, bool bInLargeIcon)
{
	const FSlateBrush* ReturnValue = FEditorStyle::GetBrush( bInLargeIcon ? TEXT("GraphEditor.Function_24x") : TEXT("GraphEditor.Function_16x") );

	check(Graph != NULL);
	const UEdGraphSchema* Schema = Graph->GetSchema();
	if (Schema != NULL)
	{
		const EGraphType GraphType = Schema->GetGraphType(Graph);
		switch (GraphType)
		{
		default:
		case GT_StateMachine:
			{
				ReturnValue = FEditorStyle::GetBrush( bInLargeIcon ? TEXT("GraphEditor.StateMachine_24x") : TEXT("GraphEditor.StateMachine_16x") );
			}
			break;
		case GT_Function:
			{
				if ( Graph->IsA(UAnimationTransitionGraph::StaticClass()) )
				{
					UObject* GraphOuter = Graph->GetOuter();
					if ( GraphOuter != NULL && GraphOuter->IsA(UAnimStateConduitNode::StaticClass()) )
					{
						ReturnValue = FEditorStyle::GetBrush( bInLargeIcon ? TEXT("GraphEditor.Conduit_24x") : TEXT("GraphEditor.Conduit_16x") );
					}
					else
					{
						ReturnValue = FEditorStyle::GetBrush( bInLargeIcon ? TEXT("GraphEditor.Rule_24x") : TEXT("GraphEditor.Rule_16x") );
					}
				}
				else
				{
					//Check for subgraph
					if( IsASubGraph( Graph ) )
					{
						ReturnValue = FEditorStyle::GetBrush( bInLargeIcon ? TEXT("GraphEditor.SubGraph_24x") : TEXT("GraphEditor.SubGraph_16x") );
					}
					else
					{
						ReturnValue = FEditorStyle::GetBrush( bInLargeIcon ? TEXT("GraphEditor.Function_24x") : TEXT("GraphEditor.Function_16x") );
					}
				}
			}
			break;
		case GT_Macro:
			{
				ReturnValue = FEditorStyle::GetBrush( bInLargeIcon ? TEXT("GraphEditor.Macro_24x") : TEXT("GraphEditor.Macro_16x") );
			}
			break;
		case GT_Ubergraph:
			{
				ReturnValue = FEditorStyle::GetBrush( bInLargeIcon ? TEXT("GraphEditor.EventGraph_24x") : TEXT("GraphEditor.EventGraph_16x") );
			}
			break;
		case GT_Animation:
			{
				if ( Graph->IsA(UAnimationStateGraph::StaticClass()) )
				{
					ReturnValue = FEditorStyle::GetBrush( bInLargeIcon ? TEXT("GraphEditor.State_24x") : TEXT("GraphEditor.State_16x") );
				}
				else
				{
					ReturnValue = FEditorStyle::GetBrush( bInLargeIcon ? TEXT("GraphEditor.Animation_24x") : TEXT("GraphEditor.Animation_16x") );
				}
			}
		}
	}

	return ReturnValue;
}

FSlateBrush const* FBlueprintEditor::GetVarIconAndColor(UClass* VarClass, FName VarName, FSlateColor& IconColorOut)
{
	FLinearColor ColorOut;
	const FSlateBrush* IconBrush = FEditorStyle::GetBrush(UK2Node_Variable::GetVariableIconAndColor(VarClass, VarName, ColorOut));
	IconColorOut = ColorOut;
	return IconBrush;
}

bool FBlueprintEditor::IsInAScriptingMode() const
{
	return IsModeCurrent(FBlueprintEditorApplicationModes::StandardBlueprintEditorMode) || IsModeCurrent(FBlueprintEditorApplicationModes::BlueprintMacroMode);
}

bool FBlueprintEditor::OnRequestClose()
{
	bEditorMarkedAsClosed = true;
	return FWorkflowCentricApplication::OnRequestClose();
}

bool FBlueprintEditor::InEditingMode() const
{
	return !InDebuggingMode();
}

bool FBlueprintEditor::IsCompilingEnabled() const
{
	UBlueprint* Blueprint = GetBlueprintObj();
	return Blueprint && Blueprint->BlueprintType != BPTYPE_MacroLibrary && InEditingMode();
}

bool FBlueprintEditor::IsPropertyEditingEnabled() const
{
	return true;
}

bool FBlueprintEditor::InDebuggingMode() const
{
	return GEditor->PlayWorld != NULL;
}

EVisibility FBlueprintEditor::IsDebuggerVisible() const
{
	return InDebuggingMode() ? EVisibility::Visible : EVisibility::Collapsed;
}

int32 FBlueprintEditor::GetNumberOfSelectedNodes() const
{
	return GetSelectedNodes().Num();
}

FGraphPanelSelectionSet FBlueprintEditor::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (FocusedGraphEd.IsValid())
	{
		CurrentSelection = FocusedGraphEd->GetSelectedNodes();
	}
	return CurrentSelection;
}

void FBlueprintEditor::RefreshEditors()
{
	DocumentManager->CleanInvalidTabs();

	DocumentManager->RefreshAllTabs();

	// The workflow manager only tracks document tabs.
	FocusInspectorOnGraphSelection(GetSelectedNodes(), true);
	if (MyBlueprintWidget.IsValid())
	{
		MyBlueprintWidget->Refresh();
	}

	if(SCSEditor.IsValid())
	{
		SCSEditor->UpdateTree();
		
		// Note: Don't pass 'true' here because we don't want the preview actor to be reconstructed until after Blueprint modification is complete.
		UpdateSCSPreview();
	}

	// Note: There is an optimization inside of ShowDetailsForSingleObject() that skips the refresh if the object being selected is the same as the previous object.
	// The SKismetInspector class is shared between both Defaults mode and Components mode, but in Defaults mode the object selected is always going to be the CDO. Given
	// that the selection does not really change, we force it to refresh and skip the optimization. Otherwise, some things may not work correctly in Defaults mode. For
	// example, transform details are customized and the rotation value is cached at customization time; if we don't force refresh here, then after an undo of a previous
	// rotation edit, transform details won't be re-customized and thus the cached rotation value will be stale, resulting in an invalid rotation value on the next edit.
	StartEditingDefaults(/*bAutoFocus=*/false, /*bForceRefresh=*/true);

	// Update associated controls like the function editor
	BroadcastRefresh();
}

void FBlueprintEditor::ClearSelectionInAllEditors()
{
	TArray< TSharedPtr<SDockTab> > GraphEditorTabs;
	DocumentManager->FindAllTabsForFactory(GraphEditorTabFactoryPtr, /*out*/ GraphEditorTabs);

	for (auto GraphEditorTabIt = GraphEditorTabs.CreateIterator(); GraphEditorTabIt; ++GraphEditorTabIt)
	{
		TSharedRef<SGraphEditor> Editor = StaticCastSharedRef<SGraphEditor>((*GraphEditorTabIt)->GetContent());

		Editor->ClearSelectionSet();
	}

	if(SCSEditor.IsValid())
	{
		SCSEditor->ClearSelection();
	}
}

void FBlueprintEditor::SummonSearchUI(bool bSetFindWithinBlueprint, FString NewSearchTerms, bool bSelectFirstResult)
{
	TabManager->InvokeTab(FBlueprintEditorTabs::FindResultsID);
	FindResults->FocusForUse(bSetFindWithinBlueprint, NewSearchTerms, bSelectFirstResult);
}

void FBlueprintEditor::UpdateSCSPreview(bool bUpdateNow)
{
	// refresh widget
	if(SCSViewport.IsValid())
	{
		// Ignore 'bUpdateNow' if "Components" mode is not current. Otherwise the preview actor might be spawned in as a result, which can lead to a few odd behaviors if the mode is not current.
		SCSViewport->RequestRefresh(false, bUpdateNow && IsModeCurrent(FBlueprintEditorApplicationModes::BlueprintComponentsMode));
	}
}



/** Create new tab for the supplied graph - don't call this directly.*/
TSharedRef<SGraphEditor> FBlueprintEditor::CreateGraphEditorWidget(TSharedRef<FTabInfo> InTabInfo, UEdGraph* InGraph)
{
	check((InGraph != NULL) && IsEditingSingleBlueprint());

	// Cache off whether or not this is an interface, since it is used to govern multiple widget's behavior
	const bool bIsInterface = (GetBlueprintObj()->BlueprintType == BPTYPE_Interface);
	const bool bIsDelegate = FBlueprintEditorUtils::IsDelegateSignatureGraph(InGraph);

	// No need to regenerate the commands.
	if(!GraphEditorCommands.IsValid())
	{
		GraphEditorCommands = MakeShareable( new FUICommandList );
		{
			GraphEditorCommands->MapAction(FGraphEditorCommands::Get().PromoteToVariable,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnPromoteToVariable ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanPromoteToVariable )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().AddExecutionPin,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnAddExecutionPin ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanAddExecutionPin )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().RemoveExecutionPin,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnRemoveExecutionPin ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanRemoveExecutionPin )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().AddOptionPin,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnAddOptionPin ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanAddOptionPin )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().RemoveOptionPin,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnRemoveOptionPin ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanRemoveOptionPin )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().ChangePinType,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnChangePinType ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanChangePinType )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().AddParentNode,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnAddParentNode ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanAddParentNode )
				);

			// Debug actions
			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().AddBreakpoint,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnAddBreakpoint ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanAddBreakpoint ),
				FIsActionChecked(),
				FIsActionButtonVisible::CreateSP( this, &FBlueprintEditor::CanAddBreakpoint )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().RemoveBreakpoint,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnRemoveBreakpoint ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanRemoveBreakpoint ),
				FIsActionChecked(),
				FIsActionButtonVisible::CreateSP( this, &FBlueprintEditor::CanRemoveBreakpoint )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().EnableBreakpoint,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnEnableBreakpoint ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanEnableBreakpoint ),
				FIsActionChecked(),
				FIsActionButtonVisible::CreateSP( this, &FBlueprintEditor::CanEnableBreakpoint )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().DisableBreakpoint,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnDisableBreakpoint ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanDisableBreakpoint ),
				FIsActionChecked(),
				FIsActionButtonVisible::CreateSP( this, &FBlueprintEditor::CanDisableBreakpoint )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().ToggleBreakpoint,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnToggleBreakpoint ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanToggleBreakpoint ),
				FIsActionChecked(),
				FIsActionButtonVisible::CreateSP( this, &FBlueprintEditor::CanToggleBreakpoint )
				);

			// Encapsulation commands
			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().CollapseNodes,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnCollapseNodes ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanCollapseNodes )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().CollapseSelectionToFunction,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnCollapseSelectionToFunction ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanCollapseSelectionToFunction ),
				FIsActionChecked(),
				FIsActionButtonVisible::CreateSP( this, &FBlueprintEditor::NewDocument_IsVisibleForType, CGT_NewFunctionGraph )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().CollapseSelectionToMacro,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnCollapseSelectionToMacro ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanCollapseSelectionToMacro ),
				FIsActionChecked(),
				FIsActionButtonVisible::CreateSP( this, &FBlueprintEditor::NewDocument_IsVisibleForType, CGT_NewMacroGraph )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().PromoteSelectionToFunction,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnPromoteSelectionToFunction ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanPromoteSelectionToFunction ),
				FIsActionChecked(),
				FIsActionButtonVisible::CreateSP( this, &FBlueprintEditor::NewDocument_IsVisibleForType, CGT_NewFunctionGraph )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().PromoteSelectionToMacro,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnPromoteSelectionToMacro ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanPromoteSelectionToMacro ),
				FIsActionChecked(),
				FIsActionButtonVisible::CreateSP( this, &FBlueprintEditor::NewDocument_IsVisibleForType, CGT_NewMacroGraph )
				);

			GraphEditorCommands->MapAction( FGenericCommands::Get().Rename,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnRenameNode ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanRenameNodes )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().ExpandNodes,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnExpandNodes ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanExpandNodes ),
				FIsActionChecked(),
				FIsActionButtonVisible::CreateSP( this, &FBlueprintEditor::CanExpandNodes )
				);

			// Editing commands
			GraphEditorCommands->MapAction( FGenericCommands::Get().SelectAll,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::SelectAllNodes ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanSelectAllNodes )
				);

			GraphEditorCommands->MapAction( FGenericCommands::Get().Delete,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::DeleteSelectedNodes ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanDeleteNodes )
				);

			GraphEditorCommands->MapAction( FGenericCommands::Get().Copy,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::CopySelectedNodes ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanCopyNodes )
				);

			GraphEditorCommands->MapAction( FGenericCommands::Get().Cut,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::CutSelectedNodes ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanCutNodes )
				);

			GraphEditorCommands->MapAction( FGenericCommands::Get().Paste,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::PasteNodes ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanPasteNodes )
				);

			GraphEditorCommands->MapAction( FGenericCommands::Get().Duplicate,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::DuplicateNodes ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanDuplicateNodes )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().SelectReferenceInLevel,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnSelectReferenceInLevel ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanSelectReferenceInLevel ),
				FIsActionChecked(),
				FIsActionButtonVisible::CreateSP( this, &FBlueprintEditor::CanSelectReferenceInLevel )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().AssignReferencedActor,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnAssignReferencedActor ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanAssignReferencedActor ) );

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().StartWatchingPin,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnStartWatchingPin ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanStartWatchingPin )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().StopWatchingPin,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnStopWatchingPin ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanStopWatchingPin )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().SelectBone,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnSelectBone ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanSelectBone )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().AddBlendListPin,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnAddPosePin ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanAddPosePin )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().RemoveBlendListPin,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnRemovePosePin ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanRemovePosePin )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().ConvertToSeqEvaluator,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnConvertToSequenceEvaluator )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().ConvertToSeqPlayer,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnConvertToSequencePlayer )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().ConvertToBSEvaluator,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnConvertToBlendSpaceEvaluator )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().ConvertToBSPlayer,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnConvertToBlendSpacePlayer )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().OpenRelatedAsset,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnOpenRelatedAsset )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().EditTunnel,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnEditTunnel )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().CreateComment,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnCreateComment )
				);

			GraphEditorCommands->MapAction(FGraphEditorCommands::Get().ShowAllPins,
				FExecuteAction::CreateSP(this, &FBlueprintEditor::SetPinVisibility, SGraphEditor::Pin_Show)
				);

			GraphEditorCommands->MapAction(FGraphEditorCommands::Get().HideNoConnectionPins,
				FExecuteAction::CreateSP(this, &FBlueprintEditor::SetPinVisibility, SGraphEditor::Pin_HideNoConnection)
				);

			GraphEditorCommands->MapAction(FGraphEditorCommands::Get().HideNoConnectionNoDefaultPins,
				FExecuteAction::CreateSP(this, &FBlueprintEditor::SetPinVisibility, SGraphEditor::Pin_HideNoConnectionNoDefault)
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().FindInstancesOfCustomEvent,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnFindInstancesCustomEvent )
				);

			GraphEditorCommands->MapAction( FGraphEditorCommands::Get().FindVariableReferences,
				FExecuteAction::CreateSP( this, &FBlueprintEditor::OnFindVariableReferences ),
				FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanFindVariableReferences )
				);
		}
	}

	// Create the title bar widget
	TSharedPtr<SWidget> TitleBarWidget = SNew(SGraphTitleBar)
		.EdGraphObj(InGraph)
		.Kismet2(SharedThis(this))
		.OnDifferentGraphCrumbClicked(this, &FBlueprintEditor::OnChangeBreadCrumbGraph)
		.HistoryNavigationWidget(InTabInfo->CreateHistoryNavigationWidget());

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP( this, &FBlueprintEditor::OnSelectedNodesChanged );
	InEvents.OnDropActor = SGraphEditor::FOnDropActor::CreateSP( this, &FBlueprintEditor::OnGraphEditorDropActor );
	InEvents.OnDropStreamingLevel = SGraphEditor::FOnDropStreamingLevel::CreateSP( this, &FBlueprintEditor::OnGraphEditorDropStreamingLevel );
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FBlueprintEditor::OnNodeDoubleClicked);
	InEvents.OnVerifyTextCommit = FOnNodeVerifyTextCommit::CreateSP(this, &FBlueprintEditor::OnNodeVerifyTitleCommit);
	InEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FBlueprintEditor::OnNodeTitleCommitted);
	InEvents.OnSpawnNodeByShortcut = SGraphEditor::FOnSpawnNodeByShortcut::CreateSP(this, &FBlueprintEditor::OnSpawnGraphNodeByShortcut, InGraph);
	InEvents.OnNodeSpawnedByKeymap = SGraphEditor::FOnNodeSpawnedByKeymap::CreateSP(this, &FBlueprintEditor::OnNodeSpawnedByKeymap );
	InEvents.OnDisallowedPinConnection = SGraphEditor::FOnDisallowedPinConnection::CreateSP(this, &FBlueprintEditor::OnDisallowedPinConnection);

	// Custom menu for K2 schemas
	if(InGraph->Schema != NULL && InGraph->Schema->IsChildOf(UEdGraphSchema_K2::StaticClass()))
	{
		InEvents.OnCreateActionMenu = SGraphEditor::FOnCreateActionMenu::CreateSP(this, &FBlueprintEditor::OnCreateGraphActionMenu);
	}

	TSharedRef<SGraphEditor> Editor = SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(this, &FBlueprintEditor::InEditingMode)
		.TitleBar(TitleBarWidget)
		.TitleBarEnabledOnly(bIsInterface || bIsDelegate)
		.Appearance(this, &FBlueprintEditor::GetGraphAppearance)
		.GraphToEdit(InGraph)
		.GraphEvents(InEvents)
		.OnNavigateHistoryBack(FSimpleDelegate::CreateSP(this, &FBlueprintEditor::NavigateTab, FDocumentTracker::NavigateBackwards))
		.OnNavigateHistoryForward(FSimpleDelegate::CreateSP(this, &FBlueprintEditor::NavigateTab, FDocumentTracker::NavigateForwards));
		//@TODO: Crashes in command list code during the callback .OnGraphModuleReloaded(FEdGraphEvent::CreateSP(this, &FBlueprintEditor::ChangeOpenGraphInDocumentEditorWidget, WeakParent))
		;

	OnSetPinVisibility.AddSP(&Editor.Get(), &SGraphEditor::SetPinVisibility);

	FVector2D ViewOffset = FVector2D::ZeroVector;
	float ZoomAmount = INDEX_NONE;

	TSharedPtr<SDockTab> ActiveTab = DocumentManager->GetActiveTab();
	if(ActiveTab.IsValid())
	{
		// Check if the graph is already opened in the current tab, if it is we want to start at the same position to stop the graph from jumping around oddly
		TSharedPtr<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(ActiveTab->GetContent());

		if(GraphEditor.IsValid() && GraphEditor->GetCurrentGraph() == InGraph)
		{
			GraphEditor->GetViewLocation(ViewOffset, ZoomAmount);
		}
	}

	Editor->SetViewLocation(ViewOffset, ZoomAmount);

	return Editor;
}

FGraphAppearanceInfo FBlueprintEditor::GetGraphAppearance() const
{
	// Create the appearance info
	FGraphAppearanceInfo AppearanceInfo;
	if (GetBlueprintObj()->IsA(UAnimBlueprint::StaticClass()))
	{
		AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_Animation", "ANIMATION").ToString();
	}
	else
	{
		switch (GetBlueprintObj()->BlueprintType)
		{
		case BPTYPE_LevelScript:
			AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_LevelScript", "LEVEL BLUEPRINT").ToString();
			break;
		case BPTYPE_MacroLibrary:
			AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_Macro", "MACRO").ToString();
			break;
		case BPTYPE_Interface:
			AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_Interface", "INTERFACE").ToString();
			break;
		default:
			AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_Blueprint", "BLUEPRINT").ToString();
			break;
		}

	}

	AppearanceInfo.PIENotifyText = GetPIEStatus();

	return AppearanceInfo;
}

// Open the editor for a given graph
void FBlueprintEditor::OnChangeBreadCrumbGraph(class UEdGraph* InGraph)
{
	if (InGraph && FocusedGraphEdPtr.IsValid())
	{
		OpenDocument(InGraph, FDocumentTracker::NavigatingCurrentDocument);
	}
}

static auto CVarBlueprintExpertMode	= IConsoleManager::Get().RegisterConsoleVariableRef(
	TEXT("Kismet.ExpertMode"),
	FK2ActionMenuBuilder::bAllowUnsafeCommands,
	*LOCTEXT("EnableExportMode_Desc", "Enable expert mode options, which will cause context menus to show additional nodes that are not verified to be safe/legal to use in the current context.").ToString()
	);

FBlueprintEditor::FBlueprintEditor()
	: bSaveIntermediateBuildProducts(false)
	, bRequestedSavingOpenDocumentState(false)
	, bBlueprintModifiedOnOpen (false)
	, PinVisibility(SGraphEditor::Pin_Show)
	, bIsActionMenuContextSensitive(true)
	, CurrentUISelection(FBlueprintEditor::NoSelection)
	, bEditorMarkedAsClosed(false)
{
	AnalyticsStats.GraphActionMenusNonCtxtSensitiveExecCount = 0;
	AnalyticsStats.GraphActionMenusCtxtSensitiveExecCount = 0;
	AnalyticsStats.GraphActionMenusCancelledCount = 0;
	AnalyticsStats.MyBlueprintNodeDragPlacementCount = 0;
	AnalyticsStats.PaletteNodeDragPlacementCount = 0;
	AnalyticsStats.NodeGraphContextCreateCount = 0;
	AnalyticsStats.NodePinContextCreateCount = 0;
	AnalyticsStats.NodeKeymapCreateCount = 0;
	AnalyticsStats.NodePasteCreateCount = 0;

	UEditorEngine* Editor = (UEditorEngine*)GEngine;
	if (Editor != NULL)
	{
		Editor->RegisterForUndo(this);
	}

	DocumentManager = MakeShareable(new FDocumentTracker);
}

void FBlueprintEditor::SetSCSNodesTransactional(USCS_Node* Node)
{
	if (Node != NULL)
	{
		Node->SetFlags(RF_Transactional);

		for (int32 i = 0; i < Node->ChildNodes.Num(); i++)
		{
			SetSCSNodesTransactional(Node->ChildNodes[i]);
		}
	}
}

void FBlueprintEditor::EnsureBlueprintIsUpToDate(UBlueprint* BlueprintObj)
{
	// Purge any NULL graphs
	FBlueprintEditorUtils::PurgeNullGraphs(BlueprintObj);

	// Make sure the blueprint is cosmetically up to date
	FKismetEditorUtilities::UpgradeCosmeticallyStaleBlueprint(BlueprintObj);

	if (FBlueprintEditorUtils::SupportsConstructionScript(BlueprintObj))
	{
		// If we don't have an SCS yet, make it
		if(BlueprintObj->SimpleConstructionScript == NULL)
		{
			check(NULL != BlueprintObj->GeneratedClass);
			BlueprintObj->SimpleConstructionScript = NewObject<USimpleConstructionScript>(BlueprintObj->GeneratedClass);
			BlueprintObj->SimpleConstructionScript->SetFlags(RF_Transactional);

			// Recreate (or create) any widgets that depend on the SCS
			SCSEditor = SNew(SSCSEditor, SharedThis(this), BlueprintObj->SimpleConstructionScript);
			SCSViewport = SAssignNew(SCSViewport, SSCSEditorViewport) .BlueprintEditor(SharedThis(this));
		}

		// If we should have a UCS but don't yet, make it
		if(!FBlueprintEditorUtils::FindUserConstructionScript(BlueprintObj))
		{
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			UEdGraph* UCSGraph = FBlueprintEditorUtils::CreateNewGraph(BlueprintObj, K2Schema->FN_UserConstructionScript, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
			FBlueprintEditorUtils::AddFunctionGraph(BlueprintObj, UCSGraph, /*bIsUserCreated=*/ false, AActor::StaticClass());
			UCSGraph->bAllowDeletion = false;
		}
	}

	// Set transactional flag on SimpleConstructionScript and Nodes associated with it.
	if ((BlueprintObj->SimpleConstructionScript != NULL) && (BlueprintObj->GetLinkerUE4Version() < VER_UE4_FLAG_SCS_TRANSACTIONAL))
	{
		BlueprintObj->SimpleConstructionScript->SetFlags( RF_Transactional );
		const TArray<USCS_Node*>& RootNodes = BlueprintObj->SimpleConstructionScript->GetRootNodes();
		for(int32 i = 0; i < RootNodes.Num(); ++i)
		{
			SetSCSNodesTransactional( RootNodes[i] );
		}
		BlueprintObj->Status = BS_Dirty;
	}

	// Make sure that this blueprint is up-to-date with regards to its parent functions
	FBlueprintEditorUtils::ConformCallsToParentFunctions(BlueprintObj);

	// Make sure that this blueprint is up-to-date with regards to its implemented events
	FBlueprintEditorUtils::ConformImplementedEvents(BlueprintObj);

	// Make sure that this blueprint is up-to-date with regards to its implemented interfaces
	FBlueprintEditorUtils::ConformImplementedInterfaces(BlueprintObj);

	// Update old composite nodes(can't do this in PostLoad)
	FBlueprintEditorUtils::UpdateOutOfDateCompositeNodes(BlueprintObj);

	// Update any nodes which might have dropped their RF_Transactional flag due to copy-n-paste issues
	FBlueprintEditorUtils::UpdateTransactionalFlags(BlueprintObj);
}

void FBlueprintEditor::CommonInitialization(const TArray<UBlueprint*>& InitBlueprints)
{
	TSharedPtr<FBlueprintEditor> ThisPtr(SharedThis(this));

	// @todo TabManagement
	DocumentManager->Initialize(ThisPtr);

	// Register the document factories
	{
		DocumentManager->RegisterDocumentFactory(MakeShareable(new FTimelineEditorSummoner(ThisPtr)));

		TSharedRef<FDocumentTabFactory> GraphEditorFactory = MakeShareable(new FGraphEditorSummoner(ThisPtr,
			FGraphEditorSummoner::FOnCreateGraphEditorWidget::CreateSP(this, &FBlueprintEditor::CreateGraphEditorWidget)
			));

		// Also store off a reference to the grapheditor factory so we can find all the tabs spawned by it later.
		GraphEditorTabFactoryPtr = GraphEditorFactory;
		DocumentManager->RegisterDocumentFactory(GraphEditorFactory);
	}

	// Make sure we know when tabs become active to update details tab
	FGlobalTabmanager::Get()->OnActiveTabChanged_Subscribe( FOnActiveTabChanged::FDelegate::CreateRaw(this, &FBlueprintEditor::OnActiveTabChanged) );

	if (InitBlueprints.Num() == 1)
	{
		UBlueprint* InitBlueprint = InitBlueprints[0];
		// Update the blueprint if required
		EBlueprintStatus OldStatus = InitBlueprint->Status;
		EnsureBlueprintIsUpToDate(InitBlueprint);
		bBlueprintModifiedOnOpen = (InitBlueprint->Status != OldStatus);

		// Flag the blueprint as having been opened
		InitBlueprint->bIsNewlyCreated = false;

		// Load blueprint libraries
		LoadLibrariesFromAssetRegistry();

		LoadUserDefinedEnumsFromAssetRegistry();

		// When the blueprint that we are observing changes, it will notify this wrapper widget.
		InitBlueprint->OnChanged().AddSP( this, &FBlueprintEditor::OnBlueprintChanged );
	}

	CreateDefaultCommands();
	CreateDefaultTabContents(InitBlueprints);
	
	if(FStructureEditorUtils::StructureEditingEnabled())
	{
		UserDefinedStructureEditor = SNew(SUserDefinedStructureEditor, SharedThis(this));
	}
}

void FBlueprintEditor::LoadUserDefinedEnumsFromAssetRegistry()
{
	if( GetBlueprintObj() )
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

		TArray<FAssetData> AssetData;
		AssetRegistryModule.Get().GetAssetsByClass(UUserDefinedEnum::StaticClass()->GetFName(), AssetData);

		for (int32 AssetIndex = 0; AssetIndex < AssetData.Num(); ++AssetIndex)
		{
			if(AssetData[AssetIndex].IsValid())
			{
				FString AssetPath = AssetData[AssetIndex].ObjectPath.ToString();
				UUserDefinedEnum* EnumObject = LoadObject<UUserDefinedEnum>(NULL, *AssetPath, NULL, 0, NULL);
				if (EnumObject)
				{
					UserDefinedEnumerators.AddUnique( TWeakObjectPtr<UUserDefinedEnum>(EnumObject) );
				}
			}
		}
	}
}

void FBlueprintEditor::LoadLibrariesFromAssetRegistry()
{
	if( GetBlueprintObj() )
	{
		FString UserDeveloperPath = FPackageName::FilenameToLongPackageName( FPaths::GameUserDeveloperDir());
		FString DeveloperPath = FPackageName::FilenameToLongPackageName( FPaths::GameDevelopersDir() );

		//Don't allow loading blueprint macros & functions for interface & data-only blueprints
		if( GetBlueprintObj()->BlueprintType != BPTYPE_Interface && GetBlueprintObj()->BlueprintType != BPTYPE_Const)
		{
			// Load the asset registry module
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

			// Collect a full list of assets with the specified class
			TArray<FAssetData> AssetData;
			AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), AssetData);

			GWarn->BeginSlowTask(LOCTEXT("LoadingBlueprintAssetData", "Loading Blueprint Asset Data"), true );

			const FName BPTypeName( TEXT( "BlueprintType" ) );
			const FString BPMacroTypeStr( TEXT( "BPTYPE_MacroLibrary" ) );
			const FString BPFunctionTypeStr( TEXT( "BPTYPE_FunctionLibrary" ) );

			for (int32 AssetIndex = 0; AssetIndex < AssetData.Num(); ++AssetIndex)
			{
				bool FoundBPType = false;
				for (TMap<FName, FString>::TConstIterator TagIt(AssetData[ AssetIndex ].TagsAndValues); TagIt; ++TagIt)
				{
					FString TagValue = AssetData[ AssetIndex ].TagsAndValues.FindRef(BPTypeName);

					//Only check for Blueprint Macros & Functions in the asset data for loading
					if ( TagValue == BPMacroTypeStr || TagValue == BPFunctionTypeStr )
					{
						FoundBPType = true;
						break;
					}
				}

				if( FoundBPType )
				{
					FString BlueprintPath = AssetData[AssetIndex].ObjectPath.ToString();

					//For blueprints inside developers folder, only allow the ones inside current user's developers folder.
					bool AllowLoadBP = true;
					if( BlueprintPath.StartsWith(DeveloperPath) )
					{
						if(  !BlueprintPath.StartsWith(UserDeveloperPath) )
						{
							AllowLoadBP = false;
						}
					}

					if( AllowLoadBP )
					{
						// Load the blueprint
						UBlueprint* BlueprintLibPtr = LoadObject<UBlueprint>(NULL, *BlueprintPath, NULL, 0, NULL);
						if (BlueprintLibPtr )
						{
							StandardLibraries.AddUnique(BlueprintLibPtr);
						}
					}
				}

			}
			GWarn->EndSlowTask();
		}
	}
}

void FBlueprintEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	//@TODO: Can't we do this sooner?
	DocumentManager->SetTabManager(TabManager);

	FWorkflowCentricApplication::RegisterTabSpawners(TabManager);
}

void FBlueprintEditor::InitBlueprintEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray<UBlueprint*>& InBlueprints, bool bShouldOpenInDefaultsMode)
{
	check(InBlueprints.Num() == 1 || bShouldOpenInDefaultsMode);

	// Newly-created Blueprints will open in Components mode rather than Standard mode
	bool bShouldOpenInComponentsMode = !bShouldOpenInDefaultsMode && InBlueprints.Num() == 1 && InBlueprints[0]->bIsNewlyCreated;

	TArray< UObject* > Objects;
	for( auto BlueprintIter = InBlueprints.CreateConstIterator(); BlueprintIter; ++BlueprintIter )
	{
		auto Blueprint = *BlueprintIter;

		// Flag the blueprint as having been opened
		Blueprint->bIsNewlyCreated = false;

		Objects.Add( Blueprint );
	}
	
	if (!Toolbar.IsValid())
	{
		Toolbar = MakeShareable(new FBlueprintEditorToolbar(SharedThis(this)));
	}

	GetToolkitCommands()->Append(FPlayWorldCommands::GlobalPlayWorldActions.ToSharedRef());

	// Initialize the asset editor and spawn nothing (dummy layout)
	const TSharedRef<FTabManager::FLayout> DummyLayout = FTabManager::NewLayout("NullLayout")->AddArea(FTabManager::NewPrimaryArea());
	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	InitAssetEditor(Mode, InitToolkitHost, BlueprintEditorAppName, DummyLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, Objects);
	
	CommonInitialization(InBlueprints);
	
	TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender);
	FKismet2Menu::SetupBlueprintEditorMenu( MenuExtender, *this );
	AddMenuExtender(MenuExtender);

	FBlueprintEditorModule* BlueprintEditorModule = &FModuleManager::LoadModuleChecked<FBlueprintEditorModule>( "Kismet" );
	AddMenuExtender(BlueprintEditorModule->GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
	
	RegenerateMenusAndToolbars();

	// Create the modes and activate one (which will populate with a real layout)
	if (UBlueprint* SingleBP = GetBlueprintObj())
	{
		if (!bShouldOpenInDefaultsMode && FBlueprintEditorUtils::IsInterfaceBlueprint(SingleBP))
		{
			// Interfaces are only valid in the Interface mode
			AddApplicationMode(
				FBlueprintEditorApplicationModes::BlueprintInterfaceMode, 
				MakeShareable(new FBlueprintInterfaceApplicationMode( SharedThis(this) )));
			SetCurrentMode(FBlueprintEditorApplicationModes::BlueprintInterfaceMode);
		}
		else if (SingleBP->BlueprintType == BPTYPE_MacroLibrary)
		{
			// Macro libraries are only valid in the Macro mode
			AddApplicationMode(
				FBlueprintEditorApplicationModes::BlueprintMacroMode, 
				MakeShareable(new FBlueprintMacroApplicationMode( SharedThis(this) )));
			SetCurrentMode(FBlueprintEditorApplicationModes::BlueprintMacroMode);
		}
		else
		{
			AddApplicationMode(
				FBlueprintEditorApplicationModes::StandardBlueprintEditorMode, 
				MakeShareable(new FBlueprintEditorApplicationMode(SharedThis(this), FBlueprintEditorApplicationModes::StandardBlueprintEditorMode)));
			AddApplicationMode(
				FBlueprintEditorApplicationModes::BlueprintDefaultsMode, 
				MakeShareable(new FBlueprintDefaultsApplicationMode(SharedThis(this))));
			AddApplicationMode(
				FBlueprintEditorApplicationModes::BlueprintComponentsMode, 
				MakeShareable(new FBlueprintComponentsApplicationMode(SharedThis(this))));

			if (bShouldOpenInDefaultsMode)
			{
				SetCurrentMode(FBlueprintEditorApplicationModes::BlueprintDefaultsMode);
			}
			else if (bShouldOpenInComponentsMode && CanAccessComponentsMode())
			{
				SetCurrentMode(FBlueprintEditorApplicationModes::BlueprintComponentsMode);
			}
			else
			{
				SetCurrentMode(FBlueprintEditorApplicationModes::StandardBlueprintEditorMode);
			}
		}
	}
	else
	{
		// We either have no blueprints or many, open in the defaults mode for multi-editing
		AddApplicationMode(
			FBlueprintEditorApplicationModes::BlueprintDefaultsMode, 
			MakeShareable(new FBlueprintDefaultsApplicationMode(SharedThis(this))));
		SetCurrentMode(FBlueprintEditorApplicationModes::BlueprintDefaultsMode);
	}

	// Post-layout initialization
	PostLayoutBlueprintEditorInitialization();

	// Find and set any instances of this blueprint type if any exists and we are not already editing one
	FBlueprintEditorUtils::FindAndSetDebuggableBlueprintInstances();	
}

void FBlueprintEditor::PostRegenerateMenusAndToolbars()
{
	UBlueprint* BluePrint = GetBlueprintObj();

	if ( BluePrint && !FBlueprintEditorUtils::IsLevelScriptBlueprint( BluePrint ) )
	{
		// build and attach the menu overlay
		TSharedRef<SHorizontalBox> MenuOverlayBox = SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.ColorAndOpacity( FSlateColor::UseSubduedForeground() )
				.ShadowOffset( FVector2D::UnitVector )
				.Text(LOCTEXT("BlueprintEditor_ParentClass", "Parent class: ").ToString())
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SSpacer)
				.Size(FVector2D(2.0f,1.0f))
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.ShadowOffset( FVector2D::UnitVector )
				.Text(this, &FBlueprintEditor::GetParentClassName)
				.ToolTipText(LOCTEXT("ParentClassToolTip", "The class that the current Blueprint is based on. The parent provides the base definition, which the current Blueprint extends.").ToString())
				.Visibility(this, &FBlueprintEditor::GetParentClassNameVisibility)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.VAlign(VAlign_Center)
				.ButtonStyle( FEditorStyle::Get(), "HoverHintOnly" )
				.OnClicked( this, &FBlueprintEditor::OnFindParentClassInContentBrowserClicked )
				.IsEnabled( this, &FBlueprintEditor::IsParentClassABlueprint )
				.Visibility( this, &FBlueprintEditor::ParentClassButtonsVisibility )
				.ToolTipText( LOCTEXT("FindParentInCBToolTip", "Find parent in Content Browser").ToString() )
				.ContentPadding(4.0f)
				.ForegroundColor( FSlateColor::UseForeground() )
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("PropertyWindow.Button_Browse"))
				]
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.VAlign(VAlign_Center)
				.ButtonStyle( FEditorStyle::Get(), "HoverHintOnly" )
				.OnClicked( this, &FBlueprintEditor::OnEditParentClassClicked )
				.IsEnabled( this, &FBlueprintEditor::IsParentClassABlueprint )
				.Visibility( this, &FBlueprintEditor::ParentClassButtonsVisibility )
				.ToolTipText( LOCTEXT("EditParentClassToolTip", "Open parent in editor").ToString() )
				.ContentPadding(4.0f)
				.ForegroundColor( FSlateColor::UseForeground() )
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("PropertyWindow.Button_Edit"))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SHyperlink)
				.Style(FEditorStyle::Get(), "HoverOnlyHyperlink")
				.IsEnabled(this, &FBlueprintEditor::IsNativeParentClassCodeLinkEnabled)
				.Visibility(this, &FBlueprintEditor::GetNativeParentClassButtonsVisibility)
				.OnNavigate(this, &FBlueprintEditor::OnEditParentClassNativeCodeClicked)
				.Text(this, &FBlueprintEditor::GetTextForNativeParentClassHeaderLink)
				.ToolTipText(LOCTEXT("GoToCode_ToolTip", "Click to open this source file in a text editor"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SSpacer)
				.Size(FVector2D(2.0f, 1.0f))
			]
			;
		SetMenuOverlay( MenuOverlayBox );
	}
}

FString FBlueprintEditor::GetParentClassName() const
{
	UClass* ParentClass = (GetBlueprintObj() != NULL) ? GetBlueprintObj()->ParentClass : NULL;
	return ((ParentClass != NULL) ? FText::FromName( ParentClass->GetFName() ) : LOCTEXT("BlueprintEditor_NoParentClass", "None")).ToString();
}

bool FBlueprintEditor::IsParentClassOfObjectABlueprint( const UBlueprint* Blueprint ) const
{
	if ( Blueprint != NULL )
	{
		UObject* ParentClass = Blueprint->ParentClass;
		if ( ParentClass != NULL )
		{
			if ( ParentClass->IsA( UBlueprintGeneratedClass::StaticClass() ) )
			{
				return true;
			}
		}
	}

	return false;
}

bool FBlueprintEditor::IsParentClassABlueprint() const
{
	const UBlueprint* Blueprint = GetBlueprintObj();

	return IsParentClassOfObjectABlueprint( Blueprint );
}

EVisibility FBlueprintEditor::ParentClassButtonsVisibility() const
{
	return IsParentClassABlueprint() ? EVisibility::Visible : EVisibility::Collapsed;
}

bool FBlueprintEditor::IsParentClassNative() const
{
	const UBlueprint* Blueprint = GetBlueprintObj();
	if (Blueprint != NULL)
	{
		UClass* ParentClass = Blueprint->ParentClass;
		if (ParentClass != NULL)
		{
			if (ParentClass->HasAllClassFlags(CLASS_Native))
			{
				return true;
			}
		}
	}

	return false;
}

bool FBlueprintEditor::IsNativeParentClassCodeLinkEnabled() const
{
	return IsParentClassNative() && FSourceCodeNavigation::IsCompilerAvailable();
}

EVisibility FBlueprintEditor::GetNativeParentClassButtonsVisibility() const
{
	return IsNativeParentClassCodeLinkEnabled() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FBlueprintEditor::GetParentClassNameVisibility() const
{
	return !IsNativeParentClassCodeLinkEnabled() ? EVisibility::Visible : EVisibility::Collapsed;
}

void FBlueprintEditor::OnEditParentClassNativeCodeClicked()
{
	const UBlueprint* Blueprint = GetBlueprintObj();
	UClass* ParentClass = Blueprint ? Blueprint->ParentClass : NULL;
	if (IsNativeParentClassCodeLinkEnabled() && ParentClass)
	{
		FString NativeParentClassHeaderPath;
		const bool bFileFound = FSourceCodeNavigation::FindClassHeaderPath(ParentClass, NativeParentClassHeaderPath) 
			&& (IFileManager::Get().FileSize(*NativeParentClassHeaderPath) != INDEX_NONE);
		if (bFileFound)
		{
			const FString AbsoluteHeaderPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*NativeParentClassHeaderPath);
			FSourceCodeNavigation::OpenSourceFile( AbsoluteHeaderPath );
		}
	}
}

FText FBlueprintEditor::GetTextForNativeParentClassHeaderLink() const
{
	// it could be done using FSourceCodeNavigation, but it could be slow
	return FText::FromString(FString::Printf(TEXT("%s.h"), *GetParentClassName()));
}

FReply FBlueprintEditor::OnFindParentClassInContentBrowserClicked()
{
	UBlueprint* Blueprint = GetBlueprintObj();
	if ( Blueprint != NULL )
	{
		UObject* ParentClass = Blueprint->ParentClass;
		if ( ParentClass != NULL )
		{
			UBlueprintGeneratedClass* ParentBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>( ParentClass );
			if ( ParentBlueprintGeneratedClass != NULL )
			{
				if ( ParentBlueprintGeneratedClass->ClassGeneratedBy != NULL )
				{
					TArray< UObject* > ParentObjectList;
					ParentObjectList.Add( ParentBlueprintGeneratedClass->ClassGeneratedBy );
					GEditor->SyncBrowserToObjects( ParentObjectList );
				}
			}
		}
	}

	return FReply::Handled();
}

FReply FBlueprintEditor::OnEditParentClassClicked()
{
	UBlueprint* Blueprint = GetBlueprintObj();
	if ( Blueprint != NULL )
	{
		UObject* ParentClass = Blueprint->ParentClass;
		if ( ParentClass != NULL )
		{
			UBlueprintGeneratedClass* ParentBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>( ParentClass );
			if ( ParentBlueprintGeneratedClass != NULL )
			{
				FAssetEditorManager::Get().OpenEditorForAsset( ParentBlueprintGeneratedClass->ClassGeneratedBy );
			}
		}
	}

	return FReply::Handled();
}

void FBlueprintEditor::PostLayoutBlueprintEditorInitialization()
{
	if (GetBlueprintObj() != NULL)
	{
		// Refresh the graphs
		RefreshEditors();
		
		// EnsureBlueprintIsUpToDate may have updated the blueprint so show notifications to user.
		if (bBlueprintModifiedOnOpen)
		{
			bBlueprintModifiedOnOpen = false;

			if (FocusedGraphEdPtr.IsValid())
			{
				FNotificationInfo Info( NSLOCTEXT("Kismet", "Blueprint Modified", "Blueprint requires updating. Please resave.") );
				Info.Image = FEditorStyle::GetBrush(TEXT("Icons.Info"));
				Info.bFireAndForget = true;
				Info.bUseSuccessFailIcons = false;
				Info.ExpireDuration = 5.0f;

				FocusedGraphEdPtr.Pin()->AddNotification(Info, true);
			}

			// Fire log message
			FString BlueprintName;
			GetBlueprintObj()->GetName(BlueprintName);

			FFormatNamedArguments Args;
			Args.Add( TEXT("BlueprintName"), FText::FromString( BlueprintName ) );
			LogSimpleMessage( FText::Format( LOCTEXT("Blueprint Modified Long", "Blueprint \"{BlueprintName}\" was updated to fix issues detected on load. Please resave."), Args ) );
		}
	}
}

void FBlueprintEditor::SetupViewForBlueprintEditingMode()
{
	// Make sure the defaults tab is pointing to the defaults
	StartEditingDefaults(/*bAutoFocus=*/ true);

	// Make sure the inspector is always on top
	//@TODO: This is necessary right now because of a bug in restoring layouts not remembering which tab is on top (to get it right initially), but do we want this behavior always?
	TabManager->InvokeTab(FBlueprintEditorTabs::DetailsID);
}

FBlueprintEditor::~FBlueprintEditor()
{
	// NOTE: Any tabs that we still have hanging out when destroyed will be cleaned up by FBaseToolkit's destructor
	UEditorEngine* Editor = (UEditorEngine*)GEngine;
	if (Editor)
	{
		Editor->UnregisterForUndo( this );
	}

	if (GetBlueprintObj())
	{
		GetBlueprintObj()->OnChanged().RemoveAll( this );
	}

	FGlobalTabmanager::Get()->OnActiveTabChanged_Unsubscribe( FOnActiveTabChanged::FDelegate::CreateRaw(this, &FBlueprintEditor::OnActiveTabChanged) );

	if (FEngineAnalytics::IsAvailable())
	{
		const UGeneralProjectSettings& ProjectSettings = *GetDefault<UGeneralProjectSettings>();
		FString ProjectID = ProjectSettings.ProjectID.ToString();

		TArray< FAnalyticsEventAttribute > BPEditorAttribs;
		BPEditorAttribs.Add( FAnalyticsEventAttribute( FString( "GraphActionMenusExecuted.NonContextSensitive" ), AnalyticsStats.GraphActionMenusNonCtxtSensitiveExecCount ));
		BPEditorAttribs.Add( FAnalyticsEventAttribute( FString( "GraphActionMenusExecuted.ContextSensitive" ), AnalyticsStats.GraphActionMenusCtxtSensitiveExecCount ));
		BPEditorAttribs.Add( FAnalyticsEventAttribute( FString( "GraphActionMenusClosed" ), AnalyticsStats.GraphActionMenusCancelledCount ));

		BPEditorAttribs.Add( FAnalyticsEventAttribute( FString( "MyBlueprintDragPlacedNodesCreated" ), AnalyticsStats.MyBlueprintNodeDragPlacementCount ));
		BPEditorAttribs.Add( FAnalyticsEventAttribute( FString( "BlueprintPaletteDragPlacedNodesCreated" ), AnalyticsStats.PaletteNodeDragPlacementCount ));
		BPEditorAttribs.Add( FAnalyticsEventAttribute( FString( "GraphContextNodesCreated" ), AnalyticsStats.NodeGraphContextCreateCount ));
		BPEditorAttribs.Add( FAnalyticsEventAttribute( FString( "GraphPinContextNodesCreated" ), AnalyticsStats.NodePinContextCreateCount ));
		BPEditorAttribs.Add( FAnalyticsEventAttribute( FString( "KeymapNodesCreated" ), AnalyticsStats.NodeKeymapCreateCount ));
		BPEditorAttribs.Add( FAnalyticsEventAttribute( FString( "PastedNodesCreated" ), AnalyticsStats.NodePasteCreateCount ));

		BPEditorAttribs.Add( FAnalyticsEventAttribute( FString( "ProjectId" ), ProjectID ) );

		TArray< FAnalyticsEventAttribute > BPEditorNodeStats;
		for( auto Iter = AnalyticsStats.CreatedNodeTypes.CreateIterator(); Iter; ++Iter )
		{
			const FString NodeType = FString::Printf( TEXT( "%s.%s" ), *Iter->Value.NodeClass.ToString(), *Iter->Key.ToString() ); 
			BPEditorNodeStats.Add( FAnalyticsEventAttribute( NodeType, Iter->Value.Instances ));
		}
		AnalyticsStats.CreatedNodeTypes.Reset();

		FEngineAnalytics::GetProvider().RecordEvent( FString( "Editor.Usage.BlueprintEditor" ), BPEditorAttribs );
		FEngineAnalytics::GetProvider().RecordEvent( FString( "Editor.Usage.BlueprintEditor.NodeCreation" ), BPEditorNodeStats );

		for (auto Iter = AnalyticsStats.GraphDisallowedPinConnections.CreateConstIterator(); Iter; ++Iter)
		{
			TArray< FAnalyticsEventAttribute > BPEditorPinConnectAttribs;
			BPEditorPinConnectAttribs.Add( FAnalyticsEventAttribute( FString( "FromPin.Category" ), Iter->PinTypeCategoryA ));
			BPEditorPinConnectAttribs.Add( FAnalyticsEventAttribute( FString( "FromPin.IsArray" ), Iter->bPinIsArrayA ));
			BPEditorPinConnectAttribs.Add( FAnalyticsEventAttribute( FString( "FromPin.IsReference" ), Iter->bPinIsReferenceA ));
			BPEditorPinConnectAttribs.Add( FAnalyticsEventAttribute( FString( "FromPin.IsWeakPointer" ), Iter->bPinIsWeakPointerA ));
			BPEditorPinConnectAttribs.Add( FAnalyticsEventAttribute( FString( "ToPin.Category" ), Iter->PinTypeCategoryB ));
			BPEditorPinConnectAttribs.Add( FAnalyticsEventAttribute( FString( "ToPin.IsArray" ), Iter->bPinIsArrayB ));
			BPEditorPinConnectAttribs.Add( FAnalyticsEventAttribute( FString( "ToPin.IsReference" ), Iter->bPinIsReferenceB ));
			BPEditorPinConnectAttribs.Add( FAnalyticsEventAttribute( FString( "ToPin.IsWeakPointer" ), Iter->bPinIsWeakPointerB ));
			BPEditorPinConnectAttribs.Add( FAnalyticsEventAttribute( FString( "ProjectId" ), ProjectID ) );

			FEngineAnalytics::GetProvider().RecordEvent( FString( "Editor.Usage.BPDisallowedPinConnection" ), BPEditorPinConnectAttribs );
		}
	}
}

void FBlueprintEditor::FocusInspectorOnGraphSelection(const FGraphPanelSelectionSet& NewSelection, bool bForceRefresh)
{
	if (NewSelection.Array().Num())
	{
		SKismetInspector::FShowDetailsOptions ShowDetailsOptions;
		ShowDetailsOptions.bForceRefresh = bForceRefresh;

		Inspector->ShowDetailsForObjects(NewSelection.Array(), ShowDetailsOptions);
	}
}

void FBlueprintEditor::CreateDefaultTabContents(const TArray<UBlueprint*>& InBlueprints)
{
	UBlueprint* InBlueprint = InBlueprints.Num() == 1 ? InBlueprints[0] : NULL;

	// Cache off whether or not this is an interface, since it is used to govern multiple widget's behavior
	const bool bIsInterface = (InBlueprint && InBlueprint->BlueprintType == BPTYPE_Interface);
	const bool bIsMacro = (InBlueprint && InBlueprint->BlueprintType == BPTYPE_MacroLibrary);

	if (InBlueprint)
	{
		this->DebuggingView =
			SNew(SKismetDebuggingView)
			. BlueprintToWatch(InBlueprint)
			. IsEnabled(!bIsInterface && !bIsMacro);

		this->Palette = 
			SNew(SBlueprintPalette, SharedThis(this))
				.IsEnabled(this, &FBlueprintEditor::InEditingMode);
	}

	FIsPropertyEditingEnabled IsPropertyEditingEnabledDelegate = FIsPropertyEditingEnabled::CreateSP(this, &FBlueprintEditor::IsPropertyEditingEnabled);

	if (IsEditingSingleBlueprint())
	{
		this->MyBlueprintWidget = SNew(SMyBlueprint, SharedThis(this));
	}
	
	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	CompilerResultsListing = MessageLogModule.CreateLogListing( "BlueprintEditorCompileResults" );
	CompilerResultsListing->OnMessageTokenClicked().AddSP(this, &FBlueprintEditor::OnLogTokenClicked);

	CompilerResults = 
		SNew(SBorder)
		.Padding(0.0f)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			MessageLogModule.CreateLogListingWidget( CompilerResultsListing.ToSharedRef() )
		];

	FindResults = SNew(SFindInBlueprints, SharedThis(this));

	this->Inspector = 
		SNew(SKismetInspector)
		. HideNameArea(true)
		. ViewIdentifier(FName("BlueprintInspector"))
		. Kismet2(SharedThis(this))
		. IsPropertyEditingEnabledDelegate( IsPropertyEditingEnabledDelegate )
		. OnFinishedChangingProperties( FOnFinishedChangingProperties::FDelegate::CreateSP(this, &FBlueprintEditor::OnFinishedChangingProperties) );

	if (InBlueprints.Num() > 0)
	{
		// Hide the inspector title for Persona, where we customize the defaults editor widget
		const bool bIsPersona = InBlueprint ? InBlueprint->IsA<UAnimBlueprint>() : InBlueprints[0]->IsA<UAnimBlueprint>();
		const bool bShowTitle = !bIsPersona;
		const bool bShowPublicView = true;
		const bool bHideNameArea = false;

		this->DefaultEditor = 
			SNew(SKismetInspector)
			. Kismet2(SharedThis(this))
			. ViewIdentifier(FName("BlueprintDefaults"))
			. IsEnabled(!bIsInterface)
			. ShowPublicViewControl(bShowPublicView)
			. ShowTitleArea(bShowTitle)
			. HideNameArea(bHideNameArea)
			. IsPropertyEditingEnabledDelegate( IsPropertyEditingEnabledDelegate )
			. OnFinishedChangingProperties( FOnFinishedChangingProperties::FDelegate::CreateSP( this, &FBlueprintEditor::OnFinishedChangingProperties ) );
	}

	if (InBlueprint && 
		InBlueprint->ParentClass &&
		InBlueprint->ParentClass->IsChildOf(AActor::StaticClass()) && 
		InBlueprint->SimpleConstructionScript )
	{
		SCSEditor = SNew(SSCSEditor, SharedThis(this), InBlueprint->SimpleConstructionScript);
		
		SCSViewport = SAssignNew(SCSViewport, SSCSEditorViewport) .BlueprintEditor(SharedThis(this));
	}
}

void FBlueprintEditor::OnLogTokenClicked(const TSharedRef<IMessageToken>& Token)
{
	if( Token->GetType() == EMessageToken::Object )
	{
		const TSharedRef<FUObjectToken> UObjectToken = StaticCastSharedRef<FUObjectToken>(Token);
		if(UObjectToken->GetObject().IsValid())
		{
			JumpToHyperlink(UObjectToken->GetObject().Get());
		}
	}
}

/** Create Default Commands **/
void FBlueprintEditor::CreateDefaultCommands()
{
	// Tell Kismet2 how to handle all the UI actions that it can handle
	// @todo: remove this once GraphEditorActions automatically register themselves.

	FGraphEditorCommands::Register();
	FBlueprintEditorCommands::Register();
	FFullBlueprintEditorCommands::Register();
	FMyBlueprintCommands::Register();
	FBlueprintSpawnNodeCommands::Register();

	ToolkitCommands->MapAction(
		FFullBlueprintEditorCommands::Get().Compile,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::Compile),
		FCanExecuteAction::CreateSP(this, &FBlueprintEditor::IsCompilingEnabled));
	
	ToolkitCommands->MapAction(
		FFullBlueprintEditorCommands::Get().SwitchToScriptingMode,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::SetCurrentMode, FBlueprintEditorApplicationModes::StandardBlueprintEditorMode),
		FCanExecuteAction::CreateSP(this, &FBlueprintEditor::IsEditingSingleBlueprint),
		FIsActionChecked::CreateSP(this, &FBlueprintEditor::IsModeCurrent, FBlueprintEditorApplicationModes::StandardBlueprintEditorMode));

	ToolkitCommands->MapAction(
		FFullBlueprintEditorCommands::Get().SwitchToBlueprintDefaultsMode,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::SetCurrentMode, FBlueprintEditorApplicationModes::BlueprintDefaultsMode),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FBlueprintEditor::IsModeCurrent, FBlueprintEditorApplicationModes::BlueprintDefaultsMode));
	
	ToolkitCommands->MapAction(
		FFullBlueprintEditorCommands::Get().SwitchToComponentsMode,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::SetCurrentMode, FBlueprintEditorApplicationModes::BlueprintComponentsMode),
		FCanExecuteAction::CreateSP(this, &FBlueprintEditor::CanAccessComponentsMode),
		FIsActionChecked::CreateSP(this, &FBlueprintEditor::IsModeCurrent, FBlueprintEditorApplicationModes::BlueprintComponentsMode));

	ToolkitCommands->MapAction(
		FFullBlueprintEditorCommands::Get().EditGlobalOptions,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::EditGlobalOptions_Clicked),
		FCanExecuteAction());

	// Edit menu actions
	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().FindInBlueprint,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::FindInBlueprint_Clicked),
		FCanExecuteAction::CreateSP( this, &FBlueprintEditor::IsInAScriptingMode )
		);

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().ReparentBlueprint,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::ReparentBlueprint_Clicked),
		FCanExecuteAction(),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FBlueprintEditor::ReparentBlueprint_IsVisible)
		);

	ToolkitCommands->MapAction( FGenericCommands::Get().Undo,
		FExecuteAction::CreateSP( this, &FBlueprintEditor::UndoGraphAction ),
		FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanUndoGraphAction )
		);

	ToolkitCommands->MapAction( FGenericCommands::Get().Redo,
		FExecuteAction::CreateSP( this, &FBlueprintEditor::RedoGraphAction ),
		FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanRedoGraphAction )
		);


	// View commands
	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().ZoomToWindow,
		FExecuteAction::CreateSP( this, &FBlueprintEditor::ZoomToWindow_Clicked ),
		FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanZoomToWindow )
		);

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().ZoomToSelection,
		FExecuteAction::CreateSP( this, &FBlueprintEditor::ZoomToSelection_Clicked ),
		FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanZoomToSelection )
		);

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().NavigateToParent,
		FExecuteAction::CreateSP( this, &FBlueprintEditor::NavigateToParentGraph_Clicked ),
		FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanNavigateToParentGraph )
		);

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().NavigateToParentBackspace,
		FExecuteAction::CreateSP( this, &FBlueprintEditor::NavigateToParentGraph_Clicked ),
		FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanNavigateToParentGraph )
		);

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().NavigateToChild,
		FExecuteAction::CreateSP( this, &FBlueprintEditor::NavigateToChildGraph_Clicked ),
		FCanExecuteAction::CreateSP( this, &FBlueprintEditor::CanNavigateToChildGraph )
		);

	ToolkitCommands->MapAction(FGraphEditorCommands::Get().ShowAllPins,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::SetPinVisibility, SGraphEditor::Pin_Show),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FBlueprintEditor::GetPinVisibility, SGraphEditor::Pin_Show));

	ToolkitCommands->MapAction(FGraphEditorCommands::Get().HideNoConnectionPins,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::SetPinVisibility, SGraphEditor::Pin_HideNoConnection),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FBlueprintEditor::GetPinVisibility, SGraphEditor::Pin_HideNoConnection));

	ToolkitCommands->MapAction(FGraphEditorCommands::Get().HideNoConnectionNoDefaultPins,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::SetPinVisibility, SGraphEditor::Pin_HideNoConnectionNoDefault),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FBlueprintEditor::GetPinVisibility, SGraphEditor::Pin_HideNoConnectionNoDefault));

	// Compile
	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().CompileBlueprint,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::Compile),
		FCanExecuteAction::CreateSP(this, &FBlueprintEditor::IsCompilingEnabled)
		);

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().RefreshAllNodes,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::RefreshAllNodes_OnClicked),
		FCanExecuteAction::CreateSP( this, &FBlueprintEditor::IsInAScriptingMode )
		);

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().DeleteUnusedVariables,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::DeleteUnusedVariables_OnClicked),
		FCanExecuteAction::CreateSP( this, &FBlueprintEditor::IsInAScriptingMode )
		);
	
	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().FindInBlueprints,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::FindInBlueprints_OnClicked)
		);

	// Debug actions
	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().ClearAllBreakpoints,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::ClearAllBreakpoints),
		FCanExecuteAction::CreateSP(this, &FBlueprintEditor::HasAnyBreakpoints)
		);

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().DisableAllBreakpoints,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::DisableAllBreakpoints),
		FCanExecuteAction::CreateSP(this, &FBlueprintEditor::HasAnyEnabledBreakpoints)
		);

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().EnableAllBreakpoints,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::EnableAllBreakpoints),
		FCanExecuteAction::CreateSP(this, &FBlueprintEditor::HasAnyDisabledBreakpoints)
		);

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().ClearAllWatches,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::ClearAllWatches),
		FCanExecuteAction::CreateSP(this, &FBlueprintEditor::HasAnyWatches)
		);

	// New document actions
	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().AddNewVariable,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::OnAddNewVariable),
		FCanExecuteAction::CreateSP(this, &FBlueprintEditor::InEditingMode),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FBlueprintEditor::NewDocument_IsVisibleForType, CGT_NewVariable));
	
	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().AddNewLocalVariable,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::OnAddNewLocalVariable),
		FCanExecuteAction::CreateSP(this, &FBlueprintEditor::InEditingMode),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FBlueprintEditor::NewDocument_IsVisibleForType, CGT_NewLocalVariable));

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().AddNewFunction,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::NewDocument_OnClicked, CGT_NewFunctionGraph),
		FCanExecuteAction::CreateSP(this, &FBlueprintEditor::InEditingMode),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FBlueprintEditor::NewDocument_IsVisibleForType, CGT_NewFunctionGraph));

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().AddNewEventGraph,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::NewDocument_OnClicked, CGT_NewEventGraph),
		FCanExecuteAction::CreateSP(this, &FBlueprintEditor::InEditingMode),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FBlueprintEditor::NewDocument_IsVisibleForType, CGT_NewEventGraph));

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().AddNewMacroDeclaration,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::NewDocument_OnClicked, CGT_NewMacroGraph),
		FCanExecuteAction::CreateSP(this, &FBlueprintEditor::InEditingMode),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FBlueprintEditor::NewDocument_IsVisibleForType, CGT_NewMacroGraph));

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().AddNewDelegate,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::OnAddNewDelegate),
		FCanExecuteAction::CreateSP(this, &FBlueprintEditor::InEditingMode),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FBlueprintEditor::AddNewDelegateIsVisible));

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().FindReferencesFromClass,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::OnListObjectsReferencedByClass),
		FCanExecuteAction());

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().FindReferencesFromBlueprint,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::OnListObjectsReferencedByBlueprint),
		FCanExecuteAction());

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().RepairCorruptedBlueprint,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::OnRepairCorruptedBlueprint),
		FCanExecuteAction());

	/*
	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().AddNewAnimationGraph,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::NewDocument_OnClicked, CGT_NewAnimationGraph),
		FCanExecuteAction(),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FBlueprintEditor::NewDocument_IsVisibleForType, CGT_NewAnimationGraph));
	*/

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().RecompileGraphEditor,
		FExecuteAction::CreateStatic( &FLocalKismetCallbacks::RecompileGraphEditor_OnClicked ),
		FCanExecuteAction::CreateStatic( &FLocalKismetCallbacks::CanRecompileModules ));
	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().RecompileKismetCompiler,
		FExecuteAction::CreateStatic( &FLocalKismetCallbacks::RecompileKismetCompiler_OnClicked ),
		FCanExecuteAction::CreateStatic( &FLocalKismetCallbacks::CanRecompileModules ));
	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().RecompileBlueprintEditor,
		FExecuteAction::CreateStatic( &FLocalKismetCallbacks::RecompileBlueprintEditor_OnClicked ),
		FCanExecuteAction::CreateStatic( &FLocalKismetCallbacks::CanRecompileModules ));
	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().RecompilePersona,
		FExecuteAction::CreateStatic( &FLocalKismetCallbacks::RecompilePersona_OnClicked ),
		FCanExecuteAction::CreateStatic( &FLocalKismetCallbacks::CanRecompileModules ));

	ToolkitCommands->MapAction( FBlueprintEditorCommands::Get().SaveIntermediateBuildProducts,
		FExecuteAction::CreateSP(this, &FBlueprintEditor::ToggleSaveIntermediateBuildProducts),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FBlueprintEditor::GetSaveIntermediateBuildProducts) );
}

void FBlueprintEditor::FindInBlueprint_Clicked()
{
	TabManager->InvokeTab(FBlueprintEditorTabs::FindResultsID);
	FindResults->FocusForUse(true);
}

void FBlueprintEditor::ReparentBlueprint_Clicked()
{
	TArray<UBlueprint*> Blueprints;
	for (int32 i = 0; i < GetEditingObjects().Num(); ++i)
	{
		auto Blueprint = Cast<UBlueprint>(GetEditingObjects()[i]);
		if (Blueprint) 
		{
			Blueprints.Add(Blueprint);
		}
	}
	FBlueprintEditorUtils::OpenReparentBlueprintMenu(Blueprints, GetToolkitHost()->GetParentWidget(), FOnClassPicked::CreateSP(this, &FBlueprintEditor::ReparentBlueprint_NewParentChosen));
}

void FBlueprintEditor::ReparentBlueprint_NewParentChosen(UClass* ChosenClass)
{
	UBlueprint* BlueprintObj = GetBlueprintObj();

	if ((BlueprintObj != NULL) && (ChosenClass != NULL) && (ChosenClass != BlueprintObj->ParentClass))
	{
		// If the chosen class differs hierarchically from the current class, warn that there may be data loss
		bool bReparent = true;
		if ( !ChosenClass->GetDefaultObject()->IsA( BlueprintObj->ParentClass ) )
		{
			const FText Title = LOCTEXT("ReparentTitle", "Reparent Blueprint"); 
			const FText Message = LOCTEXT("ReparentWarning", "Reparenting this blueprint may cause data loss.  Continue reparenting?"); 

			// Warn the user that this may result in data loss
			FSuppressableWarningDialog::FSetupInfo Info( Message, Title, "Warning_ReparentTitle" );
			Info.ConfirmText = LOCTEXT("ReparentYesButton", "Reparent");
			Info.CancelText = LOCTEXT("ReparentNoButton", "Cancel");
			Info.CheckBoxText = FText::GetEmpty();	// not suppressible

			FSuppressableWarningDialog ReparentBlueprintDlg( Info );
			if( ReparentBlueprintDlg.ShowModal() == FSuppressableWarningDialog::Cancel )
			{
				bReparent = false;
			}
		}

		if ( bReparent )
		{
			UE_LOG(LogBlueprint, Warning, TEXT("Reparenting blueprint %s from %s to %s..."), *BlueprintObj->GetFullName(), *BlueprintObj->ParentClass->GetName(), *ChosenClass->GetName());

			UClass* OldParentClass = BlueprintObj->ParentClass ;
			BlueprintObj->ParentClass = ChosenClass;

			// Ensure that the Blueprint is up-to-date (valid SCS etc.) before compiling
			EnsureBlueprintIsUpToDate(BlueprintObj);

			Compile();

			if (SCSEditor.IsValid())
			{
				SCSEditor->UpdateTree();
			}
		}
	}

	FSlateApplication::Get().DismissAllMenus();
}

bool FBlueprintEditor::ReparentBlueprint_IsVisible() const
{
	UBlueprint* Blueprint = GetBlueprintObj();
	if (Blueprint != NULL)
	{
		// Don't show the reparent option if it's an Interface or we're not in editing mode
		return !FBlueprintEditorUtils::IsInterfaceBlueprint(Blueprint) && InEditingMode();
	}
	else
	{
		return false;
	}
}

void FBlueprintEditor::EditGlobalOptions_Clicked()
{
	UBlueprint* Blueprint = GetBlueprintObj();
	if(Blueprint != NULL)
	{
		if (CurrentUISelection == FBlueprintEditor::GraphPanel)
		{
			ClearSelectionInAllEditors();
		}
		if (CurrentUISelection == FBlueprintEditor::MyBlueprint)
		{
			if (MyBlueprintWidget.IsValid())
			{
				MyBlueprintWidget->ClearGraphActionMenuSelection();
			}
		}
		CurrentUISelection = FBlueprintEditor::BlueprintProps;

		// Show details for the Blueprint instance we're editing
		Inspector->ShowDetailsForSingleObject(Blueprint);
	}
}

// Zooming to fit the entire graph
void FBlueprintEditor::ZoomToWindow_Clicked()
{
	if (SGraphEditor* GraphEd = FocusedGraphEdPtr.Pin().Get())
	{
		GraphEd->ZoomToFit(/*bOnlySelection=*/ false);
	}
}

bool FBlueprintEditor::CanZoomToWindow() const
{
	return FocusedGraphEdPtr.IsValid();
}

// Zooming to fit the current selection
void FBlueprintEditor::ZoomToSelection_Clicked()
{
	if (SGraphEditor* GraphEd = FocusedGraphEdPtr.Pin().Get())
	{
		GraphEd->ZoomToFit(/*bOnlySelection=*/ true);
	}
}

bool FBlueprintEditor::CanZoomToSelection() const
{
	return FocusedGraphEdPtr.IsValid();
}

// Navigating into/out of graphs
void FBlueprintEditor::NavigateToParentGraph_Clicked()
{
	if (FocusedGraphEdPtr.IsValid())
	{
		if (UEdGraph* ParentGraph = Cast<UEdGraph>(FocusedGraphEdPtr.Pin()->GetCurrentGraph()->GetOuter()))
		{
			OpenDocument(ParentGraph, FDocumentTracker::NavigatingCurrentDocument);
		}
	}
}

bool FBlueprintEditor::CanNavigateToParentGraph() const
{
	return FocusedGraphEdPtr.IsValid() && (FocusedGraphEdPtr.Pin()->GetCurrentGraph()->GetOuter()->IsA(UEdGraph::StaticClass()));
}

void FBlueprintEditor::NavigateToChildGraph_Clicked()
{
	if (FocusedGraphEdPtr.IsValid())
	{
		UEdGraph* CurrentGraph = FocusedGraphEdPtr.Pin()->GetCurrentGraph();

		if (CurrentGraph->SubGraphs.Num() > 0)
		{
			// Display a child jump list
			FSlateApplication::Get().PushMenu( 
				GetToolkitHost()->GetParentWidget(),
				SNew(SChildGraphPicker, CurrentGraph),
				FSlateApplication::Get().GetCursorPos(), // summon location
				FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup )
			);
		}
		else if (CurrentGraph->SubGraphs.Num() == 1)
		{
			// Jump immediately to the child if there is only one
			UEdGraph* ChildGraph = CurrentGraph->SubGraphs[0];
			OpenDocument(ChildGraph, FDocumentTracker::NavigatingCurrentDocument);
		}
	}
}

bool FBlueprintEditor::CanNavigateToChildGraph() const
{
	return FocusedGraphEdPtr.IsValid() && (FocusedGraphEdPtr.Pin()->GetCurrentGraph()->SubGraphs.Num() > 0);
}

void FBlueprintEditor::PostUndo(bool bSuccess)
{	
	// Clear selection, to avoid holding refs to nodes that go away
	if (bSuccess && GetBlueprintObj())
	{
		ClearSelectionInAllEditors();

		// Will cause a call to RefreshEditors()
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprintObj());

		TSharedPtr<class SSCSEditorViewport> ViewportPtr = GetSCSViewport();
		if (ViewportPtr.IsValid())
		{
			ViewportPtr->Invalidate();
		}

		FSlateApplication::Get().DismissAllMenus();
	}
}

void FBlueprintEditor::PostRedo(bool bSuccess)
{
	UBlueprint* BlueprintObj = GetBlueprintObj();
	if (BlueprintObj && bSuccess)
	{
		// Will cause a call to RefreshEditors()
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified( BlueprintObj );

		FSlateApplication::Get().DismissAllMenus();
	}
}

void FBlueprintEditor::AnalyticsTrackNewNode( FName NodeClass, FName NodeType )
{
	if( FAnalyticsStatistics::FNodeDetails* Result = AnalyticsStats.CreatedNodeTypes.Find( NodeType ))
	{
		Result->Instances++;
	}
	else
	{
		FAnalyticsStatistics::FNodeDetails NewEntry;
		NewEntry.NodeClass = NodeClass;
		NewEntry.Instances = 1;
		AnalyticsStats.CreatedNodeTypes.Add( NodeType, NewEntry );
	}
}

void FBlueprintEditor::UndoGraphAction()
{
	GEditor->UndoTransaction();
}

bool FBlueprintEditor::CanUndoGraphAction() const
{
	return !InDebuggingMode();
}

void FBlueprintEditor::RedoGraphAction()
{
	// Clear selection, to avoid holding refs to nodes that go away
	if (GetBlueprintObj())
	{
		ClearSelectionInAllEditors();

		GEditor->RedoTransaction();

		RefreshEditors();

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprintObj());
	}
}

bool FBlueprintEditor::CanRedoGraphAction() const
{
	return !InDebuggingMode();
}

void FBlueprintEditor::OnActiveTabChanged( TSharedPtr<SDockTab> PreviouslyActive, TSharedPtr<SDockTab> NewlyActivated )
{
	if (!NewlyActivated.IsValid())
	{
		TArray<UObject*> ObjArray;
		Inspector->ShowDetailsForObjects(ObjArray);
		//UE_LOG(LogBlueprint, Log, TEXT("OnActiveTabChanged: NONE"));
	}
	else
	{
		//UE_LOG(LogBlueprint, Log, TEXT("OnActiveTabChanged: %s"), *NewlyActivated->GetLayoutIdentifier().ToString());
	}
}

void FBlueprintEditor::OnGraphEditorFocused(const TSharedRef<SGraphEditor>& InGraphEditor)
{
	// Update the graph editor that is currently focused
	FocusedGraphEdPtr = InGraphEditor;
	InGraphEditor->SetPinVisibility(PinVisibility);

	// Update the inspector as well, to show selection from the focused graph editor
	FocusInspectorOnGraphSelection(GetSelectedNodes());

	// During undo, garbage graphs can be temporarily brought into focus, ensure that before a refresh of the MyBlueprint window that the graph is owned by a Blueprint
	if(FocusedGraphEdPtr.IsValid() && MyBlueprintWidget.IsValid())
	{
		// The focused graph can be garbage as well
		TWeakObjectPtr< UEdGraph > FocusedGraph = FocusedGraphEdPtr.Pin()->GetCurrentGraph();
		if(FocusedGraph.IsValid() && FBlueprintEditorUtils::FindBlueprintForGraph(FocusedGraph.Get()))
		{
			MyBlueprintWidget->Refresh();
		}
	}
}

void FBlueprintEditor::OnGraphEditorDropActor(const TArray< TWeakObjectPtr<AActor> >& Actors, UEdGraph* Graph, const FVector2D& DropLocation)
{
	// First we need to check that the dropped actor is in the right sublevel for the reference
	ULevelScriptBlueprint* LevelBlueprint = Cast<ULevelScriptBlueprint>(GetBlueprintObj());
	if (LevelBlueprint != NULL)
	{
		ULevel* BlueprintLevel = LevelBlueprint->GetLevel();

		for (int32 i = 0; i < Actors.Num(); i++)
		{
			AActor* DroppedActor = Actors[i].Get();
			if ((DroppedActor != NULL) && (DroppedActor->GetLevel() == BlueprintLevel))
			{
				UK2Node_Literal* LiteralNodeTemplate = NewObject<UK2Node_Literal>();
				LiteralNodeTemplate->SetObjectRef(DroppedActor);

				const FVector2D NodeLocation = DropLocation + (i * FVector2D(0,30));
				FEdGraphSchemaAction_K2NewNode::SpawnNodeFromTemplate<UK2Node_Literal>(Graph, LiteralNodeTemplate, NodeLocation);
			}
		}
	}
}

void FBlueprintEditor::OnGraphEditorDropStreamingLevel(const TArray< TWeakObjectPtr<ULevelStreaming> >& Levels, UEdGraph* Graph, const FVector2D& DropLocation)
{
	UFunction* TargetFunc = UGameplayStatics::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetStreamingLevel));
	check(TargetFunc);

	for (int32 i = 0; i < Levels.Num(); i++)
	{
		ULevelStreaming* DroppedLevel = Levels[i].Get();
		if ((DroppedLevel != NULL) && 
			(DroppedLevel->IsA(ULevelStreamingKismet::StaticClass()))) 
		{
			const FVector2D NodeLocation = DropLocation + (i * FVector2D(0,80));
				
			UK2Node_CallFunction* NodeTemplate = NewObject<UK2Node_CallFunction>();
			UK2Node_CallFunction* Node = FEdGraphSchemaAction_K2NewNode::SpawnNodeFromTemplate<UK2Node_CallFunction>(Graph, NodeTemplate, NodeLocation);
			Node->SetFromFunction(TargetFunc);
						
			// Set dropped level package name
			Node->AllocateDefaultPins();
			UEdGraphPin* PackageNameInputPin = Node->FindPinChecked(TEXT("PackageName"));
			PackageNameInputPin->DefaultValue = DroppedLevel->PackageName.ToString();
		}
	}
}

FActionMenuContent FBlueprintEditor::OnCreateGraphActionMenu(UEdGraph* InGraph, const FVector2D& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed)
{
	TSharedRef<SBlueprintActionMenu> ActionMenu = 
		SNew(SBlueprintActionMenu, SharedThis(this))
		.GraphObj(InGraph)
		.NewNodePosition(InNodePosition)
		.DraggedFromPins(InDraggedPins)
		.AutoExpandActionMenu(bAutoExpand)
		.OnClosedCallback(InOnMenuClosed)
		.OnCloseReason(this, &FBlueprintEditor::OnGraphActionMenuClosed);

	return FActionMenuContent( ActionMenu, ActionMenu->GetFilterTextBox() );
}

void FBlueprintEditor::OnGraphActionMenuClosed(bool bActionExecuted, bool bContextSensitiveChecked, bool bGraphPinContext)
{
	if (bActionExecuted)
	{
		bContextSensitiveChecked ? AnalyticsStats.GraphActionMenusCtxtSensitiveExecCount++ : AnalyticsStats.GraphActionMenusNonCtxtSensitiveExecCount++;
		UpdateNodeCreationStats( bGraphPinContext ? ENodeCreateAction::PinContext : ENodeCreateAction::GraphContext );
	}
	else
	{
		AnalyticsStats.GraphActionMenusCancelledCount++;
	}
}

void FBlueprintEditor::OnSelectedNodesChanged(const FGraphPanelSelectionSet& NewSelection)
{
	if (CurrentUISelection == FBlueprintEditor::MyBlueprint)
	{
		// clear MyBlueprint selection
		if (MyBlueprintWidget.IsValid())
		{
			MyBlueprintWidget->ClearGraphActionMenuSelection();
		}
	}
	CurrentUISelection = NewSelection.Num() > 0 ? FBlueprintEditor::GraphPanel : FBlueprintEditor::NoSelection;
	Inspector->ShowDetailsForObjects(NewSelection.Array());
}

void FBlueprintEditor::OnBlueprintChanged(UBlueprint* InBlueprint)
{
	if (InBlueprint)
	{
		// Refresh the graphs
		RefreshEditors();

		// Notify that the blueprint has been changed (update Content browser, etc)
		InBlueprint->PostEditChange();

		// Call PostEditChange() on any Actors that are based on this Blueprint
		FBlueprintEditorUtils::PostEditChangeBlueprintActors(InBlueprint);

		// In case objects were deleted, which should close the tab
		if (GetCurrentMode() == FBlueprintEditorApplicationModes::StandardBlueprintEditorMode)
		{
			SaveEditedObjectState();
		}
	}
}

void FBlueprintEditor::Compile()
{
	if (GetBlueprintObj())
	{
		FMessageLog BlueprintLog("BlueprintLog");

		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("BlueprintName"), FText::FromString(GetBlueprintObj()->GetName()));
		BlueprintLog.NewPage(FText::Format(LOCTEXT("CompilationPageLabel", "Compile {BlueprintName}"), Arguments));

		FCompilerResultsLog LogResults;
		FKismetEditorUtilities::CompileBlueprint(GetBlueprintObj(), false, false, bSaveIntermediateBuildProducts, &LogResults);

		bool bForceMessageDisplay = ((LogResults.NumWarnings > 0) || (LogResults.NumErrors > 0)) && !GetBlueprintObj()->bIsRegeneratingOnLoad;
		DumpMessagesToCompilerLog(LogResults.Messages, bForceMessageDisplay);
	}
}

FReply FBlueprintEditor::Compile_OnClickWithReply()
{
	Compile();
	return FReply::Handled();
}

void FBlueprintEditor::RefreshAllNodes_OnClicked()
{
	FBlueprintEditorUtils::RefreshAllNodes(GetBlueprintObj());
	RefreshEditors();
	Compile();
}

void FBlueprintEditor::DeleteUnusedVariables_OnClicked()
{
	auto BlueprintObj = GetBlueprintObj();
	
	bool bHasAtLeastOneVariableToCheck = false;
	FString PropertyList;
	TArray<FName> VariableNames;
	for (TFieldIterator<UProperty> PropertyIt(BlueprintObj->SkeletonGeneratedClass, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
	{
		UProperty* Property = *PropertyIt;
		// Don't show delegate properties, there is special handling for these
		const bool bDelegateProp = Property->IsA(UDelegateProperty::StaticClass()) || Property->IsA(UMulticastDelegateProperty::StaticClass());
		const bool bShouldShowProp = (!Property->HasAnyPropertyFlags(CPF_Parm) && Property->HasAllPropertyFlags(CPF_BlueprintVisible) && !bDelegateProp);

		if (bShouldShowProp)
		{
			bHasAtLeastOneVariableToCheck = true;
			FName VarName = Property->GetFName();
			
			const int32 VarInfoIndex = FBlueprintEditorUtils::FindNewVariableIndex(BlueprintObj, VarName);
			const bool bHasVarInfo = (VarInfoIndex != INDEX_NONE);
			
			const UObjectPropertyBase* ObjectProperty = Cast<const UObjectPropertyBase>(Property);
			bool bIsTimeline = ObjectProperty &&
				ObjectProperty->PropertyClass &&
				ObjectProperty->PropertyClass->IsChildOf(UTimelineComponent::StaticClass());
			if (!FBlueprintEditorUtils::IsVariableUsed(BlueprintObj, VarName) && !bIsTimeline && bHasVarInfo)
			{
				VariableNames.Add(Property->GetFName());
				if (PropertyList.IsEmpty()) {PropertyList = UEditorEngine::GetFriendlyName(Property);}
				else {PropertyList += FString::Printf(TEXT(", %s"), *UEditorEngine::GetFriendlyName(Property));}
			}
		}
	}

	if (VariableNames.Num() > 0)
	{
		FBlueprintEditorUtils::BulkRemoveMemberVariables(GetBlueprintObj(), VariableNames);
		LogSimpleMessage( FText::Format( LOCTEXT("UnusedVariablesDeletedMessage", "The following variable(s) were deleted successfully: {0}."), FText::FromString( PropertyList ) ) );
	}
	else if (bHasAtLeastOneVariableToCheck)
	{
		LogSimpleMessage(LOCTEXT("AllVariablesInUseMessage", "All variables are currently in use."));
	}
	else
	{
		LogSimpleMessage(LOCTEXT("NoVariablesToSeeMessage", "No variables to check for."));
	}
}

void FBlueprintEditor::FindInBlueprints_OnClicked()
{
	TabManager->InvokeTab(FBlueprintEditorTabs::FindResultsID);
	FindResults->FocusForUse(false);
}

void FBlueprintEditor::ClearAllBreakpoints()
{
	FDebuggingActionCallbacks::ClearBreakpoints(GetBlueprintObj());
}

void FBlueprintEditor::DisableAllBreakpoints()
{
	FDebuggingActionCallbacks::SetEnabledOnAllBreakpoints(GetBlueprintObj(), false);
}

void FBlueprintEditor::EnableAllBreakpoints()
{
	FDebuggingActionCallbacks::SetEnabledOnAllBreakpoints(GetBlueprintObj(), true);
}

void FBlueprintEditor::ClearAllWatches()
{
	FDebuggingActionCallbacks::ClearWatches(GetBlueprintObj());
}

bool FBlueprintEditor::HasAnyBreakpoints() const
{
	return GetBlueprintObj() && GetBlueprintObj()->Breakpoints.Num() > 0;
}

bool FBlueprintEditor::HasAnyEnabledBreakpoints() const
{
	if (!IsEditingSingleBlueprint()) {return false;}

	for (TArray<UBreakpoint*>::TIterator BreakpointIt(GetBlueprintObj()->Breakpoints); BreakpointIt; ++BreakpointIt)
	{
		UBreakpoint* BP = *BreakpointIt;
		if (BP->IsEnabledByUser())
		{
			return true;
		}
	}

	return false;
}

bool FBlueprintEditor::HasAnyDisabledBreakpoints() const
{
	if (!IsEditingSingleBlueprint()) {return false;}

	for (TArray<UBreakpoint*>::TIterator BreakpointIt(GetBlueprintObj()->Breakpoints); BreakpointIt; ++BreakpointIt)
	{
		UBreakpoint* BP = *BreakpointIt;
		if (!BP->IsEnabledByUser())
		{
			return true;
		}
	}

	return false;
}

bool FBlueprintEditor::HasAnyWatches() const
{
	return GetBlueprintObj() && GetBlueprintObj()->PinWatches.Num() > 0;
}

// Jumps to a hyperlinked node, pin, or graph, if it belongs to this blueprint
void FBlueprintEditor::JumpToHyperlink(const UObject* ObjectReference, bool bRequestRename)
{
	SetCurrentMode(FBlueprintEditorApplicationModes::StandardBlueprintEditorMode);

	if (const UEdGraphNode* Node = Cast<const UEdGraphNode>(ObjectReference))
	{
		if( bRequestRename )
		{
			IsNodeTitleVisible(Node, bRequestRename);
		}
		else
		{
			JumpToNode(Node, false);
		}
	}
	else if (const UEdGraphPin* Pin = Cast<const UEdGraphPin>(ObjectReference))
	{
		JumpToPin(Pin);
	}
	else if (const UEdGraph* Graph = Cast<const UEdGraph>(ObjectReference))
	{
		// Only ubergraphs should re-use the current tab
		if(Graph->GetSchema()->GetGraphType(Graph) == GT_Ubergraph || Cast<UK2Node_Composite>(Graph->GetOuter()))
		{
			OpenDocument(const_cast<UEdGraph*>(Graph), FDocumentTracker::NavigatingCurrentDocument);
		}
		else
		{
			OpenDocument(const_cast<UEdGraph*>(Graph), FDocumentTracker::OpenNewDocument);
		}
	}
	else if (const AActor* ReferencedActor = Cast<const AActor>(ObjectReference))
	{
		// Select the in-level actors referred to by this node
		GEditor->SelectNone(false, false);
		GEditor->SelectActor(const_cast<AActor*>(ReferencedActor), true, true, true);

		// Point the camera at it
		GUnrealEd->Exec( ReferencedActor->GetWorld(), TEXT("CAMERA ALIGN ACTIVEVIEWPORTONLY"));
	}
	else
	{
		UE_LOG(LogBlueprint, Warning, TEXT("Unknown type of hyperlinked object"));
	}

	//@TODO: Hacky way to ensure a message is seen when hitting an exception and doing intraframe debugging
	const FText ExceptionMessage = FKismetDebugUtilities::GetAndClearLastExceptionMessage();
	if (!ExceptionMessage.IsEmpty())
	{
		LogSimpleMessage( ExceptionMessage );
	}
}

void FBlueprintEditor::AddReferencedObjects( FReferenceCollector& Collector )
{
	for (int32 i = 0; i < GetEditingObjects().Num(); ++i)
	{
		UObject* EditingObject = GetEditingObjects()[i];
		Collector.AddReferencedObject(EditingObject);
	}
	for (int32 Index = 0; Index < StandardLibraries.Num(); Index++)
	{
		Collector.AddReferencedObject(StandardLibraries[Index]);
	}
	for (int32 Index = 0; Index < UserDefinedEnumerators.Num();)
	{
		if(UObject* EnumObj = UserDefinedEnumerators[Index].Get(false))
		{
			Collector.AddReferencedObject(EnumObj);
			Index++;
		}
		else
		{
			UserDefinedEnumerators.RemoveAtSwap(Index);
		}
	}
}

bool FBlueprintEditor::IsNodeTitleVisible(const UEdGraphNode* Node, bool bRequestRename)
{
	TSharedPtr<SGraphEditor> GraphEditor;
	if(bRequestRename)
	{
		// If we are renaming, the graph will be open already, just grab the tab and it's content and jump to the node.
		TSharedPtr<SDockTab> ActiveTab = DocumentManager->GetActiveTab();
		check(ActiveTab.IsValid());
		GraphEditor = StaticCastSharedRef<SGraphEditor>(ActiveTab->GetContent());
	}
	else
	{
		// Open a graph editor and jump to the node
		GraphEditor = OpenGraphAndBringToFront(Node->GetGraph());
	}

	bool bVisible = false;
	if (GraphEditor.IsValid())
	{
		bVisible = GraphEditor->IsNodeTitleVisible(Node, bRequestRename);
	}
	return bVisible;
}

void FBlueprintEditor::JumpToNode(const UEdGraphNode* Node, bool bRequestRename)
{
	TSharedPtr<SGraphEditor> GraphEditor;
	if(bRequestRename)
	{
		// If we are renaming, the graph will be open already, just grab the tab and it's content and jump to the node.
		TSharedPtr<SDockTab> ActiveTab = DocumentManager->GetActiveTab();
		check(ActiveTab.IsValid());
		GraphEditor = StaticCastSharedRef<SGraphEditor>(ActiveTab->GetContent());
	}
	else
	{
		// Open a graph editor and jump to the node
		GraphEditor = OpenGraphAndBringToFront(Node->GetGraph());
	}

	if (GraphEditor.IsValid())
	{
		GraphEditor->JumpToNode(Node, bRequestRename);
	}
}

void FBlueprintEditor::JumpToPin(const UEdGraphPin* Pin)
{
	if( !Pin->IsPendingKill() )
	{
		// Open a graph editor and jump to the pin
		TSharedPtr<SGraphEditor> GraphEditor = OpenGraphAndBringToFront(Pin->GetOwningNode()->GetGraph());
		if (GraphEditor.IsValid())
		{
			GraphEditor->JumpToPin(Pin);
		}
	}
}

UBlueprint* FBlueprintEditor::GetBlueprintObj() const
{
	return GetEditingObjects().Num() == 1 ? Cast<UBlueprint>(GetEditingObjects()[0]) : NULL;
}

bool FBlueprintEditor::IsEditingSingleBlueprint() const
{
	return GetBlueprintObj() != NULL;
}

FString FBlueprintEditor::GetDocumentationLink() const
{
	UBlueprint* Blueprint = GetBlueprintObj();
	if(Blueprint)
	{
		// Jump to more relevant docs if editing macro library or interface
		if(Blueprint->BlueprintType == BPTYPE_MacroLibrary)
		{
			return TEXT("Engine/Blueprints/UserGuide/Types/MacroLibrary");
		}
		else if (Blueprint->BlueprintType == BPTYPE_Interface)
		{
			return TEXT("Engine/Blueprints/UserGuide/Types/Interface");
		}
	}

	return FString(TEXT("Engine/Blueprints"));
}


bool FBlueprintEditor::CanAccessComponentsMode() const
{
	bool bCanAccess = false;

	// Ensure that we're editing a Blueprint
	if(IsEditingSingleBlueprint())
	{
		UBlueprint* Blueprint = GetBlueprintObj();
		bCanAccess = Blueprint->SimpleConstructionScript != NULL		// An SCS must be present (otherwise there is nothing valid to edit)
			&& FBlueprintEditorUtils::IsActorBased(Blueprint)			// Must be parented to an AActor-derived class (some older BPs may have an SCS but may not be Actor-based)
			&& Blueprint->BlueprintType != BPTYPE_MacroLibrary;			// Must not be a macro-type Blueprint
	}
	
	return bCanAccess;
}

void FBlueprintEditor::RegisterToolbarTab(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(TabManager);
}

void FBlueprintEditor::LogSimpleMessage(const FText& MessageText)
{
	FNotificationInfo Info( MessageText );
	Info.ExpireDuration = 3.0f;
	Info.bUseLargeFont = false;
	TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
	if ( Notification.IsValid() )
	{
		Notification->SetCompletionState( SNotificationItem::CS_Fail );
	}
}

void FBlueprintEditor::DumpMessagesToCompilerLog(const TArray<TSharedRef<FTokenizedMessage>>& Messages, bool bForceMessageDisplay)
{
	CompilerResultsListing->ClearMessages();
	CompilerResultsListing->AddMessages(Messages);
	
	if (!bEditorMarkedAsClosed && bForceMessageDisplay && GetCurrentMode() == FBlueprintEditorApplicationModes::StandardBlueprintEditorMode)
	{
		TabManager->InvokeTab(FBlueprintEditorTabs::CompilerResultsID);
	}
}

void FBlueprintEditor::DoPromoteToVariable( UBlueprint* InBlueprint, UEdGraphPin* InTargetPin )
{
	UEdGraphNode* PinNode = InTargetPin->GetOwningNode();
	check(PinNode);
	UEdGraph* GraphObj = PinNode->GetGraph();
	check(GraphObj);

	const FScopedTransaction Transaction( LOCTEXT("PromoteToVariable", "Promote To Variable") );
	InBlueprint->Modify();
	GraphObj->Modify();

	FString VarNameString = TEXT("NewVar");
	FName VarName = FName(*VarNameString);

	// Make sure the new name is valid
	TSharedPtr<INameValidatorInterface> NameValidator = MakeShareable(new FKismetNameValidator(InBlueprint));
	int32 Index = 0;
	while (NameValidator->IsValid(VarName) != Ok)
	{
		VarName = FName(*FString::Printf(TEXT("%s%i"), *VarNameString, Index));
		++Index;
	}

	const bool bWasSuccessful = FBlueprintEditorUtils::AddMemberVariable( GetBlueprintObj(), VarName, InTargetPin->PinType, InTargetPin->GetDefaultAsString() );

	if (bWasSuccessful)
	{
		// Create the new setter node
		FEdGraphSchemaAction_K2NewNode NodeInfo;

		// Create get or set node, depending on whether we clicked on an input or output pin
		UK2Node_Variable* TemplateNode = NULL;
		if (InTargetPin->Direction == EGPD_Input)
		{
			TemplateNode = NewObject<UK2Node_VariableGet>();
		}
		else
		{
			TemplateNode = NewObject<UK2Node_VariableSet>();
		}

		TemplateNode->VariableReference.SetSelfMember(VarName);
		NodeInfo.NodeTemplate = TemplateNode;

		// Set position of new node to be close to node we clicked on
		FVector2D NewNodePos;
		NewNodePos.X = (InTargetPin->Direction == EGPD_Input) ? PinNode->NodePosX - 200 : PinNode->NodePosX + 400;
		NewNodePos.Y = PinNode->NodePosY;

		NodeInfo.PerformAction(GraphObj, InTargetPin, NewNodePos, false);

		RenameNewlyAddedAction(VarName);
	}
}

void FBlueprintEditor::OnPromoteToVariable()
{
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (FocusedGraphEd.IsValid())
	{
		UEdGraphPin* TargetPin = FocusedGraphEd->GetGraphPinForMenu();

		check(IsEditingSingleBlueprint());
		check(GetBlueprintObj()->SkeletonGeneratedClass);
		check(TargetPin);

		DoPromoteToVariable( GetBlueprintObj(), TargetPin );
	}
}

bool FBlueprintEditor::CanPromoteToVariable() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	bool bCanPromote = false;
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (FocusedGraphEd.IsValid())
	{
		if (UEdGraphPin* Pin = FocusedGraphEd->GetGraphPinForMenu())
		{
			bCanPromote = K2Schema->CanPromotePinToVariable(*Pin);
		}
	}
	
	return bCanPromote;
}

void FBlueprintEditor::OnAddExecutionPin()
{
	const FGraphPanelSelectionSet& SelectedNodes = GetSelectedNodes();
	
	// Iterate over all nodes, and add the pin
	for (FGraphPanelSelectionSet::TConstIterator It(SelectedNodes); It; ++It)
	{
		UK2Node_ExecutionSequence* SeqNode = Cast<UK2Node_ExecutionSequence>(*It);
		if (SeqNode != NULL)
		{
			const FScopedTransaction Transaction( LOCTEXT("AddExecutionPin", "Add Execution Pin") );
			SeqNode->Modify();

			SeqNode->AddPinToExecutionNode();

			const UEdGraphSchema* Schema = SeqNode->GetSchema();
			Schema->ReconstructNode(*SeqNode);
		}
		else if (UK2Node_Switch* SwitchNode = Cast<UK2Node_Switch>(*It))
		{
			const FScopedTransaction Transaction( LOCTEXT("AddExecutionPin", "Add Execution Pin") );
			SwitchNode->Modify();

			SwitchNode->AddPinToSwitchNode();

			const UEdGraphSchema* Schema = SwitchNode->GetSchema();
			Schema->ReconstructNode(*SwitchNode);
		}
	}

	// Refresh the current graph, so the pins can be updated
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (FocusedGraphEd.IsValid())
	{
		FocusedGraphEd->NotifyGraphChanged();
	}
}

bool FBlueprintEditor::CanAddExecutionPin() const
{
	return true;
}

void  FBlueprintEditor::OnRemoveExecutionPin()
{
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (FocusedGraphEd.IsValid())
	{
		const FScopedTransaction Transaction( LOCTEXT("RemoveExecutionPin", "Remove Execution Pin") );

		UEdGraphPin* SelectedPin = FocusedGraphEd->GetGraphPinForMenu();
		UEdGraphNode* OwningNode = SelectedPin->GetOwningNode();

		OwningNode->Modify();
		SelectedPin->Modify();

		if (UK2Node_ExecutionSequence* SeqNode = Cast<UK2Node_ExecutionSequence>(OwningNode))
		{
			SeqNode->RemovePinFromExecutionNode(SelectedPin);
		}
		else if (UK2Node_Switch* SwitchNode = Cast<UK2Node_Switch>(OwningNode))
		{
			SwitchNode->RemovePinFromSwitchNode(SelectedPin);
		}

		// Update the graph so that the node will be refreshed
		FocusedGraphEd->NotifyGraphChanged();
	
		UEdGraph* CurrentGraph = FocusedGraphEd->GetCurrentGraph();
		UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(CurrentGraph);
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}
}

bool FBlueprintEditor::CanRemoveExecutionPin() const
{
	bool bReturnValue = true;

	const FGraphPanelSelectionSet& SelectedNodes = GetSelectedNodes();

	// Iterate over all nodes, and make sure all execution sequence nodes will always have at least 2 outs
	for (FGraphPanelSelectionSet::TConstIterator It(SelectedNodes); It; ++It)
	{
		UK2Node_ExecutionSequence* SeqNode = Cast<UK2Node_ExecutionSequence>(*It);
		if (SeqNode != NULL)
		{
			bReturnValue = SeqNode->CanRemoveExecutionPin();
			if (!bReturnValue)
			{
				break;
			}
		}
	}

	return bReturnValue;
}

void FBlueprintEditor::OnAddOptionPin()
{
	const FGraphPanelSelectionSet& SelectedNodes = GetSelectedNodes();

	// Iterate over all nodes, and add the pin
	for (FGraphPanelSelectionSet::TConstIterator It(SelectedNodes); It; ++It)
	{
		UK2Node_Select* SeqNode = Cast<UK2Node_Select>(*It);
		if (SeqNode != NULL)
		{
			const FScopedTransaction Transaction( LOCTEXT("AddOptionPin", "Add Option Pin") );
			SeqNode->Modify();

			SeqNode->AddOptionPinToNode();

			const UEdGraphSchema* Schema = SeqNode->GetSchema();
			Schema->ReconstructNode(*SeqNode);
		}
	}

	// Refresh the current graph, so the pins can be updated
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (FocusedGraphEd.IsValid())
	{
		FocusedGraphEd->NotifyGraphChanged();
	}
}

bool FBlueprintEditor::CanAddOptionPin() const
{
	const FGraphPanelSelectionSet& SelectedNodes = GetSelectedNodes();

	// Iterate over all nodes, and see if all can have a pin removed
	for (FGraphPanelSelectionSet::TConstIterator It(SelectedNodes); It; ++It)
	{
		UK2Node_Select* SeqNode = Cast<UK2Node_Select>(*It);
		// There's a bad node so return false
		if (SeqNode == NULL || !SeqNode->CanAddOptionPinToNode())
		{
			return false;
		}
	}

	return true;
}

void FBlueprintEditor::OnRemoveOptionPin()
{
	const FGraphPanelSelectionSet& SelectedNodes = GetSelectedNodes();

	// Iterate over all nodes, and add the pin
	for (FGraphPanelSelectionSet::TConstIterator It(SelectedNodes); It; ++It)
	{
		UK2Node_Select* SeqNode = Cast<UK2Node_Select>(*It);
		if (SeqNode != NULL)
		{
			const FScopedTransaction Transaction( LOCTEXT("RemoveOptionPin", "Remove Option Pin") );
			SeqNode->Modify();

			SeqNode->RemoveOptionPinToNode();

			const UEdGraphSchema* Schema = SeqNode->GetSchema();
			Schema->ReconstructNode(*SeqNode);
		}
	}

	// Refresh the current graph, so the pins can be updated
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (FocusedGraphEd.IsValid())
	{
		FocusedGraphEd->NotifyGraphChanged();
	}
}

bool FBlueprintEditor::CanRemoveOptionPin() const
{
	const FGraphPanelSelectionSet& SelectedNodes = GetSelectedNodes();

	// Iterate over all nodes, and see if all can have a pin removed
	for (FGraphPanelSelectionSet::TConstIterator It(SelectedNodes); It; ++It)
	{
		UK2Node_Select* SeqNode = Cast<UK2Node_Select>(*It);
		// There's a bad node so return false
		if (SeqNode == NULL || !SeqNode->CanRemoveOptionPinToNode())
		{
			return false;
		}
		// If this node doesn't have at least 3 options return false (need at least 2)
		else
		{
			TArray<UEdGraphPin*> OptionPins;
			SeqNode->GetOptionPins(OptionPins);
			if (OptionPins.Num() <= 2)
			{
				return false;
			}
		}
	}

	return true;
}

void FBlueprintEditor::OnChangePinType()
{
	if (UEdGraphPin* Pin = GetCurrentlySelectedPin())
	{
		TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
		if (FocusedGraphEd.IsValid())
		{
			UEdGraphPin* SelectedPin = FocusedGraphEd->GetGraphPinForMenu();
			if (SelectedPin)
			{
				const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

				// If this is the index node of the select node, we need to use the index list of types
				UK2Node_Select* SelectNode = Cast<UK2Node_Select>(SelectedPin->GetOwningNode());
				if (SelectNode && SelectNode->GetIndexPin() == SelectedPin)
				{
					TSharedRef<SCompoundWidget> PinChange = SNew(SPinTypeSelector, FGetPinTypeTree::CreateUObject(Schema, &UEdGraphSchema_K2::GetVariableIndexTypeTree))
						.TargetPinType(this, &FBlueprintEditor::OnGetPinType, SelectedPin)
						.OnPinTypeChanged(this, &FBlueprintEditor::OnChangePinTypeFinished, SelectedPin)
						.Schema(Schema)
						.bAllowExec(false)
						.IsEnabled(true)
						.bAllowArrays(false);

					PinTypeChangePopupWindow = FSlateApplication::Get().PushMenu( 
						GetToolkitHost()->GetParentWidget(), // Parent widget should be k2 not the menu thats open or it will be closed when the menu is dismissed
						PinChange,
						FSlateApplication::Get().GetCursorPos(), // summon location
						FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup)
						);
				}
				else
				{
					TSharedRef<SCompoundWidget> PinChange = SNew(SPinTypeSelector, FGetPinTypeTree::CreateUObject(Schema, &UEdGraphSchema_K2::GetVariableTypeTree))
						.TargetPinType(this, &FBlueprintEditor::OnGetPinType, SelectedPin)
						.OnPinTypeChanged(this, &FBlueprintEditor::OnChangePinTypeFinished, SelectedPin)
						.Schema(Schema)
						.bAllowExec(false)
						.IsEnabled(true)
						.bAllowArrays(false);

					PinTypeChangePopupWindow = FSlateApplication::Get().PushMenu( 
						GetToolkitHost()->GetParentWidget(), // Parent widget should be k2 not the menu thats open or it will be closed when the menu is dismissed
						PinChange,
						FSlateApplication::Get().GetCursorPos(), // summon location
						FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup)
						);
				}
			}
		}
	}
}

FEdGraphPinType FBlueprintEditor::OnGetPinType(UEdGraphPin* SelectedPin) const
{
	return SelectedPin->PinType;
}

void FBlueprintEditor::OnChangePinTypeFinished(const FEdGraphPinType& PinType, UEdGraphPin* InSelectedPin)
{
	InSelectedPin->PinType = PinType;
	if (UEdGraphPin* SelectedPin = GetCurrentlySelectedPin())
	{
		if (UK2Node_Select* SelectNode = Cast<UK2Node_Select>(SelectedPin->GetOwningNode()))
		{
			SelectNode->ChangePinType(SelectedPin);
		}
	}

	if (PinTypeChangePopupWindow.IsValid())
	{
		PinTypeChangePopupWindow.Pin()->RequestDestroyWindow();
	}
}

bool FBlueprintEditor::CanChangePinType() const
{
	if (UEdGraphPin* Pin = GetCurrentlySelectedPin())
	{
		if (UK2Node_Select* SelectNode = Cast<UK2Node_Select>(Pin->GetOwningNode()))
		{
			return SelectNode->CanChangePinType(Pin);
		}
	}
	return false;
}

struct FFunctionFromNodeHelper
{
	UFunction* Function;
	UK2Node* Node;
	FFunctionFromNodeHelper(UObject* SelectedObj) : Function(NULL), Node(NULL)
	{
		Node = Cast<UK2Node>(SelectedObj);
		UBlueprint* Blueprint = Node ? Node->GetBlueprint() : NULL;
		const UClass* SearchScope = Blueprint ? Blueprint->SkeletonGeneratedClass : NULL;
		if(SearchScope)
		{
			if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
			{
				Function = SearchScope->FindFunctionByName(EventNode->EventSignatureName);
			}
			else if(UK2Node_FunctionEntry* FunctionNode = Cast<UK2Node_FunctionEntry>(Node))
			{
				const FName FunctionName = (FunctionNode->CustomGeneratedFunctionName != NAME_None) ? FunctionNode->CustomGeneratedFunctionName : FunctionNode->GetGraph()->GetFName();
				Function = SearchScope->FindFunctionByName(FunctionName);
			}
		}
	}
};

void FBlueprintEditor::OnAddParentNode()
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	const FGraphPanelSelectionSet& SelectedNodes = GetSelectedNodes();
	if( SelectedNodes.Num() == 1 )
	{
		UObject* SelectedObj = NULL;
		for (FGraphPanelSelectionSet::TConstIterator It(SelectedNodes); It; ++It)
		{
			SelectedObj = *It;
		}

		// Get the function that the event node or function entry represents
		FFunctionFromNodeHelper FunctionFromNode(SelectedObj);
		if (FunctionFromNode.Function && FunctionFromNode.Node)
		{
			UFunction* ValidParent = Schema->GetCallableParentFunction(FunctionFromNode.Function);
			UEdGraph* TargetGraph = FunctionFromNode.Node->GetGraph();
			if (ValidParent && TargetGraph)
			{
				FGraphNodeCreator<UK2Node_CallParentFunction> FunctionNodeCreator(*TargetGraph);
				UK2Node_CallParentFunction* ParentFunctionNode = FunctionNodeCreator.CreateNode();
				ParentFunctionNode->SetFromFunction(ValidParent);
				ParentFunctionNode->AllocateDefaultPins();

				ParentFunctionNode->NodePosX = FunctionFromNode.Node->NodePosX + FunctionFromNode.Node->NodeWidth + 20;
				ParentFunctionNode->NodePosY = FunctionFromNode.Node->NodePosY + 15;
				FunctionNodeCreator.Finalize();
			}
		}
	}
}

bool FBlueprintEditor::CanAddParentNode() const
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	const FGraphPanelSelectionSet& SelectedNodes = GetSelectedNodes();
	if (SelectedNodes.Num() == 1)
	{
		UObject* SelectedObj = NULL;
		for (FGraphPanelSelectionSet::TConstIterator It(SelectedNodes); It; ++It)
		{
			SelectedObj = *It;
		}

		// Get the function that the event node or function entry represents
		FFunctionFromNodeHelper FunctionFromNode(SelectedObj);
		if (FunctionFromNode.Function)
		{
			return (Schema->GetCallableParentFunction(FunctionFromNode.Function) != NULL);
		}
	}

	return false;
}

void FBlueprintEditor::OnToggleBreakpoint()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UK2Node* SelectedNode = Cast<UK2Node>(*NodeIt);
		if ((SelectedNode != NULL) && SelectedNode->CanPlaceBreakpoints())
		{
			UBreakpoint* ExistingBreakpoint = FKismetDebugUtilities::FindBreakpointForNode(GetBlueprintObj(), SelectedNode);
			if (ExistingBreakpoint == NULL)
			{
				// Add a breakpoint on this node if there isn't one there already
				UBreakpoint* NewBreakpoint = NewObject<UBreakpoint>(GetBlueprintObj());
				FKismetDebugUtilities::SetBreakpointEnabled(NewBreakpoint, true);
				FKismetDebugUtilities::SetBreakpointLocation(NewBreakpoint, SelectedNode);
				GetBlueprintObj()->Breakpoints.Add(NewBreakpoint);
				GetBlueprintObj()->MarkPackageDirty();
			}
			else
			{
				// Remove the breakpoint if it was present
				FKismetDebugUtilities::StartDeletingBreakpoint(ExistingBreakpoint, GetBlueprintObj());
			}
		}
	}
}

bool FBlueprintEditor::CanToggleBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UK2Node* SelectedNode = Cast<UK2Node>(*NodeIt);
		if ((SelectedNode != NULL) && SelectedNode->CanPlaceBreakpoints())
		{
			return true;
		}
	}

	return false;
}

void FBlueprintEditor::OnAddBreakpoint()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UK2Node* SelectedNode = Cast<UK2Node>(*NodeIt);
		if ((SelectedNode != NULL) && SelectedNode->CanPlaceBreakpoints())
		{
			// Add a breakpoint on this node if there isn't one there already
			UBreakpoint* ExistingBreakpoint = FKismetDebugUtilities::FindBreakpointForNode(GetBlueprintObj(), SelectedNode);
			if (ExistingBreakpoint == NULL)
			{
				// Add a new breakpoint
				UBreakpoint* NewBreakpoint = NewObject<UBreakpoint>(GetBlueprintObj());
				FKismetDebugUtilities::SetBreakpointEnabled(NewBreakpoint, true);
				FKismetDebugUtilities::SetBreakpointLocation(NewBreakpoint, SelectedNode);
				GetBlueprintObj()->Breakpoints.Add(NewBreakpoint);
				GetBlueprintObj()->MarkPackageDirty();
			}
		}
	}
}

bool FBlueprintEditor::CanAddBreakpoint() const
{
	// See if any of the selected nodes are impure, and thus could have a breakpoint set on them
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UK2Node* SelectedNode = Cast<UK2Node>(*NodeIt);
		if ((SelectedNode != NULL) && SelectedNode->CanPlaceBreakpoints())
		{
			UBreakpoint* ExistingBreakpoint = FKismetDebugUtilities::FindBreakpointForNode(GetBlueprintObj(), SelectedNode);
			if (ExistingBreakpoint == NULL)
			{
				return true;
			}
		}
	}

	return false;
}

void FBlueprintEditor::OnRemoveBreakpoint()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* SelectedNode = CastChecked<UEdGraphNode>(*NodeIt);

		// Remove any pre-existing breakpoint on this node
		if (UBreakpoint* ExistingBreakpoint = FKismetDebugUtilities::FindBreakpointForNode(GetBlueprintObj(), SelectedNode))
		{
			// Remove the breakpoint
			FKismetDebugUtilities::StartDeletingBreakpoint(ExistingBreakpoint, GetBlueprintObj());
		}
	}
}

bool FBlueprintEditor::CanRemoveBreakpoint() const
{
	// See if any of the selected nodes have a breakpoint set
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* SelectedNode = CastChecked<UEdGraphNode>(*NodeIt);
		if (UBreakpoint* ExistingBreakpoint = FKismetDebugUtilities::FindBreakpointForNode(GetBlueprintObj(), SelectedNode))
		{
			return true;
		}
	}

	return false;
}

void FBlueprintEditor::OnDisableBreakpoint()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* SelectedNode = CastChecked<UEdGraphNode>(*NodeIt);

		if (UBreakpoint* ExistingBreakpoint = FKismetDebugUtilities::FindBreakpointForNode(GetBlueprintObj(), SelectedNode))
		{
			FKismetDebugUtilities::SetBreakpointEnabled(ExistingBreakpoint, false);
		}
	}
}

bool FBlueprintEditor::CanDisableBreakpoint() const
{
	// See if any of the selected nodes have a breakpoint set
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* SelectedNode = CastChecked<UEdGraphNode>(*NodeIt);
		if (UBreakpoint* ExistingBreakpoint = FKismetDebugUtilities::FindBreakpointForNode(GetBlueprintObj(), SelectedNode))
		{
			if (ExistingBreakpoint->IsEnabledByUser())
			{
				return true;
			}
		}
	}

	return false;
}

void FBlueprintEditor::OnEnableBreakpoint()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* SelectedNode = CastChecked<UEdGraphNode>(*NodeIt);

		if (UBreakpoint* ExistingBreakpoint = FKismetDebugUtilities::FindBreakpointForNode(GetBlueprintObj(), SelectedNode))
		{
			FKismetDebugUtilities::SetBreakpointEnabled(ExistingBreakpoint, true);
		}
	}
}

bool FBlueprintEditor::CanEnableBreakpoint() const
{
	// See if any of the selected nodes have a breakpoint set
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* SelectedNode = CastChecked<UEdGraphNode>(*NodeIt);
		if (UBreakpoint* ExistingBreakpoint = FKismetDebugUtilities::FindBreakpointForNode(GetBlueprintObj(), SelectedNode))
		{
			if (!ExistingBreakpoint->IsEnabledByUser())
			{
				return true;
			}
		}
	}

	return false;
}

void FBlueprintEditor::OnCollapseNodes()
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	// Does the selection set contain anything that is legal to collapse?
	TSet<UEdGraphNode*> CollapsableNodes;
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UEdGraphNode* SelectedNode = Cast<UEdGraphNode>(*NodeIt))
		{
			if (Schema->CanEncapuslateNode(*SelectedNode))
			{
				CollapsableNodes.Add(SelectedNode);
			}
		}
	}

	// Collapse them
	if (CollapsableNodes.Num())
	{
		UBlueprint* BlueprintObj = GetBlueprintObj();
		const FScopedTransaction Transaction( FGraphEditorCommands::Get().CollapseNodes->GetDescription() );
		BlueprintObj->Modify();

		CollapseNodes(CollapsableNodes);

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified( BlueprintObj );
	}
}

bool FBlueprintEditor::CanCollapseNodes() const
{
	//@TODO: ANIM: Determine what collapsing nodes means in an animation graph, and add any necessary compiler support for it
	if (IsEditingAnimGraph())
	{
		return false;
	}

	// Does the selection set contain anything that is legal to collapse?
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt))
		{
			if (Schema->CanEncapuslateNode(*Node))
			{
				return true;
			}
		}
	}

	return false;
}

void FBlueprintEditor::OnCollapseSelectionToFunction()
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	// Does the selection set contain anything that is legal to collapse?
	TSet<UEdGraphNode*> CollapsableNodes;
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UEdGraphNode* SelectedNode = Cast<UEdGraphNode>(*NodeIt))
		{
			if (Schema->CanEncapuslateNode(*SelectedNode))
			{
				CollapsableNodes.Add(SelectedNode);
			}
		}
	}

	// Collapse them
	if (CollapsableNodes.Num() && CanCollapseSelectionToFunction(CollapsableNodes))
	{
		UBlueprint* BlueprintObj = GetBlueprintObj();
		const FScopedTransaction Transaction( FGraphEditorCommands::Get().CollapseNodes->GetDescription() );
		BlueprintObj->Modify();

		UEdGraphNode* FunctionNode = NULL;
		UEdGraph* FunctionGraph = CollapseSelectionToFunction(CollapsableNodes, FunctionNode);

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified( BlueprintObj );

		MyBlueprintWidget->SelectItemByName(FunctionGraph->GetFName(),ESelectInfo::OnMouseClick);
		MyBlueprintWidget->OnRequestRenameOnActionNode();
	}
}

bool FBlueprintEditor::CanCollapseSelectionToFunction(TSet<class UEdGraphNode*>& InSelection) const
{
	bool bBadConnection = false;
	UEdGraphPin* OutputConnection = NULL;
	UEdGraphPin* InputConnection = NULL;

	// Create a function graph
	UEdGraph* FunctionGraph = FBlueprintEditorUtils::CreateNewGraph(GetBlueprintObj(), FBlueprintEditorUtils::FindUniqueKismetName(GetBlueprintObj(), TEXT("TempGraph")), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	FBlueprintEditorUtils::AddFunctionGraph(GetBlueprintObj(), FunctionGraph, /*bIsUserCreated=*/ true, NULL);

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	FCompilerResultsLog LogResults;
	LogResults.bAnnotateMentionedNodes = false;

	UEdGraphNode* InterfaceTemplateNode = nullptr;

	// Runs through every node and fully validates errors with placing selection in a function graph, reporting all errors.
	for (TSet<UEdGraphNode*>::TConstIterator NodeIt(InSelection); NodeIt; ++NodeIt)
	{
		UEdGraphNode* Node = *NodeIt;

		if(!Node->CanPasteHere(FunctionGraph, K2Schema))
		{
			if (UK2Node_CustomEvent* const CustomEvent = Cast<UK2Node_CustomEvent>(Node))
			{
				UEdGraphPin* EventExecPin = K2Schema->FindExecutionPin(*CustomEvent, EGPD_Output);
				check(EventExecPin);

				if(InterfaceTemplateNode)
				{
					LogResults.Error(*LOCTEXT("TooManyCustomEvents_Error", "Can use @@ as a template for creating the function, can only have a single custom event! Previously found @@").ToString(), CustomEvent, InterfaceTemplateNode);
				}
				else
				{
					// The custom event will be used as the template interface for the function.
					InterfaceTemplateNode = CustomEvent;
					if(InputConnection)
					{
						InputConnection = EventExecPin->LinkedTo[0];
					}
					continue;
				}
			}

			LogResults.Error(*LOCTEXT("CannotPasteNodeFunction_Error", "@@ cannot be placed in function graph").ToString(), Node);
			bBadConnection = true;
		}
		else
		{
			for (int32 PinIndex = 0; PinIndex < Node->Pins.Num(); ++PinIndex)
			{
				if(Node->Pins[PinIndex]->PinType.PinCategory == K2Schema->PC_Exec)
				{
					for (int32 LinkIndex = 0; LinkIndex < Node->Pins[PinIndex]->LinkedTo.Num(); ++LinkIndex)
					{
						if(!InSelection.Contains(Node->Pins[PinIndex]->LinkedTo[LinkIndex]->GetOwningNode()))
						{
							if(Node->Pins[PinIndex]->Direction == EGPD_Input)
							{
								// For input pins, there must be a single connection 
								if(InputConnection == NULL || InputConnection == Node->Pins[PinIndex])
								{
									InputConnection = Node->Pins[PinIndex];
								}
								else
								{
									// Check if the input connection was linked, report what node it is connected to
									LogResults.Error(*LOCTEXT("TooManyPathsMultipleInput_Error", "Found too many input connections in selection! @@ is connected to @@, previously found @@ connected to @@").ToString(), Node, Node->Pins[PinIndex]->LinkedTo[LinkIndex]->GetOwningNode(), InputConnection->GetOwningNode(), InputConnection->LinkedTo[0]->GetOwningNode());
									bBadConnection = true;
								}
							}
							else
							{
								// For output pins, as long as they all connect to the same pin, we consider the selection valid for being made into a function
								if(OutputConnection == NULL || OutputConnection == Node->Pins[PinIndex]->LinkedTo[LinkIndex])
								{
									OutputConnection = Node->Pins[PinIndex]->LinkedTo[LinkIndex];
								}
								else
								{
									check(OutputConnection->LinkedTo.Num());

									LogResults.Error(*LOCTEXT("TooManyPathsMultipleOutput_Error", "Found too many output connections in selection! @@ is connected to @@, previously found @@ connected to @@").ToString(), Node, Node->Pins[PinIndex]->LinkedTo[LinkIndex]->GetOwningNode(), OutputConnection->GetOwningNode(), OutputConnection->LinkedTo[0]->GetOwningNode());
									bBadConnection = true;
								}
							}
						}
					}
				}
			}
		}
	}

	// No need to check for cycling if the selection is invalid anyways.
	if(!bBadConnection && FBlueprintEditorUtils::CheckIfSelectionIsCycling(InSelection, LogResults))
	{
		bBadConnection = true;
	}

	FMessageLog MessageLog("BlueprintLog");
	MessageLog.NewPage(LOCTEXT("CollapseToFunctionPageLabel", "Collapse to Function"));
	MessageLog.AddMessages(LogResults.Messages);
	MessageLog.Notify(LOCTEXT("CollapseToFunctionError", "Collapsing to Function Failed!"));

	FBlueprintEditorUtils::RemoveGraph(GetBlueprintObj(), FunctionGraph);
	FunctionGraph->MarkPendingKill();
	return !bBadConnection;
}

bool FBlueprintEditor::CanCollapseSelectionToFunction() const
{
	return !IsEditingAnimGraph();
}

void FBlueprintEditor::OnCollapseSelectionToMacro()
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	// Does the selection set contain anything that is legal to collapse?
	TSet<UEdGraphNode*> CollapsableNodes;
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UEdGraphNode* SelectedNode = Cast<UEdGraphNode>(*NodeIt))
		{
			if (Schema->CanEncapuslateNode(*SelectedNode))
			{
				CollapsableNodes.Add(SelectedNode);
			}
		}
	}

	// Collapse them
	if (CollapsableNodes.Num() && CanCollapseSelectionToMacro(CollapsableNodes))
	{
		UBlueprint* BlueprintObj = GetBlueprintObj();
		const FScopedTransaction Transaction( FGraphEditorCommands::Get().CollapseNodes->GetDescription() );
		BlueprintObj->Modify();

		UEdGraphNode* MacroNode = NULL;
		UEdGraph* MacroGraph = CollapseSelectionToMacro(CollapsableNodes, MacroNode);

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified( BlueprintObj );

		MyBlueprintWidget->SelectItemByName(MacroGraph->GetFName(),ESelectInfo::OnMouseClick);
		MyBlueprintWidget->OnRequestRenameOnActionNode();
	}
}

bool FBlueprintEditor::CanCollapseSelectionToMacro(TSet<class UEdGraphNode*>& InSelection) const
{
	// Create a temporary macro graph
	UEdGraph* MacroGraph = FBlueprintEditorUtils::CreateNewGraph(GetBlueprintObj(), FBlueprintEditorUtils::FindUniqueKismetName(GetBlueprintObj(), TEXT("TempGraph")), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	FBlueprintEditorUtils::AddMacroGraph(GetBlueprintObj(), MacroGraph, /*bIsUserCreated=*/ true, NULL);

	// Does the selection set contain anything that is legal to collapse?
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	bool bCollapseAllowed = true;
	FCompilerResultsLog LogResults;
	LogResults.bAnnotateMentionedNodes = false;

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt);

		if(!Node->CanPasteHere(MacroGraph, Schema))
		{
			LogResults.Error(*LOCTEXT("CannotPasteNodeMacro_Error", "@@ cannot be placed in macro graph").ToString(), Node);
			bCollapseAllowed = false;
		}

		if (Schema->CanEncapuslateNode(*Node))
		{
			if(UK2Node_CallFunction* FunctionNode = Cast<UK2Node_CallFunction>(Node))
			{
				// Latent functions are not allowed in macros.
				if(FunctionNode->IsLatentFunction())
				{
					LogResults.Error(*LOCTEXT("LatentFunctionNotAllowed_Error", "Latent functions (@@) are not allowed in macros!").ToString(), FunctionNode);
					bCollapseAllowed = false;
				}
			}
		}
	}

	FMessageLog MessageLog("BlueprintLog");
	MessageLog.NewPage(LOCTEXT("CollapseToMacroPageLabel", "Collapse to Macro"));
	MessageLog.AddMessages(LogResults.Messages);
	MessageLog.Notify(LOCTEXT("CollapseToMacroError", "Collapsing to Macro Failed!"));

	FBlueprintEditorUtils::RemoveGraph(GetBlueprintObj(), MacroGraph);
	MacroGraph->MarkPendingKill();
	return bCollapseAllowed;
}

bool FBlueprintEditor::CanCollapseSelectionToMacro() const
{
	if (FocusedGraphEdPtr.IsValid())
	{
		if(IsEditingAnimGraph())
		{
			return false;
		}
	}

	return true;
}

void FBlueprintEditor::OnPromoteSelectionToFunction()
{
	const FScopedTransaction Transaction( LOCTEXT("ConvertCollapsedGraphToFunction", "Convert Collapse Graph to Function") );
	GetBlueprintObj()->Modify();

	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();

	// Set of nodes to select when finished
	TSet<class UEdGraphNode*> NodesToSelect;

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator It(SelectedNodes); It; ++It)
	{
		if (UK2Node_Composite* CompositeNode = Cast<UK2Node_Composite>(*It))
		{
			// Check if there is only one input and one output connection
			TSet<class UEdGraphNode*> NodesInGraph;
			NodesInGraph.Add(CompositeNode);

			if(CanCollapseSelectionToFunction(NodesInGraph))
			{
				DocumentManager->CleanInvalidTabs();

				// Expand the composite node back into the world
				UEdGraph* SourceGraph = CompositeNode->BoundGraph;

				// Expand all composite nodes back in place
				TSet<UEdGraphNode*> ExpandedNodes;
				ExpandNode(CompositeNode, SourceGraph, /*inout*/ ExpandedNodes);
				FBlueprintEditorUtils::RemoveGraph(GetBlueprintObj(), SourceGraph, EGraphRemoveFlags::Recompile);

				//Remove this node from selection
				FocusedGraphEd->SetNodeSelection(CompositeNode, false);

				UEdGraphNode* FunctionNode = NULL;
				CollapseSelectionToFunction(ExpandedNodes, FunctionNode);
				NodesToSelect.Add(FunctionNode);
			}
			else
			{
				NodesToSelect.Add(CompositeNode);
			}
		}
		else if(UEdGraphNode* Node = Cast<UEdGraphNode>(*It))
		{
			NodesToSelect.Add(Node);
		}
	}

	// Select all nodes that should still be part of selection
	for (auto NodeIt = NodesToSelect.CreateConstIterator(); NodeIt; ++NodeIt)
	{
		FocusedGraphEd->SetNodeSelection(*NodeIt, true);
	}
}

bool FBlueprintEditor::CanPromoteSelectionToFunction() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UK2Node_Composite* CompositeNode = Cast<UK2Node_Composite>(*NodeIt))
		{
			// Check if there is only one input and one output connection
			TSet<class UEdGraphNode*> NodesInGraph;
			NodesInGraph.Add(CompositeNode);

			return true;
		}
	}
	return false;
}

void FBlueprintEditor::OnPromoteSelectionToMacro()
{
	const FScopedTransaction Transaction( LOCTEXT("ConvertCollapsedGraphToMacro", "Convert Collapse Graph to Macro") );
	GetBlueprintObj()->Modify();

	// Set of nodes to select when finished
	TSet<class UEdGraphNode*> NodesToSelect;

	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator It(SelectedNodes); It; ++It)
	{
		if (UK2Node_Composite* CompositeNode = Cast<UK2Node_Composite>(*It))
		{
			TSet<class UEdGraphNode*> NodesInGraph;

			// Collect all the nodes to test if they can be made into a function
			for (auto NodeIt = CompositeNode->BoundGraph->Nodes.CreateConstIterator(); NodeIt; ++NodeIt)
			{
				UEdGraphNode* Node = *NodeIt;

				// Ignore the tunnel nodes
				if (Node->GetClass() != UK2Node_Tunnel::StaticClass())
				{
					NodesInGraph.Add(Node);
				}
			}

			if(CanCollapseSelectionToMacro(NodesInGraph))
			{
				DocumentManager->CleanInvalidTabs();

				// Expand the composite node back into the world
				UEdGraph* SourceGraph = CompositeNode->BoundGraph;

				// Expand all composite nodes back in place
				TSet<UEdGraphNode*> ExpandedNodes;
				ExpandNode(CompositeNode, SourceGraph, /*inout*/ ExpandedNodes);
				FBlueprintEditorUtils::RemoveGraph(GetBlueprintObj(), SourceGraph, EGraphRemoveFlags::Recompile);

				//Remove this node from selection
				FocusedGraphEd->SetNodeSelection(CompositeNode, false);

				UEdGraphNode* MacroNode = NULL;
				CollapseSelectionToMacro(ExpandedNodes, MacroNode);
				NodesToSelect.Add(MacroNode);
			}
			else
			{
				NodesToSelect.Add(CompositeNode);
			}
		}
		else if(UEdGraphNode* Node = Cast<UEdGraphNode>(*It))
		{
			NodesToSelect.Add(Node);
		}
	}

	// Select all nodes that should still be part of selection
	for (auto NodeIt = NodesToSelect.CreateConstIterator(); NodeIt; ++NodeIt)
	{
		FocusedGraphEd->SetNodeSelection(*NodeIt, true);
	}
}

bool FBlueprintEditor::CanPromoteSelectionToMacro() const
{
	if (FocusedGraphEdPtr.IsValid())
	{
		if(IsEditingAnimGraph())
		{
			return false;
		}
	}

	for (UObject* SelectedNode : GetSelectedNodes())
	{
		if (UK2Node_Composite* CompositeNode = Cast<UK2Node_Composite>(SelectedNode))
		{
			TSet<class UEdGraphNode*> NodesInGraph;

			// Collect all the nodes to test if they can be made into a function
			for (UEdGraphNode* Node : CompositeNode->BoundGraph->Nodes)
			{
				// Ignore the tunnel nodes
				if (Node->GetClass() != UK2Node_Tunnel::StaticClass())
				{
					NodesInGraph.Add(Node);
				}
			}

			return true;
		}
	}
	return false;
}

void FBlueprintEditor::OnExpandNodes()
{
	const FScopedTransaction Transaction( FGraphEditorCommands::Get().ExpandNodes->GetDescription() );
	GetBlueprintObj()->Modify();

	// Expand all composite nodes back in place
	TSet<UEdGraphNode*> ExpandedNodes;

	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UK2Node_MacroInstance* SelectedMacroInstanceNode = Cast<UK2Node_MacroInstance>(*NodeIt))
		{
			UEdGraph* MacroGraph = SelectedMacroInstanceNode->GetMacroGraph();
			if(MacroGraph)
			{
				DocumentManager->CleanInvalidTabs();

				// Clone the graph so that we do not delete the original
				UEdGraph* ClonedGraph = FEdGraphUtilities::CloneGraph(MacroGraph, NULL);
				ExpandNode(SelectedMacroInstanceNode, ClonedGraph, /*inout*/ ExpandedNodes);

				//Remove this node from selection
				FocusedGraphEd->SetNodeSelection(SelectedMacroInstanceNode, false);

				ClonedGraph->MarkPendingKill();

				//Add expanded nodes to selection
				for (auto ExpandedNodesIt = ExpandedNodes.CreateIterator(); ExpandedNodesIt; ++ExpandedNodesIt)
				{
					FocusedGraphEd->SetNodeSelection(*ExpandedNodesIt,true);
				}
			}
		}
		else if (UK2Node_Composite* SelectedCompositeNode = Cast<UK2Node_Composite>(*NodeIt))
		{
			DocumentManager->CleanInvalidTabs();

			// Expand the composite node back into the world
			UEdGraph* SourceGraph = SelectedCompositeNode->BoundGraph;

			ExpandNode(SelectedCompositeNode, SourceGraph, /*inout*/ ExpandedNodes);
			FBlueprintEditorUtils::RemoveGraph(GetBlueprintObj(), SourceGraph, EGraphRemoveFlags::Recompile);

			//Remove this node from selection
			FocusedGraphEd->SetNodeSelection(SelectedCompositeNode, false);

			//Add expanded nodes to selection
			for (auto ExpandedNodesIt = ExpandedNodes.CreateIterator(); ExpandedNodesIt; ++ExpandedNodesIt)
			{
				FocusedGraphEd->SetNodeSelection(*ExpandedNodesIt,true);
			}
		}
		else if (UK2Node_CallFunction* SelectedCallFunctionNode = Cast<UK2Node_CallFunction>(*NodeIt))
		{
			UEdGraph* FunctionGraph = SelectedCallFunctionNode->GetFunctionGraph();
			if(FunctionGraph)
			{
				DocumentManager->CleanInvalidTabs();

				// Clone the graph so that we do not delete the original
				UEdGraph* ClonedGraph = FEdGraphUtilities::CloneGraph(FunctionGraph, NULL);
				ExpandNode(SelectedCallFunctionNode, ClonedGraph, ExpandedNodes);

				//Remove this node from selection
				FocusedGraphEd->SetNodeSelection(SelectedCallFunctionNode, false);

				ClonedGraph->MarkPendingKill();

				//Add expanded nodes to selection
				for (auto ExpandedNodesIt = ExpandedNodes.CreateIterator(); ExpandedNodesIt; ++ExpandedNodesIt)
				{
					FocusedGraphEd->SetNodeSelection(*ExpandedNodesIt,true);
				}
			}
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprintObj());
}

bool FBlueprintEditor::CanExpandNodes() const
{
	// Does the selection set contain any composite nodes that are legal to expand?
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (Cast<UK2Node_Composite>(*NodeIt))
		{
			return true;
		}
		else if (UK2Node_MacroInstance* SelectedMacroInstanceNode = Cast<UK2Node_MacroInstance>(*NodeIt))
		{
			return SelectedMacroInstanceNode->GetMacroGraph() != NULL;
		}
		else if (UK2Node_CallFunction* SelectedCallFunctionNode = Cast<UK2Node_CallFunction>(*NodeIt))
		{
			return SelectedCallFunctionNode->GetFunctionGraph() != NULL;
		}
	}

	return false;
}

void FBlueprintEditor::SelectAllNodes()
{
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (FocusedGraphEd.IsValid())
	{
		FocusedGraphEd->SelectAllNodes();
	}
}

bool FBlueprintEditor::CanSelectAllNodes() const
{
	return true;
}

void FBlueprintEditor::DeleteSelectedNodes()
{
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (!FocusedGraphEd.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction( FGenericCommands::Get().Delete->GetDescription() );
	FocusedGraphEd->GetCurrentGraph()->Modify();

	bool bNeedToModifyStructurally = false;

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	ClearSelectionInAllEditors();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt( SelectedNodes ); NodeIt; ++NodeIt)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt))
		{
			if (Node->CanUserDeleteNode())
			{
				UK2Node* K2Node = Cast<UK2Node>(Node);
				if (K2Node != NULL && K2Node->NodeCausesStructuralBlueprintChange())
				{
					bNeedToModifyStructurally = true;
				}

				if (UK2Node_Composite* SelectedNode = Cast<UK2Node_Composite>(*NodeIt))
				{
					//Close the tab for the composite if it was open
					if (SelectedNode->BoundGraph)
					{
						DocumentManager->CleanInvalidTabs();
					}
				}
				else if (UK2Node_Timeline* TimelineNode = Cast<UK2Node_Timeline>(*NodeIt))
				{
					DocumentManager->CleanInvalidTabs();
				}

				FBlueprintEditorUtils::RemoveNode(GetBlueprintObj(), Node, true);
			}
		}
	}

	if (bNeedToModifyStructurally)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprintObj());
	}
	else
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprintObj());
	}

	//@TODO: Reselect items that were not deleted
}

bool FBlueprintEditor::CanDeleteNodes() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	return InEditingMode() && (SelectedNodes.Num() > 0);
}

void FBlueprintEditor::DeleteSelectedDuplicatableNodes()
{
	// Cache off the old selection
	const FGraphPanelSelectionSet OldSelectedNodes = GetSelectedNodes();
	
	// Clear the selection and only select the nodes that can be duplicated
	FGraphPanelSelectionSet CurrentSelection;
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (FocusedGraphEd.IsValid())
	{
		FocusedGraphEd->ClearSelectionSet();

		FGraphPanelSelectionSet RemainingNodes;
		for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
		{
			UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
			if ((Node != NULL) && Node->CanDuplicateNode())
			{
				FocusedGraphEd->SetNodeSelection(Node, true);
			}
			else
			{
				RemainingNodes.Add(Node);
			}
		}

		// Delete the duplicatable nodes
		DeleteSelectedNodes();

		// Reselect whatever's left from the original selection after the deletion
		FocusedGraphEd->ClearSelectionSet();

		for (FGraphPanelSelectionSet::TConstIterator SelectedIter(RemainingNodes); SelectedIter; ++SelectedIter)
		{
			if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
			{
				FocusedGraphEd->SetNodeSelection(Node, true);
			}
		}
	}
}

void FBlueprintEditor::CutSelectedNodes()
{
	CopySelectedNodes();
	// Cut should only delete nodes that can be duplicated
	DeleteSelectedDuplicatableNodes();
}

bool FBlueprintEditor::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FBlueprintEditor::CopySelectedNodes()
{
	// Export the selected nodes and place the text on the clipboard
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	FString ExportedText;

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		if(UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			Node->PrepareForCopying();
		}
	}

	FEdGraphUtilities::ExportNodesToText(SelectedNodes, /*out*/ ExportedText);
	FPlatformMisc::ClipboardCopy(*ExportedText);
}

bool FBlueprintEditor::CanCopyNodes() const
{
	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if ((Node != NULL) && Node->CanDuplicateNode())
		{
			return true;
		}
	}
	return false;
}

void FBlueprintEditor::PasteNodes()
{
	// Find the graph editor with focus
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (!FocusedGraphEd.IsValid())
	{
		return;
	}

	PasteNodesHere(FocusedGraphEd->GetCurrentGraph(), FocusedGraphEd->GetPasteLocation());
}


/**
 *	When copying and pasting functions from the LSBP operating on an instance to a class BP, 
 *	we should automatically transfer the functions from actors to the components
 */
struct FUpdatePastedNodes
{
	TSet<UK2Node_VariableGet*> AddedTargets;
	TSet<UK2Node_CallFunction*> AddedFunctions;
	TSet<UK2Node_Literal*> ReplacedTargets;
	TSet<UK2Node_CallFunctionOnMember*> ReplacedFunctions;

	UClass* CurrentClass;
	UEdGraph* Graph;
	TSet<UEdGraphNode*>& PastedNodes;
	const UEdGraphSchema_K2* K2Schema;

	FUpdatePastedNodes(UClass* InCurrentClass, TSet<UEdGraphNode*>& InPastedNodes, UEdGraph* InDestinationGraph)
		: CurrentClass(InCurrentClass)
		, PastedNodes(InPastedNodes)
		, Graph(InDestinationGraph)
		, K2Schema(GetDefault<UEdGraphSchema_K2>())
	{
		check(InCurrentClass && InDestinationGraph && K2Schema);
	}

	/**
	 *	Replace UK2Node_CallFunctionOnMember called on actor with a UK2Node_CallFunction. 
	 *	When the blueprint has the member.
	 */
	void ReplaceAll()
	{
		for(auto PastedNode : PastedNodes)
		{
			if (auto CallOnMember = Cast<UK2Node_CallFunctionOnMember>(PastedNode))
			{
				if (auto TargetInPin = CallOnMember->FindPin(K2Schema->PN_Self))
				{
					const auto TargetClass = Cast<const UClass>(TargetInPin->PinType.PinSubCategoryObject.Get());

					const bool bTargetIsNullOrSingleLinked = (TargetInPin->LinkedTo.Num() == 1) ||
						(!TargetInPin->LinkedTo.Num() && !TargetInPin->DefaultObject);

					const bool bCanCurrentBlueprintReplace = TargetClass
						&& CurrentClass->IsChildOf(TargetClass) // If current class if of the same type, it has the called member
						&& !CallOnMember->MemberVariableToCallOn.IsSelfContext()
						&& bTargetIsNullOrSingleLinked;

					if (bCanCurrentBlueprintReplace) 
					{
						UEdGraphNode* TargetNode = TargetInPin->LinkedTo.Num() ? TargetInPin->LinkedTo[0]->GetOwningNode() : NULL;
						auto TargetLiteralNode = Cast<UK2Node_Literal>(TargetNode);

						const bool bPastedNodeShouldBeReplacedWithTarget = TargetLiteralNode
							&& !TargetLiteralNode->GetObjectRef() //The node delivering target actor is invalid
							&& PastedNodes.Contains(TargetLiteralNode);
						const bool bPastedNodeShouldBeReplacedWithoutTarget = !TargetNode || !PastedNodes.Contains(TargetNode);

						if (bPastedNodeShouldBeReplacedWithTarget || bPastedNodeShouldBeReplacedWithoutTarget)
						{
							Replace(TargetLiteralNode, CallOnMember);
						}
					}
				}
			}
		}

		UpdatePastedCollection();
	}

private:

	void UpdatePastedCollection()
	{
		for(auto ReplacedTarget : ReplacedTargets)
		{
			if (ReplacedTarget && ReplacedTarget->GetValuePin() && !ReplacedTarget->GetValuePin()->LinkedTo.Num())
			{
				PastedNodes.Remove(ReplacedTarget);
				Graph->RemoveNode(ReplacedTarget);
			}
		}
		for(auto ReplacedFunction : ReplacedFunctions)
		{
			PastedNodes.Remove(ReplacedFunction);
			Graph->RemoveNode(ReplacedFunction);
		}
		for(auto AddedTarget : AddedTargets)
		{
			PastedNodes.Add(AddedTarget);
		}
		for(auto AddedFunction : AddedFunctions)
		{
			PastedNodes.Add(AddedFunction);
		}
	}

	bool MoveAllLinksExeptSelf(UK2Node* NewNode, UK2Node* OldNode)
	{
		bool bResult = true;
		for(auto OldPin : OldNode->Pins)
		{
			if(OldPin && (OldPin->PinName != K2Schema->PN_Self))
			{
				auto NewPin = NewNode->FindPin(OldPin->PinName);
				if (NewPin)
				{
					if (!K2Schema->MovePinLinks(*OldPin, *NewPin).CanSafeConnect())
					{
						UE_LOG(LogBlueprint, Error, TEXT("FUpdatePastedNodes: Cannot connect pin '%s' node '%s'"),
							*OldPin->PinName, *OldNode->GetName());
						bResult = false;
					}
				}
				else
				{
					UE_LOG(LogBlueprint, Error, TEXT("FUpdatePastedNodes: Cannot find pin '%s'"), *OldPin->PinName);
					bResult = false;
				}
			}
		}
		return bResult;
	}

	void InitializeNewNode(UK2Node* NewNode, UK2Node* OldNode, float NodePosX = 0.0f, float NodePosY = 0.0f)
	{	
		NewNode->NodePosX = OldNode ? OldNode->NodePosX : NodePosX;
		NewNode->NodePosY = OldNode ? OldNode->NodePosY : NodePosY;
		NewNode->SetFlags(RF_Transactional);
		Graph->AddNode(NewNode, false, false);
		NewNode->PostPlacedNewNode();
		NewNode->AllocateDefaultPins();
	}

	bool Replace(UK2Node_Literal* OldTarget, UK2Node_CallFunctionOnMember* OldCall)
	{
		bool bResult = true;
		check(OldCall);

		UK2Node_VariableGet* NewTarget = NULL;
		
		const UProperty* Property = OldCall->MemberVariableToCallOn.ResolveMember<UProperty>((UClass*)NULL);
		for (auto AddedTarget : AddedTargets)
		{
			if (AddedTarget && (Property == AddedTarget->VariableReference.ResolveMember<UProperty>(CurrentClass)))
			{
				NewTarget = AddedTarget;
				break;
			}
		}

		if (!NewTarget)
		{
			NewTarget = NewObject<UK2Node_VariableGet>(Graph);
			check(NewTarget);
			NewTarget->SetFromProperty(Property, true);
			AddedTargets.Add(NewTarget);
			const float AutoNodeOffsetX = 160.0f;
			InitializeNewNode(NewTarget, OldTarget, OldCall->NodePosX - AutoNodeOffsetX, OldCall->NodePosY);
		}

		if (OldTarget)
		{
			ReplacedTargets.Add(OldTarget);
		}

		UK2Node_CallFunction* NewCall = NewObject<UK2Node_CallFunction>(Graph);
		check(NewCall);
		NewCall->SetFromFunction(OldCall->GetTargetFunction());
		InitializeNewNode(NewCall, OldCall);
		AddedFunctions.Add(NewCall);

		if (!MoveAllLinksExeptSelf(NewCall, OldCall))
		{
			bResult = false;
		}

		if (NewTarget)
		{
			auto SelfPin = NewCall->FindPinChecked(K2Schema->PN_Self);
			if (!K2Schema->TryCreateConnection(SelfPin, NewTarget->GetValuePin()))
			{
				UE_LOG(LogBlueprint, Error, TEXT("FUpdatePastedNodes: Cannot connect new self."));
				bResult = false;
			}
		}

		OldCall->BreakAllNodeLinks();

		ReplacedFunctions.Add(OldCall);
		return bResult;
	}
};

void FBlueprintEditor::PasteNodesHere(class UEdGraph* DestinationGraph, const FVector2D& GraphLocation)
{
	// Find the graph editor with focus
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (!FocusedGraphEd.IsValid())
	{
		return;
	}
	const FScopedTransaction Transaction( FGenericCommands::Get().Paste->GetDescription() );
	DestinationGraph->Modify();

	// Clear the selection set (newly pasted stuff will be selected)
	ClearSelectionInAllEditors();

	// Grab the text to paste from the clipboard.
	FString TextToImport;
	FPlatformMisc::ClipboardPaste(TextToImport);

	// Import the nodes
	TSet<UEdGraphNode*> PastedNodes;
	FEdGraphUtilities::ImportNodesFromText(DestinationGraph, TextToImport, /*out*/ PastedNodes);

	// Update Paste Analytics
	AnalyticsStats.NodePasteCreateCount += PastedNodes.Num();

	{
		auto Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(DestinationGraph);
		auto CurrentClass = Blueprint ? Blueprint->GeneratedClass : NULL;
		if (CurrentClass)
		{
			FUpdatePastedNodes ReplaceNodes(CurrentClass, PastedNodes, DestinationGraph);
			ReplaceNodes.ReplaceAll();
		}
	}

	// Select the newly pasted stuff
	bool bNeedToModifyStructurally = false;

	//Average position of nodes so we can move them while still maintaining relative distances to each other
	FVector2D AvgNodePosition(0.0f,0.0f);

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* Node = *It;
		AvgNodePosition.X += Node->NodePosX;
		AvgNodePosition.Y += Node->NodePosY;
	}

	float InvNumNodes = 1.0f/float(PastedNodes.Num());
	AvgNodePosition.X *= InvNumNodes;
	AvgNodePosition.Y *= InvNumNodes;

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* Node = *It;
		FocusedGraphEd->SetNodeSelection(Node, true);

		Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + GraphLocation.X ;
		Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + GraphLocation.Y ;

		Node->SnapToGrid(SNodePanel::GetSnapGridSize());

		// Give new node a different Guid from the old one
		Node->CreateNewGuid();

		UK2Node* K2Node = Cast<UK2Node>(Node);
		if ((K2Node != NULL) && K2Node->NodeCausesStructuralBlueprintChange())
		{
			bNeedToModifyStructurally = true;
		}
	}

	if (bNeedToModifyStructurally)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprintObj());
	}
	else
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprintObj());
	}

	// Update UI
	FocusedGraphEd->NotifyGraphChanged();
}


bool FBlueprintEditor::CanPasteNodes() const
{
	// Find the graph editor with focus
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (!FocusedGraphEd.IsValid())
	{
		return false;
	}

	FString ClipboardContent;
	FPlatformMisc::ClipboardPaste(ClipboardContent);

	return InEditingMode() && FEdGraphUtilities::CanImportNodesFromText(FocusedGraphEd->GetCurrentGraph(), ClipboardContent);
}

void FBlueprintEditor::DuplicateNodes()
{
	// Copy and paste current selection
	CopySelectedNodes();
	PasteNodes();
}

bool FBlueprintEditor::CanDuplicateNodes() const
{
	return CanCopyNodes() && InEditingMode();
}

void FBlueprintEditor::OnAssignReferencedActor()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	USelection* SelectedActors = GEditor->GetSelectedActors();
	if ( SelectedNodes.Num() > 0 && SelectedActors != NULL && SelectedActors->Num() == 1 )
	{
		AActor* SelectedActor = Cast<AActor>( SelectedActors->GetSelectedObject(0) );
		if ( SelectedActor != NULL )
		{
			TArray<UK2Node_ActorBoundEvent*> NodesToAlter;

			for ( FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt )
			{
				UK2Node_ActorBoundEvent* SelectedNode = Cast<UK2Node_ActorBoundEvent>(*NodeIt);
				if ( SelectedNode != NULL )
				{
					NodesToAlter.Add( SelectedNode );
				}
			}

			// only create a transaction if there is a node that is affected.
			if ( NodesToAlter.Num() > 0 )
			{
				const FScopedTransaction Transaction( LOCTEXT("AssignReferencedActor", "Assign referenced Actor") );
				{
					for ( int32 NodeIndex = 0; NodeIndex < NodesToAlter.Num(); NodeIndex++ )
					{
						UK2Node_ActorBoundEvent* CurrentEvent = NodesToAlter[NodeIndex];

						// Store the node's current state and replace the referenced actor
						CurrentEvent->Modify();
						CurrentEvent->EventOwner = SelectedActor;
					}
				}
			}
		}
	}
}

bool FBlueprintEditor::CanAssignReferencedActor() const
{
	bool bWouldAssignActors = false;

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	if ( SelectedNodes.Num() > 0 )
	{
		USelection* SelectedActors = GEditor->GetSelectedActors();

		// If there is only one actor selected and at least one Blueprint graph
		// node is able to receive the assignment then return true.
		if ( SelectedActors != NULL && SelectedActors->Num() == 1 )
		{
			AActor* SelectedActor = Cast<AActor>( SelectedActors->GetSelectedObject(0) );
			if ( SelectedActor != NULL )
			{
				for ( FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt )
				{
					UK2Node_ActorBoundEvent* SelectedNode = Cast<UK2Node_ActorBoundEvent>(*NodeIt);
					if ( SelectedNode != NULL )
					{
						if ( SelectedNode->EventOwner != SelectedActor )
						{
							bWouldAssignActors = true;
							break;
						}
					}
				}
			}
		}
	}

	return bWouldAssignActors;
}

void FBlueprintEditor::OnSelectReferenceInLevel()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	if (SelectedNodes.Num() > 0)
	{
		TArray<AActor*> ActorsToSelect;

		// Iterate over all nodes, and select referenced actors.
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			UK2Node* SelectedNode = Cast<UK2Node>(*NodeIt);
			AActor* ReferencedActor = (SelectedNode) ? SelectedNode->GetReferencedLevelActor() : NULL;

			if (ReferencedActor != NULL)
			{
				ActorsToSelect.AddUnique(ReferencedActor);
			}
		}
		// If we found any actors to select clear the existing selection, select them and move the camera to show them.
		if( ActorsToSelect.Num() != 0 )
		{
			// First clear the previous selection
			GEditor->GetSelectedActors()->Modify();
			GEditor->SelectNone( false, true );

			// Now select the actors.
			for (int32 iActor = 0; iActor < ActorsToSelect.Num(); iActor++)
			{
				GEditor->SelectActor(ActorsToSelect[ iActor ], true, false, false);
			}

			// Execute the command to move camera to the object(s).
			GUnrealEd->Exec_Camera( TEXT("ALIGN ACTIVEVIEWPORTONLY"),*GLog); 
		}
	}
}

bool FBlueprintEditor::CanSelectReferenceInLevel() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	bool bCanSelectActors = false;
	if (SelectedNodes.Num() > 0)
	{
		// Iterate over all nodes, testing if they're pointing to actors.
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			UK2Node* SelectedNode = Cast<UK2Node>(*NodeIt);
			const AActor* ReferencedActor = (SelectedNode) ? SelectedNode->GetReferencedLevelActor() : NULL;

			bCanSelectActors = (ReferencedActor != NULL);
			if (ReferencedActor == NULL)
			{
				// Bail early if the selected node isn't referencing an actor
				return false;
			}
		}
	}

	return bCanSelectActors;
}

// Utility helper to get the currently hovered pin in the currently visible graph, or NULL if there isn't one
UEdGraphPin* FBlueprintEditor::GetCurrentlySelectedPin() const
{
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (FocusedGraphEd.IsValid())
	{
		return FocusedGraphEd->GetGraphPinForMenu();
	}

	return NULL;
}

void FBlueprintEditor::RegisterSCSEditorCustomization(const FName& InComponentName, TSharedPtr<ISCSEditorCustomization> InCustomization)
{
	SCSEditorCustomizations.Add(InComponentName, InCustomization);
}

void FBlueprintEditor::UnregisterSCSEditorCustomization(const FName& InComponentName)
{
	SCSEditorCustomizations.Remove(InComponentName);
}

TArray<FSCSEditorTreeNodePtrType>  FBlueprintEditor::GetSelectedSCSEditorTreeNodes() const
{
	TArray<FSCSEditorTreeNodePtrType>  Nodes;
	if (SCSEditor.IsValid())
	{
		Nodes = SCSEditor->GetSelectedNodes();
	}
	return Nodes;
}

FSCSEditorTreeNodePtrType FBlueprintEditor::FindAndSelectSCSEditorTreeNode(const UActorComponent* InComponent, bool IsCntrlDown) 
{
	FSCSEditorTreeNodePtrType NodePtr;

	if (SCSEditor.IsValid())
	{
		NodePtr = SCSEditor->GetNodeFromActorComponent(InComponent);
		if(NodePtr.IsValid())
		{
			SCSEditor->SelectNode(NodePtr, IsCntrlDown);
		}
	}

	return NodePtr;
}

void FBlueprintEditor::OnDisallowedPinConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB)
{
	FDisallowedPinConnection NewRecord;
	NewRecord.PinTypeCategoryA = PinA->PinType.PinCategory;
	NewRecord.bPinIsArrayA = PinA->PinType.bIsArray;
	NewRecord.bPinIsReferenceA = PinA->PinType.bIsReference;
	NewRecord.bPinIsWeakPointerA = PinA->PinType.bIsWeakPointer;
	NewRecord.PinTypeCategoryB = PinB->PinType.PinCategory;
	NewRecord.bPinIsArrayB = PinB->PinType.bIsArray;
	NewRecord.bPinIsReferenceB = PinB->PinType.bIsReference;
	NewRecord.bPinIsWeakPointerB = PinB->PinType.bIsWeakPointer;
	AnalyticsStats.GraphDisallowedPinConnections.Add(NewRecord);
}

void FBlueprintEditor::OnStartWatchingPin()
{
	if (UEdGraphPin* Pin = GetCurrentlySelectedPin())
	{
		// Follow an input back to it's output
		if ((Pin->Direction == EGPD_Input) && (Pin->LinkedTo.Num() > 0))
		{
			Pin = Pin->LinkedTo[0];
		}

		// Start watching it
		FKismetDebugUtilities::TogglePinWatch(GetBlueprintObj(), Pin);
	}
}

bool FBlueprintEditor::CanStartWatchingPin() const
{
	if (UEdGraphPin* Pin = GetCurrentlySelectedPin())
	{
		// Follow an input back to it's output
		if ((Pin->Direction == EGPD_Input) && (Pin->LinkedTo.Num() > 0))
		{
			Pin = Pin->LinkedTo[0];
		}

		return FKismetDebugUtilities::CanWatchPin(GetBlueprintObj(), Pin);
	}
	return false;
}

void FBlueprintEditor::OnStopWatchingPin()
{
	if (UEdGraphPin* Pin = GetCurrentlySelectedPin())
	{
		// Follow an input back to it's output
		if ((Pin->Direction == EGPD_Input) && (Pin->LinkedTo.Num() > 0))
		{
			Pin = Pin->LinkedTo[0];
		}

		FKismetDebugUtilities::TogglePinWatch(GetBlueprintObj(), Pin);
	}
}

bool FBlueprintEditor::CanStopWatchingPin() const
{
	if (UEdGraphPin* Pin = GetCurrentlySelectedPin())
	{
		// Follow an input back to it's output
		if ((Pin->Direction == EGPD_Input) && (Pin->LinkedTo.Num() > 0))
		{
			Pin = Pin->LinkedTo[0];
		}

		return FKismetDebugUtilities::IsPinBeingWatched(GetBlueprintObj(), Pin);
	}

	return false;
}

void FBlueprintEditor::ToggleSaveIntermediateBuildProducts()
{
	bSaveIntermediateBuildProducts = !bSaveIntermediateBuildProducts;
}

bool FBlueprintEditor::GetSaveIntermediateBuildProducts() const
{
	return bSaveIntermediateBuildProducts;
}

void FBlueprintEditor::OnNodeDoubleClicked(class UEdGraphNode* Node)
{
	//@TODO: Pull these last few stragglers out into their respective nodes; requires asking the same question in a different way without knowledge of the Explorer
	if (UK2Node_CallFunction* FunctionCall = Cast<UK2Node_CallFunction>(Node))
	{
		//Check whether the current graph node is a function call to a custom event.
		UK2Node_CustomEvent* CustomEventNode = nullptr;
		TArray<UK2Node_CustomEvent*> CustomNodes;
		FBlueprintEditorUtils::GetAllNodesOfClass(GetBlueprintObj(), CustomNodes);
		for (int32 i=0; i < CustomNodes.Num(); i++)
		{
			UK2Node_CustomEvent* CustomNodePtr = CustomNodes[ i ];
			if ((CustomNodePtr != nullptr) && (CustomNodePtr->CustomFunctionName == FunctionCall->FunctionReference.GetMemberName()))
			{
				CustomEventNode = CustomNodePtr;
			}
		}
		if (CustomEventNode != nullptr)
		{
			JumpToNode(CustomEventNode);
		}
		else
		{
			// Look for a local graph named the same thing as the function (e.g., a user created function or a harmless coincidence)
			UEdGraph* TargetGraph = FindObject<UEdGraph>(GetBlueprintObj(), *(FunctionCall->FunctionReference.GetMemberName().ToString()));
			if ((TargetGraph != nullptr) && !TargetGraph->HasAnyFlags(RF_Transient))
			{
				JumpToHyperlink(TargetGraph);
			}
			else
			{
				// otherwise look for the function in another blueprint, possibly a parent
				UClass* MemberParentClass = FunctionCall->FunctionReference.GetMemberParentClass(FunctionCall);
				if(MemberParentClass != nullptr)
				{
					UBlueprintGeneratedClass* ParentClass = Cast<UBlueprintGeneratedClass>(MemberParentClass);
					if(ParentClass != nullptr && ParentClass->ClassGeneratedBy != nullptr)
					{
						UBlueprint* Blueprint = Cast<UBlueprint>(ParentClass->ClassGeneratedBy);
						while(Blueprint != nullptr)
						{
							TargetGraph = FindObject<UEdGraph>(Blueprint, *(FunctionCall->FunctionReference.GetMemberName().ToString()));
							if((TargetGraph != nullptr) && !TargetGraph->HasAnyFlags(RF_Transient))
							{
								FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(TargetGraph);
								break;
							}

							ParentClass = Cast<UBlueprintGeneratedClass>(Blueprint->ParentClass);
							Blueprint = ParentClass != nullptr ? Cast<UBlueprint>(ParentClass->ClassGeneratedBy) : nullptr;
						}
					}
				}
			}
		}
	}
	else if (UK2Node_MacroInstance* MacroNode = Cast<UK2Node_MacroInstance>(Node))
	{
		UEdGraph* MacroGraph = MacroNode->GetMacroGraph();
		if (MacroGraph)
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(MacroGraph);
		}
	}
	else if (UK2Node_Timeline* TimelineNode = Cast<UK2Node_Timeline>(Node))
	{
		if (UTimelineTemplate* Timeline = GetBlueprintObj()->FindTimelineTemplateByVariableName(TimelineNode->TimelineName))
		{
			OpenDocument(Timeline, FDocumentTracker::OpenNewDocument);
		}
	}
	else if (UObject* HyperlinkTarget = Node->GetJumpTargetForDoubleClick())
	{
		FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(HyperlinkTarget);
	}
}

void FBlueprintEditor::ExtractEventTemplateForFunction(class UK2Node_CustomEvent* InCustomEvent, UEdGraphNode* InGatewayNode, class UK2Node_EditablePinBase* InEntryNode, class UK2Node_EditablePinBase* InResultNode, TSet<UEdGraphNode*>& InCollapsableNodes)
{
	check(InCustomEvent);

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	for(UEdGraphPin* Pin : InCustomEvent->Pins)
	{
		if(Pin->PinType.PinCategory == K2Schema->PC_Exec)
		{
			TArray< UEdGraphPin* > PinLinkList = Pin->LinkedTo;
			for( UEdGraphPin* PinLink : PinLinkList)
			{
				if(!InCollapsableNodes.Contains(PinLink->GetOwningNode()))
				{
					InGatewayNode->Modify();
					Pin->Modify();
					PinLink->Modify();

					K2Schema->MovePinLinks(*Pin, *K2Schema->FindExecutionPin(*InGatewayNode, EGPD_Output));
				}
			}
		}
		else if(Pin->PinType.PinCategory != K2Schema->PC_Delegate)
		{

			TArray< UEdGraphPin* > PinLinkList = Pin->LinkedTo;
			for( UEdGraphPin* PinLink : PinLinkList)
			{
				if(!InCollapsableNodes.Contains(PinLink->GetOwningNode()))
				{
					InGatewayNode->Modify();
					Pin->Modify();
					PinLink->Modify();

					FString PortName = Pin->PinName + TEXT("_Out");
					UEdGraphPin* RemotePortPin = InGatewayNode->FindPin(PortName);
					// For nodes that are connected to the event but not collapsing into the graph, they need to create a pin on the result.
					if(RemotePortPin == nullptr)
					{
						FString UniquePortName = InGatewayNode->CreateUniquePinName(PortName);

						RemotePortPin = InGatewayNode->CreatePin(
							Pin->Direction,
							Pin->PinType.PinCategory,
							Pin->PinType.PinSubCategory,
							Pin->PinType.PinSubCategoryObject.Get(),
							Pin->PinType.bIsArray,
							Pin->PinType.bIsReference,
							UniquePortName);
						InResultNode->CreateUserDefinedPin(UniquePortName, Pin->PinType);
					}
					PinLink->BreakAllPinLinks();
					PinLink->MakeLinkTo(RemotePortPin);
				}
				else
				{
					InEntryNode->Modify();

					FString UniquePortName = InGatewayNode->CreateUniquePinName(Pin->PinName);
					InEntryNode->CreateUserDefinedPin(UniquePortName, Pin->PinType);
				}
			}
		}
	}
}

void FBlueprintEditor::CollapseNodesIntoGraph(UEdGraphNode* InGatewayNode, UK2Node_EditablePinBase* InEntryNode, UK2Node_EditablePinBase* InResultNode, UEdGraph* InSourceGraph, UEdGraph* InDestinationGraph, TSet<UEdGraphNode*>& InCollapsableNodes)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Keep track of the statistics of the node positions so the new nodes can be located reasonably well
	float SumNodeX = 0.0f;
	float SumNodeY = 0.0f;
	float MinNodeX = 1e9f;
	float MinNodeY = 1e9f;
	float MaxNodeX = -1e9f;
	float MaxNodeY = -1e9f;

	UEdGraphNode* InterfaceTemplateNode = nullptr;

	// For collapsing to functions can use a single event as a template for the function. This event MUST be deleted at the end, and the pins pre-generated. 
	if(InGatewayNode->GetClass() == UK2Node_CallFunction::StaticClass())
	{
		for (UEdGraphNode* Node : InCollapsableNodes)
		{
			if (UK2Node_CustomEvent* const CustomEvent = Cast<UK2Node_CustomEvent>(Node))
			{
				check(!InterfaceTemplateNode);

				InterfaceTemplateNode = CustomEvent;
				InterfaceTemplateNode->Modify();

				ExtractEventTemplateForFunction(CustomEvent, InGatewayNode, InEntryNode, InResultNode, InCollapsableNodes);

				FString GraphName = FBlueprintEditorUtils::GenerateUniqueGraphName(GetBlueprintObj(), CustomEvent->GetNodeTitle(ENodeTitleType::ListView)).ToString();
				FBlueprintEditorUtils::RenameGraph(InDestinationGraph, GraphName);

				// Remove the node, it has no place in the new graph
				InCollapsableNodes.Remove(Node);
				break;
			}
		}
	}

	// Move the nodes over, which may create cross-graph references that we need fix up ASAP
	for (TSet<UEdGraphNode*>::TConstIterator NodeIt(InCollapsableNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* Node = *NodeIt;
		Node->Modify();

		// Update stats
		SumNodeX += Node->NodePosX;
		SumNodeY += Node->NodePosY;
		MinNodeX = FMath::Min<float>(MinNodeX, Node->NodePosX);
		MinNodeY = FMath::Min<float>(MinNodeY, Node->NodePosY);
		MaxNodeX = FMath::Max<float>(MaxNodeX, Node->NodePosX);
		MaxNodeY = FMath::Max<float>(MaxNodeY, Node->NodePosY);

		// Move the node over
		InSourceGraph->Nodes.Remove(Node);
		InDestinationGraph->Nodes.Add(Node);
		Node->Rename(/*NewName=*/ NULL, /*NewOuter=*/ InDestinationGraph);

		// Move the sub-graph to the new graph
		if(UK2Node_Composite* Composite = Cast<UK2Node_Composite>(Node))
		{
			InSourceGraph->SubGraphs.Remove(Composite->BoundGraph);
			InDestinationGraph->SubGraphs.Add(Composite->BoundGraph);
		}

		// Find cross-graph links
		for (int32 PinIndex = 0; PinIndex < Node->Pins.Num(); ++PinIndex)
		{
			UEdGraphPin* LocalPin = Node->Pins[PinIndex];

			bool bIsGatewayPin = false;
			if(LocalPin->LinkedTo.Num())
			{
				for (int32 LinkIndex = 0; LinkIndex < LocalPin->LinkedTo.Num(); ++LinkIndex)
				{
					UEdGraphPin* TrialPin = LocalPin->LinkedTo[LinkIndex];
					if (!InCollapsableNodes.Contains(TrialPin->GetOwningNode()))
					{
						bIsGatewayPin = true;
						break;
					}
				}
			}
			// If the pin has no links but is an exec pin and this is a function graph, then it is a gateway pin
			else if(InGatewayNode->GetClass() == UK2Node_CallFunction::StaticClass() && LocalPin->PinType.PinCategory == K2Schema->PC_Exec)
			{
				// Connect the gateway pin to the node, there is no remote pin to hook up because the exec pin was not originally connected
				LocalPin->Modify();
				UK2Node_EditablePinBase* LocalPort = (LocalPin->Direction == EGPD_Input) ? InEntryNode : InResultNode;
				UEdGraphPin* LocalPortPin = LocalPort->Pins[0];
				LocalPin->MakeLinkTo(LocalPortPin);
			}

			// Thunk cross-graph links thru the gateway
			if (bIsGatewayPin)
			{
				// Local port is either the entry or the result node in the collapsed graph
				// Remote port is the node placed in the source graph
				UK2Node_EditablePinBase* LocalPort = (LocalPin->Direction == EGPD_Input) ? InEntryNode : InResultNode;

				// Add a new pin to the entry/exit node and to the composite node
				UEdGraphPin* LocalPortPin = NULL;
				UEdGraphPin* RemotePortPin = NULL;

				// Function graphs have a single exec path through them, so only one exec pin for input and another for output. In this fashion, they must not be handled by name.
				if(InGatewayNode->GetClass() == UK2Node_CallFunction::StaticClass() && LocalPin->PinType.PinCategory == K2Schema->PC_Exec)
				{
					LocalPortPin = LocalPort->Pins[0];
					RemotePortPin = K2Schema->FindExecutionPin(*InGatewayNode, (LocalPortPin->Direction == EGPD_Input)? EGPD_Output : EGPD_Input);
				}
				else
				{
					// If there is a custom event being used as a template, we must check to see if any connected pins have already been built
					if(InterfaceTemplateNode && LocalPin->Direction == EGPD_Input)
					{
						// Find the pin on the entry node, we will use that pin's name to find the pin on the remote port
						UEdGraphPin* EntryNodePin = nullptr;
						EntryNodePin = InEntryNode->FindPin(LocalPin->LinkedTo[0]->PinName);
						if(EntryNodePin)
						{
							LocalPin->BreakAllPinLinks();
							LocalPin->MakeLinkTo(EntryNodePin);
							continue;
						}
					}

					if(LocalPin->LinkedTo[0]->GetOwningNode() != InEntryNode)
					{
						FString UniquePortName = InGatewayNode->CreateUniquePinName(LocalPin->PinName);

						if(!RemotePortPin && !LocalPortPin)
						{
							RemotePortPin = InGatewayNode->CreatePin(
								LocalPin->Direction,
								LocalPin->PinType.PinCategory,
								LocalPin->PinType.PinSubCategory,
								LocalPin->PinType.PinSubCategoryObject.Get(),
								LocalPin->PinType.bIsArray,
								LocalPin->PinType.bIsReference,
								UniquePortName);

							LocalPortPin = LocalPort->CreateUserDefinedPin(UniquePortName, LocalPin->PinType);
						}
					}
				}

				check(LocalPortPin);
				check(RemotePortPin);

				LocalPin->Modify();

				// Route the links
				for (int32 LinkIndex = 0; LinkIndex < LocalPin->LinkedTo.Num(); ++LinkIndex)
				{
					UEdGraphPin* RemotePin = LocalPin->LinkedTo[LinkIndex];
					RemotePin->Modify();

					if (!InCollapsableNodes.Contains(RemotePin->GetOwningNode()) && RemotePin->GetOwningNode() != InEntryNode && RemotePin->GetOwningNode() != InResultNode)
					{
						// Fix up the remote pin
						RemotePin->LinkedTo.Remove(LocalPin);
						RemotePin->MakeLinkTo(RemotePortPin);

						// Fix up the local pin
						LocalPin->LinkedTo.Remove(RemotePin);
						--LinkIndex;
						LocalPin->MakeLinkTo(LocalPortPin);
					}
				}
			}
		}

	}

	// Reposition the newly created nodes
	const int32 NumNodes = InCollapsableNodes.Num();

	// Remove the template node if one was used for generating the function
	if(InterfaceTemplateNode)
	{
		if(NumNodes == 0)
		{
			SumNodeX = InterfaceTemplateNode->NodePosX;
			SumNodeY = InterfaceTemplateNode->NodePosY;
		}

		FBlueprintEditorUtils::RemoveNode(GetBlueprintObj(), InterfaceTemplateNode);
	}

	// Using the result pin, we will ensure that there is a path through the function by checking if it is connected. If it is not, link it to the entry node.
	if(UEdGraphPin* ResultExecFunc = K2Schema->FindExecutionPin(*InResultNode, EGPD_Input))
	{
		if(ResultExecFunc->LinkedTo.Num() == 0)
		{
			K2Schema->FindExecutionPin(*InEntryNode, EGPD_Output)->MakeLinkTo(K2Schema->FindExecutionPin(*InResultNode, EGPD_Input));
		}
	}

	const float CenterX = NumNodes == 0? SumNodeX : SumNodeX / NumNodes;
	const float CenterY = NumNodes == 0? SumNodeY : SumNodeY / NumNodes;
	const float MinusOffsetX = 160.0f; //@TODO: Random magic numbers
	const float PlusOffsetX = 300.0f;

	// Put the gateway node at the center of the empty space in the old graph
	InGatewayNode->NodePosX = CenterX;
	InGatewayNode->NodePosY = CenterY;
	InGatewayNode->SnapToGrid(SNodePanel::GetSnapGridSize());

	// Put the entry and exit nodes on either side of the nodes in the new graph
	//@TODO: Should we recenter the whole ensemble?
	if(NumNodes != 0)
	{
		InEntryNode->NodePosX = MinNodeX - MinusOffsetX;
		InEntryNode->NodePosY = CenterY;
		InEntryNode->SnapToGrid(SNodePanel::GetSnapGridSize());

		InResultNode->NodePosX = MaxNodeX + PlusOffsetX;
		InResultNode->NodePosY = CenterY;
		InResultNode->SnapToGrid(SNodePanel::GetSnapGridSize());
	}
}

void FBlueprintEditor::CollapseNodes(TSet<UEdGraphNode*>& InCollapsableNodes)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (!FocusedGraphEd.IsValid())
	{
		return;
	}

	UEdGraph* SourceGraph = FocusedGraphEd->GetCurrentGraph();
	SourceGraph->Modify();

	// Create the composite node that will serve as the gateway into the subgraph
	UK2Node_Composite* GatewayNode = NULL;
	{
		UK2Node_Composite* TemplateNode = NewObject<UK2Node_Composite>();
		GatewayNode = FEdGraphSchemaAction_K2NewNode::SpawnNodeFromTemplate<UK2Node_Composite>(SourceGraph, TemplateNode, FVector2D(0,0));
		GatewayNode->bCanRenameNode = true;
		check(GatewayNode);
	}

	FName GraphName;
	GraphName = FBlueprintEditorUtils::FindUniqueKismetName(GetBlueprintObj(), TEXT("CollapseGraph"));

	// Rename the graph to the correct name
	UEdGraph* DestinationGraph = GatewayNode->BoundGraph;
	TSharedPtr<INameValidatorInterface> NameValidator = MakeShareable(new FKismetNameValidator(GetBlueprintObj(), GraphName));
	FBlueprintEditorUtils::RenameGraphWithSuggestion(DestinationGraph, NameValidator, GraphName.ToString());

	CollapseNodesIntoGraph(GatewayNode, GatewayNode->GetInputSink(), GatewayNode->GetOutputSource(), SourceGraph, DestinationGraph, InCollapsableNodes);
}

UEdGraph* FBlueprintEditor::CollapseSelectionToFunction(TSet<class UEdGraphNode*>& InCollapsableNodes, UEdGraphNode*& OutFunctionNode)
{
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (!FocusedGraphEd.IsValid())
	{
		return NULL;
	}

	UEdGraph* SourceGraph = FocusedGraphEd->GetCurrentGraph();
	SourceGraph->Modify();

	UEdGraph* NewGraph = NULL;

	FName DocumentName = FBlueprintEditorUtils::FindUniqueKismetName(GetBlueprintObj(), TEXT("NewFunction"));

	NewGraph = FBlueprintEditorUtils::CreateNewGraph(GetBlueprintObj(), DocumentName, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	FBlueprintEditorUtils::AddFunctionGraph(GetBlueprintObj(), NewGraph, /*bIsUserCreated=*/ true, NULL);

	TArray<UK2Node_FunctionEntry*> EntryNodes;
	NewGraph->GetNodesOfClass(EntryNodes);
	UK2Node_FunctionEntry* EntryNode = EntryNodes[0];
	UK2Node_FunctionResult* ResultNode = NULL;

	// Create Result
	FGraphNodeCreator<UK2Node_FunctionResult> ResultNodeCreator(*NewGraph);
	UK2Node_FunctionResult* FunctionResult = ResultNodeCreator.CreateNode();

	const UEdGraphSchema_K2* Schema = Cast<UEdGraphSchema_K2>(FunctionResult->GetSchema());
	FunctionResult->NodePosX = EntryNode->NodePosX + EntryNode->NodeWidth + 256;
	FunctionResult->NodePosY = EntryNode->NodePosY;

	ResultNodeCreator.Finalize();

	ResultNode = FunctionResult;

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// make temp list builder
	FGraphActionListBuilderBase TempListBuilder;
	TempListBuilder.OwnerOfTemporaries = NewObject<UEdGraph>(GetBlueprintObj());
	TempListBuilder.OwnerOfTemporaries->SetFlags(RF_Transient);

	// Use schema function to make 'spawn action'
	FK2ActionMenuBuilder::AddSpawnInfoForFunction(FindField<UFunction>(GetBlueprintObj()->SkeletonGeneratedClass, DocumentName), false, FFunctionTargetInfo(), FMemberReference(), TEXT(""), K2Schema->AG_LevelReference, TempListBuilder);
	// and execute it
	if(TempListBuilder.GetNumActions() == 1)
	{
		TArray<UEdGraphPin*> DummyPins;
		FGraphActionListBuilderBase::ActionGroup& Action = TempListBuilder.GetAction(0);
		OutFunctionNode = Action.Actions[0]->PerformAction(FocusedGraphEd->GetCurrentGraph(), DummyPins, FVector2D::ZeroVector, false);
	}
	check(OutFunctionNode);

	CollapseNodesIntoGraph(OutFunctionNode, EntryNode, ResultNode, SourceGraph, NewGraph, InCollapsableNodes);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprintObj());
	OutFunctionNode->ReconstructNode();

	return NewGraph;
}

UEdGraph* FBlueprintEditor::CollapseSelectionToMacro(TSet<UEdGraphNode*>& InCollapsableNodes, UEdGraphNode*& OutMacroNode)
{
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (!FocusedGraphEd.IsValid())
	{
		return NULL;
	}

	UEdGraph* SourceGraph = FocusedGraphEd->GetCurrentGraph();
	SourceGraph->Modify();

	UEdGraph* DestinationGraph = NULL;

	FName DocumentName = FBlueprintEditorUtils::FindUniqueKismetName(GetBlueprintObj(), TEXT("NewMacro"));

	DestinationGraph = FBlueprintEditorUtils::CreateNewGraph(GetBlueprintObj(), DocumentName, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	FBlueprintEditorUtils::AddMacroGraph(GetBlueprintObj(), DestinationGraph, /*bIsUserCreated=*/ true, NULL);

	UK2Node_MacroInstance* MacroTemplate = NewObject<UK2Node_MacroInstance>();
	MacroTemplate->SetMacroGraph(DestinationGraph);

	UK2Node_MacroInstance* GatewayNode = FEdGraphSchemaAction_K2NewNode::SpawnNodeFromTemplate<UK2Node_MacroInstance>(SourceGraph, MacroTemplate, FVector2D(0.0f, 0.0f), false);

	TArray<UK2Node_Tunnel*> TunnelNodes;
	GatewayNode->GetMacroGraph()->GetNodesOfClass(TunnelNodes);

	UK2Node_EditablePinBase* InputSink = nullptr;
	UK2Node_EditablePinBase* OutputSink = nullptr;

	// Retrieve the tunnel nodes to use them to match up pin links that connect to the gateway.
	for (UK2Node_Tunnel* Node : TunnelNodes)
	{
		if (Node->IsEditable())
		{
			if (Node->bCanHaveOutputs)
			{
				InputSink = Node;
			}
			else if (Node->bCanHaveInputs)
			{
				OutputSink = Node;
			}
		}
	}

	CollapseNodesIntoGraph(GatewayNode, InputSink, OutputSink, SourceGraph, DestinationGraph, InCollapsableNodes);

	OutMacroNode = GatewayNode;
	OutMacroNode->ReconstructNode();

	return DestinationGraph;
}

void FBlueprintEditor::ExpandNode(UEdGraphNode* InNodeToExpand, UEdGraph* InSourceGraph, TSet<UEdGraphNode*>& OutExpandedNodes)
{
	UEdGraph* DestinationGraph = InNodeToExpand->GetGraph();
	UEdGraph* SourceGraph = InSourceGraph;
	check(SourceGraph);

	// Mark all edited objects so they will appear in the transaction record if needed.
	DestinationGraph->Modify();
	SourceGraph->Modify();
	InNodeToExpand->Modify();

	UEdGraphNode* Entry = NULL;
	UEdGraphNode* Result = NULL;

	// Move the nodes over, remembering any that are boundary nodes
	while (SourceGraph->Nodes.Num())
	{
		UEdGraphNode* Node = SourceGraph->Nodes.Pop();

		if(!Node->CanPasteHere(DestinationGraph, Node->GetSchema()))
		{
			continue;
		}

		Node->Modify();

		Node->Rename(/*NewName=*/ NULL, /*NewOuter=*/ DestinationGraph);
		DestinationGraph->Nodes.Add(Node);

		// Want to test exactly against tunnel, we shouldn't collapse embedded collapsed
		// nodes or macros, only the tunnels in/out of the collapsed graph
		if (Node->GetClass() == UK2Node_Tunnel::StaticClass())
		{
			UK2Node_Tunnel* TunnelNode = Cast<UK2Node_Tunnel>(Node);
			if(TunnelNode->bCanHaveOutputs)
			{
				Entry = Node;
			}
			else if(TunnelNode->bCanHaveInputs)
			{
				Result = Node;
			}
		}
		else if (Node->GetClass() == UK2Node_FunctionEntry::StaticClass())
		{
			Entry = Node;
		}
		else if(Node->GetClass() == UK2Node_FunctionResult::StaticClass())
		{
			Result = Node;
		}
		else
		{
			OutExpandedNodes.Add(Node);
		}
	}

	UEdGraphPin* OutputExecPinReconnect = NULL;
	if(UK2Node_CallFunction* CallFunction = Cast<UK2Node_CallFunction>(InNodeToExpand))
	{
		if(CallFunction->GetThenPin()->LinkedTo.Num())
		{
			OutputExecPinReconnect = CallFunction->GetThenPin()->LinkedTo[0];
		}
	}

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	K2Schema->CollapseGatewayNode(Cast<UK2Node>(InNodeToExpand), Entry, Result);

	if(Entry)
	{
		Entry->DestroyNode();
	}

	if(Result)
	{
		Result->DestroyNode();
	}

	// Make sure any subgraphs get propagated appropriately
	if (SourceGraph->SubGraphs.Num() > 0)
	{
		DestinationGraph->SubGraphs.Append(SourceGraph->SubGraphs);
		SourceGraph->SubGraphs.Empty();
	}

	// Remove the gateway node and source graph
	InNodeToExpand->DestroyNode();

	// This should be set for function nodes, all expanded nodes should connect their output exec pins to the original pin.
	if(OutputExecPinReconnect)
	{
		for (TSet<UEdGraphNode*>::TConstIterator NodeIt(OutExpandedNodes); NodeIt; ++NodeIt)
		{
			UEdGraphNode* Node = *NodeIt;
			for (int32 PinIndex = 0; PinIndex < Node->Pins.Num(); ++PinIndex)
			{
				// Only hookup output exec pins that do not have a connection
				if(Node->Pins[PinIndex]->PinType.PinCategory == K2Schema->PC_Exec && Node->Pins[PinIndex]->Direction == EGPD_Output && Node->Pins[PinIndex]->LinkedTo.Num() == 0)
				{
					Node->Pins[PinIndex]->MakeLinkTo(OutputExecPinReconnect);
				}
			}
		}
	}
}

void FBlueprintEditor::SaveEditedObjectState()
{
	check(IsEditingSingleBlueprint());

	// Clear currently edited documents
	GetBlueprintObj()->LastEditedDocuments.Empty();

	// Ask all open documents to save their state, which will update LastEditedDocuments
	DocumentManager->SaveAllState();
}

void FBlueprintEditor::RequestSaveEditedObjectState()
{
	bRequestedSavingOpenDocumentState = true;
}

void FBlueprintEditor::Tick(float DeltaTime)
{
	if (bRequestedSavingOpenDocumentState)
	{
		bRequestedSavingOpenDocumentState = false;

		SaveEditedObjectState();
	}
}
TStatId FBlueprintEditor::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FBlueprintEditor, STATGROUP_Tickables);
}


void FBlueprintEditor::OnStartEditingDefaultsClicked()
{
	StartEditingDefaults(/*bAutoFocus=*/ true);
}

void FBlueprintEditor::OnListObjectsReferencedByClass()
{
	ObjectTools::ShowReferencedObjs(GetBlueprintObj()->GeneratedClass);
}

void FBlueprintEditor::OnListObjectsReferencedByBlueprint()
{
	ObjectTools::ShowReferencedObjs(GetBlueprintObj());
}

void FBlueprintEditor::OnRepairCorruptedBlueprint()
{
	IKismetCompilerInterface& Compiler = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(KISMET_COMPILER_MODULENAME);
	Compiler.RecoverCorruptedBlueprint(GetBlueprintObj());
}

void FBlueprintEditor::StartEditingDefaults(bool bAutoFocus, bool bForceRefresh)
{
	if (IsEditingSingleBlueprint())
	{
		if (GetBlueprintObj()->GeneratedClass != NULL)
		{
			DefaultEditor->ShowDetailsForSingleObject(GetBlueprintObj()->GeneratedClass->GetDefaultObject(), SKismetInspector::FShowDetailsOptions(DefaultEditString(), bForceRefresh));
		}
	}
	else if (GetEditingObjects().Num() > 0)
	{
		TArray<UObject*> DefaultObjects;
		for (int32 i = 0; i < GetEditingObjects().Num(); ++i)
		{
			auto Blueprint = (UBlueprint*)(GetEditingObjects()[i]);
			if (Blueprint->GeneratedClass)
			{
				DefaultObjects.Add(Blueprint->GeneratedClass->GetDefaultObject());
			}
		}
		if (DefaultObjects.Num())
		{
			DefaultEditor->ShowDetailsForObjects(DefaultObjects);
		}
	}
}

void FBlueprintEditor::RenameNewlyAddedAction(FName InActionName)
{
	TabManager->InvokeTab(FBlueprintEditorTabs::MyBlueprintID);
	TabManager->InvokeTab(FBlueprintEditorTabs::DetailsID);

	if (MyBlueprintWidget.IsValid())
	{
		MyBlueprintWidget->SelectItemByName(InActionName,ESelectInfo::OnMouseClick);
		MyBlueprintWidget->OnRequestRenameOnActionNode();
	}
}

void FBlueprintEditor::OnAddNewVariable()
{
	const FScopedTransaction Transaction( LOCTEXT("AddVariable", "Add Variable") );

	FString VarNameString = TEXT("NewVar");
	FName VarName = FName(*VarNameString);

	// Reset MyBlueprint item filter so new variable is visible
	MyBlueprintWidget->OnResetItemFilter();

	// Make sure the new name is valid
	TSharedPtr<INameValidatorInterface> NameValidator = MakeShareable(new FKismetNameValidator(GetBlueprintObj()));
	int32 Index = 0;
	while (NameValidator->IsValid(VarName) != Ok)
	{
		VarName = FName(*FString::Printf(TEXT("%s%i"), *VarNameString, Index));
		++Index;
	}

	GetBlueprintObj()->Modify();

	bool bSuccess = MyBlueprintWidget.IsValid() && FBlueprintEditorUtils::AddMemberVariable(GetBlueprintObj(), VarName, MyBlueprintWidget->GetLastPinTypeUsed());

	if(!bSuccess)
	{
		LogSimpleMessage( LOCTEXT("AddVariable_Error", "Adding new variable failed.") );
	}
	else
	{
		RenameNewlyAddedAction(VarName);
	}
}

void FBlueprintEditor::OnAddNewLocalVariable()
{
	UEdGraph* TargetGraph = FocusedGraphEdPtr.Pin()->GetCurrentGraph();
	if(TargetGraph != NULL)
	{
		const FScopedTransaction Transaction( LOCTEXT("AddLocalVariable", "Add Local Variable") );
		GetBlueprintObj()->Modify();

		// Figure out a decent place to stick the node
		const FVector2D NewNodePos = TargetGraph->GetGoodPlaceForNewNode();

		// Create a new event node
		UK2Node_LocalVariable* LocalVariableTemplate = NewObject<UK2Node_LocalVariable>();
		LocalVariableTemplate->VariableType = MyBlueprintWidget->GetLastPinTypeUsed();

		UK2Node_LocalVariable* LocalVariableNode = FEdGraphSchemaAction_K2NewNode::SpawnNodeFromTemplate<UK2Node_LocalVariable>(TargetGraph, LocalVariableTemplate, NewNodePos);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprintObj());

		// Finally, bring up kismet and jump to the new node
		if (LocalVariableNode != NULL)
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(LocalVariableNode);
			RenameNewlyAddedAction(LocalVariableNode->CustomVariableName);
		}
	}
}

void FBlueprintEditor::OnAddNewDelegate()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	check(NULL != K2Schema);
	UBlueprint* const Blueprint = GetBlueprintObj();
	check(NULL != Blueprint);

	// Reset MyBlueprint item filter so new variable is visible
	MyBlueprintWidget->OnResetItemFilter();

	const FString NameString = TEXT("NewEventDispatcher");
	FName Name = FName(*NameString);
	TArray<FName> Variables;
	FBlueprintEditorUtils::GetClassVariableList(Blueprint, Variables);
	int32 Index = 0;
	while (Variables.Contains(Name) || !FBlueprintEditorUtils::IsGraphNameUnique(Blueprint, Name))
	{
		Name = FName(*FString::Printf(TEXT("%s%i"), *NameString, Index));
		++Index;
	}

	const FScopedTransaction Transaction( LOCTEXT("AddNewDelegate", "Add New Event Dispatcher") ); 
	Blueprint->Modify();

	FEdGraphPinType DelegateType;
	DelegateType.PinCategory = K2Schema->PC_MCDelegate;
	const bool bVarCreatedSuccess = FBlueprintEditorUtils::AddMemberVariable(Blueprint, Name, DelegateType);
	if(!bVarCreatedSuccess)
	{
		LogSimpleMessage( LOCTEXT("AddDelegateVariable_Error", "Adding new delegate variable failed.") );
		return;
	}

	UEdGraph* const NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, Name, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	if(!NewGraph)
	{
		FBlueprintEditorUtils::RemoveMemberVariable(Blueprint, Name);
		LogSimpleMessage( LOCTEXT("AddDelegateVariable_Error", "Adding new delegate variable failed.") );
		return;
	}

	NewGraph->bEditable = false;

	K2Schema->CreateDefaultNodesForGraph(*NewGraph);
	K2Schema->CreateFunctionGraphTerminators(*NewGraph, NULL);
	K2Schema->AddExtraFunctionFlags(NewGraph, (FUNC_BlueprintCallable|FUNC_BlueprintEvent|FUNC_Public));
	K2Schema->MarkFunctionEntryAsEditable(NewGraph, true);

	Blueprint->DelegateSignatureGraphs.Add(NewGraph);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	RenameNewlyAddedAction(Name);
}

void FBlueprintEditor::NewDocument_OnClicked(ECreatedDocumentType GraphType)
{
	FString DocumentNameString;
	bool bResetMyBlueprintFilter = false;

	switch (GraphType)
	{
	case CGT_NewFunctionGraph:
		DocumentNameString = LOCTEXT("NewDocFuncName", "NewFunction").ToString();
		bResetMyBlueprintFilter = true;
		break;
	case CGT_NewEventGraph:
		DocumentNameString = LOCTEXT("NewDocEventGraphName", "NewEventGraph").ToString();
		bResetMyBlueprintFilter = true;
		break;
	case CGT_NewMacroGraph:
		DocumentNameString = LOCTEXT("NewDocMacroName", "NewMacro").ToString();
		bResetMyBlueprintFilter = true;
		break;
	case CGT_NewAnimationGraph:
		DocumentNameString = LOCTEXT("NewDocAnimationGraphName", "NewAnimationGraph").ToString();
		bResetMyBlueprintFilter = true;
		break;
	default:
		DocumentNameString = LOCTEXT("NewDocNewName", "NewDocument").ToString();
		break;
	}
	
	// Reset MyBlueprint item filter so new variable is visible
	if( bResetMyBlueprintFilter )
	{
		MyBlueprintWidget->OnResetItemFilter();
	}

	FName DocumentName = FName(*DocumentNameString);

	// Make sure the new name is valid
	int32 Index = 0;
	while (!FBlueprintEditorUtils::IsGraphNameUnique(GetBlueprintObj(), DocumentName))
	{
		DocumentName = FName(*FString::Printf(TEXT("%s%i"), *DocumentNameString, Index));
		++Index;
	}

	check(IsEditingSingleBlueprint());

	const FScopedTransaction Transaction( LOCTEXT("AddNewFunction", "Add New Function") ); 
	GetBlueprintObj()->Modify();

	UEdGraph* NewGraph = NULL;

	if (GraphType == CGT_NewFunctionGraph)
	{
		NewGraph = FBlueprintEditorUtils::CreateNewGraph(GetBlueprintObj(), DocumentName, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
		FBlueprintEditorUtils::AddFunctionGraph(GetBlueprintObj(), NewGraph, /*bIsUserCreated=*/ true, NULL);
	}
	else if (GraphType == CGT_NewMacroGraph)
	{
		NewGraph = FBlueprintEditorUtils::CreateNewGraph(GetBlueprintObj(), DocumentName, UEdGraph::StaticClass(),  UEdGraphSchema_K2::StaticClass());
		FBlueprintEditorUtils::AddMacroGraph(GetBlueprintObj(), NewGraph, /*bIsUserCreated=*/ true, NULL);
	}
	else if (GraphType == CGT_NewEventGraph)
	{
		NewGraph = FBlueprintEditorUtils::CreateNewGraph(GetBlueprintObj(), DocumentName, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
		FBlueprintEditorUtils::AddUbergraphPage(GetBlueprintObj(), NewGraph);
	}
	else if (GraphType == CGT_NewAnimationGraph)
	{
		//@TODO: ANIMREFACTOR: This code belongs in Persona, not in BlueprintEditor
		NewGraph = FBlueprintEditorUtils::CreateNewGraph(GetBlueprintObj(), DocumentName, UAnimationGraph::StaticClass(), UAnimationGraphSchema::StaticClass());
		FBlueprintEditorUtils::AddDomainSpecificGraph(GetBlueprintObj(), NewGraph);
	}
	else
	{
		ensureMsg(false, TEXT("GraphType is invalid") );
	}

	// Now open the new graph
	if (NewGraph)
	{
		OpenDocument(NewGraph, FDocumentTracker::OpenNewDocument);

		RenameNewlyAddedAction(DocumentName);
	}
	else
	{
		LogSimpleMessage( LOCTEXT("AddDocument_Error", "Adding new document failed.") );
	}
}

bool FBlueprintEditor::NewDocument_IsVisibleForType(ECreatedDocumentType GraphType) const
{
	switch (GraphType)
	{
	case CGT_NewVariable:
		return (GetBlueprintObj()->BlueprintType != BPTYPE_Interface) && (GetBlueprintObj()->BlueprintType != BPTYPE_MacroLibrary);
	case CGT_NewFunctionGraph:
		return (GetBlueprintObj()->BlueprintType != BPTYPE_MacroLibrary);
	case CGT_NewMacroGraph:
		return (GetBlueprintObj()->BlueprintType == BPTYPE_MacroLibrary) || (GetBlueprintObj()->BlueprintType == BPTYPE_Normal) || (GetBlueprintObj()->BlueprintType == BPTYPE_LevelScript);
	case CGT_NewAnimationGraph:
		return GetBlueprintObj()->IsA(UAnimBlueprint::StaticClass());
	case CGT_NewEventGraph:
		return FBlueprintEditorUtils::DoesSupportEventGraphs(GetBlueprintObj());
	case CGT_NewLocalVariable:
		return (GetBlueprintObj()->BlueprintType != BPTYPE_Interface) && (GetFocusedGraph() && GetFocusedGraph()->GetSchema()->GetGraphType(GetFocusedGraph()) == EGraphType::GT_Function);
	}

	return false;
}

bool FBlueprintEditor::AddNewDelegateIsVisible() const
{
	const UBlueprint* Blueprint = GetBlueprintObj();
	return (NULL != Blueprint)
		&& (Blueprint->BlueprintType != BPTYPE_Interface) 
		&& (Blueprint->BlueprintType != BPTYPE_MacroLibrary);
}

void FBlueprintEditor::NotifyPreChange(UProperty* PropertyAboutToChange)
{
	// this only delivers message to the "FOCUSED" one, not every one
	// internally it will only deliver the message to the selected node, not all nodes
	FString PropertyName = PropertyAboutToChange->GetName();
	if (FocusedGraphEdPtr.IsValid())
	{
		FocusedGraphEdPtr.Pin()->NotifyPrePropertyChange(PropertyName);
	}
}

void FBlueprintEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged)
{
	FString PropertyName = PropertyThatChanged->GetName();
	if (FocusedGraphEdPtr.IsValid())
	{
		FocusedGraphEdPtr.Pin()->NotifyPostPropertyChange(PropertyChangedEvent, PropertyName);
	}
	
	// Note: if change type is "interactive," hold off on applying the change (e.g. this will occur if the user is scrubbing a spinbox value; we don't want to apply the change until the mouse is released, for performance reasons)
	if( IsEditingSingleBlueprint() && PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive )
	{
		UBlueprint* Blueprint = GetBlueprintObj();
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

		// Call PostEditChange() on any Actors that might be based on this Blueprint
		FBlueprintEditorUtils::PostEditChangeBlueprintActors(Blueprint);
	}

	// Force updates to occur immediately during interactive mode (otherwise the preview won't refresh because it won't be ticking)
	UpdateSCSPreview(PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive);
}

void FBlueprintEditor::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == TEXT("bHasDefaultPin") ||
		PropertyName == TEXT("StartIndex") ||
		PropertyName == TEXT("PinNames") ||
		PropertyName == TEXT("bIsCaseSensitive"))
	{
		DocumentManager->RefreshAllTabs();
	}
}

FName FBlueprintEditor::GetToolkitFName() const
{
	return FName("BlueprintEditor");
}

FText FBlueprintEditor::GetBaseToolkitName() const
{
	return LOCTEXT( "AppLabel", "Blueprint Editor" );
}

FText FBlueprintEditor::GetToolkitName() const
{
	const auto EditingObjects = GetEditingObjects();

	if( IsEditingSingleBlueprint() )
	{
		const bool bDirtyState = GetBlueprintObj()->GetOutermost()->IsDirty();

		FFormatNamedArguments Args;
		Args.Add( TEXT("DirtyState"), bDirtyState ? FText::FromString( TEXT( "*" ) ) : FText::GetEmpty() );

		if (FBlueprintEditorUtils::IsLevelScriptBlueprint(GetBlueprintObj()))
		{
			const FString& LevelName = FPackageName::GetShortFName( GetBlueprintObj()->GetOutermost()->GetFName().GetPlainNameString() ).GetPlainNameString();	

			Args.Add( TEXT("LevelName"), FText::FromString( LevelName ) );
			return FText::Format( NSLOCTEXT("KismetEditor", "LevelScriptAppLabel", "{LevelName}{DirtyState} - Level Blueprint Editor"), Args );
		}
		else
		{
			Args.Add( TEXT("BlueprintName"), FText::FromString( GetBlueprintObj()->GetName() ) );
			return FText::Format( NSLOCTEXT("KismetEditor", "BlueprintScriptAppLabel", "{BlueprintName}{DirtyState}"), Args );
		}
	}

	TSubclassOf< UObject > SharedParentClass = NULL;

	for( auto ObjectIter = EditingObjects.CreateConstIterator(); ObjectIter; ++ObjectIter )
	{
		UBlueprint* Blueprint = Cast<UBlueprint>( *ObjectIter );;
		check( Blueprint );

		// Initialize with the class of the first object we encounter.
		if( *SharedParentClass == NULL )
		{
			SharedParentClass = Blueprint->ParentClass;
		}

		// If we've encountered an object that's not a subclass of the current best baseclass,
		// climb up a step in the class hierarchy.
		while( !Blueprint->ParentClass->IsChildOf( SharedParentClass ) )
		{
			SharedParentClass = SharedParentClass->GetSuperClass();
		}
	}

	FFormatNamedArguments Args;
	Args.Add( TEXT("NumberOfObjects"), EditingObjects.Num() );
	Args.Add( TEXT("ObjectName"), FText::FromString( SharedParentClass->GetName() ) );
	return FText::Format( NSLOCTEXT("KismetEditor", "ToolkitTitle_UniqueLayerName", "{NumberOfObjects} {ClassName} - Blueprint Defaults"), Args );
}


FLinearColor FBlueprintEditor::GetWorldCentricTabColorScale() const
{
	if ((IsEditingSingleBlueprint()) && FBlueprintEditorUtils::IsLevelScriptBlueprint(GetBlueprintObj()))
	{
		return FLinearColor( 0.0f, 0.2f, 0.3f, 0.5f );
	}
	else
	{
		return FLinearColor( 0.0f, 0.0f, 0.3f, 0.5f );
	}
}

bool FBlueprintEditor::IsBlueprintEditor() const
{
	return true;
}

FString FBlueprintEditor::GetWorldCentricTabPrefix() const
{
	check(IsEditingSingleBlueprint());

	if (FBlueprintEditorUtils::IsLevelScriptBlueprint(GetBlueprintObj()))
	{
		return NSLOCTEXT("KismetEditor", "WorldCentricTabPrefix_LevelScript", "Script ").ToString();
	}
	else
	{
		return NSLOCTEXT("KismetEditor", "WorldCentricTabPrefix_Blueprint", "Blueprint ").ToString();
	}
}

void FBlueprintEditor::VariableListWasUpdated()
{
	StartEditingDefaults(/*bAutoFocus=*/ false);
}

FString FBlueprintEditor::DefaultEditString() 
{
	return LOCTEXT("BlueprintEditingDefaults", "Editing defaults").ToString();
}

FString FBlueprintEditor::GetDefaultEditorTitle()
{
	return LOCTEXT("BlueprintDefaultsTabTitle", "Blueprint Defaults").ToString();
}

bool FBlueprintEditor::GetBoundsForSelectedNodes(class FSlateRect& Rect, float Padding)
{
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (FocusedGraphEd.IsValid())
	{
		return FocusedGraphEd->GetBoundsForSelectedNodes(Rect, Padding);
	}
	return false;
}

void FBlueprintEditor::OnRenameNode()
{
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if(FocusedGraphEd.IsValid())
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			UEdGraphNode* SelectedNode = Cast<UEdGraphNode>(*NodeIt);
			if (SelectedNode != NULL && SelectedNode->bCanRenameNode)
			{
				FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(SelectedNode, true);
				break;
			}
		}
	}
}

bool FBlueprintEditor::CanRenameNodes() const
{
	if(InEditingMode())
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		bool bCanRenameNodes = (SelectedNodes.Num() == 1) ? true : false;

		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			UEdGraphNode* SelectedNode = Cast<UEdGraphNode>(*NodeIt);
			if (SelectedNode && !SelectedNode->bCanRenameNode)
			{
				bCanRenameNodes = false;
				break;
			}
		}
		return bCanRenameNodes;
	}
	return false;
}

bool FBlueprintEditor::OnNodeVerifyTitleCommit(const FText& NewText, UEdGraphNode* NodeBeingChanged)
{
	bool bValid(false);

	if (NodeBeingChanged && NodeBeingChanged->bCanRenameNode)
	{
		// Clear off any existing error message 
		NodeBeingChanged->ErrorMsg.Empty();
		NodeBeingChanged->bHasCompilerMessage = false;

		if (!NameEntryValidator.IsValid())
		{
			NameEntryValidator = FNameValidatorFactory::MakeValidator(NodeBeingChanged);
		}

		EValidatorResult VResult = NameEntryValidator->IsValid(NewText.ToString(), true);
		if (VResult == EValidatorResult::Ok)
		{
			bValid = true;
		}
		else if (FocusedGraphEdPtr.IsValid()) 
		{
			EValidatorResult Valid = NameEntryValidator->IsValid(NewText.ToString(), false);
			
			NodeBeingChanged->bHasCompilerMessage = true;
			NodeBeingChanged->ErrorMsg = NameEntryValidator->GetErrorString(NewText.ToString(), Valid);
			NodeBeingChanged->ErrorType = EMessageSeverity::Error;
		}
	}
	NameEntryValidator.Reset();

	return bValid;
}

void FBlueprintEditor::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged)
{
	if (NodeBeingChanged)
	{
		const FScopedTransaction Transaction( NSLOCTEXT( "K2_RenameNode", "RenameNode", "Rename Node" ) );
		NodeBeingChanged->Modify();
		NodeBeingChanged->OnRenameNode(NewText.ToString());
	}
}



/////////////////////////////////////////////////////

void FBlueprintEditor::OnEditTabClosed(TSharedRef<SDockTab> Tab)
{
	// Update the edited object state
	if (GetBlueprintObj())
	{
		SaveEditedObjectState();
	}
}

// Tries to open the specified graph and bring it's document to the front
TSharedPtr<SGraphEditor> FBlueprintEditor::OpenGraphAndBringToFront(UEdGraph* Graph)
{
	// First, switch back to standard mode
	SetCurrentMode(FBlueprintEditorApplicationModes::StandardBlueprintEditorMode);

	// Next, try to make sure there is a copy open
	TSharedPtr<SDockTab> TabWithGraph = OpenDocument(Graph, FDocumentTracker::CreateHistoryEvent);

	// We know that the contents of the opened tabs will be a graph editor.
	return StaticCastSharedRef<SGraphEditor>(TabWithGraph->GetContent());
}

TSharedPtr<SDockTab> FBlueprintEditor::OpenDocument(UObject* DocumentID, FDocumentTracker::EOpenDocumentCause Cause)
{
	TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(DocumentID);
	return DocumentManager->OpenDocument(Payload, Cause);
}

void FBlueprintEditor::NavigateTab(FDocumentTracker::EOpenDocumentCause InCause)
{
	OpenDocument(NULL, InCause);
}

void FBlueprintEditor::CloseDocumentTab(UObject* DocumentID)
{
	TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(DocumentID);
	DocumentManager->CloseTab(Payload);
}

// Finds any open tabs containing the specified document and adds them to the specified array; returns true if at least one is found
bool FBlueprintEditor::FindOpenTabsContainingDocument(UObject* DocumentID, /*inout*/ TArray< TSharedPtr<SDockTab> >& Results)
{
	int32 StartingCount = Results.Num();

	TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(DocumentID);

	DocumentManager->FindMatchingTabs( Payload, /*inout*/ Results);

	// Did we add anything new?
	return (StartingCount != Results.Num());
}

void FBlueprintEditor::RestoreEditedObjectState()
{
	check(IsEditingSingleBlueprint());	

	if (GetBlueprintObj()->LastEditedDocuments.Num() == 0)
	{
		if(FBlueprintEditorUtils::SupportsConstructionScript(GetBlueprintObj()))
		{
			GetBlueprintObj()->LastEditedDocuments.Add(FBlueprintEditorUtils::FindUserConstructionScript(GetBlueprintObj()));
		}

		GetBlueprintObj()->LastEditedDocuments.Add(FBlueprintEditorUtils::FindEventGraph(GetBlueprintObj()));
	}

	for (int32 i = 0; i < GetBlueprintObj()->LastEditedDocuments.Num(); i++)
	{
		if (UObject* Obj = GetBlueprintObj()->LastEditedDocuments[i].EditedObject)
		{

			if(UEdGraph* Graph = Cast<UEdGraph>(Obj))
			{
				UEdGraph* TopLevelGraph = FBlueprintEditorUtils::GetTopLevelGraph(Graph);
				
				FDocumentTracker::EOpenDocumentCause OpenCause = TopLevelGraph != Graph? FDocumentTracker::OpenNewDocument : FDocumentTracker::RestorePreviousDocument;
				TSharedPtr<SDockTab> TabWithGraph = OpenDocument(TopLevelGraph, OpenCause);

				TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(TabWithGraph->GetContent());
				GraphEditor->SetViewLocation(GetBlueprintObj()->LastEditedDocuments[i].SavedViewOffset, GetBlueprintObj()->LastEditedDocuments[i].SavedZoomAmount);
			}
			else
			{
				TSharedPtr<SDockTab> TabWithGraph = OpenDocument(Obj, FDocumentTracker::RestorePreviousDocument);
			}
		}
	}
}

void FBlueprintEditor::OnEditTunnel()
{
	auto GraphEditor = FocusedGraphEdPtr.Pin();
	if (GraphEditor.IsValid())
	{
		TSharedPtr<SWidget> TitleBar = GraphEditor->GetTitleBar();
		if (TitleBar.IsValid())
		{
			StaticCastSharedPtr<SGraphTitleBar>(TitleBar)->BeginEditing();
		}
	}
}

void FBlueprintEditor::OnCreateComment()
{
	auto GraphEditor = FocusedGraphEdPtr.Pin();
	if (GraphEditor.IsValid())
	{
		if (UEdGraph* Graph = GraphEditor->GetCurrentGraph())
		{
			if (const UEdGraphSchema* Schema = Graph->GetSchema())
			{
				if (Schema->IsA(UEdGraphSchema_K2::StaticClass()))
				{
					FEdGraphSchemaAction_K2AddComment CommentAction;
					CommentAction.PerformAction(Graph, NULL, GraphEditor->GetPasteLocation());
				}
			}
		}
	}
}

void FBlueprintEditor::SetPinVisibility(SGraphEditor::EPinVisibility Visibility)
{
	PinVisibility = Visibility;
	OnSetPinVisibility.Broadcast(PinVisibility);
}

void FBlueprintEditor::OnFindVariableReferences()
{
	auto GraphEditor = FocusedGraphEdPtr.Pin();
	if (GraphEditor.IsValid())
	{
		const FGraphPanelSelectionSet SelectedNodes = GraphEditor->GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			UK2Node_Variable* VariableNode = Cast<UK2Node_Variable>(*NodeIt);
			if (VariableNode)
			{
				SummonSearchUI(true, VariableNode->GetVarName().ToString());
			}
		}
	}
}

bool FBlueprintEditor::CanFindVariableReferences()
{
	auto GraphEditor = FocusedGraphEdPtr.Pin();
	if (GraphEditor.IsValid())
	{
		const FGraphPanelSelectionSet SelectedNodes = GraphEditor->GetSelectedNodes();
		if( SelectedNodes.Num() == 1 )
		{
			return true;
		}
	}
	return false;
}

void FBlueprintEditor::OnFindInstancesCustomEvent()
{
	auto GraphEditor = FocusedGraphEdPtr.Pin();
	if (GraphEditor.IsValid())
	{
		const FGraphPanelSelectionSet SelectedNodes = GraphEditor->GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			UEdGraphNode* SelectedNode = Cast<UEdGraphNode>(*NodeIt);
			if (SelectedNode != NULL && SelectedNode->bCanRenameNode)
			{
				if (SelectedNode->IsA(UK2Node_CustomEvent::StaticClass()))
				{
					UK2Node_CustomEvent* CustomEvent = (UK2Node_CustomEvent*)SelectedNode;
					SummonSearchUI(true, CustomEvent->CustomFunctionName.ToString());
				}
			}
		}
	}
}

AActor* FBlueprintEditor::GetPreviewActor() const
{
	return SCSViewport->GetPreviewActor();
}

FReply FBlueprintEditor::OnSpawnGraphNodeByShortcut(FInputGesture InGesture, const FVector2D& InPosition, UEdGraph* InGraph)
{
	UEdGraph* Graph = InGraph;

	FBlueprintPaletteListBuilder PaletteBuilder(GetBlueprintObj());
	TSharedPtr< FEdGraphSchemaAction > Action = FBlueprintSpawnNodeCommands::Get().GetGraphActionByGesture(InGesture, PaletteBuilder, InGraph);

	if(Action.IsValid())
	{
		TArray<UEdGraphPin*> DummyPins;
		Action->PerformAction(Graph, DummyPins, InPosition);
	}

	return FReply::Handled();
}

void FBlueprintEditor::ToolkitBroughtToFront()
{
	UBlueprint* CurrentBlueprint = GetBlueprintObj();
	if( CurrentBlueprint != NULL )
	{
		UObject* DebugInstance = CurrentBlueprint->GetObjectBeingDebugged();
		CurrentBlueprint->SetObjectBeingDebugged( NULL );
		CurrentBlueprint->SetObjectBeingDebugged( DebugInstance );
	}
}

bool FBlueprintEditor::CanClassGenerateEvents( UClass* InClass )
{
	if( InClass )
	{
		for( TFieldIterator<UMulticastDelegateProperty> PropertyIt( InClass, EFieldIteratorFlags::IncludeSuper ); PropertyIt; ++PropertyIt )
		{
			UProperty* Property = *PropertyIt;
			if( !Property->HasAnyPropertyFlags( CPF_Parm ) && Property->HasAllPropertyFlags( CPF_BlueprintAssignable ))
			{
				return true;
			}
		}
	}
	return false;
}

void FBlueprintEditor::OnNodeSpawnedByKeymap()
{
	UpdateNodeCreationStats( ENodeCreateAction::Keymap );
}

void FBlueprintEditor::UpdateNodeCreationStats( const ENodeCreateAction::Type CreateAction )
{
	switch( CreateAction )
	{
	case ENodeCreateAction::MyBlueprintDragPlacement:
		AnalyticsStats.MyBlueprintNodeDragPlacementCount++;
		break;
	case ENodeCreateAction::PaletteDragPlacement:
		AnalyticsStats.PaletteNodeDragPlacementCount++;
		break;
	case ENodeCreateAction::GraphContext:
		AnalyticsStats.NodeGraphContextCreateCount++;
		break;
	case ENodeCreateAction::PinContext:
		AnalyticsStats.NodePinContextCreateCount++;
		break;
	case ENodeCreateAction::Keymap:
		AnalyticsStats.NodeKeymapCreateCount++;
		break;
	}
}

TSharedPtr<ISCSEditorCustomization> FBlueprintEditor::CustomizeSCSEditor(USceneComponent* InComponentToCustomize) const
{
	check(InComponentToCustomize != NULL);
	const TSharedPtr<ISCSEditorCustomization>* FoundCustomization = SCSEditorCustomizations.Find(InComponentToCustomize->GetClass()->GetFName());
	if(FoundCustomization != NULL)
	{
		return *FoundCustomization;
	}

	return TSharedPtr<ISCSEditorCustomization>();
}

FString FBlueprintEditor::GetPIEStatus() const
{
	UBlueprint* CurrentBlueprint = GetBlueprintObj();
	UWorld *DebugWorld = NULL;
	ENetMode NetMode = NM_Standalone;
	if (CurrentBlueprint)
	{
		DebugWorld = CurrentBlueprint->GetWorldBeingDebugged();
		if (DebugWorld)
		{
			NetMode = DebugWorld->GetNetMode();
		}
		else
		{
			UObject * ObjOuter = CurrentBlueprint->GetObjectBeingDebugged();
			while(DebugWorld == NULL && ObjOuter != NULL)
			{
				ObjOuter = ObjOuter->GetOuter();
				DebugWorld = Cast<UWorld>(ObjOuter);
			}
		}
	}
	
	if (DebugWorld)
	{
		NetMode = DebugWorld->GetNetMode();
	}

	if (NetMode == NM_ListenServer || NetMode == NM_DedicatedServer)
	{
		return TEXT("SERVER - SIMULATING");
	}
	else if (NetMode == NM_Client)
	{
		return TEXT("CLIENT - SIMULATING");
	}

	return TEXT("SIMULATING");
}

bool FBlueprintEditor::IsEditingAnimGraph() const
{
	if (FocusedGraphEdPtr.IsValid())
	{
		if (UEdGraph* CurrentGraph = FocusedGraphEdPtr.Pin()->GetCurrentGraph())
		{
			if (CurrentGraph->Schema->IsChildOf(UAnimationGraphSchema::StaticClass()) || (CurrentGraph->Schema == UAnimationStateMachineSchema::StaticClass()))
			{
				return true;
			}
		}
	}

	return false;
}

UEdGraph* FBlueprintEditor::GetFocusedGraph() const
{
	if(FocusedGraphEdPtr.IsValid())
	{
		return FocusedGraphEdPtr.Pin()->GetCurrentGraph();
	}
	return nullptr;
}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////


#undef LOCTEXT_NAMESPACE

