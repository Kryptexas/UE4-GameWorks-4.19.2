// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "NiagaraPrivate.h"
#include "NiagaraSimulation.h"
#include "NiagaraEffectRenderer.h"
#include "VectorVM.h"

FNiagaraSimulation::FNiagaraSimulation(TWeakObjectPtr<UNiagaraEmitterProperties> InProps, UNiagaraEffect* InEffect)
: Age(0.0f)
, Loops(0)
, bIsEnabled(true)
, SpawnRemainder(0.0f)
, CachedBounds(ForceInit)
, EffectRenderer(nullptr)
, ParentEffect(InEffect)
{
	Props = InProps;

	Init();
}

FNiagaraSimulation::FNiagaraSimulation(TWeakObjectPtr<UNiagaraEmitterProperties> InProps, UNiagaraEffect* InEffect, ERHIFeatureLevel::Type InFeatureLevel)
	: Age(0.0f)
	, Loops(0)
	, bIsEnabled(true)
	, SpawnRemainder(0.0f)
	, CachedBounds(ForceInit)
	, EffectRenderer(nullptr)
	, ParentEffect(InEffect)
{
	Props = InProps;

	Init();
	SetRenderModuleType(Props->RenderModuleType, InFeatureLevel);
}

void FNiagaraSimulation::Init()
{
	Data.Reset();
	SpawnRemainder = 0.0f;
	Constants.Empty();
	Age = 0.0f;
	Loops = 0.0f;

	UNiagaraEmitterProperties* PinnedProps = Props.Get();
	if (PinnedProps && PinnedProps->UpdateScriptProps.Script && PinnedProps->SpawnScriptProps.Script)
	{
		if (PinnedProps->UpdateScriptProps.Script->Attributes.Num() == 0 || PinnedProps->SpawnScriptProps.Script->Attributes.Num() == 0)
		{
			Data.Reset();
			bIsEnabled = false;
			UE_LOG(LogNiagara, Error, TEXT("This emitter cannot be enabled because it's spawn or update script doesn't have any attriubtes.."));
		}
		else
		{
			//Warn the user if there are any attributes used in the update script that are not initialized in the spawn script.
			//TODO: We need some window in the effect editor and possibly the graph editor for warnings and errors.
			for (FNiagaraVariableInfo& Attr : PinnedProps->UpdateScriptProps.Script->Attributes)
			{
				int32 FoundIdx;
				if (!PinnedProps->SpawnScriptProps.Script->Attributes.Find(Attr, FoundIdx))
				{
					UE_LOG(LogNiagara, Warning, TEXT("Attribute %s is used in the Update script for %s but it is not initialised in the Spawn script!"), *Attr.Name.ToString(), *Props->EmitterName);
				}
			}
			Data.AddAttributes(PinnedProps->UpdateScriptProps.Script->Attributes);
			Data.AddAttributes(PinnedProps->SpawnScriptProps.Script->Attributes);

			Constants.Empty();
			Constants.Merge(PinnedProps->UpdateScriptProps.ExternalConstants);
			Constants.Merge(PinnedProps->SpawnScriptProps.ExternalConstants);
		}
	}
	else
	{
		Data.Reset();//Clear the attributes to mark the sim as disabled independatly of the user set Enabled flag.
		bIsEnabled = false;//Also force the user flag to give an indication to the user.
		//TODO - Arbitrary named scripts. Would need some base functionality for Spawn/Udpate to be called that can be overriden in BPs for emitters with custom scripts.
		UE_LOG(LogNiagara, Error, TEXT("This emitter cannot be enabled because it's doesn't have both an update and spawn script."));
	}
}

void FNiagaraSimulation::SetProperties(TWeakObjectPtr<UNiagaraEmitterProperties> InProps)
{ 
	Props = InProps; 
	Init();
}

void FNiagaraSimulation::SetEnabled(bool bEnabled)
{
	bIsEnabled = bEnabled;
	Init();
}


float FNiagaraSimulation::GetTotalCPUTime()
{
	float Total = CPUTimeMS;
	if (EffectRenderer)
	{
		Total += EffectRenderer->GetCPUTimeMS();
	}

	return Total;
}

int FNiagaraSimulation::GetTotalBytesUsed()
{
	return Data.GetBytesUsed();
}


void FNiagaraSimulation::Tick(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraTick);

	UNiagaraEmitterProperties* PinnedProps = Props.Get();
	if (!PinnedProps || !bIsEnabled || TickState==NTS_Suspended || TickState==NTS_Dead)
		return;

	SimpleTimer TickTime;


	check(Data.GetNumAttributes() > 0);
	check(PinnedProps->SpawnScriptProps.Script);
	check(PinnedProps->UpdateScriptProps.Script);

	// Cache the ComponentToWorld transform.
//	CachedComponentToWorld = Component.GetComponentToWorld();

	Data.SwapBuffers();
	Data.SetNumParticles(Data.GetPrevNumParticles());

	int32 OrigNumParticles = Data.GetNumParticles();
	int32 NumToSpawn = 0;

	// Figure out how many we will spawn.
	NumToSpawn = CalcNumToSpawn(DeltaSeconds);
	int32 MaxNewParticles = OrigNumParticles + NumToSpawn;
	Data.Allocate(MaxNewParticles);

	Age += DeltaSeconds;
	Constants.SetOrAdd(TEXT("Emitter Age"), FVector4(Age, Age, Age, Age));
	Constants.SetOrAdd(TEXT("Delta Time"), FVector4(DeltaSeconds, DeltaSeconds, DeltaSeconds, DeltaSeconds));

	// Simulate particles forward by DeltaSeconds.
	if (TickState==NTS_Running || TickState==NTS_Dieing)
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraSimulate);
		RunVMScript(PinnedProps->UpdateScriptProps.Script, EUnusedAttributeBehaviour::Copy);
	}
	
	//Init new particles with the spawn script.
	if (TickState==NTS_Running)
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraSpawn);
		Data.SetNumParticles(MaxNewParticles);
		//For now, zero any unused attributes here. But as this is really uninitialized data we should maybe make this a more serious error.
		RunVMScript(PinnedProps->SpawnScriptProps.Script, EUnusedAttributeBehaviour::Zero, OrigNumParticles, NumToSpawn);
	}

	// Iterate over looking for dead particles and move from the end of the list to the dead location, compacting in the process
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraKill);
		int32 CurNumParticles = OrigNumParticles = Data.GetNumParticles();
		int32 ParticleIndex = 0;
		const FVector4* ParticleRelativeTimes = Data.GetAttributeData(FNiagaraVariableInfo(FName(TEXT("Age")), ENiagaraDataType::Vector));
		if (ParticleRelativeTimes)
		{
			while (ParticleIndex < OrigNumParticles)
			{
				if (ParticleRelativeTimes[ParticleIndex].X > 1.0f)
				{
					// Particle is dead, move one from the end here. 
					MoveParticleToIndex(--CurNumParticles, ParticleIndex);
				}
				ParticleIndex++;
			}
		}
		Data.SetNumParticles(CurNumParticles);

		// check if the emitter has officially died
		if (GetTickState() == NTS_Dieing && CurNumParticles == 0)
		{
			SetTickState(NTS_Dead);
		}
	}



	CPUTimeMS = TickTime.GetElapsedMilliseconds();

	INC_DWORD_STAT_BY(STAT_NiagaraNumParticles, Data.GetNumParticles());

}

void FNiagaraSimulation::RunVMScript(UNiagaraScript* Script, EUnusedAttributeBehaviour UnusedAttribBehaviour)
{
	RunVMScript(Script, UnusedAttribBehaviour, 0, Data.GetNumParticles());
}

void FNiagaraSimulation::RunVMScript(UNiagaraScript* Script, EUnusedAttributeBehaviour UnusedAttribBehaviour, uint32 StartParticle)
{
	RunVMScript(Script, UnusedAttribBehaviour, StartParticle, Data.GetNumParticles() - StartParticle);
}

void FNiagaraSimulation::RunVMScript(UNiagaraScript* Script, EUnusedAttributeBehaviour UnusedAttribBehaviour, uint32 StartParticle, uint32 NumParticles)
{
	if (NumParticles == 0)
		return;

	check(Script);
	check(Script->ByteCode.Num() > 0);
	check(Script->Attributes.Num() > 0);
	check(Data.GetNumAttributes() > 0);

	uint32 CurrNumParticles = Data.GetNumParticles();

	check(StartParticle + NumParticles <= CurrNumParticles);

	const FVector4* InputBuffer = Data.GetPreviousBuffer();
	const FVector4* OutputBuffer = Data.GetCurrentBuffer();

	uint32 InputAttributeStride = Data.GetPrevParticleAllocation();
	uint32 OutputAttributeStride = Data.GetParticleAllocation();

	VectorRegister* InputRegisters[VectorVM::MaxInputRegisters] = { 0 };
	VectorRegister* OutputRegisters[VectorVM::MaxOutputRegisters] = { 0 };
	
	//The current script run will be using NumScriptAttrs. We must pick out the Attributes form the simulation data in the order that they appear in the script.
	//The remaining attributes being handled according to UnusedAttribBehaviour
	const int32 NumSimulationAttrs = Data.GetNumAttributes();
	const int32 NumScriptAttrs = Script->Attributes.Num();

	check(NumScriptAttrs < VectorVM::MaxInputRegisters);
	check(NumScriptAttrs < VectorVM::MaxOutputRegisters);

	// Setup input and output registers.
	for (TPair<FNiagaraVariableInfo, uint32> AttrIndexPair : Data.GetAttributes())
	{
		const FNiagaraVariableInfo& AttrInfo = AttrIndexPair.Key;

		FVector4* InBuff;
		FVector4* OutBuff;
		Data.GetAttributeData(AttrInfo, InBuff, OutBuff, StartParticle);

		int32 AttrIndex = Script->Attributes.Find(AttrInfo);
		if (AttrIndex != INDEX_NONE)
		{
			//This attribute is used in this script so set it to the correct Input and Output buffer locations.
			InputRegisters[AttrIndex] = (VectorRegister*)InBuff;
			OutputRegisters[AttrIndex] = (VectorRegister*)OutBuff;
		}	
		else
		{
			//This attribute is not used in this script so handle it accordingly.
			check(OutBuff != NULL);
			if (UnusedAttribBehaviour == EUnusedAttributeBehaviour::Copy)
			{
				check(InBuff != NULL);
				FMemory::Memcpy(OutBuff, InBuff, NumParticles * sizeof(FVector4));
			}
			else if (UnusedAttribBehaviour == EUnusedAttributeBehaviour::Zero)
			{
				FMemory::Memzero(OutBuff, NumParticles * sizeof(FVector4));
			}
			else if (UnusedAttribBehaviour == EUnusedAttributeBehaviour::MarkInvalid)
			{
				FMemory::Memset(OutBuff, NIAGARA_INVALID_MEMORY, NumParticles * sizeof(FVector4));
			}
		}
	}

	//Fill constant table with required emitter constants and internal script constants.
	TArray<FVector4> ConstantTable;
	TArray<UNiagaraDataObject *>DataObjTable;
	Script->ConstantData.FillConstantTable(Constants, ConstantTable, DataObjTable);

	VectorVM::Exec(
		Script->ByteCode.GetData(),
		InputRegisters,
		NumScriptAttrs,
		OutputRegisters,
		NumScriptAttrs,
		ConstantTable.GetData(),
		DataObjTable.GetData(),
		NumParticles
		);
}

bool FNiagaraSimulation::CheckAttriubtesForRenderer()
{
	bool bOk = true;
	if (EffectRenderer)
	{
		const TArray<FNiagaraVariableInfo>& RequiredAttrs = EffectRenderer->GetRequiredAttributes();

		for (auto& Attr : RequiredAttrs)
		{
			if (!Data.HasAttriubte(Attr))
			{
				bOk = false;
				UE_LOG(LogNiagara, Error, TEXT("Cannot render %s because it does not define attriubte %s."), *Props->EmitterName, *Attr.Name.ToString());
			}
		}
	}
	return bOk;
}

/** Replace the current effect renderer with a new one of Type, transferring the existing material over
 *	to the new renderer. Don't forget to call RenderModuleUpdate on the SceneProxy after calling this! 
 */
void FNiagaraSimulation::SetRenderModuleType(EEmitterRenderModuleType Type, ERHIFeatureLevel::Type FeatureLevel)
{
	if (Type != Props->RenderModuleType || EffectRenderer==nullptr)
	{
		UMaterial *Material = UMaterial::GetDefaultMaterial(MD_Surface);

		if (EffectRenderer)
		{
			Material = EffectRenderer->GetMaterial();
			delete EffectRenderer;
		}
		else
		{
			if (Props->Material)
			{
				Material = Props->Material;
			}
		}

		Props->RenderModuleType = Type;
		switch (Type)
		{
		case RMT_Sprites: EffectRenderer = new NiagaraEffectRendererSprites(FeatureLevel, Props->RendererProperties);
			break;
		case RMT_Ribbon: EffectRenderer = new NiagaraEffectRendererRibbon(FeatureLevel, Props->RendererProperties);
			break;
		default:	EffectRenderer = new NiagaraEffectRendererSprites(FeatureLevel, Props->RendererProperties);
					Props->RenderModuleType = RMT_Sprites;
					break;
		}

		EffectRenderer->SetMaterial(Material, FeatureLevel);
		CheckAttriubtesForRenderer();
	}
}


//////////////////////////////////////////////////////////////////////////

UNiagaraEmitterProperties::UNiagaraEmitterProperties(const FObjectInitializer& Initiilaizer)
: Super(Initiilaizer)
, EmitterName(TEXT("New Emitter"))
, bIsEnabled(true)
, SpawnRate(50)
, Material(nullptr)
, RenderModuleType(RMT_Sprites)
, StartTime(0.0f)
, EndTime(0.0f)
, RendererProperties(nullptr)
, NumLoops(0)
{
}

#if WITH_EDITOR
void UNiagaraEmitterProperties::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	Init();
}
#endif

void UNiagaraEmitterProperties::InitFromOldStruct(FDeprecatedNiagaraEmitterProperties& OldStruct)
{
	EmitterName = OldStruct.Name;
	bIsEnabled = OldStruct.bIsEnabled;
	SpawnRate = OldStruct.SpawnRate;
	UpdateScriptProps.Script = OldStruct.UpdateScript;
	SpawnScriptProps.Script = OldStruct.SpawnScript;
	Material = OldStruct.Material;
	RenderModuleType = OldStruct.RenderModuleType;
	StartTime = OldStruct.StartTime;
	EndTime = OldStruct.EndTime;
	RendererProperties = OldStruct.RendererProperties;

	UpdateScriptProps.ExternalConstants.InitFromOldMap(OldStruct.ExternalConstants);
	SpawnScriptProps.ExternalConstants.InitFromOldMap(OldStruct.ExternalSpawnConstants);
	NumLoops = OldStruct.NumLoops;
}