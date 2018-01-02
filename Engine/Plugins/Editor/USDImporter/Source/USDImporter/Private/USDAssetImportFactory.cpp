// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "USDAssetImportFactory.h"
#include "USDImporter.h"
#include "IUSDImporterModule.h"
#include "ActorFactories/ActorFactoryStaticMesh.h"
#include "ScopedTimers.h"
#include "USDImportOptions.h"
#include "Engine/StaticMesh.h"
#include "Paths.h"
#include "JsonObjectConverter.h"
#include "USDAssetImportData.h"

void FUSDAssetImportContext::Init(UObject* InParent, const FString& InName, class IUsdStage* InStage)
{
	FUsdImportContext::Init(InParent, InName, InStage);
}


UUSDAssetImportFactory::UUSDAssetImportFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = false;
	bEditAfterNew = true;
	SupportedClass = UStaticMesh::StaticClass();

	ImportOptions = ObjectInitializer.CreateDefaultSubobject<UUSDImportOptions>(this, TEXT("USDImportOptions"));

	bEditorImport = true;
	bText = false;

	Formats.Add(TEXT("usd;Universal Scene Descriptor files"));
	Formats.Add(TEXT("usda;Universal Scene Descriptor files"));
	Formats.Add(TEXT("usdc;Universal Scene Descriptor files"));
}

UObject* UUSDAssetImportFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UObject* ImportedObject = nullptr;

	UUSDImporter* USDImporter = IUSDImporterModule::Get().GetImporter();

	if (IsAutomatedImport() || USDImporter->ShowImportOptions(*ImportOptions))
	{
		IUsdStage* Stage = USDImporter->ReadUSDFile(ImportContext, Filename);
		if (Stage)
		{
			ImportContext.Init(InParent, InName.ToString(), Stage);
			ImportContext.ImportOptions = ImportOptions;
			ImportContext.bApplyWorldTransformToGeometry = ImportOptions->bApplyWorldTransformToGeometry;

			TArray<FUsdAssetPrimToImport> PrimsToImport;

			ImportContext.PrimResolver->FindMeshAssetsToImport(ImportContext, ImportContext.RootPrim, PrimsToImport);

			TArray<UObject*> ImportedObjects = USDImporter->ImportMeshes(ImportContext, PrimsToImport);

			// Just return the first one imported
			ImportedObject = ImportedObjects.Num() > 0 ? ImportedObjects[0] : nullptr;
		}

		ImportContext.DisplayErrorMessages(IsAutomatedImport());
	}
	else
	{
		bOutOperationCanceled = true;
	}

	return ImportedObject;
}

bool UUSDAssetImportFactory::FactoryCanImport(const FString& Filename)
{
	const FString Extension = FPaths::GetExtension(Filename);

	if (Extension == TEXT("usd") || Extension == TEXT("usda") || Extension == TEXT("usdc"))
	{
		return true;
	}

	return false;
}

void UUSDAssetImportFactory::CleanUp()
{
	ImportContext = FUSDAssetImportContext();
	UnrealUSDWrapper::CleanUp();
}

bool UUSDAssetImportFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	UStaticMesh* Mesh = Cast<UStaticMesh>(Obj);
	if (Mesh != nullptr)
	{
		UUSDAssetImportData* ImportData = Cast<UUSDAssetImportData>(Mesh->AssetImportData);
		if (ImportData)
		{
			OutFilenames.Add(ImportData->GetFirstFilename());
			return true;
		}
	}
	return false;
}

void UUSDAssetImportFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UStaticMesh* Mesh = Cast<UStaticMesh>(Obj);
	if (Mesh != nullptr && ensure(NewReimportPaths.Num() == 1))
	{
		UUSDAssetImportData* ImportData = Cast<UUSDAssetImportData>(Mesh->AssetImportData);
		if (ImportData)
		{
			ImportData->UpdateFilenameOnly(NewReimportPaths[0]);
		}
	}
}

EReimportResult::Type UUSDAssetImportFactory::Reimport(UObject* Obj)
{
	UStaticMesh* Mesh = Cast<UStaticMesh>(Obj);
	if (Mesh != nullptr)
	{
		UUSDAssetImportData* ImportData = Cast<UUSDAssetImportData>(Mesh->AssetImportData);
		if (ImportData)
		{
			bool bOperationCancelled = false;
			UObject* Result = FactoryCreateFile(UStaticMesh::StaticClass(), (UObject*)Mesh->GetOutermost(), Mesh->GetFName(), RF_Transactional | RF_Standalone | RF_Public, ImportData->GetFirstFilename(), nullptr, GWarn, bOperationCancelled);
			if (bOperationCancelled)
			{
				return EReimportResult::Cancelled;
			}
			else
			{
				return Result ? EReimportResult::Succeeded : EReimportResult::Failed;
			}
		}
	}

	return EReimportResult::Failed;
}

void UUSDAssetImportFactory::ParseFromJson(TSharedRef<class FJsonObject> ImportSettingsJson)
{
	FJsonObjectConverter::JsonObjectToUStruct(ImportSettingsJson, ImportOptions->GetClass(), ImportOptions, 0, CPF_InstancedReference);
}
