// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraTypes.h"

#define SYS_PARAM_ENGINE_DELTA_TIME					FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Engine.DeltaTime"))
#define SYS_PARAM_ENGINE_INV_DELTA_TIME				FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Engine.InverseDeltaTime"))
#define SYS_PARAM_ENGINE_POSITION					FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Engine.Owner.Position"))
#define SYS_PARAM_ENGINE_VELOCITY					FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Engine.Owner.Velocity"))
#define SYS_PARAM_ENGINE_X_AXIS						FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Engine.Owner.SystemXAxis"))
#define SYS_PARAM_ENGINE_Y_AXIS						FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Engine.Owner.SystemYAxis"))
#define SYS_PARAM_ENGINE_Z_AXIS						FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Engine.Owner.SystemZAxis"))
#define SYS_PARAM_ENGINE_SCALE						FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Engine.Owner.Scale"))

#define SYS_PARAM_ENGINE_LOCAL_TO_WORLD				FNiagaraVariable(FNiagaraTypeDefinition::GetMatrix4Def(), TEXT("Engine.Owner.SystemLocalToWorld"))
#define SYS_PARAM_ENGINE_WORLD_TO_LOCAL				FNiagaraVariable(FNiagaraTypeDefinition::GetMatrix4Def(), TEXT("Engine.Owner.SystemWorldToLocal"))
#define SYS_PARAM_ENGINE_LOCAL_TO_WORLD_TRANSPOSED	FNiagaraVariable(FNiagaraTypeDefinition::GetMatrix4Def(), TEXT("Engine.Owner.SystemLocalToWorldTransposed"))
#define SYS_PARAM_ENGINE_WORLD_TO_LOCAL_TRANSPOSED	FNiagaraVariable(FNiagaraTypeDefinition::GetMatrix4Def(), TEXT("Engine.Owner.SystemWorldToLocalTransposed"))
#define SYS_PARAM_ENGINE_LOCAL_TO_WORLD_NO_SCALE	FNiagaraVariable(FNiagaraTypeDefinition::GetMatrix4Def(), TEXT("Engine.Owner.SystemLocalToWorldNoScale"))
#define SYS_PARAM_ENGINE_WORLD_TO_LOCAL_NO_SCALE	FNiagaraVariable(FNiagaraTypeDefinition::GetMatrix4Def(), TEXT("Engine.Owner.SystemWorldToLocalNoScale"))

#define SYS_PARAM_ENGINE_TIME_SINCE_RENDERED		FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Engine.Owner.TimeSinceRendered"))
#define SYS_PARAM_ENGINE_MIN_DIST_TO_CAMERA			FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Engine.Owner.MinDistanceToCamera"))

#define SYS_PARAM_ENGINE_EXEC_COUNT					FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Engine.ExecutionCount"))
#define SYS_PARAM_ENGINE_EMITTER_NUM_PARTICLES		FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Engine.Emitter.NumParticles"))
#define SYS_PARAM_ENGINE_SYSTEM_NUM_EMITTERS_ALIVE	FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Engine.System.NumEmittersAlive"))
#define SYS_PARAM_ENGINE_SYSTEM_NUM_EMITTERS		FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Engine.System.NumEmitters"))

#define SYS_PARAM_EMITTER_AGE						FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Emitter.Age"))
#define SYS_PARAM_EMITTER_LOCALSPACE				FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Emitter.LocalSpace"))
#define SYS_PARAM_EMITTER_SPAWNRATE					FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Emitter.SpawnRate"))
#define SYS_PARAM_EMITTER_SPAWN_INTERVAL			FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Emitter.SpawnInterval"))
#define SYS_PARAM_EMITTER_INTERP_SPAWN_START_DT		FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Emitter.InterpSpawnStartDt"))

#define SYS_PARAM_PARTICLES_POSITION				FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Particles.Position"))
#define SYS_PARAM_PARTICLES_VELOCITY				FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Particles.Velocity"))
#define SYS_PARAM_PARTICLES_COLOR					FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(), TEXT("Particles.Color"))
#define SYS_PARAM_PARTICLES_SPRITE_ROTATION			FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Particles.SpriteRotation"))
#define SYS_PARAM_PARTICLES_NORMALIZED_AGE			FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Particles.NormalizedAge"))
#define SYS_PARAM_PARTICLES_SPRITE_SIZE				FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(), TEXT("Particles.SpriteSize"))
#define SYS_PARAM_PARTICLES_SPRITE_FACING			FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Particles.SpriteFacing"))
#define SYS_PARAM_PARTICLES_SPRITE_ALIGNMENT		FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Particles.SpriteAlignment"))
#define SYS_PARAM_PARTICLES_SUB_IMAGE_INDEX			FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Particles.SubImageIndex"))
#define SYS_PARAM_PARTICLES_DYNAMIC_MATERIAL_PARAM	FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("Particles.DynamicMaterialParameter"))
#define SYS_PARAM_PARTICLES_SCALE					FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Particles.Scale"))
#define SYS_PARAM_PARTICLES_LIFETIME				FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Particles.Lifetime"))
#define SYS_PARAM_PARTICLES_MESH_ORIENTATION		FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("Particles.MeshOrientation"))
#define SYS_PARAM_INSTANCE_ALIVE					FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("DataInstance.Alive"))
#define SYS_PARAM_PARTICLES_UV_SCALE				FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(), TEXT("Particles.UVScale"))
#define SYS_PARAM_PARTICLES_CAMERA_OFFSET			FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Particles.CameraOffset"))
#define SYS_PARAM_PARTICLES_MATERIAL_RANDOM			FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Particles.MaterialRandom"))

#define SYS_PARAM_PARTICLES_RIBBONID				FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Particles.RibbonID"))
#define SYS_PARAM_PARTICLES_RIBBONWIDTH			FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Particles.RibbonWidth"))
#define SYS_PARAM_PARTICLES_RIBBONTWIST			FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Particles.RibbonTwist"))
#define SYS_PARAM_PARTICLES_RIBBONFACING			FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Particles.RibbonFacing"))

struct NIAGARA_API FNiagaraConstants
{
	static const TArray<FNiagaraVariable>& GetEngineConstants();
	static FNiagaraVariable UpdateEngineConstant(const FNiagaraVariable& InVar);
	static const FNiagaraVariable *FindEngineConstant(const FNiagaraVariable& InVar);
	static FText GetEngineConstantDescription(const FNiagaraVariable& InVar);

	static const TArray<FNiagaraVariable>& GetCommonParticleAttributes();
	static FText GetAttributeDescription(const FNiagaraVariable& InVar);
	static FString GetAttributeDefaultValue(const FNiagaraVariable& InVar);

	static bool IsNiagaraConstant(const FNiagaraVariable& InVar);
	static const FNiagaraVariableMetaData* GetConstantMetaData(const FNiagaraVariable& InVar);
};