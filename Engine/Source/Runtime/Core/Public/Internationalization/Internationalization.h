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

	static CORE_API FText ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText( const TCHAR* InTextLiteral, const TCHAR* Namespace, const TCHAR* Key )
	{
		return FText( InTextLiteral, Namespace, Key );
	}

	static CORE_API void GetTimeZonesIDs(TArray<FString>& TimeZonesIDs);

	//Set the current culture by Name
	static CORE_API void SetCurrentCulture(const FString& Name);

	//@return the current culture
	static CORE_API TSharedRef< FCulture > GetCurrentCulture();

	//@return culture object by given name, or NULL if not found
	static CORE_API TSharedPtr< FCulture > GetCulture(const FString& Name);

	//@return the default culture
	static CORE_API TSharedRef< FCulture > GetDefaultCulture()
	{
		return DefaultCulture.ToSharedRef();
	}

	//@return the invariant culture
	static CORE_API TSharedRef< FCulture > GetInvariantCulture()
	{
		return InvariantCulture.ToSharedRef();
	}

	static CORE_API void Initialize();
	static CORE_API void Terminate();

	static CORE_API bool IsInitialized() {return bIsInitialized;}

#if ENABLE_LOC_TESTING
	static CORE_API FString& Leetify(FString& SourceString);
#endif

	static CORE_API void GetCultureNames(TArray<FString>& CultureNames);

private:
	//@todo how are we going to initialize the list of cultures? below is temporary
	static CORE_API void PopulateAllCultures(void);

	//@return index of a culture by given name, or -1 if not found
	static CORE_API int32 GetCultureIndex(const FString& Name);

private:
	static bool bIsInitialized;

	struct FInternationalizationLifetimeObject
	{
		FInternationalizationLifetimeObject();
	} static InternationalizationLifetimeObject;


	static CORE_API TArray< TSharedRef< FCulture > > AllCultures;

	static CORE_API int CurrentCultureIndex;

	static CORE_API TSharedPtr< FCulture > DefaultCulture;
	static CORE_API TSharedPtr< FCulture > InvariantCulture;
};

#undef LOC_DEFINE_REGION
