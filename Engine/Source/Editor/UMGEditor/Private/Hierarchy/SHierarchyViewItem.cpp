// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "SHierarchyViewItem.h"

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

/**
 * 
 */
class FHierarchyWidgetDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FHierarchyWidgetDragDropOp, FDecoratedDragDropOp)

	virtual ~FHierarchyWidgetDragDropOp();

	/** The slot properties for the old slot the widget was in, is used to attempt to reapply the same layout information */
	TMap<FName, FString> ExportedSlotProperties;

	/** The widget being dragged and dropped */
	FWidgetReference Widget;

	/** The original parent of the widget. */
	UWidget* WidgetParent;

	/** The widget being dragged and dropped */
	FScopedTransaction* Transaction;

	/** Constructs a new drag/drop operation */
	static TSharedRef<FHierarchyWidgetDragDropOp> New(UWidgetBlueprint* Blueprint, FWidgetReference InWidget);
};

TSharedRef<FHierarchyWidgetDragDropOp> FHierarchyWidgetDragDropOp::New(UWidgetBlueprint* Blueprint, FWidgetReference InWidget)
{
	TSharedRef<FHierarchyWidgetDragDropOp> Operation = MakeShareable(new FHierarchyWidgetDragDropOp());
	Operation->Widget = InWidget;
	Operation->DefaultHoverText = FText::FromString(InWidget.GetTemplate()->GetLabel());
	Operation->CurrentHoverText = FText::FromString(InWidget.GetTemplate()->GetLabel());
	Operation->Construct();

	FWidgetBlueprintEditorUtils::ExportPropertiesToText(InWidget.GetTemplate()->Slot, Operation->ExportedSlotProperties);

	Operation->Transaction = new FScopedTransaction(LOCTEXT("Designer_MoveWidget", "Move Widget"));

	Blueprint->WidgetTree->SetFlags(RF_Transactional);
	Blueprint->WidgetTree->Modify();

	UWidget* Widget = Operation->Widget.GetTemplate();
	Widget->Modify();

	Operation->WidgetParent = Widget->GetParent();

	if ( Operation->WidgetParent )
	{
		Operation->WidgetParent->Modify();
	}

	return Operation;
}

FHierarchyWidgetDragDropOp::~FHierarchyWidgetDragDropOp()
{
	delete Transaction;
}

//////////////////////////////////////////////////////////////////////////

void SHierarchyViewItem::Construct(const FArguments& InArgs, const TSharedRef< STableViewBase >& InOwnerTableView, TSharedPtr<FWidgetBlueprintEditor> InBlueprintEditor, FWidgetReference InItem)
{
	BlueprintEditor = InBlueprintEditor;
	Item = InItem;

	STableRow< UWidget* >::Construct(
		STableRow< UWidget* >::FArguments()
		.Padding(2.0f)
		.OnDragDetected(this, &SHierarchyViewItem::HandleDragDetected)
		.OnDragEnter(this, &SHierarchyViewItem::HandleDragEnter)
		.OnDragLeave(this, &SHierarchyViewItem::HandleDragLeave)
		.OnDrop(this, &SHierarchyViewItem::HandleDrop)
		.Content()
		[
			SNew(SHorizontalBox)

			// Widget icon
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SImage)
				.ColorAndOpacity(FLinearColor(1,1,1,0.5))
				.Image(Item.GetTemplate()->GetEditorIcon())
			]

			// Name of the widget
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(2, 0, 0, 0)
			.VAlign(VAlign_Center)
			[
				SNew(SInlineEditableTextBlock)
				.Font(this, &SHierarchyViewItem::GetItemFont)
				.Text(this, &SHierarchyViewItem::GetItemText)
				.HighlightText(InArgs._HighlightText)
				.OnVerifyTextChanged(this, &SHierarchyViewItem::OnVerifyNameTextChanged)
				.OnTextCommitted(this, &SHierarchyViewItem::OnNameTextCommited)
				.IsSelected(this, &SHierarchyViewItem::IsSelectedExclusively)
			]
		],
		InOwnerTableView);
}

bool SHierarchyViewItem::OnVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage)
{
	FString NewName = InText.ToString();

	UWidgetBlueprint* Blueprint = BlueprintEditor.Pin()->GetWidgetBlueprintObj();
	UWidget* ExistingTemplate = Blueprint->WidgetTree->FindWidget(NewName);

	bool bIsSameWidget = false;
	if ( ExistingTemplate != NULL )
	{
		if ( Item.GetTemplate() == ExistingTemplate )
		{
			OutErrorMessage = LOCTEXT("ExistingWidgetName", "Existing Widget Name");
			return false;
		}
		else
		{
			bIsSameWidget = true;
		}
	}

	FKismetNameValidator Validator(Blueprint);

	const bool bUniqueNameForVariable = ( EValidatorResult::Ok == Validator.IsValid(NewName) );

	if ( !bUniqueNameForVariable && !bIsSameWidget )
	{
		OutErrorMessage = LOCTEXT("ExistingVariableName", "Existing Variable Name");
		return false;
	}

	return true;
}

void SHierarchyViewItem::OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo)
{
	FWidgetBlueprintEditorUtils::RenameWidget(BlueprintEditor.Pin().ToSharedRef(), Item.GetTemplate()->GetFName(), FName(*InText.ToString()));
}

FSlateFontInfo SHierarchyViewItem::GetItemFont() const
{
	if ( !Item.GetTemplate()->IsGeneratedName() && Item.GetTemplate()->bIsVariable )
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

FText SHierarchyViewItem::GetItemText() const
{
	return FText::FromString(Item.GetTemplate()->GetLabel());
}

FReply SHierarchyViewItem::HandleDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const bool bIsRoot = Item.GetTemplate()->GetParent() == NULL;

	if ( !bIsRoot )
	{
		return FReply::Handled().BeginDragDrop(FHierarchyWidgetDragDropOp::New(BlueprintEditor.Pin()->GetWidgetBlueprintObj(), Item));
	}

	return FReply::Unhandled();
}

void SHierarchyViewItem::HandleDragEnter(FDragDropEvent const& DragDropEvent)
{
}

void SHierarchyViewItem::HandleDragLeave(const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDecoratedDragDropOp> DecoratedDragDropOp = DragDropEvent.GetOperationAs<FDecoratedDragDropOp>();
	if ( DecoratedDragDropOp.IsValid() )
	{
		DecoratedDragDropOp->SetCursorOverride(TOptional<EMouseCursor::Type>());
		DecoratedDragDropOp->ResetToDefaultToolTip();
	}
}

FReply SHierarchyViewItem::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FWidgetTemplateDragDropOp> TemplateDragDropOp = DragDropEvent.GetOperationAs<FWidgetTemplateDragDropOp>();
	if ( TemplateDragDropOp.IsValid() )
	{
		// TODO UMG - Show feedback
		return FReply::Handled();
	}

	TSharedPtr<FHierarchyWidgetDragDropOp> HierarchyDragDropOp = DragDropEvent.GetOperationAs<FHierarchyWidgetDragDropOp>();
	if ( HierarchyDragDropOp.IsValid() )
	{
		bool bIsDrop = false;
		return ProcessDragDrop(DragDropEvent, bIsDrop);
	}

	return FReply::Unhandled();
}

FReply SHierarchyViewItem::HandleDrop(const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FWidgetTemplateDragDropOp> TemplateDragDropOp = DragDropEvent.GetOperationAs<FWidgetTemplateDragDropOp>();
	if ( TemplateDragDropOp.IsValid() )
	{
		if ( Item.GetTemplate()->IsA(UPanelWidget::StaticClass()) )
		{
			UWidgetBlueprint* BP = CastChecked<UWidgetBlueprint>(BlueprintEditor.Pin()->GetBlueprintObj());			
			UPanelWidget* Parent = Cast<UPanelWidget>(Item.GetTemplate());

			UWidget* Widget = TemplateDragDropOp->Template->Create(BP->WidgetTree);

			Parent->AddChild(Widget);

			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		}

		return FReply::Handled();
	}

	bool bIsDrop = true;
	return ProcessDragDrop(DragDropEvent, bIsDrop);
}

FReply SHierarchyViewItem::ProcessDragDrop(const FDragDropEvent& DragDropEvent, bool bIsDrop)
{
	UWidgetBlueprint* Blueprint = BlueprintEditor.Pin()->GetWidgetBlueprintObj();

	TSharedPtr<FHierarchyWidgetDragDropOp> HierarchyDragDropOp = DragDropEvent.GetOperationAs<FHierarchyWidgetDragDropOp>();
	if ( HierarchyDragDropOp.IsValid() )
	{
		HierarchyDragDropOp->SetCursorOverride(TOptional<EMouseCursor::Type>());

		if ( Item.IsValid() )
		{
			const bool bIsPanel = Item.GetTemplate()->IsA(UPanelWidget::StaticClass());
			const bool bIsDraggedObject = Item.GetTemplate() == HierarchyDragDropOp->Widget.GetTemplate();
			const bool bIsChildOfDraggedObject = Item.GetTemplate()->IsChildOf(HierarchyDragDropOp->Widget.GetTemplate());

			if ( bIsPanel && !bIsDraggedObject && !bIsChildOfDraggedObject )
			{
				UWidget* Widget = bIsDrop ? HierarchyDragDropOp->Widget.GetTemplate() : HierarchyDragDropOp->Widget.GetPreview();

				if ( Widget->GetParent() )
				{
					Widget->GetParent()->RemoveChild(Widget);
				}

				UPanelWidget* NewParent = Cast<UPanelWidget>( bIsDrop ? Item.GetTemplate() : Item.GetPreview() );

				if ( bIsDrop && NewParent != HierarchyDragDropOp->WidgetParent )
				{
					NewParent->SetFlags(RF_Transactional);
					NewParent->Modify();
				}

				if ( UPanelSlot* Slot = NewParent->AddChild(Widget) )
				{
					FWidgetBlueprintEditorUtils::ImportPropertiesFromText(Slot, HierarchyDragDropOp->ExportedSlotProperties);

					Slot->SetDesiredPosition(FVector2D::ZeroVector);

					if ( bIsDrop )
					{
						FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
					}
					else
					{
						Item.GetPreview()->GetWidget()->SlatePrepass();
					}
				}
				else
				{
					HierarchyDragDropOp->SetCursorOverride(EMouseCursor::SlashedCircle);

					// TODO UMG ERROR Slot can not be created because maybe the max children has been reached.
					//          Maybe we can traverse the hierarchy and add it to the first parent that will accept it?
				}
			}
			else
			{
				HierarchyDragDropOp->SetCursorOverride(EMouseCursor::SlashedCircle);
			}
		}
		else
		{
			HierarchyDragDropOp->SetCursorOverride(EMouseCursor::SlashedCircle);
		}

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

#undef LOCTEXT_NAMESPACE
