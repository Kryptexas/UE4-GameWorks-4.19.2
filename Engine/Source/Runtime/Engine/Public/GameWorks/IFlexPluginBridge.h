#pragma once

#include "Async/TaskGraphInterfaces.h"

class IFlexPluginBridge
{
public:
	virtual void InitGamePhysPostRHI() = 0;
	virtual void TermGamePhys() = 0;

	virtual bool HasFlexAsset(class UStaticMesh* StaticMesh) = 0;

	virtual void ReImportFlexAsset(class UStaticMesh* StaticMesh) = 0;

	/**
	* Retrieve the UFlexFluidSurfaceComponent corresponding to the UFlexFluidSurface template.
	*/
	virtual class UFlexFluidSurfaceComponent* GetFlexFluidSurface(class UWorld* World, class UFlexFluidSurface* FlexFluidSurfaceTemplate) = 0;

	/**
	* Create a new UFlexFluidSurfaceComponent, corresponding 1:1 with a UFlexFluidSurface template.
	*/
	virtual class UFlexFluidSurfaceComponent* AddFlexFluidSurface(class UWorld* World, class UFlexFluidSurface* FlexFluidSurfaceTemplate) = 0;

	/**
	* Remove a FlexFluidSurfaceComponent and it's corresponding UFlexFluidSurface.
	*/
	virtual void RemoveFlexFluidSurface(class UWorld* World, class UFlexFluidSurfaceComponent* FlexFluidSurfaceComponent) = 0;

	virtual void TickFlexFluidSurfaceComponent(class UFlexFluidSurfaceComponent* SurfaceComponent, float DeltaTime, enum ELevelTick TickType, struct FActorComponentTickFunction *ThisTickFunction) = 0;

	// PhysScene functions:
	/** Retrive the container instance for a soft joint, will return a nullptr if it doesn't already exist */
	virtual struct FFlexContainerInstance* GetFlexSoftJointContainer(class FPhysScene* PhysScene, class UFlexContainer* Template) = 0;

	virtual void WaitFlexScenes(class FPhysScene* PhysScene) = 0;
	virtual void TickFlexScenes(class FPhysScene* PhysScene, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent, float dt) = 0;

	virtual void CleanupFlexScenes(class FPhysScene* PhysScene) = 0;

	/** Retrive the container instance for a template, will create the instance if it doesn't already exist */
	virtual struct FFlexContainerInstance* GetFlexContainer(class FPhysScene* PhysScene, class UFlexContainer* Template) = 0;

	virtual void StartFlexRecord(class FPhysScene* PhysScene) = 0;
	virtual void StopFlexRecord(class FPhysScene* PhysScene) = 0;

	/** Adds a radial force to all flex container instances */
	virtual void AddRadialForceToFlex(class FPhysScene* PhysScene, FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff) = 0;

	/** Adds a radial force to all flex container instances */
	virtual void AddRadialImpulseToFlex(class FPhysScene* PhysScene, FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff, bool bVelChange) = 0;


	virtual void AttachFlexToComponent(class USceneComponent* Component, float Radius) = 0;

	virtual void ToggleFlexContainerDebugDraw(class UWorld* World) = 0;

	// Particles
	virtual void CreateFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance) = 0;
	virtual void DestroyFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance) = 0;
	virtual void TickFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance, float DeltaTime, bool bSuppressSpawning) = 0;
	virtual uint32 GetFlexEmitterInstanceRequiredBytes(struct FParticleEmitterInstance* EmitterInstance, uint32 uiBytes) = 0;
	virtual bool FlexEmitterInstanceSpawnParticle(struct FParticleEmitterInstance* EmitterInstance, struct FBaseParticle* Particle, uint32 CurrentParticleIndex) = 0;
	virtual void FlexEmitterInstanceKillParticle(struct FParticleEmitterInstance* EmitterInstance, int32 KillIndex) = 0;

	// UParticleSystemComponent
	virtual class UMaterialInstanceDynamic* CreateFlexDynamicMaterialInstance(class UParticleSystemComponent* Component, class UMaterialInterface* SourceMaterial) = 0;
	virtual class UObject* GetFirstFlexContainerTemplate(class UParticleSystemComponent* Component) = 0; // returns UFlexContainer*
	virtual void UpdateFlexSurfaceDynamicData(class UParticleSystemComponent* Component, struct FParticleEmitterInstance* EmitterInstance, struct FDynamicEmitterDataBase* EmitterDynamicData) = 0;
	virtual void ClearFlexSurfaceDynamicData(class UParticleSystemComponent* Component) = 0;
	virtual void SetEnabledReferenceCounting(class UParticleSystemComponent* Component, bool bEnable) = 0;

	virtual void RegisterNewFlexFluidSurfaceComponent(struct FParticleEmitterInstance* EmitterInstance, class UFlexFluidSurface* NewFlexFluidSurface) = 0;
	virtual void RegisterNewFlexFluidSurfaceComponent(class UParticleSystemComponent* Component, struct FParticleEmitterInstance* EmitterInstance) = 0;


	virtual bool IsValidFlexEmitter(class UParticleEmitter* Emitter) = 0;
	virtual class UFlexFluidSurface* GetFlexFluidSurfaceTemplate(class UParticleEmitter* Emitter) = 0;
};

extern ENGINE_API class IFlexPluginBridge* GFlexPluginBridge;
