// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#include "Internationalization/ICUInternationalization.h"
#include "HAL/FileManager.h"
#include "Misc/ScopeLock.h"
#include "Misc/Paths.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Stats/Stats.h"
#include "Misc/CoreStats.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/App.h"

#if UE_ENABLE_ICU

#include "Internationalization/ICURegex.h"
#include "Internationalization/ICUUtilities.h"
#include "Internationalization/ICUBreakIterator.h"
THIRD_PARTY_INCLUDES_START
	#include <unicode/locid.h>
	#include <unicode/timezone.h>
	#include <unicode/uclean.h>
	#include <unicode/udata.h>
THIRD_PARTY_INCLUDES_END

DEFINE_LOG_CATEGORY_STATIC(LogICUInternationalization, Log, All);

namespace
{
	struct FICUOverrides
	{
#if STATS
		static int64 BytesInUseCount;
		static int64 CachedBytesInUseCount;
#endif

		static void* U_CALLCONV Malloc(const void* context, size_t size)
		{
			void* Result = FMemory::Malloc(size);
#if STATS
			BytesInUseCount += FMemory::GetAllocSize(Result);
			if(FThreadStats::IsThreadingReady() && CachedBytesInUseCount != BytesInUseCount)
			{
				// The amount of startup stats messages for STAT_MemoryICUTotalAllocationSize is about 700k
				// It needs to be replaced with something like this
				//  FEngineLoop.Tick.{ 
				//		FPlatformMemory::UpdateStats();
				//		
				//	And called once per frame.
				//SET_MEMORY_STAT(STAT_MemoryICUTotalAllocationSize, BytesInUseCount);
				CachedBytesInUseCount = BytesInUseCount;
			}
#endif
			return Result;
		}

		static void* U_CALLCONV Realloc(const void* context, void* mem, size_t size)
		{
			return FMemory::Realloc(mem, size);
		}

		static void U_CALLCONV Free(const void* context, void* mem)
		{
#if STATS
			BytesInUseCount -= FMemory::GetAllocSize(mem);
			if(FThreadStats::IsThreadingReady() && CachedBytesInUseCount != BytesInUseCount)
			{
				//SET_MEMORY_STAT(STAT_MemoryICUTotalAllocationSize, BytesInUseCount);
				CachedBytesInUseCount = BytesInUseCount;
			}
#endif
			FMemory::Free(mem);
		}
	};

#if STATS
	int64 FICUOverrides::BytesInUseCount = 0;
	int64 FICUOverrides::CachedBytesInUseCount = 0;
#endif
}

FICUInternationalization::FICUInternationalization(FInternationalization* const InI18N)
	: I18N(InI18N)
{

}

bool FICUInternationalization::Initialize()
{
	UErrorCode ICUStatus = U_ZERO_ERROR;

#if NEEDS_ICU_DLLS
	LoadDLLs();
#endif /*IS_PROGRAM || !IS_MONOLITHIC*/

	u_setMemoryFunctions(NULL, &(FICUOverrides::Malloc), &(FICUOverrides::Realloc), &(FICUOverrides::Free), &(ICUStatus));

	const FString DataDirectoryRelativeToContent = FString(TEXT("Internationalization"));
	IFileManager& FileManager = IFileManager::Get();

	const FString PotentialDataDirectories[] = 
	{
		FPaths::ProjectContentDir() / DataDirectoryRelativeToContent, // Try game content directory.
		FPaths::EngineContentDir() / DataDirectoryRelativeToContent, // Try engine content directory.
	};

	bool HasFoundDataDirectory = false;
	for(const auto& DataDirectoryString : PotentialDataDirectories)
	{
		if( FileManager.DirectoryExists( *DataDirectoryString ) )
		{
			u_setDataDirectory( StringCast<char>( *DataDirectoryString ).Get() );
			HasFoundDataDirectory = true;
			break;
		}
	}

	auto GetPrioritizedDataDirectoriesString = [&]() -> FString
	{
		FString Result;
		for(const auto& DataDirectoryString : PotentialDataDirectories)
		{
			if(!Result.IsEmpty())
			{
				Result += TEXT("\n");
			}
			Result += DataDirectoryString;
		}
		return Result;
	};

	if (!HasFoundDataDirectory)
	{
		UE_LOG(LogICUInternationalization, Fatal, TEXT("ICU data directory was not discovered:\n%s"), *(GetPrioritizedDataDirectoriesString()));
	}

	u_setDataFileFunctions(nullptr, &FICUInternationalization::OpenDataFile, &FICUInternationalization::CloseDataFile, &(ICUStatus));
	u_init(&(ICUStatus));

	FICURegexManager::Create();
	FICUBreakIteratorManager::Create();

	InitializeAvailableCultures();

	bHasInitializedCultureMappings = false;
	ConditionalInitializeCultureMappings();

	bHasInitializedAllowedCultures = false;
	ConditionalInitializeAllowedCultures();

	I18N->InvariantCulture = FindOrMakeCulture(TEXT("en-US-POSIX"), EAllowDefaultCultureFallback::No);
	if (!I18N->InvariantCulture.IsValid())
	{
		I18N->InvariantCulture = FindOrMakeCulture(FString(), EAllowDefaultCultureFallback::Yes);
	}
	I18N->DefaultLanguage = FindOrMakeCulture(FPlatformMisc::GetDefaultLanguage(), EAllowDefaultCultureFallback::Yes);
	I18N->DefaultLocale = FindOrMakeCulture(FPlatformMisc::GetDefaultLocale(), EAllowDefaultCultureFallback::Yes);
	I18N->CurrentLanguage = I18N->DefaultLanguage;
	I18N->CurrentLocale = I18N->DefaultLocale;

	InitializeTimeZone();
	InitializeInvariantGregorianCalendar();

	return U_SUCCESS(ICUStatus) ? true : false;
}

void FICUInternationalization::Terminate()
{
	InvariantGregorianCalendar.Reset();

	FICURegexManager::Destroy();
	FICUBreakIteratorManager::Destroy();
	CachedCultures.Empty();

	u_cleanup();
#if NEEDS_ICU_DLLS
	UnloadDLLs();
#endif //IS_PROGRAM || !IS_MONOLITHIC
}

#if NEEDS_ICU_DLLS
void FICUInternationalization::LoadDLLs()
{
	// The base directory for ICU binaries is consistent on all platforms.
	FString ICUBinariesRoot = FPaths::EngineDir() / TEXT("Binaries") / TEXT("ThirdParty") / TEXT("ICU") / TEXT("icu4c-53_1");

#if PLATFORM_WINDOWS
#if PLATFORM_64BITS
	const FString PlatformFolderName = TEXT("Win64");
#elif PLATFORM_32BITS
	const FString PlatformFolderName = TEXT("Win32");
#endif //PLATFORM_*BITS

#if _MSC_VER >= 1900
	const FString VSVersionFolderName = TEXT("VS2015");
#else
	#error "FICUInternationalization::LoadDLLs - Unknown _MSC_VER! Please update this code for this version of MSVC."
#endif //_MSVC_VER

	// Windows requires support for 32/64 bit and different MSVC runtimes.
	const FString TargetSpecificPath = ICUBinariesRoot / PlatformFolderName / VSVersionFolderName;

	// Windows libraries use a specific naming convention.
	FString LibraryNameStems[] =
	{
		TEXT("dt"),   // Data
		TEXT("uc"),   // Unicode Common
		TEXT("in"),   // Internationalization
		TEXT("le"),   // Layout Engine
		TEXT("lx"),   // Layout Extensions
		TEXT("io")	// Input/Output
	};
#else
	// Non-Windows libraries use a consistent naming convention.
	FString LibraryNameStems[] =
	{
		TEXT("data"),   // Data
		TEXT("uc"),   // Unicode Common
		TEXT("i18n"),   // Internationalization
		TEXT("le"),   // Layout Engine
		TEXT("lx"),   // Layout Extensions
		TEXT("io")	// Input/Output
	};
#if PLATFORM_LINUX
	const FString TargetSpecificPath = ICUBinariesRoot / TEXT("Linux") / TEXT("x86_64-unknown-linux-gnu");
#elif PLATFORM_MAC
	const FString TargetSpecificPath = ICUBinariesRoot / TEXT("Mac");
#endif //PLATFORM_*
#endif //PLATFORM_*

#if UE_BUILD_DEBUG && !defined(NDEBUG)
	const FString LibraryNamePostfix = TEXT("d");
#else
	const FString LibraryNamePostfix = TEXT("");
#endif //DEBUG

	for(FString Stem : LibraryNameStems)
	{
#if PLATFORM_WINDOWS
		FString LibraryName = "icu" + Stem + LibraryNamePostfix + "53" "." "dll";
#elif PLATFORM_LINUX
		FString LibraryName = "lib" "icu" + Stem + LibraryNamePostfix + ".53.1" "." "so";
#elif PLATFORM_MAC
		FString LibraryName = "lib" "icu" + Stem + ".53.1" + LibraryNamePostfix + "." "dylib";
#endif //PLATFORM_*

		void* DLLHandle = FPlatformProcess::GetDllHandle(*(TargetSpecificPath / LibraryName));
		check(DLLHandle != nullptr);
		DLLHandles.Add(DLLHandle);
	}
}

void FICUInternationalization::UnloadDLLs()
{
	for( const auto DLLHandle : DLLHandles )
	{
		FPlatformProcess::FreeDllHandle(DLLHandle);
	}
	DLLHandles.Reset();
}
#endif // NEEDS_ICU_DLLS

#if STATS
namespace
{
	int64 DataFileBytesInUseCount = 0;
	int64 CachedDataFileBytesInUseCount = 0;
}
#endif

void FICUInternationalization::LoadAllCultureData()
{
	for (const FICUCultureData& CultureData : AllAvailableCultures)
	{
		FindOrMakeCulture(CultureData.Name, EAllowDefaultCultureFallback::No);
	}
}

void FICUInternationalization::InitializeAvailableCultures()
{
	// Build up the data about all available locales
	int32_t LocaleCount;
	const icu::Locale* const AvailableLocales = icu::Locale::getAvailableLocales(LocaleCount);

	AllAvailableCultures.Reserve(LocaleCount);
	AllAvailableCulturesMap.Reserve(LocaleCount);

	auto AppendCultureData = [&](const FString& InLanguageCode, const FString& InScriptCode, const FString& InCountryCode)
	{
		FString CultureName = InLanguageCode;
		if (!InScriptCode.IsEmpty())
		{
			CultureName.AppendChar(TEXT('-'));
			CultureName.Append(InScriptCode);
		}
		if (!InCountryCode.IsEmpty())
		{
			CultureName.AppendChar(TEXT('-'));
			CultureName.Append(InCountryCode);
		}

		if (AllAvailableCulturesMap.Contains(CultureName))
		{
			return;
		}

		const int32 CultureDataIndex = AllAvailableCultures.AddDefaulted();
		AllAvailableCulturesMap.Add(CultureName, CultureDataIndex);

		TArray<int32>& CulturesForLanguage = AllAvailableLanguagesToSubCulturesMap.FindOrAdd(InLanguageCode);
		CulturesForLanguage.Add(CultureDataIndex);

		FICUCultureData& CultureData = AllAvailableCultures[CultureDataIndex];
		CultureData.Name = CultureName;
		CultureData.LanguageCode = InLanguageCode;
		CultureData.ScriptCode = InScriptCode;
		CultureData.CountryCode = InCountryCode;
	};

	for (int32 i = 0; i < LocaleCount; ++i)
	{
		const icu::Locale& Locale = AvailableLocales[i];

		const FString LanguageCode = Locale.getLanguage();
		const FString ScriptCode = Locale.getScript();
		const FString CountryCode = Locale.getCountry();

		// AvailableLocales doesn't always contain all variations of a culture, so we try and add them all here
		// This allows the culture script look-up in GetPrioritizedCultureNames to work without having to load up culture data most of the time
		AppendCultureData(LanguageCode, FString(), FString());
		if (!CountryCode.IsEmpty())
		{
			AppendCultureData(LanguageCode, FString(), CountryCode);
		}
		if (!ScriptCode.IsEmpty())
		{
			AppendCultureData(LanguageCode, ScriptCode, FString());
		}
		if (!ScriptCode.IsEmpty() && !CountryCode.IsEmpty())
		{
			AppendCultureData(LanguageCode, ScriptCode, CountryCode);
		}
	}

	// Also add our invariant culture if it wasn't found when processing the ICU locales
	if (!AllAvailableCulturesMap.Contains(TEXT("en-US-POSIX")))
	{
		AppendCultureData(TEXT("en"), FString(), TEXT("US-POSIX"));
	}
}

TArray<FString> LoadInternationalizationConfigArray(const TCHAR* InKey)
{
	check(GConfig && GConfig->IsReadyForUse());

	const bool ShouldLoadEditor = GIsEditor;
	const bool ShouldLoadGame = FApp::IsGame();

	TArray<FString> FinalArray;
	GConfig->GetArray(TEXT("Internationalization"), InKey, FinalArray, GEngineIni);

	if (ShouldLoadEditor)
	{
		TArray<FString> EditorArray;
		GConfig->GetArray(TEXT("Internationalization"), InKey, EditorArray, GEditorIni);
		FinalArray.Append(MoveTemp(EditorArray));
	}

	if (ShouldLoadGame)
	{
		TArray<FString> GameArray;
		GConfig->GetArray(TEXT("Internationalization"), InKey, GameArray, GGameIni);
		FinalArray.Append(MoveTemp(GameArray));
	}

	return FinalArray;
}

void FICUInternationalization::ConditionalInitializeCultureMappings()
{
	if (bHasInitializedCultureMappings || !GConfig || !GConfig->IsReadyForUse())
	{
		return;
	}

	bHasInitializedCultureMappings = true;

	const TArray<FString> CultureMappingsArray = LoadInternationalizationConfigArray(TEXT("CultureMappings"));

	// An array of semicolon separated mapping entries: SourceCulture;DestCulture
	CultureMappings.Reserve(CultureMappingsArray.Num());
	for (const FString& CultureMappingStr : CultureMappingsArray)
	{
		FString SourceCulture;
		FString DestCulture;
		if (CultureMappingStr.Split(TEXT(";"), &SourceCulture, &DestCulture, ESearchCase::CaseSensitive))
		{
			if (AllAvailableCulturesMap.Contains(DestCulture))
			{
				CultureMappings.Add(MoveTemp(SourceCulture), MoveTemp(DestCulture));
			}
			else
			{
				UE_LOG(LogICUInternationalization, Warning, TEXT("Culture mapping '%s' contains an unknown culture and has been ignored."), *CultureMappingStr);
			}
		}
	}
	CultureMappings.Compact();
}

void FICUInternationalization::ConditionalInitializeAllowedCultures()
{
	if (bHasInitializedAllowedCultures || !GConfig || !GConfig->IsReadyForUse())
	{
		return;
	}

	bHasInitializedAllowedCultures = true;

	// Get our current build config string so we can compare it against the config entries
	FString BuildConfigString;
	{
		EBuildConfigurations::Type BuildConfig = FApp::GetBuildConfiguration();
		if (BuildConfig == EBuildConfigurations::DebugGame)
		{
			// Treat DebugGame and Debug as the same for loc purposes
			BuildConfig = EBuildConfigurations::Debug;
		}

		if (BuildConfig != EBuildConfigurations::Unknown)
		{
			BuildConfigString = EBuildConfigurations::ToString(BuildConfig);
		}
	}

	// An array of potentially semicolon separated mapping entries: Culture[;BuildConfig[,BuildConfig,BuildConfig]]
	// No build config(s) implies all build configs
	auto ProcessCulturesArray = [this, &BuildConfigString](const TArray<FString>& InCulturesArray, TSet<FString>& OutCulturesSet)
	{
		OutCulturesSet.Reserve(InCulturesArray.Num());
		for (const FString& CultureStr : InCulturesArray)
		{
			FString CultureName;
			FString CultureBuildConfigsStr;
			if (CultureStr.Split(TEXT(";"), &CultureName, &CultureBuildConfigsStr, ESearchCase::CaseSensitive))
			{
				// Check to see if any of the build configs matches our current build config
				TArray<FString> CultureBuildConfigs;
				if (CultureBuildConfigsStr.ParseIntoArray(CultureBuildConfigs, TEXT(",")))
				{
					bool bIsValidBuildConfig = false;
					for (const FString& CultureBuildConfig : CultureBuildConfigs)
					{
						if (BuildConfigString == CultureBuildConfig)
						{
							bIsValidBuildConfig = true;
							break;
						}
					}

					if (!bIsValidBuildConfig)
					{
						continue;
					}
				}
			}
			else
			{
				CultureName = CultureStr;
			}

			if (AllAvailableCulturesMap.Contains(CultureName))
			{
				OutCulturesSet.Add(MoveTemp(CultureName));
			}
			else
			{
				UE_LOG(LogICUInternationalization, Warning, TEXT("Culture '%s' is unknown and has been ignored when parsing the enabled/disabled culture list."), *CultureName);
			}
		}
		OutCulturesSet.Compact();
	};

	const TArray<FString> EnabledCulturesArray = LoadInternationalizationConfigArray(TEXT("EnabledCultures"));
	ProcessCulturesArray(EnabledCulturesArray, EnabledCultures);

	const TArray<FString> DisabledCulturesArray = LoadInternationalizationConfigArray(TEXT("DisabledCultures"));
	ProcessCulturesArray(DisabledCulturesArray, DisabledCultures);
}

bool FICUInternationalization::IsCultureRemapped(const FString& Name, FString* OutMappedCulture)
{
	// Make sure we've loaded the culture mappings (the config system may not have been available when we were first initialized)
	ConditionalInitializeCultureMappings();

	// Check to see if the culture has been re-mapped
	const FString* const MappedCulture = CultureMappings.Find(Name);
	if (MappedCulture && OutMappedCulture)
	{
		*OutMappedCulture = *MappedCulture;
	}

	return MappedCulture != nullptr;
}

bool FICUInternationalization::IsCultureAllowed(const FString& Name)
{
	// Make sure we've loaded the allowed cultures lists (the config system may not have been available when we were first initialized)
	ConditionalInitializeAllowedCultures();

	return (EnabledCultures.Num() == 0 || EnabledCultures.Contains(Name)) && !DisabledCultures.Contains(Name);
}

void FICUInternationalization::HandleLanguageChanged(const FString& Name)
{
	UErrorCode ICUStatus = U_ZERO_ERROR;
	uloc_setDefault(StringCast<char>(*Name).Get(), &ICUStatus);

	// Update the cached display names in any existing cultures
	FScopeLock Lock(&CachedCulturesCS);
	for (const auto& CachedCulturePair : CachedCultures)
	{
		CachedCulturePair.Value->HandleCultureChanged();
	}
}

void FICUInternationalization::GetCultureNames(TArray<FString>& CultureNames) const
{
	CultureNames.Reset(AllAvailableCultures.Num());
	for (const FICUCultureData& CultureData : AllAvailableCultures)
	{
		CultureNames.Add(CultureData.Name);
	}
}

TArray<FString> FICUInternationalization::GetPrioritizedCultureNames(const FString& Name)
{
	auto PopulateCultureData = [&](const FString& InCultureName, FICUCultureData& OutCultureData) -> bool
	{
		// First, try and find the data in the map (although it seems that not all data is in here)
		const int32* CultureDataIndexPtr = AllAvailableCulturesMap.Find(InCultureName);
		if (CultureDataIndexPtr)
		{
			OutCultureData = AllAvailableCultures[*CultureDataIndexPtr];
			return true;
		}
		
		// Failing that, try and find the culture directly (this will cause its resource data to be loaded)
		FCulturePtr Culture = FindOrMakeCulture(InCultureName, EAllowDefaultCultureFallback::No);
		if (Culture.IsValid())
		{
			OutCultureData.Name = Culture->GetName();
			OutCultureData.LanguageCode = Culture->GetTwoLetterISOLanguageName();
			OutCultureData.ScriptCode = Culture->GetScript();
			OutCultureData.CountryCode = Culture->GetRegion();
			return true;
		}

		return false;
	};

	// Apply any culture remapping
	FString GivenCulture;
	if (!IsCultureRemapped(Name, &GivenCulture))
	{
		GivenCulture = Name;
	}

	TArray<FString> PrioritizedCultureNames;

	FICUCultureData GivenCultureData;
	if (PopulateCultureData(GivenCulture, GivenCultureData))
	{
		// If we have a culture without a script, but with a country code, we can try and work out the script for the country code by enumerating all of the available cultures
		// and looking for a matching culture with a script set (eg, "zh-CN" would find "zh-Hans-CN")
		TArray<FICUCultureData> ParentCultureData;
		if (GivenCultureData.ScriptCode.IsEmpty() && !GivenCultureData.CountryCode.IsEmpty())
		{
			const TArray<int32>* const CulturesForLanguage = AllAvailableLanguagesToSubCulturesMap.Find(GivenCultureData.LanguageCode);
			if (CulturesForLanguage)
			{
				for (const int32 CultureIndex : *CulturesForLanguage)
				{
					const FICUCultureData& CultureData = AllAvailableCultures[CultureIndex];
					if (!CultureData.ScriptCode.IsEmpty() && GivenCultureData.CountryCode == CultureData.CountryCode)
					{
						ParentCultureData.Add(CultureData);
					}
				}
			}
		}
		
		if (ParentCultureData.Num() == 0)
		{
			ParentCultureData.Add(GivenCultureData);
		}

		TArray<FICUCultureData> PrioritizedCultureData;

		PrioritizedCultureData.Reserve(ParentCultureData.Num() * 3);
		for (const FICUCultureData& CultureData : ParentCultureData)
		{
			const TArray<FString> PrioritizedParentCultures = FCulture::GetPrioritizedParentCultureNames(CultureData.LanguageCode, CultureData.ScriptCode, CultureData.CountryCode);
			for (const FString& PrioritizedParentCultureName : PrioritizedParentCultures)
			{
				FICUCultureData PrioritizedParentCultureData;
				if (PopulateCultureData(PrioritizedParentCultureName, PrioritizedParentCultureData))
				{
					PrioritizedCultureData.AddUnique(PrioritizedParentCultureData);
				}
			}
		}

		// Sort the cultures by their priority
		// Special case handling for the ambiguity of Hong Kong and Macau supporting both Traditional and Simplified Chinese (prefer Traditional)
		const bool bPreferTraditionalChinese = GivenCultureData.CountryCode == TEXT("HK") || GivenCultureData.CountryCode == TEXT("MO");
		PrioritizedCultureData.Sort([bPreferTraditionalChinese](const FICUCultureData& DataOne, const FICUCultureData& DataTwo) -> bool
		{
			const int32 DataOneSortWeight = (DataOne.CountryCode.IsEmpty() ? 0 : 4) + (DataOne.ScriptCode.IsEmpty() ? 0 : 2) + (bPreferTraditionalChinese && DataOne.ScriptCode == TEXT("Hant") ? 1 : 0);
			const int32 DataTwoSortWeight = (DataTwo.CountryCode.IsEmpty() ? 0 : 4) + (DataTwo.ScriptCode.IsEmpty() ? 0 : 2) + (bPreferTraditionalChinese && DataTwo.ScriptCode == TEXT("Hant") ? 1 : 0);
			return DataOneSortWeight >= DataTwoSortWeight;
		});

		PrioritizedCultureNames.Reserve(PrioritizedCultureData.Num());
		for (const FICUCultureData& CultureData : PrioritizedCultureData)
		{
			PrioritizedCultureNames.Add(CultureData.Name);
		}
	}

	// Remove any cultures that are explicitly disallowed
	PrioritizedCultureNames.RemoveAll([&](const FString& InPrioritizedCultureName) -> bool
	{
		return !IsCultureAllowed(InPrioritizedCultureName);
	});

	// If we have no cultures, fallback to using English
	if (PrioritizedCultureNames.Num() == 0)
	{
		PrioritizedCultureNames.Add(TEXT("en"));
	}

	return PrioritizedCultureNames;
}

FCulturePtr FICUInternationalization::GetCulture(const FString& Name)
{
	return FindOrMakeCulture(Name, EAllowDefaultCultureFallback::No);
}

FCulturePtr FICUInternationalization::FindOrMakeCulture(const FString& Name, const EAllowDefaultCultureFallback AllowDefaultFallback)
{
	const FString CanonicalName = FCulture::GetCanonicalName(Name);

	// Find the cached culture.
	{
		FScopeLock Lock(&CachedCulturesCS);
		FCultureRef* FoundCulture = CachedCultures.Find(CanonicalName);
		if (FoundCulture)
		{
			return *FoundCulture;
		}
	}

	// If no cached culture is found, try to make one.
	FCulturePtr NewCulture;

	// Is this in our list of available cultures?
	if (AllAvailableCulturesMap.Contains(CanonicalName))
	{
		NewCulture = FCulture::Create(CanonicalName);
	}
	else
	{
		// We need to use a resource load in order to get the correct culture
		UErrorCode ICUStatus = U_ZERO_ERROR;
		if (UResourceBundle* ICUResourceBundle = ures_open(nullptr, StringCast<char>(*CanonicalName).Get(), &ICUStatus))
		{
			if (ICUStatus != U_USING_DEFAULT_WARNING || AllowDefaultFallback == EAllowDefaultCultureFallback::Yes)
			{
				NewCulture = FCulture::Create(CanonicalName);
			}
			ures_close(ICUResourceBundle);
		}
	}

	if (NewCulture.IsValid())
	{
		FScopeLock Lock(&CachedCulturesCS);
		CachedCultures.Add(CanonicalName, NewCulture.ToSharedRef());
	}

	return NewCulture;
}

void FICUInternationalization::InitializeTimeZone()
{
	const FString TimeZoneId = FPlatformMisc::GetTimeZoneId();

	icu::TimeZone* ICUDefaultTz = TimeZoneId.IsEmpty() ? icu::TimeZone::createDefault() : icu::TimeZone::createTimeZone(ICUUtilities::ConvertString(TimeZoneId));
	icu::TimeZone::adoptDefault(ICUDefaultTz);

	const int32 DefaultTzOffsetMinutes = ICUDefaultTz->getRawOffset() / 60000;
	const int32 RawOffsetHours = DefaultTzOffsetMinutes / 60;
	const int32 RawOffsetMinutes = DefaultTzOffsetMinutes % 60;
	UE_LOG(LogICUInternationalization, Display, TEXT("ICU TimeZone Detection - Raw Offset: %+d:%02d, Platform Override: '%s'"), RawOffsetHours, RawOffsetMinutes, *TimeZoneId);
}

void FICUInternationalization::InitializeInvariantGregorianCalendar()
{
	UErrorCode ICUStatus = U_ZERO_ERROR;
	InvariantGregorianCalendar = MakeUnique<icu::GregorianCalendar>(ICUStatus);
	InvariantGregorianCalendar->setTimeZone(icu::TimeZone::getUnknown());
}

UDate FICUInternationalization::UEDateTimeToICUDate(const FDateTime& DateTime)
{
	// UE4 and ICU have a different time scale for pre-Gregorian dates, so we can't just use the UNIX timestamp from the UE4 DateTime
	// Instead we have to explode the UE4 DateTime into its component parts, and then use an ICU GregorianCalendar (set to the "unknown" 
	// timezone so it doesn't apply any adjustment to the time) to reconstruct the DateTime as an ICU UDate in the correct scale
	int32 Year, Month, Day;
	DateTime.GetDate(Year, Month, Day);
	const int32 Hour = DateTime.GetHour();
	const int32 Minute = DateTime.GetMinute();
	const int32 Second = DateTime.GetSecond();

	{
		FScopeLock Lock(&InvariantGregorianCalendarCS);

		InvariantGregorianCalendar->set(Year, Month - 1, Day, Hour, Minute, Second);
		
		UErrorCode ICUStatus = U_ZERO_ERROR;
		return InvariantGregorianCalendar->getTime(ICUStatus);
	}
}

UBool FICUInternationalization::OpenDataFile(const void* context, void** fileContext, void** contents, const char* path)
{
	const FString PathStr = StringCast<TCHAR>(path).Get();
	auto& PathToCachedFileDataMap = FInternationalization::Get().Implementation->PathToCachedFileDataMap;

	// Try to find existing buffer
	FICUInternationalization::FICUCachedFileData* CachedFileData = PathToCachedFileDataMap.Find(PathStr);

	// If there's no file context, we might have to load the file.
	if (!CachedFileData)
	{
#if !UE_BUILD_SHIPPING
		FScopedLoadingState ScopedLoadingState(*PathStr);
#endif

		// Attempt to load the file.
		FArchive* FileAr = IFileManager::Get().CreateFileReader(*PathStr);
		if (FileAr)
		{
			const int64 FileSize = FileAr->TotalSize();

			// Create file data.
			CachedFileData = &(PathToCachedFileDataMap.Emplace(PathStr, FICUInternationalization::FICUCachedFileData(FileSize)));

			// Load file into buffer.
			FileAr->Serialize(CachedFileData->Buffer, FileSize); 
			delete FileAr;

			// Stat tracking.
#if STATS
			DataFileBytesInUseCount += FMemory::GetAllocSize(CachedFileData->Buffer);
			if (FThreadStats::IsThreadingReady() && CachedDataFileBytesInUseCount != DataFileBytesInUseCount)
			{
				SET_MEMORY_STAT(STAT_MemoryICUDataFileAllocationSize, DataFileBytesInUseCount);
				CachedDataFileBytesInUseCount = DataFileBytesInUseCount;
			}
#endif
		}
	}

	// Add a reference, either the initial one or an additional one.
	if (CachedFileData)
	{
		++(CachedFileData->ReferenceCount);
	}

	// Use the file path as the context, so we can look up the cached file data later and decrement its reference count.
	*fileContext = CachedFileData ? new FString(PathStr) : nullptr;

	// Use the buffer from the cached file data.
	*contents = CachedFileData ? CachedFileData->Buffer : nullptr;

	// If we have cached file data, we must have loaded new data or found existing data, so we've successfully "opened" and "read" the file into "contents".
	return CachedFileData != nullptr;
}

void FICUInternationalization::CloseDataFile(const void* context, void* const fileContext, void* const contents)
{
	// Early out on null context.
	if (fileContext == nullptr)
	{
		return;
	}

	auto& PathToCachedFileDataMap = FInternationalization::Get().Implementation->PathToCachedFileDataMap;

	// The file context is the path to the file.
	FString* const Path = reinterpret_cast<FString*>(fileContext);
	check(Path);

	// Look up the cached file data so we can maintain references.
	FICUInternationalization::FICUCachedFileData* const CachedFileData = PathToCachedFileDataMap.Find(*Path);
	check(CachedFileData);
	check(CachedFileData->Buffer == contents);

	// Remove a reference.
	--(CachedFileData->ReferenceCount);

	// If the last reference has been removed, the cached file data is not longer needed.
	if (CachedFileData->ReferenceCount == 0)
	{
		// Stat tracking.
#if STATS
		DataFileBytesInUseCount -= FMemory::GetAllocSize(CachedFileData->Buffer);
		if (FThreadStats::IsThreadingReady() && CachedDataFileBytesInUseCount != DataFileBytesInUseCount)
		{
			SET_MEMORY_STAT(STAT_MemoryICUDataFileAllocationSize, DataFileBytesInUseCount);
			CachedDataFileBytesInUseCount = DataFileBytesInUseCount;
		}
#endif

		// Delete the cached file data.
		PathToCachedFileDataMap.Remove(*Path);
	}

	// The path string we allocated for tracking is no longer necessary.
	delete Path;
}

FICUInternationalization::FICUCachedFileData::FICUCachedFileData(const int64 FileSize)
	: ReferenceCount(0)
	, Buffer( FICUOverrides::Malloc(nullptr, FileSize) )
{
}

FICUInternationalization::FICUCachedFileData::FICUCachedFileData(const FICUCachedFileData& Source)
{
	checkf(false, TEXT("Cached file data for ICU may not be copy constructed. Something is trying to copy construct FICUCachedFileData."));
}

FICUInternationalization::FICUCachedFileData::FICUCachedFileData(FICUCachedFileData&& Source)
	: ReferenceCount(Source.ReferenceCount)
	, Buffer( Source.Buffer )
{
	// Make sure the moved source object doesn't retain the pointer and free the memory we now point to.
	Source.Buffer = nullptr;
}

FICUInternationalization::FICUCachedFileData::~FICUCachedFileData()
{
	if (Buffer)
	{
		check(ReferenceCount == 0);
		FICUOverrides::Free(nullptr, Buffer);
	}
}

#endif
