// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#define SYS_PARAM_DELTA_TIME FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Delta Time"))
#define SYS_PARAM_EFFECT_AGE FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Effect Age"))
#define SYS_PARAM_EMITTER_AGE FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Emitter Age"))
#define SYS_PARAM_EFFECT_POSITION FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Effect Position"))
#define SYS_PARAM_EFFECT_VELOCITY FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Effect Velocity"))
#define SYS_PARAM_EFFECT_X_AXIS FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Effect X Axis"))
#define SYS_PARAM_EFFECT_Y_AXIS FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Effect Y Axis"))
#define SYS_PARAM_EFFECT_Z_AXIS FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Effect Z Axis"))

#define SYS_PARAM_EFFECT_LOCAL_TO_WORLD FNiagaraVariable(FNiagaraTypeDefinition::GetMatrix4Def(), TEXT("Effect Local To World"))
#define SYS_PARAM_EFFECT_WORLD_TO_LOCAL FNiagaraVariable(FNiagaraTypeDefinition::GetMatrix4Def(), TEXT("Effect World To Local"))
#define SYS_PARAM_EFFECT_LOCAL_TO_WORLD_TRANSPOSED FNiagaraVariable(FNiagaraTypeDefinition::GetMatrix4Def(), TEXT("Effect Local To World Transposed"))
#define SYS_PARAM_EFFECT_WORLD_TO_LOCAL_TRANSPOSED FNiagaraVariable(FNiagaraTypeDefinition::GetMatrix4Def(), TEXT("Effect World To Local Transposed"))

#define SYS_PARAM_EXEC_COUNT FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Execution Count"))

#define SYS_PARAM_SPAWNRATE FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Spawn Rate"))
#define SYS_PARAM_SPAWN_INTERVAL FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Spawn Interval"))
#define SYS_PARAM_INTERP_SPAWN_START_DT FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Interp Spawn Start Dt"))

#define SYS_PARAM_INV_DELTA_TIME FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Inv Delta Time"))

//TODO: Implement emitter's own position offset from it's parent effect? Cascade has this.
//#define SYS_PARAM_EMITTER_POSITION FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Emitter Position"));
//#define SYS_PARAM_EMITTER_VELOCITY FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Emitter Velocity"));