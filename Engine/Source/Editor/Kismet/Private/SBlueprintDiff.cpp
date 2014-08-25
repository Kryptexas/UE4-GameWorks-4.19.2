// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

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
#include "SGraphTitleBar.h"
#include "SSCSEditor.h"
#include "WorkflowOrientedApp/SModeWidget.h"

#define LOCTEXT_NAMESPACE "SBlueprintDif"

typedef TMap< FName, const UProperty* > FNamePropertyMap;

class FCDODiffControl : public TSharedFromThis<FCDODiffControl>
	, public IDiffControl
{
public:
	FCDODiffControl( const UObject* InOldCDO
					, FNamePropertyMap InOldProperties
					, const UObject* InNewCDO
					, FNamePropertyMap InNewProperities
					, TArray<FName> InDifferingProperties );

	TSharedRef<SWidget> OldDetailsWidget() { return OldDetails.DetailsWidget(); }
	TSharedRef<SWidget> NewDetailsWidget() { return NewDetails.DetailsWidget(); }

	virtual ~FCDODiffControl() {}
private:
	void NextDiff() override;
	void PrevDiff() override;
	bool HasDifferences() const override;

	void HighlightCurrentDifference();
	FDetailsDiff OldDetails;
	FDetailsDiff NewDetails;

	TArray< FName > DifferingProperties;
	int CurrentDifference;
};

FCDODiffControl::FCDODiffControl( 
		const UObject* InOldCDO
		, FNamePropertyMap InOldProperties
		, const UObject* InNewCDO
		, FNamePropertyMap InNewProperities
		, TArray<FName> InDifferingProperties )
	: OldDetails(InOldCDO, InOldProperties, TSet<FName>(InDifferingProperties))
	, NewDetails(InNewCDO, InNewProperities, TSet<FName>(InDifferingProperties))
	, DifferingProperties( InDifferingProperties )
	, CurrentDifference( -1 )
{
}

void FCDODiffControl::NextDiff()
{
	if (DifferingProperties.Num() == 0)
	{
		return;
	}

	CurrentDifference = (CurrentDifference + 1) % DifferingProperties.Num();

	HighlightCurrentDifference();
}

void FCDODiffControl::PrevDiff()
{
	if (DifferingProperties.Num() == 0)
	{
		return;
	}

	--CurrentDifference;
	if (CurrentDifference < 0)
	{
		CurrentDifference = DifferingProperties.Num() - 1;
	}

	HighlightCurrentDifference();
}

bool FCDODiffControl::HasDifferences() const
{
	return DifferingProperties.Num() != 0;
}

void FCDODiffControl::HighlightCurrentDifference()
{
	const FName PropertyName = DifferingProperties[CurrentDifference];

	OldDetails.HighlightProperty(PropertyName);
	NewDetails.HighlightProperty(PropertyName);
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

	/** Called when user presses key within the diff view */
	void KeyWasPressed(const FKeyboardEvent& InKeyboardEvent);

private:

	/** Called when user clicks button to go to next difference in graph */
	void NextDiff() override;

	/** Called when user clicks button to go to prev difference in graph */
	void PrevDiff() override;

	/** Called when determining whether the prev/nextbuttons should be enabled */
	bool HasDifferences() const override;

	/** Get Index of the current diff that is selected */
	int32 GetCurrentDiffIndex();

	/* Called when a new row is being generated */
	TSharedRef<ITableRow> OnGenerateRow(FSharedDiffOnGraph ParamItem, const TSharedRef<STableViewBase>& OwnerTable);

	/** ListView of differences */
	TSharedPtr<SListViewType> DiffList;

	/** Source for list view */
	TArray<FSharedDiffOnGraph> DiffListSource;

	/** Key commands processed by this widget */
	TSharedPtr< FUICommandList > KeyCommands;
};

FNamePropertyMap DiffUtils::GetProperties( const UObject* ForObj )
{
	FNamePropertyMap Ret;
	if( ForObj )
	{
		const UClass* Class = ForObj->GetClass();
		for (TFieldIterator<UProperty> PropertyIt(Class, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			if ((!PropertyIt->HasAnyPropertyFlags(CPF_Parm) && PropertyIt->HasAnyPropertyFlags(CPF_Edit)))
			{
				Ret.Add(PropertyIt->GetFName(), *PropertyIt);
			}
		}
	}
	return Ret;
}

static void GetPropertyNameSet( const UObject& ForObj, TSet< FName >& OutProperties )
{
	if( const UClass* Class = ForObj.GetClass() )
	{
		for (TFieldIterator<UProperty> PropertyIt(Class, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			if ((!PropertyIt->HasAnyPropertyFlags(CPF_Parm) && PropertyIt->HasAnyPropertyFlags(CPF_Edit)))
			{
				OutProperties.Add(PropertyIt->GetFName());
			}
		}
	}
}

const UObject* DiffUtils::GetCDO( const UBlueprint* ForBlueprint )
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

bool FListItemGraphToDiff::HasDifferences() const
{
	return DiffList.IsValid() && DiffListSource.Num() > 0;
}

int32 FListItemGraphToDiff::GetCurrentDiffIndex() 
{
	if ( DiffList.IsValid() )
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

void DiffUtils::CompareUnrelatedObjects( UObject const* A, const TMap< FName, const UProperty* >& PropertyMapA, UObject const* B, const TMap< FName, const UProperty* >& PropertyMapB, TArray<FName> &OutIdenticalProperties, TArray<FName> &OutDifferingProperties )
{
	TSet<FName> PropertiesInA, PropertiesInB;
	if( A )
	{
		GetPropertyNameSet(*A, PropertiesInA);
	}
	if( B )
	{
		GetPropertyNameSet(*B, PropertiesInB);
	}

	// any properties in A that aren't in B are differing:
	OutDifferingProperties.Append( PropertiesInA.Difference(PropertiesInB).Array() );

	// and the converse:
	OutDifferingProperties.Append( PropertiesInB.Difference(PropertiesInA).Array() );

	// for properties in common, dig out the uproperties and determine if they're identical:
	if( A && B )
	{
		TSet<FName> Common = PropertiesInA.Intersect(PropertiesInB);
		for (const auto& PropertyName : Common)
		{
			const UProperty* const* AProp = PropertyMapA.Find(PropertyName);
			const UProperty* const* BProp = PropertyMapB.Find(PropertyName);
			check( AProp && BProp );

			if( (*AProp)->SameType(*BProp) )
			{
				const void* AValue = (*AProp)->ContainerPtrToValuePtr<void>(A);
				const void* BValue = (*BProp)->ContainerPtrToValuePtr<void>(B);

				if ((*AProp)->Identical(AValue, BValue, PPF_DeepComparison))
				{
					OutIdenticalProperties.Add(PropertyName);
					continue;
				}
			}

			OutDifferingProperties.Add(PropertyName);
		}
	}
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
	if (CurrentIndex > 0)
	{
		return;
	}

	ListView.SetSelection(ListViewSource[CurrentIndex - 1]);
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

	const FDiffListCommands& Commands = FDiffListCommands::Get();
	KeyCommands = MakeShareable(new FUICommandList);

	KeyCommands->MapAction(Commands.Previous, FExecuteAction::CreateSP(this, &SBlueprintDiff::PrevDiff), FCanExecuteAction::CreateSP( this, &SBlueprintDiff::CanCycleDiffs) );
	KeyCommands->MapAction(Commands.Next, FExecuteAction::CreateSP(this, &SBlueprintDiff::NextDiff), FCanExecuteAction::CreateSP( this, &SBlueprintDiff::CanCycleDiffs) );

	FToolBarBuilder ToolbarBuilder(KeyCommands.ToSharedRef(), FMultiBoxCustomization::None);
	ToolbarBuilder.AddToolBarButton(Commands.Previous, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "BlueprintDif.PrevDiff"));
	ToolbarBuilder.AddToolBarButton(Commands.Next, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "BlueprintDif.NextDiff"));

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

bool SBlueprintDiff::CanCycleDiffs() const
{
	return DiffControl.IsValid() && DiffControl->HasDifferences();
}

void SBlueprintDiff::FocusOnGraphRevisions( class UEdGraph* GraphOld, class UEdGraph* GraphNew, FListItemGraphToDiff* Diff )
{
	HandleGraphChanged( GraphOld ? GraphOld->GetName() : GraphNew->GetName() );

	ResetGraphEditors();

	DiffControl = Diff->AsShared();
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
	const UObject* A = DiffUtils::GetCDO(PanelOld.Blueprint);
	FNamePropertyMap PropertyMapA = DiffUtils::GetProperties(A);
	const UObject* B = DiffUtils::GetCDO(PanelNew.Blueprint);
	FNamePropertyMap PropertyMapB = DiffUtils::GetProperties(B);
	TArray<FName> IdenticalProperties, DifferingProperties;
	DiffUtils::CompareUnrelatedObjects( A, PropertyMapA, B, PropertyMapB, IdenticalProperties, DifferingProperties);

	auto NewDiffControl = TSharedPtr<FCDODiffControl>(new FCDODiffControl(A, PropertyMapA, B, PropertyMapB, DifferingProperties) );
	//Splitter for left and right blueprint. Current convention is for the local (probably newer?) blueprint to be on the right:
	DiffControl = NewDiffControl;
	return SNew(SSplitter)
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

	DiffControl = TSharedPtr< IDiffControl >();
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

