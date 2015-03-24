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

	// Text for foliage mesh object name field
	FText GetMeshRowText() const;
	
	// Handler for foliage mesh object selection(activation) 
	void OnMeshCheckStateChanged(const ECheckBoxState NewCheckedState);
	
	// Whether foliage mesh object is selected(active)
	ECheckBoxState IsMeshChecked() const;

	// Whether 'Save' button for foliage mesh should be enabled
	// Always enabled for private object, will save as shared asset
	// On shared foliage mesh objects enabled only when asset is dirty
	bool IsSaveEnabled() const;

	// Handle 'Save' foliage mesh object
	FReply OnSave();
	
	// Icon for 'Save' button
	const FSlateBrush* GetMeshSaveBrush() const;

	// Instance count text for foliage mesh
	FText GetMeshInstanceCountText() const;
	
private:
	FFoliageMeshUIInfoPtr	MeshInfo;
	FEdModeFoliage*			FoliageEditMode;
};