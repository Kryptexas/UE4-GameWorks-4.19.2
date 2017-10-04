// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SMaterialLayersFunctionsTree.h"
#include "MaterialEditor/DEditorFontParameterValue.h"
#include "MaterialEditor/DEditorMaterialLayersParameterValue.h"
#include "MaterialEditor/DEditorScalarParameterValue.h"
#include "MaterialEditor/DEditorStaticComponentMaskParameterValue.h"
#include "MaterialEditor/DEditorStaticSwitchParameterValue.h"
#include "MaterialEditor/DEditorTextureParameterValue.h"
#include "MaterialEditor/DEditorVectorParameterValue.h"
#include "MaterialEditor/MaterialEditorInstanceConstant.h"
#include "Materials/Material.h"
#include "PropertyHandle.h"
#include "ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ISinglePropertyView.h"
#include "Materials/MaterialInstanceConstant.h"
#include "EditorStyleSet.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "IPropertyRowGenerator.h"
#include "STreeView.h"
#include "IDetailTreeNode.h"
#include "AssetThumbnail.h"
#include "MaterialEditorInstanceDetailCustomization.h"
#include "MaterialPropertyHelpers.h"
#include "DetailWidgetRow.h"
#include "SCheckBox.h"
#include "EditorSupportDelegates.h"
#include "SImage.h"

#define LOCTEXT_NAMESPACE "MaterialLayerCustomization"

FText SMaterialLayersFunctionsTree::LayerID = LOCTEXT("LayerID", "Layer Asset");
FText SMaterialLayersFunctionsTree::BlendID = LOCTEXT("BlendID", "Blend Asset");

class SMaterialLayersFunctionsTreeViewItem : public STableRow< TSharedPtr<FStackSortedData> >
{
public:

	SLATE_BEGIN_ARGS(SMaterialLayersFunctionsTreeViewItem)
		: _StackParameterData(nullptr),
		 _MaterialEditorInstance(nullptr)
	{}

	/** The item content. */
	SLATE_ARGUMENT(TSharedPtr<FStackSortedData>, StackParameterData)
	SLATE_ARGUMENT(UMaterialEditorInstanceConstant*, MaterialEditorInstance)
	SLATE_ARGUMENT(SMaterialLayersFunctionsTree*, InTree)
	SLATE_END_ARGS()

	FMaterialTreeColumnSizeData ColumnSizeData;
	void RefreshOnRowChange(const FAssetData& AssetData, SMaterialLayersFunctionsTree* InTree)
	{
		const FScopedTransaction Transaction(LOCTEXT("AddLayerAndBlend", "Add a new Layer and a Blend into it"));
		InTree->CreateGroupsWidget();
		InTree->RequestTreeRefresh();
	}


	/**
	* Construct the widget
	*
	* @param InArgs   A declaration from which to construct the widget
	*/
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		StackParameterData = InArgs._StackParameterData;
		MaterialEditorInstance = InArgs._MaterialEditorInstance;

		ColumnSizeData.LeftColumnWidth = TAttribute<float>(InArgs._InTree, &SMaterialLayersFunctionsTree::OnGetLeftColumnWidth);
		ColumnSizeData.RightColumnWidth = TAttribute<float>(InArgs._InTree, &SMaterialLayersFunctionsTree::OnGetRightColumnWidth);
		ColumnSizeData.OnWidthChanged = SSplitter::FOnSlotResized::CreateSP(InArgs._InTree, &SMaterialLayersFunctionsTree::OnSetColumnWidth);

		TSharedRef<SWidget> LeftSideWidget = SNullWidget::NullWidget;
		TSharedRef<SWidget> RightSideWidget = SNullWidget::NullWidget;
		FText NameOverride;
		TSharedRef<SVerticalBox> WrapperWidget = SNew(SVerticalBox);

		if (StackParameterData->StackDataType == EStackDataType::Property)
		{
			UMaterialFunctionInterface* OwningInterface = nullptr;
			if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::LayerParameter)
			{
				OwningInterface = InArgs._InTree->FunctionInstance->Layers[StackParameterData->ParameterInfo.Index];
			}
			if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::BlendParameter)
			{
				OwningInterface = InArgs._InTree->FunctionInstance->Blends[StackParameterData->ParameterInfo.Index];
			}

		
			TAttribute<bool> IsParamEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateStatic(&FMaterialPropertyHelpers::IsOverriddenExpression, StackParameterData->Parameter));
			NameOverride = FText::FromName(StackParameterData->Parameter->ParameterInfo.Name);
			FIsResetToDefaultVisible IsResetVisible = FIsResetToDefaultVisible::CreateStatic(&FMaterialPropertyHelpers::LayerParamShouldShowResetToDefault, StackParameterData->Parameter, OwningInterface, MaterialEditorInstance);
			FResetToDefaultHandler ResetHandler = FResetToDefaultHandler::CreateStatic(&FMaterialPropertyHelpers::ResetLayerParamToDefault, StackParameterData->Parameter, OwningInterface, MaterialEditorInstance);
			FResetToDefaultOverride ResetOverride = FResetToDefaultOverride::Create(IsResetVisible, ResetHandler);
			
			IDetailTreeNode& Node = *StackParameterData->ParameterNode;
			TSharedPtr<IDetailPropertyRow> GeneratedRow = StaticCastSharedPtr<IDetailPropertyRow>(Node.GetRow());
			IDetailPropertyRow& Row = *GeneratedRow.Get();
			Row
				.DisplayName(NameOverride)
				.OverrideResetToDefault(ResetOverride)
				.EditCondition(IsParamEnabled, FOnBooleanValueChanged::CreateStatic(&FMaterialPropertyHelpers::OnOverrideParameter, StackParameterData->Parameter, MaterialEditorInstance));


			UDEditorStaticComponentMaskParameterValue* CompMaskParam = Cast<UDEditorStaticComponentMaskParameterValue>(StackParameterData->Parameter);
			if (!CompMaskParam)
			{
				FNodeWidgets StoredNodeWidgets = Node.CreateNodeWidgets();
				TSharedRef<SWidget> StoredRightSideWidget = StoredNodeWidgets.ValueWidget.ToSharedRef();
				FDetailWidgetRow& CustomWidget = Row.CustomWidget();
				CustomWidget
				.FilterString(NameOverride)
				.NameContent()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(NameOverride)
						.ToolTipText(FMaterialPropertyHelpers::GetParameterExpressionDescription(StackParameterData->Parameter, MaterialEditorInstance))
						.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
					]
				]
				.ValueContent()
				[
					StoredRightSideWidget
				];

			}
			else
			{
				TSharedPtr<IPropertyHandle> RMaskProperty = StackParameterData->ParameterNode->CreatePropertyHandle()->GetChildHandle("R");
				TSharedPtr<IPropertyHandle> GMaskProperty = StackParameterData->ParameterNode->CreatePropertyHandle()->GetChildHandle("G");
				TSharedPtr<IPropertyHandle> BMaskProperty = StackParameterData->ParameterNode->CreatePropertyHandle()->GetChildHandle("B");
				TSharedPtr<IPropertyHandle> AMaskProperty = StackParameterData->ParameterNode->CreatePropertyHandle()->GetChildHandle("A");
				FDetailWidgetRow& CustomWidget = Row.CustomWidget();
				CustomWidget
				.FilterString(NameOverride)
				.NameContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(NameOverride)
						.ToolTipText(FMaterialPropertyHelpers::GetParameterExpressionDescription(StackParameterData->Parameter, MaterialEditorInstance))
						.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
					]
				]
				.ValueContent()
				.MaxDesiredWidth(200.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						.AutoWidth()
						[
							RMaskProperty->CreatePropertyNameWidget(FText::GetEmpty(), FText::GetEmpty(), false)
						]
						+ SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						.AutoWidth()
						[
							RMaskProperty->CreatePropertyValueWidget()
						]
						+ SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						.Padding(FMargin(10.0f, 0.0f, 0.0f, 0.0f))
						.AutoWidth()
						[
							GMaskProperty->CreatePropertyNameWidget(FText::GetEmpty(), FText::GetEmpty(), false)
						]
						+ SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						.AutoWidth()
						[
							GMaskProperty->CreatePropertyValueWidget()
						]
						+ SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						.Padding(FMargin(10.0f, 0.0f, 0.0f, 0.0f))
						.AutoWidth()
						[
							BMaskProperty->CreatePropertyNameWidget(FText::GetEmpty(), FText::GetEmpty(), false)
						]
						+ SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						.AutoWidth()
						[
							BMaskProperty->CreatePropertyValueWidget()
						]
						+ SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						.Padding(FMargin(10.0f, 0.0f, 0.0f, 0.0f))
						.AutoWidth()
						[
							AMaskProperty->CreatePropertyNameWidget(FText::GetEmpty(), FText::GetEmpty(), false)
						]
						+ SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						.AutoWidth()
						[
							AMaskProperty->CreatePropertyValueWidget()
						]
					]	
				];
			}

			FNodeWidgets NodeWidgets = Node.CreateNodeWidgets();
			LeftSideWidget = NodeWidgets.NameWidget.ToSharedRef();
			RightSideWidget = NodeWidgets.ValueWidget.ToSharedRef();
		
		}


		if (StackParameterData->StackDataType == EStackDataType::PropertyChild)
		{
			FNodeWidgets NodeWidgets = StackParameterData->ParameterNode->CreateNodeWidgets();
			LeftSideWidget = NodeWidgets.NameWidget.ToSharedRef();
			RightSideWidget = NodeWidgets.ValueWidget.ToSharedRef();
		}


		if (StackParameterData->StackDataType == EStackDataType::Stack)
		{
			WrapperWidget->AddSlot()
			.Padding(1.0f)
			.AutoHeight()
			[
				SNullWidget::NullWidget
			];
#if WITH_EDITOR
			NameOverride = InArgs._InTree->FunctionInstance->GetLayerName(StackParameterData->ParameterInfo.Index);
#endif
			TSharedRef<SHorizontalBox> HeaderRowWidget = SNew(SHorizontalBox);

			if (StackParameterData->ParameterInfo.Index != 0)
			{
				TAttribute<bool>::FGetter IsEnabledGetter = TAttribute<bool>::FGetter::CreateSP(InArgs._InTree, &SMaterialLayersFunctionsTree::IsLayerVisible, StackParameterData->ParameterInfo.Index);
				TAttribute<bool> IsEnabledAttribute = TAttribute<bool>::Create(IsEnabledGetter);

				FOnClicked VisibilityClickedDelegate = FOnClicked::CreateSP(InArgs._InTree, &SMaterialLayersFunctionsTree::ToggleLayerVisibility, StackParameterData->ParameterInfo.Index);

				HeaderRowWidget->AddSlot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						PropertyCustomizationHelpers::MakeVisibilityButton(VisibilityClickedDelegate, FText(), IsEnabledAttribute)
					];
			}
			const float ThumbnailSize = 24.0f;
			TArray<TSharedPtr<FStackSortedData>> AssetChildren = StackParameterData->Children;
			if (AssetChildren.Num() > 0)
			{
				HeaderRowWidget->AddSlot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(2.5f, 0)
					.AutoWidth()
					[
						SNullWidget::NullWidget
					];
			}
			for (TSharedPtr<FStackSortedData> AssetChild : AssetChildren)
			{
				TSharedPtr<SBox> ThumbnailBox;
				UObject* AssetObject;
				AssetChild->ParameterHandle->GetValue(AssetObject);
				const TSharedPtr<FAssetThumbnail> AssetThumbnail = MakeShareable(new FAssetThumbnail(AssetObject, ThumbnailSize, ThumbnailSize, InArgs._InTree->GetTreeThumbnailPool()));
				HeaderRowWidget->AddSlot()
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(4.0f)
					.MaxWidth(ThumbnailSize)
					[
						SAssignNew(ThumbnailBox, SBox)
						[						
							AssetThumbnail->MakeThumbnailWidget()
						]
					];
				ThumbnailBox->SetMaxDesiredHeight(ThumbnailSize);
				ThumbnailBox->SetMinDesiredHeight(ThumbnailSize);
			}

			HeaderRowWidget->AddSlot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				.Padding(5.0f)
				[
					SNew(STextBlock)
					.Text(NameOverride)
					.TextStyle(FEditorStyle::Get(), "NormalText.Important")
				];

			if (StackParameterData->ParameterInfo.Index != 0 && FMaterialPropertyHelpers::IsOverriddenExpression(InArgs._InTree->FunctionParameter))
			{
				HeaderRowWidget->AddSlot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNullWidget::NullWidget
					];
				HeaderRowWidget->AddSlot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 5.0f, 0.0f)
					[
						PropertyCustomizationHelpers::MakeClearButton(FSimpleDelegate::CreateSP(InArgs._InTree, &SMaterialLayersFunctionsTree::RemoveLayer, StackParameterData->ParameterInfo.Index))
					];
			}
			LeftSideWidget = HeaderRowWidget;
		}


		if (StackParameterData->StackDataType == EStackDataType::Group)
		{
			NameOverride = FText::FromName(StackParameterData->Group.GroupName);
			LeftSideWidget = SNew(STextBlock)
				.Text(NameOverride)
				.TextStyle(FEditorStyle::Get(), "TinyText");
		}

		

		FMargin RowMargin = FMargin(0.0f);
		
		if (StackParameterData->StackDataType == EStackDataType::Asset)
		{
			FOnShouldFilterAsset AssetFilter = FOnShouldFilterAsset::CreateStatic(&SMaterialLayersFunctionsTreeViewItem::FilterTargets, StackParameterData->ParameterInfo.Association);
			FOnSetObject ObjectChanged = FOnSetObject::CreateSP(this, &SMaterialLayersFunctionsTreeViewItem::RefreshOnRowChange, InArgs._InTree);

			if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::LayerParameter)
			{
				NameOverride = SMaterialLayersFunctionsTree::LayerID;
				StackParameterData->ParameterHandle->GetProperty()->SetMetaData(FName(TEXT("DisplayThumbnail")), TEXT("true"));
			}
			else if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::BlendParameter)
			{
				NameOverride = SMaterialLayersFunctionsTree::BlendID;
			}


			TAttribute<bool> IsParamEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateStatic(&FMaterialPropertyHelpers::IsOverriddenExpression, StackParameterData->Parameter));
			FIsResetToDefaultVisible IsResetVisible = FIsResetToDefaultVisible::CreateStatic(&FMaterialPropertyHelpers::ShouldLayerAssetShowResetToDefault, StackParameterData->Parameter, 
				StackParameterData->ParameterInfo.Association, StackParameterData->ParameterInfo.Index, MaterialEditorInstance);
			FResetToDefaultHandler ResetHandler = FResetToDefaultHandler::CreateSP(InArgs._InTree, &SMaterialLayersFunctionsTree::ResetToDefault, StackParameterData);
			FResetToDefaultOverride ResetOverride = FResetToDefaultOverride::Create(IsResetVisible, ResetHandler);

			IDetailTreeNode& Node = *StackParameterData->ParameterNode;
			FNodeWidgets NodeWidgets = Node.CreateNodeWidgets();

			LeftSideWidget = StackParameterData->ParameterHandle->CreatePropertyNameWidget(NameOverride);

			StackParameterData->ParameterHandle->MarkResetToDefaultCustomized(false);
			RightSideWidget = SNew(SObjectPropertyEntryBox)
				.AllowedClass(UMaterialFunctionInterface::StaticClass())
				.PropertyHandle(StackParameterData->ParameterHandle)
				.ThumbnailPool(InArgs._InTree->GetTreeThumbnailPool())
				.OnShouldFilterAsset(AssetFilter)
				.OnObjectChanged(ObjectChanged)
				.CustomResetToDefault(ResetOverride);


			RightSideWidget->SetEnabled(FMaterialPropertyHelpers::IsOverriddenExpression(InArgs._InTree->FunctionParameter));


		}

		if (StackParameterData->StackDataType != EStackDataType::Stack)
		{
			WrapperWidget->AddSlot()
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("MaterialInstanceEditor.StackBody"))
					.Padding(RowMargin)
					[
						SNew(SSplitter)
						.Style(FEditorStyle::Get(), "DetailsView.Splitter")
						.PhysicalSplitterHandleSize(1.0f)
						.HitDetectionSplitterHandleSize(5.0f)
						+ SSplitter::Slot()
						.Value(ColumnSizeData.LeftColumnWidth)
						.OnSlotResized(ColumnSizeData.OnWidthChanged)
						.Value(0.25f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(FMargin(3.0f))
							[
								SNew(SExpanderArrow, SharedThis(this))
							]
							+ SHorizontalBox::Slot()
							.Padding(FMargin(2.0f))
							.VAlign(VAlign_Center)
							[
								LeftSideWidget
							]
						]
						+ SSplitter::Slot()
						.Value(ColumnSizeData.RightColumnWidth)
						.OnSlotResized(ColumnSizeData.OnWidthChanged)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.MaxWidth(350.0f)
							.Padding(FMargin(5.0f, 2.0f, 0.0f, 2.0f))
							[
								RightSideWidget
							]
						]
					]
				];
		}
		else
		{
			WrapperWidget->AddSlot()
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("MaterialInstanceEditor.StackHeader"))
					.Padding(RowMargin)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(FMargin(2.0f))
						[
							SNew(SExpanderArrow, SharedThis(this))
						]
						+ SHorizontalBox::Slot()
						.Padding(FMargin(2.0f))
						.VAlign(VAlign_Center)
						[
							LeftSideWidget
						]
					]
				];
		}

		this->ChildSlot
			[
				WrapperWidget
			];

		STableRow< TSharedPtr<FStackSortedData> >::ConstructInternal(
			STableRow::FArguments()
			.Style(FEditorStyle::Get(), "DetailsView.TreeView.TableRow")
			.ShowSelection(false),
			InOwnerTableView
		);
	}

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override { return FReply::Handled(); };

	static bool FilterTargets(const struct FAssetData& InAssetData, TEnumAsByte<EMaterialParameterAssociation> MaterialType);

	/** The node info to build the tree view row from. */
	TSharedPtr<FStackSortedData> StackParameterData;

	UMaterialEditorInstanceConstant* MaterialEditorInstance;
};


bool SMaterialLayersFunctionsTreeViewItem::FilterTargets(const struct FAssetData& InAssetData, TEnumAsByte<EMaterialParameterAssociation> MaterialType)
{
	bool ShouldAssetBeFilteredOut = false;
	const FName FilterTag = FName(TEXT("MaterialFunctionUsage"));
	const FString* MaterialFunctionUsage = InAssetData.TagsAndValues.Find(FilterTag);
	FString CompareString;
	if (MaterialFunctionUsage)
	{
		switch (MaterialType)
		{
		case EMaterialParameterAssociation::LayerParameter:
		{
			CompareString = "MaterialLayer";
		}
		break;
		case EMaterialParameterAssociation::BlendParameter:
		{
			CompareString = "MaterialLayerBlend";
		}
		break;
		default:
			break;
		}

		if (*MaterialFunctionUsage != CompareString)
		{
			ShouldAssetBeFilteredOut = true;
		}
	}
	else
	{
		ShouldAssetBeFilteredOut = true;
	}
	return ShouldAssetBeFilteredOut;
}


void SMaterialLayersFunctionsTree::Construct(const FArguments& InArgs)
{
	ColumnWidth = 0.5f;
	MaterialEditorInstance = InArgs._InMaterialEditorInstance;

	CreateGroupsWidget();

	STreeView<TSharedPtr<FStackSortedData>>::Construct(
		STreeView::FArguments()
		.TreeItemsSource(&LayerProperties)
		.SelectionMode(ESelectionMode::None)
		.OnGenerateRow(this, &SMaterialLayersFunctionsTree::OnGenerateRowMaterialLayersFunctionsTreeView)
		.OnGetChildren(this, &SMaterialLayersFunctionsTree::OnGetChildrenMaterialLayersFunctionsTreeView)
		.OnExpansionChanged(this, &SMaterialLayersFunctionsTree::OnExpansionChanged)
	);

	SetParentsExpansionState();
}

TSharedRef< ITableRow > SMaterialLayersFunctionsTree::OnGenerateRowMaterialLayersFunctionsTreeView(TSharedPtr<FStackSortedData> Item, const TSharedRef< STableViewBase >& OwnerTable)
{
	TSharedRef< SMaterialLayersFunctionsTreeViewItem > ReturnRow = SNew(SMaterialLayersFunctionsTreeViewItem, OwnerTable)
		.StackParameterData(Item)
		.MaterialEditorInstance(MaterialEditorInstance)
		.InTree(this);
	return ReturnRow;
}

void SMaterialLayersFunctionsTree::OnGetChildrenMaterialLayersFunctionsTreeView(TSharedPtr<FStackSortedData> InParent, TArray< TSharedPtr<FStackSortedData> >& OutChildren)
{
	OutChildren = InParent->Children;
}


void SMaterialLayersFunctionsTree::OnExpansionChanged(TSharedPtr<FStackSortedData> Item, bool bIsExpanded)
{
	bool* ExpansionValue = MaterialEditorInstance->SourceInstance->LayerParameterExpansion.Find(Item->NodeKey);
	if (ExpansionValue == nullptr)
	{
		MaterialEditorInstance->SourceInstance->LayerParameterExpansion.Add(Item->NodeKey, bIsExpanded);
	}
	else if (*ExpansionValue != bIsExpanded)
	{
		MaterialEditorInstance->SourceInstance->LayerParameterExpansion.Emplace(Item->NodeKey, bIsExpanded);
	}
	// Expand any children that are also expanded
	for (auto Child : Item->Children)
	{
		bool* ChildExpansionValue = MaterialEditorInstance->SourceInstance->LayerParameterExpansion.Find(Child->NodeKey);
		if (ChildExpansionValue != nullptr && *ChildExpansionValue == true)
		{
			SetItemExpansion(Child, true);
		}
	}
}

void SMaterialLayersFunctionsTree::SetParentsExpansionState()
{
	for (const auto& Pair : LayerProperties)
	{
		if (Pair->Children.Num())
		{
			bool* bIsExpanded = MaterialEditorInstance->SourceInstance->LayerParameterExpansion.Find(Pair->NodeKey);
			if (bIsExpanded)
			{
				SetItemExpansion(Pair, *bIsExpanded);
			}
		}
	}
}

void SMaterialLayersFunctionsTree::ResetToDefault(TSharedPtr<IPropertyHandle> InHandle, TSharedPtr<FStackSortedData> InData)
{
	if (InData->StackDataType == EStackDataType::Stack)
	{
		FMaterialPropertyHelpers::ResetToDefault(InData->ParameterHandle.ToSharedRef(), InData->Parameter, MaterialEditorInstance);
	}
	else if (InData->StackDataType == EStackDataType::Asset)
	{

		FMaterialPropertyHelpers::ResetLayerAssetToDefault(FunctionInstanceHandle.ToSharedRef(), InData->Parameter, InData->ParameterInfo.Association, InData->ParameterInfo.Index, MaterialEditorInstance);
	}
	CreateGroupsWidget();
	RequestTreeRefresh();
}

void SMaterialLayersFunctionsTree::AddLayer()
{
	const FScopedTransaction Transaction(LOCTEXT("AddLayerAndBlend", "Add a new Layer and a Blend into it"));
	FunctionInstanceHandle->NotifyPreChange();
	FunctionInstance->AppendBlendedLayer();
	FunctionInstanceHandle->NotifyPostChange();
	CreateGroupsWidget();
	RequestTreeRefresh();
}

void SMaterialLayersFunctionsTree::RemoveLayer(int32 Index)
{
	const FScopedTransaction Transaction(LOCTEXT("RemoveLayerAndBlend", "Remove a Layer and the attached Blend"));
	FunctionInstanceHandle->NotifyPreChange();
	FunctionInstance->RemoveBlendedLayerAt(Index);
	FunctionInstanceHandle->NotifyPostChange();
	CreateGroupsWidget();
	RequestTreeRefresh();
}

FReply SMaterialLayersFunctionsTree::ToggleLayerVisibility(int32 Index)
{
	const FScopedTransaction Transaction(LOCTEXT("ToggleLayerAndBlendVisibility", "Toggles visibility for a blended layer"));
	FunctionInstanceHandle->NotifyPreChange();
	FunctionInstance->ToggleBlendedLayerVisibility(Index);
	FunctionInstanceHandle->NotifyPostChange();
	CreateGroupsWidget();
	RequestTreeRefresh();
	return FReply::Handled();
}

TSharedPtr<class FAssetThumbnailPool> SMaterialLayersFunctionsTree::GetTreeThumbnailPool()
{
	return Generator->GetGeneratedThumbnailPool();
}

void SMaterialLayersFunctionsTree::CreateGroupsWidget()
{
	check(MaterialEditorInstance);

	NonLayerProperties.Empty();
	LayerProperties.Empty();
	FPropertyEditorModule& Module = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	if (!Generator.IsValid())
	{
		FPropertyRowGeneratorArgs Args;
		Generator = Module.CreatePropertyRowGenerator(Args);

		TArray<UObject*> Objects;
		Objects.Add(MaterialEditorInstance);
		Generator->SetObjects(Objects);
	}
	else
	{
		TArray<UObject*> Objects;
		Objects.Add(MaterialEditorInstance);
		Generator->SetObjects(Objects);
	}
	const TArray<TSharedRef<IDetailTreeNode>> TestData = Generator->GetRootTreeNodes();
	TSharedPtr<IDetailTreeNode> Category = TestData[0];
	TSharedPtr<IDetailTreeNode> ParameterGroups;
	TArray<TSharedRef<IDetailTreeNode>> Children;
	Category->GetChildren(Children);

	for (int32 ChildIdx = 0; ChildIdx < Children.Num(); ChildIdx++)
	{
		if (Children[ChildIdx]->CreatePropertyHandle().IsValid() &&
			Children[ChildIdx]->CreatePropertyHandle()->GetProperty()->GetName() == "ParameterGroups")
		{
			ParameterGroups = Children[ChildIdx];
			break;
		}
	}

	Children.Empty();
	ParameterGroups->GetChildren(Children);
	for (int32 GroupIdx = 0; GroupIdx < Children.Num(); ++GroupIdx)
	{
		TArray<void*> GroupPtrs;
		TSharedPtr<IPropertyHandle> ChildHandle = Children[GroupIdx]->CreatePropertyHandle();
		ChildHandle->AccessRawData(GroupPtrs);
		auto GroupIt = GroupPtrs.CreateConstIterator();
		const FEditorParameterGroup* ParameterGroupPtr = reinterpret_cast<FEditorParameterGroup*>(*GroupIt);
		const FEditorParameterGroup& ParameterGroup = *ParameterGroupPtr;
		
		for (int32 ParamIdx = 0; ParamIdx < ParameterGroup.Parameters.Num(); ParamIdx++)
		{
			UDEditorParameterValue* Parameter = ParameterGroup.Parameters[ParamIdx];

			TSharedPtr<IPropertyHandle> ParametersArrayProperty = ChildHandle->GetChildHandle("Parameters");
			TSharedPtr<IPropertyHandle> ParameterProperty = ParametersArrayProperty->GetChildHandle(ParamIdx);
			TSharedPtr<IPropertyHandle> ParameterValueProperty = ParameterProperty->GetChildHandle("ParameterValue");
				
			if (Cast<UDEditorMaterialLayersParameterValue>(Parameter))
			{
				if (FunctionParameter == nullptr)
				{
					FunctionParameter = Parameter;
				}
				TArray<void*> StructPtrs;
				ParameterValueProperty->AccessRawData(StructPtrs);
				auto It = StructPtrs.CreateConstIterator();
				FunctionInstance = reinterpret_cast<FMaterialLayersFunctions*>(*It);
				FunctionInstanceHandle = ParameterValueProperty;
				LayersFunctionsParameterName = FName(Parameter->ParameterInfo.Name);

				TSharedPtr<IPropertyHandle>	LayerHandle = ChildHandle->GetChildHandle("Layers").ToSharedRef();
				TSharedPtr<IPropertyHandle> BlendHandle = ChildHandle->GetChildHandle("Blends").ToSharedRef();
				uint32 LayerChildren;
				LayerHandle->GetNumChildren(LayerChildren);
				uint32 BlendChildren;
				BlendHandle->GetNumChildren(BlendChildren);
					
				TSharedPtr<FStackSortedData> StackProperty(new FStackSortedData());
				StackProperty->StackDataType = EStackDataType::Stack;
				StackProperty->Parameter = Parameter;
				StackProperty->ParameterInfo.Index = LayerChildren - 1;
				StackProperty->NodeKey = FString::FromInt(Parameter->ParameterInfo.Index);


				TSharedPtr<FStackSortedData> ChildProperty(new FStackSortedData());
				ChildProperty->StackDataType = EStackDataType::Asset;
				ChildProperty->Parameter = Parameter;
				ChildProperty->ParameterHandle = LayerHandle->AsArray()->GetElement(LayerChildren - 1);
				ChildProperty->ParameterNode = Generator->FindTreeNode(ChildProperty->ParameterHandle);
				ChildProperty->ParameterInfo.Index = LayerChildren - 1;
				ChildProperty->ParameterInfo.Association = EMaterialParameterAssociation::LayerParameter;
				ChildProperty->NodeKey = FString::FromInt(Parameter->ParameterInfo.Index) + FString::FromInt(Parameter->ParameterInfo.Association);

				StackProperty->Children.Add(ChildProperty);
				LayerProperties.Add(StackProperty);
					
				if (BlendChildren > 0 && LayerChildren > BlendChildren)
				{
					for (int32 Counter = BlendChildren - 1; Counter >= 0; Counter--)
					{
						ChildProperty = MakeShareable(new FStackSortedData());
						ChildProperty->StackDataType = EStackDataType::Asset;
						ChildProperty->Parameter = Parameter;
						ChildProperty->ParameterHandle = BlendHandle->AsArray()->GetElement(Counter);	
						ChildProperty->ParameterNode = Generator->FindTreeNode(ChildProperty->ParameterHandle);
						ChildProperty->ParameterInfo.Index = Counter;
						ChildProperty->ParameterInfo.Association = EMaterialParameterAssociation::BlendParameter;
						ChildProperty->NodeKey = FString::FromInt(Parameter->ParameterInfo.Index) + FString::FromInt(Parameter->ParameterInfo.Association);
						LayerProperties.Last()->Children.Add(ChildProperty);
							
						StackProperty = MakeShareable(new FStackSortedData());
						StackProperty->StackDataType = EStackDataType::Stack;
						StackProperty->Parameter = Parameter;
						StackProperty->ParameterInfo.Index = Counter;
						StackProperty->NodeKey = FString::FromInt(Parameter->ParameterInfo.Index);
						LayerProperties.Add(StackProperty);

						ChildProperty = MakeShareable(new FStackSortedData());
						ChildProperty->StackDataType = EStackDataType::Asset;
						ChildProperty->Parameter = Parameter;
						ChildProperty->ParameterHandle = LayerHandle->AsArray()->GetElement(Counter);
						ChildProperty->ParameterNode = Generator->FindTreeNode(ChildProperty->ParameterHandle);
						ChildProperty->ParameterInfo.Index = Counter;
						ChildProperty->ParameterInfo.Association = EMaterialParameterAssociation::LayerParameter;
						ChildProperty->NodeKey = FString::FromInt(Parameter->ParameterInfo.Index) + FString::FromInt(Parameter->ParameterInfo.Association);

						LayerProperties.Last()->Children.Add(ChildProperty);
					}
				}
			}
			else
			{
				FLayerParameterUnsortedData NonLayerProperty;
				UDEditorScalarParameterValue* ScalarParam = Cast<UDEditorScalarParameterValue>(Parameter);

				if (ScalarParam && ScalarParam->SliderMax > ScalarParam->SliderMin)
				{
					ParameterValueProperty->SetInstanceMetaData("UIMin", FString::Printf(TEXT("%f"), ScalarParam->SliderMin));
					ParameterValueProperty->SetInstanceMetaData("UIMax", FString::Printf(TEXT("%f"), ScalarParam->SliderMax));
				}
					
				NonLayerProperty.Parameter = Parameter;
				NonLayerProperty.ParameterGroup = ParameterGroup;
				NonLayerProperty.ParameterNode = Generator->FindTreeNode(ParameterValueProperty);
				NonLayerProperty.ParameterHandle = NonLayerProperty.ParameterNode->CreatePropertyHandle();
				NonLayerProperty.UnsortedName = Parameter->ParameterInfo.Name;

				NonLayerProperties.Add(NonLayerProperty);
			}
		}
	}
	for (int32 LayerIdx = 0; LayerIdx < LayerProperties.Num(); LayerIdx++)
	{
		for (int32 ChildIdx = 0; ChildIdx < LayerProperties[LayerIdx]->Children.Num(); ChildIdx++)
		{
			ShowSubParameters(LayerProperties[LayerIdx]->Children[ChildIdx]);
		}
	}
	SetParentsExpansionState();
}

bool SMaterialLayersFunctionsTree::IsLayerVisible(int32 Index) const
{
	return FunctionInstance->GetLayerVisibility(Index);
}

void SMaterialLayersFunctionsTree::ShowSubParameters(TSharedPtr<FStackSortedData> ParentParameter)
{
	for (FLayerParameterUnsortedData Property : NonLayerProperties)
	{
		UDEditorParameterValue* Parameter = Property.Parameter;
		if (Parameter->ParameterInfo.Index == ParentParameter->ParameterInfo.Index
			&& Parameter->ParameterInfo.Association == ParentParameter->ParameterInfo.Association)
		{
			TSharedPtr<FStackSortedData> GroupProperty(new FStackSortedData());
			GroupProperty->StackDataType = EStackDataType::Group;
			GroupProperty->ParameterInfo.Index = Parameter->ParameterInfo.Index;
			GroupProperty->ParameterInfo.Association = Parameter->ParameterInfo.Association;
			GroupProperty->Group = Property.ParameterGroup;
			GroupProperty->NodeKey = FString::FromInt(Parameter->ParameterInfo.Index) + FString::FromInt(Parameter->ParameterInfo.Association) + Property.ParameterGroup.GroupName.ToString();

			ParentParameter->Children.AddUnique(GroupProperty);

			TSharedPtr<FStackSortedData> ChildProperty(new FStackSortedData());
			ChildProperty->StackDataType = EStackDataType::Property;
			ChildProperty->Parameter = Parameter;
			ChildProperty->ParameterInfo.Index = Parameter->ParameterInfo.Index;
			ChildProperty->ParameterInfo.Association = Parameter->ParameterInfo.Association;
			ChildProperty->ParameterNode = Property.ParameterNode;
			ChildProperty->PropertyName = Property.UnsortedName;
			ChildProperty->NodeKey = FString::FromInt(Parameter->ParameterInfo.Index) + FString::FromInt(Parameter->ParameterInfo.Association) +  Property.ParameterGroup.GroupName.ToString() + Property.UnsortedName.ToString();


			UDEditorStaticComponentMaskParameterValue* CompMaskParam = Cast<UDEditorStaticComponentMaskParameterValue>(Parameter);
			if (!CompMaskParam)
			{
				TArray<TSharedRef<IDetailTreeNode>> ParamChildren;
				Property.ParameterNode->GetChildren(ParamChildren);
				for (int32 ParamChildIdx = 0; ParamChildIdx < ParamChildren.Num(); ParamChildIdx++)
				{
					TSharedPtr<FStackSortedData> ParamChildProperty(new FStackSortedData());
					ParamChildProperty->StackDataType = EStackDataType::PropertyChild;
					ParamChildProperty->ParameterNode = ParamChildren[ParamChildIdx];
					ParamChildProperty->ParameterHandle = ParamChildProperty->ParameterNode->CreatePropertyHandle();
					ParamChildProperty->ParameterInfo.Index = Parameter->ParameterInfo.Index;
					ParamChildProperty->ParameterInfo.Association = Parameter->ParameterInfo.Association;
					ChildProperty->Children.Add(ParamChildProperty);
				}
			}
			int32 GroupParentIndex = ParentParameter->Children.Find(GroupProperty);
			if (GroupParentIndex != INDEX_NONE)
			{
				ParentParameter->Children[GroupParentIndex]->Children.Add(ChildProperty);
			}

		}
	}
}

void SMaterialLayersFunctionsInstanceWrapper::Refresh()
{
	TSharedPtr<SHorizontalBox> HeaderBox;

	this->ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("MaterialInstanceEditor.LayersBorder"))
		.Padding(FMargin(4.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(HeaderBox, SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.OnCheckStateChanged(FOnCheckStateChanged::CreateStatic(&FMaterialPropertyHelpers::OnOverrideParameterCheckbox, NestedTree->FunctionParameter, MaterialEditorInstance))
					.IsChecked(IsParamChecked)
				]
				+ SHorizontalBox::Slot()
				.Padding(FMargin(3.0f, 1.0f))
				.HAlign(HAlign_Left)
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromName(NestedTree->LayersFunctionsParameterName))
					.TextStyle(FEditorStyle::Get(), "LargeText")
				]
			]
			+ SVerticalBox::Slot()
			.Padding(FMargin(3.0f, 0.0f))
			[
				NestedTree.ToSharedRef()
			]
		]
	];
	if (NestedTree->FunctionParameter != nullptr && FMaterialPropertyHelpers::IsOverriddenExpression(NestedTree->FunctionParameter))
	{
		HeaderBox->AddSlot()
			.HAlign(HAlign_Left)
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				PropertyCustomizationHelpers::MakeAddButton(FSimpleDelegate::CreateSP(NestedTree.Get(), &SMaterialLayersFunctionsTree::AddLayer))
			];
	}
	NestedTree->CreateGroupsWidget();
	NestedTree->RequestTreeRefresh();
}


void SMaterialLayersFunctionsInstanceWrapper::Construct(const FArguments& InArgs)
{
	NestedTree = SNew(SMaterialLayersFunctionsTree)
		.InMaterialEditorInstance(InArgs._InMaterialEditorInstance);

	LayerParameter = NestedTree->FunctionParameter;
	IsParamChecked = TAttribute<ECheckBoxState>::Create(TAttribute<ECheckBoxState>::FGetter::CreateStatic(&FMaterialPropertyHelpers::IsOverriddenExpressionCheckbox, LayerParameter));

	MaterialEditorInstance = InArgs._InMaterialEditorInstance;
	FEditorSupportDelegates::UpdateUI.AddSP(this, &SMaterialLayersFunctionsInstanceWrapper::Refresh);

}

#undef LOCTEXT_NAMESPACE