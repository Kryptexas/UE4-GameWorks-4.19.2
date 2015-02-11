// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "UnrealEd.h"
#include "FoliageType.h"
#include "FoliageEdMode.h"
#include "SFoliageEditMeshRow.h"

#define LOCTEXT_NAMESPACE "FoliageEd_Mode"

void SFoliageEditMeshRow::Construct(const FArguments& InArgs, TSharedRef<STableViewBase> InOwnerTableView)
{
	MeshInfo = InArgs._MeshInfo;
	FoliageEditMode = InArgs._FoliageEditMode;
	SMultiColumnTableRow<FFoliageMeshUIInfoPtr>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
}

TSharedRef<SWidget> SFoliageEditMeshRow::GenerateWidgetForColumn(const FName& ColumnID)
{
	TSharedPtr<SWidget> TableRowContent = SNullWidget::NullWidget;

	if (ColumnID == FoliageMeshColumns::ColumnID_ToggleMesh)
	{
		TableRowContent =
			SNew(SCheckBox)
			.OnCheckStateChanged( this, &SFoliageEditMeshRow::OnMeshCheckStateChanged)
			.IsChecked( this, &SFoliageEditMeshRow::IsMeshChecked);
	} 
	else if (ColumnID == FoliageMeshColumns::ColumnID_MeshLabel)
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
				.Text(this, &SFoliageEditMeshRow::GetMeshRowText)
			];
	}
	else if (ColumnID == FoliageMeshColumns::ColumnID_InstanceCount)
	{
		TableRowContent =
			SNew(SHorizontalBox)
			
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(this, &SFoliageEditMeshRow::GetMeshInstanceCountText)
			];
	}
	else if (ColumnID == FoliageMeshColumns::ColumnID_Save)
	{
		TableRowContent = 
			SNew(SButton)
			.ContentPadding(0)
			.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
			.IsEnabled(this, &SFoliageEditMeshRow::IsSaveEnabled)
			.OnClicked(this, &SFoliageEditMeshRow::OnSave)
			.ToolTipText(LOCTEXT("SaveButtonToolTip", "Save foliage asset"))
			.HAlign( HAlign_Center )
			.VAlign( VAlign_Center )
			.Content()
			[
				SNew(SImage)
				.Image(this, &SFoliageEditMeshRow::GetMeshSaveBrush)
			];
	}

	return TableRowContent.ToSharedRef();
}

FText SFoliageEditMeshRow::GetMeshRowText() const
{
	const UFoliageType* FoliageType = MeshInfo->Settings;
	if (FoliageType->IsAsset())
	{
		return FText::FromString(FoliageType->GetName());
	}
	else
	{
		UStaticMesh* Mesh = FoliageType->GetStaticMesh();
		if (Mesh)
		{
			return FText::FromString(Mesh->GetName());
		}
		else
		{
			return LOCTEXT("EmptyMeshName", "[Empty Mesh]");
		}
	}
}
	
void SFoliageEditMeshRow::OnMeshCheckStateChanged(const ECheckBoxState NewCheckedState)
{
	bool bSelected = (NewCheckedState == ECheckBoxState::Checked);
	if (MeshInfo->Settings->IsSelected != bSelected)
	{
		MeshInfo->Settings->Modify();
		MeshInfo->Settings->IsSelected = bSelected;
	}
}

ECheckBoxState SFoliageEditMeshRow::IsMeshChecked() const
{
	return MeshInfo->Settings->IsSelected ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

bool SFoliageEditMeshRow::IsSaveEnabled() const
{
	UFoliageType* FoliageType = MeshInfo->Settings;
	// We want user to convert FoliageType to a shared asset
	return (!FoliageType->IsAsset() || FoliageType->GetOutermost()->IsDirty());
}

FReply SFoliageEditMeshRow::OnSave()
{
	UFoliageType* SavedSettings = FoliageEditMode->SaveSettingsObject(MeshInfo->Settings);
	if (SavedSettings)
	{
		MeshInfo->Settings = SavedSettings;
	}

	return FReply::Handled();
}

const FSlateBrush* SFoliageEditMeshRow::GetMeshSaveBrush() const
{
	return FEditorStyle::GetBrush("Level.SaveIcon16x");
	
	//if (LevelModel->IsLoaded())
	//{
	//	if (LevelModel->IsLocked())
	//	{
	//		return FEditorStyle::GetBrush( "Level.SaveDisabledIcon16x" );
	//	}
	//	else
	//	{
	//		if (LevelModel->IsDirty())
	//		{
	//			return SaveButton->IsHovered() ? FEditorStyle::GetBrush( "Level.SaveModifiedHighlightIcon16x" ) :
	//												FEditorStyle::GetBrush( "Level.SaveModifiedIcon16x" );
	//		}
	//		else
	//		{
	//			return SaveButton->IsHovered() ? FEditorStyle::GetBrush( "Level.SaveHighlightIcon16x" ) :
	//												FEditorStyle::GetBrush( "Level.SaveIcon16x" );
	//		}
	//	}								
	//}
	//else
	//{
	//	return FEditorStyle::GetBrush( "Level.EmptyIcon16x" );
	//}	

}

FText SFoliageEditMeshRow::GetMeshInstanceCountText() const
{
	const int32	InstanceCountTotal = MeshInfo->InstanceCountTotal;
	const int32	InstanceCountCurrentLevel = MeshInfo->InstanceCountCurrentLevel;
	
	if (InstanceCountCurrentLevel != InstanceCountTotal)
	{
		return FText::Format(LOCTEXT("InstanceCount", "{0} ({1})"), FText::AsNumber(InstanceCountCurrentLevel), FText::AsNumber(InstanceCountTotal));
	}
	else
	{
		return FText::AsNumber(InstanceCountCurrentLevel);
	}
}

#undef LOCTEXT_NAMESPACE
