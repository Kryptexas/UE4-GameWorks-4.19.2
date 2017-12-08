// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "FlexMigrateContentCommandlet.generated.h"

UCLASS()
class UFlexMigrateContentCommandlet : public UCommandlet
{
	GENERATED_UCLASS_BODY()

	virtual int32 Main(const FString& Params) override;

private:
	class UObject* MigrateStaticMesh(class UStaticMesh* StaticMesh, TArray<UPackage*>& DirtiedPackages);
	class UObject* MigrateParticleEmitter(class UParticleSpriteEmitter* ParticleEmitter, TArray<UPackage*>& DirtiedPackages);
	//class UObject* MigrateParticleSystemComponent(class UParticleSystemComponent* ParticleSystemComponent, TArray<UPackage*>& DirtiedPackages);

	bool ForceReplaceReferences(const TMap<UObject*, UObject*>& ReplacementMap, TArray<UPackage*>& DirtiedPackages);
};
