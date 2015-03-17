// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interface_AssetUserData.generated.h"

class UAssetUserData;

/** Interface for assets/objects that can own UserData **/
UINTERFACE(MinimalApi, meta=(CannotImplementInterfaceInBlueprint))
class UInterface_AssetUserData : public UInterface
{
	GENERATED_BODY()
public:
	ENGINE_API UInterface_AssetUserData(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

class IInterface_AssetUserData
{
	GENERATED_BODY()
public:

	virtual void AddAssetUserData(UAssetUserData* InUserData) {}

	virtual UAssetUserData* GetAssetUserDataOfClass(TSubclassOf<UAssetUserData> InUserDataClass)
	{
		return NULL;
	}

	virtual const TArray<UAssetUserData*>* GetAssetUserDataArray() const
	{
		return NULL;
	}

	template<typename T>
	T* GetAssetUserData()
	{
		return CastChecked<T>( GetAssetUserDataOfClass(T::StaticClass()) );
	}

	virtual void RemoveUserDataOfClass(TSubclassOf<UAssetUserData> InUserDataClass) {}

};

