// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interface_AssetUserData.generated.h"


/** Interface for assets that can own UserData **/
UINTERFACE(meta=(CannotImplementInterfaceInBlueprint))
class UInterface_AssetUserData : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class IInterface_AssetUserData
{
	GENERATED_IINTERFACE_BODY()

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

