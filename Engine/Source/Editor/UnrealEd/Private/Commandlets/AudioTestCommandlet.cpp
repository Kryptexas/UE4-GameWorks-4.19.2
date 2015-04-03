// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "UnrealAudioModule.h"
#include "UnrealAudioTests.h"

DEFINE_LOG_CATEGORY_STATIC(AudioTestCommandlet, Log, All);

#if ENABLE_UNREAL_AUDIO

/** Unreal audio module ptr */
static UAudio::IUnrealAudioModule* UnrealAudioModule = nullptr;

static bool UnrealAudioLoad()
{
	UnrealAudioModule = FModuleManager::LoadModulePtr<UAudio::IUnrealAudioModule>(FName("UnrealAudio"));
	if (UnrealAudioModule)
	{
		UnrealAudioModule->Initialize();
	}
	return UnrealAudioModule != nullptr;
}

static bool UnrealAudioUnload()
{
	if (UnrealAudioModule)
	{
		UnrealAudioModule->Shutdown();
		return true;
	}
	return false;
}

/**
* Test static functions which call into module test code
*/

static bool TestAudioDeviceAll()
{
	if (!UAudio::TestDeviceQuery())
	{
		return false;
	}

	if (!UAudio::TestDeviceOutputSimple(10))
	{
		return false;
	}

	if (!UAudio::TestDeviceOutputRandomizedFm(10))
	{
		return false;
	}

	if (!UAudio::TestDeviceOutputNoisePan(10))
	{
		return false;
	}

	if (!UAudio::TestDeviceInputSimple(10))
	{
		return false;
	}

	if (!UAudio::TestDeviceInputRandomizedDelay(10))
	{
		return false;
	}
	return true;
}

static bool TestAudioDeviceQuery()
{
	return UAudio::TestDeviceQuery();
}

static bool TestAudioDeviceOutputSimple()
{
	return UAudio::TestDeviceOutputSimple(-1);
}

static bool TestAudioDeviceInputSimple()
{
	return UAudio::TestDeviceInputSimple(-1);
}

static bool TestAudioDeviceOutputFm()
{
	return UAudio::TestDeviceOutputRandomizedFm(-1);
}

static bool TestAudioDeviceOutputPan()
{
	return UAudio::TestDeviceOutputNoisePan(-1);
}

static bool TestAudioDeviceInputDelay()
{
	return UAudio::TestDeviceInputRandomizedDelay(-1);
}

/**
* Setting up commandlet code
*/

typedef bool AudioTestFunction();

struct AudioTestInfo
{
	FString CategoryName;
	FString TestName;
	bool (*TestFunction)();
};

enum EAudioTests
{
	AUDIO_TEST_DEVICE_ALL,
	AUDIO_TEST_DEVICE_QUERY,
	AUDIO_TEST_DEVICE_OUTPUT_SIMPLE,
	AUDIO_TEST_DEVICE_OUTPUT_FM,
	AUDIO_TEST_DEVICE_OUTPUT_PAN,
	AUDIO_TEST_DEVICE_INPUT_SIMPLE,
	AUDIO_TEST_DEVICE_INPUT_DELAY,
	AUDIO_TESTS
};

static AudioTestInfo AudioTestInfoList[] =
{
	{ "device", "all",			TestAudioDeviceAll			}, // AUDIO_TEST_DEVICE_ALL
	{ "device", "query",		TestAudioDeviceQuery		}, // AUDIO_TEST_DEVICE_QUERY
	{ "device", "out",			TestAudioDeviceOutputSimple }, // AUDIO_TEST_DEVICE_OUTPUT_SIMPLE
	{ "device", "out_fm",		TestAudioDeviceOutputFm		}, // AUDIO_TEST_DEVICE_OUTPUT_FM
	{ "device", "out_pan",		TestAudioDeviceOutputPan	}, // AUDIO_TEST_DEVICE_OUTPUT_PAN
	{ "device", "in",			TestAudioDeviceInputSimple	}, // AUDIO_TEST_DEVICE_INPUT_SIMPLE
	{ "device", "in_delay",		TestAudioDeviceInputDelay	}, // AUDIO_TEST_DEVICE_INPUT_DELAY
};
static_assert(ARRAY_COUNT(AudioTestInfoList) == AUDIO_TESTS, "Mismatched info list and test enumeration");

static void PrintUsage()
{
	UE_LOG(AudioTestCommandlet, Display, TEXT("AudioTestCommandlet Usage: {Editor}.exe UnrealEd.AudioTestCommandlet {testcategory} {test}"));
	UE_LOG(AudioTestCommandlet, Display, TEXT("Possible Tests: [testcategory]: [test]"));
	for (uint32 Index = 0; Index < AUDIO_TESTS; ++Index)
	{
		UE_LOG(AudioTestCommandlet, Display, TEXT("    %s: %s"), *AudioTestInfoList[Index].CategoryName, *AudioTestInfoList[Index].TestName);
	}
}

#endif // #if ENABLE_UNREAL_AUDIO

// -- UAudioTestCommandlet Functions -------------------

UAudioTestCommandlet::UAudioTestCommandlet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

int32 UAudioTestCommandlet::Main(const FString& InParams)
{
#if ENABLE_UNREAL_AUDIO
	// Parse command line.
	TArray<FString> Tokens;
	TArray<FString> Switches;
	UCommandlet::ParseCommandLine(*InParams, Tokens, Switches);

	if (Tokens.Num() != 3)
	{
		PrintUsage();
		return 0;
	}

	if (!UnrealAudioLoad())
	{
		UE_LOG(AudioTestCommandlet, Display, TEXT("Failed to load unreal audio module. Exiting."));
		return 0;
	}

	check(UnrealAudioModule != nullptr);

	bool FoundTest = false;
	for (uint32 Index = 0; Index < AUDIO_TESTS; ++Index)
	{
		if (AudioTestInfoList[Index].CategoryName == Tokens[1])
		{
			if (AudioTestInfoList[Index].TestName == Tokens[2])
			{
				FoundTest = true;
				if (AudioTestInfoList[Index].TestFunction())
				{
					UE_LOG(AudioTestCommandlet, Display, TEXT("Test %s succeeded."), *AudioTestInfoList[Index].TestName);
				}
				else
				{
					UE_LOG(AudioTestCommandlet, Display, TEXT("Test %s failed."), *AudioTestInfoList[Index].TestName);
				}
				break;
			}
		}
	}

	if (!FoundTest)
	{
		UE_LOG(AudioTestCommandlet, Display, TEXT("Unknown category or test. Exiting."));
		return 0;
	}

	UnrealAudioUnload();
#else
	UE_LOG(AudioTestCommandlet, Display, TEXT("Unreal Audio Module Not Enabled For This Platform"));
#endif // #if ENABLE_UNREAL_AUDIO


	return 0;
}
