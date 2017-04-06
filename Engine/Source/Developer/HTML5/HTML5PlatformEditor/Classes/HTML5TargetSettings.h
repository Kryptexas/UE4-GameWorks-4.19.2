// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	HTML5TargetSettings.h: Declares the UHTML5TargetSettings class.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "HTML5TargetSettings.generated.h"


USTRUCT()
struct FHTML5LevelTransitions
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = HTML5_LevelTransitions, Meta = (DisplayName = "From Map"))
	FFilePath MapFrom;

	UPROPERTY(EditAnywhere, Category = HTML5_LevelTransitions, Meta = (DisplayName = "To Map"))
	FFilePath MapTo;
};


/**
 * Implements the settings for the HTML5 target platform.
 */
UCLASS(config=Engine, defaultconfig)
class HTML5PLATFORMEDITOR_API UHTML5TargetSettings
	: public UObject
{
public:

	GENERATED_UCLASS_BODY()

	// ------------------------------------------------------------

	/**
	 * Target Web Assembly builds
	 */
	UPROPERTY(GlobalConfig, EditAnywhere, Category=Emscripten, Meta = (DisplayName = "WebAssembly Build (or else ASM.js)"))
	bool TargetWasm;

	/**
	 * Target WebGL2 builds
	 */
	UPROPERTY(GlobalConfig, EditAnywhere, Category=Emscripten, Meta = (DisplayName = "WebGL2 Build (or else WebGL1)"))
	bool TargetWebGL2;

	/**
	 * Use Fixed TimeStep
	 */
	UPROPERTY(GlobalConfig, EditAnywhere, Category=Emscripten, Meta = (DisplayName = "Fixed TimeStep (i.e. use requestAnimationFrame)"))
	bool UseFixedTimeStep; // need to make a note of: answerhub 409629

	/**
	 * Enable SIMD - NOTE: this does not currently work with WASM - it will be forced false in this case.
	 * SIMD will be supported during WASM builds in a future emscripten release.
	 */
	UPROPERTY(GlobalConfig, EditAnywhere, Category=Emscripten, Meta = (DisplayName = "SIMD support (asm.js only)"))
	bool EnableSIMD;

	/**
	 * Enable Multithreading
	 */
	UPROPERTY(GlobalConfig, EditAnywhere, Category=Emscripten, Meta = (DisplayName = "Multithreading support (asm.js only - experimental)"))
	bool EnableMultithreading;

	/**
	 * Enable Tracing (trace.h)
	 */
	UPROPERTY(GlobalConfig, EditAnywhere, Category=Emscripten, Meta = (DisplayName = "Tracing support"))
	bool EnableTracing;

	// ------------------------------------------------------------

	/**
	 * Compress Files
	 */
	UPROPERTY(GlobalConfig, EditAnywhere, Category=Packaging, Meta = (DisplayName = "Compress files during shipping packaging"))
	bool Compressed;

	/**
	 * Setting to control HTML5 Heap size (in Development)
	 */
	UPROPERTY(GlobalConfig, EditAnywhere, Category=Packaging, Meta = (DisplayName = "Development Heap Size (in MB) - ASM.js", ClampMin="1", ClampMax="4096"))
	int32 HeapSizeDevelopment;

	/**
	 * Setting to control HTML5 Heap size
	 */
	UPROPERTY(GlobalConfig, EditAnywhere, Category=Packaging, Meta = (DisplayName = "Heap Size (in MB) - ASM.js", ClampMin="1", ClampMax="4096"))
	int32 HeapSizeShipping;

	// ------------------------------------------------------------

	/**
	 * Port to use when deploying game from the editor
	 */
	UPROPERTY(GlobalConfig, EditAnywhere, Category = Launch, Meta = (DisplayName = "Port to use when deploying game from the editor", ClampMin="49152", ClampMax="65535"))
	int32 DeployServerPort;

	/**
	 * Use a loading level and download maps during transitions.
	 */
	UPROPERTY(GlobalConfig, EditAnywhere, Category = Launch, Meta = (DisplayName = "Download maps on the fly"))
	bool UseAsyncLevelLoading;

	/**
	 * Generate Delta Pak files for these level transitions.
	 */
	UPROPERTY(GlobalConfig, EditAnywhere, Category = Launch, Meta = (DisplayName = "Level transitions for delta paks [experimental,depends on download maps]"))
	TArray<FHTML5LevelTransitions> LevelTransitions;

	// ------------------------------------------------------------

	UPROPERTY(GlobalConfig, EditAnywhere, Category = Amazon_S3, Meta = (DisplayName = "Upload builds to Amazon S3 when packaging"))
	bool UploadToS3;

	/**
	* Required
	*/
	UPROPERTY(GlobalConfig, EditAnywhere, Category = Amazon_S3, Meta = (DisplayName = "Amazon S3 Region", EditCondition = "UploadToS3"))
	FString S3Region;
	/**
	* Required
	*/
	UPROPERTY(GlobalConfig, EditAnywhere, Category = Amazon_S3, Meta = (DisplayName = "Amazon S3 Key ID", EditCondition = "UploadToS3"))
	FString S3KeyID;
	/**
	* Required
	*/
	UPROPERTY(GlobalConfig, EditAnywhere, Category = Amazon_S3, Meta = (DisplayName = "Amazon S3 Secret Access Key", EditCondition = "UploadToS3"))
	FString S3SecretAccessKey;
	/**
	* Required
	*/
	UPROPERTY(GlobalConfig, EditAnywhere, Category = Amazon_S3, Meta = (DisplayName = "Amazon S3 Bucket Name", EditCondition = "UploadToS3"))
	FString S3BucketName;
	/**
	* Provide another level of nesting beyond the bucket. Can be left empty, defaults to game name. DO NOT LEAVE A TRAILING SLASH!
	*/
	UPROPERTY(GlobalConfig, EditAnywhere, Category = Amazon_S3, Meta = (DisplayName = "Nested Folder Name", EditCondition = "UploadToS3"))
	FString S3FolderName;
};

