// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "USDImporterPrivatePCH.h"
#include "Paths.h"
#include "UObject/ObjectMacros.h"
#include "GCObject.h"
#include "USDImporter.h"

class FUSDImporterModule : public IUSDImporterModule, public FGCObject
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override
	{
		FString PluginPath = FPaths::ConvertRelativePathToFull(FPaths::EnginePluginsDir() + FString(TEXT("Editor/USDImporter")));

		UnrealUSDWrapper::Initialize(TCHAR_TO_ANSI(*PluginPath));

		USDImporter = NewObject<UUSDImporter>();
	}

	virtual void ShutdownModule() override
	{
		USDImporter = nullptr;
	}

	class UUSDImporter* GetImporter() override
	{
		return USDImporter;
	}

	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(USDImporter);
	}
private:
	UUSDImporter* USDImporter;
};


IMPLEMENT_MODULE(FUSDImporterModule, USDImporter)

