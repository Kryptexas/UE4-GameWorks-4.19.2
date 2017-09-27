#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/StaticMesh.h"
#include "FlexStaticMesh.generated.h"

//JDM: NOTE - C&Ped from UStaticMesh, with custom constructor removed. Not sure all of these flags are appropriate.
UCLASS(collapsecategories, hidecategories = Object, MinimalAPI, BlueprintType, config = Engine)
class UFlexStaticMesh : public UStaticMesh
{
	GENERATED_BODY()

public:

	UFlexStaticMesh(const class FObjectInitializer &ObjectInitializer);
	virtual ~UFlexStaticMesh() = default;

	/** Properties for the associated Flex object */
	UPROPERTY(EditAnywhere, Instanced, Category = Flex)
	class UFlexAsset* FlexAsset;
};
