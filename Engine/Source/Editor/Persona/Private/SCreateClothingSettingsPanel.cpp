// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SCreateClothingSettingsPanel.h"

#include "PropertyEditorModule.h"
#include "ModuleManager.h"
#include "IStructureDetailsView.h"
#include "IDetailsView.h"
#include "SUniformGridPanel.h"
#include "DetailLayoutBuilder.h"
#include "SBox.h"
#include "SButton.h"

#define LOCTEXT_NAMESPACE "CreateClothSettings"

void SCreateClothingSettingsPanel::Construct(const FArguments& InArgs)
{
	check(InArgs._LodIndex != INDEX_NONE && InArgs._SectionIndex != INDEX_NONE);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	TSharedPtr<IStructureDetailsView> StructureDetailsView;

	OnCreateDelegate = InArgs._OnCreateRequested;

	BuildParams.LodIndex = InArgs._LodIndex;
	BuildParams.SourceSection = InArgs._SectionIndex;

	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bAllowSearch = false;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bSearchInitialKeyFocus = true;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.NotifyHook = nullptr;
		DetailsViewArgs.bShowOptions = true;
		DetailsViewArgs.bShowModifiedPropertiesOption = false;
		DetailsViewArgs.bShowScrollBar = false;
	}

	FStructureDetailsViewArgs StructureViewArgs;
	{
		StructureViewArgs.bShowObjects = true;
		StructureViewArgs.bShowAssets = true;
		StructureViewArgs.bShowClasses = true;
		StructureViewArgs.bShowInterfaces = true;
	}

	StructureDetailsView = PropertyEditorModule.CreateStructureDetailView(DetailsViewArgs, StructureViewArgs, nullptr);

	StructureDetailsView->GetDetailsView().SetGenericLayoutDetailsDelegate(FOnGetDetailCustomizationInstance::CreateStatic(&FClothCreateSettingsCustomization::MakeInstance, InArgs._LodIndex, InArgs._SectionIndex));

	BuildParams.AssetName = InArgs._MeshName + TEXT("_Clothing");

	FStructOnScope* Struct = new FStructOnScope(FSkeletalMeshClothBuildParams::StaticStruct(), (uint8*)&BuildParams);
	StructureDetailsView->SetStructureData(MakeShareable(Struct));
	
	this->ChildSlot
	[
		SNew(SBox)
		.MinDesiredWidth(300.0f)
		[
			SNew(SVerticalBox)

			+SVerticalBox::Slot()
			.MaxHeight(500.0f)
			.Padding(2)
			[
				StructureDetailsView->GetWidget()->AsShared()
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			.HAlign(HAlign_Right)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(2)
				+SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("Label_Create", "Create"))
					.OnClicked(this, &SCreateClothingSettingsPanel::OnCreateClicked)
				]
			]
		]
	];
}

FReply SCreateClothingSettingsPanel::OnCreateClicked()
{
	OnCreateDelegate.ExecuteIfBound(BuildParams);

	return FReply::Handled();
}

void FClothCreateSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TSharedPtr<IPropertyHandle> LodIndexProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(FSkeletalMeshClothBuildParams, LodIndex), (UClass*)FSkeletalMeshClothBuildParams::StaticStruct());
	TSharedPtr<IPropertyHandle> SectionIndexProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(FSkeletalMeshClothBuildParams, SourceSection), (UClass*)FSkeletalMeshClothBuildParams::StaticStruct());

	if(LodIndexProperty.IsValid())
	{
		LodIndexProperty->MarkHiddenByCustomization();
	}

	if(SectionIndexProperty.IsValid())
	{
		SectionIndexProperty->MarkHiddenByCustomization();
	}
}

#undef LOCTEXT_NAMESPACE
