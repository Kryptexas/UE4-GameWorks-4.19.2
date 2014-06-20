// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SUMGEditorTreeItem.h"
#include "UMGEditorActions.h"
#include "WidgetTemplateDragDropOp.h"

#include "PreviewScene.h"
#include "SceneViewport.h"

#include "BlueprintEditor.h"
#include "SKismetInspector.h"
#include "BlueprintEditorUtils.h"

#include "Kismet2NameValidators.h"

#include "WidgetBlueprintEditor.h"

#define LOCTEXT_NAMESPACE "UMG"

void SUMGEditorTreeItem::Construct(const FArguments& InArgs, const TSharedRef< STableViewBase >& InOwnerTableView, TSharedPtr<FWidgetBlueprintEditor> InBlueprintEditor, UWidget* InItem)
{
	BlueprintEditor = InBlueprintEditor;
	Item = InItem;

	STableRow< UWidget* >::Construct(
		STableRow< UWidget* >::FArguments()
		.Padding(2.0f)
		.Content()
		[
			SNew(SHorizontalBox)

			// Widget icon
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SImage)
				.ColorAndOpacity(FLinearColor(1,1,1,0.5))
				.Image(Item->GetEditorIcon())
			]

			// Name of the widget
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(2, 0, 0, 0)
			.VAlign(VAlign_Center)
			[
				SNew(SInlineEditableTextBlock)
				.Font(this, &SUMGEditorTreeItem::GetItemFont)
				.Text(this, &SUMGEditorTreeItem::GetItemText)
				.HighlightText(InArgs._HighlightText)
				.OnVerifyTextChanged(this, &SUMGEditorTreeItem::OnVerifyNameTextChanged)
				.OnTextCommitted(this, &SUMGEditorTreeItem::OnNameTextCommited)
				.IsSelected(this, &SUMGEditorTreeItem::IsSelectedExclusively)
			]
		],
		InOwnerTableView);
}

bool SUMGEditorTreeItem::OnVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage)
{
	FString NewName = InText.ToString();

	UWidgetBlueprint* Blueprint = BlueprintEditor.Pin()->GetWidgetBlueprintObj();
	UWidget* ExistingWidget = Blueprint->WidgetTree->FindWidget(NewName);

	FKismetNameValidator Validator(Blueprint);

	const bool bUniqueNameForVariable = ( EValidatorResult::Ok == Validator.IsValid(NewName) );
	
	if ( ( ExistingWidget != NULL && ExistingWidget != Item ) || !bUniqueNameForVariable )
	{
		OutErrorMessage = LOCTEXT("NameConflict", "NameConflict");
		return false;
	}

	return true;
}

void SUMGEditorTreeItem::OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo)
{
	FWidgetBlueprintEditorUtils::RenameWidget(BlueprintEditor.Pin().ToSharedRef(), Item->GetFName(), FName(*InText.ToString()));
}

FSlateFontInfo SUMGEditorTreeItem::GetItemFont() const
{
	if ( !Item->IsGeneratedName() && Item->bIsVariable )
	{
		// TODO UMG Hacky move into style area
		return FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 10);
	}
	else
	{
		// TODO UMG Hacky move into style area
		return FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Normal.ttf"), 9);
	}
}

FText SUMGEditorTreeItem::GetItemText() const
{
	return FText::FromString(Item->GetLabel());
}

//@TODO UMG Allow items in the tree to be dragged, and reordered, and reparented.

void SUMGEditorTreeItem::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	//TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();
	//if ( !Operation.IsValid() )
	//{
	//	return;
	//}

	//if ( Operation->IsOfType<FWidgetTemplateDragDropOp>() )
	//{
	//	TSharedPtr<FWidgetTemplateDragDropOp> DragDropOp = StaticCastSharedPtr<FWidgetTemplateDragDropOp>(Operation);
	//	bDraggedOver = true;
	//}
}

void SUMGEditorTreeItem::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FWidgetTemplateDragDropOp> DragDropOp = DragDropEvent.GetOperationAs<FWidgetTemplateDragDropOp>();
	if ( DragDropOp.IsValid() )
	{
		DragDropOp->ResetToDefaultToolTip();
	}
}

FReply SUMGEditorTreeItem::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FWidgetTemplateDragDropOp> DragDropOp = DragDropEvent.GetOperationAs<FWidgetTemplateDragDropOp>();
	if ( DragDropOp.IsValid() )
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SUMGEditorTreeItem::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FWidgetTemplateDragDropOp> DragDropOp = DragDropEvent.GetOperationAs<FWidgetTemplateDragDropOp>();
	if ( DragDropOp.IsValid() )
	{
		if ( Item->IsA(UPanelWidget::StaticClass()) )
		{
			UWidgetBlueprint* BP = CastChecked<UWidgetBlueprint>(BlueprintEditor.Pin()->GetBlueprintObj());			
			UPanelWidget* Parent = Cast<UPanelWidget>(Item);

			UWidget* Widget = DragDropOp->Template->Create(BP->WidgetTree);

			Parent->AddChild(Widget, FVector2D(0,0));

			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		}

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

#undef LOCTEXT_NAMESPACE
