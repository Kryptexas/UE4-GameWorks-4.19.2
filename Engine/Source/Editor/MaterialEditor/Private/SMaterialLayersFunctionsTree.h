// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "Materials/Material.h"
#include "MaterialEditor/MaterialEditorInstanceConstant.h"
#include "IDetailTreeNode.h"
#include "IDetailPropertyRow.h"
#include "MaterialPropertyHelpers.h"

class IPropertyHandle;
class UMaterialEditorInstanceConstant;

class SMaterialLayersFunctionsInstanceWrapper : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterialLayersFunctionsInstanceWrapper)
		: _InMaterialEditorInstance(nullptr)
	{}

	SLATE_ARGUMENT(UMaterialEditorInstanceConstant*, InMaterialEditorInstance)

	SLATE_END_ARGS()
	void Refresh();
	void Construct(const FArguments& InArgs);
	void SetEditorInstance(UMaterialEditorInstanceConstant* InMaterialEditorInstance);

	TAttribute<ECheckBoxState> IsParamChecked;
	class UDEditorParameterValue* LayerParameter;
	class UMaterialEditorInstanceConstant* MaterialEditorInstance;
	TSharedPtr<class SMaterialLayersFunctionsInstanceTree> NestedTree;
	FSimpleDelegate OnLayerPropertyChanged;
};

class SMaterialLayersFunctionsInstanceTree : public STreeView<TSharedPtr<FStackSortedData>>
{
	friend class SMaterialLayersFunctionsInstanceTreeItem;
public:
	SLATE_BEGIN_ARGS(SMaterialLayersFunctionsInstanceTree)
		: _InMaterialEditorInstance(nullptr)
	{}

	SLATE_ARGUMENT(UMaterialEditorInstanceConstant*, InMaterialEditorInstance)
	SLATE_ARGUMENT(SMaterialLayersFunctionsInstanceWrapper*, InWrapper)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	TSharedRef< ITableRow > OnGenerateRowMaterialLayersFunctionsTreeView(TSharedPtr<FStackSortedData> Item, const TSharedRef< STableViewBase >& OwnerTable);
	void OnGetChildrenMaterialLayersFunctionsTreeView(TSharedPtr<FStackSortedData> InParent, TArray< TSharedPtr<FStackSortedData> >& OutChildren);
	void OnExpansionChanged(TSharedPtr<FStackSortedData> Item, bool bIsExpanded);
	void SetParentsExpansionState();

	float OnGetLeftColumnWidth() const { return 1.0f - ColumnWidth; }
	float OnGetRightColumnWidth() const { return ColumnWidth; }
	void OnSetColumnWidth(float InWidth) { ColumnWidth = InWidth; }
	void ShowHiddenValues(bool& bShowHiddenParameters) { bShowHiddenParameters = true; }
	FName LayersFunctionsParameterName;
	class UDEditorParameterValue* FunctionParameter;
	struct FMaterialLayersFunctions* FunctionInstance;
	TSharedPtr<IPropertyHandle> FunctionInstanceHandle;
	void RefreshOnAssetChange(const struct FAssetData& InAssetData, int32 Index, EMaterialParameterAssociation MaterialType);
	void ResetAssetToDefault(TSharedPtr<IPropertyHandle> InHandle, TSharedPtr<FStackSortedData> InData);
	void AddLayer();
	void RemoveLayer(int32 Index);
	FReply ToggleLayerVisibility(int32 Index);
	TSharedPtr<class FAssetThumbnailPool> GetTreeThumbnailPool();

	/** Object that stores all of the possible parameters we can edit */
	UMaterialEditorInstanceConstant* MaterialEditorInstance;

	/** Builds the custom parameter groups category */
	void CreateGroupsWidget();

	bool IsLayerVisible(int32 Index) const;

	SMaterialLayersFunctionsInstanceWrapper* GetWrapper() { return Wrapper; }



	TSharedRef<SWidget> CreateThumbnailWidget(EMaterialParameterAssociation InAssociation, int32 InIndex, float InThumbnailSize);
	void UpdateThumbnailMaterial(TEnumAsByte<EMaterialParameterAssociation> InAssociation, int32 InIndex, bool bAlterBlendIndex = false);
protected:

	void ShowSubParameters(TSharedPtr<FStackSortedData> ParentParameter);

private:
	TArray<TSharedPtr<FStackSortedData>> LayerProperties;

	TArray<FLayerParameterUnsortedData> NonLayerProperties;

	/** The actual width of the right column.  The left column is 1-ColumnWidth */
	float ColumnWidth;

	SMaterialLayersFunctionsInstanceWrapper* Wrapper;

	TSharedPtr<class IPropertyRowGenerator> Generator;

};

class UMaterialEditorPreviewParameters;

class SMaterialLayersFunctionsMaterialWrapper : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterialLayersFunctionsMaterialWrapper)
		: _InMaterialEditorInstance(nullptr)
	{}

	SLATE_ARGUMENT(UMaterialEditorPreviewParameters*, InMaterialEditorInstance)

	SLATE_END_ARGS()
	void Refresh();
	void Construct(const FArguments& InArgs);
	void SetEditorInstance(UMaterialEditorPreviewParameters* InMaterialEditorInstance);

	class UDEditorParameterValue* LayerParameter;
	UMaterialEditorPreviewParameters* MaterialEditorInstance;
	TSharedPtr<class SMaterialLayersFunctionsMaterialTree> NestedTree;
};


class SMaterialLayersFunctionsMaterialTree : public STreeView<TSharedPtr<FStackSortedData>>
{
	friend class SMaterialLayersFunctionsMaterialTreeItem;
public:
	SLATE_BEGIN_ARGS(SMaterialLayersFunctionsMaterialTree)
		: _InMaterialEditorInstance(nullptr)
	{}

	SLATE_ARGUMENT(UMaterialEditorPreviewParameters*, InMaterialEditorInstance)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	TSharedRef< ITableRow > OnGenerateRowMaterialLayersFunctionsTreeView(TSharedPtr<FStackSortedData> Item, const TSharedRef< STableViewBase >& OwnerTable);
	void OnGetChildrenMaterialLayersFunctionsTreeView(TSharedPtr<FStackSortedData> InParent, TArray< TSharedPtr<FStackSortedData> >& OutChildren);
	void OnExpansionChanged(TSharedPtr<FStackSortedData> Item, bool bIsExpanded);
	void SetParentsExpansionState();

	float OnGetLeftColumnWidth() const { return 1.0f - ColumnWidth; }
	float OnGetRightColumnWidth() const { return ColumnWidth; }
	void OnSetColumnWidth(float InWidth) { ColumnWidth = InWidth; }
	FName LayersFunctionsParameterName;
	class UDEditorParameterValue* FunctionParameter;
	struct FMaterialLayersFunctions* FunctionInstance;
	TSharedPtr<IPropertyHandle> FunctionInstanceHandle;
	TSharedPtr<class FAssetThumbnailPool> GetTreeThumbnailPool();

	/** Object that stores all of the possible parameters we can edit */
	UMaterialEditorPreviewParameters* MaterialEditorInstance;

	/** Builds the custom parameter groups category */
	void CreateGroupsWidget();

protected:

	void ShowSubParameters(TSharedPtr<FStackSortedData> ParentParameter);

private:
	TArray<TSharedPtr<FStackSortedData>> LayerProperties;

	TArray<FLayerParameterUnsortedData> NonLayerProperties;

	/** The actual width of the right column.  The left column is 1-ColumnWidth */
	float ColumnWidth;

	TSharedPtr<class IPropertyRowGenerator> Generator;

};