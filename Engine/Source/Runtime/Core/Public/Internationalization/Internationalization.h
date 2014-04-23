// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#define ENABLE_LOC_TESTING ( UE_BUILD_DEBUG | UE_BUILD_DEVELOPMENT | UE_BUILD_TEST )

#include "Text.h"
#include "TextLocalizationManager.h"
#include "TextLocalizationManagerGlobals.h"
#include "Culture.h"

#define LOC_DEFINE_REGION

/** The global namespace that must be defined/undefined to wrap uses of the NS-prefixed macros below */
#undef LOCTEXT_NAMESPACE

/**	
 * Creates an FText. All parameters must be string literals. All literals will be passed through the localization system.
 * The global LOCTEXT_NAMESPACE macro must be first set to a string literal to specify this localization key's namespace.
 */
#define LOCTEXT( InKey, InTextLiteral ) FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText( TEXT( InTextLiteral ), TEXT(LOCTEXT_NAMESPACE), TEXT( InKey ) )

/**
 * Creates an FText. All parameters must be string literals. All literals will be passed through the localization system.
 */
#define NSLOCTEXT( InNamespace, InKey, InTextLiteral ) FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText( TEXT( InTextLiteral ), TEXT( InNamespace ), TEXT( InKey ) )

class FInternationalization
{
public:
	static CORE_API FInternationalization& Get();

	static CORE_API FText ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText( const TCHAR* InTextLiteral, const TCHAR* Namespace, const TCHAR* Key )
	{
		return FText( InTextLiteral, Namespace, Key );
	}

	CORE_API void GetTimeZonesIDs(TArray<FString>& TimeZonesIDs);

	//Set the current culture by name
	CORE_API void SetCurrentCulture(const FString& Name);

	//@return the current culture
	CORE_API TSharedRef< FCulture > GetCurrentCulture();

	//@return culture object by given name, or NULL if not found
	CORE_API TSharedPtr< FCulture > GetCulture(const FString& Name);

	//@return the default culture
	CORE_API TSharedRef< FCulture > GetDefaultCulture()
	{
		return DefaultCulture.ToSharedRef();
	}

	//@return the invariant culture
	CORE_API TSharedRef< FCulture > GetInvariantCulture()
	{
		return InvariantCulture.ToSharedRef();
	}

	CORE_API void Initialize();
	CORE_API void Terminate();

	CORE_API bool IsInitialized() {return bIsInitialized;}

#if ENABLE_LOC_TESTING
	static CORE_API FString& Leetify(FString& SourceString);
#endif

	CORE_API void GetCultureNames(TArray<FString>& CultureNames);

private:
	FInternationalization();

	//@todo how are we going to initialize the list of cultures? below is temporary
	CORE_API void PopulateAllCultures(void);

	//@return index of a culture by given name, or -1 if not found
	CORE_API int32 GetCultureIndex(const FString& Name);

private:
	static FInternationalization* Instance;
	bool bIsInitialized;

	TArray< TSharedRef< FCulture > > AllCultures;

	int CurrentCultureIndex;

	TSharedPtr< FCulture > DefaultCulture;
	TSharedPtr< FCulture > InvariantCulture;
};

#undef LOC_DEFINE_REGION
