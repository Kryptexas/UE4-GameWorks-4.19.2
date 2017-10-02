#include "FlexPluginBridge.h"

#include "FlexAsset.h"

#include "FlexFluidSurfaceActor.h"
#include "FlexFluidSurfaceComponent.h"

#include "FlexComponent.h"
#include "FlexContainer.h"
#include "FlexContainerInstance.h"
#include "DrawDebugHelpers.h" // FlushPersistentDebugLines

#include "Engine/StaticMesh.h"
#include "FlexStaticMesh.h"

#include "PhysicsEngine/PhysXSupport.h"

#include "EngineUtils.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "ParticleEmitterInstances.h"
#include "Particles/ParticleEmitter.h"
#include "FlexParticleEmitterInstance.h"
#include "FlexCollisionComponent.h"
#include "FlexParticleSystemComponent.h"
#include "FlexParticleEmitter.h"


class UFlexAsset* FFlexPluginBridge::GetFlexAsset(class UStaticMesh* StaticMesh)
{
	UFlexStaticMesh* FSM = Cast<UFlexStaticMesh>(StaticMesh);
	return FSM ? FSM->FlexAsset : nullptr;
}

bool FFlexPluginBridge::HasFlexAsset(class UStaticMesh* StaticMesh)
{
	return FFlexPluginBridge::GetFlexAsset(StaticMesh) != nullptr;
}

void FFlexPluginBridge::ReImportFlexAsset(class UStaticMesh* StaticMesh)
{
	class UFlexAsset* FlexAsset = FFlexPluginBridge::GetFlexAsset(StaticMesh);
	if (FlexAsset)
	{
		FlexAsset->ReImport(StaticMesh);
	}
}

class UFlexFluidSurfaceComponent* FFlexPluginBridge::GetFlexFluidSurface(class UWorld* World, class UFlexFluidSurface* FlexFluidSurface)
{
	check(World);
	FFlexFuildSurfaceMap& FlexFluidSurfaceMap = WorldMap.FindOrAdd(World);

	check(FlexFluidSurface);
	UFlexFluidSurfaceComponent** Component = FlexFluidSurfaceMap.Find(FlexFluidSurface);
	return (Component != NULL) ? *Component : NULL;
}

class UFlexFluidSurfaceComponent* FFlexPluginBridge::AddFlexFluidSurface(class UWorld* World, class UFlexFluidSurface* FlexFluidSurface)
{
	check(World);
	FFlexFuildSurfaceMap& FlexFluidSurfaceMap = WorldMap.FindOrAdd(World);

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

void FFlexPluginBridge::RemoveFlexFluidSurface(class UWorld* World, class UFlexFluidSurfaceComponent* Component)
{
	check(World);
	FFlexFuildSurfaceMap& FlexFluidSurfaceMap = WorldMap.FindOrAdd(World);

	check(Component && Component->FlexFluidSurface);
	FlexFluidSurfaceMap.Remove(Component->FlexFluidSurface);
	AFlexFluidSurfaceActor* Actor = (AFlexFluidSurfaceActor*)Component->GetOwner();
	Actor->Destroy();
}

void FFlexPluginBridge::TickFlexFluidSurfaceComponent(class UFlexFluidSurfaceComponent* SurfaceComponent, float DeltaTime, enum ELevelTick TickType, struct FActorComponentTickFunction *ThisTickFunction)
{
	SurfaceComponent->TickComponent(DeltaTime, TickType, NULL);
}

struct FFlexContainerInstance* FFlexPluginBridge::GetFlexSoftJointContainer(class FPhysScene* PhysScene, class UFlexContainer* Template)
{
	if (!GFlexIsInitialized)
	{
		return nullptr;
	}

	FPhysSceneContext& PhysSceneContext = PhysSceneMap.FindOrAdd(PhysScene);

	FFlexContainerInstance** Instance = PhysSceneContext.FlexContainerMap.Find(Template);
	if (Instance)
	{
		return *Instance;
	}
	else
	{
		return nullptr;
	}
}

void FFlexPluginBridge::WaitFlexScenes(class FPhysScene* PhysScene)
{
	if (!GFlexIsInitialized)
	{
		return;
	}

	FPhysSceneContext& PhysSceneContext = PhysSceneMap.FindOrAdd(PhysScene);

	if (PhysSceneContext.FlexContainerMap.Num())
	{
		if (PhysSceneContext.FlexSimulateTaskRef.IsValid())
			FTaskGraphInterface::Get().WaitUntilTaskCompletes(PhysSceneContext.FlexSimulateTaskRef);

		// if debug draw enabled on any containers then ensure any persistent lines are flushed
		bool NeedsFlushDebugLines = false;

		for (auto It = PhysSceneContext.FlexContainerMap.CreateIterator(); It; ++It)
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
			FlushPersistentDebugLines(PhysScene->GetOwningWorld());

		// synchronize flex components with results
		for (auto It = PhysSceneContext.FlexContainerMap.CreateIterator(); It; ++It)
			It->Value->Synchronize();
	}
}

void FFlexPluginBridge::TickFlexScenes(class FPhysScene* PhysScene, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent, float dt)
{
	if (GPhysXSDK && GFlexIsInitialized)
	{
		FPhysSceneContext& PhysSceneContext = PhysSceneMap.FindOrAdd(PhysScene);

		// when true the Flex CPU update will be run as a task async to the game thread
		// note that this is different from the async tick in LevelTick.cpp
		const bool bFlexAsync = true;

		if (bFlexAsync)
		{
			PhysSceneContext.FlexSimulateTaskRef = FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
				FSimpleDelegateGraphTask::FDelegate::CreateRaw(this, &FFlexPluginBridge::TickFlexScenesTask, PhysScene, dt),
				GET_STATID(STAT_TotalPhysicsTime));
		}
		else
		{
			TickFlexScenesTask(PhysScene, dt);
		}
	}
}

void FFlexPluginBridge::TickFlexScenesTask(class FPhysScene* PhysScene, float dt)
{
	FPhysSceneContext& PhysSceneContext = PhysSceneMap.FindOrAdd(PhysScene);

	// ensure we have the correct CUDA context set for Flex
	// this would be done automatically when making a Flex API call
	// but by acquiring explicitly in advance we save some unnecessary
	// CUDA calls to repeatedly set/unset the context
	NvFlexAcquireContext(GFlexLib);

	for (auto It = PhysSceneContext.FlexContainerMap.CreateIterator(); It; ++It)
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

	NvFlexRestoreContext(GFlexLib);
}

void FFlexPluginBridge::CleanupFlexScenes(class FPhysScene* PhysScene)
{
	if (!GFlexIsInitialized)
	{
		return;
	}
	FPhysSceneContext& PhysSceneContext = PhysSceneMap.FindOrAdd(PhysScene);

	if (PhysSceneContext.FlexContainerMap.Num())
	{
		for (auto It = PhysSceneContext.FlexContainerMap.CreateIterator(); It; ++It)
		{
			UFlexContainer* FlexContainerCopy = It->Value->Template;

			delete It->Value;
			It.RemoveCurrent();

			// Destroy the UFlexContainer copy that was created by GetFlexContainer()
			if (FlexContainerCopy != nullptr && FlexContainerCopy->IsValidLowLevel())
			{
				FlexContainerCopy->ConditionalBeginDestroy();
			}
		}
	}
}

struct FFlexContainerInstance* FFlexPluginBridge::GetFlexContainer(class FPhysScene* PhysScene, class UFlexContainer* Template)
{
	if (!GFlexIsInitialized)
	{
		return nullptr;
	}
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

void FFlexPluginBridge::StartFlexRecord(class FPhysScene* PhysScene)
{
#if 0
	FPhysSceneContext& PhysSceneContext = PhysSceneMap.FindOrAdd(PhysScene);

	for (auto It = PhysSceneContext.FlexContainerMap.CreateIterator(); It; ++It)
	{
		FFlexContainerInstance* Container = It->Value;
		FString Name = Container->Template->GetName();

		flexStartRecord(Container->Solver, StringCast<ANSICHAR>(*(FString("flexCapture_") + Name + FString(".flx"))).Get());
	}
#endif
}

void FFlexPluginBridge::StopFlexRecord(class FPhysScene* PhysScene)
{
#if 0
	FPhysSceneContext& PhysSceneContext = PhysSceneMap.FindOrAdd(PhysScene);

	for (auto It = PhysSceneContext.FlexContainerMap.CreateIterator(); It; ++It)
	{
		FFlexContainerInstance* Container = It->Value;

		flexStopRecord(Container->Solver);
	}
#endif
}

void FFlexPluginBridge::AddRadialForceToFlex(class FPhysScene* PhysScene, FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff)
{
	FPhysSceneContext& PhysSceneContext = PhysSceneMap.FindOrAdd(PhysScene);

	for (auto It = PhysSceneContext.FlexContainerMap.CreateIterator(); It; ++It)
	{
		FFlexContainerInstance* Container = It->Value;
		Container->AddRadialForce(Origin, Radius, Strength, Falloff);
	}
}

void FFlexPluginBridge::AddRadialImpulseToFlex(class FPhysScene* PhysScene, FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff, bool bVelChange)
{
	FPhysSceneContext& PhysSceneContext = PhysSceneMap.FindOrAdd(PhysScene);

	for (auto It = PhysSceneContext.FlexContainerMap.CreateIterator(); It; ++It)
	{
		FFlexContainerInstance* Container = It->Value;
		Container->AddRadialImpulse(Origin, Radius, Strength, Falloff, bVelChange);
	}

}

void FFlexPluginBridge::ToggleFlexContainerDebugDraw(class UWorld* InWorld)
{
	FFlexContainerInstance::sGlobalDebugDraw = !FFlexContainerInstance::sGlobalDebugDraw;

	// if disabling debug draw ensure any persistent lines are flushed
	if (!FFlexContainerInstance::sGlobalDebugDraw)
		FlushPersistentDebugLines(InWorld);
}

void FFlexPluginBridge::AttachFlexToComponent(class USceneComponent* Component, float Radius)
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
			UFlexParticleSystemComponent* ParticleSystemComponet = Cast<UFlexParticleSystemComponent>(ParticleComponents[ComponentIdx]);

			// is this a PSC with Flex?
			if (ParticleSystemComponet && ParticleSystemComponet->GetFirstFlexContainerTemplate())
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
								auto FlexEmitter = Cast<UFlexParticleEmitter>(EmitterInstance->SpriteTemplate);
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

void FFlexPluginBridge::CreateFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance)
{
	if (EmitterInstance->FlexEmitterInstance)
	{
		delete EmitterInstance->FlexEmitterInstance;
		EmitterInstance->FlexEmitterInstance = nullptr;
	}

	auto FlexEmitter = Cast<UFlexParticleEmitter>(EmitterInstance->SpriteTemplate);
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

void FFlexPluginBridge::DestroyFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance)
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
					verify(EmitterInstance->FlexDataOffset > 0);
					int32 CurrentOffset = EmitterInstance->FlexDataOffset;
					const uint8* ParticleBase = (const uint8*)&Particle;
					PARTICLE_ELEMENT(int32, FlexParticleIndex);
					Container->DestroyParticle(FlexParticleIndex);
				}
			}
		}
		delete EmitterInstance->FlexEmitterInstance;
	}

	if (EmitterInstance->FlexFluidSurfaceComponent)
	{
		EmitterInstance->FlexFluidSurfaceComponent->UnregisterEmitterInstance(EmitterInstance);
	}
}

void FFlexPluginBridge::TickFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance, float DeltaTime, bool bSuppressSpawning)
{
	auto FlexEmitter = Cast<UFlexParticleEmitter>(EmitterInstance->SpriteTemplate);
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

		EmitterInstance->bFlexAnisotropyData = (Container->Template->AnisotropyScale > 0.0f);
		verify(!EmitterInstance->bFlexAnisotropyData || Container->Anisotropy1.size() > 0);

		// process report shapes
		if (Container->CollisionReportComponents.Num() > 0)
		{
			for (int32 i = 0; i < EmitterInstance->ActiveParticles; i++)
			{
				DECLARE_PARTICLE(Particle, EmitterInstance->ParticleData + EmitterInstance->ParticleStride * EmitterInstance->ParticleIndices[i]);

				verify(EmitterInstance->FlexDataOffset > 0);

				int32 CurrentOffset = EmitterInstance->FlexDataOffset;
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

			verify(EmitterInstance->FlexDataOffset > 0);

			int32 CurrentOffset = EmitterInstance->FlexDataOffset;
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

			if (EmitterInstance->bFlexAnisotropyData)
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

uint32 FFlexPluginBridge::GetFlexEmitterInstanceRequiredBytes(struct FParticleEmitterInstance* EmitterInstance, uint32 uiBytes)
{
	auto FlexEmitter = Cast<UFlexParticleEmitter>(EmitterInstance->SpriteTemplate);

	if (FlexEmitter && FlexEmitter->FlexContainerTemplate)
	{
		EmitterInstance->FlexDataOffset = EmitterInstance->PayloadOffset + uiBytes;

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

bool FFlexPluginBridge::FlexEmitterInstanceSpawnParticle(struct FParticleEmitterInstance* EmitterInstance, struct FBaseParticle* Particle, uint32 CurrentParticleIndex)
{
	auto FlexEmitter = Cast<UFlexParticleEmitter>(EmitterInstance->SpriteTemplate);
	if (FlexEmitter == nullptr)
		return true;

	const float FlexInvMass = (FlexEmitter->Mass > 0.0f) ? (1.0f / FlexEmitter->Mass) : 0.0f;

	if (EmitterInstance->FlexEmitterInstance && EmitterInstance->FlexEmitterInstance->Container && (!GIsEditor || GIsPlayInEditorWorld))
	{
		verify(EmitterInstance->FlexDataOffset > 0);

		int32 CurrentOffset = EmitterInstance->FlexDataOffset;
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

void FFlexPluginBridge::FlexEmitterInstanceKillParticle(struct FParticleEmitterInstance* EmitterInstance, int32 KillIndex)
{
	if (EmitterInstance->FlexEmitterInstance && EmitterInstance->FlexEmitterInstance->Container && (!GIsEditor || GIsPlayInEditorWorld))
	{
		verify(EmitterInstance->FlexDataOffset > 0);

		const uint8* ParticleBase = EmitterInstance->ParticleData + KillIndex * EmitterInstance->ParticleStride;
		int32 CurrentOffset = EmitterInstance->FlexDataOffset;
		PARTICLE_ELEMENT(int32, FlexParticleIndex);

		if (FlexParticleIndex >= 0)
		{
			EmitterInstance->FlexEmitterInstance->DestroyParticle(FlexParticleIndex);
		}
	}
}

void FFlexPluginBridge::UpdateFlexSurfaceDynamicData(class UParticleSystemComponent* Component, struct FParticleEmitterInstance* EmitterInstance, struct FDynamicEmitterDataBase* EmitterDynamicData)
{
	auto FlexComponent = Cast<UFlexParticleSystemComponent>(Component);
	if (FlexComponent)
	{
		FlexComponent->UpdateFlexSurfaceDynamicData(EmitterInstance, EmitterDynamicData);
	}
}

void FFlexPluginBridge::ClearFlexSurfaceDynamicData(class UParticleSystemComponent* Component)
{
	auto FlexComponent = Cast<UFlexParticleSystemComponent>(Component);
	if (FlexComponent)
	{
		FlexComponent->ClearFlexSurfaceDynamicData();
	}
}

void FFlexPluginBridge::SetEnabledReferenceCounting(class UParticleSystemComponent* Component, bool bEnable)
{
	auto FlexComponent = Cast<UFlexParticleSystemComponent>(Component);
	if (FlexComponent)
	{
		if (FlexComponent->FlexFluidSurfaceOverride)
		{
			UFlexFluidSurfaceComponent* SurfaceComponent = FFlexPluginBridge::GetFlexFluidSurface(FlexComponent->GetWorld(), FlexComponent->FlexFluidSurfaceOverride);

			check(SurfaceComponent);
			// This is necessary because we need to hold the reference to the fluid surface so it doesn't go away with a SetTemplate() call
			SurfaceComponent->SetEnabledReferenceCounting(bEnable);
		}
	}
}

void FFlexPluginBridge::RegisterNewFlexFluidSurfaceComponent(struct FParticleEmitterInstance* EmitterInstance, class UFlexFluidSurface* NewFlexFluidSurface)
{
	if (EmitterInstance->FlexFluidSurfaceComponent)
	{
		EmitterInstance->FlexFluidSurfaceComponent->UnregisterEmitterInstance(EmitterInstance);
		EmitterInstance->FlexFluidSurfaceComponent = nullptr;
	}

	if (NewFlexFluidSurface)
	{
		EmitterInstance->FlexFluidSurfaceComponent = AddFlexFluidSurface(EmitterInstance->GetWorld(), NewFlexFluidSurface);
		EmitterInstance->FlexFluidSurfaceComponent->RegisterEmitterInstance(EmitterInstance);
	}
}

void FFlexPluginBridge::RegisterNewFlexFluidSurfaceComponent(class UParticleSystemComponent* Component, struct FParticleEmitterInstance* EmitterInstance)
{
	auto FlexComponent = Cast<UFlexParticleSystemComponent>(Component);
	if (FlexComponent && FlexComponent->FlexFluidSurfaceOverride)
	{
		if (EmitterInstance && EmitterInstance->SpriteTemplate)
		{
			auto FlexEmitter = Cast<UFlexParticleEmitter>(EmitterInstance->SpriteTemplate);
			auto FlexFluidSurfaceTemplate = FlexEmitter ? FlexEmitter->FlexFluidSurfaceTemplate : nullptr;

			if (FlexFluidSurfaceTemplate && FlexFluidSurfaceTemplate->Material)
			{
				FFlexPluginBridge::RegisterNewFlexFluidSurfaceComponent(EmitterInstance, FlexComponent->FlexFluidSurfaceOverride);
			}
		}
	}
}

bool FFlexPluginBridge::IsValidFlexEmitter(class UParticleEmitter* Emitter)
{
	auto FlexEmitter = Cast<UFlexParticleEmitter>(Emitter);
	return (FlexEmitter && FlexEmitter->FlexContainerTemplate);
}

class UFlexFluidSurface* FFlexPluginBridge::GetFlexFluidSurfaceTemplate(class UParticleEmitter* Emitter)
{
	auto FlexEmitter = Cast<UFlexParticleEmitter>(Emitter);
	return FlexEmitter ? FlexEmitter->FlexFluidSurfaceTemplate : nullptr;
}
