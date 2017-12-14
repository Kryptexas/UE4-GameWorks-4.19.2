// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ObjectMacros.h"
#include "GLTFAsset.h"

class UStaticMesh;
class UMaterial;

UStaticMesh* ImportStaticMesh(const GLTF::FAsset&, const TArray<UMaterial*>&, UObject* InParent, FName InName, EObjectFlags Flags, uint32 Index = 0);
TArray<UStaticMesh*> ImportStaticMeshes(const GLTF::FAsset&, const TArray<UMaterial*>&, UObject* InParent, FName InName, EObjectFlags Flags);
