// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "ScriptAsset.generated.h"

/** Definition of a specific ability that is applied to a character. Exists as part of a Trait. */
UCLASS(BlueprintType)
class SCRIPTPLUGIN_API UScriptAsset : public UDataAsset
{
	GENERATED_UCLASS_BODY()

	/** Path to the script text used to construct this asset. Relative to the object's package, BaseDir() or absolute */
	UPROPERTY(Category = Script, VisibleAnywhere, BlueprintReadWrite)
	FString SourceFilePath;

	/** Date/Time-stamp of the file from the last import. */
	UPROPERTY(Category = Script, VisibleAnywhere)
	FString SourceFileTimestamp;

	/** Generated script bytecode */
	UPROPERTY()
	TArray<uint8> ByteCode;

	/** Script source code. @todo: this should be editor-only */
	UPROPERTY()
	FString SourceCode;

	// UObject interface
	virtual void PostLoad() OVERRIDE;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent);

	/** Re-load script if it has changed and compile */
	virtual void UpdateAndCompile();

	/** Compile script to bytecode */
	virtual void Compile();
#endif
};
