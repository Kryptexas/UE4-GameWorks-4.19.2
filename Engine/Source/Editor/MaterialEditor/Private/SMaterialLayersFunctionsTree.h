// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

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
#include "SMaterialLayersFunctionsTree.generated.h"

class IPropertyHandle;
class UMaterialEditorInstanceConstant;

enum EStackDataType
{
	Stack,
	Asset,
	Group,
	Property,
	PropertyChild,
};

USTRUCT()
struct MATERIALEDITOR_API FStackSortedData 
{
	GENERATED_USTRUCT_BODY()

public:
	EStackDataType StackDataType;

	class UDEditorParameterValue* Parameter;

	FName PropertyName;

	FEditorParameterGroup Group;

	FMaterialParameterInfo ParameterInfo;

	TSharedPtr<class IDetailTreeNode> ParameterNode;

	TSharedPtr<class IPropertyHandle> ParameterHandle;

	TArray<TSharedPtr<struct FStackSortedData>> Children;

	FString NodeKey;
};

struct FLayerParameterUnsortedData
{
	class UDEditorParameterValue* Parameter;
	struct FEditorParameterGroup ParameterGroup;
	TSharedPtr<class IDetailTreeNode> ParameterNode;
	FName UnsortedName;
	TSharedPtr<class IPropertyHandle> ParameterHandle;
};

struct FMaterialTreeColumnSizeData
{
	TAttribute<float> LeftColumnWidth;
	TAttribute<float> RightColumnWidth;
	SSplitter::FOnSlotResized OnWidthChanged;

	void SetColumnWidth(float InWidth) { OnWidthChanged.ExecuteIfBound(InWidth); }
};

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
	TAttribute<ECheckBoxState> IsParamChecked;
	class UDEditorParameterValue* LayerParameter;
	class UMaterialEditorInstanceConstant* MaterialEditorInstance;
	TSharedPtr<class SMaterialLayersFunctionsTree> NestedTree;
};

class SMaterialLayersFunctionsTree : public STreeView<TSharedPtr<FStackSortedData>>
{
	friend class SMaterialLayersFunctionsTreeViewItem;
public:
	SLATE_BEGIN_ARGS(SMaterialLayersFunctionsTree)
		: _InMaterialEditorInstance(nullptr)
	{}

	SLATE_ARGUMENT(UMaterialEditorInstanceConstant*, InMaterialEditorInstance)

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

	void ResetToDefault(TSharedPtr<IPropertyHandle> InHandle, TSharedPtr<FStackSortedData> InData);
	void AddLayer();
	void RemoveLayer(int32 Index);
	FReply ToggleLayerVisibility(int32 Index);
	TSharedPtr<class FAssetThumbnailPool> GetTreeThumbnailPool();

	/** Object that stores all of the possible parameters we can edit */
	UMaterialEditorInstanceConstant* MaterialEditorInstance;

	/** Builds the custom parameter groups category */
	void CreateGroupsWidget();

	bool IsLayerVisible(int32 Index) const;

protected:


	void ShowSubParameters(TSharedPtr<FStackSortedData> ParentParameter);


private:
	TArray<TSharedPtr<FStackSortedData>> LayerProperties;

	TArray<FLayerParameterUnsortedData> NonLayerProperties;

	static FText LayerID;
	static FText BlendID;

	/** The actual width of the right column.  The left column is 1-ColumnWidth */
	float ColumnWidth;

	
	TSharedPtr<class IPropertyRowGenerator> Generator;

};

