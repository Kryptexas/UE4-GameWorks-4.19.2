// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "Modules/ModuleInterface.h"
#include "Engine/World.h"

class FNiagaraWorldManager;
class UNiagaraEmitter;

/**
* Niagara module interface
*/
class INiagaraModule : public IModuleInterface
{
public:
#if WITH_EDITOR
	struct FMergeEmitterResults
	{
		FMergeEmitterResults()
			: bSucceeded(true)
			, bModifiedGraph(false)
		{
		}

		bool bSucceeded;
		TArray<FText> ErrorMessages;
		bool bModifiedGraph;
		UNiagaraEmitter* MergedInstance;

		FString GetErrorMessagesString() const
		{
			TArray<FString> ErrorMessageStrings;
			for (FText ErrorMessage : ErrorMessages)
			{
				ErrorMessageStrings.Add(ErrorMessage.ToString());
			}
			return FString::Join(ErrorMessageStrings, TEXT("\n"));
		}
	};
#endif

public:
#if WITH_EDITOR
	DECLARE_DELEGATE_RetVal_ThreeParams(FMergeEmitterResults, FOnMergeEmitter, UNiagaraEmitter&, UNiagaraEmitter&, UNiagaraEmitter&);
#endif
	DECLARE_DELEGATE_RetVal(void, FOnProcessQueue);

public:
	virtual void StartupModule()override;
	virtual void ShutdownModule()override;

	FDelegateHandle NIAGARA_API SetOnProcessShaderCompilationQueue(FOnProcessQueue InOnProcessQueue);
	void NIAGARA_API ResetOnProcessShaderCompilationQueue(FDelegateHandle DelegateHandle);
	void ProcessShaderCompilationQueue();


	FNiagaraWorldManager* GetWorldManager(UWorld* World);

	void DestroyAllSystemSimulations(class UNiagaraSystem* System);

	// Callback function registered with global world delegates to instantiate world manager when a game world is created
	void OnWorldInit(UWorld* World, const UWorld::InitializationValues IVS);

	// Callback function registered with global world delegates to cleanup world manager contents
	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	// Callback function registered with global world delegates to cleanup world manager when a game world is destroyed
	void OnPreWorldFinishDestroy(UWorld* World);

	void TickWorld(UWorld* World, ELevelTick TickType, float DeltaSeconds);

#if WITH_EDITOR
	FMergeEmitterResults MergeEmitter(UNiagaraEmitter& Source, UNiagaraEmitter& LastMergedSource, UNiagaraEmitter& Instance);

	FDelegateHandle NIAGARA_API RegisterOnMergeEmitter(FOnMergeEmitter OnMergeEmitter);

	void NIAGARA_API UnregisterOnMergeEmitter(FDelegateHandle DelegateHandle);
#endif

	FORCEINLINE static int32 GetDetailLevel() { return EngineDetailLevel; }
private:
	TMap<class UWorld*, class FNiagaraWorldManager*> WorldManagers;
	FOnProcessQueue OnProcessQueue;

#if WITH_EDITORONLY_DATA
	FOnMergeEmitter OnMergeEmitterDelegate;
#endif

	void OnChangeDetailLevel(class IConsoleVariable* CVar);
	static int32 EngineDetailLevel;
};

