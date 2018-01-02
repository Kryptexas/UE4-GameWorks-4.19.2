// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ObjectMacros.h"
#include "GLTFAsset.h"

class UTexture2D;
class UMaterial;

TArray<UTexture2D*> ImportTextures(const GLTF::FAsset&, UObject* InParent, FName InName, EObjectFlags Flags);
TArray<UMaterial*> ImportMaterials(const GLTF::FAsset&, UObject* InParent, FName InName, EObjectFlags Flags);
