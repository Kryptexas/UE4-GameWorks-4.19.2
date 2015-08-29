// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEditorPrivatePCH.h"
#include "NiagaraEditorModule.h"
#include "EditorSupportDelegates.h"
#include "UnrealEd.h"
#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"
#include "NiagaraEffectEditor.h"
#include "NiagaraEmitterPropertiesDetailsCustomization.h"
#include "ScopedTransaction.h"
#include "DetailWidgetRow.h"
#include "AssetData.h"

#define LOCTEXT_NAMESPACE "NiagaraEffectEditor"

TSharedRef<IDetailCustomization> FNiagaraEmitterPropertiesDetails::MakeInstance(TWeakObjectPtr<UNiagaraEmitterProperties> EmitterProperties)
{
	return MakeShareable(new FNiagaraEmitterPropertiesDetails(EmitterProperties));
}

FNiagaraEmitterPropertiesDetails::FNiagaraEmitterPropertiesDetails(TWeakObjectPtr<UNiagaraEmitterProperties> EmitterProperties)
: EmitterProps(EmitterProperties)
{
}

void FNiagaraEmitterPropertiesDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	FName EmitterCategoryName = TEXT("Emitter");
	IDetailCategoryBuilder& EmitterCategory = DetailLayout.EditCategory(EmitterCategoryName);

	TSharedRef<IPropertyHandle> EmitterName = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UNiagaraEmitterProperties, EmitterName));
	TSharedRef<IPropertyHandle> EmitterEnabled = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UNiagaraEmitterProperties, bIsEnabled));

	DetailLayout.HideProperty(EmitterName);
	DetailLayout.HideProperty(EmitterEnabled);
	
	TSharedRef<IPropertyHandle> UpdateScriptProps = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UNiagaraEmitterProperties, UpdateScriptProps));
	TSharedRef<IPropertyHandle> SpawnScriptProps = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UNiagaraEmitterProperties, SpawnScriptProps));

	BuildScriptProperties(DetailLayout, SpawnScriptProps, TEXT("Spawn Script"), LOCTEXT("SpawnScriptDisplayName", "Spawn Script"));
	BuildScriptProperties(DetailLayout, UpdateScriptProps, TEXT("Update Script"), LOCTEXT("UpdateScriptDisplayName", "Update Script"));
}

void FNiagaraEmitterPropertiesDetails::BuildScriptProperties(IDetailLayoutBuilder& DetailLayout, TSharedRef<IPropertyHandle> ScriptPropsHandle, FName Name, FText DisplayName)
{
	TFunction<void(TSharedRef<IPropertyHandle>)> HideAllBelow = [&](TSharedRef<IPropertyHandle> Prop)
	{
		DetailLayout.HideProperty(Prop);

		//Don't hide sub properties for data objects.
		if (Prop->GetProperty()->GetName() == GET_MEMBER_NAME_CHECKED(FNiagaraConstants, DataObjectConstants).ToString())
			return; 

		uint32 NumChildren = 0;
		if (Prop->GetNumChildren(NumChildren) == FPropertyAccess::Success)
		{
			for (uint32 i = 0; i < NumChildren; ++i)
			{
				TSharedPtr<IPropertyHandle> Child = Prop->GetChildHandle(i);
				if (Child.IsValid())
				{
					HideAllBelow(Child.ToSharedRef());
				}				
			}
		}
	};
	HideAllBelow(ScriptPropsHandle);

	IDetailCategoryBuilder& ScriptCategory = DetailLayout.EditCategory(Name, DisplayName);

	//Script
	TSharedPtr<IPropertyHandle> ScriptHandle = ScriptPropsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNiagaraEmitterScriptProperties, Script));
	ScriptCategory.AddProperty(ScriptHandle);

	//Constants
	const bool bGenerateHeader = false;
	const bool bDisplayResetToDefault = false;
	TSharedPtr<IPropertyHandle> Constants = ScriptPropsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNiagaraEmitterScriptProperties, ExternalConstants));
	//Scalar Constants.
	TSharedPtr<IPropertyHandle> ScalarConstants = ScriptPropsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNiagaraConstants, ScalarConstants));
	TSharedRef<FDetailArrayBuilder> ScalarConstantsBuilder = MakeShareable(new FDetailArrayBuilder(ScalarConstants.ToSharedRef(), bGenerateHeader, bDisplayResetToDefault));
	ScalarConstantsBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FNiagaraEmitterPropertiesDetails::OnGenerateScalarConstantEntry));
	ScriptCategory.AddCustomBuilder(ScalarConstantsBuilder);
	//Vector Constants.
	TSharedPtr<IPropertyHandle> VectorConstants = ScriptPropsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNiagaraConstants, VectorConstants));
	TSharedRef<FDetailArrayBuilder> VectorConstantsBuilder = MakeShareable(new FDetailArrayBuilder(VectorConstants.ToSharedRef(), bGenerateHeader, bDisplayResetToDefault));
	VectorConstantsBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FNiagaraEmitterPropertiesDetails::OnGenerateVectorConstantEntry));
	ScriptCategory.AddCustomBuilder(VectorConstantsBuilder);
	//Matrix Constants.
	TSharedPtr<IPropertyHandle> MatrixConstants = ScriptPropsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNiagaraConstants, MatrixConstants));
	TSharedRef<FDetailArrayBuilder> MatrixConstantsBuilder = MakeShareable(new FDetailArrayBuilder(MatrixConstants.ToSharedRef(), bGenerateHeader, bDisplayResetToDefault));
	MatrixConstantsBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FNiagaraEmitterPropertiesDetails::OnGenerateMatrixConstantEntry));
	ScriptCategory.AddCustomBuilder(MatrixConstantsBuilder);
	//DataObject Constants.
	TSharedPtr<IPropertyHandle> DataObjectConstants = ScriptPropsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNiagaraConstants, DataObjectConstants));
	TSharedRef<FDetailArrayBuilder> DataObjectConstantsBuilder = MakeShareable(new FDetailArrayBuilder(DataObjectConstants.ToSharedRef(), bGenerateHeader, bDisplayResetToDefault));
	DataObjectConstantsBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FNiagaraEmitterPropertiesDetails::OnGenerateDataObjectConstantEntry));
	ScriptCategory.AddCustomBuilder(DataObjectConstantsBuilder);

	//TODO: Events.
}

void FNiagaraEmitterPropertiesDetails::OnGenerateScalarConstantEntry(TSharedRef<IPropertyHandle> ElementProperty, int32 ElementIndex, IDetailChildrenBuilder& ChildrenBuilder)
{
	TSharedPtr<IPropertyHandle> ValueProperty = ElementProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNiagaraConstants_Float, Value));
	TSharedPtr<IPropertyHandle> NameProperty = ElementProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNiagaraConstants_Float, Name));

	FName DisplayName;
	NameProperty->GetValue(DisplayName);
	//Don't display system constants
	if (UNiagaraComponent::GetSystemConstants().Find(FNiagaraVariableInfo(DisplayName, ENiagaraDataType::Scalar)) == INDEX_NONE)
	{
		ChildrenBuilder.AddChildProperty(ValueProperty.ToSharedRef()).DisplayName(FText::FromName(DisplayName));
	}
}

void FNiagaraEmitterPropertiesDetails::OnGenerateVectorConstantEntry(TSharedRef<IPropertyHandle> ElementProperty, int32 ElementIndex, IDetailChildrenBuilder& ChildrenBuilder)
{
	TSharedPtr<IPropertyHandle> ValueProperty = ElementProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNiagaraConstants_Vector, Value));
	TSharedPtr<IPropertyHandle> NameProperty = ElementProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNiagaraConstants_Vector, Name));

	FName DisplayName;
	NameProperty->GetValue(DisplayName);
	//Don't display system constants
	if (UNiagaraComponent::GetSystemConstants().Find(FNiagaraVariableInfo(DisplayName, ENiagaraDataType::Vector)) == INDEX_NONE)
	{
		ChildrenBuilder.AddChildProperty(ValueProperty.ToSharedRef()).DisplayName(FText::FromName(DisplayName));
	}
}

void FNiagaraEmitterPropertiesDetails::OnGenerateMatrixConstantEntry(TSharedRef<IPropertyHandle> ElementProperty, int32 ElementIndex, IDetailChildrenBuilder& ChildrenBuilder)
{
	TSharedPtr<IPropertyHandle> ValueProperty = ElementProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNiagaraConstants_Matrix, Value));
	TSharedPtr<IPropertyHandle> NameProperty = ElementProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNiagaraConstants_Matrix, Name));

	FName DisplayName;
	NameProperty->GetValue(DisplayName);
	//Don't display system constants
	if (UNiagaraComponent::GetSystemConstants().Find(FNiagaraVariableInfo(DisplayName, ENiagaraDataType::Matrix)) == INDEX_NONE)
	{
		ChildrenBuilder.AddChildProperty(ValueProperty.ToSharedRef()).DisplayName(FText::FromName(DisplayName));
	}
}

void FNiagaraEmitterPropertiesDetails::OnGenerateDataObjectConstantEntry(TSharedRef<IPropertyHandle> ElementProperty, int32 ElementIndex, IDetailChildrenBuilder& ChildrenBuilder)
{
	TSharedPtr<IPropertyHandle> ValueProperty = ElementProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNiagaraConstants_DataObject, Value));
	TSharedPtr<IPropertyHandle> NameProperty = ElementProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNiagaraConstants_DataObject, Name));

	FName DisplayName;
	NameProperty->GetValue(DisplayName);
	//Don't display system constants
	if (UNiagaraComponent::GetSystemConstants().Find(FNiagaraVariableInfo(DisplayName, ENiagaraDataType::Curve)) == INDEX_NONE)
	{
		ChildrenBuilder.AddChildProperty(ValueProperty.ToSharedRef()).DisplayName(FText::FromName(DisplayName));
	}
}

#undef LOCTEXT_NAMESPACE

