// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SUMGEditorTree.h"
#include "UMGEditor.h"
#include "UMGEditorViewportClient.h"
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
			OutChildren.Add( Widget->GetChildAt(i) );
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
	//TSharedRef<ISCSEditor> SCSEditor = BlueprintEditor.Pin()->GetSCSEditorModel();
	//UCanvasPanelComponent* Canvas = Cast<UCanvasPanelComponent>(SCSEditor->AddNewComponent(UCanvasPanelComponent::StaticClass(), NULL));
	//UButtonComponent* Button = Cast<UButtonComponent>(SCSEditor->AddNewComponent(UButtonComponent::StaticClass(), NULL));

	UWidgetBlueprint* BP = GetBlueprint();

	if ( BP->WidgetTemplates.Num() > 0 )
	{
		UVerticalBoxComponent* Vertical = CastChecked<UVerticalBoxComponent>(BP->WidgetTemplates[2]);

		UButtonComponent* NewButton = ConstructObject<UButtonComponent>(UButtonComponent::StaticClass(), BP);
		NewButton->ButtonText = FText::FromString("New Button");

		Vertical->AddChild(NewButton);

		BP->WidgetTemplates.Add(NewButton);
	}
	else
	{
		UCanvasPanelComponent* Canvas = ConstructObject<UCanvasPanelComponent>(UCanvasPanelComponent::StaticClass(), BP);
		UBorderComponent* Border = ConstructObject<UBorderComponent>(UBorderComponent::StaticClass(), BP);
		UVerticalBoxComponent* Vertical = ConstructObject<UVerticalBoxComponent>(UVerticalBoxComponent::StaticClass(), BP);
		UButtonComponent* Button1 = ConstructObject<UButtonComponent>(UButtonComponent::StaticClass(), BP);
		Button1->ButtonText = FText::FromString("Button 1");
		UButtonComponent* Button2 = ConstructObject<UButtonComponent>(UButtonComponent::StaticClass(), BP);
		Button2->ButtonText = FText::FromString("Button 2");
		UButtonComponent* Button3 = ConstructObject<UButtonComponent>(UButtonComponent::StaticClass(), BP);
		Button3->ButtonText = FText::FromString("Button 3");
		UButtonComponent* Button4 = ConstructObject<UButtonComponent>(UButtonComponent::StaticClass(), BP);

		UTextBlockComponent* Text1 = ConstructObject<UTextBlockComponent>(UTextBlockComponent::StaticClass(), BP);
		Text1->Text = FText::FromString(TEXT("Button!"));
		Button4->SetContent(Text1);

		BP->WidgetTemplates.Add(Canvas);
		BP->WidgetTemplates.Add(Border);
		BP->WidgetTemplates.Add(Vertical);
		BP->WidgetTemplates.Add(Button1);
		BP->WidgetTemplates.Add(Button2);
		BP->WidgetTemplates.Add(Button3);
		BP->WidgetTemplates.Add(Button4);
		BP->WidgetTemplates.Add(Text1);

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

void SUMGEditorTree::RefreshTree()
{
	RootWidgets.Reset();

	UWidgetBlueprint* Blueprint = GetBlueprint();
	if ( Blueprint->WidgetTemplates.Num() > 0 )
	{
		RootWidgets.Add(Blueprint->WidgetTemplates[0]);
	}

	WidgetTreeView->RequestTreeRefresh();
}
