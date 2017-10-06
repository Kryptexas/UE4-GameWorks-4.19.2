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

bool FFlexEditorPluginBridge::KeepFlexSimulationChanges(class AActor* EditorWorldActor, class AActor* SimWorldActor)
{
	// save flex actors' simulated positions, note does not support undo
	AFlexActor* FlexEditorActor = Cast<AFlexActor>(EditorWorldActor);
	AFlexActor* FlexSimActor = Cast<AFlexActor>(SimWorldActor);
	if (FlexEditorActor != NULL && FlexSimActor != NULL)
	{
		UFlexComponent* FlexEditorComponent = (UFlexComponent*)(FlexEditorActor->GetRootComponent());
		UFlexComponent* FlexSimComponent = (UFlexComponent*)(FlexSimActor->GetRootComponent());
		if (FlexEditorComponent != NULL && FlexSimComponent != NULL)
		{
			FlexEditorComponent->PreSimPositions.SetNum(FlexSimComponent->SimPositions.Num());

			for (int i = 0; i < FlexSimComponent->SimPositions.Num(); ++i)
				FlexEditorComponent->PreSimPositions[i] = FlexSimComponent->SimPositions[i];

			FlexEditorComponent->SavedRelativeLocation = FlexEditorComponent->RelativeLocation;
			FlexEditorComponent->SavedRelativeRotation = FlexEditorComponent->RelativeRotation;
			FlexEditorComponent->SavedTransform = FlexEditorComponent->GetComponentTransform();

			FlexEditorComponent->PreSimRelativeLocation = FlexSimComponent->RelativeLocation;
			FlexEditorComponent->PreSimRelativeRotation = FlexSimComponent->RelativeRotation;
			FlexEditorComponent->PreSimTransform = FlexSimComponent->GetComponentTransform();

			FlexEditorComponent->PreSimShapeTranslations = FlexSimComponent->PreSimShapeTranslations;
			FlexEditorComponent->PreSimShapeRotations = FlexSimComponent->PreSimShapeRotations;

			FlexEditorComponent->SendRenderTransform_Concurrent();
			return true;
		}
	}
	return false;
}

bool FFlexEditorPluginBridge::ClearFlexSimulationChanges(class AActor* EditorWorldActor)
{
	// clear flex actors' simulated positions, note does not support undo
	AFlexActor* FlexEditorActor = Cast<AFlexActor>(EditorWorldActor);
	if (FlexEditorActor != NULL)
	{
		UFlexComponent* FlexEditorComponent = (UFlexComponent*)(FlexEditorActor->GetRootComponent());
		if (FlexEditorComponent != NULL && FlexEditorComponent->PreSimPositions.Num())
		{
			FlexEditorComponent->PreSimPositions.SetNum(0);
			FlexEditorComponent->PreSimShapeTranslations.SetNum(0);
			FlexEditorComponent->PreSimShapeRotations.SetNum(0);

			FlexEditorComponent->RelativeLocation = FlexEditorComponent->SavedRelativeLocation;
			FlexEditorComponent->RelativeRotation = FlexEditorComponent->SavedRelativeRotation;
			FlexEditorComponent->SetComponentToWorld(FlexEditorComponent->SavedTransform);

			FlexEditorComponent->SendRenderTransform_Concurrent();
			return true;
		}
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
