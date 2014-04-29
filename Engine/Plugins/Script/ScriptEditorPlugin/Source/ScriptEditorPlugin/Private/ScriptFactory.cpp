// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved. 
#include "ScriptEditorPluginPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////

UScriptFactory::UScriptFactory(const FPostConstructInitializeProperties& PCIP)
	: Super( PCIP )
{
	SupportedClass = UScriptAsset::StaticClass();

	Formats.Add(TEXT("txt;Script"));
	Formats.Add(TEXT("lua;Script"));

	bCreateNew = false;
	bEditorImport = true;
	bText = true;
}

void UScriptFactory::PostInitProperties()
{
	Super::PostInitProperties();
}

bool UScriptFactory::DoesSupportClass(UClass* Class)
{
	return Class == UScriptAsset::StaticClass();
}

UObject* UScriptFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
	UScriptAsset* ScriptAsset = NewNamedObject<UScriptAsset>(InParent, InName, Flags);
	
	ScriptAsset->SourceFilePath = FReimportManager::SanitizeImportFilename(CurrentFilename, ScriptAsset);
	ScriptAsset->SourceFileTimestamp = IFileManager::Get().GetTimeStamp(*CurrentFilename).ToString();
	ScriptAsset->SourceCode = Buffer;

	FEditorDelegates::OnAssetPostImport.Broadcast(this, ScriptAsset);
	ScriptAsset->PostEditChange();

	return ScriptAsset;
}


/** UReimportScriptFactory */

UReimportScriptFactory::UReimportScriptFactory(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}


bool UReimportScriptFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	UScriptAsset* ScriptAsset = Cast<UScriptAsset>(Obj);
	if (ScriptAsset)
	{
		OutFilenames.Add(FReimportManager::ResolveImportFilename(ScriptAsset->SourceFilePath, ScriptAsset));
		return true;
	}
	return false;
}

void UReimportScriptFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UScriptAsset* ScriptAsset = Cast<UScriptAsset>(Obj);
	if (ScriptAsset && ensure(NewReimportPaths.Num() == 1))
	{
		ScriptAsset->SourceFilePath = FReimportManager::SanitizeImportFilename(NewReimportPaths[0], Obj);
	}
}

/**
* Reimports specified texture from its source material, if the meta-data exists
*/
EReimportResult::Type UReimportScriptFactory::Reimport(UObject* Obj)
{
	UScriptAsset* ScriptAsset = Cast<UScriptAsset>(Obj);
	if (!ScriptAsset)
	{
		return EReimportResult::Failed;
	}

	TGuardValue<UScriptAsset*> OriginalScriptGuardValue(OriginalScript, ScriptAsset);

	const FString ResolvedSourceFilePath = FReimportManager::ResolveImportFilename(ScriptAsset->SourceFilePath, ScriptAsset);
	if (!ResolvedSourceFilePath.Len())
	{
		return EReimportResult::Failed;
	}

	UE_LOG(LogScriptEditorPlugin, Log, TEXT("Performing atomic reimport of [%s]"), *ResolvedSourceFilePath);

	// Ensure that the file provided by the path exists
	if (IFileManager::Get().FileSize(*ResolvedSourceFilePath) == INDEX_NONE)
	{
		UE_LOG(LogScriptEditorPlugin, Warning, TEXT("Cannot reimport: source file cannot be found."));
		return EReimportResult::Failed;
	}

	if (UFactory::StaticImportObject(ScriptAsset->GetClass(), ScriptAsset->GetOuter(), *ScriptAsset->GetName(), RF_Public | RF_Standalone, *ResolvedSourceFilePath, NULL, this))
	{
		UE_LOG(LogScriptEditorPlugin, Log, TEXT("Imported successfully"));
		// Try to find the outer package so we can dirty it up
		if (ScriptAsset->GetOuter())
		{
			ScriptAsset->GetOuter()->MarkPackageDirty();
		}
		else
		{
			ScriptAsset->MarkPackageDirty();
		}
	}
	else
	{
		UE_LOG(LogScriptEditorPlugin, Warning, TEXT("-- import failed"));
	}

	return EReimportResult::Succeeded;
}
