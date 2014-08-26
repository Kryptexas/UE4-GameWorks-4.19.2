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

FReply ProcessHierarchyDragDrop(const FDragDropEvent& DragDropEvent, bool bIsDrop, UWidgetBlueprint* Blueprint, FWidgetReference TargetItem)
{
	// Is this a drag/drop op to create a new widget in the tree?
	TSharedPtr<FWidgetTemplateDragDropOp> TemplateDragDropOp = DragDropEvent.GetOperationAs<FWidgetTemplateDragDropOp>();
	if ( TemplateDragDropOp.IsValid() )
	{
		TemplateDragDropOp->SetCursorOverride(TOptional<EMouseCursor::Type>());

		// Are we adding to the root?
		if ( !TargetItem.IsValid() && Blueprint->WidgetTree->RootWidget == NULL )
		{
			// TODO UMG Allow showing a preview of this.
			if ( bIsDrop )
			{
				Blueprint->WidgetTree->RootWidget = TemplateDragDropOp->Template->Create(Blueprint->WidgetTree);
				FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
			}
		}
		// Are we adding to a panel?
		else if ( UPanelWidget* Parent = Cast<UPanelWidget>(TargetItem.GetTemplate()) )
		{
			// TODO UMG Allow showing a preview of this.
			if ( bIsDrop )
			{
				UWidget* Widget = TemplateDragDropOp->Template->Create(Blueprint->WidgetTree);
				
				Parent->AddChild(Widget);

				FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
			}
		}
		//TODO Are we adding to a named slot?
		else
		{
			TemplateDragDropOp->SetCursorOverride(EMouseCursor::SlashedCircle);
		}

		return FReply::Handled();
	}

	TSharedPtr<FHierarchyWidgetDragDropOp> HierarchyDragDropOp = DragDropEvent.GetOperationAs<FHierarchyWidgetDragDropOp>();
	if ( HierarchyDragDropOp.IsValid() )
	{
		HierarchyDragDropOp->SetCursorOverride(TOptional<EMouseCursor::Type>());

		// If the target item is valid we're dealing with a normal widget in the hierarchy, otherwise we should assume it's
		// the null case and we should be adding it as the root widget.
		if ( TargetItem.IsValid() )
		{
			const bool bIsPanel = TargetItem.GetTemplate()->IsA(UPanelWidget::StaticClass());
			const bool bIsDraggedObject = TargetItem.GetTemplate() == HierarchyDragDropOp->Widget.GetTemplate();
			const bool bIsChildOfDraggedObject = TargetItem.GetTemplate()->IsChildOf(HierarchyDragDropOp->Widget.GetTemplate());

			if ( bIsPanel && !bIsDraggedObject && !bIsChildOfDraggedObject )
			{
				UWidget* Widget = bIsDrop ? HierarchyDragDropOp->Widget.GetTemplate() : HierarchyDragDropOp->Widget.GetPreview();

				if ( Widget->GetParent() )
				{
					Widget->GetParent()->RemoveChild(Widget);
				}

				UPanelWidget* NewParent = Cast<UPanelWidget>(bIsDrop ? TargetItem.GetTemplate() : TargetItem.GetPreview());

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
						TSharedPtr<SWidget> Widget = TargetItem.GetPreview()->GetCachedWidget();
						if ( Widget.IsValid() )
						{
							Widget->SlatePrepass();
						}
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

//////////////////////////////////////////////////////////////////////////

FHierarchyModel::FHierarchyModel()
	: bInitialized(false)
{

}

FReply FHierarchyModel::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	return FReply::Unhandled();
}

FReply FHierarchyModel::HandleDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Unhandled();
}

void FHierarchyModel::HandleDragEnter(const FDragDropEvent& DragDropEvent)
{

}

void FHierarchyModel::HandleDragLeave(const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDecoratedDragDropOp> DecoratedDragDropOp = DragDropEvent.GetOperationAs<FDecoratedDragDropOp>();
	if ( DecoratedDragDropOp.IsValid() )
	{
		DecoratedDragDropOp->SetCursorOverride(TOptional<EMouseCursor::Type>());
		DecoratedDragDropOp->ResetToDefaultToolTip();
	}
}

FReply FHierarchyModel::HandleDrop(FDragDropEvent const& DragDropEvent)
{
	return FReply::Unhandled();
}

bool FHierarchyModel::OnVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage)
{
	return false;
}

void FHierarchyModel::OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo)
{

}

void FHierarchyModel::GatherChildren(TArray< TSharedPtr<FHierarchyModel> >& Children)
{
	if ( !bInitialized )
	{
		bInitialized = true;
		GetChildren(Models);
	}

	Children.Append(Models);
}

//////////////////////////////////////////////////////////////////////////

FHierarchyRoot::FHierarchyRoot(TSharedPtr<FWidgetBlueprintEditor> InBlueprintEditor)
	: BlueprintEditor(InBlueprintEditor)
{
}

FText FHierarchyRoot::GetText() const
{
	return LOCTEXT("Root", "[Root]");
}

const FSlateBrush* FHierarchyRoot::GetImage() const
{
	return NULL;
}

FSlateFontInfo FHierarchyRoot::GetFont() const
{
	return FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 10);
}

void FHierarchyRoot::GetChildren(TArray< TSharedPtr<FHierarchyModel> >& Children)
{
	TSharedPtr<FWidgetBlueprintEditor> BPEd = BlueprintEditor.Pin();
	UWidgetBlueprint* Blueprint = BPEd->GetWidgetBlueprintObj();

	if ( Blueprint->WidgetTree->RootWidget )
	{
		TSharedPtr<FHierarchyWidget> RootChild = MakeShareable(new FHierarchyWidget(BPEd->GetReferenceFromTemplate(Blueprint->WidgetTree->RootWidget), BPEd));
		Children.Add(RootChild);
	}
}

void FHierarchyRoot::OnSelection()
{
	TSharedPtr<FWidgetBlueprintEditor> BPEd = BlueprintEditor.Pin();
	if ( UWidget* Default =BPEd->GetWidgetBlueprintObj()->GeneratedClass->GetDefaultObject<UWidget>() )
	{
		TSet<UObject*> SelectedObjects;
		SelectedObjects.Add(Default);
		BPEd->SelectObjects(SelectedObjects);
	}
}

FReply FHierarchyRoot::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	UWidgetBlueprint* Blueprint = BlueprintEditor.Pin()->GetWidgetBlueprintObj();
	bool bIsDrop = false;
	return ProcessHierarchyDragDrop(DragDropEvent, bIsDrop, Blueprint, FWidgetReference());
}

FReply FHierarchyRoot::HandleDrop(FDragDropEvent const& DragDropEvent)
{
	UWidgetBlueprint* Blueprint = BlueprintEditor.Pin()->GetWidgetBlueprintObj();
	bool bIsDrop = true;
	return ProcessHierarchyDragDrop(DragDropEvent, bIsDrop, Blueprint, FWidgetReference());
}

//////////////////////////////////////////////////////////////////////////

FHierarchyWidget::FHierarchyWidget(FWidgetReference InItem, TSharedPtr<FWidgetBlueprintEditor> InBlueprintEditor)
	: Item(InItem)
	, BlueprintEditor(InBlueprintEditor)
{
}

FText FHierarchyWidget::GetText() const
{
	UWidget* WidgetTemplate = Item.GetTemplate();
	if ( WidgetTemplate )
	{
		return FText::FromString(WidgetTemplate->GetLabel());
	}

	return FText::GetEmpty();
}

const FSlateBrush* FHierarchyWidget::GetImage() const
{
	return Item.GetTemplate()->GetEditorIcon();
}

FSlateFontInfo FHierarchyWidget::GetFont() const
{
	UWidget* WidgetTemplate = Item.GetTemplate();
	if ( WidgetTemplate )
	{
		if ( !WidgetTemplate->IsGeneratedName() && WidgetTemplate->bIsVariable )
		{
			// TODO UMG Hacky move into style area
			return FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 10);
		}
	}

	static FName NormalFont("NormalFont");
	return FCoreStyle::Get().GetFontStyle(NormalFont);
}

FReply FHierarchyWidget::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
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
		UWidgetBlueprint* Blueprint = BlueprintEditor.Pin()->GetWidgetBlueprintObj();
		return ProcessHierarchyDragDrop(DragDropEvent, bIsDrop, Blueprint, Item);
	}

	return FReply::Unhandled();
}

FReply FHierarchyWidget::HandleDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const bool bIsRoot = Item.GetTemplate()->GetParent() == NULL;

	if ( !bIsRoot )
	{
		return FReply::Handled().BeginDragDrop(FHierarchyWidgetDragDropOp::New(BlueprintEditor.Pin()->GetWidgetBlueprintObj(), Item));
	}

	return FReply::Unhandled();
}

void FHierarchyWidget::HandleDragLeave(const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDecoratedDragDropOp> DecoratedDragDropOp = DragDropEvent.GetOperationAs<FDecoratedDragDropOp>();
	if ( DecoratedDragDropOp.IsValid() )
	{
		DecoratedDragDropOp->SetCursorOverride(TOptional<EMouseCursor::Type>());
		DecoratedDragDropOp->ResetToDefaultToolTip();
	}
}

FReply FHierarchyWidget::HandleDrop(const FDragDropEvent& DragDropEvent)
{
	UWidgetBlueprint* Blueprint = BlueprintEditor.Pin()->GetWidgetBlueprintObj();
	bool bIsDrop = true;
	return ProcessHierarchyDragDrop(DragDropEvent, bIsDrop, Blueprint, Item);
}

bool FHierarchyWidget::OnVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage)
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

void FHierarchyWidget::OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo)
{
	FWidgetBlueprintEditorUtils::RenameWidget(BlueprintEditor.Pin().ToSharedRef(), Item.GetTemplate()->GetFName(), FName(*InText.ToString()));
}

void FHierarchyWidget::GetChildren(TArray< TSharedPtr<FHierarchyModel> >& Children)
{
	TSharedPtr<FWidgetBlueprintEditor> BPEd = BlueprintEditor.Pin();
	
	UPanelWidget* Widget = Cast<UPanelWidget>(Item.GetTemplate());
	if ( Widget )
	{
		for ( int32 i = 0; i < Widget->GetChildrenCount(); i++ )
		{
			UWidget* Child = Widget->GetChildAt(i);
			if ( Child )
			{
				TSharedPtr<FHierarchyWidget> ChildItem = MakeShareable(new FHierarchyWidget(BPEd->GetReferenceFromTemplate(Child), BPEd));
				Children.Add(ChildItem);
			}
		}
	}
}

void FHierarchyWidget::OnSelection()
{
	TSet<FWidgetReference> SelectedWidgets;
	SelectedWidgets.Add(Item);

	BlueprintEditor.Pin()->SelectWidgets(SelectedWidgets);
}

//////////////////////////////////////////////////////////////////////////

void SHierarchyViewItem::Construct(const FArguments& InArgs, const TSharedRef< STableViewBase >& InOwnerTableView, TSharedPtr<FHierarchyModel> InModel)
{
	Model = InModel;

	STableRow< TSharedPtr<FHierarchyModel> >::Construct(
		STableRow< TSharedPtr<FHierarchyModel> >::FArguments()
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
				.Image(Model->GetImage())
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
	return Model->OnVerifyNameTextChanged(InText, OutErrorMessage);
}

void SHierarchyViewItem::OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo)
{
	return Model->OnNameTextCommited(InText, CommitInfo);
}

FSlateFontInfo SHierarchyViewItem::GetItemFont() const
{
	return Model->GetFont();
}

FText SHierarchyViewItem::GetItemText() const
{
	return Model->GetText();
}

FReply SHierarchyViewItem::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	return Model->OnDragOver(MyGeometry, DragDropEvent);
}

FReply SHierarchyViewItem::HandleDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return Model->HandleDragDetected(MyGeometry, MouseEvent);
}

void SHierarchyViewItem::HandleDragEnter(FDragDropEvent const& DragDropEvent)
{
	Model->HandleDragEnter(DragDropEvent);
}

void SHierarchyViewItem::HandleDragLeave(const FDragDropEvent& DragDropEvent)
{
	Model->HandleDragLeave(DragDropEvent);
}

FReply SHierarchyViewItem::HandleDrop(const FDragDropEvent& DragDropEvent)
{
	return Model->HandleDrop(DragDropEvent);
}

#undef LOCTEXT_NAMESPACE
