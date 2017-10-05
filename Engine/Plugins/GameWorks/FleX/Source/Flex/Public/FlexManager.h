#pragma once

#include "GameWorks/IFlexPluginBridge.h"

#include "NvFlex.h"
#include "NvFlexExt.h"
#include "NvFlexDevice.h"


class FFlexManager : public IFlexPluginBridge
{
public:
	static FFlexManager& get()
	{
		static FFlexManager instance;
		return instance;
	}

	NvFlexLibrary* GetFlexLib() { return FlexLib; }


	FFlexManager();
	virtual ~FFlexManager() {}

	// IFlexPluginBridge impl.
	virtual void InitGamePhysPostRHI();
	virtual void TermGamePhys();

	virtual bool HasFlexAsset(class UStaticMesh* StaticMesh);

	virtual void ReImportFlexAsset(class UStaticMesh* StaticMesh);

	virtual class UFlexFluidSurfaceComponent* GetFlexFluidSurface(class UWorld* World, class UFlexFluidSurface* FlexFluidSurfaceTemplate);

	virtual class UFlexFluidSurfaceComponent* AddFlexFluidSurface(class UWorld* World, class UFlexFluidSurface* FlexFluidSurfaceTemplate);

	virtual void RemoveFlexFluidSurface(class UWorld* World, class UFlexFluidSurfaceComponent* FlexFluidSurfaceComponent);

	virtual void TickFlexFluidSurfaceComponent(class UFlexFluidSurfaceComponent* SurfaceComponent, float DeltaTime, enum ELevelTick TickType, struct FActorComponentTickFunction *ThisTickFunction);

	virtual struct FFlexContainerInstance* GetFlexSoftJointContainer(class FPhysScene* PhysScene, class UFlexContainer* Template);

	virtual void WaitFlexScenes(class FPhysScene* PhysScene);
	virtual void TickFlexScenes(class FPhysScene* PhysScene, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent, float dt);

	virtual void CleanupFlexScenes(class FPhysScene* PhysScene);

	virtual struct FFlexContainerInstance* GetFlexContainer(class FPhysScene* PhysScene, class UFlexContainer* Template);

	virtual void StartFlexRecord(class FPhysScene* PhysScene);
	virtual void StopFlexRecord(class FPhysScene* PhysScene);

	virtual void AddRadialForceToFlex(class FPhysScene* PhysScene, FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff);
	virtual void AddRadialImpulseToFlex(class FPhysScene* PhysScene, FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff, bool bVelChange);

	virtual void ToggleFlexContainerDebugDraw(class UWorld* World);

	virtual void AttachFlexToComponent(class USceneComponent* Component, float Radius);


	virtual void CreateFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance);
	virtual void DestroyFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance);
	virtual void TickFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance, float DeltaTime, bool bSuppressSpawning);
	virtual uint32 GetFlexEmitterInstanceRequiredBytes(struct FParticleEmitterInstance* EmitterInstance, uint32 uiBytes);
	virtual bool FlexEmitterInstanceSpawnParticle(struct FParticleEmitterInstance* EmitterInstance, struct FBaseParticle* Particle, uint32 CurrentParticleIndex);
	virtual void FlexEmitterInstanceKillParticle(struct FParticleEmitterInstance* EmitterInstance, int32 KillIndex);

	virtual class UMaterialInstanceDynamic* CreateFlexDynamicMaterialInstance(class UParticleSystemComponent* Component, class UMaterialInterface* SourceMaterial);
	virtual class UObject* GetFirstFlexContainerTemplate(class UParticleSystemComponent* Component);
	virtual void UpdateFlexSurfaceDynamicData(class UParticleSystemComponent* Component, struct FParticleEmitterInstance* EmitterInstance, struct FDynamicEmitterDataBase* EmitterDynamicData);
	virtual void ClearFlexSurfaceDynamicData(class UParticleSystemComponent* Component);
	virtual void SetEnabledReferenceCounting(class UParticleSystemComponent* Component, bool bEnable);

	virtual void RegisterNewFlexFluidSurfaceComponent(struct FParticleEmitterInstance* EmitterInstance, class UFlexFluidSurface* NewFlexFluidSurface);
	virtual void RegisterNewFlexFluidSurfaceComponent(class UParticleSystemComponent* Component, struct FParticleEmitterInstance* EmitterInstance);

	virtual bool IsValidFlexEmitter(class UParticleEmitter* Emitter);
	virtual class UFlexFluidSurface* GetFlexFluidSurfaceTemplate(class UParticleEmitter* Emitter);


private:
	void TickFlexScenesTask(class FPhysScene* PhysScene, float dt);

	static class UFlexAsset* GetFlexAsset(class UStaticMesh* StaticMesh);

private:

	bool bFlexInitialized;
	NvFlexLibrary* FlexLib;

	/** Map from Flex fluid surface template to fluid surface components*/
	typedef TMap<class UFlexFluidSurface*, class UFlexFluidSurfaceComponent*> FFlexFuildSurfaceMap;
	TMap<TWeakObjectPtr<UWorld>, FFlexFuildSurfaceMap> WorldMap;

	struct FPhysSceneContext
	{
		/** Map from Flex container template to instances belonging to this physscene */
		TMap<class UFlexContainer*, struct FFlexContainerInstance*> FlexContainerMap;
		FGraphEventRef FlexSimulateTaskRef;
	};
	TMap<class FPhysScene*, FPhysSceneContext> PhysSceneMap;
};
