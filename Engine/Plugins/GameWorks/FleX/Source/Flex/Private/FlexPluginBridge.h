#pragma once

#include "GameWorks/IFlexPluginBridge.h"

class FFlexPluginBridge : public IFlexPluginBridge
{
public:
	virtual void ReImportAsset(class UFlexAsset* FlexAsset, class UStaticMesh* StaticMesh);

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

	virtual ~FFlexPluginBridge() {}

private:
	void TickFlexScenesTask(class FPhysScene* PhysScene, float dt);

private:
	/** Map from Flex fluid surface template to fluid surface components*/
	typedef TMap<class UFlexFluidSurface*, class UFlexFluidSurfaceComponent*> FFlexFuildSurfaceMap;
	TMap<class UWorld*, FFlexFuildSurfaceMap> WorldMap;

	struct FPhysSceneContext
	{
		/** Map from Flex container template to instances belonging to this physscene */
		TMap<class UFlexContainer*, struct FFlexContainerInstance*> FlexContainerMap;
		FGraphEventRef FlexSimulateTaskRef;
	};
	TMap<class FPhysScene*, FPhysSceneContext> PhysSceneMap;
};
