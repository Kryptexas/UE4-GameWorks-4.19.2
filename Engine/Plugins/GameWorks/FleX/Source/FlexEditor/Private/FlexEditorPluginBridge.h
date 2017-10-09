#pragma once

#include "GameWorks/IFlexEditorPluginBridge.h"


class FFlexEditorPluginBridge : public IFlexEditorPluginBridge
{
public:
	virtual class UFlexAsset* GetFlexAsset(class UStaticMesh* StaticMesh);
	virtual void SetFlexAsset(class UStaticMesh* StaticMesh, class UFlexAsset* FlexAsset);

	virtual class UPrimitiveComponent* UpdateFlexPreviewComponent(bool bDrawFlexPreview, class FAdvancedPreviewScene* PreviewScene, class UStaticMesh* StaticMesh, class UPrimitiveComponent* FlexPreviewComponent);

	virtual bool IsObjectFlexAssetOrContainer(class UObject* ObjectBeingModified, class UStaticMesh* StaticMesh);
	virtual bool IsChildOfFlexAsset(class UClass* Class);

	virtual bool GetFlexAssetStats(class UStaticMesh* StaticMesh, FlexAssetStats& FlexAssetStats);

	virtual bool KeepFlexSimulationChanges(class AActor* EditorWorldActor, class AActor* SimWorldActor);
	virtual bool ClearFlexSimulationChanges(class AActor* EditorWorldActor);

	virtual class AActor* SpawnFlexActor(class UWorld* World, struct FTransform const* UserTransformPtr, const struct FActorSpawnParameters& SpawnParameters);

	virtual bool IsFlexActor(class UClass* Class);
	virtual void SetFlexActorCollisionProfileName(class AStaticMeshActor* SMActor, FName CollisionProfileName);

	virtual class UClass* GetFlexParticleSpriteEmitterClass();

	virtual bool IsFlexStaticMesh(class UStaticMesh* StaticMesh);

private:
};
