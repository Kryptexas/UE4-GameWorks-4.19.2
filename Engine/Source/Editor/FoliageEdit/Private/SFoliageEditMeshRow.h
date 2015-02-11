// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

namespace FoliageMeshColumns
{
	/** IDs for list columns */
	static const FName ColumnID_MeshLabel("Mesh");
	static const FName ColumnID_ToggleMesh("Toggle");
	static const FName ColumnID_InstanceCount("InstanceCount");
	static const FName ColumnID_Save("Save");
};

class FEdModeFoliage;
struct FFoliageMeshUIInfo;
typedef TSharedPtr<FFoliageMeshUIInfo> FFoliageMeshUIInfoPtr; //should match typedef in FoliageEdMode.h

class SFoliageEditMeshRow
	: public SMultiColumnTableRow<FFoliageMeshUIInfoPtr>
{
public:
	SLATE_BEGIN_ARGS( SFoliageEditMeshRow )
		{}
		SLATE_ARGUMENT(FFoliageMeshUIInfoPtr,	MeshInfo)
		SLATE_ARGUMENT(FEdModeFoliage*,			FoliageEditMode)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<STableViewBase> OwnerTableView);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

	FText GetMeshRowText() const;
	
	void OnMeshCheckStateChanged(const ECheckBoxState NewCheckedState);
	ECheckBoxState IsMeshChecked() const;

	bool IsSaveEnabled() const;
	FReply OnSave();
	const FSlateBrush* GetMeshSaveBrush() const;

	FText GetMeshInstanceCountText() const;
	
private:
	FFoliageMeshUIInfoPtr	MeshInfo;
	FEdModeFoliage*			FoliageEditMode;
};