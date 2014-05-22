// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ScriptGeneratorPluginPrivatePCH.h"
#include "ScriptCodeGeneratorBase.h"
#include "GenericScriptCodeGenerator.h"
#include "LuaScriptCodeGenerator.h"

DEFINE_LOG_CATEGORY(LogScriptGenerator);

class FScriptGeneratorPlugin : public IScriptGeneratorPlugin
{
	/** Specialized script code generator */
	TAutoPtr<FScriptCodeGeneratorBase> CodeGenerator;

	/** IModuleInterface implementation */
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;

	/** IScriptGeneratorPlugin interface */
	virtual FString GetGeneratedCodeModuleName() const OVERRIDE { return TEXT("ScriptPlugin"); }
	virtual bool ShouldExportClassesForModule(const FString& ModuleName, EBuildModuleType::Type ModuleType) const;
	virtual bool SupportsTarget(const FString& TargetName) const OVERRIDE { return true; }
	virtual void Initialize(const FString& RootLocalPath, const FString& RootBuildPath, const FString& OutputDirectory) OVERRIDE;
	virtual void ExportClass(UClass* Class, const FString& SourceHeaderFilename, const FString& GeneratedHeaderFilename, bool bHasChanged) OVERRIDE;
	virtual void FinishExport() OVERRIDE;
};

IMPLEMENT_MODULE( FScriptGeneratorPlugin, ScriptGeneratorPlugin )


void FScriptGeneratorPlugin::StartupModule()
{
}

void FScriptGeneratorPlugin::ShutdownModule()
{
	CodeGenerator.Reset();
}

void FScriptGeneratorPlugin::Initialize(const FString& RootLocalPath, const FString& RootBuildPath, const FString& OutputDirectory)
{
#if WITH_LUA
	UE_LOG(LogScriptGenerator, Log, TEXT("Using Lua Script Generator."));
	CodeGenerator = new FLuaScriptCodeGenerator(RootLocalPath, RootBuildPath, OutputDirectory);
#else
	UE_LOG(LogScriptGenerator, Log, TEXT("Using Generic Script Generator."));
	CodeGenerator = new FGenericScriptCodeGenerator(RootLocalPath, RootBuildPath, OutputDirectory);
#endif
	UE_LOG(LogScriptGenerator, Log, TEXT("Output directory: %s"), *OutputDirectory);
}

bool FScriptGeneratorPlugin::ShouldExportClassesForModule(const FString& ModuleName, EBuildModuleType::Type ModuleType) const
{ 
	bool bCanExport = (ModuleType == EBuildModuleType::Runtime || ModuleType == EBuildModuleType::Game);
	if (bCanExport)
	{
		// Only export functions from selected modules
		static struct FSupportedModules
		{
			TArray<FString> SupportedScriptModules;
			FSupportedModules()
			{
				GConfig->GetArray(TEXT("Plugins"), TEXT("ScriptSupportedModules"), SupportedScriptModules, GEngineIni);
			}
		} SupportedModules;
		bCanExport = SupportedModules.SupportedScriptModules.Num() == 0 || SupportedModules.SupportedScriptModules.Contains(ModuleName);
	}
	return bCanExport;
}

void FScriptGeneratorPlugin::ExportClass(UClass* Class, const FString& SourceHeaderFilename, const FString& GeneratedHeaderFilename, bool bHasChanged)
{
	CodeGenerator->ExportClass(Class, SourceHeaderFilename, GeneratedHeaderFilename, bHasChanged);
}

void FScriptGeneratorPlugin::FinishExport()
{
	CodeGenerator->FinishExport();
}
