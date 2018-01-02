// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"

#include "DatasmithAreaLightActor.generated.h"

// Keep in sync with EDatasmithLightShape from DatasmithDefinitions
UENUM(BlueprintType)
enum class EDatasmithAreaLightActorShape : uint8
{
	Rectangle,
	Disc,
	Sphere,
	Cylinder
};

UCLASS(BlueprintType, Blueprintable, MinimalAPI)
class ADatasmithAreaLightActor : public AActor
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	EDatasmithAreaLightActorShape LightShape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FVector2D Dimensions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor Color;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	float Intensity;
};