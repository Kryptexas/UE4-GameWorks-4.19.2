#include "ActorFactoryFlex.h"
#include "FlexActor.h"
#include "FlexComponent.h"
#include "StaticMeshResources.h"

/*-----------------------------------------------------------------------------
UActorFactoryFlex
-----------------------------------------------------------------------------*/
UActorFactoryFlex::UActorFactoryFlex(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DisplayName = NSLOCTEXT("Flex", "FlexDisplayName", "Flex Actor");
	NewActorClass = AFlexActor::StaticClass();
}

bool UActorFactoryFlex::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (!AssetData.IsValid() || !AssetData.GetClass()->IsChildOf(UStaticMesh::StaticClass()))
	{
		OutErrorMsg = NSLOCTEXT("CanCreateActor", "NoStaticMesh", "A valid static mesh must be specified.");
		return false;
	}

	return true;
}

void UActorFactoryFlex::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);

	if (StaticMesh)
	{
		UE_LOG(LogActorFactory, Log, TEXT("Actor Factory created %s"), *StaticMesh->GetName());

		// Change properties
		AFlexActor* FlexActor = CastChecked<AFlexActor>(NewActor);
		UStaticMeshComponent* StaticMeshComponent = FlexActor->GetStaticMeshComponent();
		check(StaticMeshComponent);

		StaticMeshComponent->UnregisterComponent();

		StaticMeshComponent->SetStaticMesh(StaticMesh);
		StaticMeshComponent->StaticMeshDerivedDataKey = StaticMesh->RenderData->DerivedDataKey;

		// Init Component
		StaticMeshComponent->RegisterComponent();
	}
}
