// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintEditorPrivatePCH.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "GraphEditor.h"
#include "BlueprintUtilities.h"

#include "SGraphTitleBar.h"

#define LOCTEXT_NAMESPACE "SGraphTitleBar"


SGraphTitleBar::~SGraphTitleBar()
{
	// Unregister for notifications
	if (Kismet2Ptr.IsValid())
	{
		Kismet2Ptr.Pin()->OnRefresh().RemoveAll(this);
	}
}

const FSlateBrush* SGraphTitleBar::GetTypeGlyph() const
{
	check(EdGraphObj != NULL);
	return FBlueprintEditor::GetGlyphForGraph(EdGraphObj, true);
}

FString SGraphTitleBar::GetTitleForOneCrumb(const UEdGraph* Graph)
{
	const UEdGraphSchema* Schema = Graph->GetSchema();

	FGraphDisplayInfo DisplayInfo;
	Schema->GetGraphDisplayInformation(*Graph, /*out*/ DisplayInfo);

	return DisplayInfo.DisplayName.ToString() + TEXT(" ") + DisplayInfo.GetNotesAsString();
}

FString SGraphTitleBar::GetTitleExtra() const
{
	check(EdGraphObj != NULL);

	const bool bEditable = (EdGraphObj->bEditable) && (Kismet2Ptr.Pin()->InEditingMode());
	return (bEditable)
		? TEXT("")
		: TEXT(" (READ-ONLY)");
}


EVisibility SGraphTitleBar::IsGraphBlueprintNameVisible() const
{
	return bShowBlueprintTitle ? EVisibility::Visible : EVisibility::Collapsed;
}

void SGraphTitleBar::OnBreadcrumbClicked(UEdGraph* const& Item)
{
	OnDifferentGraphCrumbClicked.ExecuteIfBound(Item);
}

void SGraphTitleBar::Construct( const FArguments& InArgs )
{
	EdGraphObj = InArgs._EdGraphObj;
	OnDifferentGraphCrumbClicked = InArgs._OnDifferentGraphCrumbClicked;

	check(EdGraphObj);
	Kismet2Ptr = InArgs._Kismet2;
	check(Kismet2Ptr.IsValid());

	bEditingFunction = false;

	// Set-up shared breadcrumb defaults
	FMargin BreadcrumbTrailPadding = FMargin(4.f, 2.f);
	const FSlateBrush* BreadcrumbButtonImage = FEditorStyle::GetBrush("BreadcrumbTrail.Delimiter");
	
	this->ChildSlot
	[
		SNew(STutorialWrapper)
		.Name(TEXT("EventGraphTitleBar"))
		.Content()
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush( TEXT("Graph.TitleBackground") ) )
			.HAlign(HAlign_Fill)
			[
				SNew(SVerticalBox)
				// Title text/icon
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						InArgs._HistoryNavigationWidget.ToSharedRef()
					]

					+SHorizontalBox::Slot()
					.HAlign(HAlign_Center)
					.FillWidth(1.f)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding( 10,5 )
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
							.Image( this, &SGraphTitleBar::GetTypeGlyph )
						]

						// show fake 'root' breadcrumb for the title
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(BreadcrumbTrailPadding)
						[
							SNew(STextBlock)
							.Text(this, &SGraphTitleBar::GetBlueprintTitle )
							.TextStyle( FEditorStyle::Get(), TEXT("GraphBreadcrumbButtonText") )
							.Visibility( this, &SGraphTitleBar::IsGraphBlueprintNameVisible )
						]
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
							.Image( BreadcrumbButtonImage )
							.Visibility( this, &SGraphTitleBar::IsGraphBlueprintNameVisible )
						]

						// New style breadcrumb
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SAssignNew(BreadcrumbTrail, SBreadcrumbTrail<UEdGraph*>)
							.ButtonStyle(FEditorStyle::Get(), "GraphBreadcrumbButton")
							.TextStyle(FEditorStyle::Get(), "GraphBreadcrumbButtonText")
							.ButtonContentPadding( BreadcrumbTrailPadding )
							.DelimiterImage( BreadcrumbButtonImage )
							.PersistentBreadcrumbs( true )
							.OnCrumbClicked( this, &SGraphTitleBar::OnBreadcrumbClicked )
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Font( FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 14 ) )
							.ColorAndOpacity( FLinearColor(1,1,1,0.5) )
							.Text( this, &SGraphTitleBar::GetTitleExtra )
						]
					]
				]
			]
		]
	];

	RebuildBreadcrumbTrail();

	UBlueprint* BlueprintObj = FBlueprintEditorUtils::FindBlueprintForGraph(this->EdGraphObj);
	if (BlueprintObj)
	{
		bShowBlueprintTitle = true;
		BlueprintTitle = BlueprintObj->GetFriendlyName();

		// Register for notifications to refresh UI
		if( Kismet2Ptr.IsValid() )
		{
			Kismet2Ptr.Pin()->OnRefresh().AddRaw(this, &SGraphTitleBar::Refresh);
		}
	}
}

void SGraphTitleBar::RebuildBreadcrumbTrail()
{
	// Build up a stack of graphs so we can pop them in reverse order and create breadcrumbs
	TArray<UEdGraph*> Stack;
	for (UEdGraph* OuterChain = EdGraphObj; OuterChain != NULL; OuterChain = GetOuterGraph(OuterChain))
	{
		Stack.Push(OuterChain);
	}

	BreadcrumbTrail->ClearCrumbs(false);

	//Get the last object in the array
	UEdGraph* LastObj = NULL;
	if( Stack.Num() > 0 )
	{
		LastObj = Stack[Stack.Num() -1];
	}

	while (Stack.Num() > 0)
	{
		UEdGraph* Graph = Stack.Pop();
		
		auto Foo = TAttribute<FString>::Create(TAttribute<FString>::FGetter::CreateStatic<const UEdGraph*>(&SGraphTitleBar::GetTitleForOneCrumb, Graph));
		BreadcrumbTrail->PushCrumb(Foo, Graph);
	}
}

void SGraphTitleBar::BeginEditing()
{
	bEditingFunction = true;
}

UEdGraph* SGraphTitleBar::GetOuterGraph( UObject* Obj )
{
	if( Obj )
	{
		UObject* OuterObj = Obj->GetOuter();
		if( OuterObj )
		{
			if( OuterObj->IsA( UEdGraph::StaticClass()) )
			{
				return Cast<UEdGraph>(OuterObj);
			}
			else
			{
				return GetOuterGraph( OuterObj );
			}
		}
	}
	return NULL;
}

FString SGraphTitleBar::GetBlueprintTitle() const
{
	return BlueprintTitle;
}

void SGraphTitleBar::Refresh()
{
	// Refresh UI on request
	if (EdGraphObj)
	{
		if (UBlueprint* BlueprintObj = FBlueprintEditorUtils::FindBlueprintForGraph(this->EdGraphObj))
		{
			BlueprintTitle = BlueprintObj->GetFriendlyName();
			RebuildBreadcrumbTrail();
		}
	}
}

#undef LOCTEXT_NAMESPACE
