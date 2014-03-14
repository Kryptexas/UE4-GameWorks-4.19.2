// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Internationalization/Text.h"

#if !UE_ENABLE_ICU
#include "NumberFormattingRules.h"
#include "TextFormattingRules.h"
#include "DateTimeFormattingRules.h"
#endif

class CORE_API FCulture
{
#if UE_ENABLE_ICU
public:
	FCulture(const FString& LocaleName);
#else
public:
	FCulture(	const FText& InDisplayName, 
				const FString& InEnglishName, 
				const int InKeyboardLayoutId, 
				const int InLCID, 
				const FString& InName, 
				const FString& InNativeName, 
				const FString& InUnrealLegacyThreeLetterISOLanguageName, 
				const FString& InThreeLetterISOLanguageName, 
				const FString& InTwoLetterISOLanguageName, 
				const FNumberFormattingRules& InNumberFormattingRule, 
				const FTextFormattingRules& InTextFormattingRule, 
				const FDateTimeFormattingRules& InDateTimeFormattingRule);
#endif

public:
	FString GetDisplayName() const;

	FString GetEnglishName() const;

	int GetKeyboardLayoutId() const;

	int GetLCID() const;

	FString GetName() const;

	FString GetNativeName() const;

	FString GetNativeLanguage() const;

	FString GetNativeRegion() const;

	FString GetUnrealLegacyThreeLetterISOLanguageName() const;

	FString GetThreeLetterISOLanguageName() const;

	FString GetTwoLetterISOLanguageName() const;

	FString GetCountry() const;

	FString GetVariant() const;

#if UE_ENABLE_ICU
public:
	class FICUCultureImplementation;
	TSharedRef<FICUCultureImplementation> Implementation;
#else
public:
	class FLegacyCultureImplementation;
	TSharedRef<FLegacyCultureImplementation> Implementation;

	// The date formating rule for this culture
	const FDateTimeFormattingRules DateTimeFormattingRule;

	// The text formating rule for this culture
	const FTextFormattingRules TextFormattingRule;

	// The number formating rule for this culture
	const FNumberFormattingRules NumberFormattingRule;
#endif
};
