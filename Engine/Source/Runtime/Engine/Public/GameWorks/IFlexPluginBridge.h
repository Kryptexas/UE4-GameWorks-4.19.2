#pragma once

#include "Async/TaskGraphInterfaces.h"

struct FFlexGPUParticleSimulationParameters;

class IFlexPluginBridge
{
public:
	// PhysLevel
	virtual void InitGamePhysPostRHI() = 0;
	virtual void TermGamePhys() = 0;

	// StaticMesh
	virtual bool HasFlexAsset(class UStaticMesh* StaticMesh) = 0;
	virtual void ReImportFlexAsset(class UStaticMesh* StaticMesh) = 0;

	// PhysScene
	virtual void WaitFlexScenes(class FPhysScene* PhysScene) = 0;
	virtual void TickFlexScenes(class FPhysScene* PhysScene, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent, float dt) = 0;
	virtual void CleanupFlexScenes(class FPhysScene* PhysScene) = 0;

	/** Adds a radial force to all flex container instances */
	virtual void AddRadialForceToFlex(class FPhysScene* PhysScene, FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff) = 0;

	/** Adds a radial force to all flex container instances */
	virtual void AddRadialImpulseToFlex(class FPhysScene* PhysScene, FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff, bool bVelChange) = 0;

	// RadialForceComponent
	virtual void AttachFlexToComponent(class USceneComponent* Component, float Radius) = 0;

	// ParticleEmitterInstance
	virtual void CreateFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance) = 0;
	virtual void DestroyFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance) = 0;
	virtual void TickFlexEmitterInstance(struct FParticleEmitterInstance* EmitterInstance, float DeltaTime, bool bSuppressSpawning) = 0;
	virtual uint32 GetFlexEmitterInstanceRequiredBytes(struct FParticleEmitterInstance* EmitterInstance, uint32 uiBytes) = 0;
	virtual bool FlexEmitterInstanceSpawnParticle(struct FParticleEmitterInstance* EmitterInstance, struct FBaseParticle* Particle, uint32 CurrentParticleIndex) = 0;
	virtual void FlexEmitterInstanceKillParticle(struct FParticleEmitterInstance* EmitterInstance, int32 KillIndex) = 0;
	virtual bool IsFlexEmitterInstanceDynamicDataRequired(struct FParticleEmitterInstance* EmitterInstance) = 0;

	// ParticleSystemComponent
	virtual class UObject* GetFirstFlexContainerTemplate(class UParticleSystemComponent* Component) = 0; // returns UFlexContainer*

	virtual bool IsValidFlexEmitter(class UParticleEmitter* Emitter) = 0;

	// GPUSpriteEmitterInstance
	virtual FRenderResource* GPUSpriteEmitterInstance_Init(struct FFlexParticleEmitterInstance* FlexEmitterInstance) = 0;
	virtual void GPUSpriteEmitterInstance_Tick(struct FFlexParticleEmitterInstance* FlexEmitterInstance, float DeltaSeconds, bool bSuppressSpawning, FRenderResource* FlexSimulationResource) = 0;
	virtual void GPUSpriteEmitterInstance_DestroyParticles(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 Start, int32 Count) = 0;
	virtual void GPUSpriteEmitterInstance_DestroyAllParticles(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 ParticlesPerTile, bool bFreeParticleIndices) = 0;

	virtual void GPUSpriteEmitterInstance_AllocParticleIndices(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 Count) = 0;
	virtual void GPUSpriteEmitterInstance_FreeParticleIndices(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 Start, int32 Count) = 0;

	virtual int32 GPUSpriteEmitterInstance_CreateNewParticles(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 NewStart, int32 NewCount) = 0;
	virtual void GPUSpriteEmitterInstance_DestroyNewParticles(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 NewStart, int32 NewCount) = 0;
	virtual void GPUSpriteEmitterInstance_InitNewParticle(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 NewIndex, int32 RegularIndex) = 0;
	virtual void GPUSpriteEmitterInstance_SetNewParticle(struct FFlexParticleEmitterInstance* FlexEmitterInstance, int32 NewIndex, const FVector& Position, const FVector& Velocity, float RelativeTime, float TimeScale, float InitialSize) = 0;

	virtual void GPUSpriteEmitterInstance_FillSimulationParams(FRenderResource* FlexSimulationResource, FFlexGPUParticleSimulationParameters& SimulationParams) = 0;
};

extern ENGINE_API class IFlexPluginBridge* GFlexPluginBridge;
