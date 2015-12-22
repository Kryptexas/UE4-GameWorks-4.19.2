// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Classes/Factories/FbxSceneImportFactory.h"
#include "SSceneImportStaticMeshListView.h"

class SFbxSceneStaticMeshReimportListView : public SListView<FbxMeshInfoPtr>
{
public:

	~SFbxSceneStaticMeshReimportListView();

	SLATE_BEGIN_ARGS(SFbxSceneStaticMeshReimportListView)
		: _SceneInfo(nullptr)
		, _SceneInfoOriginal(nullptr)
		, _MeshStatusMap(nullptr)
		, _GlobalImportSettings(nullptr)
		, _OverrideNameOptionsMap(nullptr)
		, _SceneImportOptionsStaticMeshDisplay(nullptr)
	{}
		SLATE_ARGUMENT(TSharedPtr<FFbxSceneInfo>, SceneInfo)
		SLATE_ARGUMENT(TSharedPtr<FFbxSceneInfo>, SceneInfoOriginal)
		SLATE_ARGUMENT(FbxSceneReimportStatusMapPtr, MeshStatusMap)
		SLATE_ARGUMENT( UnFbx::FBXImportOptions*, GlobalImportSettings)
		SLATE_ARGUMENT( ImportOptionsNameMapPtr, OverrideNameOptionsMap)
		SLATE_ARGUMENT( class UFbxSceneImportOptionsStaticMesh*, SceneImportOptionsStaticMeshDisplay)
		SLATE_END_ARGS()

	/** Construct this widget */
	void Construct(const FArguments& InArgs);
	TSharedRef< ITableRow > OnGenerateRowFbxSceneListView(FbxMeshInfoPtr Item, const TSharedRef< STableViewBase >& OwnerTable);
	FString FindUniqueOptionName();
	FReply OnCreateOverrideOptions();
	void OnToggleSelectAll(ECheckBoxState CheckType);

	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
	TSharedPtr<STextComboBox> CreateOverrideOptionComboBox();

	/** Filter show every reimport that will add content */
	void OnToggleFilterAddContent(ECheckBoxState CheckType);
	ECheckBoxState IsFilterAddContentChecked() const
	{
		return FilterAddContent ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	/** Filter show every reimport that will delete content */
	void OnToggleFilterDeleteContent(ECheckBoxState CheckType);
	ECheckBoxState IsFilterDeleteContentChecked() const
	{
		return FilterDeleteContent ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	/** Filter show every reimport that will override content */
	void OnToggleFilterOverwriteContent(ECheckBoxState CheckType);
	ECheckBoxState IsFilterOverwriteContentChecked() const
	{
		return FilterOverwriteContent ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	/** Filter show every reimport that dont match between the original fbx and the new one */
	void OnToggleFilterDiff(ECheckBoxState CheckType);
	ECheckBoxState IsFilterDiffChecked() const
	{
		return FilterDiff ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

protected:
	TSharedPtr<FFbxSceneInfo> SceneInfo;
	TSharedPtr<FFbxSceneInfo> SceneInfoOriginal;
	UnFbx::FBXImportOptions* GlobalImportSettings;
	ImportOptionsNameMapPtr OverrideNameOptionsMap;
	class UFbxSceneImportOptionsStaticMesh* SceneImportOptionsStaticMeshDisplay;

	UnFbx::FBXImportOptions *CurrentStaticMeshImportOptions;

	/** the elements we show in the list view */
	TArray<FbxMeshInfoPtr> FbxMeshesArray;
	TArray<FbxMeshInfoPtr> FilterFbxMeshesArray;
	bool FilterAddContent;
	bool FilterDeleteContent;
	bool FilterOverwriteContent;
	bool FilterDiff;

	void UpdateFilterList();

	FbxSceneReimportStatusMapPtr MeshStatusMap;

	/** Open a context menu for the current selection */
	TSharedPtr<SWidget> OnOpenContextMenu();
	void AssignToStaticMesh();
	bool ShowResetAssignToStaticMesh();
	void ResetAssignToStaticMesh();
	void AddSelectionToImport();
	void RemoveSelectionFromImport();
	void SetSelectionImportState(bool MarkForImport);

	void OnSelectionChanged(FbxMeshInfoPtr Item, ESelectInfo::Type SelectionType);
	void OnChangedOverrideOptions(TSharedPtr<FString> ItemSelected, ESelectInfo::Type SelectInfo);
	TSharedPtr<FString> FindOptionNameFromName(FString OptionName);
	void AssignToOptions(FString OptionName);

	TArray<TSharedPtr<FString>> OverrideNameOptions;
	TSharedPtr<STextComboBox> OptionComboBox;
	TSharedPtr<FString> DefaultOptionNamePtr;
};