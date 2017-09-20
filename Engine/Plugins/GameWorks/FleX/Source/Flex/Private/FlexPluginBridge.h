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

	virtual ~FFlexPluginBridge() {}

private:
	/** Map from Flex fluid surface template to fluid surface components*/
	typedef TMap<class UFlexFluidSurface*, class UFlexFluidSurfaceComponent*> FFlexFuildSurfaceMap;
	TMap<class UWorld*, FFlexFuildSurfaceMap> WorldMap;

};
