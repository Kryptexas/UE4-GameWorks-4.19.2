// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "CoreTypes.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTCharToUTF8Test, "System.Core.Misc.TCharToUtf8", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FTCharToUTF8Test::RunTest(const FString& Parameters)
{
	{
		// Hebrew letter final nun
		const TCHAR TestString[] = TEXT("\x05DF");
		const ANSICHAR ExpectedResult[] = "\xD7\x9F";
		{
			const FTCHARToUTF8 ConvertedValue(TestString);
			const FUTF8ToTCHAR ReverseConvertedValue(ConvertedValue.Get());
			TestTrue(FString(TEXT("Expected test string 1 to have different encoded value")), FCStringAnsi::Strcmp(ExpectedResult, ConvertedValue.Get()) == 0);
			TestTrue(FString(TEXT("Expected test string 1 to have different decoded value")), FCString::Strcmp(TestString, ReverseConvertedValue.Get()) == 0);
		}

		{
			const FUTF8ToTCHAR ConvertedValue(ExpectedResult);
			const FTCHARToUTF8 ReverseConvertedValue(ConvertedValue.Get());
			TestTrue(FString(TEXT("Expected test string 2 to have different encoded value")), FCString::Strcmp(TestString, ConvertedValue.Get()) == 0);
			TestTrue(FString(TEXT("Expected test string 2 to have different decoded value")), FCStringAnsi::Strcmp(ExpectedResult, ReverseConvertedValue.Get()) == 0);
		}
	}

	{
		// Smiling face with open mouth and tightly-closed eyes and a Four of Circles
		const TCHAR TestString[] = TEXT("\xD83D\xDE06\xD83C\xDC1C");
		const ANSICHAR ExpectedResult[] = "\xF0\x9F\x98\x86\xF0\x9F\x80\x9C";
		{
			const FTCHARToUTF8 ConvertedValue(TestString);
			const FUTF8ToTCHAR ReverseConvertedValue(ConvertedValue.Get());
			TestTrue(FString(TEXT("Expected test string 3 to have different encoded value")), FCStringAnsi::Strcmp(ExpectedResult, ConvertedValue.Get()) == 0);
			TestTrue(FString(TEXT("Expected test string 3 to have different decoded value")), FCString::Strcmp(TestString, ReverseConvertedValue.Get()) == 0);
		}

		{
			const FUTF8ToTCHAR ConvertedValue(ExpectedResult);
			const FTCHARToUTF8 ReverseConvertedValue(ConvertedValue.Get());
			TestTrue(FString(TEXT("Expected test string 4 to have different encoded value")), FCString::Strcmp(TestString, ConvertedValue.Get()) == 0);
			TestTrue(FString(TEXT("Expected test string 4 to have different decoded value")), FCStringAnsi::Strcmp(ExpectedResult, ReverseConvertedValue.Get()) == 0);
		}
	}

	{
		// "Internationalization" Test string
		const TCHAR TestString[] = TEXT("\x0049\x00F1\x0074\x00EB\x0072\x006E\x00E2\x0074\x0069\x00F4\x006E\x00E0\x006C\x0069\x007A\x00E6\x0074\x0069\x00F8\x006E");
		const ANSICHAR ExpectedResult[] = "\x49\xC3\xB1\x74\xC3\xAB\x72\x6E\xC3\xA2\x74\x69\xC3\xB4\x6E\xC3\xA0\x6C\x69\x7A\xC3\xA6\x74\x69\xC3\xB8\x6E";
		{
			const FTCHARToUTF8 ConvertedValue(TestString);
			const FUTF8ToTCHAR ReverseConvertedValue(ConvertedValue.Get());
			TestTrue(FString(TEXT("Expected test string 5 to have different encoded value")), FCStringAnsi::Strcmp(ExpectedResult, ConvertedValue.Get()) == 0);
			TestTrue(FString(TEXT("Expected test string 5 to have different decoded value")), FCString::Strcmp(TestString, ReverseConvertedValue.Get()) == 0);
		}

		{
			const FUTF8ToTCHAR ConvertedValue(ExpectedResult);
			const FTCHARToUTF8 ReverseConvertedValue(ConvertedValue.Get());
			TestTrue(FString(TEXT("Expected test string 6 to have different encoded value")), FCString::Strcmp(TestString, ConvertedValue.Get()) == 0);
			TestTrue(FString(TEXT("Expected test string 6 to have different decoded value")), FCStringAnsi::Strcmp(ExpectedResult, ReverseConvertedValue.Get()) == 0);
		}
	}

	{
		// UE Test string
		const TCHAR TestString[] = TEXT("\xD835\xDE50\x043F\xD835\xDDCB\xD835\xDE26\xD835\xDD52\x006C\x0020\xD835\xDC6C\xD835\xDD2B\xFF47\xD835\xDCBE\xD835\xDD93\xD835\xDD56\x0020\xD835\xDE92\xFF53\x0020\xD835\xDC6E\xD835\xDE9B\x212E\xD835\xDF36\xFF54\x0021");
		const ANSICHAR ExpectedResult[] = "\xF0\x9D\x99\x90\xD0\xBF\xF0\x9D\x97\x8B\xF0\x9D\x98\xA6\xF0\x9D\x95\x92\x6C\x20\xF0\x9D\x91\xAC\xF0\x9D\x94\xAB\xEF\xBD\x87\xF0\x9D\x92\xBE\xF0\x9D\x96\x93\xF0\x9D\x95\x96\x20\xF0\x9D\x9A\x92\xEF\xBD\x93\x20\xF0\x9D\x91\xAE\xF0\x9D\x9A\x9B\xE2\x84\xAE\xF0\x9D\x9C\xB6\xEF\xBD\x94\x21";
		{
			const FTCHARToUTF8 ConvertedValue(TestString);
			const FUTF8ToTCHAR ReverseConvertedValue(ConvertedValue.Get());
			TestTrue(FString(TEXT("Expected test string 7 to have different encoded value")), FCStringAnsi::Strcmp(ExpectedResult, ConvertedValue.Get()) == 0);
			TestTrue(FString(TEXT("Expected test string 7 to have different decoded value")), FCString::Strcmp(TestString, ReverseConvertedValue.Get()) == 0);
		}

		{
			const FUTF8ToTCHAR ConvertedValue(ExpectedResult);
			const FTCHARToUTF8 ReverseConvertedValue(ConvertedValue.Get());
			TestTrue(FString(TEXT("Expected test string 8 to have different encoded value")), FCString::Strcmp(TestString, ConvertedValue.Get()) == 0);
			TestTrue(FString(TEXT("Expected test string 8 to have different decoded value")), FCStringAnsi::Strcmp(ExpectedResult, ReverseConvertedValue.Get()) == 0);
		}
	}

	return !HasAnyErrors();
}

#endif //WITH_DEV_AUTOMATION_TESTS
