// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

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
#include "MaterialEditor/MaterialEditorPreviewParameters.h"
#include "SButton.h"


#define LOCTEXT_NAMESPACE "MaterialLayerCustomization"

class SMaterialLayersFunctionsInstanceTreeItem : public STableRow< TSharedPtr<FStackSortedData> >
{
public:

	SLATE_BEGIN_ARGS(SMaterialLayersFunctionsInstanceTreeItem)
		: _StackParameterData(nullptr),
		 _MaterialEditorInstance(nullptr)
	{}

	/** The item content. */
	SLATE_ARGUMENT(TSharedPtr<FStackSortedData>, StackParameterData)
	SLATE_ARGUMENT(UMaterialEditorInstanceConstant*, MaterialEditorInstance)
	SLATE_ARGUMENT(SMaterialLayersFunctionsInstanceTree*, InTree)
	SLATE_END_ARGS()

	FMaterialTreeColumnSizeData ColumnSizeData;

	void RefreshOnRowChange(const FAssetData& AssetData, SMaterialLayersFunctionsInstanceTree* InTree)
	{
		if (SMaterialLayersFunctionsInstanceWrapper* Wrapper = InTree->GetWrapper())
		{
			if (Wrapper->OnLayerPropertyChanged.IsBound())
			{
				Wrapper->OnLayerPropertyChanged.Execute();
			}
			else
			{
				InTree->CreateGroupsWidget();
			}
		}
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
		SMaterialLayersFunctionsInstanceTree* Tree = InArgs._InTree;
		ColumnSizeData.LeftColumnWidth = TAttribute<float>(Tree, &SMaterialLayersFunctionsInstanceTree::OnGetLeftColumnWidth);
		ColumnSizeData.RightColumnWidth = TAttribute<float>(Tree, &SMaterialLayersFunctionsInstanceTree::OnGetRightColumnWidth);
		ColumnSizeData.OnWidthChanged = SSplitter::FOnSlotResized::CreateSP(Tree, &SMaterialLayersFunctionsInstanceTree::OnSetColumnWidth);

		TSharedRef<SWidget> LeftSideWidget = SNullWidget::NullWidget;
		TSharedRef<SWidget> RightSideWidget = SNullWidget::NullWidget;
		FText NameOverride;
		TSharedRef<SVerticalBox> WrapperWidget = SNew(SVerticalBox);

// STACK --------------------------------------------------
		if (StackParameterData->StackDataType == EStackDataType::Stack)
		{
			WrapperWidget->AddSlot()
				.Padding(3.0f)
				.AutoHeight()
				[
					SNullWidget::NullWidget
				];
#if WITH_EDITOR
			NameOverride = Tree->FunctionInstance->GetLayerName(StackParameterData->ParameterInfo.Index);
#endif
			TSharedRef<SHorizontalBox> HeaderRowWidget = SNew(SHorizontalBox);

			if (StackParameterData->ParameterInfo.Index != 0)
			{
				TAttribute<bool>::FGetter IsEnabledGetter = TAttribute<bool>::FGetter::CreateSP(InArgs._InTree, &SMaterialLayersFunctionsInstanceTree::IsLayerVisible, StackParameterData->ParameterInfo.Index);
				TAttribute<bool> IsEnabledAttribute = TAttribute<bool>::Create(IsEnabledGetter);

				FOnClicked VisibilityClickedDelegate = FOnClicked::CreateSP(InArgs._InTree, &SMaterialLayersFunctionsInstanceTree::ToggleLayerVisibility, StackParameterData->ParameterInfo.Index);

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

			if (StackParameterData->ParameterInfo.Index != 0 && FMaterialPropertyHelpers::IsOverriddenExpression(Tree->FunctionParameter))
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
						PropertyCustomizationHelpers::MakeClearButton(FSimpleDelegate::CreateSP(InArgs._InTree, &SMaterialLayersFunctionsInstanceTree::RemoveLayer, StackParameterData->ParameterInfo.Index))
					];
			}
			LeftSideWidget = HeaderRowWidget;
		}
// END STACK

// GROUP --------------------------------------------------
		if (StackParameterData->StackDataType == EStackDataType::Group)
		{
			NameOverride = FText::FromName(StackParameterData->Group.GroupName);
			LeftSideWidget = SNew(STextBlock)
				.Text(NameOverride)
				.TextStyle(FEditorStyle::Get(), "TinyText");
		}
// END GROUP

// ASSET --------------------------------------------------
		if (StackParameterData->StackDataType == EStackDataType::Asset)
		{
			TSharedRef<STextBlock> ParentTextBlock = SNew(STextBlock)
				.Text(LOCTEXT("Parent", "Parent "))
				.TextStyle(FEditorStyle::Get(), "TinyText");
			ParentTextBlock->SetToolTipText(LOCTEXT("ParentTooltip", "This allows you to set the parent class of your layer or blend asset to filter the possible assets to use."));
			ParentTextBlock->EnableToolTipForceField(true);

			FOnSetObject ObjectChanged = FOnSetObject::CreateSP(this, &SMaterialLayersFunctionsInstanceTreeItem::RefreshOnRowChange, Tree);
			StackParameterData->ParameterHandle->GetProperty()->SetMetaData(FName(TEXT("DisplayThumbnail")), TEXT("true"));
			FIntPoint ThumbnailOverride;
			if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::LayerParameter)
			{
				NameOverride = FMaterialPropertyHelpers::LayerID;
				ThumbnailOverride = FIntPoint(64, 64);
			}
			else if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::BlendParameter)
			{
				NameOverride = FMaterialPropertyHelpers::BlendID;
				ThumbnailOverride = FIntPoint(32, 32);
			}


			TAttribute<bool> IsParamEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateStatic(&FMaterialPropertyHelpers::IsOverriddenExpression, StackParameterData->Parameter));
			FIsResetToDefaultVisible IsFilterResetVisible = FIsResetToDefaultVisible::CreateStatic(&FMaterialPropertyHelpers::ShouldLayerAssetShowResetToDefault, StackParameterData, EStackAssetType::Filter, MaterialEditorInstance->Parent);
			FResetToDefaultHandler ResetFilterHandler = FResetToDefaultHandler::CreateSP(InArgs._InTree, &SMaterialLayersFunctionsInstanceTree::ResetFilterToDefault, StackParameterData);
			FResetToDefaultOverride ResetFilterOverride = FResetToDefaultOverride::Create(IsFilterResetVisible, ResetFilterHandler);

			FIsResetToDefaultVisible IsInstanceResetVisible = FIsResetToDefaultVisible::CreateStatic(&FMaterialPropertyHelpers::ShouldLayerAssetShowResetToDefault, StackParameterData, EStackAssetType::Instance, MaterialEditorInstance->Parent);
			FResetToDefaultHandler ResetInstanceHandler = FResetToDefaultHandler::CreateSP(InArgs._InTree, &SMaterialLayersFunctionsInstanceTree::ResetInstanceToDefault, StackParameterData);
			FResetToDefaultOverride ResetInstanceOverride = FResetToDefaultOverride::Create(IsInstanceResetVisible, ResetInstanceHandler);

			IDetailTreeNode& Node = *StackParameterData->ParameterNode;
			FNodeWidgets NodeWidgets = Node.CreateNodeWidgets();

			LeftSideWidget = StackParameterData->ParameterHandle->CreatePropertyNameWidget(NameOverride);

			StackParameterData->ParameterHandle->MarkResetToDefaultCustomized(false);

			FString FilterPath;
			if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::BlendParameter && Tree->FunctionInstance->FilterBlends.IsValidIndex(StackParameterData->ParameterInfo.Index))
			{
				FilterPath = Tree->FunctionInstance->FilterBlends[StackParameterData->ParameterInfo.Index]->GetPathName();
			}
			else if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::LayerParameter && Tree->FunctionInstance->FilterLayers.IsValidIndex(StackParameterData->ParameterInfo.Index))
			{
				FilterPath = Tree->FunctionInstance->FilterLayers[StackParameterData->ParameterInfo.Index]->GetPathName();
			}

			FString InstancePath;
			if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::BlendParameter && Tree->FunctionInstance->InstanceBlends.IsValidIndex(StackParameterData->ParameterInfo.Index))
			{
				InstancePath = Tree->FunctionInstance->InstanceBlends[StackParameterData->ParameterInfo.Index]->GetPathName();
			}
			else if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::LayerParameter && Tree->FunctionInstance->InstanceLayers.IsValidIndex(StackParameterData->ParameterInfo.Index))
			{
				InstancePath = Tree->FunctionInstance->InstanceLayers[StackParameterData->ParameterInfo.Index]->GetPathName();
			}

			EMaterialParameterAssociation InAssociation = StackParameterData->ParameterInfo.Association;

			FOnShouldFilterAsset FilterFilter = FOnShouldFilterAsset::CreateStatic(&FMaterialPropertyHelpers::FilterAssetFilters, InAssociation);
			FOnShouldFilterAsset InstanceFilter = FOnShouldFilterAsset::CreateStatic(&FMaterialPropertyHelpers::FilterAssetInstances, Tree->FunctionInstance, InAssociation, StackParameterData->ParameterInfo.Index);

			FOnSetObject FilterAssetChanged = FOnSetObject::CreateSP(Tree, &SMaterialLayersFunctionsInstanceTree::RefreshOnAssetChange, StackParameterData->ParameterInfo.Index, InAssociation, true);
			FOnSetObject AssetChanged = FOnSetObject::CreateSP(Tree, &SMaterialLayersFunctionsInstanceTree::RefreshOnAssetChange, StackParameterData->ParameterInfo.Index, InAssociation, false);


			RightSideWidget = SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(5.0f)
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					ParentTextBlock
				]
			+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UMaterialFunction::StaticClass())
				.ObjectPath(FilterPath)
				.OnShouldFilterAsset(FilterFilter)
				.OnObjectChanged(FilterAssetChanged)
				.CustomResetToDefault(ResetFilterOverride)
				]
				]
			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UMaterialFunctionInterface::StaticClass())
				.ObjectPath(InstancePath)
				.ThumbnailPool(Tree->GetTreeThumbnailPool())
				.OnShouldFilterAsset(InstanceFilter)
				.OnObjectChanged(AssetChanged)
				.CustomResetToDefault(ResetInstanceOverride)
				.DisplayCompactSize(true)
				.ThumbnailSizeOverride(ThumbnailOverride)
				];


			RightSideWidget->SetEnabled(FMaterialPropertyHelpers::IsOverriddenExpression(InArgs._InTree->FunctionParameter));


		}
// END ASSET

// PROPERTY ----------------------------------------------
		if (StackParameterData->StackDataType == EStackDataType::Property)
		{
			TAttribute<bool> IsParamEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateStatic(&FMaterialPropertyHelpers::IsOverriddenExpression, StackParameterData->Parameter));
			NameOverride = FText::FromName(StackParameterData->Parameter->ParameterInfo.Name);
			FIsResetToDefaultVisible IsResetVisible = FIsResetToDefaultVisible::CreateStatic(&FMaterialPropertyHelpers::ShouldShowResetToDefault, StackParameterData->Parameter, MaterialEditorInstance);
			FResetToDefaultHandler ResetHandler = FResetToDefaultHandler::CreateStatic(&FMaterialPropertyHelpers::ResetToDefault, StackParameterData->Parameter, MaterialEditorInstance);
			FResetToDefaultOverride ResetOverride = FResetToDefaultOverride::Create(IsResetVisible, ResetHandler);
			
			IDetailTreeNode& Node = *StackParameterData->ParameterNode;
			TSharedPtr<IDetailPropertyRow> GeneratedRow = StaticCastSharedPtr<IDetailPropertyRow>(Node.GetRow());
			IDetailPropertyRow& Row = *GeneratedRow.Get();
			Row
				.DisplayName(NameOverride)
				.OverrideResetToDefault(ResetOverride)
				.EditCondition(IsParamEnabled, FOnBooleanValueChanged::CreateStatic(&FMaterialPropertyHelpers::OnOverrideParameter, StackParameterData->Parameter, MaterialEditorInstance));


			UDEditorStaticComponentMaskParameterValue* CompMaskParam = Cast<UDEditorStaticComponentMaskParameterValue>(StackParameterData->Parameter);
			UDEditorVectorParameterValue* VectorParam = Cast<UDEditorVectorParameterValue>(StackParameterData->Parameter);

			if (VectorParam && VectorParam->bIsUsedAsChannelMask)
			{
				FOnGetPropertyComboBoxStrings GetMaskStrings = FOnGetPropertyComboBoxStrings::CreateStatic(&FMaterialPropertyHelpers::GetVectorChannelMaskComboBoxStrings);
				FOnGetPropertyComboBoxValue GetMaskValue = FOnGetPropertyComboBoxValue::CreateStatic(&FMaterialPropertyHelpers::GetVectorChannelMaskValue, StackParameterData->Parameter);
				FOnPropertyComboBoxValueSelected SetMaskValue = FOnPropertyComboBoxValueSelected::CreateStatic(&FMaterialPropertyHelpers::SetVectorChannelMaskValue, StackParameterData->ParameterNode->CreatePropertyHandle(), StackParameterData->Parameter, (UObject*)MaterialEditorInstance);

				FDetailWidgetRow& CustomWidget = Row.CustomWidget();
				CustomWidget
				.FilterString(NameOverride)
				.NameContent()
				[
					SNew(STextBlock)
					.Text(NameOverride)
					.ToolTipText(FMaterialPropertyHelpers::GetParameterExpressionDescription(StackParameterData->Parameter, MaterialEditorInstance))
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
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
							PropertyCustomizationHelpers::MakePropertyComboBox(StackParameterData->ParameterNode->CreatePropertyHandle(), GetMaskStrings, GetMaskValue, SetMaskValue)
						]
					]
				];
			}
			else if (!CompMaskParam)
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
// END PROPERTY

// PROPERTY CHILD ----------------------------------------
		if (StackParameterData->StackDataType == EStackDataType::PropertyChild)
		{
			FNodeWidgets NodeWidgets = StackParameterData->ParameterNode->CreateNodeWidgets();
			LeftSideWidget = NodeWidgets.NameWidget.ToSharedRef();
			RightSideWidget = NodeWidgets.ValueWidget.ToSharedRef();
		}
// END PROPERTY CHILD

// FINAL WRAPPER
		if (StackParameterData->StackDataType == EStackDataType::Stack)
		{
			WrapperWidget->AddSlot()
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("MaterialInstanceEditor.StackHeader"))
					.Padding(0.0f)
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
		else
		{
			FSlateBrush* StackBrush;
			switch (StackParameterData->ParameterInfo.Association)
			{
			case EMaterialParameterAssociation::LayerParameter:
			{
				StackBrush = const_cast<FSlateBrush*>(FEditorStyle::GetBrush("MaterialInstanceEditor.StackBody"));
				break;
			}
			case EMaterialParameterAssociation::BlendParameter:
			{
				StackBrush = const_cast<FSlateBrush*>(FEditorStyle::GetBrush("MaterialInstanceEditor.StackBodyBlend"));
				break;
			}
			default:
				break;
			}
			WrapperWidget->AddSlot()
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(StackBrush)
					.Padding(0.0f)
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

	// Disable double-click expansion
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override { return FReply::Handled(); };

	/** The node info to build the tree view row from. */
	TSharedPtr<FStackSortedData> StackParameterData;

	UMaterialEditorInstanceConstant* MaterialEditorInstance;
};


void SMaterialLayersFunctionsInstanceTree::Construct(const FArguments& InArgs)
{
	ColumnWidth = 0.5f;
	MaterialEditorInstance = InArgs._InMaterialEditorInstance;
	Wrapper = InArgs._InWrapper;
	CreateGroupsWidget();

	STreeView<TSharedPtr<FStackSortedData>>::Construct(
		STreeView::FArguments()
		.TreeItemsSource(&LayerProperties)
		.SelectionMode(ESelectionMode::None)
		.OnGenerateRow(this, &SMaterialLayersFunctionsInstanceTree::OnGenerateRowMaterialLayersFunctionsTreeView)
		.OnGetChildren(this, &SMaterialLayersFunctionsInstanceTree::OnGetChildrenMaterialLayersFunctionsTreeView)
		.OnExpansionChanged(this, &SMaterialLayersFunctionsInstanceTree::OnExpansionChanged)
	);

	SetParentsExpansionState();
}

TSharedRef< ITableRow > SMaterialLayersFunctionsInstanceTree::OnGenerateRowMaterialLayersFunctionsTreeView(TSharedPtr<FStackSortedData> Item, const TSharedRef< STableViewBase >& OwnerTable)
{
	TSharedRef< SMaterialLayersFunctionsInstanceTreeItem > ReturnRow = SNew(SMaterialLayersFunctionsInstanceTreeItem, OwnerTable)
		.StackParameterData(Item)
		.MaterialEditorInstance(MaterialEditorInstance)
		.InTree(this);
	return ReturnRow;
}

void SMaterialLayersFunctionsInstanceTree::OnGetChildrenMaterialLayersFunctionsTreeView(TSharedPtr<FStackSortedData> InParent, TArray< TSharedPtr<FStackSortedData> >& OutChildren)
{
	OutChildren = InParent->Children;
}


void SMaterialLayersFunctionsInstanceTree::OnExpansionChanged(TSharedPtr<FStackSortedData> Item, bool bIsExpanded)
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

void SMaterialLayersFunctionsInstanceTree::SetParentsExpansionState()
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

void SMaterialLayersFunctionsInstanceTree::RefreshOnAssetChange(const struct FAssetData& InAssetData, int32 Index, EMaterialParameterAssociation MaterialType, const bool bIsFilterField)
{
	FMaterialPropertyHelpers::OnMaterialLayerAssetChanged(InAssetData, Index, MaterialType, FunctionInstanceHandle, FunctionInstance, bIsFilterField);
	CreateGroupsWidget();
	RequestTreeRefresh();
}


void SMaterialLayersFunctionsInstanceTree::ResetFilterToDefault(TSharedPtr<IPropertyHandle> InHandle, TSharedPtr<FStackSortedData> InData)
{
	FMaterialPropertyHelpers::ResetLayerFilterAssetToDefault(FunctionInstanceHandle.ToSharedRef(), InData->Parameter, InData->ParameterInfo.Association, InData->ParameterInfo.Index, MaterialEditorInstance);	
	CreateGroupsWidget();
	RequestTreeRefresh();
}

void SMaterialLayersFunctionsInstanceTree::ResetInstanceToDefault(TSharedPtr<IPropertyHandle> InHandle, TSharedPtr<FStackSortedData> InData)
{
	FMaterialPropertyHelpers::ResetLayerInstanceAssetToDefault(FunctionInstanceHandle.ToSharedRef(), InData->Parameter, InData->ParameterInfo.Association, InData->ParameterInfo.Index, MaterialEditorInstance);
	CreateGroupsWidget();
	RequestTreeRefresh();
}

void SMaterialLayersFunctionsInstanceTree::AddLayer()
{
	const FScopedTransaction Transaction(LOCTEXT("AddLayerAndBlend", "Add a new Layer and a Blend into it"));
	FunctionInstanceHandle->NotifyPreChange();
	FunctionInstance->AppendBlendedLayer();
	FunctionInstanceHandle->NotifyPostChange();
	CreateGroupsWidget();
	RequestTreeRefresh();
}

void SMaterialLayersFunctionsInstanceTree::RemoveLayer(int32 Index)
{
	const FScopedTransaction Transaction(LOCTEXT("RemoveLayerAndBlend", "Remove a Layer and the attached Blend"));
	FunctionInstanceHandle->NotifyPreChange();
	FunctionInstance->RemoveBlendedLayerAt(Index);
	FunctionInstanceHandle->NotifyPostChange();
	CreateGroupsWidget();
	RequestTreeRefresh();
}

FReply SMaterialLayersFunctionsInstanceTree::ToggleLayerVisibility(int32 Index)
{
	const FScopedTransaction Transaction(LOCTEXT("ToggleLayerAndBlendVisibility", "Toggles visibility for a blended layer"));
	FunctionInstanceHandle->NotifyPreChange();
	FunctionInstance->ToggleBlendedLayerVisibility(Index);
	FunctionInstanceHandle->NotifyPostChange();
	CreateGroupsWidget();
	return FReply::Handled();
}

TSharedPtr<class FAssetThumbnailPool> SMaterialLayersFunctionsInstanceTree::GetTreeThumbnailPool()
{
	return Generator->GetGeneratedThumbnailPool();
}

void SMaterialLayersFunctionsInstanceTree::CreateGroupsWidget()
{
	check(MaterialEditorInstance);

	NonLayerProperties.Empty();
	LayerProperties.Empty();
	FunctionParameter = nullptr;
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
				StackProperty->NodeKey = FString::FromInt(StackProperty->ParameterInfo.Index);


				TSharedPtr<FStackSortedData> ChildProperty(new FStackSortedData());
				ChildProperty->StackDataType = EStackDataType::Asset;
				ChildProperty->Parameter = Parameter;
				ChildProperty->ParameterHandle = LayerHandle->AsArray()->GetElement(LayerChildren - 1);
				ChildProperty->ParameterNode = Generator->FindTreeNode(ChildProperty->ParameterHandle);
				ChildProperty->ParameterInfo.Index = LayerChildren - 1;
				ChildProperty->ParameterInfo.Association = EMaterialParameterAssociation::LayerParameter;
				ChildProperty->NodeKey = FString::FromInt(ChildProperty->ParameterInfo.Index) + FString::FromInt(ChildProperty->ParameterInfo.Association);

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
						ChildProperty->NodeKey = FString::FromInt(ChildProperty->ParameterInfo.Index) + FString::FromInt(ChildProperty->ParameterInfo.Association);
						LayerProperties.Last()->Children.Add(ChildProperty);
							
						StackProperty = MakeShareable(new FStackSortedData());
						StackProperty->StackDataType = EStackDataType::Stack;
						StackProperty->Parameter = Parameter;
						StackProperty->ParameterInfo.Index = Counter;
						StackProperty->NodeKey = FString::FromInt(StackProperty->ParameterInfo.Index);
						LayerProperties.Add(StackProperty);

						ChildProperty = MakeShareable(new FStackSortedData());
						ChildProperty->StackDataType = EStackDataType::Asset;
						ChildProperty->Parameter = Parameter;
						ChildProperty->ParameterHandle = LayerHandle->AsArray()->GetElement(Counter);
						ChildProperty->ParameterNode = Generator->FindTreeNode(ChildProperty->ParameterHandle);
						ChildProperty->ParameterInfo.Index = Counter;
						ChildProperty->ParameterInfo.Association = EMaterialParameterAssociation::LayerParameter;
						ChildProperty->NodeKey = FString::FromInt(ChildProperty->ParameterInfo.Index) + FString::FromInt(ChildProperty->ParameterInfo.Association);

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

bool SMaterialLayersFunctionsInstanceTree::IsLayerVisible(int32 Index) const
{
	return FunctionInstance->GetLayerVisibility(Index);
}

void SMaterialLayersFunctionsInstanceTree::ShowSubParameters(TSharedPtr<FStackSortedData> ParentParameter)
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
			GroupProperty->NodeKey = FString::FromInt(GroupProperty->ParameterInfo.Index) + FString::FromInt(GroupProperty->ParameterInfo.Association) + Property.ParameterGroup.GroupName.ToString();

			bool bAddNewGroup = true;
			for (TSharedPtr<struct FStackSortedData> GroupChild : ParentParameter->Children)
			{
				if (GroupChild->NodeKey == GroupProperty->NodeKey)
				{
					bAddNewGroup = false;
				}
			}
			if (bAddNewGroup)
			{
				ParentParameter->Children.Add(GroupProperty);
			}

			TSharedPtr<FStackSortedData> ChildProperty(new FStackSortedData());
			ChildProperty->StackDataType = EStackDataType::Property;
			ChildProperty->Parameter = Parameter;
			ChildProperty->ParameterInfo.Index = Parameter->ParameterInfo.Index;
			ChildProperty->ParameterInfo.Association = Parameter->ParameterInfo.Association;
			ChildProperty->ParameterNode = Property.ParameterNode;
			ChildProperty->PropertyName = Property.UnsortedName;
			ChildProperty->NodeKey = FString::FromInt(ChildProperty->ParameterInfo.Index) + FString::FromInt(ChildProperty->ParameterInfo.Association) +  Property.ParameterGroup.GroupName.ToString() + Property.UnsortedName.ToString();


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
			for (TSharedPtr<struct FStackSortedData> GroupChild : ParentParameter->Children)
			{
				if (GroupChild->Group.GroupName == Property.ParameterGroup.GroupName
					&& GroupChild->ParameterInfo.Association == ChildProperty->ParameterInfo.Association
					&&  GroupChild->ParameterInfo.Index == ChildProperty->ParameterInfo.Index)
				{
					GroupChild->Children.Add(ChildProperty);
				}
			}

		}
	}
}

void SMaterialLayersFunctionsInstanceWrapper::Refresh()
{
	LayerParameter = nullptr;
	TSharedPtr<SHorizontalBox> HeaderBox;
	NestedTree->CreateGroupsWidget();
	LayerParameter = NestedTree->FunctionParameter;
	FOnClicked 	OnChildButtonClicked = FOnClicked::CreateStatic(&FMaterialPropertyHelpers::OnClickedSaveNewMaterialInstance, Cast<UMaterialInterface>(MaterialEditorInstance->SourceInstance), Cast<UObject>(MaterialEditorInstance));
	FOnClicked	OnSiblingButtonClicked = FOnClicked::CreateStatic(&FMaterialPropertyHelpers::OnClickedSaveNewMaterialInstance, MaterialEditorInstance->SourceInstance->Parent, Cast<UObject>(MaterialEditorInstance));

	if (LayerParameter != nullptr)
	{
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
					PropertyCustomizationHelpers::MakeAddButton(FSimpleDelegate::CreateSP(NestedTree.Get(), &SMaterialLayersFunctionsInstanceTree::AddLayer))
				];
		}
		HeaderBox->AddSlot()
			.FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			];
		HeaderBox->AddSlot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.HAlign(HAlign_Right)
					.OnClicked(OnSiblingButtonClicked)
					.Text(LOCTEXT("SaveToSiblingInstance", "Save To Sibling Instance"))
				];
		HeaderBox->AddSlot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.HAlign(HAlign_Right)
				.OnClicked(OnChildButtonClicked)
				.Text(LOCTEXT("SaveToChildInstance", "Save To Child Instance"))
			];
	}
	else
	{
		this->ChildSlot
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("MaterialInstanceEditor.StackBody"))
				.Padding(FMargin(4.0f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AddLayerParameterPrompt", "Add a Material Attribute Layers parameter to see it here."))
				]
			];
	}
}


void SMaterialLayersFunctionsInstanceWrapper::Construct(const FArguments& InArgs)
{
	NestedTree = SNew(SMaterialLayersFunctionsInstanceTree)
		.InMaterialEditorInstance(InArgs._InMaterialEditorInstance)
		.InWrapper(this);

	LayerParameter = NestedTree->FunctionParameter;
	if (LayerParameter != nullptr)
	{
		IsParamChecked = TAttribute<ECheckBoxState>::Create(TAttribute<ECheckBoxState>::FGetter::CreateStatic(&FMaterialPropertyHelpers::IsOverriddenExpressionCheckbox, LayerParameter));
	}
	MaterialEditorInstance = InArgs._InMaterialEditorInstance;
	FEditorSupportDelegates::UpdateUI.AddSP(this, &SMaterialLayersFunctionsInstanceWrapper::Refresh);

}

void SMaterialLayersFunctionsInstanceWrapper::SetEditorInstance(UMaterialEditorInstanceConstant* InMaterialEditorInstance)
{
	NestedTree->MaterialEditorInstance = InMaterialEditorInstance;
	NestedTree->CreateGroupsWidget();
	Refresh();
}

/////////////// MATERIAL VERSION


class SMaterialLayersFunctionsMaterialTreeItem : public STableRow< TSharedPtr<FStackSortedData> >
{
public:

	SLATE_BEGIN_ARGS(SMaterialLayersFunctionsMaterialTreeItem)
		: _StackParameterData(nullptr),
		_MaterialEditorInstance(nullptr)
	{}

	/** The item content. */
	SLATE_ARGUMENT(TSharedPtr<FStackSortedData>, StackParameterData)
	SLATE_ARGUMENT(UMaterialEditorPreviewParameters*, MaterialEditorInstance)
	SLATE_ARGUMENT(SMaterialLayersFunctionsMaterialTree*, InTree)
	SLATE_END_ARGS()

	FMaterialTreeColumnSizeData ColumnSizeData;

	/**
	* Construct the widget
	*
	* @param InArgs   A declaration from which to construct the widget
	*/
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		// todo: add tooltips
		StackParameterData = InArgs._StackParameterData;
		MaterialEditorInstance = InArgs._MaterialEditorInstance;
		SMaterialLayersFunctionsMaterialTree* Tree = InArgs._InTree;
		ColumnSizeData.LeftColumnWidth = TAttribute<float>(Tree, &SMaterialLayersFunctionsMaterialTree::OnGetLeftColumnWidth);
		ColumnSizeData.RightColumnWidth = TAttribute<float>(Tree, &SMaterialLayersFunctionsMaterialTree::OnGetRightColumnWidth);
		ColumnSizeData.OnWidthChanged = SSplitter::FOnSlotResized::CreateSP(Tree, &SMaterialLayersFunctionsMaterialTree::OnSetColumnWidth);

		TSharedRef<SWidget> LeftSideWidget = SNullWidget::NullWidget;
		TSharedRef<SWidget> RightSideWidget = SNullWidget::NullWidget;
		FText NameOverride;
		TSharedRef<SVerticalBox> WrapperWidget = SNew(SVerticalBox);


	// STACK ---------------------------------------------------------------------
		if (StackParameterData->StackDataType == EStackDataType::Stack)
		{
			WrapperWidget->AddSlot()
				.Padding(1.0f)
				.AutoHeight()
				[
					SNullWidget::NullWidget
				];
#if WITH_EDITOR
			NameOverride = Tree->FunctionInstance->GetLayerName(StackParameterData->ParameterInfo.Index);
#endif
			TSharedRef<SHorizontalBox> HeaderRowWidget = SNew(SHorizontalBox);
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
			LeftSideWidget = HeaderRowWidget;
		}
	// END STACK
	
	// GROUP ---------------------------------------------------------------------
		if (StackParameterData->StackDataType == EStackDataType::Group)
		{
			NameOverride = FText::FromName(StackParameterData->Group.GroupName);
			LeftSideWidget = SNew(STextBlock)
				.Text(NameOverride)
				.TextStyle(FEditorStyle::Get(), "TinyText");
		}
	// END GROUP

	// ASSET ---------------------------------------------------------------------
		if (StackParameterData->StackDataType == EStackDataType::Asset)
		{
			TSharedRef<STextBlock> ParentTextBlock = SNew(STextBlock)
				.Text(LOCTEXT("Parent", "Parent "))
				.TextStyle(FEditorStyle::Get(), "TinyText");
			ParentTextBlock->SetToolTipText(LOCTEXT("ParentTooltip", "This allows you to set the parent class of your layer or blend asset to filter the possible assets to use."));
			ParentTextBlock->EnableToolTipForceField(true);

			if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::LayerParameter)
			{
				NameOverride = FMaterialPropertyHelpers::LayerID;
				StackParameterData->ParameterHandle->GetProperty()->SetMetaData(FName(TEXT("DisplayThumbnail")), TEXT("true"));
			}
			else if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::BlendParameter)
			{
				NameOverride = FMaterialPropertyHelpers::BlendID;
			}

			IDetailTreeNode& Node = *StackParameterData->ParameterNode;
			FNodeWidgets NodeWidgets = Node.CreateNodeWidgets();

			LeftSideWidget = StackParameterData->ParameterHandle->CreatePropertyNameWidget(NameOverride);

			StackParameterData->ParameterHandle->MarkResetToDefaultCustomized(false);

			FString FilterPath;
			if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::BlendParameter && Tree->FunctionInstance->FilterBlends.IsValidIndex(StackParameterData->ParameterInfo.Index))
			{
				FilterPath = Tree->FunctionInstance->FilterBlends[StackParameterData->ParameterInfo.Index]->GetPathName();
			}
			else if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::LayerParameter && Tree->FunctionInstance->FilterLayers.IsValidIndex(StackParameterData->ParameterInfo.Index))
			{
				FilterPath = Tree->FunctionInstance->FilterLayers[StackParameterData->ParameterInfo.Index]->GetPathName();
			}

			FString InstancePath;
			if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::BlendParameter && Tree->FunctionInstance->InstanceBlends.IsValidIndex(StackParameterData->ParameterInfo.Index))
			{
				InstancePath = Tree->FunctionInstance->InstanceBlends[StackParameterData->ParameterInfo.Index]->GetPathName();
			}
			else if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::LayerParameter && Tree->FunctionInstance->InstanceLayers.IsValidIndex(StackParameterData->ParameterInfo.Index))
			{
				InstancePath = Tree->FunctionInstance->InstanceLayers[StackParameterData->ParameterInfo.Index]->GetPathName();
			}

			EMaterialParameterAssociation InAssociation = StackParameterData->ParameterInfo.Association;

			RightSideWidget = SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						ParentTextBlock
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SObjectPropertyEntryBox)
						.AllowedClass(UMaterialFunction::StaticClass())
						.ObjectPath(FilterPath)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UMaterialFunctionInterface::StaticClass())
					.ObjectPath(InstancePath)
					.ThumbnailPool(Tree->GetTreeThumbnailPool())
					.DisplayCompactSize(true)
				];
		}
	// END ASSET

	// PROPERTY ------------------------------------------------------------------
		if (StackParameterData->StackDataType == EStackDataType::Property)
		{
			UMaterialFunctionInterface* OwningInterface = nullptr;
			if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::LayerParameter)
			{
				OwningInterface = Tree->FunctionInstance->Layers[StackParameterData->ParameterInfo.Index];
			}
			if (StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::BlendParameter)
			{
				OwningInterface = Tree->FunctionInstance->Blends[StackParameterData->ParameterInfo.Index];
			}

			NameOverride = FText::FromName(StackParameterData->Parameter->ParameterInfo.Name);

			IDetailTreeNode& Node = *StackParameterData->ParameterNode;
			TSharedPtr<IDetailPropertyRow> GeneratedRow = StaticCastSharedPtr<IDetailPropertyRow>(Node.GetRow());
			IDetailPropertyRow& Row = *GeneratedRow.Get();
			Row
				.DisplayName(NameOverride)
				.EditCondition(false, nullptr);


			UDEditorStaticComponentMaskParameterValue* CompMaskParam = Cast<UDEditorStaticComponentMaskParameterValue>(StackParameterData->Parameter);
			UDEditorVectorParameterValue* VectorParam = Cast<UDEditorVectorParameterValue>(StackParameterData->Parameter);

			if (VectorParam && VectorParam->bIsUsedAsChannelMask)
			{
				FOnGetPropertyComboBoxStrings GetMaskStrings = FOnGetPropertyComboBoxStrings::CreateStatic(&FMaterialPropertyHelpers::GetVectorChannelMaskComboBoxStrings);
				FOnGetPropertyComboBoxValue GetMaskValue = FOnGetPropertyComboBoxValue::CreateStatic(&FMaterialPropertyHelpers::GetVectorChannelMaskValue, StackParameterData->Parameter);
				FOnPropertyComboBoxValueSelected SetMaskValue = FOnPropertyComboBoxValueSelected::CreateStatic(&FMaterialPropertyHelpers::SetVectorChannelMaskValue, StackParameterData->ParameterNode->CreatePropertyHandle(), StackParameterData->Parameter, (UObject*)MaterialEditorInstance);

				FDetailWidgetRow& CustomWidget = Row.CustomWidget();
				CustomWidget
				.FilterString(NameOverride)
				.NameContent()
				[
					SNew(STextBlock)
					.Text(NameOverride)
					.ToolTipText(FMaterialPropertyHelpers::GetParameterExpressionDescription(StackParameterData->Parameter, MaterialEditorInstance))
					.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
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
							PropertyCustomizationHelpers::MakePropertyComboBox(StackParameterData->ParameterNode->CreatePropertyHandle(), GetMaskStrings, GetMaskValue, SetMaskValue)
						]
					]
				];
			}
			else if (!CompMaskParam)
			{
				FNodeWidgets StoredNodeWidgets = Node.CreateNodeWidgets();
				TSharedRef<SWidget> StoredRightSideWidget = StoredNodeWidgets.ValueWidget.ToSharedRef();
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
	// END PROPERTY

	// PROPERTY CHILD ------------------------------------------------------------
		if (StackParameterData->StackDataType == EStackDataType::PropertyChild)
		{
			FNodeWidgets NodeWidgets = StackParameterData->ParameterNode->CreateNodeWidgets();
			LeftSideWidget = NodeWidgets.NameWidget.ToSharedRef();
			RightSideWidget = NodeWidgets.ValueWidget.ToSharedRef();
		}
	// END PROPERTY CHILD

		RightSideWidget->SetEnabled(false);

		// Final wrapper
		if (StackParameterData->StackDataType == EStackDataType::Stack)
		{
			WrapperWidget->AddSlot()
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("MaterialInstanceEditor.StackHeader"))
					.Padding(0.0f)
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
		else
		{
			if (StackParameterData->StackDataType == EStackDataType::Asset && StackParameterData->ParameterInfo.Association == EMaterialParameterAssociation::BlendParameter)
			{
				WrapperWidget->AddSlot()
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("MaterialInstanceEditor.StackBody"))
						.Padding(FMargin(5.0f, 2.0f))
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("ThinLine.Horizontal"))
						.ColorAndOpacity(FLinearColor(.2f, .2f, .2f, 1.0f))
					]
					];
			}
			WrapperWidget->AddSlot()
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("MaterialInstanceEditor.StackBody"))
					.Padding(0.0f)
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

	// Block double click expansion
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override { return FReply::Handled(); };

	/** The node info to build the tree view row from. */
	TSharedPtr<FStackSortedData> StackParameterData;

	UMaterialEditorPreviewParameters* MaterialEditorInstance;
};

void SMaterialLayersFunctionsMaterialWrapper::Refresh()
{
	LayerParameter = nullptr;
	NestedTree->CreateGroupsWidget();
	LayerParameter = NestedTree->FunctionParameter;
	FOnClicked 	OnChildButtonClicked = FOnClicked::CreateStatic(&FMaterialPropertyHelpers::OnClickedSaveNewMaterialInstance, Cast<UMaterialInterface>(MaterialEditorInstance->OriginalMaterial), Cast<UObject>(MaterialEditorInstance));

	if (LayerParameter != nullptr)
	{
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
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[					
						SNew(STextBlock)
						.Text(FText::FromName(NestedTree->LayersFunctionsParameterName))
						.TextStyle(FEditorStyle::Get(), "LargeText")
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNullWidget::NullWidget
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2.0f)
					[
						SNew(SButton)
						.HAlign(HAlign_Right)
						.OnClicked(OnChildButtonClicked)
						.Text(LOCTEXT("SaveToChildInstance", "Save To Child Instance"))
					]
				]
				+ SVerticalBox::Slot()
				.Padding(FMargin(3.0f, 0.0f))
				[
					NestedTree.ToSharedRef()
				]
			]
		];
	}
	else if(MaterialEditorInstance->OriginalFunction)
	{
		this->ChildSlot
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("MaterialInstanceEditor.StackBody"))
				.Padding(FMargin(4.0f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NoMaterialAttributeLayersAllowed", "Material Functions, Layers, and Blends cannot contain Material Attribute Layers nodes."))
					.AutoWrapText(true)
				]
			];
	}
	else
	{
		this->ChildSlot
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("MaterialInstanceEditor.StackBody"))
				.Padding(FMargin(4.0f))
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AddMaterialLayerParameterPrompt", "Add a Material Attribute Layers parameter to see it here."))
				.AutoWrapText(true)
			]
			];
	}
}


void SMaterialLayersFunctionsMaterialWrapper::Construct(const FArguments& InArgs)
{
	NestedTree = SNew(SMaterialLayersFunctionsMaterialTree)
		.InMaterialEditorInstance(InArgs._InMaterialEditorInstance);

	LayerParameter = NestedTree->FunctionParameter;
	MaterialEditorInstance = InArgs._InMaterialEditorInstance;
	FEditorSupportDelegates::UpdateUI.AddSP(this, &SMaterialLayersFunctionsMaterialWrapper::Refresh);

}

void SMaterialLayersFunctionsMaterialWrapper::SetEditorInstance(UMaterialEditorPreviewParameters* InMaterialEditorInstance)
{
	NestedTree->MaterialEditorInstance = InMaterialEditorInstance;
	NestedTree->CreateGroupsWidget();
	Refresh();
}


void SMaterialLayersFunctionsMaterialTree::Construct(const FArguments& InArgs)
{
	ColumnWidth = 0.5f;
	MaterialEditorInstance = InArgs._InMaterialEditorInstance;

	CreateGroupsWidget();

	STreeView<TSharedPtr<FStackSortedData>>::Construct(
		STreeView::FArguments()
		.TreeItemsSource(&LayerProperties)
		.SelectionMode(ESelectionMode::None)
		.OnGenerateRow(this, &SMaterialLayersFunctionsMaterialTree::OnGenerateRowMaterialLayersFunctionsTreeView)
		.OnGetChildren(this, &SMaterialLayersFunctionsMaterialTree::OnGetChildrenMaterialLayersFunctionsTreeView)
		.OnExpansionChanged(this, &SMaterialLayersFunctionsMaterialTree::OnExpansionChanged)
	);

	SetParentsExpansionState();
}

TSharedRef< ITableRow > SMaterialLayersFunctionsMaterialTree::OnGenerateRowMaterialLayersFunctionsTreeView(TSharedPtr<FStackSortedData> Item, const TSharedRef< STableViewBase >& OwnerTable)
{
	TSharedRef< SMaterialLayersFunctionsMaterialTreeItem > ReturnRow = SNew(SMaterialLayersFunctionsMaterialTreeItem, OwnerTable)
		.StackParameterData(Item)
		.MaterialEditorInstance(MaterialEditorInstance)
		.InTree(this);
	return ReturnRow;
}

void SMaterialLayersFunctionsMaterialTree::OnGetChildrenMaterialLayersFunctionsTreeView(TSharedPtr<FStackSortedData> InParent, TArray< TSharedPtr<FStackSortedData> >& OutChildren)
{
	OutChildren = InParent->Children;
}

void SMaterialLayersFunctionsMaterialTree::OnExpansionChanged(TSharedPtr<FStackSortedData> Item, bool bIsExpanded)
{
	bool* ExpansionValue = MaterialEditorInstance->OriginalMaterial->LayerParameterExpansion.Find(Item->NodeKey);
	if (ExpansionValue == nullptr)
	{
		MaterialEditorInstance->OriginalMaterial->LayerParameterExpansion.Add(Item->NodeKey, bIsExpanded);
	}
	else if (*ExpansionValue != bIsExpanded)
	{
		MaterialEditorInstance->OriginalMaterial->LayerParameterExpansion.Emplace(Item->NodeKey, bIsExpanded);
	}
	// Expand any children that are also expanded
	for (auto Child : Item->Children)
	{
		bool* ChildExpansionValue = MaterialEditorInstance->OriginalMaterial->LayerParameterExpansion.Find(Child->NodeKey);
		if (ChildExpansionValue != nullptr && *ChildExpansionValue == true)
		{
			SetItemExpansion(Child, true);
		}
	}
}

void SMaterialLayersFunctionsMaterialTree::SetParentsExpansionState()
{
	for (const auto& Pair : LayerProperties)
	{
		if (Pair->Children.Num())
		{
			bool* bIsExpanded = MaterialEditorInstance->OriginalMaterial->LayerParameterExpansion.Find(Pair->NodeKey);
			if (bIsExpanded)
			{
				SetItemExpansion(Pair, *bIsExpanded);
			}
		}
	}
}

TSharedPtr<class FAssetThumbnailPool> SMaterialLayersFunctionsMaterialTree::GetTreeThumbnailPool()
{
	return Generator->GetGeneratedThumbnailPool();
}

void SMaterialLayersFunctionsMaterialTree::CreateGroupsWidget()
{
	check(MaterialEditorInstance);
	FunctionParameter = nullptr;
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
				StackProperty->NodeKey = FString::FromInt(StackProperty->ParameterInfo.Index);


				TSharedPtr<FStackSortedData> ChildProperty(new FStackSortedData());
				ChildProperty->StackDataType = EStackDataType::Asset;
				ChildProperty->Parameter = Parameter;
				ChildProperty->ParameterHandle = LayerHandle->AsArray()->GetElement(LayerChildren - 1);
				ChildProperty->ParameterNode = Generator->FindTreeNode(ChildProperty->ParameterHandle);
				ChildProperty->ParameterInfo.Index = LayerChildren - 1;
				ChildProperty->ParameterInfo.Association = EMaterialParameterAssociation::LayerParameter;
				ChildProperty->NodeKey = FString::FromInt(ChildProperty->ParameterInfo.Index) + FString::FromInt(ChildProperty->ParameterInfo.Association);

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
						ChildProperty->NodeKey = FString::FromInt(ChildProperty->ParameterInfo.Index) + FString::FromInt(ChildProperty->ParameterInfo.Association);
						LayerProperties.Last()->Children.Add(ChildProperty);

						StackProperty = MakeShareable(new FStackSortedData());
						StackProperty->StackDataType = EStackDataType::Stack;
						StackProperty->Parameter = Parameter;
						StackProperty->ParameterInfo.Index = Counter;
						StackProperty->NodeKey = FString::FromInt(StackProperty->ParameterInfo.Index);
						LayerProperties.Add(StackProperty);

						ChildProperty = MakeShareable(new FStackSortedData());
						ChildProperty->StackDataType = EStackDataType::Asset;
						ChildProperty->Parameter = Parameter;
						ChildProperty->ParameterHandle = LayerHandle->AsArray()->GetElement(Counter);
						ChildProperty->ParameterNode = Generator->FindTreeNode(ChildProperty->ParameterHandle);
						ChildProperty->ParameterInfo.Index = Counter;
						ChildProperty->ParameterInfo.Association = EMaterialParameterAssociation::LayerParameter;
						ChildProperty->NodeKey = FString::FromInt(ChildProperty->ParameterInfo.Index) + FString::FromInt(ChildProperty->ParameterInfo.Association);

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

void SMaterialLayersFunctionsMaterialTree::ShowSubParameters(TSharedPtr<FStackSortedData> ParentParameter)
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
			GroupProperty->NodeKey = FString::FromInt(GroupProperty->ParameterInfo.Index) + FString::FromInt(GroupProperty->ParameterInfo.Association) + Property.ParameterGroup.GroupName.ToString();

			bool bAddNewGroup = true;
			for (TSharedPtr<struct FStackSortedData> GroupChild : ParentParameter->Children)
			{
				if (GroupChild->NodeKey == GroupProperty->NodeKey)
				{
					bAddNewGroup = false;
				}
			}
			if (bAddNewGroup)
			{
				ParentParameter->Children.Add(GroupProperty);
			}
			TSharedPtr<FStackSortedData> ChildProperty(new FStackSortedData());
			ChildProperty->StackDataType = EStackDataType::Property;
			ChildProperty->Parameter = Parameter;
			ChildProperty->ParameterInfo.Index = Parameter->ParameterInfo.Index;
			ChildProperty->ParameterInfo.Association = Parameter->ParameterInfo.Association;
			ChildProperty->ParameterNode = Property.ParameterNode;
			ChildProperty->PropertyName = Property.UnsortedName;
			ChildProperty->NodeKey = FString::FromInt(ChildProperty->ParameterInfo.Index) + FString::FromInt(ChildProperty->ParameterInfo.Association) + Property.ParameterGroup.GroupName.ToString() + Property.UnsortedName.ToString();


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

			for (TSharedPtr<struct FStackSortedData> GroupChild : ParentParameter->Children)
			{
				if (GroupChild->Group.GroupName == Property.ParameterGroup.GroupName
					&& GroupChild->ParameterInfo.Association == ChildProperty->ParameterInfo.Association
					&&  GroupChild->ParameterInfo.Index == ChildProperty->ParameterInfo.Index)
				{
					GroupChild->Children.Add(ChildProperty);
				}
			}
		}
	}
}


#undef LOCTEXT_NAMESPACE