#include "FlexPluginBridge.h"

#include "FlexAsset.h"

#include "FlexFluidSurfaceActor.h"
#include "FlexFluidSurfaceComponent.h"

#include "FlexComponent.h"
#include "FlexContainer.h"
#include "FlexContainerInstance.h"
#include "DrawDebugHelpers.h" // FlushPersistentDebugLines

#include "Engine/StaticMesh.h"
#include "PhysicsEngine/PhysXSupport.h"

#include "EngineUtils.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"



void FFlexPluginBridge::ReImportAsset(class UFlexAsset* FlexAsset, class UStaticMesh* StaticMesh)
{
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
			UParticleSystemComponent* ParticleSystemComponet = Cast<UParticleSystemComponent>(ParticleComponents[ComponentIdx]);

			// is this a PSC with Flex?
			if (ParticleSystemComponet && ParticleSystemComponet->GetFirstFlexContainerTemplate())
			{
				const FBoxSphereBounds FlexBounds = ParticleSystemComponet->CalcBounds(FTransform::Identity);

				// dist of force field to flex bounds
				const float DistSq = FlexBounds.ComputeSquaredDistanceFromBoxToPoint(Origin);

				if (DistSq < Radius*Radius)
				{
					ParticleSystemComponet->AttachFlexToComponent(Component, Radius);
				}
			}
		}
	}
}
