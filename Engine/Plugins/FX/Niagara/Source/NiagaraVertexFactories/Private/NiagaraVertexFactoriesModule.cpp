// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraVertexFactoriesModule.h"
#include "Modules/ModuleManager.h"
#include "NiagaraVertexFactory.h"

IMPLEMENT_MODULE(INiagaraVertexFactoriesModule, NiagaraVertexFactories);

FRWBuffer FNiagaraVertexFactoryBase::DummyBuffer;