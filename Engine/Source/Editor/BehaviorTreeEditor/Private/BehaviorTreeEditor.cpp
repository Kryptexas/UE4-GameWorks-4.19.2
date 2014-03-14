// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"
#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "BehaviorTreeColors.h"
#include "EdGraphUtilities.h"

#include "Toolkits/IToolkitHost.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"

#define LOCTEXT_NAMESPACE "BehaviorTreeEditor"

const FName FBehaviorTreeEditor::BTGraphTabId( TEXT( "Document" ) );
const FName FBehaviorTreeEditor::BTPropertiesTabId( TEXT( "BehaviorTreeEditor_Properties" ) );
const FName FBehaviorTreeEditor::BTSearchTabId(TEXT("BehaviorTreeEditor_Search"));

//////////////////////////////////////////////////////////////////////////
struct FBTGraphEditorSummoner : public FDocumentTabFactoryForObjects<UEdGraph>
{
public:
	DECLARE_DELEGATE_RetVal_OneParam(TSharedRef<SGraphEditor>, FOnCreateGraphEditorWidget, UEdGraph*);
public:
	FBTGraphEditorSummoner(TSharedPtr<class FBehaviorTreeEditor> InBehaviorTreeEditorPtr, FOnCreateGraphEditorWidget CreateGraphEditorWidgetCallback)
		: FDocumentTabFactoryForObjects<UEdGraph>(FBehaviorTreeEditor::BTGraphTabId, InBehaviorTreeEditorPtr)
		, BehaviorTreeEditorPtr(InBehaviorTreeEditorPtr)
		, OnCreateGraphEditorWidget(CreateGraphEditorWidgetCallback)
	{
	}

	virtual void OnTabActivated(TSharedPtr<SDockTab> Tab) const OVERRIDE
	{
		TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
		BehaviorTreeEditorPtr.Pin()->OnGraphEditorFocused(GraphEditor);
	}

	virtual void OnTabRefreshed(TSharedPtr<SDockTab> Tab) const OVERRIDE
	{
		TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
		GraphEditor->NotifyGraphChanged();
	}

protected:
	virtual TAttribute<FText> ConstructTabNameForObject(UEdGraph* DocumentID) const OVERRIDE
	{
		return TAttribute<FText>( FText::FromString( DocumentID->GetName() ) );
	}

	virtual TSharedRef<SWidget> CreateTabBodyForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* DocumentID) const OVERRIDE
	{
		return OnCreateGraphEditorWidget.Execute(DocumentID);
	}

	virtual const FSlateBrush* GetTabIconForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* DocumentID) const OVERRIDE
	{
		return FEditorStyle::GetBrush("NoBrush");
	}

protected:
	TWeakPtr<class FBehaviorTreeEditor> BehaviorTreeEditorPtr;
	FOnCreateGraphEditorWidget OnCreateGraphEditorWidget;
};

//////////////////////////////////////////////////////////////////////////
class FBTCommonCommands : public TCommands<FBTCommonCommands>
{
public:
	/** Constructor */
	FBTCommonCommands() 
		: TCommands<FBTCommonCommands>("BTEditor.Common", LOCTEXT("Common", "Common"), NAME_None, FEditorStyle::GetStyleSetName())
	{
	}

	TSharedPtr<FUICommandInfo> SearchBT;

	/** Initialize commands */
	virtual void RegisterCommands() OVERRIDE;
};

void FBTCommonCommands::RegisterCommands()
{
	UI_COMMAND(SearchBT, "Search", "Search this Behavior Tree.", EUserInterfaceActionType::Button, FInputGesture(EModifierKey::Control, EKeys::F));
}

//////////////////////////////////////////////////////////////////////////
class FBTDebuggerCommands : public TCommands<FBTDebuggerCommands>
{
public:
	/** Constructor */
	FBTDebuggerCommands() 
		: TCommands<FBTDebuggerCommands>("BTEditor.Debugger", LOCTEXT("Debugger", "Debugger"), NAME_None, FEditorStyle::GetStyleSetName())
	{
	}

	TSharedPtr<FUICommandInfo> ShowPrevStep;
	TSharedPtr<FUICommandInfo> ShowNextStep;
	TSharedPtr<FUICommandInfo> Step;

	TSharedPtr<FUICommandInfo> PausePlaySession;
	TSharedPtr<FUICommandInfo> ResumePlaySession;
	TSharedPtr<FUICommandInfo> StopPlaySession;

	/** Initialize commands */
	virtual void RegisterCommands() OVERRIDE;
};

void FBTDebuggerCommands::RegisterCommands()
{
	UI_COMMAND(ShowPrevStep, "Show Prev", "Show state from previous step.", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(ShowNextStep, "Show Next", "Show state from next step.", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(Step, "Step", "Step forward and break on node change.", EUserInterfaceActionType::Button, FInputGesture());

	UI_COMMAND(PausePlaySession, "Pause", "Pause simulation", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(ResumePlaySession, "Resume", "Resume simulation", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND(StopPlaySession, "Stop", "Stop simulation", EUserInterfaceActionType::Button, FInputGesture());
}


//////////////////////////////////////////////////////////////////////////
FBehaviorTreeEditor::FBehaviorTreeEditor() 
	: IBehaviorTreeEditor()
{
	UEditorEngine* Editor = (UEditorEngine*)GEngine;
	if (Editor != NULL)
	{
		Editor->RegisterForUndo(this);
	}

	bShowDecoratorRangeLower = false;
	bShowDecoratorRangeSelf = false;
	SelectedNodesCount = 0;
}

FBehaviorTreeEditor::~FBehaviorTreeEditor()
{
	UEditorEngine* Editor = (UEditorEngine*)GEngine;
	if (Editor)
	{
		Editor->UnregisterForUndo( this );
	}

	Debugger.Reset();
}

void FBehaviorTreeEditor::PostUndo(bool bSuccess)
{	
	if (bSuccess)
	{
		// Clear selection, to avoid holding refs to nodes that go away
		TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusedGraphEdPtr.Pin();
		if (CurrentGraphEditor.IsValid())
		{
			CurrentGraphEditor->ClearSelectionSet();
			CurrentGraphEditor->NotifyGraphChanged();
		}
		FSlateApplication::Get().DismissAllMenus();
	}
}

void FBehaviorTreeEditor::PostRedo(bool bSuccess)
{
	if (bSuccess)
	{
		// Clear selection, to avoid holding refs to nodes that go away
		TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusedGraphEdPtr.Pin();
		if (CurrentGraphEditor.IsValid())
		{
			CurrentGraphEditor->ClearSelectionSet();
			CurrentGraphEditor->NotifyGraphChanged();
		}
		FSlateApplication::Get().DismissAllMenus();
	}
}

void FBehaviorTreeEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	// @todo TabManagement
	FAssetEditorToolkit::RegisterTabSpawners(TabManager);

	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	TabManager->RegisterTabSpawner( BTPropertiesTabId, FOnSpawnTab::CreateSP(this, &FBehaviorTreeEditor::SpawnTab_Properties) )
		.SetDisplayName( LOCTEXT( "PropertiesTab", "Details" ) )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );

	TabManager->RegisterTabSpawner(BTSearchTabId, FOnSpawnTab::CreateSP(this, &FBehaviorTreeEditor::SpawnTab_Search))
		.SetDisplayName(LOCTEXT("SearchTab", "Search"))
		.SetGroup(MenuStructure.GetAssetEditorCategory());

	DocumentManager.SetTabManager(TabManager);
}

void FBehaviorTreeEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	TabManager->UnregisterTabSpawner( BTPropertiesTabId );

	TabManager->UnregisterTabSpawner(BTSearchTabId);

	FAssetEditorToolkit::UnregisterTabSpawners(TabManager);
}


void FBehaviorTreeEditor::InitBehaviorTreeEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UBehaviorTree* InScript )
{
	Script = InScript;
	check(Script != NULL);

	TSharedPtr<FBehaviorTreeEditor> ThisPtr(SharedThis(this));
	DocumentManager.Initialize(ThisPtr);

	// Register the document factories
	{
		TSharedRef<FDocumentTabFactory> GraphEditorFactory = MakeShareable(new FBTGraphEditorSummoner(ThisPtr,
			FBTGraphEditorSummoner::FOnCreateGraphEditorWidget::CreateSP(this, &FBehaviorTreeEditor::CreateGraphEditorWidget)
			));

		// Also store off a reference to the grapheditor factory so we can find all the tabs spawned by it later.
		GraphEditorTabFactoryPtr = GraphEditorFactory;
		DocumentManager.RegisterDocumentFactory(GraphEditorFactory);
	}

	FGraphEditorCommands::Register();
	FBTCommonCommands::Register();
	FBTDebuggerCommands::Register();

	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout( "Standalone_BehaviorTree_Layout" )
	->AddArea
	(
		FTabManager::NewPrimaryArea() ->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewSplitter() ->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->AddTab(GetToolbarTabId(), ETabState::OpenedTab) 
				->SetHideTabWell(true) 
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.2f)
				->AddTab(BTSearchTabId, ETabState::ClosedTab)
			)			
		)
		->Split
		(
			FTabManager::NewSplitter() ->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.7f)
				->AddTab( BTGraphTabId, ETabState::ClosedTab )
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.3f)
				->AddTab( BTPropertiesTabId, ETabState::OpenedTab )
			)
		)
	);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor( Mode, InitToolkitHost, FBehaviorTreeEditorModule::BehaviorTreeEditorAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, Script );

	// Update BT asset data based on saved graph to have correct data in editor
	{
		UBehaviorTreeGraph* MyGraph = Cast<UBehaviorTreeGraph>(Script->BTGraph);
		if (MyGraph == NULL)
		{
			Script->BTGraph = FBlueprintEditorUtils::CreateNewGraph(Script, TEXT("Behavior Tree"), UBehaviorTreeGraph::StaticClass(), UEdGraphSchema_BehaviorTree::StaticClass());
			MyGraph = Cast<UBehaviorTreeGraph>(Script->BTGraph);
			check(MyGraph);

			// Initialize the behavior tree graph
			const UEdGraphSchema* Schema = MyGraph->GetSchema();
			Schema->CreateDefaultNodesForGraph(*MyGraph);
		}
		else
		{
			MyGraph->UpdatePinConnectionTypes();
		}

		MyGraph->UpdateBlackboardChange();

		TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(MyGraph);
		DocumentManager.OpenDocument(Payload, FDocumentTracker::OpenNewDocument);

		MyGraph->UpdateAsset(UBehaviorTreeGraph::ClearDebuggerFlags);

		FAbortDrawHelper EmptyMode;
		bShowDecoratorRangeLower = false;
		bShowDecoratorRangeSelf = false;
		MyGraph->UpdateAbortHighlight(EmptyMode, EmptyMode);
	}

	BindCommonCommands();
	ExtendMenu();

	Debugger = MakeShareable(new FBehaviorTreeDebugger);
	Debugger->Setup(Script, this, DebuggerView);
	BindDebuggerToolbarCommands();

	ExtentToolbar();
	RegenerateMenusAndToolbars();
}

bool FBehaviorTreeEditor::IsDebuggerReady() const
{
	return Debugger.IsValid() && Debugger->IsDebuggerReady();
}

EVisibility FBehaviorTreeEditor::GetDebuggerDetailsVisibility() const
{
	return Debugger.IsValid() && Debugger->IsDebuggerRunning() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FBehaviorTreeEditor::GetRangeLowerVisibility() const
{
	return FBehaviorTreeDebugger::IsPIENotSimulating() && bShowDecoratorRangeLower ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FBehaviorTreeEditor::GetRangeSelfVisibility() const
{
	return FBehaviorTreeDebugger::IsPIENotSimulating() && bShowDecoratorRangeSelf ? EVisibility::Visible : EVisibility::Collapsed;
}

FGraphAppearanceInfo FBehaviorTreeEditor::GetGraphAppearance() const
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText", "BEHAVIOR TREE").ToString();

	const int32 StepIdx = Debugger.IsValid() ? Debugger->GetShownStateIndex() : 0;
	if (Debugger.IsValid() && !Debugger->IsDebuggerRunning())
	{
		AppearanceInfo.PIENotifyText = TEXT("INACTIVE");
	}
	else if (StepIdx)
	{
		AppearanceInfo.PIENotifyText = FString::Printf(TEXT("%d STEP%s BACK"), StepIdx, StepIdx > 1 ? TEXT("S") : TEXT(""));
	}
	else if (FBehaviorTreeDebugger::IsPlaySessionPaused())
	{
		AppearanceInfo.PIENotifyText = TEXT("PAUSED");
	}
	
	return AppearanceInfo;
}

FName FBehaviorTreeEditor::GetToolkitFName() const
{
	return FName("Behavior Tree");
}

FText FBehaviorTreeEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "BehaviorTree");
}

FString FBehaviorTreeEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "BehaviorTree ").ToString();
}


FLinearColor FBehaviorTreeEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor( 0.0f, 0.0f, 0.2f, 0.5f );
}


/** Create new tab for the supplied graph - don't call this directly, call SExplorer->FindTabForGraph.*/
TSharedRef<SGraphEditor> FBehaviorTreeEditor::CreateGraphEditorWidget(UEdGraph* InGraph)
{
	check(InGraph != NULL);
	
	if (!GraphEditorCommands.IsValid())
	{
		GraphEditorCommands = MakeShareable( new FUICommandList );

		// Editing commands
		GraphEditorCommands->MapAction( FGenericCommands::Get().SelectAll,
			FExecuteAction::CreateSP( this, &FBehaviorTreeEditor::SelectAllNodes ),
			FCanExecuteAction::CreateSP( this, &FBehaviorTreeEditor::CanSelectAllNodes )
			);

		GraphEditorCommands->MapAction( FGenericCommands::Get().Delete,
			FExecuteAction::CreateSP( this, &FBehaviorTreeEditor::DeleteSelectedNodes ),
			FCanExecuteAction::CreateSP( this, &FBehaviorTreeEditor::CanDeleteNodes )
			);

		GraphEditorCommands->MapAction( FGenericCommands::Get().Copy,
			FExecuteAction::CreateSP( this, &FBehaviorTreeEditor::CopySelectedNodes ),
			FCanExecuteAction::CreateSP( this, &FBehaviorTreeEditor::CanCopyNodes )
			);

		GraphEditorCommands->MapAction( FGenericCommands::Get().Cut,
			FExecuteAction::CreateSP( this, &FBehaviorTreeEditor::CutSelectedNodes ),
			FCanExecuteAction::CreateSP( this, &FBehaviorTreeEditor::CanCutNodes )
			);

		GraphEditorCommands->MapAction( FGenericCommands::Get().Paste,
			FExecuteAction::CreateSP( this, &FBehaviorTreeEditor::PasteNodes ),
			FCanExecuteAction::CreateSP( this, &FBehaviorTreeEditor::CanPasteNodes )
			);

		GraphEditorCommands->MapAction( FGenericCommands::Get().Duplicate,
			FExecuteAction::CreateSP( this, &FBehaviorTreeEditor::DuplicateNodes ),
			FCanExecuteAction::CreateSP( this, &FBehaviorTreeEditor::CanDuplicateNodes )
			);

		GraphEditorCommands->MapAction( FGraphEditorCommands::Get().RemoveExecutionPin,
			FExecuteAction::CreateSP( this, &FBehaviorTreeEditor::OnRemoveInputPin ),
			FCanExecuteAction::CreateSP( this, &FBehaviorTreeEditor::CanRemoveInputPin )
			);

		GraphEditorCommands->MapAction( FGraphEditorCommands::Get().AddExecutionPin,
			FExecuteAction::CreateSP( this, &FBehaviorTreeEditor::OnAddInputPin ),
			FCanExecuteAction::CreateSP( this, &FBehaviorTreeEditor::CanAddInputPin )
			);

		// Debug actions
		GraphEditorCommands->MapAction( FGraphEditorCommands::Get().AddBreakpoint,
			FExecuteAction::CreateSP( this, &FBehaviorTreeEditor::OnAddBreakpoint ),
			FCanExecuteAction::CreateSP( this, &FBehaviorTreeEditor::CanAddBreakpoint ),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP( this, &FBehaviorTreeEditor::CanAddBreakpoint )
			);

		GraphEditorCommands->MapAction( FGraphEditorCommands::Get().RemoveBreakpoint,
			FExecuteAction::CreateSP( this, &FBehaviorTreeEditor::OnRemoveBreakpoint ),
			FCanExecuteAction::CreateSP( this, &FBehaviorTreeEditor::CanRemoveBreakpoint ),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP( this, &FBehaviorTreeEditor::CanRemoveBreakpoint )
			);

		GraphEditorCommands->MapAction( FGraphEditorCommands::Get().EnableBreakpoint,
			FExecuteAction::CreateSP( this, &FBehaviorTreeEditor::OnEnableBreakpoint ),
			FCanExecuteAction::CreateSP( this, &FBehaviorTreeEditor::CanEnableBreakpoint ),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP( this, &FBehaviorTreeEditor::CanEnableBreakpoint )
			);

		GraphEditorCommands->MapAction( FGraphEditorCommands::Get().DisableBreakpoint,
			FExecuteAction::CreateSP( this, &FBehaviorTreeEditor::OnDisableBreakpoint ),
			FCanExecuteAction::CreateSP( this, &FBehaviorTreeEditor::CanDisableBreakpoint ),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP( this, &FBehaviorTreeEditor::CanDisableBreakpoint )
			);

		GraphEditorCommands->MapAction( FGraphEditorCommands::Get().ToggleBreakpoint,
			FExecuteAction::CreateSP( this, &FBehaviorTreeEditor::OnToggleBreakpoint ),
			FCanExecuteAction::CreateSP( this, &FBehaviorTreeEditor::CanToggleBreakpoint ),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateSP( this, &FBehaviorTreeEditor::CanToggleBreakpoint )
			);
	}

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FBehaviorTreeEditor::OnSelectedNodesChanged);
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FBehaviorTreeEditor::OnNodeDoubleClicked);

	// Make title bar
	TSharedRef<SWidget> TitleBarWidget = 
		SNew(SBorder)
		.BorderImage( FEditorStyle::GetBrush( TEXT("Graph.TitleBackground") ) )
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.FillWidth(1.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("BehaviorTreeGraphLabel", "Behavior Tree"))
				.TextStyle( FEditorStyle::Get(), TEXT("GraphBreadcrumbButtonText") )
			]
		];

	// Make full graph editor
	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(this, &FBehaviorTreeEditor::InEditingMode)
		.Appearance(this, &FBehaviorTreeEditor::GetGraphAppearance)
		.TitleBar(TitleBarWidget)
		.GraphToEdit(InGraph)
		.GraphEvents(InEvents);
}

bool FBehaviorTreeEditor::InEditingMode() const
{
	return FBehaviorTreeDebugger::IsPIENotSimulating();
}

TSharedRef<SDockTab> FBehaviorTreeEditor::SpawnTab_Search(const FSpawnTabArgs& Args)
{
	FindResults = SNew(SFindInBT, SharedThis(this));

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("SoundClassEditor.Tabs.Properties"))
		.Label(LOCTEXT("BehaviorTreeEditorSearchTitle", "Search"))
		[
			FindResults.ToSharedRef()
		];

	return SpawnedTab;
}

TSharedRef<SDockTab> FBehaviorTreeEditor::SpawnTab_Properties(const FSpawnTabArgs& Args)
{
	check( Args.GetTabId() == BTPropertiesTabId );

	CreateInternalWidgets();

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("SoundClassEditor.Tabs.Properties") )
		.Label( LOCTEXT( "SoundClassPropertiesTitle", "Details" ) )
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			[
				DetailsView.ToSharedRef()
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.HAlign(HAlign_Fill)
				.Padding(0.0f, 5.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(BehaviorTreeColors::NodeBorder::HighlightAbortRange0)
					.BorderImage( FEditorStyle::GetBrush( "Graph.StateNode.Body" ) )
					.Visibility(this, &FBehaviorTreeEditor::GetRangeLowerVisibility)
					.Padding(FMargin(5.0f))
					[
						SNew(STextBlock)
						.Text(FString("Nodes aborted by mode: Lower Priority"))
					]
				]
				+SVerticalBox::Slot()
				.HAlign(HAlign_Fill)
				.Padding(0.0f, 5.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(BehaviorTreeColors::NodeBorder::HighlightAbortRange1)
					.BorderImage( FEditorStyle::GetBrush( "Graph.StateNode.Body" ) )
					.Visibility(this, &FBehaviorTreeEditor::GetRangeSelfVisibility)
					.Padding(FMargin(5.0f))
					[
						SNew(STextBlock)
						.Text(FString("Nodes aborted by mode: Self"))
					]
				]
			]
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Bottom)
			.Padding(5.0f)
			[
				SNew(SSeparator)
				.Orientation( Orient_Horizontal )
				.Visibility(this, &FBehaviorTreeEditor::GetDebuggerDetailsVisibility)
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			[
				DebuggerView.ToSharedRef()
			]
		];

	return SpawnedTab;
}

void FBehaviorTreeEditor::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	TArray<UObject*> Selection;
	
	UBehaviorTreeGraphNode_CompositeDecorator* FoundGraphNode_CompDecorator = NULL;
	UBTDecorator* FoundDecorator = NULL;

	SelectedNodesCount = NewSelection.Num();
	if(NewSelection.Num())
	{
		for(TSet<class UObject*>::TConstIterator SetIt(NewSelection);SetIt;++SetIt)
		{
			UBehaviorTreeGraphNode_Composite* GraphNode_Composite = Cast<UBehaviorTreeGraphNode_Composite>(*SetIt);
			if (GraphNode_Composite)
			{
				Selection.Add(GraphNode_Composite->NodeInstance);
				continue;
			}

			UBehaviorTreeGraphNode_Task* GraphNode_Task = Cast<UBehaviorTreeGraphNode_Task>(*SetIt);
			if (GraphNode_Task)
			{
				Selection.Add(GraphNode_Task->NodeInstance);
				continue;
			}

			UBehaviorTreeGraphNode_Decorator* GraphNode_Decorator1 = Cast<UBehaviorTreeGraphNode_Decorator>(*SetIt);
			if (GraphNode_Decorator1)
			{
				Selection.Add(GraphNode_Decorator1->NodeInstance);
				FoundDecorator = Cast<UBTDecorator>(GraphNode_Decorator1->NodeInstance);
				continue;
			}

			UBehaviorTreeDecoratorGraphNode_Decorator* GraphNode_Decorator2 = Cast<UBehaviorTreeDecoratorGraphNode_Decorator>(*SetIt);
			if (GraphNode_Decorator2)
			{
				Selection.Add(GraphNode_Decorator2->NodeInstance);
				continue;
			}
			
			UBehaviorTreeGraphNode_Service* GraphNode_Service = Cast<UBehaviorTreeGraphNode_Service>(*SetIt);
			if (GraphNode_Service)
			{
				Selection.Add(GraphNode_Service->NodeInstance);
				continue;
			}

			UBehaviorTreeGraphNode_CompositeDecorator* GraphNode_CompDecorator = Cast<UBehaviorTreeGraphNode_CompositeDecorator>(*SetIt);
			if (GraphNode_CompDecorator)
			{
				FoundGraphNode_CompDecorator = GraphNode_CompDecorator;
			}

			Selection.Add(*SetIt);
		}
	}

	UBehaviorTreeGraph* MyGraph = Cast<UBehaviorTreeGraph>(Script->BTGraph);
	FAbortDrawHelper Mode0, Mode1;
	bShowDecoratorRangeLower = false;
	bShowDecoratorRangeSelf = false;

	if (Selection.Num() == 1)
	{
		if (DetailsView.IsValid())
		{
			DetailsView->SetObjects(Selection);
		}

		if (FoundDecorator)
		{
			GetAbortModePreview(FoundDecorator, Mode0, Mode1);
		}
		else if (FoundGraphNode_CompDecorator)
		{
			GetAbortModePreview(FoundGraphNode_CompDecorator, Mode0, Mode1);
		}
	}
	else if (DetailsView.IsValid())
	{
		DetailsView->SetObject(NULL);
	}

	MyGraph->UpdateAbortHighlight(Mode0, Mode1);
}

static uint16 GetMaxAllowedRange(const class UBTDecorator* DecoratorOb)
{
	uint16 MaxRange = MAX_uint16;
	
	UBTCompositeNode* TestParent = DecoratorOb->GetParentNode();
	while (TestParent)
	{
		if (TestParent->IsA(UBTComposite_SimpleParallel::StaticClass()))
		{
			MaxRange = TestParent->GetLastExecutionIndex();
			break;
		}

		TestParent = TestParent->GetParentNode();
	}

	return MaxRange;
}

static void FillAbortPreview_LowerPriority(const class UBTDecorator* DecoratorOb, const UBTCompositeNode* DecoratorParent, FAbortDrawHelper& Mode)
{
	const uint16 MaxRange = GetMaxAllowedRange(DecoratorOb);

	Mode.AbortStart = DecoratorParent->GetChildExecutionIndex(DecoratorOb->GetChildIndex() + 1);
	Mode.AbortEnd = MaxRange;
	Mode.SearchStart = DecoratorParent->GetExecutionIndex();
	Mode.SearchEnd = MaxRange;
}

static void FillAbortPreview_Self(const class UBTDecorator* DecoratorOb, const UBTCompositeNode* DecoratorParent, FAbortDrawHelper& Mode)
{
	const uint16 MaxRange = GetMaxAllowedRange(DecoratorOb);

	Mode.AbortStart = DecoratorOb->GetExecutionIndex();
	Mode.AbortEnd = DecoratorParent->GetChildExecutionIndex(DecoratorOb->GetChildIndex() + 1) - 1;
	Mode.SearchStart = DecoratorParent->GetChildExecutionIndex(DecoratorOb->GetChildIndex() + 1);
	Mode.SearchEnd = MaxRange;
}

void FBehaviorTreeEditor::GetAbortModePreview(const class UBehaviorTreeGraphNode_CompositeDecorator* Node, struct FAbortDrawHelper& Mode0, struct FAbortDrawHelper& Mode1)
{
	Mode0.SearchStart = MAX_uint16;
	Mode0.AbortStart = MAX_uint16;
	Mode1.SearchStart = MAX_uint16;
	Mode1.AbortStart = MAX_uint16;

	TArray<UBTDecorator*> Decorators;
	TArray<FBTDecoratorLogic> Operations;
	Node->CollectDecoratorData(Decorators, Operations);

	int32 LowerPriIdx = INDEX_NONE;
	int32 SelfIdx = INDEX_NONE;
	
	for (int32 i = 0; i < Decorators.Num(); i++)
	{
		EBTFlowAbortMode::Type FlowAbort = Decorators[i]->GetParentNode() ? Decorators[i]->GetFlowAbortMode() : EBTFlowAbortMode::None;

		if (FlowAbort == EBTFlowAbortMode::LowerPriority || FlowAbort == EBTFlowAbortMode::Both)
		{
			LowerPriIdx = i;
		}
		
		if (FlowAbort == EBTFlowAbortMode::Self || FlowAbort == EBTFlowAbortMode::Both)
		{
			SelfIdx = i;
		}
	}

	if (Decorators.IsValidIndex(LowerPriIdx))
	{
		FillAbortPreview_LowerPriority(Decorators[LowerPriIdx], Decorators[LowerPriIdx]->GetParentNode(), Mode0);
		bShowDecoratorRangeLower = true;
	}

	if (Decorators.IsValidIndex(SelfIdx))
	{
		FillAbortPreview_Self(Decorators[SelfIdx], Decorators[SelfIdx]->GetParentNode(), Mode1);
		bShowDecoratorRangeSelf = true;
	}
}

void FBehaviorTreeEditor::GetAbortModePreview(const class UBTDecorator* DecoratorOb, FAbortDrawHelper& Mode0, FAbortDrawHelper& Mode1)
{
	const UBTCompositeNode* DecoratorParent = DecoratorOb ? DecoratorOb->GetParentNode() : NULL;
	EBTFlowAbortMode::Type FlowAbort = DecoratorParent ? DecoratorOb->GetFlowAbortMode() : EBTFlowAbortMode::None;

	Mode0.SearchStart = MAX_uint16;
	Mode0.AbortStart = MAX_uint16;
	Mode1.SearchStart = MAX_uint16;
	Mode1.AbortStart = MAX_uint16;

	switch (FlowAbort)
	{
		case EBTFlowAbortMode::LowerPriority:
			FillAbortPreview_LowerPriority(DecoratorOb, DecoratorParent, Mode0);
			bShowDecoratorRangeLower = true;
			break;

		case EBTFlowAbortMode::Self:
			FillAbortPreview_Self(DecoratorOb, DecoratorParent, Mode1);
			bShowDecoratorRangeSelf = true;
			break;

		case EBTFlowAbortMode::Both:
			FillAbortPreview_LowerPriority(DecoratorOb, DecoratorParent, Mode0);
			FillAbortPreview_Self(DecoratorOb, DecoratorParent, Mode1);
			bShowDecoratorRangeLower = true;
			bShowDecoratorRangeSelf = true;
			break;

		default:
			break;
	}
}

void FBehaviorTreeEditor::CreateInternalWidgets()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>( "PropertyEditor" );
	const FDetailsViewArgs DetailsViewArgs( false, false, true, true, false );
	DetailsView = PropertyEditorModule.CreateDetailView( DetailsViewArgs );
	DetailsView->SetObject( NULL );
	DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &FBehaviorTreeEditor::IsPropertyVisible));
	DetailsView->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateStatic(&FBehaviorTreeDebugger::IsPIENotSimulating));
	DetailsView->OnFinishedChangingProperties().AddSP(this, &FBehaviorTreeEditor::OnFinishedChangingProperties);

	DebuggerView = SNew(SBehaviorTreeDebuggerView);
}

void FBehaviorTreeEditor::ExtendMenu()
{
	struct Local
	{
		static void FillEditMenu(FMenuBuilder& MenuBuilder)
		{
			MenuBuilder.BeginSection("EditSearch", LOCTEXT("EditMenu_SearchHeading", "Search"));
			{
				MenuBuilder.AddMenuEntry(FBTCommonCommands::Get().SearchBT);
			}
			MenuBuilder.EndSection();
		}
	};

	TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender);

	// Extend the Edit menu
	MenuExtender->AddMenuExtension(
		"EditHistory",
		EExtensionHook::After,
		GetToolkitCommands(),
		FMenuExtensionDelegate::CreateStatic(&Local::FillEditMenu));

	AddMenuExtender(MenuExtender);
}

void FBehaviorTreeEditor::BindCommonCommands()
{
	ToolkitCommands->MapAction(FBTCommonCommands::Get().SearchBT,
			FExecuteAction::CreateSP(this, &FBehaviorTreeEditor::SearchTree),
			FCanExecuteAction::CreateSP(this, &FBehaviorTreeEditor::CanSearchTree)
			);
}

void FBehaviorTreeEditor::SearchTree()
{
	TabManager->InvokeTab(BTSearchTabId);
	FindResults->FocusForUse();
}

bool FBehaviorTreeEditor::CanSearchTree() const
{
	return true;
}

void FBehaviorTreeEditor::ExtentToolbar()
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder, FBehaviorTreeEditor* EditorOwner, TSharedRef<SWidget> SelectionBox)
		{
			const bool bCanShowDebugger = EditorOwner && EditorOwner->IsDebuggerReady();
			if (bCanShowDebugger)
			{
				ToolbarBuilder.BeginSection("CachedState");
				{
					ToolbarBuilder.AddToolBarButton(FBTDebuggerCommands::Get().ShowPrevStep);
					ToolbarBuilder.AddToolBarButton(FBTDebuggerCommands::Get().ShowNextStep);
					ToolbarBuilder.AddToolBarButton(FBTDebuggerCommands::Get().Step);
				}
				ToolbarBuilder.EndSection();
				ToolbarBuilder.BeginSection("World");
				{
					ToolbarBuilder.AddToolBarButton(FBTDebuggerCommands::Get().PausePlaySession);
					ToolbarBuilder.AddToolBarButton(FBTDebuggerCommands::Get().ResumePlaySession);
					ToolbarBuilder.AddToolBarButton(FBTDebuggerCommands::Get().StopPlaySession);
					ToolbarBuilder.AddSeparator();
					ToolbarBuilder.AddWidget(SelectionBox);
				}
				ToolbarBuilder.EndSection();
			}
		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	TSharedRef<SWidget> SelectionBox = SNew(SComboButton)
		.OnGetMenuContent( this, &FBehaviorTreeEditor::OnGetDebuggerActorsMenu )
		.ButtonContent()
		[
			SNew(STextBlock)
			.ToolTipText( LOCTEXT("SelectDebugActor", "Pick actor to debug") )
			.Text(this, &FBehaviorTreeEditor::GetDebuggerActorDesc )
		];

	ToolbarExtender->AddToolBarExtension("Asset", EExtensionHook::After, GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic( &Local::FillToolbar, this, SelectionBox )
		);

	AddToolbarExtender(ToolbarExtender);

	FBehaviorTreeEditorModule& BehaviorTreeEditorModule = FModuleManager::LoadModuleChecked<FBehaviorTreeEditorModule>( "BehaviorTreeEditor" );
	AddMenuExtender(BehaviorTreeEditorModule.GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
}

TSharedRef<class SWidget> FBehaviorTreeEditor::OnGetDebuggerActorsMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);

	if (Debugger.IsValid())
	{
		TArray<UBehaviorTreeComponent*> MatchingInstances;
		Debugger->GetMatchingInstances(MatchingInstances);

		// Fill the combo menu with presets of common screen resolutions
		for (int32 i = 0; i < MatchingInstances.Num(); i++)
		{
			const FText ActorDesc = FText::FromString( Debugger->DescribeInstance(MatchingInstances[i]) );
			TWeakObjectPtr<UBehaviorTreeComponent> InstancePtr = MatchingInstances[i];

			FUIAction ItemAction(FExecuteAction::CreateSP(this, &FBehaviorTreeEditor::OnDebuggerActorSelected, InstancePtr));
			MenuBuilder.AddMenuEntry(ActorDesc, TAttribute<FText>(), FSlateIcon(), ItemAction);
		}

		// Failsafe when no actor match
		if (MatchingInstances.Num() == 0)
		{
			const FText ActorDesc = LOCTEXT("NoMatchForDebug","Can't find matching actors");
			TWeakObjectPtr<UBehaviorTreeComponent> InstancePtr;

			FUIAction ItemAction(FExecuteAction::CreateSP(this, &FBehaviorTreeEditor::OnDebuggerActorSelected, InstancePtr));
			MenuBuilder.AddMenuEntry(ActorDesc, TAttribute<FText>(), FSlateIcon(), ItemAction);
		}
	}

	return MenuBuilder.MakeWidget();
}

void FBehaviorTreeEditor::OnDebuggerActorSelected(TWeakObjectPtr<UBehaviorTreeComponent> InstanceToDebug)
{
	if (Debugger.IsValid())
	{
		Debugger->OnInstanceSelectedInDropdown(InstanceToDebug.Get());
	}
}

FString FBehaviorTreeEditor::GetDebuggerActorDesc() const
{
	return Debugger.IsValid() ? Debugger->GetDebuggedInstanceDesc() : FString();
}

void FBehaviorTreeEditor::BindDebuggerToolbarCommands()
{
	const FBTDebuggerCommands& Commands = FBTDebuggerCommands::Get();
	TSharedRef<FBehaviorTreeDebugger> DebuggerOb = Debugger.ToSharedRef();

	ToolkitCommands->MapAction(
		Commands.ShowNextStep,
		FExecuteAction::CreateSP(DebuggerOb, &FBehaviorTreeDebugger::ShowNextStep),
		FCanExecuteAction::CreateSP(DebuggerOb, &FBehaviorTreeDebugger::CanShowNextStep));

	ToolkitCommands->MapAction(
		Commands.ShowPrevStep,
		FExecuteAction::CreateSP(DebuggerOb, &FBehaviorTreeDebugger::ShowPrevStep),
		FCanExecuteAction::CreateSP(DebuggerOb, &FBehaviorTreeDebugger::CanShowPrevStep));

	ToolkitCommands->MapAction(
		Commands.Step,
		FExecuteAction::CreateSP(DebuggerOb, &FBehaviorTreeDebugger::StepForward),
		FCanExecuteAction::CreateSP(DebuggerOb, &FBehaviorTreeDebugger::CanStepFoward));

	ToolkitCommands->MapAction(
		Commands.PausePlaySession,
		FExecuteAction::CreateStatic(&FBehaviorTreeDebugger::PausePlaySession),
		FCanExecuteAction::CreateStatic(&FBehaviorTreeDebugger::IsPlaySessionRunning),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic(&FBehaviorTreeDebugger::IsPlaySessionRunning));

	ToolkitCommands->MapAction(
		Commands.ResumePlaySession,
		FExecuteAction::CreateStatic(&FBehaviorTreeDebugger::ResumePlaySession),
		FCanExecuteAction::CreateStatic(&FBehaviorTreeDebugger::IsPlaySessionPaused),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic(&FBehaviorTreeDebugger::IsPlaySessionPaused));

	ToolkitCommands->MapAction(
		Commands.StopPlaySession,
		FExecuteAction::CreateStatic(&FBehaviorTreeDebugger::StopPlaySession));
}

bool FBehaviorTreeEditor::IsPropertyVisible(UProperty const * const InProperty) const
{
	return !InProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance);
}

void FBehaviorTreeEditor::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (FocusedGraphOwner.IsValid())
	{
		FocusedGraphOwner->OnInnerGraphChanged();
	}

	// update abort range highlight when changing decorator's flow abort mode
	if (PropertyChangedEvent.Property &&
		(PropertyChangedEvent.Property->GetFName() == TEXT("FlowAbortMode") || PropertyChangedEvent.Property->GetFName() == TEXT("bRestartToRequestedNode")))
	{
		bShowDecoratorRangeLower = false;
		bShowDecoratorRangeSelf = false;

		FGraphPanelSelectionSet CurrentSelection = GetSelectedNodes();
		if (CurrentSelection.Num() == 1)
		{
			for (FGraphPanelSelectionSet::TConstIterator It(CurrentSelection); It; ++It)
			{
				UBehaviorTreeGraphNode_Decorator* DecoratorNode = Cast<UBehaviorTreeGraphNode_Decorator>(*It);
				if (DecoratorNode)
				{
					FAbortDrawHelper Mode0, Mode1;
					GetAbortModePreview(Cast<UBTDecorator>(DecoratorNode->NodeInstance), Mode0, Mode1);

					UBehaviorTreeGraph* MyGraph = Cast<UBehaviorTreeGraph>(Script->BTGraph);
					MyGraph->UpdateAbortHighlight(Mode0, Mode1);
				}
			}			
		}
	}
}

FGraphPanelSelectionSet FBehaviorTreeEditor::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (FocusedGraphEd.IsValid())
	{
		CurrentSelection = FocusedGraphEd->GetSelectedNodes();
	}
	return CurrentSelection;
}

void FBehaviorTreeEditor::SelectAllNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusedGraphEdPtr.Pin();
	if (CurrentGraphEditor.IsValid())
	{
		CurrentGraphEditor->SelectAllNodes();
	}
}

bool FBehaviorTreeEditor::CanSelectAllNodes() const
{
	return true;
}

void FBehaviorTreeEditor::DeleteSelectedNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusedGraphEdPtr.Pin();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction( FGenericCommands::Get().Delete->GetDescription() );
	CurrentGraphEditor->GetCurrentGraph()->Modify();

	const FGraphPanelSelectionSet SelectedNodes = CurrentGraphEditor->GetSelectedNodes();
	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt( SelectedNodes ); NodeIt; ++NodeIt)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt))
		{
			if (Node->CanUserDeleteNode())
			{
				Node->Modify();
				Node->DestroyNode();
			}
		}
	}
}

bool FBehaviorTreeEditor::CanDeleteNodes() const
{
	// If any of the nodes can be deleted then we should allow deleting
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanUserDeleteNode())
		{
			return true;
		}
	}

	return false;
}

void FBehaviorTreeEditor::DeleteSelectedDuplicatableNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusedGraphEdPtr.Pin();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	const FGraphPanelSelectionSet OldSelectedNodes = CurrentGraphEditor->GetSelectedNodes();
	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}

	// Delete the duplicatable nodes
	DeleteSelectedNodes();

	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}
}

void FBehaviorTreeEditor::CutSelectedNodes()
{
	CopySelectedNodes();
	DeleteSelectedDuplicatableNodes();
}

bool FBehaviorTreeEditor::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FBehaviorTreeEditor::CopySelectedNodes()
{
	// Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	TArray<UBehaviorTreeGraphNode*> SubNodes;

	FString ExportedText;

	int32 CopySubNodeIndex = 0;
	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UBehaviorTreeGraphNode* Node = Cast<UBehaviorTreeGraphNode>(*SelectedIter);
		
		// skip all manually selected subnodes
		if (Node == NULL || Node->IsSubNode())
		{
			SelectedIter.RemoveCurrent();
			continue;
		}

		Node->PrepareForCopying();
		Node->CopySubNodeIndex = CopySubNodeIndex;

		// append all subnodes for selection
		for (int32 i = 0; i < Node->Decorators.Num(); i++)
		{
			Node->Decorators[i]->CopySubNodeIndex = CopySubNodeIndex;
			SubNodes.Add(Node->Decorators[i]);
		}

		for (int32 i = 0; i < Node->Services.Num(); i++)
		{
			Node->Services[i]->CopySubNodeIndex = CopySubNodeIndex;
			SubNodes.Add(Node->Services[i]);
		}

		CopySubNodeIndex++;
	}

	for (int32 i = 0; i < SubNodes.Num(); i++)
	{
		SelectedNodes.Add(SubNodes[i]);
		SubNodes[i]->PrepareForCopying();
	}

	FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
	FPlatformMisc::ClipboardCopy(*ExportedText);

	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UBehaviorTreeGraphNode* Node = Cast<UBehaviorTreeGraphNode>(*SelectedIter);
		Node->PostCopyNode();
	}
}

bool FBehaviorTreeEditor::CanCopyNodes() const
{
	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			return true;
		}
	}

	return false;
}

void FBehaviorTreeEditor::PasteNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusedGraphEdPtr.Pin();
	if (CurrentGraphEditor.IsValid())
	{
		PasteNodesHere(CurrentGraphEditor->GetPasteLocation());
	}
}

void FBehaviorTreeEditor::PasteNodesHere(const FVector2D& Location)
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusedGraphEdPtr.Pin();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	// Undo/Redo support
	const FScopedTransaction Transaction( FGenericCommands::Get().Paste->GetDescription() );
	UEdGraph* BTGraph = CurrentGraphEditor->GetCurrentGraph();
	BTGraph->Modify();

	// Clear the selection set (newly pasted stuff will be selected)
	CurrentGraphEditor->ClearSelectionSet();

	// Grab the text to paste from the clipboard.
	FString TextToImport;
	FPlatformMisc::ClipboardPaste(TextToImport);

	// Import the nodes
	TSet<UEdGraphNode*> PastedNodes;
	FEdGraphUtilities::ImportNodesFromText(BTGraph, TextToImport, /*out*/ PastedNodes);

	//Average position of nodes so we can move them while still maintaining relative distances to each other
	FVector2D AvgNodePosition(0.0f,0.0f);

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(*It);
		if (BTNode && !BTNode->IsSubNode())
		{
			AvgNodePosition.X += BTNode->NodePosX;
			AvgNodePosition.Y += BTNode->NodePosY;
		}
	}

	if (PastedNodes.Num() > 0)
	{
		float InvNumNodes = 1.0f/float(PastedNodes.Num());
		AvgNodePosition.X *= InvNumNodes;
		AvgNodePosition.Y *= InvNumNodes;
	}

	TMap<int32, UBehaviorTreeGraphNode*> ParentMap;
	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(*It);
		if (BTNode && !BTNode->IsSubNode())
		{
			// Select the newly pasted stuff
			CurrentGraphEditor->SetNodeSelection(BTNode, true);

			BTNode->NodePosX = (BTNode->NodePosX - AvgNodePosition.X) + Location.X ;
			BTNode->NodePosY = (BTNode->NodePosY - AvgNodePosition.Y) + Location.Y ;

			BTNode->SnapToGrid(16);

			// Give new node a different Guid from the old one
			BTNode->CreateNewGuid();

			BTNode->Decorators.Reset();
			BTNode->Services.Reset();

			ParentMap.Add(BTNode->CopySubNodeIndex, BTNode);
		}
	}

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(*It);
		if (BTNode && BTNode->IsSubNode())
		{
			BTNode->NodePosX = 0;
			BTNode->NodePosY = 0;

			// Give new node a different Guid from the old one
			BTNode->CreateNewGuid();

			// remove subnode from graph, it will be referenced from parent node
			BTNode->DestroyNode();

			BTNode->ParentNode = ParentMap.FindRef(BTNode->CopySubNodeIndex);
			if (BTNode->ParentNode)
			{
				const bool bIsService = BTNode->IsA(UBehaviorTreeGraphNode_Service::StaticClass());
				if (bIsService)
				{
					BTNode->ParentNode->Services.Add(BTNode);
				}
				else
				{
					BTNode->ParentNode->Decorators.Add(BTNode);
				}
			}
		}
	}

	// Update UI
	CurrentGraphEditor->NotifyGraphChanged();

	Script->PostEditChange();
	Script->MarkPackageDirty();
}

bool FBehaviorTreeEditor::CanPasteNodes() const
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusedGraphEdPtr.Pin();
	if (!CurrentGraphEditor.IsValid())
	{
		return false;
	}

	FString ClipboardContent;
	FPlatformMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(CurrentGraphEditor->GetCurrentGraph(), ClipboardContent);
}

void FBehaviorTreeEditor::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

bool FBehaviorTreeEditor::CanDuplicateNodes() const
{
	return CanCopyNodes();
}

void FBehaviorTreeEditor::OnNodeDoubleClicked(class UEdGraphNode* Node)
{
	if (UBehaviorTreeGraphNode_CompositeDecorator* Decorator = Cast<UBehaviorTreeGraphNode_CompositeDecorator>(Node))
	{
		if (Decorator->GetBoundGraph())
		{
			TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(const_cast<UEdGraph*>(Decorator->GetBoundGraph()));
			
			TArray< TSharedPtr<SDockTab> > MatchingTabs;
			DocumentManager.FindMatchingTabs(Payload, MatchingTabs);
			if (MatchingTabs.Num())
			{
				DocumentManager.CloseTab(Payload);
				DocumentManager.OpenDocument(Payload, FDocumentTracker::RestorePreviousDocument);
			}
			else
			{
				DocumentManager.OpenDocument(Payload, FDocumentTracker::OpenNewDocument);
			}
		}
	}
	else if (UBehaviorTreeGraphNode_Task* Task = Cast<UBehaviorTreeGraphNode_Task>(Node)) 
	{
		if (UBTTask_RunBehavior* RunTask = Cast<UBTTask_RunBehavior>(Task->NodeInstance))
		{
			if (RunTask->BehaviorAsset != NULL)
			{
				FAssetEditorManager::Get().OpenEditorForAsset(RunTask->BehaviorAsset);

				IBehaviorTreeEditor* ChildNodeEditor = static_cast<IBehaviorTreeEditor*>(FAssetEditorManager::Get().FindEditorForAsset(RunTask->BehaviorAsset, true));
				if (ChildNodeEditor)
				{
					ChildNodeEditor->InitializeDebuggerState(Debugger.Get());
				}
			}
		}
	}

	UBehaviorTreeGraphNode* MyNode = Cast<UBehaviorTreeGraphNode>(Node);
	if (MyNode && MyNode->NodeInstance &&
		MyNode->NodeInstance->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		UClass* NodeClass = MyNode->NodeInstance->GetClass();
		UPackage* Pkg = NodeClass->GetOuterUPackage();
		FString ClassName = NodeClass->GetName().LeftChop(2);
		UBlueprint* BlueprintOb = FindObject<UBlueprint>(Pkg, *ClassName);

		FAssetEditorManager::Get().OpenEditorForAsset(BlueprintOb);
	}
}

void FBehaviorTreeEditor::OnAddInputPin()
{
	FGraphPanelSelectionSet CurrentSelection;
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (FocusedGraphEd.IsValid())
	{
		CurrentSelection = FocusedGraphEd->GetSelectedNodes();
	}

	// Iterate over all nodes, and add the pin
	for (FGraphPanelSelectionSet::TConstIterator It(CurrentSelection); It; ++It)
	{
		UBehaviorTreeDecoratorGraphNode_Logic* LogicNode = Cast<UBehaviorTreeDecoratorGraphNode_Logic>(*It);
		if (LogicNode)
		{
			const FScopedTransaction Transaction( LOCTEXT("AddInputPin", "Add Input Pin") );

			LogicNode->Modify();
			LogicNode->AddInputPin();

			const UEdGraphSchema* Schema = LogicNode->GetSchema();
			Schema->ReconstructNode(*LogicNode);
		}
	}

	// Refresh the current graph, so the pins can be updated
	if (FocusedGraphEd.IsValid())
	{
		FocusedGraphEd->NotifyGraphChanged();
	}
}

bool FBehaviorTreeEditor::CanAddInputPin() const
{
	FGraphPanelSelectionSet CurrentSelection = GetSelectedNodes();
	bool bReturnValue = false;

	// Iterate over all nodes, and make sure all execution sequence nodes will always have at least 2 outs
	for (FGraphPanelSelectionSet::TConstIterator It(CurrentSelection); It; ++It)
	{
		UBehaviorTreeDecoratorGraphNode_Logic* LogicNode = Cast<UBehaviorTreeDecoratorGraphNode_Logic>(*It);
		if (LogicNode)
		{
			bReturnValue = LogicNode->CanAddPins();
			break;
		}
	}

	return bReturnValue;
};

void FBehaviorTreeEditor::OnRemoveInputPin()
{
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (FocusedGraphEd.IsValid())
	{
		const FScopedTransaction Transaction( LOCTEXT("RemoveInputPin", "Remove Input Pin") );

		UEdGraphPin* SelectedPin = FocusedGraphEd->GetGraphPinForMenu();
		UEdGraphNode* OwningNode = SelectedPin->GetOwningNode();

		OwningNode->Modify();
		SelectedPin->Modify();

		if (UBehaviorTreeDecoratorGraphNode_Logic* LogicNode = Cast<UBehaviorTreeDecoratorGraphNode_Logic>(OwningNode))
		{
			LogicNode->RemoveInputPin(SelectedPin);
		}

		// Update the graph so that the node will be refreshed
		FocusedGraphEd->NotifyGraphChanged();
	}
}

bool FBehaviorTreeEditor::CanRemoveInputPin() const
{
	FGraphPanelSelectionSet CurrentSelection = GetSelectedNodes();
	bool bReturnValue = false;

	// Iterate over all nodes, and make sure all execution sequence nodes will always have at least 2 outs
	for (FGraphPanelSelectionSet::TConstIterator It(CurrentSelection); It; ++It)
	{
		UBehaviorTreeDecoratorGraphNode_Logic* LogicNode = Cast<UBehaviorTreeDecoratorGraphNode_Logic>(*It);
		if (LogicNode)
		{
			bReturnValue = LogicNode->CanRemovePins();
			break;
		}
	}

	return bReturnValue;
};

void FBehaviorTreeEditor::OnGraphEditorFocused(const TSharedRef<SGraphEditor>& InGraphEditor)
{
	FocusedGraphEdPtr = InGraphEditor;
	FocusedGraphOwner = Cast<UBehaviorTreeGraphNode_CompositeDecorator>(InGraphEditor->GetCurrentGraph()->GetOuter());

	FGraphPanelSelectionSet CurrentSelection;
	CurrentSelection = InGraphEditor->GetSelectedNodes();
	OnSelectedNodesChanged(CurrentSelection);
}

void FBehaviorTreeEditor::SaveAsset_Execute()
{
	if (Script)
	{
		UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(Script->BTGraph);
		if (BTGraph)
		{
			BTGraph->UpdateAsset(UBehaviorTreeGraph::SkipDebuggerFlags);
		}
	}
	// save it
	IBehaviorTreeEditor::SaveAsset_Execute();
}

void FBehaviorTreeEditor::OnEnableBreakpoint()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UBehaviorTreeGraphNode* SelectedNode = Cast<UBehaviorTreeGraphNode>(*NodeIt);
		if (SelectedNode && SelectedNode->bHasBreakpoint && !SelectedNode->bIsBreakpointEnabled)
		{
			SelectedNode->bIsBreakpointEnabled = true;
			Debugger->OnBreakpointAdded(SelectedNode);
		}
	}
}

bool FBehaviorTreeEditor::CanEnableBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UBehaviorTreeGraphNode* SelectedNode = Cast<UBehaviorTreeGraphNode>(*NodeIt);
		if (SelectedNode && SelectedNode->bHasBreakpoint && !SelectedNode->bIsBreakpointEnabled)
		{
			return true;
		}
	}

	return false;
}

void FBehaviorTreeEditor::OnDisableBreakpoint()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UBehaviorTreeGraphNode* SelectedNode = Cast<UBehaviorTreeGraphNode>(*NodeIt);
		if (SelectedNode && SelectedNode->bHasBreakpoint && SelectedNode->bIsBreakpointEnabled)
		{
			SelectedNode->bIsBreakpointEnabled = false;
			Debugger->OnBreakpointRemoved(SelectedNode);
		}
	}
}

bool FBehaviorTreeEditor::CanDisableBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UBehaviorTreeGraphNode* SelectedNode = Cast<UBehaviorTreeGraphNode>(*NodeIt);
		if (SelectedNode && SelectedNode->bHasBreakpoint && SelectedNode->bIsBreakpointEnabled)
		{
			return true;
		}
	}

	return false;
}

void FBehaviorTreeEditor::OnAddBreakpoint()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UBehaviorTreeGraphNode* SelectedNode = Cast<UBehaviorTreeGraphNode>(*NodeIt);
		if (SelectedNode && SelectedNode->CanPlaceBreakpoints() && !SelectedNode->bHasBreakpoint)
		{
			SelectedNode->bHasBreakpoint = true;
			SelectedNode->bIsBreakpointEnabled = true;
			Debugger->OnBreakpointAdded(SelectedNode);
		}
	}
}

bool FBehaviorTreeEditor::CanAddBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UBehaviorTreeGraphNode* SelectedNode = Cast<UBehaviorTreeGraphNode>(*NodeIt);
		if (SelectedNode && SelectedNode->CanPlaceBreakpoints() && !SelectedNode->bHasBreakpoint)
		{
			return true;
		}
	}

	return false;
}

void FBehaviorTreeEditor::OnRemoveBreakpoint()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UBehaviorTreeGraphNode* SelectedNode = Cast<UBehaviorTreeGraphNode>(*NodeIt);
		if (SelectedNode && SelectedNode->CanPlaceBreakpoints() && SelectedNode->bHasBreakpoint)
		{
			SelectedNode->bHasBreakpoint = false;
			SelectedNode->bIsBreakpointEnabled = false;
			Debugger->OnBreakpointRemoved(SelectedNode);
		}
	}
}

bool FBehaviorTreeEditor::CanRemoveBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UBehaviorTreeGraphNode* SelectedNode = Cast<UBehaviorTreeGraphNode>(*NodeIt);
		if (SelectedNode && SelectedNode->CanPlaceBreakpoints() && SelectedNode->bHasBreakpoint)
		{
			return true;
		}
	}

	return false;
}

void FBehaviorTreeEditor::OnToggleBreakpoint()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UBehaviorTreeGraphNode* SelectedNode = Cast<UBehaviorTreeGraphNode>(*NodeIt);
		if (SelectedNode && SelectedNode->CanPlaceBreakpoints())
		{
			if (SelectedNode->bHasBreakpoint)
			{
				SelectedNode->bHasBreakpoint = false;
				SelectedNode->bIsBreakpointEnabled = false;
				Debugger->OnBreakpointRemoved(SelectedNode);
			}
			else
			{
				SelectedNode->bHasBreakpoint = true;
				SelectedNode->bIsBreakpointEnabled = true;
				Debugger->OnBreakpointAdded(SelectedNode);
			}
		}
	}
}

bool FBehaviorTreeEditor::CanToggleBreakpoint() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UBehaviorTreeGraphNode* SelectedNode = Cast<UBehaviorTreeGraphNode>(*NodeIt);
		if (SelectedNode && SelectedNode->CanPlaceBreakpoints())
		{
			return true;
		}
	}

	return false;
}

void FBehaviorTreeEditor::JumpToNode(const UEdGraphNode* Node)
{
	TSharedPtr<SGraphEditor> GraphEditor;
	TSharedPtr<SDockTab> ActiveTab = DocumentManager.GetActiveTab();
	check(ActiveTab.IsValid());
	GraphEditor = StaticCastSharedRef<SGraphEditor>(ActiveTab->GetContent());
	if (GraphEditor.IsValid())
	{
		GraphEditor->JumpToNode(Node,false);
	}
}

TWeakPtr<SGraphEditor> FBehaviorTreeEditor::GetFocusedGraphPtr() const
{
	return FocusedGraphEdPtr;
}

void FBehaviorTreeEditor::InitializeDebuggerState(class FBehaviorTreeDebugger* ParentDebugger) const
{
	if (Debugger.IsValid())
	{
		Debugger->InitializeFromParent(ParentDebugger);
	}
}

void FBehaviorTreeEditor::DebuggerSwitchAsset(UBehaviorTree* NewAsset)
{
	if (NewAsset)
	{
		FAssetEditorManager::Get().OpenEditorForAsset(NewAsset);

		IBehaviorTreeEditor* ChildNodeEditor = static_cast<IBehaviorTreeEditor*>(FAssetEditorManager::Get().FindEditorForAsset(NewAsset, true));
		if (ChildNodeEditor)
		{
			ChildNodeEditor->InitializeDebuggerState(Debugger.Get());
		}
	}
}

#undef LOCTEXT_NAMESPACE
