// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#include "NiagaraEmitterProperties.h"
#include "NiagaraSpriteRendererProperties.h"

void FNiagaraEmitterScriptProperties::InitDataSetAccess()
{
	EventReceivers.Empty();
	EventGenerators.Empty();

	if (Script)
	{
		// TODO: add event receiver and generator lists to the script properties here
		//
		for (FNiagaraDataSetID &ReadID : Script->ReadDataSets)
		{
			EventReceivers.Add( FNiagaraEventReceiverProperties(ReadID.Name, "", "") );
		}

		for (FNiagaraDataSetProperties &WriteID : Script->WriteDataSets)
		{
			FNiagaraEventGeneratorProperties Props(WriteID, "", "");
			EventGenerators.Add(Props);
		}
	}
}

//////////////////////////////////////////////////////////////////////////

UNiagaraEmitterProperties::UNiagaraEmitterProperties(const FObjectInitializer& Initializer)
: Super(Initializer)
, SpawnRate(50)
, Material(nullptr)
, StartTime(0.0f)
, EndTime(0.0f)
, NumLoops(0)
, CollisionMode(ENiagaraCollisionMode::None)
, RendererProperties(nullptr)
{
}

void UNiagaraEmitterProperties::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad) == false)
	{
		RendererProperties = NewObject<UNiagaraSpriteRendererProperties>(this, "Renderer");

		SpawnScriptProps.Script = NewObject<UNiagaraScript>(this, "SpawnScript", EObjectFlags::RF_Transactional);
		SpawnScriptProps.Script->Usage = ENiagaraScriptUsage::SpawnScript;

		UpdateScriptProps.Script = NewObject<UNiagaraScript>(this, "UpdateScript", EObjectFlags::RF_Transactional);
		UpdateScriptProps.Script->Usage = ENiagaraScriptUsage::UpdateScript;
	}
}


void UNiagaraEmitterProperties::PostLoad()
{
	Super::PostLoad();

	if (GIsEditor)
	{
		SetFlags(RF_Transactional);
	}
}

void UNiagaraEmitterProperties::Init()
{
	//SpawnScriptProps.Init(this);
	//UpdateScriptProps.Init(this);
	return;
}

#if WITH_EDITOR
void UNiagaraEmitterProperties::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	Init();
}
#endif
