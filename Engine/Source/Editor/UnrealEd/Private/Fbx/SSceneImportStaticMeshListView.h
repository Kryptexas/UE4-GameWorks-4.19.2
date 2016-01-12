// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Classes/Factories/FbxSceneImportFactory.h"

class STextComboBox;

typedef TSharedPtr<FFbxMeshInfo> FbxMeshInfoPtr;
typedef TMap<FString, EFbxSceneReimportStatusFlags> FbxSceneReimportStatusMap;
typedef FbxSceneReimportStatusMap* FbxSceneReimportStatusMapPtr;

class SFbxSceneStaticMeshListView : public SListView<FbxMeshInfoPtr>
{
public:
	
	~SFbxSceneStaticMeshListView();

	SLATE_BEGIN_ARGS(SFbxSceneStaticMeshListView)
	: _SceneInfo(nullptr)
	, _GlobalImportSettings(nullptr)
	, _OverrideNameOptionsMap(nullptr)
	, _SceneImportOptionsStaticMeshDisplay(nullptr)
	{}
		SLATE_ARGUMENT(TSharedPtr<FFbxSceneInfo>, SceneInfo)
		SLATE_ARGUMENT(UnFbx::FBXImportOptions*, GlobalImportSettings)
		SLATE_ARGUMENT(ImportOptionsNameMapPtr, OverrideNameOptionsMap)
		SLATE_ARGUMENT(class UFbxSceneImportOptionsStaticMesh*, SceneImportOptionsStaticMeshDisplay)
	SLATE_END_ARGS()
	
	/** Construct this widget */
	void Construct(const FArguments& InArgs);
	TSharedRef< ITableRow > OnGenerateRowFbxSceneListView(FbxMeshInfoPtr Item, const TSharedRef< STableViewBase >& OwnerTable);
	bool CanDeleteOverride()  const;
	FReply OnDeleteOverride();
	FReply OnSelectAssetUsing();
	FReply OnCreateOverrideOptions();
	void OnToggleSelectAll(ECheckBoxState CheckType);
	
	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
	TSharedPtr<STextComboBox> CreateOverrideOptionComboBox();

protected:
	TSharedPtr<FFbxSceneInfo> SceneInfo;
	UnFbx::FBXImportOptions* GlobalImportSettings;

	UnFbx::FBXImportOptions *CurrentStaticMeshImportOptions;

	/** the elements we show in the tree view */
	TArray<FbxMeshInfoPtr> FbxMeshesArray;
	class UFbxSceneImportOptionsStaticMesh *SceneImportOptionsStaticMeshDisplay;

	/** Open a context menu for the current selection */
	TSharedPtr<SWidget> OnOpenContextMenu();
	void AddSelectionToImport();
	void RemoveSelectionFromImport();
	void SetSelectionImportState(bool MarkForImport);
	void OnSelectionChanged(FbxMeshInfoPtr Item, ESelectInfo::Type SelectionType);

	void OnChangedOverrideOptions(TSharedPtr<FString> ItemSelected, ESelectInfo::Type SelectInfo);
	TSharedPtr<FString> FindOptionNameFromName(FString OptionName);
	void AssignToOptions(FString OptionName);
	FString FindUniqueOptionName();
	

	TArray<TSharedPtr<FString>> OverrideNameOptions;
	ImportOptionsNameMapPtr OverrideNameOptionsMap;
	TSharedPtr<STextComboBox> OptionComboBox;
	TSharedPtr<FString> DefaultOptionNamePtr;
};
