// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/Set.h"
#include "Containers/UnrealString.h"
#include "Templates/SharedPointer.h"

#if UE_ENABLE_ICU
THIRD_PARTY_INCLUDES_START
	#include <unicode/regex.h>
THIRD_PARTY_INCLUDES_END

/**
 * Manages the lifespan of ICU regex objects
 */
class FICURegexManager
{
public:
	static void Create();
	static void Destroy();
	static bool IsInitialized();
	static FICURegexManager& Get();

	TWeakPtr<icu::RegexPattern> CreateRegexPattern(const FString& InSourceString);
	void DestroyRegexPattern(TWeakPtr<icu::RegexPattern>& InICURegexPattern);

	TWeakPtr<icu::RegexMatcher> CreateRegexMatcher(icu::RegexPattern* InPattern, const icu::UnicodeString* InInputString);
	void DestroyRegexMatcher(TWeakPtr<icu::RegexMatcher>& InICURegexMatcher);

private:
	static FICURegexManager* Singleton;

	FCriticalSection AllocatedRegexPatternsCS;
	TSet<TSharedPtr<icu::RegexPattern>> AllocatedRegexPatterns;

	FCriticalSection AllocatedRegexMatchersCS;
	TSet<TSharedPtr<icu::RegexMatcher>> AllocatedRegexMatchers;
};

#endif
