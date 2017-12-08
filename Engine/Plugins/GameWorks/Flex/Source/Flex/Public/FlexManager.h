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

	/** Retrive the container instance for a soft joint, will return a nullptr if it doesn't already exist */
	struct FFlexContainerInstance* FindFlexContainerInstance(class FPhysScene* PhysScene, class UFlexContainer* Template);

	/** Retrive the container instance for a template, will create the instance if it doesn't already exist */
	struct FFlexContainerInstance* FindOrCreateFlexContainerInstance(class FPhysScene* PhysScene, class UFlexContainer* Template);

	virtual void WaitFlexScenes(class FPhysScene* PhysScene);
	virtual void TickFlexScenes(class FPhysScene* PhysScene, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent, float dt);
	virtual void CleanupFlexScenes(class FPhysScene* PhysScene);

	virtual void AddRadialForceToFlex(class FPhysScene* PhysScene, FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff);
	virtual void AddRadialImpulseToFlex(class FPhysScene* PhysScene, FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff, bool bVelChange);

	void StartFlexRecord(class FPhysScene* PhysScene);
	void StopFlexRecord(class FPhysScene* PhysScene);
	void ToggleFlexContainerDebugDraw(class UWorld* World);

	virtual void AttachFlexToComponent(class USceneComponent* Component, float Radius);


	virtual void CreateFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance);
	virtual void DestroyFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance);
	virtual void TickFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance, float DeltaTime, bool bSuppressSpawning);
	virtual uint32 GetFlexEmitterInstanceRequiredBytes(struct FParticleEmitterInstance* EmitterInstance, uint32 uiBytes);
	virtual bool FlexEmitterInstanceSpawnParticle(struct FParticleEmitterInstance* EmitterInstance, struct FBaseParticle* Particle, uint32 CurrentParticleIndex);
	virtual void FlexEmitterInstanceKillParticle(struct FParticleEmitterInstance* EmitterInstance, int32 KillIndex);
	virtual bool FlexEmitterInstanceShouldRenderParticles(struct FParticleEmitterInstance* EmitterInstance);
	virtual bool FlexEmitterInstanceShouldForceLocalSpace(struct FParticleEmitterInstance* EmitterInstance);

	virtual class UObject* GetFirstFlexContainerTemplate(class UParticleSystemComponent* Component);

	virtual bool IsValidFlexEmitter(class UParticleEmitter* Emitter);

	// GPUSpriteEmitterInstance
	virtual FRenderResource* GPUSpriteEmitterInstance_Init(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 ParticlesPerTile) override;
	virtual void GPUSpriteEmitterInstance_Tick(struct FFlexParticleEmitterInstance* FlexEmitterInstance, float DeltaSeconds, bool bSuppressSpawning, FRenderResource* FlexSimulationResource) override;
	virtual bool GPUSpriteEmitterInstance_ShouldRenderParticles(struct FFlexParticleEmitterInstance* FlexEmitterInstance) override;
	virtual void GPUSpriteEmitterInstance_DestroyTileParticles(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 TileIndex) override;
	virtual void GPUSpriteEmitterInstance_DestroyAllParticles(struct FFlexParticleEmitterInstance* FlexEmitterInstance, bool bFreeParticleIndices) override;

	virtual void GPUSpriteEmitterInstance_AllocParticleIndices(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 TileCount) override;
	virtual void GPUSpriteEmitterInstance_FreeParticleIndices(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 TileStart, int32 TileCount) override;

	virtual int32 GPUSpriteEmitterInstance_CreateNewParticles(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 NewStart, int32 NewCount) override;
	virtual void GPUSpriteEmitterInstance_DestroyNewParticles(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 NewStart, int32 NewCount) override;
	virtual void GPUSpriteEmitterInstance_AddNewParticle(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 NewIndex, int32 TileIndex, int32 SubTileIndex) override;
	virtual void GPUSpriteEmitterInstance_SetNewParticle(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 NewIndex, const FVector& Position, const FVector& Velocity, float RelativeTime, float TimeScale, float InitialSize) override;

	virtual void GPUSpriteEmitterInstance_FillSimulationParams(FRenderResource* FlexSimulationResource, FFlexGPUParticleSimulationParameters& SimulationParams) override;

private:
	struct FPhysSceneContext;

	void TickFlexScenesTask(FFlexManager::FPhysSceneContext* PhysSceneContext, float dt, bool bIsLocked);

	static class UFlexAsset* GetFlexAsset(class UStaticMesh* StaticMesh);

private:
	bool bFlexInitialized;
	NvFlexLibrary* FlexLib;

	struct FPhysSceneContext
	{
		/** Map from Flex container template to instances belonging to this physscene */
		TMap<class UFlexContainer*, struct FFlexContainerInstance*> FlexContainerMap;
		FGraphEventRef FlexSimulateTaskRef;
	};
	TMap<class FPhysScene*, FPhysSceneContext> PhysSceneMap;

	mutable FRWLock PhysSceneMapLock;

};
