#include "FlexEditorPluginBridge.h"

#include "FlexActor.h"
#include "FlexAsset.h"
#include "FlexComponent.h"
#include "FlexContainer.h"
#include "FlexAssetPreviewComponent.h"

#include "Engine/StaticMesh.h"
#include "Engine/World.h"

#include "AdvancedPreviewScene.h"

#include "FlexStaticMesh.h"
#include "FlexParticleSpriteEmitter.h"
#include "FlexSoftJointComponent.h"

#include "ComponentVisualizers.h"
#include "FlexSoftJointComponentVisualizer.h"

class UFlexAsset* FFlexEditorPluginBridge::GetFlexAsset(class UStaticMesh* StaticMesh)
{
	UFlexStaticMesh* FSM = Cast<UFlexStaticMesh>(StaticMesh);
	return FSM ? FSM->FlexAsset : nullptr;
}

void FFlexEditorPluginBridge::SetFlexAsset(class UStaticMesh* StaticMesh, class UFlexAsset* FlexAsset)
{
	UFlexStaticMesh* FSM = Cast<UFlexStaticMesh>(StaticMesh);
	if (FSM)
	{
		FSM->FlexAsset = FlexAsset;
	}
}


class UPrimitiveComponent* FFlexEditorPluginBridge::UpdateFlexPreviewComponent(bool bDrawFlexPreview, class FAdvancedPreviewScene* PreviewScene, class UStaticMesh* StaticMesh, class UPrimitiveComponent* InFlexPreviewComponent)
{
	UFlexAssetPreviewComponent* FlexPreviewComponent = Cast<UFlexAssetPreviewComponent>(InFlexPreviewComponent);

	if (FlexPreviewComponent)
	{
		PreviewScene->RemoveComponent(FlexPreviewComponent);
		FlexPreviewComponent = NULL;
	}

	class UFlexAsset* FlexAsset = FFlexEditorPluginBridge::GetFlexAsset(StaticMesh);

	bool bDisplayFlexParticles = FlexAsset && FlexAsset->ContainerTemplate && bDrawFlexPreview;
	if (bDisplayFlexParticles)
	{
		FlexPreviewComponent = NewObject<UFlexAssetPreviewComponent>();
		FlexPreviewComponent->FlexAsset = FlexAsset;
		PreviewScene->AddComponent(FlexPreviewComponent, FTransform::Identity);
	}

	return FlexPreviewComponent;
}

bool FFlexEditorPluginBridge::IsObjectFlexAssetOrContainer(class UObject* ObjectBeingModified, class UStaticMesh* StaticMesh)
{
	class UFlexAsset* FlexAsset = FFlexEditorPluginBridge::GetFlexAsset(StaticMesh);
	return (ObjectBeingModified == FlexAsset || (FlexAsset && ObjectBeingModified == FlexAsset->ContainerTemplate));
}

bool FFlexEditorPluginBridge::IsChildOfFlexAsset(class UClass* Class)
{
	return Class->IsChildOf(UFlexAsset::StaticClass());
}

bool FFlexEditorPluginBridge::GetFlexAssetStats(class UStaticMesh* StaticMesh, FlexAssetStats& FlexAssetStats)
{
	class UFlexAsset* FlexAsset = FFlexEditorPluginBridge::GetFlexAsset(StaticMesh);
	if (FlexAsset)
	{
		FlexAssetStats.NumParticles = FlexAsset->Particles.Num();
		FlexAssetStats.NumShapes = FlexAsset->ShapeCenters.Num();
		FlexAssetStats.NumSprings = FlexAsset->SpringCoefficients.Num();
		return true;
	}
	return false;
}

class AActor* FFlexEditorPluginBridge::SpawnFlexActor(class UWorld* World, struct FTransform const* UserTransformPtr, const struct FActorSpawnParameters& SpawnParameters)
{
	return World->SpawnActor(AFlexActor::StaticClass(), UserTransformPtr, SpawnParameters);
}

bool FFlexEditorPluginBridge::IsFlexActor(class UClass* Class)
{
	return Class->IsChildOf(AFlexActor::StaticClass());
}

void FFlexEditorPluginBridge::SetFlexActorCollisionProfileName(class AStaticMeshActor* SMActor, FName CollisionProfileName)
{
	AFlexActor* FlexActor = CastChecked<AFlexActor>(SMActor);
	if (FlexActor)
	{
		FlexActor->GetStaticMeshComponent()->SetCollisionProfileName(CollisionProfileName);
	}
}

class UClass* FFlexEditorPluginBridge::GetFlexParticleSpriteEmitterClass()
{
	return UFlexParticleSpriteEmitter::StaticClass();
}

bool FFlexEditorPluginBridge::IsFlexStaticMesh(class UStaticMesh* StaticMesh)
{
	return StaticMesh ? StaticMesh->GetClass()->IsChildOf<UFlexStaticMesh>() : false;
}

void FFlexEditorPluginBridge::ScaleComponent(class USceneComponent* RootComponent, float Scale)
{
	if (UFlexSoftJointComponent* FlexSoftJointComponent = Cast< UFlexSoftJointComponent >(RootComponent))
	{
		FlexSoftJointComponent->Radius *= Scale;
	}
}

void FFlexEditorPluginBridge::RegisterComponentVisualizers(class FComponentVisualizersModule* ComponentVisualizersModule)
{
	ComponentVisualizersModule->RegisterComponentVisualizer(UFlexSoftJointComponent::StaticClass()->GetFName(), MakeShareable(new FFlexSoftJointComponentVisualizer));
}
