#pragma once

class IFlexPluginBridge
{
public:
	virtual void ReImportAsset(class UFlexAsset* FlexAsset, class UStaticMesh* StaticMesh) = 0;

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
};

extern ENGINE_API class IFlexPluginBridge* GFlexPluginBridge;
