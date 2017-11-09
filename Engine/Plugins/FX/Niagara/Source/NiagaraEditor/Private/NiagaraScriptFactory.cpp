// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/ConfigCacheIni.h"
#include "Modules/ModuleManager.h"
#include "NiagaraCommon.h"
#include "NiagaraEditorModule.h"
#include "NiagaraScript.h"
#include "EdGraph/EdGraph.h"
#include "NiagaraGraph.h"
#include "NiagaraScriptSource.h"
#include "NiagaraScriptFactoryNew.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraEditorSettings.h"
#include "IAssetTypeActions.h"
#include "AssetToolsModule.h"
#include "AssetTypeActions/AssetTypeActions_NiagaraScript.h"

#define LOCTEXT_NAMESPACE "NiagaraScriptFactory"

UNiagaraScriptFactoryNew::UNiagaraScriptFactoryNew(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	SupportedClass = UNiagaraScript::StaticClass();
	bCreateNew = false;
	bEditAfterNew = true;
	bCreateNew = true;
}

UObject* UNiagaraScriptFactoryNew::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	check(Class->IsChildOf(UNiagaraScript::StaticClass()));
	
	const UNiagaraEditorSettings* Settings = GetDefault<UNiagaraEditorSettings>();
	check(Settings);

	UNiagaraScript* NewScript;
	const FStringAssetReference& SettingDefaultScript = GetDefaultScriptFromSettings(Settings);
	if (UNiagaraScript* Default = Cast<UNiagaraScript>(SettingDefaultScript.TryLoad()))
	{
		NewScript = Cast<UNiagaraScript>(StaticDuplicateObject(Default, InParent, Name, Flags, Class));
	}
	else
	{
		NewScript = NewObject<UNiagaraScript>(InParent, Class, Name, Flags | RF_Transactional);
		InitializeScript(NewScript);
	}

	return NewScript;
}

const FStringAssetReference& UNiagaraScriptFactoryNew::GetDefaultScriptFromSettings(const UNiagaraEditorSettings* Settings)
{
	switch (GetScriptUsage())
	{
	case ENiagaraScriptUsage::Module:
		if (Settings->DefaultModuleScript.IsValid())
		{
			return Settings->DefaultModuleScript;
		}
		break;
	case ENiagaraScriptUsage::DynamicInput:
		if (Settings->DefaultDynamicInputScript.IsValid())
		{
			return Settings->DefaultDynamicInputScript;
		}
		break;
	case ENiagaraScriptUsage::Function:
		if (Settings->DefaultFunctionScript.IsValid())
		{
			return Settings->DefaultFunctionScript;
		}
		break;
	}
	return Settings->DefaultScript;
}

FText UNiagaraScriptFactoryNew::GetDisplayName() const
{
	// Get the displayname for this factory from the action type.
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

	UClass* LocalSupportedClass = GetSupportedClass();
	if (LocalSupportedClass)
	{
		TArray<TWeakPtr<IAssetTypeActions>> AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsListForClass(LocalSupportedClass);
		for (auto& CurrentAssetTypeAction : AssetTypeActions)
		{
			TSharedPtr<FAssetTypeActions_NiagaraScript> AssetTypeAction = StaticCastSharedPtr<FAssetTypeActions_NiagaraScript>(CurrentAssetTypeAction.Pin());
			if (AssetTypeAction.IsValid() && AssetTypeAction->GetTypeName() == GetAssetTypeActionName())
			{
				FText Name = AssetTypeAction->GetName();
				if (!Name.IsEmpty())
				{
					return Name;
				}
			}
		}
	}

	// Factories that have no supported class have no display name.
	return FText();
}

void UNiagaraScriptFactoryNew::InitializeScript(UNiagaraScript* NewScript)
{
	if(NewScript != NULL)
	{
		UNiagaraScriptSource* Source = NewObject<UNiagaraScriptSource>(NewScript, NAME_None, RF_Transactional);
		if(Source)
		{
			UNiagaraGraph* CreatedGraph = NewObject<UNiagaraGraph>(Source, NAME_None, RF_Transactional);
			Source->NodeGraph = CreatedGraph;

			FGraphNodeCreator<UNiagaraNodeOutput> OutputNodeCreator(*CreatedGraph);
			UNiagaraNodeOutput* OutputNode = OutputNodeCreator.CreateNode();

			FNiagaraVariable PosAttrib(FNiagaraTypeDefinition::GetVec3Def(), "Position");
			FNiagaraVariable VelAttrib(FNiagaraTypeDefinition::GetVec3Def(), "Velocity");
			FNiagaraVariable RotAttrib(FNiagaraTypeDefinition::GetFloatDef(), "Rotation");
			FNiagaraVariable AgeAttrib(FNiagaraTypeDefinition::GetFloatDef(), "NormalizedAge");
			FNiagaraVariable ColAttrib(FNiagaraTypeDefinition::GetColorDef(), "Color");
			FNiagaraVariable SizeAttrib(FNiagaraTypeDefinition::GetVec2Def(), "Size");

			OutputNode->Outputs.Add(PosAttrib);
			OutputNode->Outputs.Add(VelAttrib);
			OutputNode->Outputs.Add(RotAttrib);
			OutputNode->Outputs.Add(ColAttrib);
			OutputNode->Outputs.Add(SizeAttrib);
			OutputNode->Outputs.Add(AgeAttrib);

			OutputNodeCreator.Finalize();


			// Set pointer in script to source
			NewScript->SetSource(Source);

			FString ErrorMessages;
			FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::Get().LoadModuleChecked<FNiagaraEditorModule>(TEXT("NiagaraEditor"));
			NiagaraEditorModule.CompileScript(NewScript, ErrorMessages);
		}
	}
}

ENiagaraScriptUsage UNiagaraScriptFactoryNew::GetScriptUsage() const
{
	return ENiagaraScriptUsage::Module;
}

/************************************************************************/
/* UNiagaraScriptModulesFactory											*/
/************************************************************************/
UNiagaraModuleScriptFactory::UNiagaraModuleScriptFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FName UNiagaraModuleScriptFactory::GetNewAssetThumbnailOverride() const
{
	return TEXT("NiagaraEditor.Thumbnails.Modules");
}

FName UNiagaraModuleScriptFactory::GetAssetTypeActionName() const
{
	return FAssetTypeActions_NiagaraScriptModules::NiagaraModuleScriptName;
}

ENiagaraScriptUsage UNiagaraModuleScriptFactory::GetScriptUsage() const
{
	return ENiagaraScriptUsage::Module;
}

/************************************************************************/
/* UNiagaraScriptFunctionsFactory										*/
/************************************************************************/
UNiagaraFunctionScriptFactory::UNiagaraFunctionScriptFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FName UNiagaraFunctionScriptFactory::GetNewAssetThumbnailOverride() const
{
	return TEXT("NiagaraEditor.Thumbnails.Functions");
}

FName UNiagaraFunctionScriptFactory::GetAssetTypeActionName() const
{
	return FAssetTypeActions_NiagaraScriptFunctions::NiagaraFunctionScriptName;
}

ENiagaraScriptUsage UNiagaraFunctionScriptFactory::GetScriptUsage() const
{
	return ENiagaraScriptUsage::Function;
}

/************************************************************************/
/* UNiagaraScriptDynamicInputsFactory									*/
/************************************************************************/
UNiagaraDynamicInputScriptFactory::UNiagaraDynamicInputScriptFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FName UNiagaraDynamicInputScriptFactory::GetNewAssetThumbnailOverride() const
{
	return TEXT("NiagaraEditor.Thumbnails.DynamicInputs");
}

FName UNiagaraDynamicInputScriptFactory::GetAssetTypeActionName() const
{
	return FAssetTypeActions_NiagaraScriptDynamicInputs::NiagaraDynamicInputScriptName;
}

ENiagaraScriptUsage UNiagaraDynamicInputScriptFactory::GetScriptUsage() const
{
	return ENiagaraScriptUsage::DynamicInput;
}

#undef LOCTEXT_NAMESPACE
