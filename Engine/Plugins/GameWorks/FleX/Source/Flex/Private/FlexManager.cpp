#include "FlexManager.h"

#include "FlexAsset.h"

#include "FlexFluidSurfaceActor.h"
#include "FlexFluidSurfaceComponent.h"

#include "FlexComponent.h"
#include "FlexContainer.h"
#include "FlexContainerInstance.h"
#include "DrawDebugHelpers.h" // FlushPersistentDebugLines

#include "Engine/StaticMesh.h"
#include "FlexStaticMesh.h"

#include "EngineUtils.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "ParticleEmitterInstances.h"
#include "Particles/ParticleEmitter.h"
#include "FlexParticleEmitterInstance.h"
#include "FlexCollisionComponent.h"
#include "FlexParticleSpriteEmitter.h"
#include "FlexGPUParticleEmitterInstance.h"

#include "Misc/ScopeRWLock.h"

void FlexErrorFunc(NvFlexErrorSeverity, const char* msg, const char* file, int line)
{
	UE_LOG(LogFlex, Warning, TEXT("Flex Error: %s, %s:%d"), ANSI_TO_TCHAR(msg), ANSI_TO_TCHAR(file), line);
}

FFlexManager::FFlexManager()
{
	bFlexInitialized = false;
	FlexLib = nullptr;
}

void FFlexManager::InitGamePhysPostRHI()
{
	if (!GUsingNullRHI)
	{
		NvFlexInitDesc desc;
		memset(&desc, 0, sizeof(NvFlexInitDesc));

#if WITH_FLEX_CUDA
		// query the CUDA device index from the NVIDIA control panel
		int SuggestedOrdinal = NvFlexDeviceGetSuggestedOrdinal();

		// create an optimized CUDA context for Flex, the context will
		// be made current on the calling thread, note that if using
		// GPU PhysX then it is recommended to skip this step and use
		// the same CUDA context as PhysX
		NvFlexDeviceCreateCudaContext(SuggestedOrdinal);

		desc.computeType = eNvFlexCUDA;
#else

		static const bool bD3D12 = FParse::Param(FCommandLine::Get(), TEXT("d3d12")) || FParse::Param(FCommandLine::Get(), TEXT("dx12"));
		desc.computeType = bD3D12 ? eNvFlexD3D12 : eNvFlexD3D11;

#endif
		FlexLib = NvFlexInit(NV_FLEX_VERSION, FlexErrorFunc, &desc);
		if (FlexLib)
		{
			UE_LOG(LogFlex, Display, TEXT("Initialized Flex with GPU: %s"), ANSI_TO_TCHAR(NvFlexGetDeviceName(FlexLib)));
		}
	}

	if (FlexLib != nullptr)
	{
		bFlexInitialized = true;
	}
}

void FFlexManager::TermGamePhys()
{
	if (bFlexInitialized)
	{
		NvFlexShutdown(FlexLib);
		FlexLib = nullptr;

		bFlexInitialized = false;
	}
}


class UFlexAsset* FFlexManager::GetFlexAsset(class UStaticMesh* StaticMesh)
{
	UFlexStaticMesh* FSM = Cast<UFlexStaticMesh>(StaticMesh);
	return FSM ? FSM->FlexAsset : nullptr;
}

bool FFlexManager::HasFlexAsset(class UStaticMesh* StaticMesh)
{
	return FFlexManager::GetFlexAsset(StaticMesh) != nullptr;
}

void FFlexManager::ReImportFlexAsset(class UStaticMesh* StaticMesh)
{
	class UFlexAsset* FlexAsset = FFlexManager::GetFlexAsset(StaticMesh);
	if (FlexAsset)
	{
		FlexAsset->ReImport(StaticMesh);
	}
}

class UFlexFluidSurfaceComponent* FFlexManager::GetFlexFluidSurface(class UWorld* World, class UFlexFluidSurface* FlexFluidSurface)
{
	check(World);
	FFlexFuildSurfaceMap* FlexFluidSurfaceMap = WorldMap.Find(TWeakObjectPtr<UWorld>(World));
	if (FlexFluidSurfaceMap)
	{
		check(FlexFluidSurface);
		UFlexFluidSurfaceComponent** Component = FlexFluidSurfaceMap->Find(FlexFluidSurface);
		if (Component) return *Component;
	}
	return nullptr;
}

class UFlexFluidSurfaceComponent* FFlexManager::AddFlexFluidSurface(class UWorld* World, class UFlexFluidSurface* FlexFluidSurface)
{
	check(World);
	FFlexFuildSurfaceMap& FlexFluidSurfaceMap = WorldMap.FindOrAdd(TWeakObjectPtr<UWorld>(World));

	check(FlexFluidSurface);
	UFlexFluidSurfaceComponent** Component = FlexFluidSurfaceMap.Find(FlexFluidSurface);
	if (Component)
	{
		return *Component;
	}
	else
	{
		FActorSpawnParameters ActorSpawnParameters;
		//necessary for preview in blueprint editor
		ActorSpawnParameters.bAllowDuringConstructionScript = true;
		AFlexFluidSurfaceActor* NewActor = World->SpawnActor<AFlexFluidSurfaceActor>(AFlexFluidSurfaceActor::StaticClass(), ActorSpawnParameters);
		check(NewActor);

		UFlexFluidSurfaceComponent* NewComponent = NewActor->GetComponent();
		NewComponent->FlexFluidSurface = FlexFluidSurface;	//can't pass arbitrary parameters into SpawnActor			

		FlexFluidSurfaceMap.Add(FlexFluidSurface, NewComponent);
		return NewComponent;
	}
}

void FFlexManager::RemoveFlexFluidSurface(class UWorld* World, class UFlexFluidSurfaceComponent* Component)
{
	check(World);
	FFlexFuildSurfaceMap* FlexFluidSurfaceMap = WorldMap.Find(TWeakObjectPtr<UWorld>(World));
	if (FlexFluidSurfaceMap)
	{
		check(Component && Component->FlexFluidSurface);
		FlexFluidSurfaceMap->Remove(Component->FlexFluidSurface);
		AFlexFluidSurfaceActor* Actor = (AFlexFluidSurfaceActor*)Component->GetOwner();
		Actor->Destroy();
	}
}

void FFlexManager::TickFlexFluidSurfaceComponents(class UWorld* World, const TArray<struct FParticleEmitterInstance*>& EmitterInstances, float DeltaTime, enum ELevelTick TickType)
{
	int32 NumEmitters = EmitterInstances.Num();
	TSet<class UFlexFluidSurfaceComponent*> FlexFluidSurfaces;
	for (int32 EmitterIndex = 0; EmitterIndex < NumEmitters; EmitterIndex++)
	{
		FParticleEmitterInstance* EmitterInstance = EmitterInstances[EmitterIndex];
		if (EmitterInstance && EmitterInstance->SpriteTemplate)
		{
			auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(EmitterInstance->SpriteTemplate);
			auto FlexFluidSurfaceTemplate = FlexEmitter ? FlexEmitter->FlexFluidSurfaceTemplate : nullptr;
			if (FlexFluidSurfaceTemplate)
			{
				class UFlexFluidSurfaceComponent* SurfaceComponent = FFlexManager::GetFlexFluidSurface(World, FlexFluidSurfaceTemplate);
				check(SurfaceComponent);
				if (!FlexFluidSurfaces.Contains(SurfaceComponent))
				{
					SurfaceComponent->TickComponent(DeltaTime, TickType, NULL);

					FlexFluidSurfaces.Add(SurfaceComponent);
				}
			}
		}
	}
}

struct FFlexContainerInstance* FFlexManager::FindFlexContainerInstance(class FPhysScene* PhysScene, class UFlexContainer* Template)
{
	if (!bFlexInitialized)
	{
		return nullptr;
	}

	FRWScopeLock Lock(PhysSceneMapLock, SLT_ReadOnly);

	FPhysSceneContext* PhysSceneContext = PhysSceneMap.Find(PhysScene);
	if (PhysSceneContext)
	{
		FFlexContainerInstance** Instance = PhysSceneContext->FlexContainerMap.Find(Template);
		if (Instance)
		{
			return *Instance;
		}
	}
	return nullptr;
}

struct FFlexContainerInstance* FFlexManager::FindOrCreateFlexContainerInstance(class FPhysScene* PhysScene, class UFlexContainer* Template)
{
	if (!bFlexInitialized)
	{
		return nullptr;
	}

	FRWScopeLock Lock(PhysSceneMapLock, SLT_Write);

	FPhysSceneContext& PhysSceneContext = PhysSceneMap.FindOrAdd(PhysScene);

	FFlexContainerInstance** Instance = PhysSceneContext.FlexContainerMap.Find(Template);
	if (Instance)
	{
		return *Instance;
	}
	else
	{
		// Make a copy of the UFlexContainer so that modifying it in blueprint doesn't change the asset
		// The owning object will be the Transient Pacakge
		auto ContainerCopy = DuplicateObject<UFlexContainer>(Template, GetTransientPackage());

		// no Garbage Collection please, we need this object to last as long as the FFlexContainerInstance
		ContainerCopy->AddToRoot();
		FFlexContainerInstance* NewInst = new FFlexContainerInstance(ContainerCopy, PhysScene);
		PhysSceneContext.FlexContainerMap.Add(Template, NewInst);

		return NewInst;
	}
}

void FFlexManager::WaitFlexScenes(class FPhysScene* PhysScene)
{
	if (!bFlexInitialized)
	{
		return;
	}

	PhysSceneMapLock.ReadLock();

	FPhysSceneContext* PhysSceneContext = PhysSceneMap.Find(PhysScene);
	if (PhysSceneContext == nullptr)
	{
		PhysSceneMapLock.ReadUnlock();
		return;
	}

	// wait for async task to finish
	auto FlexSimulateTaskRef = PhysSceneContext->FlexSimulateTaskRef; //make a copy

	PhysSceneMapLock.ReadUnlock();

	if (FlexSimulateTaskRef.IsValid())
	{
		FTaskGraphInterface::Get().WaitUntilTaskCompletes(FlexSimulateTaskRef);
	}

	PhysSceneMapLock.WriteLock();

	PhysSceneContext->FlexSimulateTaskRef.SafeRelease();

	if (PhysSceneContext->FlexContainerMap.Num() > 0)
	{
		// if debug draw enabled on any containers then ensure any persistent lines are flushed
		bool NeedsFlushDebugLines = false;

		for (auto It = PhysSceneContext->FlexContainerMap.CreateIterator(); It; ++It)
		{
			// The container instances can be removed, so we need to check and handle that case
			if (!It->Value->TemplateRef.IsValid())
			{
				delete It->Value;
				It.RemoveCurrent();
			}
			else if (It->Value->Template->DebugDraw)
			{
				NeedsFlushDebugLines = true;
				break;
			}
		}

		if (FFlexContainerInstance::sGlobalDebugDraw || NeedsFlushDebugLines)
		{
			FlushPersistentDebugLines(PhysScene->GetOwningWorld());
		}

		// synchronize flex components with results
		for (auto It = PhysSceneContext->FlexContainerMap.CreateIterator(); It; ++It)
		{
			It->Value->Synchronize();
		}
	}

	PhysSceneMapLock.WriteUnlock();
}

void FFlexManager::TickFlexScenes(class FPhysScene* PhysScene, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent, float dt)
{
	if (!bFlexInitialized)
	{
		return;
	}

	FRWScopeLock Lock(PhysSceneMapLock, SLT_ReadOnly);

	FPhysSceneContext* PhysSceneContext = PhysSceneMap.Find(PhysScene);
	if (PhysSceneContext && PhysSceneContext->FlexContainerMap.Num() > 0)
	{
		// when true the Flex CPU update will be run as a task async to the game thread
		// note that this is different from the async tick in LevelTick.cpp
		const bool bFlexAsync = true;

		if (bFlexAsync)
		{
			PhysSceneContext->FlexSimulateTaskRef = FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
				FSimpleDelegateGraphTask::FDelegate::CreateRaw(this, &FFlexManager::TickFlexScenesTask, PhysSceneContext, dt, false),
				GET_STATID(STAT_TotalPhysicsTime));
		}
		else
		{
			Lock.RaiseLockToWrite();

			TickFlexScenesTask(PhysSceneContext, dt, true);
		}
	}
}

void FFlexManager::TickFlexScenesTask(FFlexManager::FPhysSceneContext* PhysSceneContext, float dt, bool bIsLocked)
{
	check(PhysSceneContext);

	if (!bIsLocked)
	{
		PhysSceneMapLock.WriteLock();
	}

	if (PhysSceneContext->FlexContainerMap.Num() > 0)
	{
		// ensure we have the correct CUDA context set for Flex
		// this would be done automatically when making a Flex API call
		// but by acquiring explicitly in advance we save some unnecessary
		// CUDA calls to repeatedly set/unset the context
		NvFlexAcquireContext(FlexLib);

		for (auto It = PhysSceneContext->FlexContainerMap.CreateIterator(); It; ++It)
		{
			// if template has been garbage collected then remove container (need to use the thread safe IsValid() flag)
			if (!It->Value->TemplateRef.IsValid(false, true))
			{
				delete It->Value;
				It.RemoveCurrent();
			}
			else
			{
				It->Value->Simulate(dt);
			}
		}

		NvFlexRestoreContext(FlexLib);
	}

	if (!bIsLocked)
	{
		PhysSceneMapLock.WriteUnlock();
	}
}

void FFlexManager::CleanupFlexScenes(class FPhysScene* PhysScene)
{
	if (!bFlexInitialized)
	{
		return;
	}

	PhysSceneMapLock.ReadLock();

	FPhysSceneContext* PhysSceneContext = PhysSceneMap.Find(PhysScene);
	if (PhysSceneContext == nullptr)
	{
		PhysSceneMapLock.ReadUnlock();
		return;
	}

	// wait for async task to finish
	auto FlexSimulateTaskRef = PhysSceneContext->FlexSimulateTaskRef; //make a copy

	PhysSceneMapLock.ReadUnlock();

	if (FlexSimulateTaskRef.IsValid())
	{
		FTaskGraphInterface::Get().WaitUntilTaskCompletes(FlexSimulateTaskRef);
	}

	PhysSceneMapLock.WriteLock();

	PhysSceneContext->FlexSimulateTaskRef.SafeRelease();

	for (auto It = PhysSceneContext->FlexContainerMap.CreateIterator(); It; ++It)
	{
		UFlexContainer* FlexContainerCopy = It->Value->Template;

		delete It->Value;

		// Destroy the UFlexContainer copy that was created by FindOrCreateFlexContainerInstance()
		if (FlexContainerCopy != nullptr && FlexContainerCopy->IsValidLowLevel())
		{
			FlexContainerCopy->ConditionalBeginDestroy();
		}
	}

	PhysSceneMap.Remove(PhysScene);

	PhysSceneMapLock.WriteUnlock();
}

void FFlexManager::StartFlexRecord(class FPhysScene* PhysScene)
{
#if 0
	FRWScopeLock Lock(PhysSceneMapLock, SLT_ReadOnly);

	FPhysSceneContext* PhysSceneContext = PhysSceneMap.Find(PhysScene);
	if (PhysSceneContext)
	{
		for (auto It = PhysSceneContext->FlexContainerMap.CreateIterator(); It; ++It)
		{
			FFlexContainerInstance* Container = It->Value;
			FString Name = Container->Template->GetName();

			flexStartRecord(Container->Solver, StringCast<ANSICHAR>(*(FString("flexCapture_") + Name + FString(".flx"))).Get());
		}
	}
#endif
}

void FFlexManager::StopFlexRecord(class FPhysScene* PhysScene)
{
#if 0
	FRWScopeLock Lock(PhysSceneMapLock, SLT_ReadOnly);

	FPhysSceneContext* PhysSceneContext = PhysSceneMap.Find(PhysScene);
	if (PhysSceneContext)
	{
		for (auto It = PhysSceneContext->FlexContainerMap.CreateIterator(); It; ++It)
		{
			FFlexContainerInstance* Container = It->Value;

			flexStopRecord(Container->Solver);
		}
	}
#endif
}

void FFlexManager::AddRadialForceToFlex(class FPhysScene* PhysScene, FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff)
{
	FRWScopeLock Lock(PhysSceneMapLock, SLT_ReadOnly);

	FPhysSceneContext* PhysSceneContext = PhysSceneMap.Find(PhysScene);
	if (PhysSceneContext)
	{
		for (auto It = PhysSceneContext->FlexContainerMap.CreateIterator(); It; ++It)
		{
			FFlexContainerInstance* Container = It->Value;
			Container->AddRadialForce(Origin, Radius, Strength, Falloff);
		}
	}
}

void FFlexManager::AddRadialImpulseToFlex(class FPhysScene* PhysScene, FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff, bool bVelChange)
{
	FRWScopeLock Lock(PhysSceneMapLock, SLT_ReadOnly);

	FPhysSceneContext* PhysSceneContext = PhysSceneMap.Find(PhysScene);
	if (PhysSceneContext)
	{
		for (auto It = PhysSceneContext->FlexContainerMap.CreateIterator(); It; ++It)
		{
			FFlexContainerInstance* Container = It->Value;
			Container->AddRadialImpulse(Origin, Radius, Strength, Falloff, bVelChange);
		}
	}
}

void FFlexManager::ToggleFlexContainerDebugDraw(class UWorld* InWorld)
{
	FFlexContainerInstance::sGlobalDebugDraw = !FFlexContainerInstance::sGlobalDebugDraw;

	// if disabling debug draw ensure any persistent lines are flushed
	if (!FFlexContainerInstance::sGlobalDebugDraw)
		FlushPersistentDebugLines(InWorld);
}

void FFlexManager::AttachFlexToComponent(class USceneComponent* Component, float Radius)
{
	const FVector Origin = Component->GetComponentLocation();

	for (TActorIterator<AFlexActor> It(Component->GetWorld()); It; ++It)
	{
		AFlexActor* FlexActor = (*It);
		UFlexComponent* FlexComponent = Cast<UFlexComponent>(FlexActor->GetRootComponent());

		if (FlexComponent)
		{
			const FBoxSphereBounds FlexBounds = FlexComponent->GetBounds();

			// dist of force field to flex bounds
			const float DistSq = FlexBounds.ComputeSquaredDistanceFromBoxToPoint(Origin);

			if (DistSq < Radius*Radius)
			{
				FlexComponent->AttachToComponent(Component, Radius);
			}
		}
	}

	// Find all ParticleSystemComponents
	for (TActorIterator<AActor> It(Component->GetWorld()); It; ++It)
	{
		AActor* TestActor = (*It);
		TArray<UActorComponent*> ParticleComponents = TestActor->GetComponentsByClass(UParticleSystemComponent::StaticClass());
		for (int32 ComponentIdx = 0; ComponentIdx < ParticleComponents.Num(); ++ComponentIdx)
		{
			auto ParticleSystemComponet = Cast<UParticleSystemComponent>(ParticleComponents[ComponentIdx]);

			// is this a PSC with Flex?
			if (ParticleSystemComponet && FFlexManager::GetFirstFlexContainerTemplate(ParticleSystemComponet))
			{
				const FBoxSphereBounds FlexBounds = ParticleSystemComponet->CalcBounds(FTransform::Identity);

				// dist of force field to flex bounds
				const float DistSq = FlexBounds.ComputeSquaredDistanceFromBoxToPoint(Origin);

				if (DistSq < Radius*Radius)
				{
					{
						// Forward to all Flex emitters
						// TODO: check for actual overlaps first
						for (int32 EmitterIndex = 0; EmitterIndex < ParticleSystemComponet->EmitterInstances.Num(); EmitterIndex++)
						{
							FParticleEmitterInstance* EmitterInstance = ParticleSystemComponet->EmitterInstances[EmitterIndex];
							if (EmitterInstance && EmitterInstance->SpriteTemplate && EmitterInstance->FlexEmitterInstance)
							{
								auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(EmitterInstance->SpriteTemplate);
								if (FlexEmitter && FlexEmitter->FlexContainerTemplate)
								{
									EmitterInstance->FlexEmitterInstance->AddPendingComponentToAttach(Component, Radius);
								}
							}
						}

					}
				}
			}
		}
	}
}

void FFlexManager::CreateFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance)
{
	if (EmitterInstance->FlexEmitterInstance)
	{
		delete EmitterInstance->FlexEmitterInstance;
		EmitterInstance->FlexEmitterInstance = nullptr;
	}

	auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(EmitterInstance->SpriteTemplate);
	if (FlexEmitter)
	{
		if (FlexEmitter->FlexContainerTemplate && (!GIsEditor || GIsPlayInEditorWorld))
		{
			FPhysScene* scene = EmitterInstance->Component->GetWorld()->GetPhysicsScene();
			if (scene)
			{
				EmitterInstance->FlexEmitterInstance = new FFlexParticleEmitterInstance(EmitterInstance);
			}
		}
	}
}

void FFlexManager::DestroyFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance)
{
	if (EmitterInstance->FlexEmitterInstance)
	{
		delete EmitterInstance->FlexEmitterInstance;
		EmitterInstance->FlexEmitterInstance = nullptr;
	}
}

void FFlexManager::TickFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance, float DeltaTime, bool bSuppressSpawning)
{
	if (EmitterInstance->FlexEmitterInstance)
	{
		EmitterInstance->FlexEmitterInstance->Tick(DeltaTime, bSuppressSpawning);
	}
}

uint32 FFlexManager::GetFlexEmitterInstanceRequiredBytes(struct FParticleEmitterInstance* EmitterInstance, uint32 uiBytes)
{
	if (EmitterInstance->FlexEmitterInstance)
	{
		uiBytes = EmitterInstance->FlexEmitterInstance->GetRequiredBytes(uiBytes);
	}
	return uiBytes;
}

bool FFlexManager::FlexEmitterInstanceSpawnParticle(struct FParticleEmitterInstance* EmitterInstance, struct FBaseParticle* Particle, uint32 CurrentParticleIndex)
{
	if (EmitterInstance->FlexEmitterInstance)
	{
		return EmitterInstance->FlexEmitterInstance->SpawnParticle(Particle, CurrentParticleIndex);
	}
	return true;
}

void FFlexManager::FlexEmitterInstanceKillParticle(struct FParticleEmitterInstance* EmitterInstance, int32 KillIndex)
{
	if (EmitterInstance->FlexEmitterInstance)
	{
		EmitterInstance->FlexEmitterInstance->KillParticle(KillIndex);
	}
}

void FFlexManager::FlexEmitterInstanceFillReplayData(struct FParticleEmitterInstance* EmitterInstance, struct FDynamicSpriteEmitterReplayDataBase* ReplayData)
{
	if (EmitterInstance->FlexEmitterInstance)
	{
		EmitterInstance->FlexEmitterInstance->FillReplayData(ReplayData);
	}
}

FRenderResource* FFlexManager::GPUSpriteEmitterInstance_Init(struct FFlexParticleEmitterInstance* FlexEmitterInstance)
{
	verify(FlexEmitterInstance);
	verify(FlexEmitterInstance->GPUImpl == nullptr);
	FlexEmitterInstance->GPUImpl = new FFlexGPUParticleEmitterInstance(FlexEmitterInstance);
	verify(FlexEmitterInstance->GPUImpl);
	return FlexEmitterInstance->GPUImpl->CreateSimulationResource();
}

void FFlexManager::GPUSpriteEmitterInstance_Tick(struct FFlexParticleEmitterInstance* FlexEmitterInstance, float DeltaSeconds, bool bSuppressSpawning, FRenderResource* FlexSimulationResource)
{
	verify(FlexEmitterInstance);
	verify(FlexEmitterInstance->GPUImpl);
	FlexEmitterInstance->GPUImpl->Tick(DeltaSeconds, bSuppressSpawning, FlexSimulationResource);
}

void FFlexManager::GPUSpriteEmitterInstance_DestroyParticles(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 Start, int32 Count)
{
	verify(FlexEmitterInstance);
	verify(FlexEmitterInstance->GPUImpl);
	FlexEmitterInstance->GPUImpl->DestroyParticles(Start, Count);
}

void FFlexManager::GPUSpriteEmitterInstance_DestroyAllParticles(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 ParticlesPerTile, bool bFreeParticleIndices)
{
	verify(FlexEmitterInstance);
	if (FlexEmitterInstance->GPUImpl)
	{
		FlexEmitterInstance->GPUImpl->DestroyAllParticles(ParticlesPerTile, bFreeParticleIndices);
	}
}

void FFlexManager::GPUSpriteEmitterInstance_AllocParticleIndices(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 Count)
{
	verify(FlexEmitterInstance);
	verify(FlexEmitterInstance->GPUImpl);
	FlexEmitterInstance->GPUImpl->AllocParticleIndices(Count);
}

void FFlexManager::GPUSpriteEmitterInstance_FreeParticleIndices(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 Start, int32 Count)
{
	verify(FlexEmitterInstance);
	verify(FlexEmitterInstance->GPUImpl);
	FlexEmitterInstance->GPUImpl->FreeParticleIndices(Start, Count);
}

int32 FFlexManager::GPUSpriteEmitterInstance_CreateNewParticles(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 NewStart, int32 NewCount)
{
	verify(FlexEmitterInstance);
	verify(FlexEmitterInstance->GPUImpl);
	return FlexEmitterInstance->GPUImpl->CreateNewParticles(NewStart, NewCount);
}

void FFlexManager::GPUSpriteEmitterInstance_DestroyNewParticles(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 NewStart, int32 NewCount)
{
	verify(FlexEmitterInstance);
	verify(FlexEmitterInstance->GPUImpl);
	FlexEmitterInstance->GPUImpl->DestroyNewParticles(NewStart, NewCount);
}

void FFlexManager::GPUSpriteEmitterInstance_InitNewParticle(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 NewIndex, int32 RegularIndex)
{
	verify(FlexEmitterInstance);
	verify(FlexEmitterInstance->GPUImpl);
	FlexEmitterInstance->GPUImpl->InitNewParticle(NewIndex, RegularIndex);
}

void FFlexManager::GPUSpriteEmitterInstance_SetNewParticle(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 NewIndex, const FVector& Position, const FVector& Velocity)
{
	verify(FlexEmitterInstance);
	verify(FlexEmitterInstance->GPUImpl);
	FlexEmitterInstance->GPUImpl->SetNewParticle(NewIndex, Position, Velocity);
}

void FFlexManager::GPUSpriteEmitterInstance_FillSimulationParams(FRenderResource* FlexSimulationResource, FFlexGPUParticleSimulationParameters& SimulationParams)
{
	verify(FlexSimulationResource);
	return FFlexGPUParticleEmitterInstance::FillSimulationParams(FlexSimulationResource, SimulationParams);
}


UObject* FFlexManager::GetFirstFlexContainerTemplate(class UParticleSystemComponent* Component)
{
	for (int32 EmitterIndex = 0; EmitterIndex < Component->EmitterInstances.Num(); EmitterIndex++)
	{
		FParticleEmitterInstance* EmitterInstance = Component->EmitterInstances[EmitterIndex];
		if (EmitterInstance && EmitterInstance->SpriteTemplate)
		{
			auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(EmitterInstance->SpriteTemplate);
			if (FlexEmitter && FlexEmitter->FlexContainerTemplate)
			{
				FPhysScene* Scene = EmitterInstance->Component->GetWorld()->GetPhysicsScene();

				FFlexContainerInstance* ContainerInstance = FFlexManager::FindOrCreateFlexContainerInstance(Scene, FlexEmitter->FlexContainerTemplate);
				return ContainerInstance ? ContainerInstance->Template : nullptr;
			}
		}
	}
	return nullptr;
}

UMaterialInstanceDynamic* FFlexManager::CreateFlexDynamicMaterialInstance(class UParticleSystemComponent* Component, class UMaterialInterface* SourceMaterial)
{
	if (!SourceMaterial)
	{
		return nullptr;
	}

	for (int32 EmitterIndex = 0; EmitterIndex < Component->EmitterInstances.Num(); EmitterIndex++)
	{
		FParticleEmitterInstance* EmitterInstance = Component->EmitterInstances[EmitterIndex];
		if (EmitterInstance && 	EmitterInstance->SpriteTemplate)
		{
			auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(EmitterInstance->SpriteTemplate);
			if (FlexEmitter && FlexEmitter->FlexFluidSurfaceTemplate && FlexEmitter->FlexFluidSurfaceTemplate->Material)
			{
				UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(SourceMaterial);

				if (!MID)
				{
					// Create and set the dynamic material instance.
					MID = UMaterialInstanceDynamic::Create(SourceMaterial, Component);
				}

				if (MID)
				{
					// Make a copy of the FlexFluidSurfaceTemplate
					auto NewFlexFluidSurface = DuplicateObject<UFlexFluidSurface>(FlexEmitter->FlexFluidSurfaceTemplate, Component);
					// Set the material in the new FlexFluidSurfaceTemplate
					NewFlexFluidSurface->Material = MID;
					// Set the FlexFluidSurfaceTemplate override in this class
					Component->FlexFluidSurfaceOverride = NewFlexFluidSurface;

					if (EmitterInstance->FlexEmitterInstance)
					{
						// Tell the ParticleEmiterInstance to update its FlexFluidSurfaceComponent
						EmitterInstance->FlexEmitterInstance->RegisterNewFlexFluidSurfaceComponent(NewFlexFluidSurface);
					}
				}
				else
				{
					UE_LOG(LogFlex, Warning, TEXT("CreateFlexDynamicMaterialInstance on %s: Material is invalid."), *Component->GetPathName());
				}

				return MID;
			}
		}
	}
	return NULL;
}

void FFlexManager::UpdateFlexSurfaceDynamicData(class UParticleSystemComponent* Component, struct FParticleEmitterInstance* EmitterInstance, struct FDynamicEmitterDataBase* EmitterDynamicData)
{
	check(Component);
	check(EmitterInstance);
	check(EmitterDynamicData);

	if (Component->SceneProxy)
	{
		auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(EmitterInstance->SpriteTemplate);
		auto FlexFluidSurfaceTemplate = FlexEmitter ? FlexEmitter->FlexFluidSurfaceTemplate : nullptr;

		UFlexFluidSurface* FlexFluidSurfaceOverride = Cast<UFlexFluidSurface>(Component->FlexFluidSurfaceOverride);
		UFlexFluidSurface* FlexFluidSurface = FlexFluidSurfaceOverride ? FlexFluidSurfaceOverride : FlexFluidSurfaceTemplate;
		if (FlexFluidSurface)
		{
			UFlexFluidSurfaceComponent* SurfaceComponent = FFlexManager::GetFlexFluidSurface(Component->GetWorld(), FlexFluidSurface);
			check(SurfaceComponent);

			SurfaceComponent->SendRenderEmitterDynamicData_Concurrent((FParticleSystemSceneProxy*)Component->SceneProxy, EmitterDynamicData);
		}
	}
}

void FFlexManager::ClearFlexSurfaceDynamicData(class UParticleSystemComponent* Component)
{
	check(Component);
	if (Component->SceneProxy)
	{
		for (int32 EmitterIndex = 0; EmitterIndex < Component->EmitterInstances.Num(); EmitterIndex++)
		{
			FParticleEmitterInstance* EmitterInstance = Component->EmitterInstances[EmitterIndex];
			if (EmitterInstance)
			{
				auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(EmitterInstance->SpriteTemplate);
				auto FlexFluidSurfaceTemplate = FlexEmitter ? FlexEmitter->FlexFluidSurfaceTemplate : nullptr;

				UFlexFluidSurface* FlexFluidSurfaceOverride = Cast<UFlexFluidSurface>(Component->FlexFluidSurfaceOverride);
				UFlexFluidSurface* FlexFluidSurface = FlexFluidSurfaceOverride ? FlexFluidSurfaceOverride : FlexFluidSurfaceTemplate;
				if (FlexFluidSurface)
				{
					UFlexFluidSurfaceComponent* SurfaceComponent = FFlexManager::GetFlexFluidSurface(Component->GetWorld(), FlexFluidSurface);
					if (SurfaceComponent)
					{
						SurfaceComponent->SendRenderEmitterDynamicData_Concurrent((FParticleSystemSceneProxy*)Component->SceneProxy, nullptr);
					}
				}
			}
		}
	}
}

void FFlexManager::SetEnabledReferenceCounting(class UParticleSystemComponent* Component, bool bEnable)
{
	check(Component);
	auto FlexFluidSurfaceOverride = Cast<UFlexFluidSurface>(Component->FlexFluidSurfaceOverride);
	if (FlexFluidSurfaceOverride)
	{
		UFlexFluidSurfaceComponent* SurfaceComponent = FFlexManager::GetFlexFluidSurface(Component->GetWorld(), FlexFluidSurfaceOverride);

		check(SurfaceComponent);
		// This is necessary because we need to hold the reference to the fluid surface so it doesn't go away with a SetTemplate() call
		SurfaceComponent->SetEnabledReferenceCounting(bEnable);
	}
}

void FFlexManager::RegisterNewFlexFluidSurfaceComponent(class UParticleSystemComponent* Component, struct FParticleEmitterInstance* EmitterInstance)
{
	check(Component);
	auto FlexFluidSurfaceOverride = Cast<UFlexFluidSurface>(Component->FlexFluidSurfaceOverride);
	auto FlexEmitterInstance = EmitterInstance->FlexEmitterInstance;

	if (FlexFluidSurfaceOverride && FlexEmitterInstance)
	{
		if (EmitterInstance && EmitterInstance->SpriteTemplate)
		{
			auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(EmitterInstance->SpriteTemplate);
			auto FlexFluidSurfaceTemplate = FlexEmitter ? FlexEmitter->FlexFluidSurfaceTemplate : nullptr;

			if (FlexFluidSurfaceTemplate && FlexFluidSurfaceTemplate->Material)
			{
				FlexEmitterInstance->RegisterNewFlexFluidSurfaceComponent(FlexFluidSurfaceOverride);
			}
		}
	}
}

bool FFlexManager::IsValidFlexEmitter(class UParticleEmitter* Emitter)
{
	auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(Emitter);
	return (FlexEmitter && FlexEmitter->FlexContainerTemplate);
}
