// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IGLTFImporter.h"

// #include "PropertyEditorModule.h"
// #include "GLTFImportData.h"

/**
 * glTF Importer module implementation (private)
 */
class FGLTFImporterModule : public IGLTFImporter
{
public:
	virtual void StartupModule() override
	{
		// FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		// PropertyModule.RegisterCustomClassLayout(USpeedTreeImportData::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FSpeedTreeImportDataDetails::MakeInstance));
	}

	virtual void ShutdownModule() override
	{
	}
};

IMPLEMENT_MODULE(FGLTFImporterModule, GLTFImporter);
