// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RendererSupport.cpp: Central place for various rendering functionality that exists in Engine
=============================================================================*/

#include "EnginePrivate.h"
#include "FXSystem.h"
#include "RenderCore.h"
#include "GlobalShader.h"
#include "Slate.h"

/** Clears and optionally backs up all references to renderer module classes in other modules, particularly engine. */
void ClearReferencesToRendererModuleClasses(
	TMap<UWorld*, bool>& WorldsToUpdate, 
	TMap<FMaterialShaderMap*, TScopedPointer<TArray<uint8> > >& ShaderMapToSerializedShaderData,
	TScopedPointer<TArray<uint8> >& GlobalShaderData,
	TMap<FShaderType*, FString>& ShaderTypeNames,
	TMap<FVertexFactoryType*, FString>& VertexFactoryTypeNames)
{
	// Destroy all renderer scenes
	for (TObjectIterator<UWorld> WorldIt; WorldIt; ++WorldIt)
	{
		UWorld* World = *WorldIt;

		if (World->Scene)
		{
			WorldsToUpdate.Add(World, World->FXSystem != NULL);

			for (int32 LevelIndex = 0; LevelIndex < World->GetNumLevels(); LevelIndex++)
			{
				ULevel* Level = World->GetLevel(LevelIndex);
				Level->ReleaseRenderingResources();
			}

			if (World->FXSystem != NULL)
			{
				FFXSystemInterface::Destroy(World->FXSystem);
				World->FXSystem = NULL;
			}

			World->Scene->Release();
			World->Scene = NULL;
		}
	}

	// Save off shaders by serializing them into memory, and remove all shader map references to FShaders
	GlobalShaderData = TScopedPointer<TArray<uint8> >(BackupGlobalShaderMap(GRHIShaderPlatform));
	UMaterial::BackupMaterialShadersToMemory(GRHIShaderPlatform, ShaderMapToSerializedShaderData);

	// Verify no FShaders still in memory
	for(TLinkedList<FShaderType*>::TIterator It(FShaderType::GetTypeList()); It; It.Next())
	{
		FShaderType* ShaderType = *It;
		check(ShaderType->GetNumShaders() == 0);
		ShaderTypeNames.Add(ShaderType, ShaderType->GetName());
	}

	for(TLinkedList<FVertexFactoryType*>::TIterator It(FVertexFactoryType::GetTypeList()); It; It.Next())
	{
		FVertexFactoryType* VertexFactoryType = *It;
		VertexFactoryTypeNames.Add(VertexFactoryType, VertexFactoryType->GetName());
	}

	// Destroy misc renderer module classes and remove references
	FSceneViewStateReference::DestroyAll();
	FSlateApplication::Get().InvalidateAllViewports();

	// Invalidate cached shader type data
	UninitializeShaderTypes();

	// Delete pending cleanup objects to remove those references, which are potentially renderer module classes
	FPendingCleanupObjects* PendingCleanupObjects = GetPendingCleanupObjects();
	delete PendingCleanupObjects;
	GEngine->EngineLoop->ClearPendingCleanupObjects();

	ResetCachedRendererModule();
}

/** Recompiles the renderer module, retrying until successful. */
void RecompileRendererModule()
{
	const FName RendererModuleName = TEXT("Renderer");
	// Unload first so that RecompileModule will not using a rolling module name
	verify(FModuleManager::Get().UnloadModule(RendererModuleName));

	bool bCompiledSuccessfully = false;
	do 
	{
		bCompiledSuccessfully = FModuleManager::Get().RecompileModule(RendererModuleName, false, *GLog);

		if (!bCompiledSuccessfully)
		{
			// Pop up a blocking dialog if there were compilation errors
			// Compiler output will be in the log
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, *FText::Format(
				NSLOCTEXT("UnrealEd", "Error_RetryCompilation", "C++ compilation of module {0} failed!  Details in the log.  \r\nFix the error then click Ok to retry."),
				FText::FromName(RendererModuleName)).ToString(), TEXT("Error"));
		}
	} 
	while (!bCompiledSuccessfully);

	verify(FModuleManager::Get().LoadModule(RendererModuleName, true).IsValid());
}

/** Restores systems that need references to classes in the renderer module. */
void RestoreReferencesToRendererModuleClasses(
	const TMap<UWorld*, bool>& WorldsToUpdate, 
	const TMap<FMaterialShaderMap*, TScopedPointer<TArray<uint8> > >& ShaderMapToSerializedShaderData,
	const TScopedPointer<TArray<uint8> >& GlobalShaderData,
	const TMap<FShaderType*, FString>& ShaderTypeNames,
	const TMap<FVertexFactoryType*, FString>& VertexFactoryTypeNames)
{
	FlushShaderFileCache();

	// Initialize cached shader type data
	InitializeShaderTypes();

	IRendererModule& RendererModule = GetRendererModule();

	FSceneViewStateReference::AllocateAll();

	// Recreate all renderer scenes
	for (TMap<UWorld*, bool>::TConstIterator It(WorldsToUpdate); It; ++It)
	{
		UWorld* World = It.Key();

		World->Scene = RendererModule.AllocateScene(World, World->RequiresHitProxies());

		if (It.Value())
		{
			World->FXSystem = FFXSystemInterface::Create();
			World->Scene->SetFXSystem( World->FXSystem );
		}

		for (int32 LevelIndex = 0; LevelIndex < World->GetNumLevels(); LevelIndex++)
		{
			ULevel* Level = World->GetLevel(LevelIndex);
			Level->InitializeRenderingResources();
		}
	}

	// Restore FShaders from the serialized memory blobs
	// Shader maps may still not be complete after this due to code changes picked up in the recompile
	RestoreGlobalShaderMap(GRHIShaderPlatform, *GlobalShaderData);
	UMaterial::RestoreMaterialShadersFromMemory(GRHIShaderPlatform, ShaderMapToSerializedShaderData);
	FMaterialShaderMap::FixupShaderTypes(GRHIShaderPlatform, ShaderTypeNames, VertexFactoryTypeNames);

	TArray<FShaderType*> OutdatedShaderTypes;
	TArray<const FVertexFactoryType*> OutdatedFactoryTypes;
	FShaderType::GetOutdatedTypes(OutdatedShaderTypes, OutdatedFactoryTypes);

	// Recompile any missing shaders
	BeginRecompileGlobalShaders(OutdatedShaderTypes);
	UMaterial::UpdateMaterialShaders(OutdatedShaderTypes, OutdatedFactoryTypes);

	// Block on global shader jobs
	FinishRecompileGlobalShaders();
}

/** 
 * Handles recompiling the renderer module, including removing all references, recompiling the dll and restoring references.
 */
void RecompileRenderer(const TArray<FString>& Args)
{
	// So that we can see the slow task dialog
	FSlateApplication::Get().DismissAllMenus();

	GWarn->BeginSlowTask( NSLOCTEXT("Renderer", "BeginRecompileRendererTask", "Recompiling Rendering Module..."), true);

	const double StartTime = FPlatformTime::Seconds();
	double EndShutdownTime;
	double EndRecompileTime;

	{
		// Deregister all components from their renderer scenes
		FGlobalComponentReregisterContext ReregisterContext;
		// Shut down the rendering thread so that the game thread will process all rendering commands during this scope
		SCOPED_SUSPEND_RENDERING_THREAD(true);

		TMap<UWorld*, bool> WorldsToUpdate;
		TMap<FMaterialShaderMap*, TScopedPointer<TArray<uint8> > > ShaderMapToSerializedShaderData;
		TScopedPointer<TArray<uint8> > GlobalShaderData;
		TMap<FShaderType*, FString> ShaderTypeNames;
		TMap<FVertexFactoryType*, FString> VertexFactoryTypeNames;

		ClearReferencesToRendererModuleClasses(WorldsToUpdate, ShaderMapToSerializedShaderData, GlobalShaderData, ShaderTypeNames, VertexFactoryTypeNames);

		EndShutdownTime = FPlatformTime::Seconds();
		UE_LOG(LogShaders, Warning, TEXT("Shutdown complete %.1fs"),(float)(EndShutdownTime - StartTime));

		RecompileRendererModule();

		EndRecompileTime = FPlatformTime::Seconds();
		UE_LOG(LogShaders, Warning, TEXT("Recompile complete %.1fs"),(float)(EndRecompileTime - EndShutdownTime));

		RestoreReferencesToRendererModuleClasses(WorldsToUpdate, ShaderMapToSerializedShaderData, GlobalShaderData, ShaderTypeNames, VertexFactoryTypeNames);
	}

#if WITH_EDITOR
	// Refresh viewports
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
#endif

	const double EndTime = FPlatformTime::Seconds();
	UE_LOG(LogShaders, Warning, 
		TEXT("Recompile of Renderer module complete \n")
		TEXT("                                     Total = %.1fs, Shutdown = %.1fs, Recompile = %.1fs, Reload = %.1fs"),
		(float)(EndTime - StartTime),
		(float)(EndShutdownTime - StartTime),
		(float)(EndRecompileTime - EndShutdownTime),
		(float)(EndTime - EndRecompileTime));
	
	GWarn->EndSlowTask();
}

FAutoConsoleCommand RecompileRendererCommand(
	TEXT("r.RecompileRenderer"),
	TEXT("Recompiles the renderer module on the fly."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&RecompileRenderer)
	);
