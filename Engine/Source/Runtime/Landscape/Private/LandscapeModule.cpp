// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Landscape.h"
#include "LandscapeModule.h"

class FLandscapeModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	void StartupModule() override;
	void ShutdownModule() override;
};

/**
 * Add landscape-specific per-world data.
 *
 * @param World A pointer to world that this data should be created for.
 */
void AddPerWorldLandscapeData(UWorld* World)
{
	if (!World->PerModuleDataObjects.FindItemByClass<ULandscapeInfoMap>())
	{
		World->PerModuleDataObjects.Add(NewObject<ULandscapeInfoMap>(GetTransientPackage()));
	}
}

/**
 * Gets landscape-specific material's static parameters values.
 *
 * @param OutStaticParameterSet A set that should be updated with found parameters values.
 * @param Material Material instance to look for parameters.
 */
void LandscapeMaterialsParameterValuesGetter(FStaticParameterSet &OutStaticParameterSet, UMaterialInstance* Material);

/**
 * Updates landscape-specific material parameters.
 *
 * @param OutStaticParameterSet A set of parameters.
 * @param Material A material to update.
 */
bool LandscapeMaterialsParameterSetUpdater(FStaticParameterSet &OutStaticParameterSet, UMaterial* Material);

/**
 * Function that will fire every time a world is created.
 *
 * @param World A world that was created.
 * @param IVS Initialization values.
 */
void WorldCreationEventFunction(UWorld* World)
{
	AddPerWorldLandscapeData(World);
}

/**
 * Function that will fire every time a world is destroyed.
 *
 * @param World A world that's being destroyed.
 */
void WorldDestroyEventFunction(UWorld* World);

/**
 * A function that fires everytime a world is duplicated.
 *
 * @param World A world that was duplicated.
 * @param bDuplicateForPIE If this duplication was done for PIE.
 */
void WorldDuplicateEventFunction(UWorld* World, bool bDuplicateForPIE)
{
	int32 Index;
	ULandscapeInfoMap *InfoMap;
	if (World->PerModuleDataObjects.FindItemByClass(&InfoMap, &Index))
	{
		World->PerModuleDataObjects[Index] = Cast<ULandscapeInfoMap>(
			StaticDuplicateObject(InfoMap, InfoMap->GetOuter(), nullptr)
			);
	}
	else
	{
		AddPerWorldLandscapeData(World);
	}
}

void FLandscapeModule::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
	UMaterialInstance::CustomStaticParametersGetters.AddStatic(
		&LandscapeMaterialsParameterValuesGetter
	);

	UMaterialInstance::CustomParameterSetUpdaters.Add(
		UMaterialInstance::FCustomParameterSetUpdaterDelegate::CreateStatic(
			&LandscapeMaterialsParameterSetUpdater
		)
	);

#if WITH_EDITORONLY_DATA
	FWorldDelegates::OnPostWorldCreation.AddStatic(
		&WorldCreationEventFunction
	);
	FWorldDelegates::OnPreWorldFinishDestroy.AddStatic(
		&WorldDestroyEventFunction
	);
#endif // WITH_EDITORONLY_DATA

	FWorldDelegates::OnPostDuplicate.AddStatic(
		&WorldDuplicateEventFunction
	);
}

void FLandscapeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

IMPLEMENT_MODULE(FLandscapeModule, Landscape);