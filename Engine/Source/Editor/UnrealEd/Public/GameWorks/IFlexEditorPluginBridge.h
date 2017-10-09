#pragma once

class IFlexEditorPluginBridge
{
public:
	virtual bool IsFlexStaticMesh(class UStaticMesh* StaticMesh) = 0;

	virtual class UFlexAsset* GetFlexAsset(class UStaticMesh* StaticMesh) = 0;
	virtual void SetFlexAsset(class UStaticMesh* StaticMesh, class UFlexAsset* FlexAsset) = 0;

	virtual class UPrimitiveComponent* UpdateFlexPreviewComponent(bool bDrawFlexPreview, class FAdvancedPreviewScene* PreviewScene, class UStaticMesh* StaticMesh, class UPrimitiveComponent* FlexPreviewComponent) = 0;

	virtual bool IsObjectFlexAssetOrContainer(class UObject* ObjectBeingModified, class UStaticMesh* StaticMesh) = 0;
	virtual bool IsChildOfFlexAsset(class UClass* Class) = 0;

	struct FlexAssetStats
	{
		int32 NumParticles = 0;
		int32 NumShapes = 0;
		int32 NumSprings = 0;
	};
	virtual bool GetFlexAssetStats(class UStaticMesh* StaticMesh, FlexAssetStats& FlexAssetStats) = 0;

	virtual bool KeepFlexSimulationChanges(class AActor* EditorWorldActor, class AActor* SimWorldActor) = 0;
	virtual bool ClearFlexSimulationChanges(class AActor* EditorWorldActor) = 0;

	virtual class AActor* SpawnFlexActor(class UWorld* World, struct FTransform const* UserTransformPtr, const struct FActorSpawnParameters& SpawnParameters) = 0;

	virtual bool IsFlexActor(class UClass* Class) = 0;
	virtual void SetFlexActorCollisionProfileName(class AStaticMeshActor* SMActor, FName CollisionProfileName) = 0;

	virtual class UClass* GetFlexParticleSpriteEmitterClass() = 0;
};

extern UNREALED_API class IFlexEditorPluginBridge* GFlexEditorPluginBridge;
