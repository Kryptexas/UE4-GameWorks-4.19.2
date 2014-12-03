// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "AIModulePrivate.h"
#include "AISystem.h"
#if WITH_EDITOR && ENABLE_VISUAL_LOG
#	include "VisualLoggerExtension.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogAIModule, Log, All);

class FAIModule : public IAIModule
{
	// Begin IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	virtual UAISystemBase* CreateAISystemInstance(UWorld* World) override;
	// End IModuleInterface

protected:
#if WITH_EDITOR && ENABLE_VISUAL_LOG
	FVisualLoggerExtension	VisualLoggerExtension;
#endif
};

IMPLEMENT_MODULE(FAIModule, AIModule)

void FAIModule::StartupModule()
{ 
#if WITH_EDITOR 
	FModuleManager::LoadModulePtr< IModuleInterface >("AITestSuite");
#endif // WITH_EDITOR 

#if WITH_EDITOR && ENABLE_VISUAL_LOG
	FVisualLogger::Get().RegisterExtension(*EVisLogTags::TAG_EQS, &VisualLoggerExtension);
#endif
}

void FAIModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
#if WITH_EDITOR && ENABLE_VISUAL_LOG
	FVisualLogger::Get().UnregisterExtension(*EVisLogTags::TAG_EQS, &VisualLoggerExtension);
#endif
}

UAISystemBase* FAIModule::CreateAISystemInstance(UWorld* World)
{
	UE_LOG(LogAIModule, Log, TEXT("Creating AISystem for world %s"), *GetNameSafe(World));
	
	TSubclassOf<UAISystemBase> AISystemClass = LoadClass<UAISystemBase>(NULL, *UAISystem::GetAISystemClassName().ToString(), NULL, LOAD_None, NULL);
	return ConstructObject<UAISystemBase>(AISystemClass, World);
}
