// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "WorldBrowserPrivatePCH.h"

#include "SWorldTreeItem.h"

#define LOCTEXT_NAMESPACE "WorldBrowser"

//////////////////////////
// SWorldTreeItem
//////////////////////////

void SWorldTreeItem::Construct(const FArguments& InArgs, TSharedRef<STableViewBase> OwnerTableView)
{
	WorldModel = InArgs._InWorldModel;
	LevelModel = InArgs._InItemModel;
	IsItemExpanded = InArgs._IsItemExpanded;
	HighlightText = InArgs._HighlightText;

	MapPackageLoaded = FEditorStyle::GetBrush("WorldBrowser.LevelLoaded");
	MapPackageUnloaded = FEditorStyle::GetBrush("WorldBrowser.LevelUnloaded");
	MapPackagePending = FEditorStyle::GetBrush("WorldBrowser.LevelPending");

	SMultiColumnTableRow<TSharedPtr<FLevelModel>>::Construct(
		FSuperRowType::FArguments()
			.OnDragDetected(this, &SWorldTreeItem::OnItemDragDetected), 
		OwnerTableView
	);
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
TSharedRef< SWidget > SWorldTreeItem::GenerateWidgetForColumn( const FName & ColumnID )
{
	TSharedPtr< SWidget > TableRowContent = SNullWidget::NullWidget;

	if (ColumnID == HierarchyColumns::ColumnID_LevelLabel)
	{
		TableRowContent =
			SNew(SHorizontalBox)
			
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SExpanderArrow, SharedThis(this))
			]
			
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Font(this, &SWorldTreeItem::GetLevelDisplayNameFont)
				.Text(this, &SWorldTreeItem::GetLevelDisplayNameText)
				.ColorAndOpacity(this, &SWorldTreeItem::GetLevelDisplayNameColorAndOpacity)
				.HighlightText(HighlightText)
				.ToolTipText(LOCTEXT("DoubleClickToolTip", "Double-Click to make this the current Level"))
			]
		;
	}
	else if (ColumnID == HierarchyColumns::ColumnID_Lock)
	{
		TableRowContent = 
			SAssignNew(LockButton, SButton)
			.ContentPadding(0 )
			.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
			.IsEnabled(this, &SWorldTreeItem::IsLockEnabled)
			.OnClicked(this, &SWorldTreeItem::OnToggleLock)
			.ToolTipText(this, &SWorldTreeItem::GetLevelLockToolTip)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Content()
			[
				SNew(SImage)
				.Image(this, &SWorldTreeItem::GetLevelLockBrush)
			]
		;
	}
	else if (ColumnID == HierarchyColumns::ColumnID_Visibility)
	{
		TableRowContent = 
			SAssignNew(VisibilityButton, SButton)
			.ContentPadding(0)
			.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
			.IsEnabled(this, &SWorldTreeItem::IsVisibilityEnabled)
			.OnClicked(this, &SWorldTreeItem::OnToggleVisibility)
			.ToolTipText(LOCTEXT("VisibilityButtonToolTip", "Toggle Level Visibility"))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Content()
			[
				SNew( SImage )
				.Image(this, &SWorldTreeItem::GetLevelVisibilityBrush)
			]
		;
	}
	else if (ColumnID == HierarchyColumns::ColumnID_Kismet)
	{
		TableRowContent = 
			SAssignNew(KismetButton, SButton)
			.ContentPadding(0)
			.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
			.IsEnabled(this, &SWorldTreeItem::IsKismetEnabled)
			.OnClicked(this, &SWorldTreeItem::OnOpenKismet)
			.ToolTipText(LOCTEXT("KismetButtonToolTip", "Open Level Blueprint"))
			.HAlign( HAlign_Center )
			.VAlign( VAlign_Center )
			.Content()
			[
				SNew(SImage)
				.Image(this, &SWorldTreeItem::GetLevelKismetBrush)
			]
		;
	}
	else if (ColumnID == HierarchyColumns::ColumnID_SCCStatus)
	{
		TableRowContent = 
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew( SImage )
						.Image(this, &SWorldTreeItem::GetSCCStateImage)
						.ToolTipText(this, &SWorldTreeItem::GetSCCStateTooltip)
				]
			]
		;
	}
	else if (ColumnID == HierarchyColumns::ColumnID_Save)
	{
		TableRowContent = 
			SAssignNew(SaveButton, SButton)
			.ContentPadding(0)
			.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
			.IsEnabled(this, &SWorldTreeItem::IsSaveEnabled)
			.OnClicked(this, &SWorldTreeItem::OnSave)
			.ToolTipText(LOCTEXT("SaveButtonToolTip", "Save Level"))
			.HAlign( HAlign_Center )
			.VAlign( VAlign_Center )
			.Content()
			[
				SNew(SImage)
				.Image(this, &SWorldTreeItem::GetLevelSaveBrush)
			]
		;
	}

	return TableRowContent.ToSharedRef();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FString SWorldTreeItem::GetLevelDisplayNameText() const
{
	return LevelModel->GetDisplayName();
}

bool SWorldTreeItem::IsSaveEnabled() const
{
	return LevelModel->IsLoaded();
}

bool SWorldTreeItem::IsLockEnabled() const
{
	return LevelModel->IsLoaded();
}

bool SWorldTreeItem::IsVisibilityEnabled() const
{
	return LevelModel->IsLoaded();
}

bool SWorldTreeItem::IsKismetEnabled() const
{
	return LevelModel->HasKismet();
}

FReply SWorldTreeItem::OnToggleVisibility()
{
	FLevelModelList LevelList; 
	LevelList.Add(LevelModel);

	if (LevelModel->IsVisible())
	{
		WorldModel->HideLevels(LevelList);
	}
	else
	{
		WorldModel->ShowLevels(LevelList);
	}

	return FReply::Handled();
}

FReply SWorldTreeItem::OnToggleLock()
{
	FLevelModelList LevelList; 
	LevelList.Add(LevelModel);
	
	if (LevelModel->IsLocked())
	{
		WorldModel->UnlockLevels(LevelList);
	}
	else
	{
		WorldModel->LockLevels(LevelList);
	}
	
	return FReply::Handled();
}

FReply SWorldTreeItem::OnSave()
{
	FLevelModelList LevelList; 
	LevelList.Add(LevelModel);


	WorldModel->SaveLevels(LevelList);
	return FReply::Handled();
}

FReply SWorldTreeItem::OnOpenKismet()
{
	LevelModel->OpenKismet();
	return FReply::Handled();
}

const FSlateBrush* SWorldTreeItem::GetLevelImage() const
{
	if (LevelModel->IsLoaded())
	{
		return MapPackageLoaded;
	}

	return LevelModel->IsLoading() ? MapPackagePending : MapPackageUnloaded;
}

FReply SWorldTreeItem::OnItemDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		TSharedPtr<FLevelDragDropOp> Op = WorldModel->CreateDragDropOp();
		if (Op.IsValid())
		{
			// Start a drag-drop
			return FReply::Handled().BeginDragDrop(Op.ToSharedRef());
		}
	}

	return FReply::Unhandled();
}

FReply SWorldTreeItem::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	auto Op = DragDropEvent.GetOperationAs<FLevelDragDropOp>();
	if (Op.IsValid() && LevelModel->IsGoodToDrop(Op))
	{
		LevelModel->OnDrop(Op);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SWorldTreeItem::OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	SCompoundWidget::OnDragEnter(MyGeometry, DragDropEvent);
	
	auto Op = DragDropEvent.GetOperationAs<FLevelDragDropOp>();
	if (Op.IsValid())
	{
		// to show mouse hover effect
		SWidget::OnMouseEnter(MyGeometry, DragDropEvent);
		// D&D decorator icon
		Op->bGoodToDrop = LevelModel->IsGoodToDrop(Op);
	}
}

void SWorldTreeItem::OnDragLeave( const FDragDropEvent& DragDropEvent )
{
	SCompoundWidget::OnDragLeave(DragDropEvent);

	auto Op = DragDropEvent.GetOperationAs<FLevelDragDropOp>();
	if (Op.IsValid())
	{
		// to hide mouse hover effect
		SWidget::OnMouseLeave(DragDropEvent);
		// D&D decorator icon
		Op->bGoodToDrop = false;
	}
}

FSlateFontInfo SWorldTreeItem::GetLevelDisplayNameFont() const
{
	if (LevelModel->IsCurrent())
	{
		return FEditorStyle::GetFontStyle("LevelBrowser.LabelFontBold");
	}
	else
	{
		return FEditorStyle::GetFontStyle("LevelBrowser.LabelFont");
	}
}

FSlateColor SWorldTreeItem::GetLevelDisplayNameColorAndOpacity() const
{
	// Force the text to display red if level is missing
	if (!LevelModel->HasValidPackage())
	{
		return FLinearColor(1.0f, 0.0f, 0.0f);
	}
		
	// Highlight text differently if it doesn't match the search filter (e.g., parent levels to child levels that
	// match search criteria.)
	if (LevelModel->GetLevelFilteredOutFlag())
	{
		return FLinearColor(0.30f, 0.30f, 0.30f);
	}

	if (!LevelModel->IsLoaded())
	{
		return FSlateColor::UseSubduedForeground();
	}
		
	if (LevelModel->IsCurrent())
	{
		return LevelModel->GetLevelSelectionFlag() ? FSlateColor::UseForeground() : FLinearColor(0.12f, 0.56f, 1.0f);
	}

	if (LevelModel->IsAlwaysLoaded())
	{
		return LevelModel->GetLevelSelectionFlag() ? FSlateColor::UseForeground() : FLinearColor(1.0f, 0.56f, 0.12f);
	}
	
	return FSlateColor::UseForeground();
}

const FSlateBrush* SWorldTreeItem::GetLevelVisibilityBrush() const
{
	if (LevelModel->GetLevelObject())
	{
		if (LevelModel->IsVisible())
		{
			return VisibilityButton->IsHovered() ? FEditorStyle::GetBrush( "Level.VisibleHighlightIcon16x" ) :
													FEditorStyle::GetBrush( "Level.VisibleIcon16x" );
		}
		else
		{
			return VisibilityButton->IsHovered() ? FEditorStyle::GetBrush( "Level.NotVisibleHighlightIcon16x" ) :
													FEditorStyle::GetBrush( "Level.NotVisibleIcon16x" );
		}
	}
	else
	{
		return FEditorStyle::GetBrush( "Level.EmptyIcon16x" );
	}
}

const FSlateBrush* SWorldTreeItem::GetLevelLockBrush() const
{
	if (!LevelModel->IsLoaded() || LevelModel->IsPersistent())
	{
		//Locking the persistent level is not allowed; stub in a different brush
		return FEditorStyle::GetBrush( "Level.EmptyIcon16x" );
	}
	else 
	{
		////Non-Persistent
		//if ( GEngine && GEngine->bLockReadOnlyLevels )
		//{
		//	if(LevelModel->IsReadOnly())
		//	{
		//		return LockButton->IsHovered() ? FEditorStyle::GetBrush( "Level.ReadOnlyLockedHighlightIcon16x" ) :
		//											FEditorStyle::GetBrush( "Level.ReadOnlyLockedIcon16x" );
		//	}
		//}
			
		if (LevelModel->IsLocked())
		{
			return LockButton->IsHovered() ? FEditorStyle::GetBrush( "Level.LockedHighlightIcon16x" ) :
												FEditorStyle::GetBrush( "Level.LockedIcon16x" );
		}
		else
		{
			return LockButton->IsHovered() ? FEditorStyle::GetBrush( "Level.UnlockedHighlightIcon16x" ) :
												FEditorStyle::GetBrush( "Level.UnlockedIcon16x" );
		}
	}
}

FString SWorldTreeItem::GetLevelLockToolTip() const
{
	//Non-Persistent
	if (GEngine && GEngine->bLockReadOnlyLevels)
	{
		if (LevelModel->IsFileReadOnly())
		{
			return LOCTEXT("ReadOnly_LockButtonToolTip", "Read-Only levels are locked!").ToString();
		}
	}

	return LOCTEXT("LockButtonToolTip", "Toggle Level Lock").ToString();
}

FString SWorldTreeItem::GetSCCStateTooltip() const
{
	FSourceControlStatePtr SourceControlState = ISourceControlModule::Get().GetProvider().GetState(LevelModel->GetPackageFileName(), EStateCacheUsage::Use);
	if(SourceControlState.IsValid())
	{
		return SourceControlState->GetDisplayTooltip().ToString();
	}

	return FString();
}

const FSlateBrush* SWorldTreeItem::GetSCCStateImage() const
{
	FSourceControlStatePtr SourceControlState = ISourceControlModule::Get().GetProvider().GetState(LevelModel->GetPackageFileName(), EStateCacheUsage::Use);
	if(SourceControlState.IsValid())
	{
		return FEditorStyle::GetBrush(SourceControlState->GetSmallIconName());
	}

	return NULL;
}

const FSlateBrush* SWorldTreeItem::GetLevelSaveBrush() const
{
	if (LevelModel->IsLoaded())
	{
		if (LevelModel->IsLocked())
		{
			return FEditorStyle::GetBrush( "Level.SaveDisabledIcon16x" );
		}
		else
		{
			if (LevelModel->IsDirty())
			{
				return SaveButton->IsHovered() ? FEditorStyle::GetBrush( "Level.SaveModifiedHighlightIcon16x" ) :
													FEditorStyle::GetBrush( "Level.SaveModifiedIcon16x" );
			}
			else
			{
				return SaveButton->IsHovered() ? FEditorStyle::GetBrush( "Level.SaveHighlightIcon16x" ) :
													FEditorStyle::GetBrush( "Level.SaveIcon16x" );
			}
		}								
	}
	else
	{
		return FEditorStyle::GetBrush( "Level.EmptyIcon16x" );
	}	
}

const FSlateBrush* SWorldTreeItem::GetLevelKismetBrush() const
{
	if (LevelModel->IsLoaded())
	{
		if (LevelModel->HasKismet())
		{
			return KismetButton->IsHovered() ? FEditorStyle::GetBrush( "Level.ScriptHighlightIcon16x" ) :
												FEditorStyle::GetBrush( "Level.ScriptIcon16x" );
		}
		else
		{
			return FEditorStyle::GetBrush( "Level.EmptyIcon16x" );
		}
	}
	else
	{
		return FEditorStyle::GetBrush( "Level.EmptyIcon16x" );
	}	
}

#undef LOCTEXT_NAMESPACE
