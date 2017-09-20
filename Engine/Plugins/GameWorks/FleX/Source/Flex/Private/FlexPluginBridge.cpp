#include "FlexPluginBridge.h"

#include "FlexAsset.h"

#include "FlexFluidSurfaceActor.h"
#include "FlexFluidSurfaceComponent.h"

#include "Engine/StaticMesh.h"


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
	FFlexFuildSurfaceMap& FlexFluidSurfaceMap = WorldMap[World];

	check(FlexFluidSurface);
	UFlexFluidSurfaceComponent** Component = FlexFluidSurfaceMap.Find(FlexFluidSurface);
	return (Component != NULL) ? *Component : NULL;
}

class UFlexFluidSurfaceComponent* FFlexPluginBridge::AddFlexFluidSurface(class UWorld* World, class UFlexFluidSurface* FlexFluidSurface)
{
	check(World);
	FFlexFuildSurfaceMap& FlexFluidSurfaceMap = WorldMap[World];

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
	FFlexFuildSurfaceMap& FlexFluidSurfaceMap = WorldMap[World];

	check(Component && Component->FlexFluidSurface);
	FlexFluidSurfaceMap.Remove(Component->FlexFluidSurface);
	AFlexFluidSurfaceActor* Actor = (AFlexFluidSurfaceActor*)Component->GetOwner();
	Actor->Destroy();
}

void FFlexPluginBridge::TickFlexFluidSurfaceComponent(class UFlexFluidSurfaceComponent* SurfaceComponent, float DeltaTime, enum ELevelTick TickType, struct FActorComponentTickFunction *ThisTickFunction)
{
	SurfaceComponent->TickComponent(DeltaTime, TickType, NULL);
}
