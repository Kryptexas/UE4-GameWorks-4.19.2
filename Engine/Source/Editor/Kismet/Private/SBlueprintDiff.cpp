// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"

#include "BlueprintEditor.h"
#include "BlueprintEditorCommands.h"
#include "BlueprintEditorModes.h"
#include "BlueprintUtilities.h"
#include "DetailsDiff.h"
#include "Editor/Kismet/Public/SBlueprintEditorToolbar.h"
#include "Editor/Kismet/Public/SKismetInspector.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/UnrealEd/Public/EdGraphUtilities.h"
#include "GraphDiffControl.h"
#include "GraphEditor.h"
#include "IDiffControl.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "SBlueprintDiff.h"
#include "SDockTab.h"
#include "SGraphTitleBar.h"
#include "SMyBlueprint.h"
#include "SSCSEditor.h"
#include "SCSDiff.h"
#include "WorkflowOrientedApp/SModeWidget.h"
#include "GenericCommands.h"

#define LOCTEXT_NAMESPACE "SBlueprintDif"

typedef TMap< FName, const UProperty* > FNamePropertyMap;

const FName DiffMyBluerpintTabId = FName(TEXT("DiffMyBluerpintTab"));
const FName DiffGraphTabId = FName(TEXT("DiffGraphTab"));

// Each difference in the tree will either be a tree node that is added in one Blueprint 
// or a tree node and an FName of a property that has been added or edited in one Blueprint
class FSCSDiffControl	: public TSharedFromThis< FSCSDiffControl >
						, public IDiffControl
{
public:
	FSCSDiffControl(
					const UBlueprint* InOldBlueprint
					, const UBlueprint* InNewBlueprint
					);


	TSharedRef<SWidget> OldTreeWidget() { return OldSCS.TreeWidget(); }
	TSharedRef<SWidget> NewTreeWidget() { return NewSCS.TreeWidget(); }

	virtual ~FSCSDiffControl() {}
private:
	void NextDiff() override;
	void PrevDiff() override;
	bool HasNextDifference() const override;
	bool HasPrevDifference() const override;

	void HighlightCurrentDifference();

	FSCSDiffRoot DifferingProperties;
	int CurrentDifference;

	FSCSDiff OldSCS;
	FSCSDiff NewSCS;
};

FSCSDiffControl::FSCSDiffControl(
		const UBlueprint* InOldBlueprint
		, const UBlueprint* InNewBlueprint
	)
	: DifferingProperties()
	, CurrentDifference(-1)
	, OldSCS( InOldBlueprint )
	, NewSCS( InNewBlueprint )
{
	TArray< FSCSResolvedIdentifier > OldHierarchy = OldSCS.GetDisplayedHierarchy();
	TArray< FSCSResolvedIdentifier > NewHierarchy = NewSCS.GetDisplayedHierarchy();
	DiffUtils::CompareUnrelatedSCS(InOldBlueprint, OldHierarchy, InNewBlueprint, NewHierarchy, DifferingProperties);
}

void FSCSDiffControl::NextDiff()
{
	++CurrentDifference;
	HighlightCurrentDifference();
}

void FSCSDiffControl::PrevDiff()
{
	--CurrentDifference;
	HighlightCurrentDifference();
}

bool FSCSDiffControl::HasNextDifference() const
{
	return DifferingProperties.Entries.IsValidIndex(CurrentDifference + 1);
}

bool FSCSDiffControl::HasPrevDifference() const
{
	return DifferingProperties.Entries.IsValidIndex(CurrentDifference - 1);
}

void FSCSDiffControl::HighlightCurrentDifference()
{
	OldSCS.HighlightProperty(DifferingProperties.Entries[CurrentDifference].TreeIdentifier.Name, FPropertyPath());
	NewSCS.HighlightProperty(DifferingProperties.Entries[CurrentDifference].TreeIdentifier.Name, FPropertyPath());
}

class FCDODiffControl	: public TSharedFromThis<FCDODiffControl>
						, public IDiffControl
{
public:
	FCDODiffControl( const UObject* InOldCDO
					, const UObject* InNewCDO
					, const TArray<FPropertySoftPath>& InDifferingProperties );

	TSharedRef<SWidget> OldDetailsWidget() { return OldDetails.DetailsWidget(); }
	TSharedRef<SWidget> NewDetailsWidget() { return NewDetails.DetailsWidget(); }

	virtual ~FCDODiffControl() {}
private:
	void NextDiff() override;
	void PrevDiff() override;
	bool HasNextDifference() const override;
	bool HasPrevDifference() const override;

	void HighlightCurrentDifference();

	void UpdateDisplayedDifferences();

	FDetailsDiff OldDetails;
	FDetailsDiff NewDetails;

	TArray<FPropertySoftPath> DisplayedDifferingProperties;
	FPropertySoftPathSet DifferingProperties;
	int CurrentDifference;
};

FCDODiffControl::FCDODiffControl( 
		const UObject* InOldCDO
		, const UObject* InNewCDO
		, const TArray<FPropertySoftPath>& InDifferingProperties)
	: OldDetails(InOldCDO, DiffUtils::ResolveAll( InOldCDO, InDifferingProperties), FDetailsDiff::FOnDisplayedPropertiesChanged::CreateRaw(this, &FCDODiffControl::UpdateDisplayedDifferences) )
	, NewDetails(InNewCDO, DiffUtils::ResolveAll( InNewCDO, InDifferingProperties), FDetailsDiff::FOnDisplayedPropertiesChanged::CreateRaw(this, &FCDODiffControl::UpdateDisplayedDifferences) )
	, DisplayedDifferingProperties()
	, DifferingProperties(InDifferingProperties)
	, CurrentDifference( -1 )
{
	UpdateDisplayedDifferences();
}

void FCDODiffControl::NextDiff()
{
	++CurrentDifference;
	HighlightCurrentDifference();
}

void FCDODiffControl::PrevDiff()
{
	--CurrentDifference;
	HighlightCurrentDifference();
}

bool FCDODiffControl::HasNextDifference() const
{
	return DisplayedDifferingProperties.IsValidIndex(CurrentDifference + 1);
}

bool FCDODiffControl::HasPrevDifference() const
{
	return DisplayedDifferingProperties.IsValidIndex(CurrentDifference - 1);
}

void FCDODiffControl::HighlightCurrentDifference()
{
	const FPropertySoftPath& PropertyName = DisplayedDifferingProperties[CurrentDifference];

	OldDetails.HighlightProperty(PropertyName);
	NewDetails.HighlightProperty(PropertyName);
}

void FCDODiffControl::UpdateDisplayedDifferences()
{
	DisplayedDifferingProperties.Empty();
	CurrentDifference = 0;

	// create differing properties list based on what is displayed by the old properties..
	TArray<FPropertySoftPath> OldProperties = OldDetails.GetDisplayedProperties();
	TArray<FPropertySoftPath> NewProperties = NewDetails.GetDisplayedProperties();

	// zip the two sets of properties, zip iterators are not trivial to write in c++,
	// so this procedural stuff will have to do:
	int IterOld = 0;
	int IterNew = 0;
	while (IterOld != OldProperties.Num() || IterNew != NewProperties.Num())
	{
		if (IterOld != OldProperties.Num() && IterNew != NewProperties.Num()
			&& OldProperties[IterOld] == NewProperties[IterNew])
		{
			if( DifferingProperties.Contains(OldProperties[IterOld]) )
			{
				DisplayedDifferingProperties.Push(OldProperties[IterOld]);
			}
			++IterOld;
			++IterNew;
		}
		else if (IterOld != OldProperties.Num())
		{
			if (DifferingProperties.Contains(OldProperties[IterOld]))
			{
				DisplayedDifferingProperties.Push(OldProperties[IterOld]);
			}
			++IterOld;
		}
		else
		{
			check(IterNew != NewProperties.Num());
			if (DifferingProperties.Contains(NewProperties[IterNew]))
			{
				DisplayedDifferingProperties.Push(NewProperties[IterNew]);
			}
			++IterNew;
		}
	}
}

/*List item that entry for a graph*/
struct KISMET_API FListItemGraphToDiff	: public TSharedFromThis<FListItemGraphToDiff>
										, public IDiffControl
{
	FListItemGraphToDiff(class SBlueprintDiff* Diff, class UEdGraph* GraphOld, class UEdGraph* GraphNew, const FRevisionInfo& RevisionOld, const FRevisionInfo& RevisionNew);
	virtual ~FListItemGraphToDiff();

	/*Generate Widget for list item*/
	TSharedRef<SWidget> GenerateWidget();

	/*Get tooltip for list item */
	FText   GetToolTip();

	/*Get old(left) graph*/
	UEdGraph* GetGraphOld()const{ return GraphOld; }

	/*Get new(right) graph*/
	UEdGraph* GetGraphNew()const{ return GraphNew; }

	/** Source for list view */
	TArray<TSharedPtr<struct FDiffResultItem>> DiffListSource;
private:

	/*Get icon to use by graph name*/
	static const FSlateBrush* GetIconForGraph(UEdGraph* Graph);

	/*Diff widget*/
	class SBlueprintDiff* Diff;

	/*The old graph(left)*/
	class UEdGraph*	GraphOld;

	/*The new graph(right)*/
	class UEdGraph* GraphNew;

	/*Description of Old and new graph*/
	FRevisionInfo	RevisionOld, RevisionNew;

	//////////////////////////////////////////////////////////////////////////
	// Diff list
	//////////////////////////////////////////////////////////////////////////

	typedef TSharedPtr<struct FDiffResultItem>	FSharedDiffOnGraph;
	typedef SListView<FSharedDiffOnGraph >		SListViewType;

public:

	/** Called when the Newer Graph is modified*/
	void OnGraphChanged(const FEdGraphEditAction& Action);

	/** Generate list of differences*/
	TSharedRef<SWidget> GenerateDiffListWidget();

	/** Build up the Diff Source Array*/
	void BuildDiffSourceArray();

	/** Called when user clicks on a new graph list item */
	void OnSelectionChanged(FSharedDiffOnGraph Item, ESelectInfo::Type SelectionType);

private:

	/** Called when user clicks button to go to next difference in graph */
	void NextDiff() override;

	/** Called when user clicks button to go to prev difference in graph */
	void PrevDiff() override;

	/** Called when determining whether the prev/nextbuttons should be enabled */
	bool HasNextDifference() const override;
	bool HasPrevDifference() const override;

	/** Get Index of the current diff that is selected */
	int32 GetCurrentDiffIndex() const;

	/* Called when a new row is being generated */
	TSharedRef<ITableRow> OnGenerateRow(FSharedDiffOnGraph ParamItem, const TSharedRef<STableViewBase>& OwnerTable);

	/** ListView of differences */
	TSharedPtr<SListViewType> DiffList;
};

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

	BuildDiffSourceArray();
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

	const bool bHasDiffs = DiffListSource.Num() > 0;

	//give it a red color to indicate that the graph contains differences
	if(bHasDiffs)
	{
		Color =  FLinearColor(1.0f,0.2f,0.3f);
	}

	return SNew(SHorizontalBox)
	+ SHorizontalBox::Slot()
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

TSharedRef<SWidget> FListItemGraphToDiff::GenerateDiffListWidget()
{
	if(DiffListSource.Num() > 0)
	{
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
		Diff->OnDiffListSelectionChanged(Item);
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

	FName GraphName = GraphOld ? GraphOld->GetFName() : GraphNew->GetFName();

	DiffListSource.Empty();
	for (auto DiffIt(FoundDiffs.CreateIterator()); DiffIt; ++DiffIt)
	{
		DiffIt->OwningGraph = GraphName;
		DiffListSource.Add(FSharedDiffOnGraph(new FDiffResultItem(*DiffIt)));
	}

	struct SortDiff
	{
		bool operator () (const FSharedDiffOnGraph& A, const FSharedDiffOnGraph& B) const
		{
			return A->Result.Diff < B->Result.Diff;
		}
	};

	Sort(DiffListSource.GetData(), DiffListSource.Num(), SortDiff());
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

bool FListItemGraphToDiff::HasNextDifference() const
{
	if (!DiffList.IsValid())
	{
		return false;
	}
	int32 Index = GetCurrentDiffIndex();
	return DiffListSource.IsValidIndex(Index + 1);
}

bool FListItemGraphToDiff::HasPrevDifference() const
{
	if (!DiffList.IsValid())
	{
		return false;
	}

	int32 Index = GetCurrentDiffIndex();
	return DiffListSource.IsValidIndex(Index - 1);
}

int32 FListItemGraphToDiff::GetCurrentDiffIndex() const
{
	if ( DiffList.IsValid() )
	{
		auto Selected = DiffList->GetSelectedItems();
		if(Selected.Num() == 1)
		{	
			int32 Index = 0;
			for(auto It(DiffListSource.CreateConstIterator());It;++It,Index++)
			{
				if(*It == Selected[0])
				{
					return Index;
				}
			}
		}
	}
	return -1;
}

void FListItemGraphToDiff::OnGraphChanged( const FEdGraphEditAction& Action )
{
	Diff->OnGraphChanged(this);
}

FDiffPanel::FDiffPanel()
{
	Blueprint = NULL;
	LastFocusedPin = NULL;
}

void FDiffPanel::InitializeDiffPanel()
{
	TSharedRef< SKismetInspector > Inspector = SNew(SKismetInspector)
		.HideNameArea(true)
		.ViewIdentifier(FName("BlueprintInspector"))
		.MyBlueprintWidget(MyBlueprint)
		.IsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateStatic([] { return false; }));
	DetailsView = Inspector;
	MyBlueprint->SetInspector(DetailsView);
}

int32 GetCurrentIndex( SListView< TSharedPtr< struct FDiffSingleResult> > const& ListView, const TArray< TSharedPtr< FDiffSingleResult > >& ListViewSource )
{
	const auto& Selected = ListView.GetSelectedItems();
	if (Selected.Num() == 1)
	{
		int32 Index = 0;
		for (auto It(ListViewSource.CreateConstIterator()); It; ++It, Index++)
		{
			if (*It == Selected[0])
			{
				return Index;
			}
		}
	}
	return -1;
}

void DiffWidgetUtils::SelectNextRow( SListView< TSharedPtr< FDiffSingleResult> >& ListView, const TArray< TSharedPtr< FDiffSingleResult > >& ListViewSource )
{
	int32 CurrentIndex = GetCurrentIndex(ListView, ListViewSource);
	if( CurrentIndex == ListViewSource.Num() - 1 )
	{
		return;
	}

	ListView.SetSelection(ListViewSource[CurrentIndex + 1]);
}

void DiffWidgetUtils::SelectPrevRow(SListView< TSharedPtr< FDiffSingleResult> >& ListView, const TArray< TSharedPtr< FDiffSingleResult > >& ListViewSource )
{
	int32 CurrentIndex = GetCurrentIndex(ListView, ListViewSource);
	if (CurrentIndex == 0)
	{
		return;
	}

	ListView.SetSelection(ListViewSource[CurrentIndex - 1]);
}

bool DiffWidgetUtils::HasNextDifference(SListView< TSharedPtr< struct FDiffSingleResult> >& ListView, const TArray< TSharedPtr< struct FDiffSingleResult > >& ListViewSource)
{
	int32 CurrentIndex = GetCurrentIndex(ListView, ListViewSource);
	return ListViewSource.IsValidIndex(CurrentIndex+1);
}

bool DiffWidgetUtils::HasPrevDifference(SListView< TSharedPtr< struct FDiffSingleResult> >& ListView, const TArray< TSharedPtr< struct FDiffSingleResult > >& ListViewSource)
{
	int32 CurrentIndex = GetCurrentIndex(ListView, ListViewSource);
	return ListViewSource.IsValidIndex(CurrentIndex - 1);
}

//////////////////////////////////////////////////////////////////////////
// SBlueprintDiff

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SBlueprintDiff::Construct( const FArguments& InArgs)
{
	const TSharedRef<SDockTab> MajorTab = SNew(SDockTab)
		.TabRole(ETabRole::MajorTab);
	TabManager = FGlobalTabmanager::Get()->NewTabManager(MajorTab);

	TabManager->RegisterTabSpawner(DiffGraphTabId,
		FOnSpawnTab::CreateRaw(this, &SBlueprintDiff::CreateGraphDiffViews ))
		.SetDisplayName(NSLOCTEXT("SBlueprintDiff", "GraphsTabTitle", "Graphs"))
		.SetTooltipText(NSLOCTEXT("SBlueprintDiff", "GraphsTooltipText", "Differences in the various graphs present in the blueprint"));

	TabManager->RegisterTabSpawner(DiffMyBluerpintTabId,
		FOnSpawnTab::CreateRaw(this, &SBlueprintDiff::CreateMyBlueprintsViews))
		.SetDisplayName(NSLOCTEXT("SBlueprintDiff", "MyBlueprintTabTitle", "My Blueprint"))
		.SetTooltipText(NSLOCTEXT("SBlueprintDiff", "MyBlueprintTooltipText", "Differences in the 'My Blueprints' attributes of the blueprint"));

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

	FToolBarBuilder ToolbarBuilder(TSharedPtr< const FUICommandList >(), FMultiBoxCustomization::None);
	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SBlueprintDiff::PrevDiff),
			FCanExecuteAction::CreateSP( this, &SBlueprintDiff::HasPrevDiff)
		)
		, NAME_None
		, LOCTEXT("PrevDiffLabel", "Prev")
		, LOCTEXT("PrevDiffTooltip", "Go to previous difference")
		, FSlateIcon(FEditorStyle::GetStyleSetName()
		, "BlueprintDif.PrevDiff")
	);
	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateSP(this, &SBlueprintDiff::NextDiff),
			FCanExecuteAction::CreateSP(this, &SBlueprintDiff::HasNextDiff)
		)
		, NAME_None
		, LOCTEXT("NextDiffLabel", "Next")
		, LOCTEXT("NextDiffTooltip", "Go to next difference")
		, FSlateIcon(FEditorStyle::GetStyleSetName(
		), "BlueprintDif.NextDiff")
	);
	ToolbarBuilder.AddSeparator();
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateSP(this, &SBlueprintDiff::OnToggleLockView2))
		, NAME_None
		, LOCTEXT("LockGraphsLabel", "Lock Graphs")
		, LOCTEXT("LockGraphsTooltip", "Lock the various panels displayed in the graph view, so that they scroll together")
		, TAttribute<FSlateIcon>(this, &SBlueprintDiff::GetLockViewImage2)
	);

	GraphPanel = GenerateGraphPanel();
	DefaultsPanel = GenerateDefaultsPanel();
	ComponentsPanel = GenerateComponentsPanel();

	// Unfortunately we can't perform the diff until the UI is generated, the primary reason for this is that
	// details customizations determine what is actually editable:
	for (const auto& Graph : Graphs)
	{
		TArray< TSharedPtr<FBlueprintDifferenceTreeEntry> > Children;
		for (const auto& Difference : Graph->DiffListSource)
		{
			auto ChildEntry = MakeShareable(
				new FBlueprintDifferenceTreeEntry(
				FOnDiffEntryFocused::CreateRaw(this, &SBlueprintDiff::OnDiffListSelectionChanged, Difference)
				, FGenerateDiffEntryWidget::CreateSP(Difference.ToSharedRef(), &FDiffResultItem::GenerateWidget)
				, TArray< TSharedPtr<FBlueprintDifferenceTreeEntry> >()
				)
			);
			Children.Push(ChildEntry);
		}

		auto Entry = MakeShareable(
			new FBlueprintDifferenceTreeEntry(
				FOnDiffEntryFocused::CreateRaw(this, &SBlueprintDiff::OnSelectionChanged, Graph, ESelectInfo::Direct )
				, FGenerateDiffEntryWidget::CreateSP(Graph.ToSharedRef(), &FListItemGraphToDiff::GenerateWidget)
				, Children)
			);
		MasterDifferencesList.Push(Entry);
	}

	//DefaultsPanel.DiffControl->CalculateDifferences(MasterDifferencesList);
	//ComponentsPanel.DiffControl->CalculateDifferences(MasterDifferencesList);

	const auto RowGenerator = []( TSharedPtr< FBlueprintDifferenceTreeEntry > Entry, const TSharedRef<STableViewBase>& Owner ) -> TSharedRef< ITableRow >
	{
		return SNew( STableRow<TSharedPtr<FBlueprintDifferenceTreeEntry> >, Owner )
			[
				Entry->GenerateWidget.Execute()
			];
	};

	const auto ChildrenAccessor = [](TSharedPtr<FBlueprintDifferenceTreeEntry> InTreeItem, TArray< TSharedPtr< FBlueprintDifferenceTreeEntry > >& OutChildren, const TArray< TSharedPtr<FBlueprintDifferenceTreeEntry> >* MasterList )
	{
		OutChildren = InTreeItem->Children;
	};

	const auto Selector = [](TSharedPtr<FBlueprintDifferenceTreeEntry> InTreeItem, ESelectInfo::Type Type)
	{
		if( InTreeItem.IsValid() )
		{
			InTreeItem->OnFocus.ExecuteIfBound();
		}
	};
	
	const TArray< TSharedPtr<FBlueprintDifferenceTreeEntry> >* MasterDifferencesListPtr = &MasterDifferencesList;

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
					.Padding(16.f)
					.AutoWidth()
					[
						ToolbarBuilder.MakeWidget()
					]
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
					SNew(SSplitter)
					+ SSplitter::Slot()
					.Value(.2f)
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							SNew(STreeView< TSharedPtr< FBlueprintDifferenceTreeEntry > >)
							.OnGenerateRow( STreeView< TSharedPtr< FBlueprintDifferenceTreeEntry > >::FOnGenerateRow::CreateStatic(RowGenerator) )
							.OnGetChildren( STreeView< TSharedPtr< FBlueprintDifferenceTreeEntry > >::FOnGetChildren::CreateStatic(ChildrenAccessor, MasterDifferencesListPtr) )
							.OnSelectionChanged( STreeView< TSharedPtr< FBlueprintDifferenceTreeEntry > >::FOnSelectionChanged::CreateStatic(Selector) )
							.TreeItemsSource(&MasterDifferencesList)
						]
					]
					+ SSplitter::Slot()
					.Value(.8f)
					[
						SAssignNew(ModeContents, SBox)
					]
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
	if(!Item.IsValid())
	{
		return;
	}

	FocusOnGraphRevisions(Item.Get());

}

void SBlueprintDiff::OnGraphChanged(FListItemGraphToDiff* Diff)
{
	if(PanelNew.GraphEditor.IsValid() && PanelNew.GraphEditor.Pin()->GetCurrentGraph() == Diff->GetGraphNew())
	{
		FocusOnGraphRevisions(Diff);
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

void SBlueprintDiff::NextDiff()
{
	if( DiffControl.IsValid() )
	{
		DiffControl->NextDiff();
	}
}

void SBlueprintDiff::PrevDiff()
{
	if (DiffControl.IsValid())
	{
		DiffControl->PrevDiff();
	}
}

bool SBlueprintDiff::HasNextDiff() const
{
	return DiffControl.IsValid() && DiffControl->HasNextDifference();
}

bool SBlueprintDiff::HasPrevDiff() const
{
	return DiffControl.IsValid() && DiffControl->HasPrevDifference();
}

TSharedRef<SDockTab> SBlueprintDiff::CreateGraphDiffViews(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
	[
		SNew(SSplitter) 
		+ SSplitter::Slot()
		[
			SAssignNew(PanelOld.GraphEditorBorder, SBox)
			.VAlign(VAlign_Fill)
			[
				DefaultEmptyPanel()
			]
		]
		+ SSplitter::Slot()
		[
			SAssignNew(PanelNew.GraphEditorBorder, SBox)
			.VAlign(VAlign_Fill)
			[
				DefaultEmptyPanel()
			]
		]
	];
}

TSharedRef<SDockTab> SBlueprintDiff::CreateMyBlueprintsViews(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
	[
		SNew(SSplitter)
		+ SSplitter::Slot()
		[
			PanelOld.GenerateMyBlueprintPanel()
		]
		+ SSplitter::Slot()
		[
			PanelNew.GenerateMyBlueprintPanel()
		]
	];
}

FListItemGraphToDiff* SBlueprintDiff::FindGraphToDiffEntry(FName ByName)
{
	for( const auto& Graph : Graphs )
	{
		FName GraphName = Graph->GetGraphOld() ? Graph->GetGraphOld()->GetFName() : Graph->GetGraphNew()->GetFName();
		if (GraphName == ByName )
		{
			return Graph.Get();
		}
	}
	return nullptr;
}

void SBlueprintDiff::FocusOnGraphRevisions( FListItemGraphToDiff* Diff )
{
	FName GraphName = Diff->GetGraphOld() ? Diff->GetGraphOld()->GetFName() : Diff->GetGraphNew()->GetFName();
	HandleGraphChanged( GraphName );

	ResetGraphEditors();
}

void SBlueprintDiff::OnDiffListSelectionChanged(TSharedPtr<FDiffResultItem> TheDiff )
{
	check( TheDiff->Result.OwningGraph != FName() );
	FocusOnGraphRevisions( FindGraphToDiffEntry( TheDiff->Result.OwningGraph ) );
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

void	SBlueprintDiff::OnToggleLockView2()
{
	OnToggleLockView();
}

FReply	SBlueprintDiff::OnToggleLockView()
{
	bLockViews = !bLockViews;
	ResetGraphEditors();
	return FReply::Handled();
}

FSlateIcon SBlueprintDiff::GetLockViewImage2() const
{
	return FSlateIcon(FEditorStyle::GetStyleSetName(), bLockViews ? "GenericLock" : "GenericUnlock");
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

void FDiffPanel::GeneratePanel(UEdGraph* Graph, UEdGraph* GraphToDiff )
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
		{
			const auto SelectionChangedHandler = [](const FGraphPanelSelectionSet& SelectionSet, TSharedPtr<SKismetInspector> Container)
			{
				Container->ShowDetailsForObjects(SelectionSet.Array());
			};
			InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateStatic(SelectionChangedHandler, DetailsView);
		}

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

		MyBlueprint->SetFocusedGraph(Graph);
		MyBlueprint->Refresh();

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

TSharedRef<SWidget> FDiffPanel::GenerateMyBlueprintPanel()
{
	return SAssignNew(MyBlueprint, SMyBlueprint, TWeakPtr<FBlueprintEditor>(), Blueprint);
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

void SBlueprintDiff::HandleGraphChanged( const FName GraphName )
{
	if( CurrentMode != FBlueprintEditorApplicationModes::StandardBlueprintEditorMode)
	{
		SetCurrentMode(FBlueprintEditorApplicationModes::StandardBlueprintEditorMode);
	}
	
	TArray<UEdGraph*> GraphsOld, GraphsNew;
	PanelOld.Blueprint->GetAllGraphs(GraphsOld);
	PanelNew.Blueprint->GetAllGraphs(GraphsNew);

	UEdGraph* GraphOld = NULL;
	if( UEdGraph** Iter = GraphsOld.FindByPredicate(FMatchFName(GraphName)) )
	{
		GraphOld = *Iter;
	}
	UEdGraph* GraphNew = NULL;
	if (UEdGraph** Iter = GraphsNew.FindByPredicate(FMatchFName(GraphName)))
	{
		GraphNew = *Iter;
	}

	PanelOld.GeneratePanel(GraphOld, GraphNew);
	PanelNew.GeneratePanel(GraphNew, GraphOld);
}

SBlueprintDiff::FDiffControl SBlueprintDiff::GenerateGraphPanel()
{
	const TSharedRef<FTabManager::FLayout> DefaultLayout = FTabManager::NewLayout("BlueprintDiff_Layout_v1")
	->AddArea
	(
		FTabManager::NewPrimaryArea()
		->Split
		(
			FTabManager::NewStack()
			->AddTab(DiffMyBluerpintTabId, ETabState::OpenedTab)
			->AddTab(DiffGraphTabId, ETabState::OpenedTab)
		)
	);

	// SMyBlueprint needs to be created *before* the KismetInspector, because the KismetInspector's customizations
	// need a reference to the SMyBlueprint widget that is controlling them...
	TSharedRef<SWidget> TabControl = TabManager->RestoreFrom(DefaultLayout, TSharedPtr<SWindow>()).ToSharedRef();

	const auto CreateInspector = [](TSharedPtr<SMyBlueprint> InMyBlueprint) {
		return SNew( SKismetInspector)
			.HideNameArea(true)
			.ViewIdentifier(FName("BlueprintInspector"))
			.MyBlueprintWidget(InMyBlueprint)
			.IsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateStatic([] { return false; }));
	};

	SBlueprintDiff::FDiffControl Ret;

	PanelOld.DetailsView = CreateInspector(PanelOld.MyBlueprint);
	PanelOld.MyBlueprint->SetInspector(PanelOld.DetailsView);
	PanelNew.DetailsView = CreateInspector(PanelNew.MyBlueprint);
	PanelNew.MyBlueprint->SetInspector(PanelNew.DetailsView);

	Ret.Widget = SNew(SBorder)
	.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
	.Content()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				//diff window
				SNew(SSplitter)
				.Orientation(Orient_Vertical)
				+SSplitter::Slot()
				.Value(.8f)
				[
					// graph and my blueprint views:
					TabControl
				]
				+ SSplitter::Slot()
				.Value(.2f)
				[
					SNew( SSplitter )
					+SSplitter::Slot()
					[
						PanelOld.DetailsView.ToSharedRef()
					]
					+ SSplitter::Slot()
					[
						PanelNew.DetailsView.ToSharedRef()
					]
				]
			]
		]
	];

	return Ret;
}

SBlueprintDiff::FDiffControl SBlueprintDiff::GenerateDefaultsPanel()
{
	const UObject* A = DiffUtils::GetCDO(PanelOld.Blueprint);
	const UObject* B = DiffUtils::GetCDO(PanelNew.Blueprint);
	TArray<FPropertySoftPath> DifferingProperties;
	DiffUtils::CompareUnrelatedObjects( A, B, DifferingProperties);

	SBlueprintDiff::FDiffControl Ret;
	auto NewDiffControl = TSharedPtr<FCDODiffControl>(new FCDODiffControl(A, B, DifferingProperties) );
	//Splitter for left and right blueprint. Current convention is for the local (probably newer?) blueprint to be on the right:
	Ret.DiffControl = NewDiffControl;
	Ret.Widget = SNew(SSplitter)
		+ SSplitter::Slot()
		.Value(0.5f)
		[
			SNew(SBorder)
			.VAlign(VAlign_Fill)
			[
				NewDiffControl->OldDetailsWidget()
			]
		]
	+ SSplitter::Slot()
		.Value(0.5f)
		[
			SNew(SBorder)
			.VAlign(VAlign_Fill)
			[
				NewDiffControl->NewDetailsWidget()
			]
		];

	return Ret;
}

SBlueprintDiff::FDiffControl SBlueprintDiff::GenerateComponentsPanel()
{
	SBlueprintDiff::FDiffControl Ret;

	//Splitter for left and right blueprint. Current convention is for the local (probably newer?) blueprint to be on the right:
	auto NewDiffControl = TSharedPtr<FSCSDiffControl>(new FSCSDiffControl(PanelOld.Blueprint, PanelNew.Blueprint) );
	Ret.DiffControl = NewDiffControl;
	Ret.Widget = SNew(SSplitter)
		+ SSplitter::Slot()
		.Value(0.5f)
		[
			SNew(SBorder)
			.VAlign(VAlign_Fill)
			[
				NewDiffControl->OldTreeWidget()
			]
		]
		+ SSplitter::Slot()
		.Value(0.5f)
		[
			SNew(SBorder)
			.VAlign(VAlign_Fill)
			[
				NewDiffControl->NewTreeWidget()
			]
		];

	return Ret;
}

void SBlueprintDiff::SetCurrentMode(FName NewMode)
{
	CurrentMode = NewMode;

	DiffControl = TSharedPtr< IDiffControl >();
	if( NewMode == FBlueprintEditorApplicationModes::StandardBlueprintEditorMode)
	{
		DiffControl = GraphPanel.DiffControl;
		ModeContents->SetContent( GraphPanel.Widget.ToSharedRef() );
	}
	else if( NewMode == FBlueprintEditorApplicationModes::BlueprintDefaultsMode )
	{
		DiffControl = DefaultsPanel.DiffControl;
		ModeContents->SetContent( DefaultsPanel.Widget.ToSharedRef() );
	}
	else if (NewMode == FBlueprintEditorApplicationModes::BlueprintComponentsMode)
	{
		DiffControl = ComponentsPanel.DiffControl;
		ModeContents->SetContent( ComponentsPanel.Widget.ToSharedRef() );
	}
	else
	{
		ensureMsgf(false, TEXT("Diff panel does not support mode %s"), *NewMode.ToString() );
	}
}

#undef LOCTEXT_NAMESPACE

