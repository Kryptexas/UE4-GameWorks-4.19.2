// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Math/Vector.h"
#include "Math/Rotator.h"

class UNREALED_API FComponentEditorUtils
{
public:

	static class USceneComponent* GetSceneComponent(class UObject* Object, class UObject* SubObject = NULL);

	static void GetArchetypeInstances(class UObject* Object, TArray<class UObject*>& ArchetypeInstances);

	struct FTransformData
	{
		FTransformData(const class USceneComponent& Component)
			: Trans(Component.RelativeLocation)
			, Rot(Component.RelativeRotation)
			, Scale(Component.RelativeScale3D)
		{}

		FVector Trans; 
		FRotator Rot;
		FVector Scale;
	};

	static void PropagateTransformPropertyChange(class USceneComponent* SceneComponentTemplate, const FTransformData& TransformOld, const FTransformData& TransformNew, TSet<class USceneComponent*>& UpdatedComponents);

};