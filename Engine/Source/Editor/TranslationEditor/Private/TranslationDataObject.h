// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "TranslationDataObject.generated.h"

USTRUCT()
struct FTranslationChange
{
	GENERATED_USTRUCT_BODY()

public:

	/** The changelist of this change */
	UPROPERTY(Category=Translation, VisibleAnywhere)
	FString Version;

	/** Date of this change */
	UPROPERTY(Category=Translation, VisibleAnywhere)//, meta=(DisplayName = "Date & Time"))
	FDateTime DateAndTime;

	/** Source at time of this change */
	UPROPERTY(Category=Translation, VisibleAnywhere)
	FString Source;

	/** Translation at time of this change */
	UPROPERTY(Category=Translation, VisibleAnywhere)
	FString Translation;
};

USTRUCT()
struct FTranslationContextInfo
{
	GENERATED_USTRUCT_BODY()

public:

	/** The key specified in LOCTEXT */
	UPROPERTY(Category=Translation, VisibleAnywhere)
	FString Key;

	/** What file and line this translation is from */
	UPROPERTY(Category=Translation, VisibleAnywhere)
	FString Context;

	/** List of previous versions of the source text for this context */
	UPROPERTY(Category=Translation, VisibleAnywhere)
	TArray<FTranslationChange> Changes;
};

USTRUCT()
struct FTranslationUnit
{
	GENERATED_USTRUCT_BODY()

public:

	/** The localization namespace for this translation */
	UPROPERTY(Category=Translation, VisibleAnywhere)
	FString Namespace;

	/** Original text from the source language */
	UPROPERTY(Category=Translation, VisibleAnywhere)
	FString Source;

	/** Translations */
	UPROPERTY(Category=Translation, EditAnywhere)
	FString Translation;

	/** Contexts the source was found in */
	UPROPERTY(Category=Translation, VisibleAnywhere)
	TArray<FTranslationContextInfo> Contexts;

	/** Whether the changes have been reviewed */
	UPROPERTY(Category=Translation, EditAnywhere)
	bool HasBeenReviewed;
};

/**
 * Just a wrapper for the struct with real data in it.
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UTranslationDataObject : public UObject
{
	GENERATED_UCLASS_BODY()
		  
public:  
	/** List of items that need translations */
	UPROPERTY(Category=Translation, EditAnywhere, EditFixedSize)
	TArray<FTranslationUnit> Untranslated;

	/** List of items that need review */
	UPROPERTY(Category=Translation, EditAnywhere, EditFixedSize)
	TArray<FTranslationUnit> Review;

	/** List of items whose translation is complete */
	UPROPERTY(Category=Translation, EditAnywhere, EditFixedSize)
	TArray<FTranslationUnit> Complete;
};
