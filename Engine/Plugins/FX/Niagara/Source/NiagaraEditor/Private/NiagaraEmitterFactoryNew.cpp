// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEmitterFactoryNew.h"
#include "NiagaraEmitter.h"
#include "NiagaraEditorModule.h"
#include "NiagaraScriptFactoryNew.h"
#include "NiagaraGraph.h"
#include "NiagaraScriptSource.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraEditorSettings.h"
#include "AssetData.h"
#include "NiagaraStackGraphUtilities.h"
#include "NiagaraConstants.h"
#include "NiagaraNodeAssignment.h"

#include "ConfigCacheIni.h"

#define LOCTEXT_NAMESPACE "NiagaraEmitterFactory"

UNiagaraEmitterFactoryNew::UNiagaraEmitterFactoryNew(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SupportedClass = UNiagaraEmitter::StaticClass();
	bCreateNew = false;
	bEditAfterNew = true;
	bCreateNew = true;
}

UObject* UNiagaraEmitterFactoryNew::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	check(Class->IsChildOf(UNiagaraEmitter::StaticClass()));

	const UNiagaraEditorSettings* Settings = GetDefault<UNiagaraEditorSettings>();
	check(Settings);

	UNiagaraEmitter* NewEmitter;

	if (UNiagaraEmitter* Default = Cast<UNiagaraEmitter>(Settings->DefaultEmitter.TryLoad()))
	{
		NewEmitter = Cast<UNiagaraEmitter>(StaticDuplicateObject(Default, InParent, Name, Flags, Class));
	}
	else
	{
		UE_LOG(LogNiagaraEditor, Log, TEXT("Default Emitter \"%s\" could not be loaded. Creating graph procedurally."), *Settings->DefaultEmitter.ToString());

		NewEmitter = NewObject<UNiagaraEmitter>(InParent, Class, Name, Flags | RF_Transactional);

		UNiagaraScriptSource* Source = NewObject<UNiagaraScriptSource>(NewEmitter, NAME_None, RF_Transactional);
		if (Source)
		{
			UNiagaraGraph* CreatedGraph = NewObject<UNiagaraGraph>(Source, NAME_None, RF_Transactional);
			Source->NodeGraph = CreatedGraph;

			// Set pointer in script to source
			NewEmitter->GraphSource = Source;
			NewEmitter->SpawnScriptProps.Script->SetSource(Source);
			NewEmitter->UpdateScriptProps.Script->SetSource(Source);
			NewEmitter->EmitterSpawnScriptProps.Script->SetSource(Source);
			NewEmitter->EmitterUpdateScriptProps.Script->SetSource(Source);
			NewEmitter->SimTarget = ENiagaraSimTarget::CPUSim;

			bool bCreateDefaultNodes = true;
			if (bCreateDefaultNodes)
			{
				if (Source)
				{
					UNiagaraNodeOutput* EmitterSpawnOutputNode = FNiagaraStackGraphUtilities::ResetGraphForOutput(*Source->NodeGraph, ENiagaraScriptUsage::EmitterSpawnScript, NewEmitter->EmitterSpawnScriptProps.Script->GetUsageId());
					UNiagaraNodeOutput* EmitterUpdateOutputNode = FNiagaraStackGraphUtilities::ResetGraphForOutput(*Source->NodeGraph, ENiagaraScriptUsage::EmitterUpdateScript, NewEmitter->EmitterUpdateScriptProps.Script->GetUsageId());
					UNiagaraNodeOutput* ParticleSpawnOutputNode = FNiagaraStackGraphUtilities::ResetGraphForOutput(*Source->NodeGraph, ENiagaraScriptUsage::ParticleSpawnScript, NewEmitter->SpawnScriptProps.Script->GetUsageId());
					UNiagaraNodeOutput* ParticleUpdateOutputNode = FNiagaraStackGraphUtilities::ResetGraphForOutput(*Source->NodeGraph, ENiagaraScriptUsage::ParticleUpdateScript, NewEmitter->UpdateScriptProps.Script->GetUsageId());

					{
						FStringAssetReference EmitterUpdateScriptRef(TEXT("/Niagara/Modules/Emitter/EmitterLifeCycle.EmitterLifeCycle"));
						UNiagaraScript* EmitterUpdateScript = Cast<UNiagaraScript>(EmitterUpdateScriptRef.TryLoad());
						FAssetData EmitterUpdateModuleScriptAsset(EmitterUpdateScript);
						if (EmitterUpdateOutputNode && EmitterUpdateModuleScriptAsset.IsValid())
						{
							FNiagaraStackGraphUtilities::AddScriptModuleToStack(EmitterUpdateModuleScriptAsset, *EmitterUpdateOutputNode);
						}
						else
						{
							UE_LOG(LogNiagaraEditor, Error, TEXT("Missing %s"), *EmitterUpdateScriptRef.ToString());
						}
					}

					{
						FStringAssetReference EmitterUpdateScriptRef(TEXT("/Niagara/Modules/Emitter/SpawnRate.SpawnRate"));
						UNiagaraScript* EmitterUpdateScript = Cast<UNiagaraScript>(EmitterUpdateScriptRef.TryLoad());
						FAssetData EmitterUpdateModuleScriptAsset(EmitterUpdateScript);
						if (EmitterUpdateOutputNode && EmitterUpdateModuleScriptAsset.IsValid())
						{
							FNiagaraStackGraphUtilities::AddScriptModuleToStack(EmitterUpdateModuleScriptAsset, *EmitterUpdateOutputNode);
						}
						else
						{
							UE_LOG(LogNiagaraEditor, Error, TEXT("Missing %s"), *EmitterUpdateScriptRef.ToString());
						}
					}

					{
						FStringAssetReference SpawnScriptRef(TEXT("/Niagara/Modules/Spawn/Location/SystemLocation.SystemLocation"));
						UNiagaraScript* SpawnScript = Cast<UNiagaraScript>(SpawnScriptRef.TryLoad());
						FAssetData SpawnModuleScriptAsset(SpawnScript);
						if (ParticleSpawnOutputNode && SpawnModuleScriptAsset.IsValid())
						{
							FNiagaraStackGraphUtilities::AddScriptModuleToStack(SpawnModuleScriptAsset, *ParticleSpawnOutputNode);
						}
						else
						{
							UE_LOG(LogNiagaraEditor, Error, TEXT("Missing %s"), *SpawnScriptRef.ToString());
						}
					}

					{
						FStringAssetReference SpawnScriptRef(TEXT("/Niagara/Modules/Spawn/Velocity/AddVelocity.AddVelocity"));
						UNiagaraScript* SpawnScript = Cast<UNiagaraScript>(SpawnScriptRef.TryLoad());
						FAssetData SpawnModuleScriptAsset(SpawnScript);
						if (ParticleSpawnOutputNode && SpawnModuleScriptAsset.IsValid())
						{
							UNiagaraNodeFunctionCall* CallNode = FNiagaraStackGraphUtilities::AddScriptModuleToStack(SpawnModuleScriptAsset, *ParticleSpawnOutputNode);
							if (CallNode)
							{
								FNiagaraVariable VelocityVar(FNiagaraTypeDefinition::GetVec3Def(), *(TEXT("Constants.Emitter.") + CallNode->GetFunctionName() + TEXT(".Velocity")));
								VelocityVar.SetValue(FVector(0.0f, 0.0f, 100.0f));
								NewEmitter->SpawnScriptProps.Script->RapidIterationParameters.AddParameter(VelocityVar);
							}
						}
						else
						{
							UE_LOG(LogNiagaraEditor, Error, TEXT("Missing %s"), *SpawnScriptRef.ToString());
						}

					}

					{
						if (ParticleSpawnOutputNode)
						{
							{
								FNiagaraVariable Var = SYS_PARAM_PARTICLES_SPRITE_SIZE;
								FString DefaultValue = FNiagaraConstants::GetAttributeDefaultValue(Var);
								FNiagaraStackGraphUtilities::AddParameterModuleToStack(Var, *ParticleSpawnOutputNode, INDEX_NONE,&DefaultValue);
							}

							{
								FNiagaraVariable Var = SYS_PARAM_PARTICLES_SPRITE_ROTATION;
								FString DefaultValue = FNiagaraConstants::GetAttributeDefaultValue(Var);
								FNiagaraStackGraphUtilities::AddParameterModuleToStack(Var, *ParticleSpawnOutputNode, INDEX_NONE, &DefaultValue);
							}

							{
								FNiagaraVariable Var = SYS_PARAM_PARTICLES_LIFETIME;
								FString DefaultValue = FNiagaraConstants::GetAttributeDefaultValue(Var);
								FNiagaraStackGraphUtilities::AddParameterModuleToStack(Var, *ParticleSpawnOutputNode, INDEX_NONE, &DefaultValue);
							}

						}
					}
					
					{
						FStringAssetReference UpdateScriptRef(TEXT("/Niagara/Modules/Update/Lifetime/UpdateAge.UpdateAge"));
						UNiagaraScript* UpdateScript = Cast<UNiagaraScript>(UpdateScriptRef.TryLoad());
						FAssetData UpdateModuleScriptAsset(UpdateScript);
						if (ParticleUpdateOutputNode && UpdateModuleScriptAsset.IsValid())
						{
							FNiagaraStackGraphUtilities::AddScriptModuleToStack(UpdateModuleScriptAsset, *ParticleUpdateOutputNode);
						}
						else
						{
							UE_LOG(LogNiagaraEditor, Error, TEXT("Missing %s"), *UpdateScriptRef.ToString());
						}
					}

					{
						FStringAssetReference UpdateScriptRef(TEXT("/Niagara/Modules/Update/Color/Color.Color"));
						UNiagaraScript* UpdateScript = Cast<UNiagaraScript>(UpdateScriptRef.TryLoad());
						FAssetData UpdateModuleScriptAsset(UpdateScript);
						if (ParticleUpdateOutputNode && UpdateModuleScriptAsset.IsValid())
						{
							FNiagaraStackGraphUtilities::AddScriptModuleToStack(UpdateModuleScriptAsset, *ParticleUpdateOutputNode);
						}
						else
						{
							UE_LOG(LogNiagaraEditor, Error, TEXT("Missing %s"), *UpdateScriptRef.ToString());
						}
					}

					{
						FStringAssetReference UpdateScriptRef(TEXT("/Niagara/Modules/Solvers/SolveForcesAndVelocity.SolveForcesAndVelocity"));
						UNiagaraScript* UpdateScript = Cast<UNiagaraScript>(UpdateScriptRef.TryLoad());
						FAssetData UpdateModuleScriptAsset(UpdateScript);
						if (ParticleUpdateOutputNode && UpdateModuleScriptAsset.IsValid())
						{
							FNiagaraStackGraphUtilities::AddScriptModuleToStack(UpdateModuleScriptAsset, *ParticleUpdateOutputNode);
						}
						else
						{
							UE_LOG(LogNiagaraEditor, Error, TEXT("Missing %s"), *UpdateScriptRef.ToString());
						}
					}

					FNiagaraStackGraphUtilities::RelayoutGraph(*Source->NodeGraph);
					NewEmitter->bInterpolatedSpawning = true;
					NewEmitter->SpawnScriptProps.Script->SetUsage(ENiagaraScriptUsage::ParticleSpawnScriptInterpolated);
				}
			}
		}
	}
	
	return NewEmitter;
}

#undef LOCTEXT_NAMESPACE