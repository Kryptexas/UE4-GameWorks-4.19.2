#pragma once

#include "GameWorks/IFlexEditorPluginBridge.h"


class FFlexEditorPluginBridge : public IFlexEditorPluginBridge
{
public:
	virtual class UPrimitiveComponent* UpdateFlexPreviewComponent(bool bDrawFlexPreview, class FAdvancedPreviewScene* PreviewScene, class UStaticMesh* StaticMesh, class UPrimitiveComponent* FlexPreviewComponent);

	virtual bool IsObjectFlexAssetOrContainer(class UObject* ObjectBeingModified, class UStaticMesh* StaticMesh);
	virtual bool IsChildOfFlexAsset(class UClass* Class);

	virtual void GetFlexAssetStats(class UFlexAsset* FlexAsset, FlexAssetStats& FlexAssetStats);

	virtual bool KeepFlexSimulationChanges(class AActor* EditorWorldActor, class AActor* SimWorldActor);
	virtual bool ClearFlexSimulationChanges(class AActor* EditorWorldActor);

	virtual class AActor* SpawnFlexActor(class UWorld* World, struct FTransform const* UserTransformPtr, const struct FActorSpawnParameters& SpawnParameters);

	virtual bool IsFlexActor(class UClass* Class);
	virtual void SetFlexActorCollisionProfileName(class AStaticMeshActor* SMActor, FName CollisionProfileName);

private:
};
