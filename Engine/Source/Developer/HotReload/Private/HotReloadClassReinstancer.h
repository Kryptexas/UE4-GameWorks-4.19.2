// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#if WITH_ENGINE

#include "Editor/UnrealEd/Public/Kismet2/KismetReinstanceUtilities.h"

/**
 * Helper class used for re-instancing native and blueprint classes after hot-reload
 */
class FHotReloadClassReinstancer : public FBlueprintCompileReinstancer
{
	/** True if the provided native class needs re-instancing */
	bool bNeedsReinstancing;

	/** 
	 * Sets the re-instancer up for new class re-instancing 
	 *
	 * @param InNewClass Class that has changed after hot-reload
	 * @param InOldClass Class before it was hot-reloaded
	 */
	void SetupNewClassReinstancing(UClass* InNewClass, UClass* InOldClass);

	/**
	* Sets the re-instancer up for old class re-instancing. Always re-creates the CDO.
	*
	* @param InOldClass Class that has NOT changed after hot-reload
	*/
	void RecreateCDOAndSetupOldClassReinstancing(UClass* InOldClass);
	
	/**
	* Creates a mem-comparable array of data containing CDO property values.
	*
	* @param InObject CDO
	* @param OutData Data containing all of the CDO property values
	*/
	void SerializeCDOProperties(UObject* InObject, TArray<uint8>& OutData);

	/**
	* Re-creates class default object
	*
	* @param InOldClass Class that has NOT changed after hot-reload
	*/
	void ReconstructClassDefaultObject(UClass* InOldClass);

public:

	/** Sets the re-instancer up to re-instance native classes */
	FHotReloadClassReinstancer(UClass* InNewClass, UClass* InOldClass);
	
	/** Destructor */
	virtual ~FHotReloadClassReinstancer();

	/** If true, the class needs re-instancing */
	FORCEINLINE bool ClassNeedsReinstancing() const
	{
		return bNeedsReinstancing;
	}
};

#endif // WITH_ENGINE