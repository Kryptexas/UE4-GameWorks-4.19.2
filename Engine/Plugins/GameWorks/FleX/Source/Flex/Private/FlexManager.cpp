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

				// need to ensure tick happens after GPU update
				EmitterInstance->Component->SetTickGroup(TG_EndPhysics);

				USceneComponent* Parent = EmitterInstance->Component->GetAttachParent();
				if (Parent && FlexEmitter->bLocalSpace)
				{
					//update frame
					const FTransform ParentTransform = Parent->GetComponentTransform();
					const FVector Translation = ParentTransform.GetTranslation();
					const FQuat Rotation = ParentTransform.GetRotation();

					NvFlexExtMovingFrameInit(&EmitterInstance->FlexEmitterInstance->MeshFrame, (float*)(&Translation.X), (float*)(&Rotation.X));
				}
			}
		}

		RegisterNewFlexFluidSurfaceComponent(EmitterInstance, FlexEmitter->FlexFluidSurfaceTemplate);
	}
}

void FFlexManager::DestroyFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance)
{
	if (EmitterInstance->FlexEmitterInstance)
	{
		if (!GIsEditor || GIsPlayInEditorWorld)
		{
			FFlexContainerInstance* Container = EmitterInstance->FlexEmitterInstance->Container;

			if (Container)
			{
				for (int32 i = 0; i < EmitterInstance->ActiveParticles; i++)
				{
					DECLARE_PARTICLE(Particle, EmitterInstance->ParticleData + EmitterInstance->ParticleStride * EmitterInstance->ParticleIndices[i]);
					verify(EmitterInstance->FlexEmitterInstance->FlexDataOffset > 0);
					int32 CurrentOffset = EmitterInstance->FlexEmitterInstance->FlexDataOffset;
					const uint8* ParticleBase = (const uint8*)&Particle;
					PARTICLE_ELEMENT(int32, FlexParticleIndex);
					Container->DestroyParticle(FlexParticleIndex);
				}
			}
		}
		delete EmitterInstance->FlexEmitterInstance;
	}

	if (EmitterInstance->FlexEmitterInstance->FlexFluidSurfaceComponent)
	{
		EmitterInstance->FlexEmitterInstance->FlexFluidSurfaceComponent->UnregisterEmitterInstance(EmitterInstance);
	}
}

void FFlexManager::TickFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance, float DeltaTime, bool bSuppressSpawning)
{
	auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(EmitterInstance->SpriteTemplate);
	if (FlexEmitter == nullptr)
		return;

	if (EmitterInstance->FlexEmitterInstance && EmitterInstance->FlexEmitterInstance->Container && (!GIsEditor || GIsPlayInEditorWorld))
	{
		EmitterInstance->FlexEmitterInstance->ExecutePendingComponentsToAttach();
		EmitterInstance->FlexEmitterInstance->SynchronizeAttachments(DeltaTime);

		// all Flex components should be ticked during the synchronization 
		// phase of the Flex update, which corresponds to the EndPhysics tick group
		verify(EmitterInstance->FlexEmitterInstance->Container->IsMapped());

		FFlexContainerInstance* Container = EmitterInstance->FlexEmitterInstance->Container;

		EmitterInstance->FlexEmitterInstance->bFlexAnisotropyData = (Container->Template->AnisotropyScale > 0.0f);
		verify(!EmitterInstance->FlexEmitterInstance->bFlexAnisotropyData || Container->Anisotropy1.size() > 0);

		// process report shapes
		if (Container->CollisionReportComponents.Num() > 0)
		{
			for (int32 i = 0; i < EmitterInstance->ActiveParticles; i++)
			{
				DECLARE_PARTICLE(Particle, EmitterInstance->ParticleData + EmitterInstance->ParticleStride * EmitterInstance->ParticleIndices[i]);

				verify(EmitterInstance->FlexEmitterInstance->FlexDataOffset > 0);

				int32 CurrentOffset = EmitterInstance->FlexEmitterInstance->FlexDataOffset;
				const uint8* ParticleBase = (const uint8*)&Particle;
				PARTICLE_ELEMENT(int32, FlexParticleIndex);

				const int ContactIndex = Container->ContactIndices[FlexParticleIndex];
				if (ContactIndex == -1)
					continue;

				bool bKillParticle = false;

				const uint32 Count = Container->ContactCounts[ContactIndex];
				for (uint32 c = 0; c < Count; c++)
				{
					FVector4 ContactVelocity = Container->ContactVelocities[ContactIndex*FFlexContainerInstance::MaxContactsPerParticle + c];
					int32 FlexShapeIndex = int(ContactVelocity.W);
					int32 ShapeReportIndex = Container->CollisionReportIndices[FlexShapeIndex];
					if (ShapeReportIndex >= 0)
					{
						UFlexCollisionComponent* CollisionComp = Container->CollisionReportComponents[ShapeReportIndex];

						if (CollisionComp)
						{
							CollisionComp->Count++;

							if (CollisionComp->bDrain)
								bKillParticle = true;
						}
					}
				}

				if (bKillParticle)
				{
					EmitterInstance->KillParticle(i);
					continue;
				}
			}
		}

		FTransform ParentTransform;
		FVector Translation;
		FQuat Rotation;
		USceneComponent* Parent = nullptr;

		if (EmitterInstance->ActiveParticles > 0)
		{
			Parent = EmitterInstance->Component->GetAttachParent();
			if (Parent && FlexEmitter->bLocalSpace)
			{
				//update frame
				ParentTransform = Parent->GetComponentTransform();
				Translation = ParentTransform.GetTranslation();
				Rotation = ParentTransform.GetRotation();

				NvFlexExtMovingFrameUpdate(&EmitterInstance->FlexEmitterInstance->MeshFrame, (float*)(&Translation.X), (float*)(&Rotation.X), DeltaTime);
			}
		}

		// sync UE4 particles with FLEX
		for (int32 i = 0; i < EmitterInstance->ActiveParticles; i++)
		{
			DECLARE_PARTICLE(Particle, EmitterInstance->ParticleData + EmitterInstance->ParticleStride * EmitterInstance->ParticleIndices[i]);

			verify(EmitterInstance->FlexEmitterInstance->FlexDataOffset > 0);

			int32 CurrentOffset = EmitterInstance->FlexEmitterInstance->FlexDataOffset;
			const uint8* ParticleBase = (const uint8*)&Particle;
			PARTICLE_ELEMENT(int32, FlexParticleIndex);

			if (Parent && FlexEmitter->bLocalSpace)
			{
				// Localize the position and velocity using the localization API
				// NOTE: Once we have a feature to detect particle inside the mesh container
				//       we can then test for it and apply localization as needed.
				FVector4* Positions = (FVector4*)&Container->Particles[FlexParticleIndex];
				FVector* Velocities = (FVector*)&Container->Velocities[FlexParticleIndex];

				NvFlexExtMovingFrameApply(&EmitterInstance->FlexEmitterInstance->MeshFrame, (float*)Positions, (float*)Velocities,
					1, EmitterInstance->FlexEmitterInstance->LinearInertialScale, EmitterInstance->FlexEmitterInstance->AngularInertialScale, DeltaTime);
			}

			// sync UE4 particle with FLEX
			if (Container->SmoothPositions.size() > 0)
			{
				Particle.Location = Container->SmoothPositions[FlexParticleIndex];
			}
			else
			{
				Particle.Location = Container->Particles[FlexParticleIndex];
			}

			Particle.Velocity = Container->Velocities[FlexParticleIndex];

			if (EmitterInstance->FlexEmitterInstance->bFlexAnisotropyData)
			{
				PARTICLE_ELEMENT(FVector, Alignment16);

				PARTICLE_ELEMENT(FVector4, FlexAnisotropy1);
				PARTICLE_ELEMENT(FVector4, FlexAnisotropy2);
				PARTICLE_ELEMENT(FVector4, FlexAnisotropy3);

				FlexAnisotropy1 = Container->Anisotropy1[FlexParticleIndex];
				FlexAnisotropy2 = Container->Anisotropy2[FlexParticleIndex];
				FlexAnisotropy3 = Container->Anisotropy3[FlexParticleIndex];
			}
		}
	}
}

uint32 FFlexManager::GetFlexEmitterInstanceRequiredBytes(struct FParticleEmitterInstance* EmitterInstance, uint32 uiBytes)
{
	auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(EmitterInstance->SpriteTemplate);

	if (FlexEmitter && FlexEmitter->FlexContainerTemplate)
	{
		EmitterInstance->FlexEmitterInstance->FlexDataOffset = EmitterInstance->PayloadOffset + uiBytes;

		// flex particle index
		uiBytes += sizeof(int32);

		if (FlexEmitter->FlexContainerTemplate->AnisotropyScale > 0.0f)
		{
			// 16 byte align for inheriting emitter instance types
			uiBytes += sizeof(FVector);

			// flex anisotropy 
			uiBytes += 3 * sizeof(FVector4);
		}
	}
	return uiBytes;
}

bool FFlexManager::FlexEmitterInstanceSpawnParticle(struct FParticleEmitterInstance* EmitterInstance, struct FBaseParticle* Particle, uint32 CurrentParticleIndex)
{
	auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(EmitterInstance->SpriteTemplate);
	if (FlexEmitter == nullptr)
		return true;

	const float FlexInvMass = (FlexEmitter->Mass > 0.0f) ? (1.0f / FlexEmitter->Mass) : 0.0f;

	if (EmitterInstance->FlexEmitterInstance && EmitterInstance->FlexEmitterInstance->Container && (!GIsEditor || GIsPlayInEditorWorld))
	{
		verify(EmitterInstance->FlexEmitterInstance->FlexDataOffset > 0);

		int32 CurrentOffset = EmitterInstance->FlexEmitterInstance->FlexDataOffset;
		const uint8* ParticleBase = (const uint8*)Particle;
		PARTICLE_ELEMENT(int32, FlexParticleIndex);

		// allocate a new particle in the flex solver and store a
		// reference to it in this particle's payload
		FlexParticleIndex = EmitterInstance->FlexEmitterInstance->Container->CreateParticle(FVector4(Particle->Location, FlexInvMass), Particle->Velocity, EmitterInstance->FlexEmitterInstance->Phase);

		if (FlexParticleIndex == -1)
		{
			// could not allocate a flex particle so kill immediately
			EmitterInstance->KillParticle(CurrentParticleIndex);
			return false;
		}

		Particle->Flags |= STATE_Particle_FreezeTranslation;
	}
	return true;
}

void FFlexManager::FlexEmitterInstanceKillParticle(struct FParticleEmitterInstance* EmitterInstance, int32 KillIndex)
{
	if (EmitterInstance->FlexEmitterInstance && EmitterInstance->FlexEmitterInstance->Container && (!GIsEditor || GIsPlayInEditorWorld))
	{
		verify(EmitterInstance->FlexEmitterInstance->FlexDataOffset > 0);

		const uint8* ParticleBase = EmitterInstance->ParticleData + KillIndex * EmitterInstance->ParticleStride;
		int32 CurrentOffset = EmitterInstance->FlexEmitterInstance->FlexDataOffset;
		PARTICLE_ELEMENT(int32, FlexParticleIndex);

		if (FlexParticleIndex >= 0)
		{
			EmitterInstance->FlexEmitterInstance->DestroyParticle(FlexParticleIndex);
		}
	}
}

void FFlexManager::FlexEmitterInstanceFillReplayData(struct FParticleEmitterInstance* EmitterInstance, struct FDynamicSpriteEmitterReplayDataBase* ReplayData)
{
	if (EmitterInstance->FlexEmitterInstance)
	{
		ReplayData->FlexDataOffset = EmitterInstance->FlexEmitterInstance->FlexDataOffset;
		ReplayData->bFlexAnisotropyData = EmitterInstance->FlexEmitterInstance->bFlexAnisotropyData;
		ReplayData->bFlexSurface = (EmitterInstance->FlexEmitterInstance->FlexFluidSurfaceComponent != nullptr);
	}
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

					// Tell the ParticleEmiterInstance to update its FlexFluidSurfaceComponent
					FFlexManager::RegisterNewFlexFluidSurfaceComponent(EmitterInstance, NewFlexFluidSurface);
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
	if (FlexFluidSurfaceOverride)
	{
		if (EmitterInstance && EmitterInstance->SpriteTemplate)
		{
			auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(EmitterInstance->SpriteTemplate);
			auto FlexFluidSurfaceTemplate = FlexEmitter ? FlexEmitter->FlexFluidSurfaceTemplate : nullptr;

			if (FlexFluidSurfaceTemplate && FlexFluidSurfaceTemplate->Material)
			{
				FFlexManager::RegisterNewFlexFluidSurfaceComponent(EmitterInstance, FlexFluidSurfaceOverride);
			}
		}
	}
}

void FFlexManager::RegisterNewFlexFluidSurfaceComponent(struct FParticleEmitterInstance* EmitterInstance, class UFlexFluidSurface* NewFlexFluidSurface)
{
	if (EmitterInstance->FlexEmitterInstance->FlexFluidSurfaceComponent)
	{
		EmitterInstance->FlexEmitterInstance->FlexFluidSurfaceComponent->UnregisterEmitterInstance(EmitterInstance);
		EmitterInstance->FlexEmitterInstance->FlexFluidSurfaceComponent = nullptr;
	}

	if (NewFlexFluidSurface)
	{
		EmitterInstance->FlexEmitterInstance->FlexFluidSurfaceComponent = AddFlexFluidSurface(EmitterInstance->GetWorld(), NewFlexFluidSurface);
		EmitterInstance->FlexEmitterInstance->FlexFluidSurfaceComponent->RegisterEmitterInstance(EmitterInstance);
	}
}

bool FFlexManager::IsValidFlexEmitter(class UParticleEmitter* Emitter)
{
	auto FlexEmitter = Cast<UFlexParticleSpriteEmitter>(Emitter);
	return (FlexEmitter && FlexEmitter->FlexContainerTemplate);
}
