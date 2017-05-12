// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SClothPaintWidget.h"

#include "ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "SScrollBox.h"
#include "SBoxPanel.h"

#include "ClothPaintSettingsCustomization.h"
#include "MeshPaintSettings.h"
#include "ClothPainter.h"
#include "ClothPaintSettings.h"
#include "SComboBox.h"
#include "ClothingAsset.h"
#include "SUniformGridPanel.h"
#include "SInlineEditableTextBlock.h"
#include "MultiBoxBuilder.h"
#include "SButton.h"
#include "ClothPaintToolBase.h"

#define LOCTEXT_NAMESPACE "ClothPaintWidget"

class SMaskListRow : public SMultiColumnTableRow<TSharedPtr<FClothingMaskListItem>>
{
public:

	SLATE_BEGIN_ARGS(SMaskListRow)
	{}

	SLATE_EVENT(FSimpleDelegate, OnInvalidateList)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<FClothingMaskListItem> InItem)
	{
		OnInvalidateList = InArgs._OnInvalidateList;
		Item = InItem;

		SMultiColumnTableRow<TSharedPtr<FClothingMaskListItem>>::Construct(FSuperRowType::FArguments(), InOwnerTable);
	}

	static FName Column_MaskName;
	static FName Column_CurrentTarget;

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		if(InColumnName == Column_MaskName)
		{
			return SAssignNew(InlineText, SInlineEditableTextBlock)
				.Text(this, &SMaskListRow::GetMaskName)
				.OnTextCommitted(this, &SMaskListRow::OnCommitMaskName)
				.IsSelected(this, &SMultiColumnTableRow<TSharedPtr<FClothingMaskListItem>>::IsSelectedExclusively);
		}

		if(InColumnName == Column_CurrentTarget)
		{
			FClothParameterMask_PhysMesh* Mask = Item->GetMask();
			UEnum* Enum = FindObject<UEnum>(ANY_PACKAGE, TEXT(PREPROCESSOR_TO_STRING(MaskTarget_PhysMesh)), true);
			if(Enum && Mask)
			{
				return SNew(STextBlock).Text(Enum->GetDisplayNameTextByIndex((int32)Mask->CurrentTarget));
			}
		}

		return SNullWidget::NullWidget;
	}

	FText GetMaskName() const
	{
		if(Item.IsValid())
		{
			if(FClothParameterMask_PhysMesh* Mask = Item->GetMask())
			{
				return FText::FromName(Mask->MaskName);
			}
		}

		return LOCTEXT("MaskName_Invalid", "Invalid Mask");
	}

	void OnCommitMaskName(const FText& InText, ETextCommit::Type CommitInfo)
	{
		if(Item.IsValid())
		{
			if(FClothParameterMask_PhysMesh* Mask = Item->GetMask())
			{
				FText TrimText = FText::TrimPrecedingAndTrailing(InText);
				Mask->MaskName = FName(*TrimText.ToString());
			}
		}
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		// Spawn menu
		if(MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && Item.IsValid())
		{
			if(FClothParameterMask_PhysMesh* Mask = Item->GetMask())
			{
				FMenuBuilder Builder(true, nullptr);

				FUIAction MoveUpAction(FExecuteAction::CreateSP(this, &SMaskListRow::OnMoveUp), FCanExecuteAction::CreateSP(this, &SMaskListRow::CanMoveUp));
				FUIAction MoveDownAction(FExecuteAction::CreateSP(this, &SMaskListRow::OnMoveDown), FCanExecuteAction::CreateSP(this, &SMaskListRow::CanMoveDown));
				FUIAction DeleteAction(FExecuteAction::CreateSP(this, &SMaskListRow::OnDeleteMask));
				FUIAction ApplyAction(FExecuteAction::CreateSP(this, &SMaskListRow::OnApply));

				Builder.BeginSection(NAME_None, LOCTEXT("MaskActions_SectionName", "Actions"));
				{
					Builder.AddSubMenu(LOCTEXT("MaskActions_SetTarget", "Set Target"), LOCTEXT("MaskActions_SetTarget_Tooltip", "Choose the target for this mask"), FNewMenuDelegate::CreateSP(this, &SMaskListRow::BuildTargetSubmenu));
					Builder.AddMenuEntry(LOCTEXT("MaskActions_MoveUp", "Move Up"), LOCTEXT("MaskActions_MoveUp_Tooltip", "Move the mask up in the list"), FSlateIcon(), MoveUpAction);
					Builder.AddMenuEntry(LOCTEXT("MaskActions_MoveDown", "Move Down"), LOCTEXT("MaskActions_MoveDownp_Tooltip", "Move the mask down in the list"), FSlateIcon(), MoveDownAction);
					Builder.AddMenuEntry(LOCTEXT("MaskActions_Delete", "Delete"), LOCTEXT("MaskActions_Delete_Tooltip", "Delete the mask"), FSlateIcon(), DeleteAction);
					Builder.AddMenuEntry(LOCTEXT("MaskActions_Apply", "Apply"), LOCTEXT("MaskActions_Apply_Tooltip", "Apply the mask to the physical mesh"), FSlateIcon(), ApplyAction);
				}
				Builder.EndSection();

				FWidgetPath Path = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();

				FSlateApplication::Get().PushMenu(AsShared(), Path, Builder.MakeWidget(), MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect::ContextMenu);

				return FReply::Handled();
			}
		}

		return SMultiColumnTableRow<TSharedPtr<FClothingMaskListItem>>::OnMouseButtonUp(MyGeometry, MouseEvent);
	}

	void EditName()
	{
		if(InlineText.IsValid())
		{
			InlineText->EnterEditingMode();
		}
	}

private:

	FClothLODData* GetCurrentLod() const
	{
		if(Item.IsValid())
		{
			if(UClothingAsset* Asset = Item->ClothingAsset.Get())
			{
				if(Asset->LodData.IsValidIndex(Item->LodIndex))
				{
					FClothLODData& LodData = Asset->LodData[Item->LodIndex];

					return &LodData;
				}
			}
		}

		return nullptr;
	}

	void OnDeleteMask()
	{
		if(FClothLODData* LodData = GetCurrentLod())
		{
			if(LodData->ParameterMasks.IsValidIndex(Item->MaskIndex))
			{
				LodData->ParameterMasks.RemoveAt(Item->MaskIndex);
				OnInvalidateList.ExecuteIfBound();
			}
		}
	}

	void OnMoveUp()
	{
		if(Item->MaskIndex > 0)
		{
			if(FClothLODData* LodData = GetCurrentLod())
			{
				if(LodData->ParameterMasks.IsValidIndex(Item->MaskIndex))
				{
					LodData->ParameterMasks.Swap(Item->MaskIndex, Item->MaskIndex - 1);
					OnInvalidateList.ExecuteIfBound();
				}
			}
		}
	}

	bool CanMoveUp() const
	{
		return(Item.IsValid() && Item->MaskIndex > 0);
	}

	void OnMoveDown()
	{
		if(FClothLODData* LodData = GetCurrentLod())
		{
			if(LodData->ParameterMasks.IsValidIndex(Item->MaskIndex + 1))
			{
				LodData->ParameterMasks.Swap(Item->MaskIndex, Item->MaskIndex + 1);
				OnInvalidateList.ExecuteIfBound();
			}
		}
	}

	bool CanMoveDown() const
	{
		if(Item.IsValid())
		{
			if(FClothLODData* LodData = GetCurrentLod())
			{
				return LodData->ParameterMasks.IsValidIndex(Item->MaskIndex + 1);
			}
		}

		return false;
	}

	void OnApply()
	{
		if(FClothParameterMask_PhysMesh* Mask = Item->GetMask())
		{
			if(FClothLODData* LodData = GetCurrentLod())
			{
				Mask->Apply(LodData->PhysicalMeshData);
			}
		}
	}

	void OnSetTarget(int32 InTargetEntryIndex)
	{
		if(Item.IsValid())
		{
			if(FClothParameterMask_PhysMesh* Mask = Item->GetMask())
			{
				Mask->CurrentTarget = (MaskTarget_PhysMesh)InTargetEntryIndex;

				OnInvalidateList.ExecuteIfBound();
			}
		}
	}

	void BuildTargetSubmenu(FMenuBuilder& Builder)
	{
		Builder.BeginSection(NAME_None, LOCTEXT("MaskTargets_SectionName", "Targets"));
		{
			UEnum* Enum = FindObject<UEnum>(ANY_PACKAGE, TEXT(PREPROCESSOR_TO_STRING(MaskTarget_PhysMesh)), true);
			if(Enum)
			{
				const int32 NumEntries = Enum->NumEnums();

				// Iterate to -1 to skip the _MAX entry appended to the end of the enum
				for(int32 Index = 0; Index < NumEntries - 1; ++Index)
				{
					FUIAction EntryAction(FExecuteAction::CreateSP(this, &SMaskListRow::OnSetTarget, Index));

					FText EntryText = Enum->GetDisplayNameTextByIndex(Index);

					Builder.AddMenuEntry(EntryText, FText::GetEmpty(), FSlateIcon(), EntryAction);
				}
			}
		}
		Builder.EndSection();
	}

	FSimpleDelegate OnInvalidateList;
	TSharedPtr<FClothingMaskListItem> Item;
	TSharedPtr<SInlineEditableTextBlock> InlineText;
};
FName SMaskListRow::Column_MaskName(TEXT("MaskName"));
FName SMaskListRow::Column_CurrentTarget(TEXT("CurrentTarget"));

void SClothPaintWidget::Construct(const FArguments& InArgs, FClothPainter* InPainter)
{
	Painter = InPainter;

	Objects.Add(Painter->GetBrushSettings());
	Objects.Add(Painter->GetPainterSettings());

	UObject* ToolSettings = Painter->GetSelectedTool()->GetSettingsObject();
	if(ToolSettings)
	{
		Objects.Add(ToolSettings);
	}

	ClothPainterSettings = Cast<UClothPainterSettings>(InPainter->GetPainterSettings());
	CreateDetailsView(InPainter);

	RefreshClothingAssetListItems();
	RefreshClothingLodItems();
	RefreshMaskListItems();

	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SBorder)
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 2.0f))
			.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(3.0f)
				.AutoHeight()
				[
					SNew(SBorder)
					.Padding(0)
					.BorderImage(FEditorStyle::GetBrush("Docking.Tab.ContentAreaBrush"))
					[
						SNew(SBox)
						.MinDesiredHeight(100.0f)
						.MaxDesiredHeight(100.0f)
						[
							SAssignNew(AssetList, SAssetList)
							.ItemHeight(24)
							.ListItemsSource(&AssetListItems)
							.OnGenerateRow(this, &SClothPaintWidget::OnGenerateWidgetForClothingAssetItem)
							.OnSelectionChanged(this, &SClothPaintWidget::OnAssetListSelectionChanged)
							.HeaderRow
							(
								SNew(SHeaderRow)
								+ SHeaderRow::Column(TEXT("Asset"))
								.DefaultLabel(LOCTEXT("AssetListHeader", "Assets"))
							)
						]
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(3.0f, 3.0f, 0.0f, 3.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ClothPanelCurrentLod", "Asset LOD: "))
					]

					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					[
						SNew(SComboBox<TSharedPtr<FClothingAssetLodItem>>)
						.OnGenerateWidget(this, &SClothPaintWidget::OnGenerateWidgetForLodItem)
						.OnSelectionChanged(this, &SClothPaintWidget::OnClothingLodChanged)
						.OptionsSource(&LodListItems)
						.Content()
						[
							SNew(STextBlock)
							.Text(this, &SClothPaintWidget::GetLodDropdownText)
						]
					]
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.Padding(FMargin(0.0f, 0.0f, 3.0f, 0.0f))
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("MaskButton_New", "New Mask"))
						.OnClicked(this, &SClothPaintWidget::AddNewMask)
						.IsEnabled(this, &SClothPaintWidget::CanAddNewMask)
					]
				]

				+ SVerticalBox::Slot()
				.Padding(3.0f)
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.FillWidth(0.8f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						[
							SNew(SBorder)
							.Padding(0)
							.BorderImage(FEditorStyle::GetBrush("Docking.Tab.ContentAreaBrush"))
							[
								SNew(SBox)
								.MinDesiredHeight(100.0f)
								.MaxDesiredHeight(200.0f)
								[
									SAssignNew(MaskList, SMaskList)
									.ItemHeight(24)
									.ListItemsSource(&MaskListItems)
									.OnGenerateRow(this, &SClothPaintWidget::OnGenerateWidgetForMaskItem)
									.OnSelectionChanged(this, &SClothPaintWidget::OnMaskSelectionChanged)
									.HeaderRow
									(
										SNew(SHeaderRow)
										+ SHeaderRow::Column(SMaskListRow::Column_MaskName)
										.FillWidth(0.6f)
										.DefaultLabel(LOCTEXT("MaskListHeader_Name", "Mask"))
										+ SHeaderRow::Column(SMaskListRow::Column_CurrentTarget)
										.FillWidth(0.4f)
										.DefaultLabel(LOCTEXT("MaskListHeader_Target", "Target"))
									)
								]
							]
						]
					]
				]
			]
		]
		+ SScrollBox::Slot()
		.Padding(FMargin(0.0f, 3.0f, 0.0f, 0.0f))
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Content()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0)
				[
					DetailsView->AsShared()
				]
			]
		]
	];
}

void SClothPaintWidget::CreateDetailsView(FClothPainter* InPainter)
{
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs(
		/*bUpdateFromSelection=*/ false,
		/*bLockable=*/ false,
		/*bAllowSearch=*/ false,
		FDetailsViewArgs::HideNameArea,
		/*bHideSelectionTip=*/ true,
		/*InNotifyHook=*/ nullptr,
		/*InSearchInitialKeyFocus=*/ false,
		/*InViewIdentifier=*/ NAME_None);
	DetailsViewArgs.DefaultsOnlyVisibility = FDetailsViewArgs::EEditDefaultsOnlyNodeVisibility::Automatic;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowMultipleTopLevelObjects = true;
	
	DetailsView = EditModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetRootObjectCustomizationInstance(MakeShareable(new FClothPaintSettingsRootObjectCustomization));
	DetailsView->RegisterInstancedCustomPropertyLayout(UClothPainterSettings::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FClothPaintSettingsCustomization::MakeInstance, InPainter));
	DetailsView->SetObjects(Objects, true);
}

void SClothPaintWidget::OnRefresh()
{
	RefreshClothingAssetListItems();
	RefreshClothingLodItems();
	RefreshMaskListItems();

	if(AssetList.IsValid())
	{
		AssetList->RequestListRefresh();
	}

	if(MaskList.IsValid())
	{
		MaskList->RequestListRefresh();
	}

	if(DetailsView.IsValid())
	{
		Objects.Reset();
		Objects.Add(Painter->GetBrushSettings());
		Objects.Add(Painter->GetPainterSettings());

		UObject* ToolSettings = Painter->GetSelectedTool()->GetSettingsObject();
		if(ToolSettings)
		{
			Objects.Add(ToolSettings);
		}

		DetailsView->SetObjects(Objects, true);
	}
}

void SClothPaintWidget::RefreshClothingAssetListItems()
{
	AssetListItems.Empty();

	for(UClothingAsset* Asset : ClothPainterSettings->ClothingAssets)
	{
		TSharedPtr<FClothingAssetListItem> Entry = MakeShareable(new FClothingAssetListItem);

		Entry->ClothingAsset = Asset;

		AssetListItems.Add(Entry);
	}

	if(AssetListItems.Num() == 0)
	{
		// Add an invalid entry so we can show a "none" line
		AssetListItems.Add(MakeShareable(new FClothingAssetListItem));
	}
}

TSharedRef<ITableRow> SClothPaintWidget::OnGenerateWidgetForClothingAssetItem(TSharedPtr<FClothingAssetListItem> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	if(UClothingAsset* Asset = InItem->ClothingAsset.Get())
	{
		return SNew(STableRow<TSharedPtr<FClothingAssetListItem>>, OwnerTable)
			.Content()
			[
				SNew(STextBlock).Text(FText::FromString(*Asset->GetName()))
			];
	}

	return SNew(STableRow<TSharedPtr<FClothingAssetListItem>>, OwnerTable)
		.Content()
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("No Assets Available")))
		];
}

void SClothPaintWidget::OnAssetListSelectionChanged(TSharedPtr<FClothingAssetListItem> InSelectedItem, ESelectInfo::Type InSelectInfo)
{
	if(InSelectedItem.IsValid() && ClothPainterSettings)
	{
		SelectedAsset = InSelectedItem->ClothingAsset;

		if(UClothingAsset* NewAsset = SelectedAsset.Get())
		{
			if(NewAsset->LodData.Num() > 0)
			{
				SelectedLod = 0;
			}
			else
			{
				SelectedLod = INDEX_NONE;
			}

			ClothPainterSettings->OnAssetSelectionChanged.Broadcast(NewAsset, SelectedLod);
		}
	}

	RefreshClothingLodItems();
}

void SClothPaintWidget::RefreshClothingLodItems()
{
	LodListItems.Reset();
	if(UClothingAsset* Asset = SelectedAsset.Get())
	{
		const int32 NumLods = Asset->LodData.Num();

		for(int32 LodIndex = 0; LodIndex < NumLods; ++LodIndex)
		{
			TSharedPtr<FClothingAssetLodItem> NewItem = MakeShareable(new FClothingAssetLodItem);

			NewItem->Lod = LodIndex;

			LodListItems.Add(NewItem);
		}
	}
	else
	{
		TSharedPtr<FClothingAssetLodItem> NewItem = MakeShareable(new FClothingAssetLodItem);

		// Marker so we can still make some content
		NewItem->Lod = INDEX_NONE;

		LodListItems.Add(NewItem);
	}
}

TSharedRef<SWidget> SClothPaintWidget::OnGenerateWidgetForLodItem(TSharedPtr<FClothingAssetLodItem> InItem)
{
	if(InItem->Lod == INDEX_NONE)
	{
		return SNew(STextBlock)
			.Text(LOCTEXT("LodDropdownNoSelection", "Select an Asset..."));
	}

	return SNew(STextBlock)
		.Text(FText::AsNumber(InItem->Lod));
}

FText SClothPaintWidget::GetLodDropdownText() const
{
	if(SelectedLod != INDEX_NONE)
	{
		return FText::AsNumber(SelectedLod);
	}

	return LOCTEXT("LodDropdownContentNoSelection", "None");
}

void SClothPaintWidget::OnClothingLodChanged(TSharedPtr<FClothingAssetLodItem> InSelectedItem, ESelectInfo::Type InSelectInfo)
{
	if(SelectedAsset.IsValid())
	{
		if(InSelectedItem.IsValid())
		{
			SelectedLod = InSelectedItem->Lod;
		}
		else
		{
			SelectedLod = INDEX_NONE;
		}

		ClothPainterSettings->OnAssetSelectionChanged.Broadcast(SelectedAsset.Get(), SelectedLod);
	}
}

void SClothPaintWidget::RefreshMaskListItems()
{
	MaskListItems.Empty();

	UClothingAsset* Asset = SelectedAsset.Get();
	if(Asset && Asset->IsValidLod(SelectedLod))
	{
		FClothLODData& LodData = Asset->LodData[SelectedLod];
		const int32 NumMasks = LodData.ParameterMasks.Num();

		for(int32 Index = 0; Index < NumMasks; ++Index)
		{
			TSharedPtr<FClothingMaskListItem> NewItem = MakeShareable(new FClothingMaskListItem);
			NewItem->ClothingAsset = SelectedAsset;
			NewItem->LodIndex = SelectedLod;
			NewItem->MaskIndex = Index;
			MaskListItems.Add(NewItem);
		}
	}

	if(MaskListItems.Num() == 0)
	{
		// Add invalid entry so we can make a widget for "none"
		TSharedPtr<FClothingMaskListItem> NewItem = MakeShareable(new FClothingMaskListItem);
		MaskListItems.Add(NewItem);
	}
}

TSharedRef<ITableRow> SClothPaintWidget::OnGenerateWidgetForMaskItem(TSharedPtr<FClothingMaskListItem> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	if(FClothParameterMask_PhysMesh* Mask = InItem->GetMask())
	{
		return SNew(SMaskListRow, OwnerTable, InItem)
			.OnInvalidateList(this, &SClothPaintWidget::OnRefresh);
	}

	return SNew(STableRow<TSharedPtr<FClothingMaskListItem>>, OwnerTable)
	[
		SNew(STextBlock).Text(LOCTEXT("MaskList_NoMasks", "No masks available"))
	];
}

void SClothPaintWidget::OnMaskSelectionChanged(TSharedPtr<FClothingMaskListItem> InSelectedItem, ESelectInfo::Type InSelectInfo)
{
	// To be implemented for mask hookup to painter
}

FReply SClothPaintWidget::AddNewMask()
{
	if(UClothingAsset* Asset = SelectedAsset.Get())
	{
		if(Asset->LodData.IsValidIndex(SelectedLod))
		{
			FClothLODData& LodData = Asset->LodData[SelectedLod];
			const int32 NumRequiredValues = LodData.PhysicalMeshData.Vertices.Num();

			LodData.ParameterMasks.AddDefaulted();

			FClothParameterMask_PhysMesh& NewMask = LodData.ParameterMasks.Last();

			NewMask.MaskName = TEXT("New Mask");
			NewMask.CurrentTarget = MaskTarget_PhysMesh::None;
			NewMask.MaxValue = 0.0f;
			NewMask.Values.AddZeroed(NumRequiredValues);

			OnRefresh();
		}
	}

	return FReply::Handled();
}

bool SClothPaintWidget::CanAddNewMask() const
{
	return SelectedAsset.Get() != nullptr;
}

FClothParameterMask_PhysMesh* FClothingMaskListItem::GetMask()
{
	if(UClothingAsset* Asset = ClothingAsset.Get())
	{
		if(Asset->IsValidLod(LodIndex))
		{
			FClothLODData& LodData = Asset->LodData[LodIndex];
			if(LodData.ParameterMasks.IsValidIndex(MaskIndex))
			{
				return &LodData.ParameterMasks[MaskIndex];
			}
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
