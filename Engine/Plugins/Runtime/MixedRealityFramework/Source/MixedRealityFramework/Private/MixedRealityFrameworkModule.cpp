// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "IMixedRealityFrameworkModule.h"
#include "ModuleManager.h" // for IMPLEMENT_MODULE()
#include "Engine/Engine.h"
#include "MixedRealityConfigurationSaveGame.h" // for SaveSlotName/UserIndex
#include "Kismet/GameplayStatics.h" // for DoesSaveGameExist()
#include "MixedRealityCaptureActor.h"
#include "MixedRealityCaptureComponent.h"
#include "UObject/UObjectIterator.h"
#include "MotionDelayBuffer.h" // for SetEnabled()

class FMixedRealityFrameworkModule : public IMixedRealityFrameworkModule
{
public:
	FMixedRealityFrameworkModule();

public:
	//~ IModuleInterface interface

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void OnWorldCreated(UWorld* NewWorld);

private:
	FDelegateHandle WorldEventBinding;
	bool bHasMRConfigFile;
	FString TargetConfigName;
	int32 TargetConfigIndex;
};

FMixedRealityFrameworkModule::FMixedRealityFrameworkModule()
	: bHasMRConfigFile(false)
	, TargetConfigIndex(0)
{}

void FMixedRealityFrameworkModule::StartupModule()
{
	const UMixedRealityConfigurationSaveGame* DefaultSaveData = GetDefault<UMixedRealityConfigurationSaveGame>();
	TargetConfigName  = DefaultSaveData->SaveSlotName;
	TargetConfigIndex = DefaultSaveData->UserIndex;

	bHasMRConfigFile = UGameplayStatics::DoesSaveGameExist(TargetConfigName, TargetConfigIndex);
	if (bHasMRConfigFile)
	{
		WorldEventBinding = GEngine->OnWorldAdded().AddRaw(this, &FMixedRealityFrameworkModule::OnWorldCreated);
	}

	FMotionDelayService::SetEnabled(true);
}

void FMixedRealityFrameworkModule::ShutdownModule()
{
	if (GEngine)
	{
		GEngine->OnWorldAdded().Remove(WorldEventBinding);
	}
}

void FMixedRealityFrameworkModule::OnWorldCreated(UWorld* NewWorld)
{
#if WITH_EDITORONLY_DATA
	const bool bIsGameInst = !IsRunningCommandlet() && (NewWorld->WorldType != EWorldType::Editor) && (NewWorld->WorldType != EWorldType::EditorPreview);
	if (bIsGameInst)
#endif 
	{
		if (bHasMRConfigFile)
		{
			UMixedRealityCaptureComponent* MRCaptureComponent = nullptr;
			for (TObjectIterator<UMixedRealityCaptureComponent> ObjIt; ObjIt; ++ObjIt)
			{
				if (ObjIt->GetWorld() == NewWorld)
				{
					MRCaptureComponent = *ObjIt;
				}
			}

			if (MRCaptureComponent == nullptr)
			{
				AMixedRealityCaptureActor* MRActor = NewWorld->SpawnActor<AMixedRealityCaptureActor>();
				MRCaptureComponent = MRActor->CaptureComponent;
			}

			MRCaptureComponent->LoadConfiguration(TargetConfigName, TargetConfigIndex);
		}
	}
}

IMPLEMENT_MODULE(FMixedRealityFrameworkModule, MixedRealityFramework);
