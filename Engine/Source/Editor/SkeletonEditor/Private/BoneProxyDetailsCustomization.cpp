// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "BoneProxyDetailsCustomization.h"
#include "BoneProxy.h"
#include "AnimPreviewInstance.h"
#include "PropertyHandle.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "ScopedTransaction.h"
#include "MultiBoxBuilder.h"
#include "SComboButton.h"
#include "SBox.h"

#define LOCTEXT_NAMESPACE "FBoneProxyDetailsCustomization"

namespace BoneProxyCustomizationConstants
{
	static const float ItemWidth = 125.0f;
}

static FText GetTransformFieldText(bool* bValuePtr, FText Label)
{
	return *bValuePtr ? FText::Format(LOCTEXT("Local", "Local {0}"), Label) : FText::Format(LOCTEXT("World", "World {0}"), Label);
}

static void OnSetRelativeTransform(bool* bValuePtr)
{
	*bValuePtr = true;
}

static void OnSetWorldTransform(bool* bValuePtr)
{
	*bValuePtr = false;
}

static bool IsRelativeTransformChecked(bool* bValuePtr)
{
	return *bValuePtr;
}

static bool IsWorldTransformChecked(bool* bValuePtr)
{
	return !*bValuePtr;
}

static TSharedRef<SWidget> BuildTransformFieldLabel(bool* bValuePtr, const FText& Label)
{
	FMenuBuilder MenuBuilder( true, nullptr, nullptr);

	FUIAction SetRelativeLocationAction
	(
		FExecuteAction::CreateStatic(&OnSetRelativeTransform, bValuePtr),
		FCanExecuteAction(),
		FIsActionChecked::CreateStatic(&IsRelativeTransformChecked, bValuePtr)
	);

	FUIAction SetWorldLocationAction
	(
		FExecuteAction::CreateStatic(&OnSetWorldTransform, bValuePtr),
		FCanExecuteAction(),
		FIsActionChecked::CreateStatic(&IsWorldTransformChecked, bValuePtr)
	);

	MenuBuilder.BeginSection( TEXT("TransformType"), FText::Format( LOCTEXT("TransformType", "{0} Type"), Label ) );

	MenuBuilder.AddMenuEntry
	(
		FText::Format( LOCTEXT( "LocalLabel", "Local"), Label ),
		FText::Format( LOCTEXT( "LocalLabel_ToolTip", "{0} is relative to its parent"), Label ),
		FSlateIcon(),
		SetRelativeLocationAction,
		NAME_None, 
		EUserInterfaceActionType::RadioButton
	);

	MenuBuilder.AddMenuEntry
	(
		FText::Format( LOCTEXT( "WorldLabel", "World"), Label ),
		FText::Format( LOCTEXT( "WorldLabel_ToolTip", "{0} is relative to the world"), Label ),
		FSlateIcon(),
		SetWorldLocationAction,
		NAME_None,
		EUserInterfaceActionType::RadioButton
	);

	MenuBuilder.EndSection();

	return 
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		[
			SNew(SComboButton)
			.ContentPadding( 0 )
			.ButtonStyle( FEditorStyle::Get(), "NoBorder" )
			.ForegroundColor( FSlateColor::UseForeground() )
			.MenuContent()
			[
				MenuBuilder.MakeWidget()
			]
			.ButtonContent()
			[
				SNew( SBox )
				.Padding( FMargin( 0.0f, 0.0f, 2.0f, 0.0f ) )
				[
					SNew(STextBlock)
					.Text_Static(&GetTransformFieldText, bValuePtr, Label)
					.Font(IDetailLayoutBuilder::GetDetailFont())
				]
			]
		];
}

void FBoneProxyDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	check(Objects.Num() == 1);

	UBoneProxy* BoneProxy = CastChecked<UBoneProxy>(Objects[0].Get());
	bool bIsEditingEnabled = true;
	if (UDebugSkelMeshComponent* Component = BoneProxy->SkelMeshComponent.Get())
	{
		bIsEditingEnabled = (Component->AnimScriptInstance == Component->PreviewInstance);
	}

	TSharedRef<IPropertyHandle> LocationProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UBoneProxy, Location));
	TSharedRef<IPropertyHandle> RotationProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UBoneProxy, Rotation));
	TSharedRef<IPropertyHandle> ScaleProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UBoneProxy, Scale));

	IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory(TEXT("Transform"));

	TSharedPtr<SWidget> NameWidget;
	TSharedPtr<SWidget> ValueWidget;

	IDetailPropertyRow& LocationPropertyRow = CategoryBuilder.AddProperty(LocationProperty);
	LocationPropertyRow.GetDefaultWidgets(NameWidget, ValueWidget);
	LocationPropertyRow.OverrideResetToDefault(FResetToDefaultOverride::Create(FIsResetToDefaultVisible::CreateSP(this, &FBoneProxyDetailsCustomization::IsResetLocationVisible, BoneProxy), FResetToDefaultHandler::CreateSP(this, &FBoneProxyDetailsCustomization::HandleResetLocation, BoneProxy)));

	LocationPropertyRow.CustomWidget()
	.NameContent()
	[
		BuildTransformFieldLabel(&BoneProxy->bLocalLocation, LOCTEXT("Location", "Location"))
	]
	.ValueContent()
	.MinDesiredWidth(BoneProxyCustomizationConstants::ItemWidth * 3.0f)
	.MaxDesiredWidth(BoneProxyCustomizationConstants::ItemWidth * 3.0f)
	[
		SNew(SBox)
		.IsEnabled(bIsEditingEnabled)
		[
			ValueWidget.ToSharedRef()
		]
	];

	IDetailPropertyRow& RotationPropertyRow = CategoryBuilder.AddProperty(RotationProperty);
	RotationPropertyRow.GetDefaultWidgets(NameWidget, ValueWidget);
	RotationPropertyRow.OverrideResetToDefault(FResetToDefaultOverride::Create(FIsResetToDefaultVisible::CreateSP(this, &FBoneProxyDetailsCustomization::IsResetRotationVisible, BoneProxy), FResetToDefaultHandler::CreateSP(this, &FBoneProxyDetailsCustomization::HandleResetRotation, BoneProxy)));

	RotationPropertyRow.CustomWidget()
	.NameContent()
	[
		BuildTransformFieldLabel(&BoneProxy->bLocalRotation, LOCTEXT("Rotation", "Rotation"))
	]
	.ValueContent()
	.MinDesiredWidth(BoneProxyCustomizationConstants::ItemWidth * 3.0f)
	.MaxDesiredWidth(BoneProxyCustomizationConstants::ItemWidth * 3.0f)
	[
		SNew(SBox)
		.IsEnabled(bIsEditingEnabled)
		[
			ValueWidget.ToSharedRef()
		]
	];

	IDetailPropertyRow& ScalePropertyRow = CategoryBuilder.AddProperty(ScaleProperty);
	ScalePropertyRow.GetDefaultWidgets(NameWidget, ValueWidget);
	ScalePropertyRow.OverrideResetToDefault(FResetToDefaultOverride::Create(FIsResetToDefaultVisible::CreateSP(this, &FBoneProxyDetailsCustomization::IsResetScaleVisible, BoneProxy), FResetToDefaultHandler::CreateSP(this, &FBoneProxyDetailsCustomization::HandleResetScale, BoneProxy)));

	ScalePropertyRow.CustomWidget()
	.NameContent()
	[
		SNew(STextBlock)
		.Text(ScaleProperty->GetPropertyDisplayName())
		.Font(DetailBuilder.GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(BoneProxyCustomizationConstants::ItemWidth * 3.0f)
	.MaxDesiredWidth(BoneProxyCustomizationConstants::ItemWidth * 3.0f)
	[
		SNew(SBox)
		.IsEnabled(bIsEditingEnabled)
		[
			ValueWidget.ToSharedRef()
		]
	];
}

bool FBoneProxyDetailsCustomization::IsResetLocationVisible(TSharedRef<IPropertyHandle> InPropertyHandle, UBoneProxy* InBoneProxy)
{
	if (UDebugSkelMeshComponent* Component = InBoneProxy->SkelMeshComponent.Get())
	{
		if (FAnimNode_ModifyBone* ModifyBone = Component->PreviewInstance->FindModifiedBone(InBoneProxy->BoneName))
		{
			return ModifyBone->Translation != FVector::ZeroVector;
		}
	}

	return false;
}

bool FBoneProxyDetailsCustomization::IsResetRotationVisible(TSharedRef<IPropertyHandle> InPropertyHandle, UBoneProxy* InBoneProxy)
{
	if (UDebugSkelMeshComponent* Component = InBoneProxy->SkelMeshComponent.Get())
	{
		if (FAnimNode_ModifyBone* ModifyBone = Component->PreviewInstance->FindModifiedBone(InBoneProxy->BoneName))
		{
			return ModifyBone->Rotation != FRotator::ZeroRotator;
		}
	}

	return false;
}

bool FBoneProxyDetailsCustomization::IsResetScaleVisible(TSharedRef<IPropertyHandle> InPropertyHandle, UBoneProxy* InBoneProxy)
{
	if (UDebugSkelMeshComponent* Component = InBoneProxy->SkelMeshComponent.Get())
	{
		if (FAnimNode_ModifyBone* ModifyBone = Component->PreviewInstance->FindModifiedBone(InBoneProxy->BoneName))
		{
			return ModifyBone->Scale != FVector(1.0f);
		}
	}

	return false;
}

void FBoneProxyDetailsCustomization::HandleResetLocation(TSharedRef<IPropertyHandle> InPropertyHandle, UBoneProxy* InBoneProxy)
{
	if (UDebugSkelMeshComponent* Component = InBoneProxy->SkelMeshComponent.Get())
	{
		FScopedTransaction Transaction(LOCTEXT("ResetLocation", "Reset Location"));

		InBoneProxy->Modify();
		Component->PreviewInstance->Modify();

		FAnimNode_ModifyBone& ModifyBone = Component->PreviewInstance->ModifyBone(InBoneProxy->BoneName);
		ModifyBone.Translation = FVector::ZeroVector;

		RemoveUnnecessaryModifications(Component, ModifyBone);
	}
}

void FBoneProxyDetailsCustomization::HandleResetRotation(TSharedRef<IPropertyHandle> InPropertyHandle, UBoneProxy* InBoneProxy)
{
	if (UDebugSkelMeshComponent* Component = InBoneProxy->SkelMeshComponent.Get())
	{
		FScopedTransaction Transaction(LOCTEXT("ResetRotation", "Reset Rotation"));

		InBoneProxy->Modify();
		Component->PreviewInstance->Modify();

		FAnimNode_ModifyBone& ModifyBone = Component->PreviewInstance->ModifyBone(InBoneProxy->BoneName);
		ModifyBone.Rotation = FRotator::ZeroRotator;

		RemoveUnnecessaryModifications(Component, ModifyBone);
	}
}

void FBoneProxyDetailsCustomization::HandleResetScale(TSharedRef<IPropertyHandle> InPropertyHandle, UBoneProxy* InBoneProxy)
{
	if (UDebugSkelMeshComponent* Component = InBoneProxy->SkelMeshComponent.Get())
	{
		FScopedTransaction Transaction(LOCTEXT("ResetScale", "Reset Scale"));

		InBoneProxy->Modify();
		Component->PreviewInstance->Modify();

		FAnimNode_ModifyBone& ModifyBone = Component->PreviewInstance->ModifyBone(InBoneProxy->BoneName);
		ModifyBone.Scale = FVector(1.0f);

		RemoveUnnecessaryModifications(Component, ModifyBone);
	}
}

void FBoneProxyDetailsCustomization::RemoveUnnecessaryModifications(UDebugSkelMeshComponent* Component, FAnimNode_ModifyBone& ModifyBone)
{
	if (ModifyBone.Translation == FVector::ZeroVector && ModifyBone.Rotation == FRotator::ZeroRotator && ModifyBone.Scale == FVector(1.0f))
	{
		Component->PreviewInstance->RemoveBoneModification(ModifyBone.BoneToModify.BoneName);
	}
}

#undef LOCTEXT_NAMESPACE
