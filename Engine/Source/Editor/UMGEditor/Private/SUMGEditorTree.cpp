// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SUMGEditorTree.h"
#include "UMGEditorActions.h"
#include "SUMGEditorTreeItem.h"

#include "PreviewScene.h"
#include "SceneViewport.h"

#include "BlueprintEditor.h"
#include "SKismetInspector.h"
#include "BlueprintEditorUtils.h"

void SUMGEditorTree::Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintEditor, USimpleConstructionScript* InSCS)
{
	BlueprintEditor = InBlueprintEditor;

	UWidgetBlueprint* Blueprint = GetBlueprint();
	Blueprint->OnChanged().AddSP(this, &SUMGEditorTree::OnBlueprintChanged);

	FCoreDelegates::OnObjectPropertyChanged.Add( FCoreDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(this, &SUMGEditorTree::OnObjectPropertyChanged) );

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SButton)
			.OnClicked(this, &SUMGEditorTree::CreateTestUI)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("SUMGEditorTree", "CreateTestUI", "Create Test UI"))
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SButton)
			.OnClicked(this, &SUMGEditorTree::DeleteSelected)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("SUMGEditorTree", "DeleteSelected", "Delete Selected"))
			]
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(WidgetTreeView, STreeView< USlateWrapperComponent* >)
			.ItemHeight(20.0f)
			.SelectionMode(ESelectionMode::Single)
			.OnGetChildren(this, &SUMGEditorTree::WidgetHierarchy_OnGetChildren)
			.OnGenerateRow(this, &SUMGEditorTree::WidgetHierarchy_OnGenerateRow)
			.OnSelectionChanged(this, &SUMGEditorTree::WidgetHierarchy_OnSelectionChanged)
			.TreeItemsSource(&RootWidgets)
		]
	];

	RefreshTree();
}

SUMGEditorTree::~SUMGEditorTree()
{
	UWidgetBlueprint* Blueprint = GetBlueprint();
	if ( Blueprint )
	{
		Blueprint->OnChanged().RemoveAll(this);
	}

	FCoreDelegates::OnObjectPropertyChanged.Remove( FCoreDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(this, &SUMGEditorTree::OnObjectPropertyChanged) );
}

UWidgetBlueprint* SUMGEditorTree::GetBlueprint() const
{
	if ( BlueprintEditor.IsValid() )
	{
		UBlueprint* BP = BlueprintEditor.Pin()->GetBlueprintObj();
		return CastChecked<UWidgetBlueprint>(BP);
	}

	return NULL;
}

void SUMGEditorTree::OnBlueprintChanged(UBlueprint* InBlueprint)
{
	if ( InBlueprint )
	{
		RefreshTree();
	}
}

void SUMGEditorTree::OnObjectPropertyChanged(UObject* ObjectBeingModified)
{
	if ( !ensure(ObjectBeingModified) )
	{
		return;
	}
}

void SUMGEditorTree::ShowDetailsForObjects(TArray<USlateWrapperComponent*> Widgets)
{
	// Convert the selection set to an array of UObject* pointers
	FString InspectorTitle;
	TArray<UObject*> InspectorObjects;
	InspectorObjects.Empty(Widgets.Num());
	for ( USlateWrapperComponent* Widget : Widgets )
	{
		//if ( NodePtr->CanEditDefaults() )
		{
			InspectorTitle = "Widget";// Widget->GetDisplayString();
			InspectorObjects.Add(Widget);
		}
	}

	UWidgetBlueprint* Blueprint = GetBlueprint();

	// Update the details panel
	SKismetInspector::FShowDetailsOptions Options(InspectorTitle, true);
	BlueprintEditor.Pin()->GetInspector()->ShowDetailsForObjects(InspectorObjects, Options);
}

void SUMGEditorTree::WidgetHierarchy_OnGetChildren(USlateWrapperComponent* InParent, TArray< USlateWrapperComponent* >& OutChildren)
{
	USlateNonLeafWidgetComponent* Widget = Cast<USlateNonLeafWidgetComponent>(InParent);
	if ( Widget )
	{
		for ( int32 i = 0; i < Widget->GetChildrenCount(); i++ )
		{
			USlateWrapperComponent* Child = Widget->GetChildAt(i);
			if ( Child )
			{
				OutChildren.Add(Child);
			}
		}
	}
}

TSharedRef< ITableRow > SUMGEditorTree::WidgetHierarchy_OnGenerateRow(USlateWrapperComponent* InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return
		SNew(STableRow< USlateWrapperComponent* >, OwnerTable)
		.Padding(2.0f)
//		.OnDragDetected(this, &SUMGEditorWidgetTemplates::OnDraggingWidgetTemplateItem)
		[
			SNew(SUMGEditorTreeItem, BlueprintEditor.Pin(), InItem)
		];
}

void SUMGEditorTree::WidgetHierarchy_OnSelectionChanged(USlateWrapperComponent* SelectedItem, ESelectInfo::Type SelectInfo)
{
	if ( SelectInfo != ESelectInfo::Direct )
	{
		TArray<USlateWrapperComponent*> Items;
		Items.Add(SelectedItem);
		ShowDetailsForObjects(Items);
	}
}

FReply SUMGEditorTree::CreateTestUI()
{
	UWidgetBlueprint* BP = GetBlueprint();
	TArray<USlateWrapperComponent*>& WidgetTemplates = BP->WidgetTree->WidgetTemplates;

	if ( WidgetTemplates.Num() > 0 )
	{
		UVerticalBoxComponent* Vertical = CastChecked<UVerticalBoxComponent>(WidgetTemplates[2]);

		UButtonComponent* NewButton = BP->WidgetTree->ConstructWidget<UButtonComponent>(UButtonComponent::StaticClass());

		Vertical->AddChild(NewButton, FVector2D(0,0));
	}
	else
	{
		UCanvasPanelComponent* Canvas = BP->WidgetTree->ConstructWidget<UCanvasPanelComponent>(UCanvasPanelComponent::StaticClass());
		UBorderComponent* Border = BP->WidgetTree->ConstructWidget<UBorderComponent>(UBorderComponent::StaticClass());
		UVerticalBoxComponent* Vertical = BP->WidgetTree->ConstructWidget<UVerticalBoxComponent>(UVerticalBoxComponent::StaticClass());
		UButtonComponent* Button1 = BP->WidgetTree->ConstructWidget<UButtonComponent>(UButtonComponent::StaticClass());
		UButtonComponent* Button2 = BP->WidgetTree->ConstructWidget<UButtonComponent>(UButtonComponent::StaticClass());
		UButtonComponent* Button3 = BP->WidgetTree->ConstructWidget<UButtonComponent>(UButtonComponent::StaticClass());
		UButtonComponent* Button4 = BP->WidgetTree->ConstructWidget<UButtonComponent>(UButtonComponent::StaticClass());

		UTextBlockComponent* Text1 = BP->WidgetTree->ConstructWidget<UTextBlockComponent>(UTextBlockComponent::StaticClass());
		Text1->Text = FText::FromString(TEXT("Button!"));
		Button4->SetContent(Text1);

		UCanvasPanelSlot* Slot = Canvas->AddSlot(Border);
		Slot->Size.X = 200;
		Slot->Size.Y = 300;
		Slot->Position.X = 20;
		Slot->Position.Y = 50;

		Border->SetContent(Vertical);

		Vertical->AddSlot(Button1);
		Vertical->AddSlot(Button2);
		Vertical->AddSlot(Button3);
		Vertical->AddSlot(Button4);
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);

	return FReply::Handled();
}

FReply SUMGEditorTree::DeleteSelected()
{
	TArray<USlateWrapperComponent*> SelectedItems = WidgetTreeView->GetSelectedItems();
	if ( SelectedItems.Num() > 0 )
	{
		UWidgetBlueprint* BP = GetBlueprint();

		bool bRemoved = false;
		for ( USlateWrapperComponent* Item : SelectedItems )
		{
			bRemoved = BP->WidgetTree->RemoveWidget(Item);
		}

		if ( bRemoved )
		{
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		}
	}

	return FReply::Handled();
}

void SUMGEditorTree::RefreshTree()
{
	RootWidgets.Reset();

	UWidgetBlueprint* Blueprint = GetBlueprint();
	TArray<USlateWrapperComponent*>& WidgetTemplates = Blueprint->WidgetTree->WidgetTemplates;

	if ( WidgetTemplates.Num() > 0 )
	{
		RootWidgets.Add(WidgetTemplates[0]);
	}

	WidgetTreeView->RequestTreeRefresh();
}


//@TODO UMG Drop widgets onto the tree, when nothing is present, if there is a root node present, what happens then, let the root node attempt to place it?