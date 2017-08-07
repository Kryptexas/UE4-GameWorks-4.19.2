// Copyright 2017 Google Inc.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/LatentActionManager.h"
#include "EcefToolsLibrary.h"
#include "ThreadSafeBool.h"
#include "TangoAreaLearningLibrary.generated.h"

USTRUCT(BlueprintType)
struct FTangoAreaDescriptionMetadata
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango|Area Learning")
	FString ID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango|Area Learning")
	FECEF_Transform ECEF_Transform;
};

/**
 *
 */
UCLASS()
class TANGOAREALEARNING_API UTangoAreaLearningLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	/**
	 * Saves the currently loaded local area description
	 */
	UFUNCTION(BlueprintCallable, Category = "Tango|Area Learning", meta = (Latent, WorldContext = "WorldContextObject", LatentInfo = "LatentInfo", Keywords = "ADF, area, learning, description, save, current"))
	static void SaveCurrentLocalAreaDescription(
		UObject* WorldContextObject, struct FLatentActionInfo LatentInfo,
		const FECEF_Transform& ECEF_Transform, FTangoAreaDescriptionMetadata& AreaDescriptionMetadata,
		bool& bWasSuccessful
	);

	/**
	 * Returns the currently loaded local area description metadata
	 */
	UFUNCTION(BlueprintCallable, Category = "Tango|Area Learning")
	static void GetCurrentLocalAreaDescriptionMetadata(
		FTangoAreaDescriptionMetadata& AreaDescriptionMetadata,
		bool& bWasSuccessful);

	/**
	 * Returns the local area description metadata for the given area description id
	 */
	UFUNCTION(BlueprintCallable, Category = "Tango|Area Learning")
	static void GetLocalAreaDescriptionMetadata(
		const FString& AreaDescriptionID,
		FTangoAreaDescriptionMetadata& AreaDescriptionMetadata,
		bool& bWasSuccessful);

	/**
	 * Removes the area description with the given area description id from the device
	 */
	UFUNCTION(BlueprintCallable, Category = "Tango|Area Learning")
	static void DeleteLocalAreaDescription(const FString& AreaDescriptionID, bool& bWasSuccessful);

	/**
	 * Returns a list of the metadata for all the local area descriptions on the device
	 */
	UFUNCTION(BlueprintCallable, Category = "Tango|Area Learning")
	static void ListLocalAreaDescriptionMetadata(
		TArray<FTangoAreaDescriptionMetadata>& AreaDescriptionMetadataList,
		bool& bWasSuccessful);

	/**
	 * Copies the local area description with the given area description id to the specified
	 * external storage location folder
	 */
	UFUNCTION(BlueprintCallable, Category = "Tango|Area Learning", meta = (Latent, WorldContext = "WorldContextObject", LatentInfo = "LatentInfo"))
	static void ExportLocalAreaDescription(
		UObject* WorldContextObject, struct FLatentActionInfo LatentInfo,
		const FString& AreaDescriptionID, const FString& ToFolder, bool& bWasSuccessful
	);

	/**
	 * Copies the given area description file from external storage
	 */
	UFUNCTION(BlueprintCallable, Category = "Tango|Area Learning", meta = (Latent, WorldContext = "WorldContextObject", LatentInfo = "LatentInfo"))
	static void ImportLocalAreaDescription(
		UObject* WorldContextObject, struct FLatentActionInfo LatentInfo,
		const FString& Filename, bool& bWasSuccessful
	);

	static void OnTangoConnect(bool bConnected)
	{
		bTangoIsConnected.AtomicSet(bConnected);
		if (!bTangoIsConnected)
		{
			bCurrentTangoLocalAreaDescriptionMetadataIsValid.AtomicSet(false);
		}
	}
	// Cache of current meta data. This is necessary for the polling api that
	// we provide to be called on the game thread
	static FDelegateHandle OnTangoConnectedHandle;
	static FTangoAreaDescriptionMetadata CurrentTangoLocalAreaDescriptionMetadata;
	static FThreadSafeBool bCurrentTangoLocalAreaDescriptionMetadataIsValid;
	static FThreadSafeBool bTangoIsConnected;
};
