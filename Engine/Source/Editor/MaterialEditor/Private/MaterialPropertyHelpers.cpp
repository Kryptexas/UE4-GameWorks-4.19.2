// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MaterialPropertyHelpers.h"
#include "Misc/MessageDialog.h"
#include "Misc/Guid.h"
#include "UObject/UnrealType.h"
#include "Layout/Margin.h"
#include "Misc/Attribute.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SToolTip.h"
#include "EditorStyleSet.h"
#include "Materials/MaterialInterface.h"
#include "MaterialEditor/DEditorFontParameterValue.h"
#include "MaterialEditor/DEditorMaterialLayersParameterValue.h"
#include "MaterialEditor/DEditorScalarParameterValue.h"
#include "MaterialEditor/DEditorStaticComponentMaskParameterValue.h"
#include "MaterialEditor/DEditorStaticSwitchParameterValue.h"
#include "MaterialEditor/DEditorTextureParameterValue.h"
#include "MaterialEditor/DEditorVectorParameterValue.h"
#include "MaterialEditor/MaterialEditorInstanceConstant.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialExpressionParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"
#include "Materials/MaterialExpressionFontSampleParameter.h"
#include "Materials/MaterialExpressionMaterialAttributeLayers.h"
#include "Materials/MaterialExpressionChannelMaskParameter.h"
#include "EditorSupportDelegates.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "IDetailPropertyRow.h"
#include "DetailLayoutBuilder.h"
#include "IDetailGroup.h"
#include "DetailCategoryBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialFunctionInstance.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialFunctionInterface.h"
#include "ModuleManager.h"
#include "AssetToolsModule.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "StaticParameterSet.h"
#include "MaterialEditor/MaterialEditorPreviewParameters.h"
#include "Factories/MaterialFunctionInstanceFactory.h"
#include "SMaterialLayersFunctionsTree.h"
#include "Materials/MaterialFunctionMaterialLayer.h"
#include "Materials/MaterialFunctionMaterialLayerBlend.h"
#include "Factories/MaterialFunctionMaterialLayerFactory.h"
#include "Factories/MaterialFunctionMaterialLayerBlendFactory.h"

#define LOCTEXT_NAMESPACE "MaterialPropertyHelper"

FText FMaterialPropertyHelpers::LayerID = LOCTEXT("LayerID", "Layer Asset");
FText FMaterialPropertyHelpers::BlendID = LOCTEXT("BlendID", "Blend Asset");
FName FMaterialPropertyHelpers::LayerParamName = FName("Global Material Layers Parameter Values");

EVisibility FMaterialPropertyHelpers::ShouldShowExpression(UDEditorParameterValue* Parameter, UMaterialEditorInstanceConstant* MaterialEditorInstance, FGetShowHiddenParameters ShowHiddenDelegate)
{
	bool bShowHidden = true;

	ShowHiddenDelegate.ExecuteIfBound(bShowHidden);

	return (bShowHidden || MaterialEditorInstance->VisibleExpressions.Contains(Parameter->ParameterInfo))? EVisibility::Visible: EVisibility::Collapsed;
}

void FMaterialPropertyHelpers::OnMaterialLayerAssetChanged(const struct FAssetData& InAssetData, int32 Index, EMaterialParameterAssociation MaterialType, TSharedPtr<class IPropertyHandle> InHandle, FMaterialLayersFunctions* InMaterialFunction)
{
	const FScopedTransaction Transaction(LOCTEXT("SetLayerorBlendAsset", "Set Layer or Blend Asset"));
	InHandle->NotifyPreChange();
	const FName FilterTag = FName(TEXT("MaterialFunctionUsage"));
	const FString* MaterialFunctionUsage = InAssetData.TagsAndValues.Find(FilterTag);
	FString CompareString;
	if (MaterialFunctionUsage || InAssetData.AssetName == NAME_None)
	{
		switch (MaterialType)
		{
		case EMaterialParameterAssociation::LayerParameter:
		{
			if (Cast<UMaterialFunctionInterface>(InAssetData.GetAsset()))
			{
				InMaterialFunction->Layers[Index] = Cast<UMaterialFunctionInterface>(InAssetData.GetAsset());
			}
			else
			{
				InMaterialFunction->Layers[Index] = nullptr;
			}
		}
		break;
		case EMaterialParameterAssociation::BlendParameter:
		{
			if (Cast<UMaterialFunctionInterface>(InAssetData.GetAsset()))
			{
				InMaterialFunction->Blends[Index] = Cast<UMaterialFunctionInterface>(InAssetData.GetAsset());
			}
			else
			{
				InMaterialFunction->Blends[Index] = nullptr;
			}
		}
		break;
		default:
		break;
		}
	}
	InMaterialFunction->UpdateStaticPermutationString();
	InHandle->NotifyPostChange();
}

bool FMaterialPropertyHelpers::FilterLayerAssets(const struct FAssetData& InAssetData, FMaterialLayersFunctions* LayerFunction, EMaterialParameterAssociation MaterialType, int32 Index)
{
	bool ShouldAssetBeFilteredOut = false;
	const FName FilterTag = FName(TEXT("MaterialFunctionUsage"));
	const FName BaseTag = FName(TEXT("Base"));
	const FString* MaterialFunctionUsage = InAssetData.TagsAndValues.Find(FilterTag);
	FName BaseClassName;
	FName InstanceClassName;

	FString CompareString;
	if (MaterialFunctionUsage)
	{
		FString* Base = const_cast<FString*>(InAssetData.TagsAndValues.Find(BaseTag));

		FString BaseString;
		FString DiscardString;
		FString CleanString;
		if (Base != nullptr)
		{
			BaseString = *Base;
			BaseString.Split(".", &DiscardString, &CleanString);
			CleanString.Split("'", &CleanString, &DiscardString);
		}
		else
		{
			CleanString = InAssetData.AssetName.ToString();
		}

		FString LeftPath;
		FString RightPath;
		bool bShouldFilter = false;
		switch (MaterialType)
		{
		case EMaterialParameterAssociation::LayerParameter:
		{
			CompareString = "MaterialLayer";
			if (LayerFunction->Layers[Index] != nullptr)
			{
				RightPath = LayerFunction->Layers[Index]->GetBaseFunction()->GetFName().ToString();
				if (RightPath.IsEmpty())
				{
					RightPath = LayerFunction->Layers[Index]->GetFName().ToString();
				}
			}
			bShouldFilter = LayerFunction->RestrictToLayerRelatives[Index];
			BaseClassName = UMaterialFunctionMaterialLayer::StaticClass()->GetFName();
			InstanceClassName = UMaterialFunctionMaterialLayerInstance::StaticClass()->GetFName();
		}
		break;
		case EMaterialParameterAssociation::BlendParameter:
		{
			CompareString = "MaterialLayerBlend";
			if (LayerFunction->Blends[Index] != nullptr)
			{
				RightPath = LayerFunction->Blends[Index]->GetBaseFunction()->GetFName().ToString();
				if (RightPath.IsEmpty())
				{
					RightPath = LayerFunction->Blends[Index]->GetFName().ToString();
				}
			}
			bShouldFilter = LayerFunction->RestrictToBlendRelatives[Index];
			BaseClassName = UMaterialFunctionMaterialLayerBlend::StaticClass()->GetFName();
			InstanceClassName = UMaterialFunctionMaterialLayerBlendInstance::StaticClass()->GetFName();
		}
		break;
		default:
		break;
		}

		if (*MaterialFunctionUsage != CompareString)
		{

			ShouldAssetBeFilteredOut = true;
		}
		else
		{
			bool bSameBase = CleanString == RightPath;
			if (!RightPath.IsEmpty() && !bSameBase && bShouldFilter)
			{
				ShouldAssetBeFilteredOut = true;
			}
		}
	}
	else
	{
		ShouldAssetBeFilteredOut = true;
	}
	return ShouldAssetBeFilteredOut;
}

FReply FMaterialPropertyHelpers::OnClickedSaveNewMaterialInstance(UMaterialInterface* Parent, UObject* EditorObject)
{
	const FString DefaultSuffix = TEXT("_Inst");
	TArray<FEditorParameterGroup> ParameterGroups;
	UMaterialEditorInstanceConstant* MaterialInstanceEditor = Cast<UMaterialEditorInstanceConstant>(EditorObject);
	if (MaterialInstanceEditor)
	{
		ParameterGroups = MaterialInstanceEditor->ParameterGroups;
	}
	UMaterialEditorPreviewParameters* MaterialEditor = Cast<UMaterialEditorPreviewParameters>(EditorObject);
	if (MaterialEditor)
	{
		ParameterGroups = MaterialEditor->ParameterGroups;
	}
	if (!MaterialEditor && !MaterialInstanceEditor)
	{
		return FReply::Unhandled();
	}

	if (Parent)
	{
		// Create an appropriate and unique name 
		FString Name;
		FString PackageName;
		FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().CreateUniqueAssetName(Parent->GetOutermost()->GetName(), DefaultSuffix, PackageName, Name);

		UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
		Factory->InitialParent = Parent;

		UObject* Child = AssetToolsModule.Get().CreateAssetWithDialog(Name, FPackageName::GetLongPackagePath(PackageName), UMaterialInstanceConstant::StaticClass(), Factory);
		UMaterialInstanceConstant* ChildInstance = Cast<UMaterialInstanceConstant>(Child);
		CopyMaterialToInstance(ChildInstance, ParameterGroups);

	}
	return FReply::Handled();
}

void FMaterialPropertyHelpers::CopyMaterialToInstance(UMaterialInstanceConstant* ChildInstance, TArray<FEditorParameterGroup> &ParameterGroups)
{
	if (ChildInstance)
	{
		if (ChildInstance->IsTemplate(RF_ClassDefaultObject) == false)
		{
			ChildInstance->MarkPackageDirty();
			ChildInstance->ClearParameterValuesEditorOnly();
			//propagate changes to the base material so the instance will be updated if it has a static permutation resource
			FStaticParameterSet NewStaticParameters;
			for (int32 GroupIdx = 0; GroupIdx < ParameterGroups.Num(); GroupIdx++)
			{
				FEditorParameterGroup& Group = ParameterGroups[GroupIdx];
				for (int32 ParameterIdx = 0; ParameterIdx < Group.Parameters.Num(); ParameterIdx++)
				{
					if (Group.Parameters[ParameterIdx] == NULL)
					{
						continue;
					}
					UDEditorScalarParameterValue* ScalarParameterValue = Cast<UDEditorScalarParameterValue>(Group.Parameters[ParameterIdx]);
					if (ScalarParameterValue)
					{
						if (ScalarParameterValue->bOverride)
						{
							ChildInstance->SetScalarParameterValueEditorOnly(ScalarParameterValue->ParameterInfo, ScalarParameterValue->ParameterValue);
							continue;
						}
					}
					UDEditorFontParameterValue* FontParameterValue = Cast<UDEditorFontParameterValue>(Group.Parameters[ParameterIdx]);
					if (FontParameterValue)
					{
						if (FontParameterValue->bOverride)
						{
							ChildInstance->SetFontParameterValueEditorOnly(FontParameterValue->ParameterInfo, FontParameterValue->ParameterValue.FontValue, FontParameterValue->ParameterValue.FontPage);
							continue;
						}
					}

					UDEditorTextureParameterValue* TextureParameterValue = Cast<UDEditorTextureParameterValue>(Group.Parameters[ParameterIdx]);
					if (TextureParameterValue)
					{
						if (TextureParameterValue->bOverride)
						{
							ChildInstance->SetTextureParameterValueEditorOnly(TextureParameterValue->ParameterInfo, TextureParameterValue->ParameterValue);
							continue;
						}
					}
					UDEditorVectorParameterValue* VectorParameterValue = Cast<UDEditorVectorParameterValue>(Group.Parameters[ParameterIdx]);
					if (VectorParameterValue)
					{
						if (VectorParameterValue->bOverride)
						{
							ChildInstance->SetVectorParameterValueEditorOnly(VectorParameterValue->ParameterInfo, VectorParameterValue->ParameterValue);
							continue;
						}
					}

					UDEditorMaterialLayersParameterValue* LayersParameterValue = Cast<UDEditorMaterialLayersParameterValue>(Group.Parameters[ParameterIdx]);
					if (LayersParameterValue)
					{
						FMaterialLayersFunctions LayerValue = LayersParameterValue->ParameterValue;
						FGuid ExpressionIdValue = LayersParameterValue->ExpressionId;

						if (LayersParameterValue->bOverride)
						{
							FStaticMaterialLayersParameter* NewParameter =
								new(NewStaticParameters.MaterialLayersParameters) FStaticMaterialLayersParameter(LayersParameterValue->ParameterInfo, LayerValue, LayersParameterValue->bOverride, ExpressionIdValue);
						}
					}

					UDEditorStaticSwitchParameterValue* StaticSwitchParameterValue = Cast<UDEditorStaticSwitchParameterValue>(Group.Parameters[ParameterIdx]);
					if (StaticSwitchParameterValue)
					{
						bool SwitchValue = StaticSwitchParameterValue->ParameterValue;
						FGuid ExpressionIdValue = StaticSwitchParameterValue->ExpressionId;

						if (StaticSwitchParameterValue->bOverride)
						{
							FStaticSwitchParameter* NewParameter =
								new(NewStaticParameters.StaticSwitchParameters) FStaticSwitchParameter(StaticSwitchParameterValue->ParameterInfo, SwitchValue, StaticSwitchParameterValue->bOverride, ExpressionIdValue);
						}
					}

					// static component mask

					UDEditorStaticComponentMaskParameterValue* StaticComponentMaskParameterValue = Cast<UDEditorStaticComponentMaskParameterValue>(Group.Parameters[ParameterIdx]);
					if (StaticComponentMaskParameterValue)
					{
						bool MaskR = StaticComponentMaskParameterValue->ParameterValue.R;
						bool MaskG = StaticComponentMaskParameterValue->ParameterValue.G;
						bool MaskB = StaticComponentMaskParameterValue->ParameterValue.B;
						bool MaskA = StaticComponentMaskParameterValue->ParameterValue.A;
						FGuid ExpressionIdValue = StaticComponentMaskParameterValue->ExpressionId;

						if (StaticComponentMaskParameterValue->bOverride)
						{
							FStaticComponentMaskParameter* NewParameter = new(NewStaticParameters.StaticComponentMaskParameters)
								FStaticComponentMaskParameter(StaticComponentMaskParameterValue->ParameterInfo, MaskR, MaskG, MaskB, MaskA, StaticComponentMaskParameterValue->bOverride, ExpressionIdValue);
						}
					}
				}
			}

			ChildInstance->UpdateStaticPermutation(NewStaticParameters);
		}
	}
}


void FMaterialPropertyHelpers::TransitionAndCopyParameters(UMaterialInstanceConstant* ChildInstance, TArray<FEditorParameterGroup> &ParameterGroups)
{
	if (ChildInstance)
	{
		if (ChildInstance->IsTemplate(RF_ClassDefaultObject) == false)
		{
			ChildInstance->MarkPackageDirty();
			ChildInstance->ClearParameterValuesEditorOnly();
			//propagate changes to the base material so the instance will be updated if it has a static permutation resource
			FStaticParameterSet NewStaticParameters;
			for (int32 GroupIdx = 0; GroupIdx < ParameterGroups.Num(); GroupIdx++)
			{
				FEditorParameterGroup& Group = ParameterGroups[GroupIdx];
				for (int32 ParameterIdx = 0; ParameterIdx < Group.Parameters.Num(); ParameterIdx++)
				{
					if (Group.Parameters[ParameterIdx] == NULL)
					{
						continue;
					}
					UDEditorScalarParameterValue* ScalarParameterValue = Cast<UDEditorScalarParameterValue>(Group.Parameters[ParameterIdx]);
					if (ScalarParameterValue)
					{
						if (ScalarParameterValue->bOverride)
						{
							FMaterialParameterInfo TransitionedScalarInfo = FMaterialParameterInfo();
							TransitionedScalarInfo.Name = ScalarParameterValue->ParameterInfo.Name;
							ChildInstance->SetScalarParameterValueEditorOnly(TransitionedScalarInfo, ScalarParameterValue->ParameterValue);
							continue;
						}
					}
					UDEditorFontParameterValue* FontParameterValue = Cast<UDEditorFontParameterValue>(Group.Parameters[ParameterIdx]);
					if (FontParameterValue)
					{
						if (FontParameterValue->bOverride)
						{
							FMaterialParameterInfo TransitionedFontInfo = FMaterialParameterInfo();
							TransitionedFontInfo.Name = FontParameterValue->ParameterInfo.Name;
							ChildInstance->SetFontParameterValueEditorOnly(TransitionedFontInfo, FontParameterValue->ParameterValue.FontValue, FontParameterValue->ParameterValue.FontPage);
							continue;
						}
					}

					UDEditorTextureParameterValue* TextureParameterValue = Cast<UDEditorTextureParameterValue>(Group.Parameters[ParameterIdx]);
					if (TextureParameterValue)
					{
						if (TextureParameterValue->bOverride)
						{
							FMaterialParameterInfo TransitionedTextureInfo = FMaterialParameterInfo();
							TransitionedTextureInfo.Name = TextureParameterValue->ParameterInfo.Name;
							ChildInstance->SetTextureParameterValueEditorOnly(TransitionedTextureInfo, TextureParameterValue->ParameterValue);
							continue;
						}
					}
					UDEditorVectorParameterValue* VectorParameterValue = Cast<UDEditorVectorParameterValue>(Group.Parameters[ParameterIdx]);
					if (VectorParameterValue)
					{
						if (VectorParameterValue->bOverride)
						{
							FMaterialParameterInfo TransitionedVectorInfo = FMaterialParameterInfo();
							TransitionedVectorInfo.Name = VectorParameterValue->ParameterInfo.Name;
							ChildInstance->SetVectorParameterValueEditorOnly(TransitionedVectorInfo, VectorParameterValue->ParameterValue);
							continue;
						}
					}

					UDEditorStaticSwitchParameterValue* StaticSwitchParameterValue = Cast<UDEditorStaticSwitchParameterValue>(Group.Parameters[ParameterIdx]);
					if (StaticSwitchParameterValue)
					{
						bool SwitchValue = StaticSwitchParameterValue->ParameterValue;
						FGuid ExpressionIdValue = StaticSwitchParameterValue->ExpressionId;

						if (StaticSwitchParameterValue->bOverride)
						{
							FMaterialParameterInfo TransitionedSwitchInfo = FMaterialParameterInfo();
							TransitionedSwitchInfo.Name = StaticSwitchParameterValue->ParameterInfo.Name;
							FStaticSwitchParameter* NewParameter =
								new(NewStaticParameters.StaticSwitchParameters) FStaticSwitchParameter(TransitionedSwitchInfo, SwitchValue, StaticSwitchParameterValue->bOverride, ExpressionIdValue);
						}
					}

					// static component mask

					UDEditorStaticComponentMaskParameterValue* StaticComponentMaskParameterValue = Cast<UDEditorStaticComponentMaskParameterValue>(Group.Parameters[ParameterIdx]);
					if (StaticComponentMaskParameterValue)
					{
						bool MaskR = StaticComponentMaskParameterValue->ParameterValue.R;
						bool MaskG = StaticComponentMaskParameterValue->ParameterValue.G;
						bool MaskB = StaticComponentMaskParameterValue->ParameterValue.B;
						bool MaskA = StaticComponentMaskParameterValue->ParameterValue.A;
						FGuid ExpressionIdValue = StaticComponentMaskParameterValue->ExpressionId;

						if (StaticComponentMaskParameterValue->bOverride)
						{
							FMaterialParameterInfo TransitionedMaskInfo = FMaterialParameterInfo();
							TransitionedMaskInfo.Name = StaticComponentMaskParameterValue->ParameterInfo.Name;
							FStaticComponentMaskParameter* NewParameter = new(NewStaticParameters.StaticComponentMaskParameters)
								FStaticComponentMaskParameter(TransitionedMaskInfo, MaskR, MaskG, MaskB, MaskA, StaticComponentMaskParameterValue->bOverride, ExpressionIdValue);
						}
					}
				}
			}

			ChildInstance->UpdateStaticPermutation(NewStaticParameters);
		}
	}
}

FReply FMaterialPropertyHelpers::OnClickedSaveNewFunctionInstance(class UMaterialFunctionInterface* Object, class UMaterialInterface* PreviewMaterial, UObject* EditorObject)
{
	const FString DefaultSuffix = TEXT("_Inst");
	TArray<FEditorParameterGroup> ParameterGroups;
	UMaterialEditorInstanceConstant* MaterialInstanceEditor = Cast<UMaterialEditorInstanceConstant>(EditorObject);
	UMaterialInterface* FunctionPreviewMaterial = nullptr;
	if (MaterialInstanceEditor)
	{
		ParameterGroups = MaterialInstanceEditor->ParameterGroups;
		FunctionPreviewMaterial = MaterialInstanceEditor->SourceInstance;
	}
	UMaterialEditorPreviewParameters* MaterialEditor = Cast<UMaterialEditorPreviewParameters>(EditorObject);
	if (MaterialEditor)
	{
		ParameterGroups = MaterialEditor->ParameterGroups;
		FunctionPreviewMaterial = PreviewMaterial;
	}
	if (!MaterialEditor && !MaterialInstanceEditor)
	{
		return FReply::Unhandled();
	}

	if (Object)
	{
		UMaterial* EditedMaterial = Cast<UMaterial>(FunctionPreviewMaterial);
		if (EditedMaterial)
		{

			UMaterialInstanceConstant* ProxyMaterial = NewObject<UMaterialInstanceConstant>(GetTransientPackage(), NAME_None, RF_Transactional);
			ProxyMaterial->SetParentEditorOnly(EditedMaterial);
			ProxyMaterial->PreEditChange(NULL);
			ProxyMaterial->PostEditChange();
			CopyMaterialToInstance(ProxyMaterial, ParameterGroups);
			FunctionPreviewMaterial = ProxyMaterial;
		}
		// Create an appropriate and unique name 
		FString Name;
		FString PackageName;
		FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackageName, Name);

		
		UObject* Child;
		if (Object->GetMaterialFunctionUsage() == EMaterialFunctionUsage::MaterialLayer)
		{
			UMaterialFunctionMaterialLayerInstanceFactory* LayerFactory = NewObject<UMaterialFunctionMaterialLayerInstanceFactory>();
			LayerFactory->InitialParent = Object;
			Child = AssetToolsModule.Get().CreateAssetWithDialog(Name, FPackageName::GetLongPackagePath(PackageName), UMaterialFunctionMaterialLayerInstance::StaticClass(), LayerFactory);
		}
		else if (Object->GetMaterialFunctionUsage() == EMaterialFunctionUsage::MaterialLayerBlend)
		{
			UMaterialFunctionMaterialLayerBlendInstanceFactory* BlendFactory = NewObject<UMaterialFunctionMaterialLayerBlendInstanceFactory>();
			BlendFactory->InitialParent = Object;
			Child = AssetToolsModule.Get().CreateAssetWithDialog(Name, FPackageName::GetLongPackagePath(PackageName), UMaterialFunctionMaterialLayerBlendInstance::StaticClass(), BlendFactory);
		}
		else
		{
			UMaterialFunctionInstanceFactory* Factory = NewObject<UMaterialFunctionInstanceFactory>();
			Factory->InitialParent = Object;
			Child = AssetToolsModule.Get().CreateAssetWithDialog(Name, FPackageName::GetLongPackagePath(PackageName), UMaterialFunctionInstance::StaticClass(), Factory);
		}

		UMaterialFunctionInstance* ChildInstance = Cast<UMaterialFunctionInstance>(Child);
		if (ChildInstance)
		{
			if (ChildInstance->IsTemplate(RF_ClassDefaultObject) == false)
			{
				ChildInstance->MarkPackageDirty();
				ChildInstance->SetParent(Object);
				UMaterialInstance* EditedInstance = Cast<UMaterialInstance>(FunctionPreviewMaterial);
				if (EditedInstance)
				{
					ChildInstance->ScalarParameterValues = EditedInstance->ScalarParameterValues;
					ChildInstance->VectorParameterValues = EditedInstance->VectorParameterValues;
					ChildInstance->TextureParameterValues = EditedInstance->TextureParameterValues;
					ChildInstance->FontParameterValues = EditedInstance->FontParameterValues;

					const FStaticParameterSet& StaticParameters = EditedInstance->GetStaticParameters();
					ChildInstance->StaticSwitchParameterValues = StaticParameters.StaticSwitchParameters;
					ChildInstance->StaticComponentMaskParameterValues = StaticParameters.StaticComponentMaskParameters;
				}
			}
		}
	}
	return FReply::Handled();
}


FReply FMaterialPropertyHelpers::OnClickedSaveNewLayerInstance(class UMaterialFunctionInterface* Object, TSharedPtr<FStackSortedData> InSortedData)
{
	const FString DefaultSuffix = TEXT("_Inst");
	TArray<FEditorParameterGroup> ParameterGroups;
	UMaterialInterface* FunctionPreviewMaterial = nullptr;
	if (Object)
	{
		FunctionPreviewMaterial = Object->GetPreviewMaterial();
	}
	for (TSharedPtr<FStackSortedData> Group : InSortedData->Children)
	{
		FEditorParameterGroup DuplicatedGroup = FEditorParameterGroup();
		DuplicatedGroup.GroupAssociation = Group->Group.GroupAssociation;
		DuplicatedGroup.GroupName = Group->Group.GroupName;
		DuplicatedGroup.GroupSortPriority = Group->Group.GroupSortPriority;
		for (UDEditorParameterValue* Parameter : Group->Group.Parameters)
		{
			if (Parameter->ParameterInfo.Index == InSortedData->ParameterInfo.Index)
			{
				DuplicatedGroup.Parameters.Add(Parameter);
			}
		}
		ParameterGroups.Add(DuplicatedGroup);
	}

	if (Object)
	{
		UMaterialInterface* EditedMaterial = Cast<UMaterialInterface>(FunctionPreviewMaterial);
		if (EditedMaterial)
		{
			UMaterialInstanceConstant* ProxyMaterial = NewObject<UMaterialInstanceConstant>(GetTransientPackage(), NAME_None, RF_Transactional);
			ProxyMaterial->SetParentEditorOnly(EditedMaterial);
			ProxyMaterial->PreEditChange(NULL);
			ProxyMaterial->PostEditChange();
			TransitionAndCopyParameters(ProxyMaterial, ParameterGroups);
			FunctionPreviewMaterial = ProxyMaterial;
		}
		// Create an appropriate and unique name 
		FString Name;
		FString PackageName;
		FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackageName, Name);


		UObject* Child;
		if (Object->GetMaterialFunctionUsage() == EMaterialFunctionUsage::MaterialLayer)
		{
			UMaterialFunctionMaterialLayerInstanceFactory* LayerFactory = NewObject<UMaterialFunctionMaterialLayerInstanceFactory>();
			LayerFactory->InitialParent = Object;
			Child = AssetToolsModule.Get().CreateAssetWithDialog(Name, FPackageName::GetLongPackagePath(PackageName), UMaterialFunctionMaterialLayerInstance::StaticClass(), LayerFactory);
		}
		else if (Object->GetMaterialFunctionUsage() == EMaterialFunctionUsage::MaterialLayerBlend)
		{
			UMaterialFunctionMaterialLayerBlendInstanceFactory* BlendFactory = NewObject<UMaterialFunctionMaterialLayerBlendInstanceFactory>();
			BlendFactory->InitialParent = Object;
			Child = AssetToolsModule.Get().CreateAssetWithDialog(Name, FPackageName::GetLongPackagePath(PackageName), UMaterialFunctionMaterialLayerBlendInstance::StaticClass(), BlendFactory);
		}
		else
		{
			UMaterialFunctionInstanceFactory* Factory = NewObject<UMaterialFunctionInstanceFactory>();
			Factory->InitialParent = Object;
			Child = AssetToolsModule.Get().CreateAssetWithDialog(Name, FPackageName::GetLongPackagePath(PackageName), UMaterialFunctionInstance::StaticClass(), Factory);
		}

		UMaterialFunctionInstance* ChildInstance = Cast<UMaterialFunctionInstance>(Child);
		if (ChildInstance)
		{
			if (ChildInstance->IsTemplate(RF_ClassDefaultObject) == false)
			{
				ChildInstance->MarkPackageDirty();
				ChildInstance->SetParent(Object);
				UMaterialInstance* EditedInstance = Cast<UMaterialInstance>(FunctionPreviewMaterial);
				if (EditedInstance)
				{
					ChildInstance->ScalarParameterValues = EditedInstance->ScalarParameterValues;
					ChildInstance->VectorParameterValues = EditedInstance->VectorParameterValues;
					ChildInstance->TextureParameterValues = EditedInstance->TextureParameterValues;
					ChildInstance->FontParameterValues = EditedInstance->FontParameterValues;

					const FStaticParameterSet& StaticParameters = EditedInstance->GetStaticParameters();
					ChildInstance->StaticSwitchParameterValues = StaticParameters.StaticSwitchParameters;
					ChildInstance->StaticComponentMaskParameterValues = StaticParameters.StaticComponentMaskParameters;
				}
			}
		}
	}
	return FReply::Handled();
}

bool FMaterialPropertyHelpers::IsOverriddenExpression(UDEditorParameterValue* Parameter)
{
	return Parameter->bOverride != 0;
}

ECheckBoxState FMaterialPropertyHelpers::IsOverriddenExpressionCheckbox(UDEditorParameterValue* Parameter)
{
	return IsOverriddenExpression(Parameter) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FMaterialPropertyHelpers::OnOverrideParameter(bool NewValue, class UDEditorParameterValue* Parameter, UMaterialEditorInstanceConstant* MaterialEditorInstance)
{
	const FScopedTransaction Transaction( LOCTEXT( "OverrideParameter", "Override Parameter" ) );
	Parameter->Modify();
	Parameter->bOverride = NewValue;

	// Fire off a dummy event to the material editor instance, so it knows to update the material, then refresh the viewports.
	FPropertyChangedEvent OverrideEvent(NULL);
	MaterialEditorInstance->PostEditChangeProperty( OverrideEvent );
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
	FEditorSupportDelegates::UpdateUI.Broadcast();
}

FText FMaterialPropertyHelpers::GetParameterExpressionDescription(UDEditorParameterValue* Parameter, UObject* MaterialEditorInstance)
{
	UMaterial* BaseMaterial = nullptr;

	UMaterialEditorInstanceConstant* MaterialInstanceEditor = Cast<UMaterialEditorInstanceConstant>(MaterialEditorInstance);
	if (MaterialInstanceEditor)
	{
		BaseMaterial = MaterialInstanceEditor->SourceInstance->GetMaterial();;
	}
	UMaterialEditorPreviewParameters* MaterialEditor = Cast<UMaterialEditorPreviewParameters>(MaterialEditorInstance);
	if (MaterialEditor)
	{
		BaseMaterial = MaterialEditor->OriginalMaterial;
	}

	// TODO: This needs to support functions added by SourceInstance layers
	if ( BaseMaterial )
	{
		UMaterialExpression* MaterialExpression = BaseMaterial->FindExpressionByGUID<UMaterialExpression>(Parameter->ExpressionId);

		if (MaterialExpression)
		{
			return FText::FromString(MaterialExpression->Desc);
		}
	}

	return FText::GetEmpty();
}

void FMaterialPropertyHelpers::ResetToDefault(TSharedPtr<IPropertyHandle> PropertyHandle, class UDEditorParameterValue* Parameter, UMaterialEditorInstanceConstant* MaterialEditorInstance)
{
	const FScopedTransaction Transaction( LOCTEXT( "ResetToDefault", "Reset To Default" ) );
	Parameter->Modify();

	const FMaterialParameterInfo& ParameterInfo = Parameter->ParameterInfo;
	
	UDEditorScalarParameterValue* ScalarParam = Cast<UDEditorScalarParameterValue>(Parameter);
	UDEditorVectorParameterValue* VectorParam = Cast<UDEditorVectorParameterValue>(Parameter);
	UDEditorTextureParameterValue* TextureParam = Cast<UDEditorTextureParameterValue>(Parameter);
	UDEditorFontParameterValue* FontParam = Cast<UDEditorFontParameterValue>(Parameter);
	UDEditorStaticSwitchParameterValue* SwitchParam = Cast<UDEditorStaticSwitchParameterValue>(Parameter);
	UDEditorStaticComponentMaskParameterValue* CompMaskParam = Cast<UDEditorStaticComponentMaskParameterValue>(Parameter);
	UDEditorMaterialLayersParameterValue* LayersParam = Cast<UDEditorMaterialLayersParameterValue>(Parameter);

	if (ScalarParam)
	{
		float OutValue;
		if (MaterialEditorInstance->SourceInstance->GetScalarParameterDefaultValue(ParameterInfo, OutValue))
		{
			ScalarParam->ParameterValue = OutValue;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (VectorParam)
	{
		FLinearColor OutValue;
		if (MaterialEditorInstance->SourceInstance->GetVectorParameterDefaultValue(ParameterInfo, OutValue))
		{
			VectorParam->ParameterValue = OutValue;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (TextureParam)
	{
		UTexture* OutValue;
		if (MaterialEditorInstance->SourceInstance->GetTextureParameterDefaultValue(ParameterInfo, OutValue))
		{
			TextureParam->ParameterValue = OutValue;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (FontParam)
	{
		UFont* OutFontValue;
		int32 OutFontPage;
		if (MaterialEditorInstance->SourceInstance->GetFontParameterDefaultValue(ParameterInfo, OutFontValue, OutFontPage))
		{
			FontParam->ParameterValue.FontValue = OutFontValue;
			FontParam->ParameterValue.FontPage = OutFontPage;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (SwitchParam)
	{
		bool OutValue;
		FGuid TempGuid(0,0,0,0);
		if (MaterialEditorInstance->SourceInstance->GetStaticSwitchParameterDefaultValue(ParameterInfo, OutValue, TempGuid))
		{
			SwitchParam->ParameterValue = OutValue;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (CompMaskParam)
	{
		bool OutValue[4];
		FGuid TempGuid(0,0,0,0);
		if (MaterialEditorInstance->SourceInstance->GetStaticComponentMaskParameterDefaultValue(ParameterInfo, OutValue[0], OutValue[1], OutValue[2], OutValue[3], TempGuid))
		{
			CompMaskParam->ParameterValue.R = OutValue[0];
			CompMaskParam->ParameterValue.G = OutValue[1];
			CompMaskParam->ParameterValue.B = OutValue[2];
			CompMaskParam->ParameterValue.A = OutValue[3];
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
}

void FMaterialPropertyHelpers::ResetLayerAssetToDefault(TSharedPtr<IPropertyHandle> PropertyHandle,  class UDEditorParameterValue* InParameter, TEnumAsByte<EMaterialParameterAssociation> InAssociation, int32 Index, UMaterialEditorInstanceConstant* MaterialEditorInstance)
{
	
	const FScopedTransaction Transaction(LOCTEXT("ResetToDefault", "Reset To Default"));
	InParameter->Modify();
	
	const FMaterialParameterInfo& ParameterInfo = InParameter->ParameterInfo;
	UDEditorMaterialLayersParameterValue* LayersParam = Cast<UDEditorMaterialLayersParameterValue>(InParameter);

	if (LayersParam)
	{
		FMaterialLayersFunctions LayersValue;
		FGuid TempGuid(0, 0, 0, 0);
		if (MaterialEditorInstance->Parent->GetMaterialLayersParameterValue(ParameterInfo, LayersValue, TempGuid))
		{
			FMaterialLayersFunctions StoredValue = LayersParam->ParameterValue;
			if (InAssociation == EMaterialParameterAssociation::BlendParameter)
			{
				if (Index < LayersValue.Blends.Num())
				{
					StoredValue.Blends[Index] = LayersValue.Blends[Index];
				}
				else
				{
					StoredValue.Blends[Index] = nullptr;
					MaterialEditorInstance->StoredBlendPreviews[Index] = nullptr;
				}
			}
			else if (InAssociation == EMaterialParameterAssociation::LayerParameter)
			{
				if (Index < LayersValue.Layers.Num())
				{
					StoredValue.Layers[Index] = LayersValue.Layers[Index];
				}
				else
				{
					StoredValue.Layers[Index] = nullptr;
					MaterialEditorInstance->StoredLayerPreviews[Index] = nullptr;
				}
			}
			LayersParam->ParameterValue = StoredValue;
			LayersParam->ParameterValue.UpdateStaticPermutationString();
		}
	}

	FPropertyChangedEvent OverrideEvent(NULL);
	MaterialEditorInstance->PostEditChangeProperty(OverrideEvent);
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
	FEditorSupportDelegates::UpdateUI.Broadcast();
	
}

bool FMaterialPropertyHelpers::ShouldLayerAssetShowResetToDefault(TSharedPtr<IPropertyHandle> PropertyHandle, TSharedPtr<FStackSortedData> InParameterData, UMaterialInterface* InMaterial)
{
	if (!InParameterData->Parameter)
	{
		return false;
	}

	TArray<class UMaterialFunctionInterface*> StoredAssets;
	TArray<class UMaterialFunctionInterface*> ParentAssets;

	const FMaterialParameterInfo& ParameterInfo = InParameterData->Parameter->ParameterInfo;
	int32 Index = InParameterData->ParameterInfo.Index;
	UDEditorMaterialLayersParameterValue* LayersParam = Cast<UDEditorMaterialLayersParameterValue>(InParameterData->Parameter);
	if (LayersParam)
	{
		FMaterialLayersFunctions LayersValue;
		FGuid TempGuid(0, 0, 0, 0);
		if (InMaterial->GetMaterialLayersParameterValue(ParameterInfo, LayersValue, TempGuid))
		{
			FMaterialLayersFunctions StoredValue = LayersParam->ParameterValue;
			if (InParameterData->ParameterInfo.Association == EMaterialParameterAssociation::BlendParameter)
			{
				StoredAssets = StoredValue.Blends;
				ParentAssets = LayersValue.Blends;
	
			}
			else if (InParameterData->ParameterInfo.Association == EMaterialParameterAssociation::LayerParameter)
			{
				StoredAssets = StoredValue.Layers;
				ParentAssets = LayersValue.Layers;
			}

			// Compare to the parent MaterialFunctionInterface array
			if (Index < ParentAssets.Num())
			{
				if (StoredAssets[Index] == ParentAssets[Index])
				{
					return false;
				}
				else
				{
					return true;
				}
			}
			else if (StoredAssets[Index] != nullptr)
			{
				return true;
			}
		}
	}
	return false;
}

bool FMaterialPropertyHelpers::ShouldShowResetToDefault(TSharedPtr<IPropertyHandle> PropertyHandle, UDEditorParameterValue* InParameter, UMaterialEditorInstanceConstant* MaterialEditorInstance)
{
	const FMaterialParameterInfo& ParameterInfo = InParameter->ParameterInfo;

	UDEditorFontParameterValue* FontParam = Cast<UDEditorFontParameterValue>(InParameter);
	UDEditorScalarParameterValue* ScalarParam = Cast<UDEditorScalarParameterValue>(InParameter);
	UDEditorStaticComponentMaskParameterValue* CompMaskParam = Cast<UDEditorStaticComponentMaskParameterValue>(InParameter);
	UDEditorStaticSwitchParameterValue* SwitchParam = Cast<UDEditorStaticSwitchParameterValue>(InParameter);
	UDEditorTextureParameterValue* TextureParam = Cast<UDEditorTextureParameterValue>(InParameter);
	UDEditorVectorParameterValue* VectorParam = Cast<UDEditorVectorParameterValue>(InParameter);

	if (ScalarParam)
	{
		float OutValue;
		if (MaterialEditorInstance->SourceInstance->GetScalarParameterDefaultValue(ParameterInfo, OutValue))
		{
			if (ScalarParam->ParameterValue != OutValue)
			{
				return true;
			}
		}
	}
	else if (FontParam)
	{
		UFont* OutFontValue;
		int32 OutFontPage;
		if (MaterialEditorInstance->SourceInstance->GetFontParameterDefaultValue(ParameterInfo, OutFontValue, OutFontPage))
		{
			if (FontParam->ParameterValue.FontValue != OutFontValue ||
				FontParam->ParameterValue.FontPage != OutFontPage)
			{
				return true;
			}
		}
	}
	else if (TextureParam)
	{
		UTexture* OutValue;
		if (MaterialEditorInstance->SourceInstance->GetTextureParameterDefaultValue(ParameterInfo, OutValue))
		{
			if (TextureParam->ParameterValue != OutValue)
			{
				return true;
			}
		}
	}
	else if (VectorParam)
	{
		FLinearColor OutValue;
		if (MaterialEditorInstance->SourceInstance->GetVectorParameterDefaultValue(ParameterInfo, OutValue))
		{
			if (VectorParam->ParameterValue != OutValue)
			{
				return true;
			}
		}
	}
	else if (SwitchParam)
	{
		bool OutValue;
		FGuid TempGuid(0, 0, 0, 0);
		if (MaterialEditorInstance->SourceInstance->GetStaticSwitchParameterDefaultValue(ParameterInfo, OutValue, TempGuid))
		{
			if (SwitchParam->ParameterValue != OutValue)
			{
				return true;
			}
		}
	}
	else if (CompMaskParam)
	{
		bool OutValue[4];
		FGuid TempGuid(0, 0, 0, 0);
		if (MaterialEditorInstance->SourceInstance->GetStaticComponentMaskParameterDefaultValue(ParameterInfo, OutValue[0], OutValue[1], OutValue[2], OutValue[3], TempGuid))
		{
			if (CompMaskParam->ParameterValue.R != OutValue[0] ||
				CompMaskParam->ParameterValue.G != OutValue[1] ||
				CompMaskParam->ParameterValue.B != OutValue[2] ||
				CompMaskParam->ParameterValue.A != OutValue[3])
			{
				return true;
			}
		}
	}
	return false;
}

FEditorParameterGroup&  FMaterialPropertyHelpers::GetParameterGroup(UMaterial* InMaterial, FName& ParameterGroup, TArray<struct FEditorParameterGroup>& ParameterGroups)
{
	if (ParameterGroup == TEXT(""))
	{
		ParameterGroup = TEXT("None");
	}
	for (int32 i = 0; i < ParameterGroups.Num(); i++)
	{
		FEditorParameterGroup& Group = ParameterGroups[i];
		if (Group.GroupName == ParameterGroup)
		{
			return Group;
		}
	}
	int32 ind = ParameterGroups.AddZeroed(1);
	FEditorParameterGroup& Group = ParameterGroups[ind];
	Group.GroupName = ParameterGroup;
	UMaterial* ParentMaterial = InMaterial;
	int32 NewSortPriority;
	if (ParentMaterial->GetGroupSortPriority(ParameterGroup.ToString(), NewSortPriority))
	{
		Group.GroupSortPriority = NewSortPriority;
	}
	else
	{
		Group.GroupSortPriority = 0;
	}
	Group.GroupAssociation = EMaterialParameterAssociation::GlobalParameter;

	return Group;
}

void FMaterialPropertyHelpers::GetVectorChannelMaskComboBoxStrings(TArray<TSharedPtr<FString>>& OutComboBoxStrings, TArray<TSharedPtr<SToolTip>>& OutToolTips, TArray<bool>& OutRestrictedItems)
{
	const UEnum* ChannelEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EChannelMaskParameterColor"));
	check(ChannelEnum);

	// Add RGBA string options (Note: Exclude the "::Max" entry)
	const int32 NumEnums = ChannelEnum->NumEnums() - 1;
	for (int32 Entry = 0; Entry < NumEnums; ++Entry)
	{
		FText EnumName = ChannelEnum->GetDisplayNameTextByIndex(Entry);

		OutComboBoxStrings.Add(MakeShared<FString>(EnumName.ToString()));
		OutToolTips.Add(SNew(SToolTip).Text(EnumName));
		OutRestrictedItems.Add(false);
	}
}

FString FMaterialPropertyHelpers::GetVectorChannelMaskValue(UDEditorParameterValue* InParameter)
{
	UDEditorVectorParameterValue* VectorParam = Cast<UDEditorVectorParameterValue>(InParameter);
	check(VectorParam && VectorParam->bIsUsedAsChannelMask);

	const UEnum* ChannelEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EChannelMaskParameterColor"));
	check(ChannelEnum);

	// Convert from vector to RGBA string
	int64 ChannelType = 0;

	if (VectorParam->ParameterValue.R > 0.0f)
	{
		ChannelType = EChannelMaskParameterColor::Red;
	}
	else if (VectorParam->ParameterValue.G > 0.0f)
	{
		ChannelType = EChannelMaskParameterColor::Green;
	}
	else if (VectorParam->ParameterValue.B > 0.0f)
	{
		ChannelType = EChannelMaskParameterColor::Blue;
	}
	else
	{
		ChannelType = EChannelMaskParameterColor::Alpha;
	}

	return ChannelEnum->GetDisplayNameTextByValue(ChannelType).ToString();
}

void FMaterialPropertyHelpers::SetVectorChannelMaskValue(const FString& StringValue, TSharedPtr<IPropertyHandle> PropertyHandle, UDEditorParameterValue* InParameter, UObject* MaterialEditorInstance)
{
	UDEditorVectorParameterValue* VectorParam = Cast<UDEditorVectorParameterValue>(InParameter);
	check(VectorParam && VectorParam->bIsUsedAsChannelMask);

	const UEnum* ChannelEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EChannelMaskParameterColor"));
	check(ChannelEnum);

	// Convert from RGBA string to vector
	int64 ChannelValue = ChannelEnum->GetValueByNameString(StringValue);
	FLinearColor NewValue;

	switch (ChannelValue)
	{
	case EChannelMaskParameterColor::Red:
		NewValue = FLinearColor(1.0f, 0.0f, 0.0f, 0.0f); break;
	case EChannelMaskParameterColor::Green:
		NewValue = FLinearColor(0.0f, 1.0f, 0.0f, 0.0f); break;
	case EChannelMaskParameterColor::Blue:
		NewValue = FLinearColor(0.0f, 0.0f, 1.0f, 0.0f); break;
	default:
		NewValue = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
	}

	// If changed, propagate the update
	if (VectorParam->ParameterValue != NewValue)
	{
		const FScopedTransaction Transaction(LOCTEXT("SetVectorChannelMaskValue", "Set Vector Channel Mask Value"));
		VectorParam->Modify();

		PropertyHandle->NotifyPreChange();
		VectorParam->ParameterValue = NewValue;

		UMaterialEditorInstanceConstant* MaterialInstanceEditor = Cast<UMaterialEditorInstanceConstant>(MaterialEditorInstance);
		if (MaterialInstanceEditor)
		{
			MaterialInstanceEditor->CopyToSourceInstance();
		}

		PropertyHandle->NotifyPostChange();
	}
}

TArray<UFactory*> FMaterialPropertyHelpers::GetAssetFactories(EMaterialParameterAssociation AssetType)
{
	TArray<UFactory*> NewAssetFactories;
	switch (AssetType)
	{
	case LayerParameter:
	{
	//	NewAssetFactories.Add(NewObject<UMaterialFunctionMaterialLayerFactory>());
		break;
	}
	case BlendParameter:
	{
	//	NewAssetFactories.Add(NewObject<UMaterialFunctionMaterialLayerBlendFactory>());
		break;
	}
	case GlobalParameter:
		break;
	default:
		break;
	}
	
	return NewAssetFactories;
}

#undef LOCTEXT_NAMESPACE

