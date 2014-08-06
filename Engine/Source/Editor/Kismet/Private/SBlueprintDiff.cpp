// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"

#include "BlueprintEditor.h"
#include "BlueprintEditorCommands.h"
#include "BlueprintEditorModes.h"
#include "BlueprintUtilities.h"
#include "Editor/Kismet/Public/SBlueprintEditorToolbar.h"
#include "Editor/Kismet/Public/SKismetInspector.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/UnrealEd/Public/EdGraphUtilities.h"
#include "GraphDiffControl.h"
#include "GraphEditor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "SBlueprintDiff.h"
#include "SGraphTitleBar.h"
#include "SSCSEditor.h"
#include "WorkflowOrientedApp/SModeWidget.h"

#define LOCTEXT_NAMESPACE "SBlueprintDif"

struct FPropertyFilter : public TSharedFromThis<FPropertyFilter>
{
	FPropertyFilter( TSet< const UProperty*> InHiddenProperties )
		: HiddenProperties( InHiddenProperties )
	{
	}

	~FPropertyFilter()
	{
		UE_LOG(LogBlueprintUserMessages, Verbose, TEXT(" Nuking property filter.."));
	}

	bool IsVisible(const FPropertyAndParent& Property)
	{
		return !HiddenProperties.Contains(&Property.Property);
	}

	TSet< const UProperty*> HiddenProperties;
};

typedef TMap< FString, const UProperty* > FNamePropertyMap;

static FNamePropertyMap GetCDOProperties( const UBlueprint* ForBlueprint )
{
	FNamePropertyMap Ret;

	if( ForBlueprint )
	{
		if( const UClass* GeneratedClass = ForBlueprint->GeneratedClass )
		{
			if( const UObject* CDO = GeneratedClass->ClassDefaultObject )
			{
				if( const UClass* CDOClass = CDO->GetClass() )
				{
					check( CDOClass == GeneratedClass );
					for (TFieldIterator<UProperty> PropertyIt(CDOClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
					{
						Ret.Add(PropertyIt->GetName(), *PropertyIt);
					}
				}
			}
		}
	}

	return Ret;
}

const UObject* GetCDO( const UBlueprint* ForBlueprint )
{
	if( !ForBlueprint 
		|| !ForBlueprint->GeneratedClass )
	{
		return NULL;
	}

	return ForBlueprint->GeneratedClass->ClassDefaultObject;
}

TSharedRef<SWidget>	FDiffResultItem::GenerateWidget() const
{
	FString ToolTip = Result.ToolTip;
	FLinearColor Color = Result.DisplayColor;
	FString Text = Result.DisplayString;
	if (Text.Len() == 0)
	{
		Text = LOCTEXT("DIF_UnknownDiff", "Unknown Diff").ToString();
		ToolTip = LOCTEXT("DIF_Confused", "There is an unspecified difference").ToString();
	}
	return SNew(STextBlock)
		.ToolTipText(ToolTip)
		.ColorAndOpacity(Color)
		.Text(Text);
}

FListItemGraphToDiff::FListItemGraphToDiff( class SBlueprintDiff* InDiff, class UEdGraph* InGraphOld, class UEdGraph* InGraphNew, const FRevisionInfo& InRevisionOld, const FRevisionInfo& InRevisionNew )
	: Diff(InDiff), GraphOld(InGraphOld), GraphNew(InGraphNew), RevisionOld(InRevisionOld), RevisionNew(InRevisionNew)
{
	check(InGraphOld || InGraphNew); //one of them needs to exist

	//need to know when it is modified
	if(InGraphNew)
	{
		InGraphNew->AddOnGraphChangedHandler( FOnGraphChanged::FDelegate::CreateRaw(this, &FListItemGraphToDiff::OnGraphChanged));
	}
}

FListItemGraphToDiff::~FListItemGraphToDiff()
{
	if(GraphNew)
	{
		GraphNew->RemoveOnGraphChangedHandler( FOnGraphChanged::FDelegate::CreateRaw(this, &FListItemGraphToDiff::OnGraphChanged));
	}
}

TSharedRef<SWidget> FListItemGraphToDiff::GenerateWidget() 
{
	auto Graph = GraphOld ? GraphOld : GraphNew;
	
	FLinearColor Color = (GraphOld && GraphNew) ? FLinearColor::White : FLinearColor(0.3f,0.3f,1.f);

	BuildDiffSourceArray();

	const bool bHasDiffs = DiffListSource.Num() > 0;

	//give it a red color to indicate that the graph contains differences
	if(bHasDiffs)
	{
		Color =  FLinearColor(1.0f,0.2f,0.3f);
	}

	return SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.MaxWidth(8.0f)
		[
			SNew(SImage)
			.ColorAndOpacity(Color)
			.Image(GetIconForGraph(GraphOld))

		]
	+SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(STextBlock)
			.ColorAndOpacity(Color)
			.Text(Graph->GetName())

		]
	+SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.MaxWidth(8.0f)
		[
			SNew(SImage)
			.ColorAndOpacity(Color)
			.Image(GetIconForGraph(GraphNew))
		];
}

//////////////////////////////////////////////////////////////////////////
// FDiffListCommands

class FDiffListCommands : public TCommands<FDiffListCommands>
{
public:
	/** Constructor */
	FDiffListCommands() 
		: TCommands<FDiffListCommands>("DiffList", NSLOCTEXT("Diff", "Blueprint", "Blueprint Diff"), NAME_None, FEditorStyle::GetStyleSetName())
	{
	}

	/** Go to previous difference */
	TSharedPtr<FUICommandInfo> Previous;

	/** Go to next difference */
	TSharedPtr<FUICommandInfo> Next;

	/** Initialize commands */
	virtual void RegisterCommands() override;
};

void FDiffListCommands::RegisterCommands()
{
	UI_COMMAND(Previous, "Prev", "Go to previous difference", EUserInterfaceActionType::Button, FInputGesture(EKeys::F7, EModifierKey::Control));
	UI_COMMAND(Next, "Next", "Go to next difference", EUserInterfaceActionType::Button, FInputGesture(EKeys::F7));
}


TSharedRef<SWidget> FListItemGraphToDiff::GenerateDiffListWidget() 
{
	BuildDiffSourceArray();
	if(DiffListSource.Num() > 0)
	{
		struct SortDiff
		{
			bool operator () (const FSharedDiffOnGraph& A, const FSharedDiffOnGraph& B) const
			{
				return A->Result.Diff < B->Result.Diff;
			}
		};

		Sort(DiffListSource.GetTypedData(),DiffListSource.Num(), SortDiff());

		
		const FDiffListCommands& Commands = FDiffListCommands::Get();

		// Map commands through UI
		KeyCommands = MakeShareable(new FUICommandList );

		KeyCommands->MapAction( Commands.Previous, FExecuteAction::CreateSP(this, &FListItemGraphToDiff::PrevDiff));
		KeyCommands->MapAction( Commands.Next, FExecuteAction::CreateSP(this, &FListItemGraphToDiff::NextDiff));

		FToolBarBuilder ToolbarBuilder( KeyCommands.ToSharedRef(), FMultiBoxCustomization::None );
		ToolbarBuilder.AddToolBarButton(Commands.Previous, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "BlueprintDif.PrevDiff"));
		ToolbarBuilder.AddToolBarButton(Commands.Next, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "BlueprintDif.NextDiff"));

		TSharedPtr<SListViewType> DiffListRef;
		TSharedRef<SHorizontalBox> Result =	SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.f)
			.MaxWidth(350.f)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.Padding(0.f)
				.AutoHeight()
				[
					ToolbarBuilder.MakeWidget()
				]
				+SVerticalBox::Slot()
				.Padding(0.f)
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("PropertyWindow.CategoryBackground"))
					.Padding(FMargin(2.0f))
					.ForegroundColor(FEditorStyle::GetColor("PropertyWindow.CategoryForeground"))
					.ToolTipText(LOCTEXT("BlueprintDifDifferencesToolTip", "List of differences found between revisions, click to select"))
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("RevisionDifferences", "Revision Differences"))
					]
				]
				+SVerticalBox::Slot()
				.Padding(1.f)
				.FillHeight(1.f)
				[
					SAssignNew(DiffList, SListViewType)
					.ItemHeight(24)
					.ListItemsSource( &DiffListSource )
					.OnGenerateRow( this, &FListItemGraphToDiff::OnGenerateRow )
					.SelectionMode( ESelectionMode::Single )
					.OnSelectionChanged(this, &FListItemGraphToDiff::OnSelectionChanged)
				]
			];
		return Result;
	}
	else
	{
		return SNew(SBorder).Visibility(EVisibility::Hidden);
	}
		
	
}

TSharedRef<ITableRow> FListItemGraphToDiff::OnGenerateRow( FSharedDiffOnGraph ParamItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	return	SNew( STableRow< FSharedDiffOnGraph >, OwnerTable )
		.Content()
		[
			ParamItem->GenerateWidget()
		];
}

void FListItemGraphToDiff::OnSelectionChanged( FSharedDiffOnGraph Item, ESelectInfo::Type SelectionType )
{
	if(Item.IsValid())
	{
		Diff->OnDiffListSelectionChanged(Item, this);
	}
}

const FSlateBrush* FListItemGraphToDiff::GetIconForGraph( UEdGraph* Graph ) 
{
	return Graph? FEditorStyle::GetBrush("BlueprintDif.HasGraph") : FEditorStyle::GetBrush("BlueprintDif.MissingGraph");
}

FText FListItemGraphToDiff::GetToolTip() 
{
	if ( GraphOld && GraphNew )
	{
		BuildDiffSourceArray();
		if ( DiffListSource.Num() > 0 )
		{
			return LOCTEXT("ContainsDifferences", "Revisions are different");
		}
		else
		{
			return LOCTEXT("GraphsIdentical", "Revisions appear to be identical");
		}
	}
	else
	{
		auto GoodGraph = GraphOld ? GraphOld : GraphNew;
		const FRevisionInfo& Revision = GraphNew ? RevisionOld : RevisionNew;
		FText RevisionText = LOCTEXT("CurrentRevision", "Current Revision");

		if ( Revision.Revision >= 0 )
		{
			RevisionText = FText::Format( LOCTEXT("Revision Number", "Revision {0}") , FText::AsNumber( Revision.Revision, NULL, FInternationalization::Get().GetInvariantCulture() ) );
		}

		return FText::Format( LOCTEXT("MissingGraph", "Graph '{0}' missing from {1}"), FText::FromString( GoodGraph->GetName() ), RevisionText );
	}
}

void FListItemGraphToDiff::BuildDiffSourceArray()
{
	TArray<FDiffSingleResult> FoundDiffs;
	FGraphDiffControl::DiffGraphs(GraphOld, GraphNew, FoundDiffs);

	DiffListSource.Empty();
	for (auto DiffIt(FoundDiffs.CreateIterator()); DiffIt; ++DiffIt)
	{
		DiffListSource.Add(FSharedDiffOnGraph(new FDiffResultItem(*DiffIt)));
	}
}

void FListItemGraphToDiff::NextDiff()
{
	int32 Index = (GetCurrentDiffIndex() + 1) % DiffListSource.Num();

	DiffList->SetSelection(DiffListSource[Index]);
}

void FListItemGraphToDiff::PrevDiff()
{
	int32 Index = GetCurrentDiffIndex();
	if(Index == 0)
	{
		Index = DiffListSource.Num() - 1;
	}
	else
	{
		Index = (Index - 1) % DiffListSource.Num();
	}

	DiffList->SetSelection(DiffListSource[Index]);
}

int32 FListItemGraphToDiff::GetCurrentDiffIndex() 
{
	auto Selected = DiffList->GetSelectedItems();
	if(Selected.Num() == 1)
	{	
		int32 Index = 0;
		for(auto It(DiffListSource.CreateIterator());It;++It,Index++)
		{
			if(*It == Selected[0])
			{
				return Index;
			}
		}
	}
	return 0;
}

void FListItemGraphToDiff::OnGraphChanged( const FEdGraphEditAction& Action )
{
	Diff->OnGraphChanged(this);
}

void FListItemGraphToDiff::KeyWasPressed( const FKeyboardEvent& InKeyboardEvent )
{
	if ( KeyCommands.IsValid() )
	{
		KeyCommands->ProcessCommandBindings(InKeyboardEvent);
	}
}


FDiffPanel::FDiffPanel()
{
	Blueprint = NULL;
	LastFocusedPin = NULL;
}

// Generates a widget showing CDO values. NewBlueprint's CDO values will be displayed by the widget (Base just used for determining what to hide):
static TSharedRef<SWidget> GenerateCDODiff( UBlueprint const* BaseBlueprint, UBlueprint const* NewBlueprint )
{
	const auto GenerateEmptyPanel = []( const FText& Message) { return SNew(STextBlock).Text(Message); };

	// filter out properties that have not changed from the GraphToDiff's blueprint:	
	FNamePropertyMap BaseProperties = GetCDOProperties(BaseBlueprint);
	FNamePropertyMap BlueprintProperties = GetCDOProperties(NewBlueprint);

	if( BlueprintProperties.Num() == 0 )
	{
		if( NewBlueprint )
		{
			return GenerateEmptyPanel( FText::Format( LOCTEXT("BPNoCDO", "Blueprint {0} has no Defaults"), FText::FromString( NewBlueprint->GetName() ) ) );
		}
		else
		{
			return GenerateEmptyPanel( LOCTEXT("BPMissingBP", "Missing Blueprint" ) );
		}
	}

	const UObject* BaseBlueprintCDO = GetCDO(BaseBlueprint);
	const UObject* NewBlueprintCDO = GetCDO(NewBlueprint);

	TSet< const UProperty*> HiddenProperties;
	for (const auto& Property : BlueprintProperties)
	{
		const FString& PropertyName = Property.Key;
		const UProperty* PropertyValue = Property.Value;

		const UProperty** BaseProperty = BaseProperties.Find(PropertyName);
		if (BaseProperty && (*BaseProperty)->SameType(PropertyValue))
		{
			const void* MyValue = PropertyValue->ContainerPtrToValuePtr<void>(NewBlueprintCDO);
			const void* BaseValue = (*BaseProperty)->ContainerPtrToValuePtr<void>(BaseBlueprintCDO);

			if ((*BaseProperty)->Identical(MyValue, BaseValue, 0))
			{
				HiddenProperties.Add(PropertyValue);
			}
		}
	}

	if( HiddenProperties.Num() == BlueprintProperties.Num() )
	{
		return GenerateEmptyPanel(LOCTEXT("BPNoCDODifference", "No differences found in Defaults"));
	}

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bShowOptions = false;
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	auto DetailsView = EditModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateStatic([]{return false; }));
	// This filter goes out of scope and is destroyed when this function returns (the control just keeps a weak ptr)
	// but that seems to be long enough to hide the properties that we want to hide:
	auto Filter = TSharedPtr<FPropertyFilter>(new FPropertyFilter(HiddenProperties));
	DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(Filter.ToSharedRef(), &FPropertyFilter::IsVisible));
	// This is a read only details view (see the property editing delegate), but it is not const correct:
	DetailsView->SetObject(const_cast<UObject*>(NewBlueprintCDO) );

	return DetailsView;
}

static TSharedRef<SWidget> GenerateComponentsDiff( UBlueprint const* BaseBlueprint, UBlueprint const* DisplayedBlueprint )
{
	TSharedRef<SKismetInspector> Inspector = SNew(SKismetInspector)
		.HideNameArea(true)
		.ViewIdentifier(FName("BlueprintInspector"))
		.IsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateStatic([] { return false; }));

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSCSEditor, TSharedPtr<FBlueprintEditor>(), DisplayedBlueprint->SimpleConstructionScript, const_cast<UBlueprint*>(DisplayedBlueprint), Inspector )
		]
		+ SVerticalBox::Slot()
		[
			Inspector
		];
}

//////////////////////////////////////////////////////////////////////////
// SBlueprintDiff

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SBlueprintDiff::Construct( const FArguments& InArgs)
{
	FDiffListCommands::Register();
	check(InArgs._BlueprintOld && InArgs._BlueprintNew);
	PanelOld.Blueprint = InArgs._BlueprintOld;
	PanelNew.Blueprint = InArgs._BlueprintNew;

	PanelOld.RevisionInfo = InArgs._OldRevision;
	PanelNew.RevisionInfo = InArgs._NewRevision;

	// sometimes we want to clearly identify the assets being diffed (when it's
	// not the same asset in each panel)
	PanelOld.bShowAssetName = InArgs._ShowAssetNames;
	PanelNew.bShowAssetName = InArgs._ShowAssetNames;

	bLockViews = true;

	TArray<UEdGraph*> GraphsOld,GraphsNew;
	PanelOld.Blueprint->GetAllGraphs(GraphsOld);
	PanelNew.Blueprint->GetAllGraphs(GraphsNew);
	
	//Add Graphs that exist in both blueprints, or in blueprint 1 only
	for(auto It(GraphsOld.CreateConstIterator());It;It++)
	{
		UEdGraph* GraphOld = *It;
		UEdGraph* GraphNew = NULL;
		for(auto It2(GraphsNew.CreateIterator());It2;It2++)
		{
			UEdGraph* TestGraph = *It2;
			if(TestGraph && GraphOld->GetName() == TestGraph->GetName())
			{
				GraphNew = TestGraph;
				*It2 = NULL;
				break;
			}
		}
		CreateGraphEntry(GraphOld,GraphNew);
	}

	//Add graphs that only exist in 2nd(new) blueprint
	for(auto It2(GraphsNew.CreateIterator());It2;It2++)
	{
		UEdGraph* GraphNew = *It2;
		if(GraphNew != NULL)
		{
			CreateGraphEntry(NULL,GraphNew);
		}
	}

	TAttribute<FName> GetActiveMode(this, &SBlueprintDiff::GetCurrentMode);
	FOnModeChangeRequested SetActiveMode = FOnModeChangeRequested::CreateRaw(this, &SBlueprintDiff::SetCurrentMode);

	auto Widgets = FBlueprintEditorToolbar::GenerateToolbarWidgets( InArgs._BlueprintNew, GetActiveMode, SetActiveMode );

	TSharedRef<SHorizontalBox> MiscWidgets = SNew(SHorizontalBox);

	for (int32 WidgetIdx = 0; WidgetIdx < Widgets.Num(); ++WidgetIdx)
	{
		MiscWidgets->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				Widgets[WidgetIdx].ToSharedRef()
			];
	}

	this->ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush( "Docking.Tab", ".ContentAreaBrush" ))
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 2.0f, 0.0f, 2.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SNew(SSpacer)
					]
					+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							// Nested box just give us padding similar to the main blueprint editor,
							// one border has the dark grey "Toolbar.Background" style:
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.Padding(0.0f, 4.f, 0.0f, 12.f)
							[
								SNew(SBorder)
								.BorderImage(FEditorStyle::GetBrush(TEXT("Toolbar.Background")))
								[
									MiscWidgets
								]
							]
						]
				]
				+ SVerticalBox::Slot()
				[
					SAssignNew(ModeContents, SBorder)
				]
			]
			
		];

	SetCurrentMode( FBlueprintEditorApplicationModes::StandardBlueprintEditorMode );
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<ITableRow> SBlueprintDiff::OnGenerateRow( FGraphToDiff ParamItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	return	SNew( STableRow< FGraphToDiff >, OwnerTable )
			.ToolTipText(ParamItem->GetToolTip())
			.Content()
			[
				ParamItem->GenerateWidget()
			];
}

void SBlueprintDiff::CreateGraphEntry( class UEdGraph* GraphOld, class UEdGraph* GraphNew )
{
	Graphs.Add(FGraphToDiff(new FListItemGraphToDiff(this, GraphOld, GraphNew, PanelOld.RevisionInfo, PanelNew.RevisionInfo)));
}

void SBlueprintDiff::OnSelectionChanged( FGraphToDiff Item, ESelectInfo::Type SelectionType) 
{
	if(SelectionType != ESelectInfo::OnMouseClick || !Item.IsValid())
	{
		return;
	}

	FocusOnGraphRevisions(Item->GetGraphOld(), Item->GetGraphNew(), Item.Get());

}

void SBlueprintDiff::OnGraphChanged(FListItemGraphToDiff* Diff)
{
	if(PanelNew.GraphEditor.IsValid() && PanelNew.GraphEditor.Pin()->GetCurrentGraph() == Diff->GetGraphNew())
	{
		FocusOnGraphRevisions(Diff->GetGraphOld(), Diff->GetGraphNew(), Diff);
	}
}

TSharedRef<SWidget> SBlueprintDiff::DefaultEmptyPanel()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BlueprintDifGraphsToolTip", "Select Graph to Diff"))
		];
}

void SBlueprintDiff::FocusOnGraphRevisions( class UEdGraph* GraphOld, class UEdGraph* GraphNew, FListItemGraphToDiff* Diff )
{
	HandleGraphChanged( GraphOld ? GraphOld->GetName() : GraphNew->GetName() );

	ResetGraphEditors();

	//set new diff list
	DiffListBorder->SetContent(Diff->GenerateDiffListWidget());
}

void SBlueprintDiff::OnDiffListSelectionChanged(const TSharedPtr<struct FDiffResultItem>& TheDiff, FListItemGraphToDiff* GraphDiffer )
{
	FDiffSingleResult Result = TheDiff->Result;

	const auto SafeClearSelection = []( TWeakPtr<SGraphEditor> GraphEditor )
	{
		auto GraphEditorPtr = GraphEditor.Pin();
		if( GraphEditorPtr.IsValid())
		{
			GraphEditorPtr->ClearSelectionSet();
		}
	};

	SafeClearSelection( PanelNew.GraphEditor );
	SafeClearSelection( PanelOld.GraphEditor );

	if (Result.Pin1)
	{
		GetDiffPanelForNode(*Result.Pin1->GetOwningNode()).FocusDiff(*Result.Pin1);
		if (Result.Pin2)
		{
			GetDiffPanelForNode(*Result.Pin2->GetOwningNode()).FocusDiff(*Result.Pin2);
		}
	}
	else if (Result.Node1)
	{
		GetDiffPanelForNode(*Result.Node1).FocusDiff(*Result.Node1);
		if (Result.Node2)
		{
			GetDiffPanelForNode(*Result.Node2).FocusDiff(*Result.Node2);
		}
	}
}

FReply	SBlueprintDiff::OnToggleLockView()
{
	bLockViews = !bLockViews;
	ResetGraphEditors();
	return FReply::Handled();
}

const FSlateBrush* SBlueprintDiff::GetLockViewImage() const
{
	return bLockViews ? FEditorStyle::GetBrush("GenericLock") : FEditorStyle::GetBrush("GenericUnlock");
}

void SBlueprintDiff::ResetGraphEditors()
{
	if(PanelOld.GraphEditor.IsValid() && PanelNew.GraphEditor.IsValid())
	{
		if(bLockViews)
		{
			PanelOld.GraphEditor.Pin()->LockToGraphEditor(PanelNew.GraphEditor);
			PanelNew.GraphEditor.Pin()->LockToGraphEditor(PanelOld.GraphEditor);
		}
		else
		{
			PanelOld.GraphEditor.Pin()->UnlockFromGraphEditor(PanelNew.GraphEditor);
			PanelNew.GraphEditor.Pin()->UnlockFromGraphEditor(PanelOld.GraphEditor);
		}	
	}
}

void FDiffPanel::GeneratePanel(UEdGraph* Graph, UEdGraph* GraphToDiff)
{
	if( LastFocusedPin )
	{
		LastFocusedPin->bIsDiffing = false;
	}
	LastFocusedPin = NULL;

	TSharedPtr<SWidget> Widget = SNew(SBorder)
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock).Text( LOCTEXT("BPDifPanelNoGraphTip", "Graph does not exist in this revision"))
								];
	if(Graph)
	{
		SGraphEditor::FGraphEditorEvents InEvents;

		FGraphAppearanceInfo AppearanceInfo;
		AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_BlueprintDif", "DIFF").ToString();

		if ( !GraphEditorCommands.IsValid() )
		{
			GraphEditorCommands = MakeShareable( new FUICommandList() );

			GraphEditorCommands->MapAction( FGenericCommands::Get().Copy,
				FExecuteAction::CreateRaw( this, &FDiffPanel::CopySelectedNodes ),
				FCanExecuteAction::CreateRaw( this, &FDiffPanel::CanCopyNodes )
				);
		}

		auto Editor = SNew(SGraphEditor)
			.AdditionalCommands(GraphEditorCommands)
			.GraphToEdit(Graph)
			.GraphToDiff(GraphToDiff)
			.IsEditable(false)
			.TitleBarEnabledOnly(false)
			.TitleBar(SNew(SBorder).HAlign(HAlign_Center)
						[
							SNew(STextBlock).Text(GetTitle())
						])
			.Appearance(AppearanceInfo)
			.GraphEvents(InEvents);

		GraphEditor = Editor;
		Widget = Editor;
	}

	GraphEditorBorder->SetContent(Widget.ToSharedRef());
}

FGraphPanelSelectionSet FDiffPanel::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;
	TSharedPtr<SGraphEditor> FocusedGraphEd = GraphEditor.Pin();
	if (FocusedGraphEd.IsValid())
	{
		CurrentSelection = FocusedGraphEd->GetSelectedNodes();
	}
	return CurrentSelection;
}

void FDiffPanel::CopySelectedNodes()
{
	// Export the selected nodes and place the text on the clipboard
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	FString ExportedText;
	FEdGraphUtilities::ExportNodesToText(SelectedNodes, /*out*/ ExportedText);
	FPlatformMisc::ClipboardCopy(*ExportedText);
}


FString FDiffPanel::GetTitle() const
{
	FString Title = LOCTEXT("CurrentRevision", "Current Revision").ToString();

	// if this isn't the current working version being displayed
	if (RevisionInfo.Revision >= 0)
	{
		FString DateString = RevisionInfo.Date.ToString(TEXT("%m/%d/%Y"));
		if (bShowAssetName)
		{
			FString AssetName = Blueprint->GetName();

			FText LocalizedFormat = LOCTEXT("NamedRevisionDiff", "%s - Revision %i, CL %i, %s");
			Title = FString::Printf(*LocalizedFormat.ToString(), *AssetName, RevisionInfo.Revision, RevisionInfo.Changelist, *DateString);
		}
		else
		{
			FText LocalizedFormat = LOCTEXT("PreviousRevisionDif", "Revision %i, CL %i, %s");
			Title = FString::Printf(*LocalizedFormat.ToString(), RevisionInfo.Revision, RevisionInfo.Changelist, *DateString);
		}
	}
	else if (bShowAssetName)
	{
		FString AssetName = Blueprint->GetName();

		FText LocalizedFormat = LOCTEXT("NamedCurrentRevision", "%s - Current Revision");
		Title = FString::Printf(*LocalizedFormat.ToString(), *AssetName);
	}

	return Title;
}

bool FDiffPanel::CanCopyNodes() const
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

void FDiffPanel::FocusDiff(UEdGraphPin& Pin)
{
	if( LastFocusedPin )
	{
		LastFocusedPin->bIsDiffing = false;
	}
	Pin.bIsDiffing = true;
	LastFocusedPin = &Pin;

	GraphEditor.Pin()->JumpToPin(&Pin);
}

void FDiffPanel::FocusDiff(UEdGraphNode& Node)
{
	if (LastFocusedPin)
	{
		LastFocusedPin->bIsDiffing = false;
	}
	LastFocusedPin = NULL;

	GraphEditor.Pin()->JumpToNode(&Node, false);
}

FDiffPanel& SBlueprintDiff::GetDiffPanelForNode(UEdGraphNode& Node)
{
	auto OldGraphEditorPtr = PanelOld.GraphEditor.Pin();
	if (OldGraphEditorPtr.IsValid() && Node.GetGraph() == OldGraphEditorPtr->GetCurrentGraph())
	{
		return PanelOld;
	}
	auto NewGraphEditorPtr = PanelNew.GraphEditor.Pin();
	if (NewGraphEditorPtr.IsValid() && Node.GetGraph() == NewGraphEditorPtr->GetCurrentGraph())
	{
		return PanelNew;
	}
	checkf(false, TEXT("Looking for node %s but it cannot be found in provided panels"), *Node.GetName());
	static FDiffPanel Default;
	return Default;
}

void SBlueprintDiff::HandleGraphChanged( const FString& GraphName )
{
	TArray<UEdGraph*> GraphsOld, GraphsNew;
	PanelOld.Blueprint->GetAllGraphs(GraphsOld);
	PanelNew.Blueprint->GetAllGraphs(GraphsNew);

	UEdGraph* GraphOld = NULL;
	if( UEdGraph** Iter = GraphsOld.FindByPredicate(FMatchName(GraphName)) )
	{
		GraphOld = *Iter;
	}
	UEdGraph* GraphNew = NULL;
	if (UEdGraph** Iter = GraphsNew.FindByPredicate(FMatchName(GraphName)))
	{
		GraphNew = *Iter;
	}

	PanelOld.GeneratePanel(GraphOld, GraphNew);
	PanelNew.GeneratePanel(GraphNew, GraphOld);
}

TSharedRef<SWidget> SBlueprintDiff::GenerateGraphPanel()
{
	return SNew(SBorder)
	.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
	.Content()
	[
		SNew(SBorder)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SSplitter)
				+ SSplitter::Slot()
				.Value(0.2f)
				[
					SNew(SBorder)
					[
						SNew(SSplitter)
						.Orientation(Orient_Vertical)
						+ SSplitter::Slot()
						.Value(0.3f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Left)
							[
								//toggle lock button
								SNew(SButton)
								.OnClicked(this, &SBlueprintDiff::OnToggleLockView)
								.Content()
								[
									SNew(SImage)
									.Image(this, &SBlueprintDiff::GetLockViewImage)
									.ToolTipText(LOCTEXT("BPDifLock", "Lock the blueprint views?"))
								]
							]
							+ SVerticalBox::Slot()
								.FillHeight(1.f)
								[
									//graph selection 
									SNew(SBorder)
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot()
										.AutoHeight()
										[
											SNew(SBorder)
											.BorderImage(FEditorStyle::GetBrush("PropertyWindow.CategoryBackground"))
											.Padding(FMargin(2.0f))
											.ForegroundColor(FEditorStyle::GetColor("PropertyWindow.CategoryForeground"))
											.ToolTipText(LOCTEXT("BlueprintDifGraphsToolTip", "Select Graph to Diff"))
											.HAlign(HAlign_Center)
											[
												SNew(STextBlock)
												.Text(LOCTEXT("BlueprintDifGraphs", "Graphs"))
											]
										]
										+ SVerticalBox::Slot()
											.FillHeight(1.f)
											[
												SAssignNew(GraphsToDiff, SListViewType)
												.ItemHeight(24)
												.ListItemsSource(&Graphs)
												.OnGenerateRow(this, &SBlueprintDiff::OnGenerateRow)
												.SelectionMode(ESelectionMode::Single)
												.OnSelectionChanged(this, &SBlueprintDiff::OnSelectionChanged)
											]
									]
								]
						]
						+ SSplitter::Slot()
							.Value(0.7f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.FillHeight(1.f)
								[
									SAssignNew(DiffListBorder, SBorder)
								]
							]
					]
				]
				+ SSplitter::Slot()
					.Value(0.8f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						[
							//diff window
							SNew(SSplitter)
							+ SSplitter::Slot()
							.Value(0.5f)
							[
								//left blueprint
								SAssignNew(PanelOld.GraphEditorBorder, SBorder)
								.VAlign(VAlign_Fill)
								[
									DefaultEmptyPanel()
								]
							]
							+ SSplitter::Slot()
							.Value(0.5f)
							[
								//right blueprint
								SAssignNew(PanelNew.GraphEditorBorder, SBorder)
								.VAlign(VAlign_Fill)
								[
									DefaultEmptyPanel()
								]
							]
						]
					]
			]
		]
	];
}

TSharedRef<SWidget> SBlueprintDiff::GenerateDefaultsPanel()
{
	//Splitter for left and right blueprint. Current convention is for the local (probably newer?) blueprint to be on the right:
	return SNew(SSplitter)
		+ SSplitter::Slot()
		.Value(0.5f)
		[
			SNew(SBorder)
			.VAlign(VAlign_Fill)
			[
				GenerateCDODiff(PanelNew.Blueprint, PanelOld.Blueprint)
			]
		]
	+ SSplitter::Slot()
		.Value(0.5f)
		[
			SNew(SBorder)
			.VAlign(VAlign_Fill)
			[
				GenerateCDODiff(PanelOld.Blueprint, PanelNew.Blueprint)
			]
		];
}

TSharedRef<SWidget> SBlueprintDiff::GenerateComponentsPanel()
{
	//Splitter for left and right blueprint. Current convention is for the local (probably newer?) blueprint to be on the right:
	return SNew(SSplitter)
		+ SSplitter::Slot()
		.Value(0.5f)
		[
			SNew(SBorder)
			.VAlign(VAlign_Fill)
			[
				GenerateComponentsDiff(PanelNew.Blueprint, PanelOld.Blueprint)
			]
		]
	+ SSplitter::Slot()
		.Value(0.5f)
		[
			SNew(SBorder)
			.VAlign(VAlign_Fill)
			[
				GenerateComponentsDiff(PanelOld.Blueprint, PanelNew.Blueprint)
			]
		];
}

void SBlueprintDiff::SetCurrentMode(FName NewMode)
{
	CurrentMode = NewMode;

	if( NewMode == FBlueprintEditorApplicationModes::StandardBlueprintEditorMode)
	{
		ModeContents->SetContent( GenerateGraphPanel() );
	}
	else if( NewMode == FBlueprintEditorApplicationModes::BlueprintDefaultsMode )
	{
		ModeContents->SetContent( GenerateDefaultsPanel() );
	}
	else if (NewMode == FBlueprintEditorApplicationModes::BlueprintComponentsMode)
	{
		ModeContents->SetContent( GenerateComponentsPanel() );
	}
	else
	{
		ensureMsgf(false, TEXT("Diff panel does not support mode %s"), *NewMode.ToString() );
	}
}

FReply SBlueprintDiff::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) 
{
	TArray< FGraphToDiff>  Selected = GraphsToDiff->GetSelectedItems();
	for(auto It(Selected.CreateIterator());It;++It)
	{
		FGraphToDiff& Diff = *It;
		Diff->KeyWasPressed(InKeyboardEvent);
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

