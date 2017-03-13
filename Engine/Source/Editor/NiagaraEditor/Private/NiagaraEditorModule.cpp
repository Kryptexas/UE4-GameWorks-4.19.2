// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEditorModule.h"
#include "Modules/ModuleManager.h"
#include "IAssetTypeActions.h"
#include "AssetToolsModule.h"
#include "Misc/ConfigCacheIni.h"

#include "AssetTypeActions/AssetTypeActions_NiagaraEffect.h"
#include "AssetTypeActions/AssetTypeActions_NiagaraEmitter.h"
#include "AssetTypeActions/AssetTypeActions_NiagaraScript.h"

#include "EdGraphUtilities.h"
#include "SGraphPin.h"
#include "SGraphPinVector4.h"
#include "SGraphPinNum.h"
#include "SGraphPinInteger.h"
#include "SGraphPinVector.h"
#include "SGraphPinVector2D.h"
#include "SGraphPinObject.h"
#include "SGraphPinColor.h"
#include "SNiagaraGraphPinNumeric.h"
#include "SNiagaraGraphPinAdd.h"
#include "NiagaraNodeConvert.h"
#include "EdGraphSchema_Niagara.h"
#include "NiagaraFloatTypeEditorUtilities.h"
#include "NiagaraIntegerTypeEditorUtilities.h"
#include "NiagaraBoolTypeEditorUtilities.h"
#include "NiagaraFloatTypeEditorUtilities.h"
#include "NiagaraVectorTypeEditorUtilities.h"
#include "NiagaraColorTypeEditorUtilities.h"
#include "NiagaraEditorStyle.h"
#include "NiagaraEditorCommands.h"
#include "PropertyEditorModule.h"

#include "Customizations/NiagaraComponentDetails.h"

IMPLEMENT_MODULE( FNiagaraEditorModule, NiagaraEditor );

#define LOCTEXT_NAMESPACE "NiagaraEditorModule"

const FName FNiagaraEditorModule::NiagaraEditorAppIdentifier( TEXT( "NiagaraEditorApp" ) );
const FLinearColor FNiagaraEditorModule::WorldCentricTabColorScale(0.0f, 0.0f, 0.2f, 0.5f);
const FColor FNiagaraEditorModule::TypeColor(255, 194, 0);

EAssetTypeCategories::Type FNiagaraEditorModule::NiagaraAssetCategory;

//////////////////////////////////////////////////////////////////////////

class FNiagaraScriptGraphPanelPinFactory : public FGraphPanelPinFactory
{
public:
	DECLARE_DELEGATE_RetVal_OneParam(TSharedPtr<class SGraphPin>, FCreateGraphPin, UEdGraphPin*);

	/** Registers a delegate for creating a pin for a specific type. */
	void RegisterTypePin(const UScriptStruct* Type, FCreateGraphPin CreateGraphPin)
	{
		TypeToCreatePinDelegateMap.Add(Type, CreateGraphPin);
	}

	/** Registers a delegate for creating a pin for for a specific miscellaneous sub category. */
	void RegisterMiscSubCategoryPin(FString SubCategory, FCreateGraphPin CreateGraphPin)
	{
		MiscSubCategoryToCreatePinDelegateMap.Add(SubCategory, CreateGraphPin);
	}

	//~ FGraphPanelPinFactory interface
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override
	{
		if (const UEdGraphSchema_Niagara* NSchema = Cast<UEdGraphSchema_Niagara>(InPin->GetSchema()))
		{
			if (InPin->PinType.PinCategory == UEdGraphSchema_Niagara::PinCategoryType)
			{
				const UScriptStruct* Struct = CastChecked<const UScriptStruct>(InPin->PinType.PinSubCategoryObject.Get());
				const FCreateGraphPin* CreateGraphPin = TypeToCreatePinDelegateMap.Find(Struct);
				if (CreateGraphPin != nullptr)
				{
					return (*CreateGraphPin).Execute(InPin);
				}
			}
			else if (InPin->PinType.PinCategory == UEdGraphSchema_Niagara::PinCategoryMisc)
			{
				const FCreateGraphPin* CreateGraphPin = MiscSubCategoryToCreatePinDelegateMap.Find(InPin->PinType.PinSubCategory);
				if (CreateGraphPin != nullptr)
				{
					return (*CreateGraphPin).Execute(InPin);
				}
			}

			return SNew(SGraphPin, InPin);
		}
		return nullptr;
	}

private:
	TMap<const UScriptStruct*, FCreateGraphPin> TypeToCreatePinDelegateMap;
	TMap<FString, FCreateGraphPin> MiscSubCategoryToCreatePinDelegateMap;
};

void FNiagaraEditorModule::StartupModule()
{
	MenuExtensibilityManager = MakeShareable(new FExtensibilityManager);
	ToolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);

	// Only allow asset creation if niagara has been enabled.
	bool bEnableNiagara = false;
	GConfig->GetBool(TEXT("Niagara"), TEXT("EnableNiagara"), bEnableNiagara, GEngineIni);
	if (bEnableNiagara)
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		NiagaraAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Niagara")), LOCTEXT("NiagaraAssetsCategory", "Niagara"));
		RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_NiagaraEffect()));
		RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_NiagaraEmitter()));
		RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_NiagaraScript()));
	}

	// register details customization
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout("NiagaraComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FNiagaraComponentDetails::MakeInstance));

	FNiagaraEditorStyle::Initialize();
	FNiagaraEditorCommands::Register();

	TSharedPtr<FNiagaraScriptGraphPanelPinFactory> GraphPanelPinFactory = MakeShareable(new FNiagaraScriptGraphPanelPinFactory());

	GraphPanelPinFactory->RegisterTypePin(FNiagaraTypeDefinition::GetFloatStruct(), FNiagaraScriptGraphPanelPinFactory::FCreateGraphPin::CreateLambda(
		[](UEdGraphPin* GraphPin) -> TSharedRef<SGraphPin> { return SNew(SGraphPinNum, GraphPin); }));

	GraphPanelPinFactory->RegisterTypePin(FNiagaraTypeDefinition::GetIntStruct(), FNiagaraScriptGraphPanelPinFactory::FCreateGraphPin::CreateLambda(
		[](UEdGraphPin* GraphPin) -> TSharedRef<SGraphPin> { return SNew(SGraphPinInteger, GraphPin); }));

	GraphPanelPinFactory->RegisterTypePin(FNiagaraTypeDefinition::GetVec2Struct(), FNiagaraScriptGraphPanelPinFactory::FCreateGraphPin::CreateLambda(
		[](UEdGraphPin* GraphPin) -> TSharedRef<SGraphPin> { return SNew(SGraphPinVector2D, GraphPin); }));

	GraphPanelPinFactory->RegisterTypePin(FNiagaraTypeDefinition::GetVec3Struct(), FNiagaraScriptGraphPanelPinFactory::FCreateGraphPin::CreateLambda(
		[](UEdGraphPin* GraphPin) -> TSharedRef<SGraphPin> { return SNew(SGraphPinVector, GraphPin); }));

	GraphPanelPinFactory->RegisterTypePin(FNiagaraTypeDefinition::GetVec4Struct(), FNiagaraScriptGraphPanelPinFactory::FCreateGraphPin::CreateLambda(
		[](UEdGraphPin* GraphPin) -> TSharedRef<SGraphPin> { return SNew(SGraphPinVector4, GraphPin); }));

	GraphPanelPinFactory->RegisterTypePin(FNiagaraTypeDefinition::GetColorStruct(), FNiagaraScriptGraphPanelPinFactory::FCreateGraphPin::CreateLambda(
		[](UEdGraphPin* GraphPin) -> TSharedRef<SGraphPin> { return SNew(SGraphPinColor, GraphPin); }));

	GraphPanelPinFactory->RegisterTypePin(FNiagaraTypeDefinition::GetGenericNumericStruct(), FNiagaraScriptGraphPanelPinFactory::FCreateGraphPin::CreateLambda(
		[](UEdGraphPin* GraphPin) -> TSharedRef<SGraphPin> { return SNew(SNiagaraGraphPinNumeric, GraphPin); }));

	// TODO: Don't register this here.
	GraphPanelPinFactory->RegisterMiscSubCategoryPin(UNiagaraNodeWithDynamicPins::AddPinSubCategory, FNiagaraScriptGraphPanelPinFactory::FCreateGraphPin::CreateLambda(
		[](UEdGraphPin* GraphPin) -> TSharedRef<SGraphPin> { return SNew(SNiagaraGraphPinAdd, GraphPin); }));

	RegisterTypeUtilities(FNiagaraTypeDefinition::GetFloatDef(), MakeShareable(new FNiagaraEditorFloatTypeUtilities()));
	RegisterTypeUtilities(FNiagaraTypeDefinition::GetIntDef(), MakeShareable(new FNiagaraIntegerTypeEditorUtilities()));
	RegisterTypeUtilities(FNiagaraTypeDefinition::GetBoolDef(), MakeShareable(new FNiagaraEditorBoolTypeUtilities()));
	RegisterTypeUtilities(FNiagaraTypeDefinition::GetVec2Def(), MakeShareable(new FNiagaraEditorVector2TypeUtilities()));
	RegisterTypeUtilities(FNiagaraTypeDefinition::GetVec3Def(), MakeShareable(new FNiagaraEditorVector3TypeUtilities()));
	RegisterTypeUtilities(FNiagaraTypeDefinition::GetVec4Def(), MakeShareable(new FNiagaraEditorVector4TypeUtilities()));
	RegisterTypeUtilities(FNiagaraTypeDefinition::GetColorDef(), MakeShareable(new FNiagaraEditorColorTypeUtilities()));

	FEdGraphUtilities::RegisterVisualPinFactory(GraphPanelPinFactory);

	FNiagaraOpInfo::Init();
}


void FNiagaraEditorModule::ShutdownModule()
{
	MenuExtensibilityManager.Reset();
	ToolBarExtensibilityManager.Reset();

	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (auto CreatedAssetTypeAction : CreatedAssetTypeActions)
		{
			AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeAction.ToSharedRef());
		}
	}
	CreatedAssetTypeActions.Empty();
	
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout("NiagaraComponent");
	}

	FNiagaraEditorStyle::Shutdown();
}


void FNiagaraEditorModule::RegisterTypeUtilities(FNiagaraTypeDefinition Type, TSharedRef<INiagaraEditorTypeUtilities> EditorUtilities)
{
	TypeToEditorUtilitiesMap.Add(Type, EditorUtilities);
}


TSharedPtr<INiagaraEditorTypeUtilities> FNiagaraEditorModule::GetTypeUtilities(const FNiagaraTypeDefinition& Type)
{
	TSharedRef<INiagaraEditorTypeUtilities>* EditorUtilities = TypeToEditorUtilitiesMap.Find(Type);
	if (EditorUtilities != nullptr)
	{
		return *EditorUtilities;
	}
	return TSharedPtr<INiagaraEditorTypeUtilities>();
}


void FNiagaraEditorModule::RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}

#undef LOCTEXT_NAMESPACE