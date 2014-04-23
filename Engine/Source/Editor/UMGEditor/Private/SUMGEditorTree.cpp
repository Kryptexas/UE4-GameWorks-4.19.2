// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SUMGEditorTree.h"
#include "UMGEditor.h"
#include "UMGEditorViewportClient.h"
#include "UMGEditorActions.h"

#include "PreviewScene.h"
#include "SceneViewport.h"

#include "BlueprintEditor.h"

void SUMGEditorTree::Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintEditor, USimpleConstructionScript* InSCS)
{
	BlueprintEditor = InBlueprintEditor;

	UBlueprint* BP = InBlueprintEditor->GetBlueprintObj();
	//BP->ComponentTemplates

	//FCoreDelegates::OnObjectPropertyChanged.Add( FCoreDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(this, &SUMGEditorTree::OnObjectPropertyChanged) );

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
			SAssignNew(PageTreeView, STreeView<TSharedPtr<FString>>)
			.ItemHeight(20.0f)
			//.OnGenerateRow(this, &SDeviceProcesses::HandleProcessTreeViewGenerateRow)
			//.OnGetChildren(this, &SDeviceProcesses::HandleProcessTreeViewGetChildren)
			//.SelectionMode(ESelectionMode::Multi)
			//.TreeItemsSource(&ProcessList)
		]
	];
}

SUMGEditorTree::~SUMGEditorTree()
{
	//FCoreDelegates::OnObjectPropertyChanged.Remove( FCoreDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(this, &SUMGEditorTree::OnObjectPropertyChanged) );
}

FReply SUMGEditorTree::CreateTestUI()
{
	//TSharedRef<ISCSEditor> SCSEditor = BlueprintEditor.Pin()->GetSCSEditorModel();
	//UCanvasPanelComponent* Canvas = Cast<UCanvasPanelComponent>(SCSEditor->AddNewComponent(UCanvasPanelComponent::StaticClass(), NULL));
	//UButtonComponent* Button = Cast<UButtonComponent>(SCSEditor->AddNewComponent(UButtonComponent::StaticClass(), NULL));

	UWidgetBlueprint* BP = CastChecked<UWidgetBlueprint>(BlueprintEditor.Pin()->GetBlueprintObj());

	UCanvasPanelComponent* Canvas = ConstructObject<UCanvasPanelComponent>(UCanvasPanelComponent::StaticClass(), BP);
	UVerticalBoxComponent* Vertical = ConstructObject<UVerticalBoxComponent>(UVerticalBoxComponent::StaticClass(), BP);
	UButtonComponent* Button1 = ConstructObject<UButtonComponent>(UButtonComponent::StaticClass(), BP);
	Button1->ButtonText = FText::FromString("Button 1");
	UButtonComponent* Button2 = ConstructObject<UButtonComponent>(UButtonComponent::StaticClass(), BP);
	Button2->ButtonText = FText::FromString("Button 2");
	UButtonComponent* Button3 = ConstructObject<UButtonComponent>(UButtonComponent::StaticClass(), BP);
	Button3->ButtonText = FText::FromString("Button 3");

	BP->WidgetTemplates.Add(Canvas);
	BP->WidgetTemplates.Add(Vertical);
	BP->WidgetTemplates.Add(Button1);
	BP->WidgetTemplates.Add(Button2);
	BP->WidgetTemplates.Add(Button3);

	UCanvasPanelSlot* Slot = Canvas->AddSlot(Vertical);
	Slot->Size.X = 100;
	Slot->Size.Y = 100;
	Slot->Position.X = 20;
	Slot->Position.Y = 50;

	Vertical->AddSlot(Button1);
	Vertical->AddSlot(Button2);
	Vertical->AddSlot(Button3);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);

	return FReply::Handled();
}

//void SUMGEditorTree::AddReferencedObjects( FReferenceCollector& Collector )
//{
//	Collector.AddReferencedObject(Page);
//}
//
//void SUMGEditorTree::OnObjectPropertyChanged(UObject* ObjectBeingModified)
//{
//	if ( !ensure(ObjectBeingModified) )
//	{
//		return;
//	}
//}
