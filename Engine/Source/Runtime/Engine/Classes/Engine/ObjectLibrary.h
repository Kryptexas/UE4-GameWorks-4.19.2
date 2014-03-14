// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ObjectLibrary.generated.h"

/** Class that holds a library of Objects */
UCLASS(MinimalAPI)
class UObjectLibrary : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Class that Objects must be of */
	UPROPERTY(EditAnywhere, Category=ObjectLibrary, meta=(AllowAbstract = ""))
	UClass*				ObjectBaseClass;

protected:

	/** List of Objects in library */
	UPROPERTY(EditAnywhere, Category=ObjectLibrary)
	TArray<UObject*>	Objects;

	/** Weak pointers to objects */
	UPROPERTY()
	TArray<TWeakObjectPtr <UObject> >	WeakObjects;

	/** If this library should use weak pointers */
	UPROPERTY(Transient)
	bool bUseWeakReferences;

	/** True if we've already fully loaded this library, can't do it twice */
	UPROPERTY(Transient)
	bool bIsFullyLoaded;

	/** Asset data of objects that will belong in library, possibly not loaded yet */
	TArray<class FAssetData>	AssetDataList;

public:
	// Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	// End UObject Interface

	/** Set rather this library is using weak or strong references */
	virtual void UseWeakReferences(bool bSetUseWeak);

	/** Attempt to add a new Object, return true if added */
	virtual bool AddObject(UObject *NewObject);

	/** Attempt to remove an Object from the object, return true if removed */
	virtual bool RemoveObject(UObject *ObjectToRemove);

	/** Fills in a passed in array of objects, casts to proper type */
	template <typename T>
	void GetObjects(TArray<T*>& OutObjects)
	{
		for (int32 i = 0; i < Objects.Num(); i++)
		{
			T* Obj = Cast<T>(Objects[i]);
			if (Obj)
			{
				OutObjects.Add(Obj);
			}
		}

		for (int32 i = 0; i < WeakObjects.Num(); i++)
		{
			T* Obj = Cast<T>(WeakObjects[i].Get());
			if (Obj)
			{
				OutObjects.Add(Obj);
			}
		}
	}

	/** Returns the number of objects */
	int32 GetObjectCount() const 
	{
		return Objects.Num() + WeakObjects.Num();
	}

	/** Returns the list of asset data */
	virtual void GetAssetDataList(TArray<FAssetData>& OutAssetData);

	/** Returns the number of objects */
	int32 GetAssetDataCount() const 
	{
		return AssetDataList.Num();
	}

	/** Clears the current loaded objects and asset data */
	virtual void ClearLoaded();

	/** Load an entire subdirectory of assets into this object library. Returns number of assets loaded */
	virtual int32 LoadAssetsFromPath(const FString& Path);

	/** Load an entire subdirectory of blueprints into this object library. Only loads blueprints of passed in class. Returns number of assets loaded */
	virtual int32 LoadBlueprintsFromPath(const FString& Path, UClass *ParentClass);

	/** Gets asset data for assets in a subdirectory. Returns number of assets data loaded */
	virtual int32 LoadAssetDataFromPath(const FString& Path);

	/** Load an entire subdirectory of blueprints into this object library. Only loads asset data for blueprints of passed in class. Returns number of asset data loaded loaded */
	virtual int32 LoadBlueprintAssetDataFromPath(const FString& Path, UClass *ParentClass);

	/** Load all of the objects in asset data list into memory  */
	virtual int32 LoadAssetsFromAssetData();


};
