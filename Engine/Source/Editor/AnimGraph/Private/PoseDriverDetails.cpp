// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "PoseDriverDetails.h"
#include "Classes/AnimGraphNode_PoseDriver.h"
#include "DetailLayoutBuilder.h"
#include "IDetailsView.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "PropertyCustomizationHelpers.h"
#include "SVectorInputBox.h"
#include "SRotatorInputBox.h"
#include "SNumericEntryBox.h"
#include "SEditableTextBox.h"
#include "SWidgetSwitcher.h"
#include "SExpandableArea.h"
#include "SComboBox.h"
#include "SButton.h"

#define LOCTEXT_NAMESPACE "PoseDriverDetails"

static const FName ColumnId_Target("Target");

void SPDD_TargetRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	TargetIndex = InArgs._TargetIndex;
	PoseDriverDetailsPtr = InArgs._PoseDriverDetails;

	SMultiColumnTableRow< TSharedPtr<FPDD_TargetInfo> >::Construct(FSuperRowType::FArguments(), InOwnerTableView);
}

TSharedRef< SWidget > SPDD_TargetRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	TSharedPtr<FPoseDriverDetails> PoseDriverDetails = PoseDriverDetailsPtr.Pin();

	return
	SNew(SBox)
	.Padding(2)
	[
		SNew(SBorder)
		.Padding(0)
		.ForegroundColor(FLinearColor::White)
		.BorderImage(FEditorStyle::GetBrush("NoBorder"))
		[
			SNew(SExpandableArea)
			.Padding(0)
			.InitiallyCollapsed(true)
			.BorderBackgroundColor(FLinearColor(.6f, .6f, .6f))
			.OnAreaExpansionChanged(this, &SPDD_TargetRow::OnTargetExpansionChanged)
			.HeaderContent()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(this, &SPDD_TargetRow::GetTargetTitleText)
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SNew(SSpacer)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.MinDesiredWidth(150)
					.Content()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.FillWidth(TAttribute<float>::Create(TAttribute<float>::FGetter::CreateLambda([this] { return 1.f - this->GetTargetWeight(); })))
						[
							SNew(SSpacer)
						]

						+ SHorizontalBox::Slot()
						.FillWidth(TAttribute<float>::Create(TAttribute<float>::FGetter::CreateLambda([this] { return this->GetTargetWeight(); })))
						[
							SNew(SImage)
							.ColorAndOpacity(this, &SPDD_TargetRow::GetWeightBarColor)
							.Image(FEditorStyle::GetBrush("WhiteBrush"))
						]
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin(3,0))
				[
					SNew(SBox)
					.MinDesiredWidth(40)
					.MaxDesiredWidth(40)
					.Content()
					[
						SNew(STextBlock)
						.Text(this, &SPDD_TargetRow::GetTargetWeightText)
					]
				]
			]
			.BodyContent()
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2.0f)
					.VAlign(VAlign_Fill)
					[
						SNew(SBox)
						.MaxDesiredWidth(800.f)
						.Content()
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.FillWidth(1)
							[
								SNew(SWidgetSwitcher)
								.WidgetIndex(this, &SPDD_TargetRow::GetTransRotWidgetIndex)

								+SWidgetSwitcher::Slot()
								[
									SNew(SVectorInputBox)
									.X(this, &SPDD_TargetRow::GetTranslationX)
									.OnXChanged(this, &SPDD_TargetRow::SetTranslationX)
									.Y(this, &SPDD_TargetRow::GetTranslationY)
									.OnYChanged(this, &SPDD_TargetRow::SetTranslationY)
									.Z(this, &SPDD_TargetRow::GetTranslationZ)
									.OnZChanged(this, &SPDD_TargetRow::SetTranslationZ)
								]

								+ SWidgetSwitcher::Slot()
								[
									SNew(SRotatorInputBox)
									.Roll(this, &SPDD_TargetRow::GetRotationRoll)
									.OnRollChanged(this, &SPDD_TargetRow::SetRotationRoll)
									.Pitch(this, &SPDD_TargetRow::GetRotationPitch)
									.OnPitchChanged(this, &SPDD_TargetRow::SetRotationPitch)
									.Yaw(this, &SPDD_TargetRow::GetRotationYaw)
									.OnYawChanged(this, &SPDD_TargetRow::SetRotationYaw)
								]
							]
						]
					]

					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2.0f)
					.VAlign(VAlign_Fill)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						.Padding(FMargin(0, 0, 3, 0))
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Scale", "Scale:"))
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SBox)
							.MinDesiredWidth(150.f)
							.Content()
							[
								SNew(SNumericEntryBox<float>)
								.MinSliderValue(0.f)
								.MaxSliderValue(1.f)
								.Value(this, &SPDD_TargetRow::GetScale)
								.OnValueChanged(this, &SPDD_TargetRow::SetScale)
								.AllowSpin(true)
							]
						]

						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						.Padding(FMargin(6, 0, 3, 0))
						[
							SNew(STextBlock)
							.Text(LOCTEXT("DrivenName","Drive:"))
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew( SComboBox<TSharedPtr<FName>>)
							.OptionsSource(&PoseDriverDetails->DrivenNameOptions)
							.OnGenerateWidget(this, &SPDD_TargetRow::MakeDrivenNameWidget)
							.OnSelectionChanged(this, &SPDD_TargetRow::OnDrivenNameChanged)
							.Content()						
							[
								SNew(STextBlock)
								.Text(this, &SPDD_TargetRow::GetDrivenNameText)
							]
						]

						+ SHorizontalBox::Slot()
						.FillWidth(1)
						[
							SNew(SSpacer)
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							PropertyCustomizationHelpers::MakeEmptyButton(FSimpleDelegate::CreateSP(this, &SPDD_TargetRow::RemoveTarget), LOCTEXT("RemoveTarget", "Remove target"))
						]
					]
				]
			]
		]
	];
}

FPoseDriverTarget& SPDD_TargetRow::GetTarget() const
{
	TSharedPtr<FPoseDriverDetails> PoseDriverDetails = PoseDriverDetailsPtr.Pin();
	return PoseDriverDetails->GetFirstSelectedPoseDriver()->Node.PoseTargets[TargetIndex];
}

bool SPDD_TargetRow::IsEditingRotation() const
{
	TSharedPtr<FPoseDriverDetails> PoseDriverDetails = PoseDriverDetailsPtr.Pin();
	return (PoseDriverDetails->GetFirstSelectedPoseDriver()->Node.DriveSource == EPoseDriverSource::Rotation);
}

void SPDD_TargetRow::NotifyTargetChanged()
{
	TSharedPtr<FPoseDriverDetails> PoseDriverDetails = PoseDriverDetailsPtr.Pin();
	PoseDriverDetails->NodePropHandle->NotifyPostChange(); // Will push change to preview node instance
}

float SPDD_TargetRow::GetTargetWeight() const
{
	float OutWeight = 0.f;

	TSharedPtr<FPoseDriverDetails> PoseDriverDetails = PoseDriverDetailsPtr.Pin();
	FAnimNode_PoseDriver* PreviewNode = PoseDriverDetails->GetFirstSelectedPoseDriver()->GetPreviewPoseDriverNode();
	if (PreviewNode)
	{
		for (const FRBFOutputWeight& Weight : PreviewNode->OutputWeights)
		{
			if (Weight.TargetIndex == TargetIndex)
			{
				OutWeight = Weight.TargetWeight;
			}
		}
	}

	return OutWeight;
}


int32 SPDD_TargetRow::GetTransRotWidgetIndex() const
{
	return IsEditingRotation() ? 1 : 0;
}


TOptional<float> SPDD_TargetRow::GetTranslationX() const
{
	return GetTarget().TargetTranslation.X;
}

TOptional<float> SPDD_TargetRow::GetTranslationY() const
{
	return GetTarget().TargetTranslation.Y;
}

TOptional<float> SPDD_TargetRow::GetTranslationZ() const
{
	return GetTarget().TargetTranslation.Z;
}

TOptional<float> SPDD_TargetRow::GetRotationRoll() const
{
	return GetTarget().TargetRotation.Roll;
}

TOptional<float> SPDD_TargetRow::GetRotationPitch() const
{
	return GetTarget().TargetRotation.Pitch;
}

TOptional<float> SPDD_TargetRow::GetRotationYaw() const
{
	return GetTarget().TargetRotation.Yaw;
}

TOptional<float> SPDD_TargetRow::GetScale() const
{
	return GetTarget().TargetScale;
}

FText SPDD_TargetRow::GetDrivenNameText() const
{
	return FText::FromName(GetTarget().DrivenName);
}

void SPDD_TargetRow::OnDrivenNameChanged(TSharedPtr<FName> NewName, ESelectInfo::Type SelectInfo)
{
	if (SelectInfo != ESelectInfo::Direct)
	{
		GetTarget().DrivenName = *NewName.Get();
		NotifyTargetChanged();
	}
}

TSharedRef<SWidget> SPDD_TargetRow::MakeDrivenNameWidget(TSharedPtr<FName> InItem)
{
	return 
		SNew(STextBlock)
		.Text(FText::FromName(*InItem));
}

FText SPDD_TargetRow::GetTargetTitleText() const
{
	FString Title = FString::Printf(TEXT("%d - %s"), TargetIndex, *GetTarget().DrivenName.ToString());
	return FText::FromString(Title);
}

FText SPDD_TargetRow::GetTargetWeightText() const
{
	FString Title = FString::Printf(TEXT("%2.1f"), GetTargetWeight() * 100.f);
	return FText::FromString(Title);
}

FSlateColor SPDD_TargetRow::GetWeightBarColor() const
{
	return FMath::Lerp(FLinearColor::White, FLinearColor::Red, GetTargetWeight());
}


void SPDD_TargetRow::SetTranslationX(float NewX)
{
	GetTarget().TargetTranslation.X = NewX;
	NotifyTargetChanged();
}

void SPDD_TargetRow::SetTranslationY(float NewY)
{
	GetTarget().TargetTranslation.Y = NewY;
	NotifyTargetChanged();
}

void SPDD_TargetRow::SetTranslationZ(float NewZ)
{
	GetTarget().TargetTranslation.Z = NewZ;
	NotifyTargetChanged();
}

void SPDD_TargetRow::SetRotationRoll(float NewRoll)
{
	GetTarget().TargetRotation.Roll = NewRoll;
	NotifyTargetChanged();
}

void SPDD_TargetRow::SetRotationPitch(float NewPitch)
{
	GetTarget().TargetRotation.Pitch = NewPitch;
	NotifyTargetChanged();
}

void SPDD_TargetRow::SetRotationYaw(float NewYaw)
{
	GetTarget().TargetRotation.Yaw = NewYaw;
	NotifyTargetChanged();
}

void SPDD_TargetRow::SetScale(float NewScale)
{
	GetTarget().TargetScale = NewScale;
	NotifyTargetChanged();
}

void SPDD_TargetRow::SetDrivenNameText(const FText& NewText, ETextCommit::Type CommitType)
{
	GetTarget().DrivenName = FName(*NewText.ToString());
	NotifyTargetChanged();
}

void SPDD_TargetRow::RemoveTarget()
{
	TSharedPtr<FPoseDriverDetails> PoseDriverDetails = PoseDriverDetailsPtr.Pin();
	PoseDriverDetails->RemoveTarget(TargetIndex); // This will remove me
}

void SPDD_TargetRow::OnTargetExpansionChanged(bool bExpanded)
{
	TSharedPtr<FPoseDriverDetails> PoseDriverDetails = PoseDriverDetailsPtr.Pin();
	PoseDriverDetails->SelectTarget(TargetIndex);
}


//////////////////////////////////////////////////////////////////////////

TSharedRef<IDetailCustomization> FPoseDriverDetails::MakeInstance()
{
	return MakeShareable(new FPoseDriverDetails);
}

void FPoseDriverDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& PoseTargetsCategory = DetailBuilder.EditCategory("PoseTargets");

	// Get a property handle because we might need to to invoke NotifyPostChange
	NodePropHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UAnimGraphNode_PoseDriver, Node));

	// Bind delegate to PoseAsset changing
	TSharedRef<IPropertyHandle> PoseAssetPropHandle = DetailBuilder.GetProperty("Node.PoseAsset");
	PoseAssetPropHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPoseDriverDetails::OnPoseAssetChanged));

	// Cache set of selected things
	SelectedObjectsList = DetailBuilder.GetDetailsView().GetSelectedObjects();

	// Create list of driven names
	UpdateDrivenNameOptions();

	TSharedPtr<IPropertyHandle> PoseTargetsProperty = DetailBuilder.GetProperty("Node.PoseTargets", UAnimGraphNode_PoseDriver::StaticClass());

	IDetailPropertyRow& PoseTargetsRow = PoseTargetsCategory.AddProperty(PoseTargetsProperty);
	PoseTargetsRow.ShowPropertyButtons(false);
	FDetailWidgetRow& PoseTargetRowWidget = PoseTargetsRow.CustomWidget();

	PoseTargetRowWidget.WholeRowContent()
	.HAlign(HAlign_Fill)
	[
		SNew(SVerticalBox)

		+SVerticalBox::Slot()
		.FillHeight(1)
		[
			SAssignNew(TargetListWidget, SPDD_TargetListType)
			.ListItemsSource(&TargetInfos)
			.OnGenerateRow(this, &FPoseDriverDetails::GenerateTargetRow)
			.SelectionMode(ESelectionMode::SingleToggle)
			.OnSelectionChanged(this, &FPoseDriverDetails::OnTargetSelectionChanged)
			.HeaderRow
			(
				SNew(SHeaderRow)
				.Visibility(EVisibility::Collapsed)
				+ SHeaderRow::Column(ColumnId_Target)
			)
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(5, 2))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.Padding(2)
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked(this, &FPoseDriverDetails::ClickedAddTarget)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AddTarget", "Add Target"))
				]
			]

			+ SHorizontalBox::Slot()
			.Padding(2)
			.AutoWidth()
			[
				SNew(SButton)
				.ToolTipText(LOCTEXT("CopyFromPoseAssetTooltip", "Copy target positions from PoseAsset. Will overwrite any existing targets."))
				.OnClicked(this, &FPoseDriverDetails::ClickedOnCopyFromPoseAsset)
				.IsEnabled(this, &FPoseDriverDetails::CopyFromPoseAssetIsEnabled)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CopyFromPoseAsset", "Copy All From PoseAsset"))
				]
			]

			+ SHorizontalBox::Slot()
			.Padding(2)
			.AutoWidth()
			[
				SNew(SButton)
				.ToolTipText(LOCTEXT("AutoTargetScaleTooltip", "Automatically set all Scale factors, based on distance to nearest neighbour targets."))
				.OnClicked(this, &FPoseDriverDetails::ClickedOnAutoScaleFactors)
				.IsEnabled(this, &FPoseDriverDetails::AutoScaleFactorsIsEnabled)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AutoTargetScale", "Auto Scale"))
				]
			]
		]
	];

	// Update target list from selected pose driver node
	UpdateTargetInfosList();
}

TSharedRef<ITableRow> FPoseDriverDetails::GenerateTargetRow(TSharedPtr<FPDD_TargetInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable)
{
	check(InInfo.IsValid());

	return
		SNew(SPDD_TargetRow, OwnerTable)
		.TargetIndex(InInfo->TargetIndex)
		.PoseDriverDetails(SharedThis(this));
}

void FPoseDriverDetails::OnTargetSelectionChanged(TSharedPtr<FPDD_TargetInfo> InInfo, ESelectInfo::Type SelectInfo)
{
	UAnimGraphNode_PoseDriver* PoseDriver = GetFirstSelectedPoseDriver();
	if (PoseDriver)
	{
		if (InInfo.IsValid())
		{
			PoseDriver->SelectedTargetIndex = InInfo->TargetIndex;
		}
		else
		{
			PoseDriver->SelectedTargetIndex = INDEX_NONE;
		}
	}
}

void FPoseDriverDetails::OnPoseAssetChanged()
{
	UpdateDrivenNameOptions();
}


UAnimGraphNode_PoseDriver* FPoseDriverDetails::GetFirstSelectedPoseDriver() const
{
	for (const TWeakObjectPtr<UObject>& Object : SelectedObjectsList)
	{
		UAnimGraphNode_PoseDriver* TestPoseDriver = Cast<UAnimGraphNode_PoseDriver>(Object.Get());
		if (TestPoseDriver != nullptr && !TestPoseDriver->IsTemplate())
		{
			return TestPoseDriver;
		}
	}

	return nullptr;
}

void FPoseDriverDetails::UpdateTargetInfosList()
{
	TargetInfos.Empty();
	
	UAnimGraphNode_PoseDriver* PoseDriver = GetFirstSelectedPoseDriver();
	if (PoseDriver)
	{
		for (int32 i = 0; i < PoseDriver->Node.PoseTargets.Num(); i++)
		{
			TargetInfos.Add( FPDD_TargetInfo::Make(i) );
		}
	}

	TargetListWidget->RequestListRefresh();
}

void FPoseDriverDetails::UpdateDrivenNameOptions()
{
	DrivenNameOptions.Empty();

	UAnimGraphNode_PoseDriver* PoseDriver = GetFirstSelectedPoseDriver();
	if (PoseDriver)
	{
		// None is always an option
		DrivenNameOptions.Add(MakeShareable(new FName(NAME_None)));

		// Compile list of all curves in Skeleton
		if (PoseDriver->Node.DriveOutput == EPoseDriverOutput::DriveCurves)
		{
			USkeleton* Skeleton = PoseDriver->GetAnimBlueprint()->TargetSkeleton;
			if (Skeleton)
			{
				const FSmartNameMapping* Mapping = Skeleton->GetSmartNameContainer(USkeleton::AnimCurveMappingName);
				if (Mapping)
				{
					TArray<SmartName::UID_Type> UidList;
					Mapping->FillUidArray(UidList);

					for (SmartName::UID_Type Uid : UidList)
					{
						FSmartName SmartName;
						Mapping->FindSmartNameByUID(Uid, SmartName);

						DrivenNameOptions.Add(MakeShareable(new FName(SmartName.DisplayName)));
					}
				}
			}
		}
		// Compile list of all poses in PoseAsset
		else
		{
			if (PoseDriver->Node.PoseAsset)
			{
				const TArray<FSmartName> PoseNames = PoseDriver->Node.PoseAsset->GetPoseNames();
				for (const FSmartName& SmartName : PoseNames)
				{
					DrivenNameOptions.Add(MakeShareable(new FName(SmartName.DisplayName)));
				}
			}
		}
	}
}


FReply FPoseDriverDetails::ClickedOnCopyFromPoseAsset()
{
	UAnimGraphNode_PoseDriver* PoseDriver = GetFirstSelectedPoseDriver();
	if (PoseDriver)
	{
		PoseDriver->CopyTargetsFromPoseAsset();

		// Also update radius/scales
		float MaxDist;
		PoseDriver->AutoSetTargetScales(MaxDist);
		PoseDriver->Node.RBFParams.Radius = 0.5f * MaxDist; // reasonable default radius

		UpdateTargetInfosList();
		NodePropHandle->NotifyPostChange();
	}
	return FReply::Handled();
}

bool FPoseDriverDetails::CopyFromPoseAssetIsEnabled() const
{
	UAnimGraphNode_PoseDriver* PoseDriver = GetFirstSelectedPoseDriver();
	return(PoseDriver && PoseDriver->Node.PoseAsset);
}


FReply FPoseDriverDetails::ClickedOnAutoScaleFactors()
{
	UAnimGraphNode_PoseDriver* PoseDriver = GetFirstSelectedPoseDriver();
	if (PoseDriver)
	{
		float MaxDist;
		PoseDriver->AutoSetTargetScales(MaxDist);
		PoseDriver->Node.RBFParams.Radius = 0.5f * MaxDist; // reasonable default radius
		NodePropHandle->NotifyPostChange();
	}
	return FReply::Handled();
}

bool FPoseDriverDetails::AutoScaleFactorsIsEnabled() const
{
	UAnimGraphNode_PoseDriver* PoseDriver = GetFirstSelectedPoseDriver();
	return(PoseDriver && PoseDriver->Node.PoseTargets.Num() > 1);
}

FReply FPoseDriverDetails::ClickedAddTarget()
{
	UAnimGraphNode_PoseDriver* PoseDriver = GetFirstSelectedPoseDriver();
	if (PoseDriver)
	{
		FPoseDriverTarget NewTarget;
		PoseDriver->Node.PoseTargets.Add(NewTarget);
		UpdateTargetInfosList();
		NodePropHandle->NotifyPostChange(); // will push changes to preview node instance
	}
	return FReply::Handled();
}

void FPoseDriverDetails::RemoveTarget(int32 TargetIndex)
{
	UAnimGraphNode_PoseDriver* PoseDriver = GetFirstSelectedPoseDriver();
	if (PoseDriver && PoseDriver->Node.PoseTargets.IsValidIndex(TargetIndex))
	{
		PoseDriver->Node.PoseTargets.RemoveAt(TargetIndex);
		UpdateTargetInfosList();
		NodePropHandle->NotifyPostChange(); // will push changes to preview node instance
	}
}

void FPoseDriverDetails::SelectTarget(int32 TargetIndex)
{
	if (TargetInfos.IsValidIndex(TargetIndex))
	{
		TargetListWidget->SetSelection(TargetInfos[TargetIndex]);
	}
}

#undef LOCTEXT_NAMESPACE

