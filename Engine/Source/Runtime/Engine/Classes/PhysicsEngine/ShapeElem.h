// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EngineDefines.h"
#include "PhysxUserData.h"
#include "ShapeElem.generated.h"

namespace EAggCollisionShape
{
	enum Type
	{
		Sphere,
		Box,
		Sphyl,
		Convex,
		Unknown
	};
}

/** Sphere shape used for collision */
USTRUCT()
struct FKShapeElem
{
	GENERATED_USTRUCT_BODY()

	FKShapeElem()
	: ShapeType(EAggCollisionShape::Unknown)
#if WITH_PHYSX
	, UserData(this)
#endif
	
	{}

	FKShapeElem(EAggCollisionShape::Type InShapeType)
	: ShapeType(InShapeType)
#if WITH_PHYSX
	, UserData(this)
#endif
	{}

	FKShapeElem(const FKShapeElem& Copy)
	: ShapeType(Copy.ShapeType)
#if WITH_PHYSX
	, UserData(this)
#endif
	{
	}

	virtual ~FKShapeElem(){}

	const FKShapeElem& operator=(const FKShapeElem& Other)
	{
		CloneElem(Other);
		return *this;
	}

	template <typename T>
	T* GetShapeCheck()
	{
		check(T::StaticShapeType == ShapeType);
		return (T*)this;
	}

#if WITH_PHYSX
	const FPhysxUserData* GetUserData() const { FPhysxUserData::Set<FKShapeElem>((void*)&UserData, const_cast<FKShapeElem*>(this));  return &UserData; }
#endif // WITH_PHYSX

	ENGINE_API static EAggCollisionShape::Type StaticShapeType;

#if WITH_EDITORONLY_DATA
	/** Get the user-defined name for this shape */
	ENGINE_API const FString& GetName() { return Name; }

	/** Set the user-defined name for this shape */
	ENGINE_API void SetName(const FString& InName) { Name = InName; }
#endif

protected:
	/** Helper function to safely clone instances of this shape element */
	void CloneElem(const FKShapeElem& Other)
	{
		ShapeType = Other.ShapeType;
	}

private:
#if WITH_EDITORONLY_DATA
	/** User-defined name for this shape */
	UPROPERTY(Category=Shape, EditAnywhere)
	FString Name;
#endif

	EAggCollisionShape::Type ShapeType;

#if WITH_PHYSX
	FPhysxUserData UserData;
#endif // WITH_PHYSX
};
