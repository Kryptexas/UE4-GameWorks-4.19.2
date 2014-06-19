// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "InvariantCulture.h"
#if ENABLE_LOC_TESTING
#include "LEETCulture.h"
#endif
#include "AmericanEnglish.h"
// this header has hardcoded 2byte characters that won't work with 1 byte TCHARs
#if !PLATFORM_TCHAR_IS_1_BYTE
#include "IndiaHindi.h"
#include "JapaneseCulture.h"
#include "KoreanCulture.h"
#endif

#if UE_ENABLE_ICU
#include "ICUUtilities.h"
#include <unicode/locid.h>
#include <unicode/timezone.h>
#include <unicode/uclean.h>
#include <unicode/udata.h>
#endif

#define LOCTEXT_NAMESPACE "Internationalization"

namespace
{

FString GetCanonicalName(const FString& Name)
{
#if UE_ENABLE_ICU
	static const int32 MaximumNameLength = 64;
	const int32 NameLength = Name.Len();
	check(NameLength < MaximumNameLength);
	char CanonicalName[MaximumNameLength];

	UErrorCode ICUStatus = U_ZERO_ERROR;
	uloc_canonicalize(TCHAR_TO_ANSI( *Name ), CanonicalName, MaximumNameLength, &ICUStatus);
	return CanonicalName;
#else
	return Name;
#endif
}

}

FInternationalization* FInternationalization::Instance;

FInternationalization& FInternationalization::Get()
{
	if(!Instance)
	{
		Instance = new FInternationalization();
	}
	if(Instance && !Instance->IsInitialized())
	{
		Instance->Initialize();
	}
	return *Instance;
}

void FInternationalization::TearDown()
{
	if (Instance && Instance->IsInitialized())
	{
		Instance->Terminate();
	}
}

void FInternationalization::GetTimeZonesIDs(TArray<FString>& TimeZonesIDs) const
{
	TimeZonesIDs.Empty();
#if UE_ENABLE_ICU
	UErrorCode ICUStatus = U_ZERO_ERROR;

	TSharedPtr<icu::StringEnumeration> StringEnumeration( icu::TimeZone::createEnumeration() );
	TimeZonesIDs.Reserve( StringEnumeration->count(ICUStatus) );

	const icu::UnicodeString* ICUString;
	do
	{
		ICUString = StringEnumeration->snext(ICUStatus);
		if(ICUString)
		{
			FString NativeString;
			ICUUtilities::Convert(*ICUString, NativeString);
			TimeZonesIDs.Add( NativeString );
		}
	} while( ICUString );
#endif
}

void FInternationalization::SetCurrentCulture(const FString& Name)
{
	const int32 CultureIndex = GetCultureIndex(Name);

	if(CultureIndex != INDEX_NONE && CurrentCultureIndex != CultureIndex)
	{
		CurrentCultureIndex = CultureIndex;

#if UE_ENABLE_ICU
		UErrorCode ICUStatus = U_ZERO_ERROR;
		icu::Locale::setDefault(icu::Locale( TCHAR_TO_ANSI(*Name) ), ICUStatus);
#endif

		FCoreDelegates::OnCultureChanged.Broadcast();
	}
}

TSharedRef<FCulture, ESPMode::ThreadSafe> FInternationalization::GetCurrentCulture() const
{
	return AllCultures[CurrentCultureIndex];
}

TSharedPtr<FCulture, ESPMode::ThreadSafe> FInternationalization::GetCulture(const FString& Name) const
{
	int32 CultureIndex = GetCultureIndex(Name);
	return CultureIndex != -1 ? AllCultures[CultureIndex] : TSharedPtr<FCulture, ESPMode::ThreadSafe>();
}

int32 FInternationalization::GetCultureIndex(const FString& Name) const
{
	const FString CanonicalName = GetCanonicalName(Name);

	const int32 CultureCount = AllCultures.Num();
	int32 i;
	for (i = 0; i < CultureCount; ++i)
	{
		if( AllCultures[i]->GetName() == CanonicalName )
		{
			break;
		}
	}
	if(i >= CultureCount)
	{
		i = -1;
	}
	return i;
}

#if UE_ENABLE_ICU
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
				SET_MEMORY_STAT(STAT_MemoryICUTotalAllocationSize, BytesInUseCount);
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
				SET_MEMORY_STAT(STAT_MemoryICUTotalAllocationSize, BytesInUseCount);
				CachedBytesInUseCount = BytesInUseCount;
			}
#endif
			FMemory::Free(mem);
		}
	};

#if STATS
	int64 FICUOverrides::BytesInUseCount = 0;
	int64 FICUOverrides::CachedBytesInUseCount = 0;
	static int64 DataFileBytesInUseCount = 0;
	static int64 CachedDataFileBytesInUseCount = 0;
#endif

	UBool OpenDataFile(const void* context, void** fileContext, void** contents, const char* path)
	{
		FArchive* FileAr = IFileManager::Get().CreateFileReader(StringCast<TCHAR>(path).Get());
		if(!FileAr)
		{
			return FALSE;
		}

		int64 FileSize = FileAr->TotalSize();

		*fileContext = FileAr;
		*contents = FICUOverrides::Malloc(nullptr, FileSize);

#if STATS
		DataFileBytesInUseCount += FMemory::GetAllocSize(*contents);
		if(FThreadStats::IsThreadingReady() && CachedDataFileBytesInUseCount != DataFileBytesInUseCount)
		{
			SET_MEMORY_STAT(STAT_MemoryICUDataFileAllocationSize, DataFileBytesInUseCount);
			CachedDataFileBytesInUseCount = DataFileBytesInUseCount;
		}
#endif

		FileAr->Serialize(*contents, FileSize); 

		FileAr->Close(); // Close the file archive to free up the handle - it is unneeded since the data is in memory.

		return TRUE;
	}

	void CloseDataFile(const void* context, void* const fileContext, void* const contents)
	{
		if(fileContext)
		{
			if(contents)
			{
#if STATS
				DataFileBytesInUseCount -= FMemory::GetAllocSize(contents);
				if(FThreadStats::IsThreadingReady() && CachedDataFileBytesInUseCount != DataFileBytesInUseCount)
				{
					SET_MEMORY_STAT(STAT_MemoryICUDataFileAllocationSize, DataFileBytesInUseCount);
					CachedDataFileBytesInUseCount = DataFileBytesInUseCount;
				}
#endif
				FICUOverrides::Free(nullptr, contents);
			}
		}
	}
}
#endif

void FInternationalization::Initialize()
{
	static bool IsInitializing = false;

	if(IsInitialized() || IsInitializing)
	{
		return;
	}
	struct FInitializingGuard
	{
		FInitializingGuard()	{IsInitializing = true;}
		~FInitializingGuard()	{IsInitializing = false;}
	} InitializingGuard;

#if UE_ENABLE_ICU
	UErrorCode ICUStatus = U_ZERO_ERROR;

#if IS_PROGRAM || !IS_MONOLITHIC
	LoadDLLs();
#endif /*IS_PROGRAM || !IS_MONOLITHIC*/

	u_setMemoryFunctions(NULL, &(FICUOverrides::Malloc), &(FICUOverrides::Realloc), &(FICUOverrides::Free), &(ICUStatus));

	FString DataDirectory;
	const FString DataDirectoryRelativeToContent = FString(TEXT("Localization")) / FString(TEXT("ICU"));
	IPlatformFile& PlatformFile = IPlatformFile::GetPlatformPhysical();
	DataDirectory = FPaths::GameContentDir() / DataDirectoryRelativeToContent; // Try game content directory.
	if( !PlatformFile.DirectoryExists(*DataDirectory) )
	{
		DataDirectory = FString(FPlatformProcess::BaseDir()) / FPaths::GameContentDir() / DataDirectoryRelativeToContent; // Try game content directory with appropriate base.
	}
	if( !PlatformFile.DirectoryExists(*DataDirectory) )
	{
		FString DataDirectory = FPaths::EngineContentDir() / DataDirectoryRelativeToContent; // Try engine content directory.
	}
	if( !PlatformFile.DirectoryExists(*DataDirectory) )
	{
		DataDirectory = FString(FPlatformProcess::BaseDir()) / FPaths::EngineContentDir() / DataDirectoryRelativeToContent; // Try engine content directory with appropriate base.
	}
	check( PlatformFile.DirectoryExists(*DataDirectory) );
	u_setDataDirectory(StringCast<char>(*DataDirectory).Get());

	u_setDataFileFunctions(nullptr, &OpenDataFile, &CloseDataFile, &(ICUStatus));
	u_init(&(ICUStatus));
#endif /*UE_ENABLE_ICU*/

	PopulateAllCultures();

#if UE_ENABLE_ICU
	SetCurrentCulture( GetDefaultCulture()->GetName() );
#else
	SetCurrentCulture( TEXT("") );
#endif

#if UE_ENABLE_ICU
	bIsInitialized = U_SUCCESS(ICUStatus) ? true : false;
#else
	bIsInitialized = true;
#endif
}

void FInternationalization::Terminate()
{
	DefaultCulture.Reset();
	InvariantCulture.Reset();
	AllCultures.Empty();
	bIsInitialized = false;

#if UE_ENABLE_ICU
	u_cleanup();
#if IS_PROGRAM || !IS_MONOLITHIC
	UnloadDLLs();
#endif //IS_PROGRAM || !IS_MONOLITHIC
#endif //UE_ENABLE_ICU

	delete Instance;
	Instance = nullptr;
}

#if UE_ENABLE_ICU && (IS_PROGRAM || !IS_MONOLITHIC)
void FInternationalization::LoadDLLs()
{
	// The base directory for ICU binaries is consistent on all platforms.
	FString ICUBinariesRoot = FPaths::EngineDir() / TEXT("Binaries") / TEXT("ThirdParty") / TEXT("ICU") / TEXT("icu4c-53_1");

#if PLATFORM_WINDOWS
#if PLATFORM_64BITS
	const FString PlatformFolderName = TEXT("Win64");
#elif PLATFORM_32BITS
	const FString PlatformFolderName = TEXT("Win32");
#endif //PLATFORM_*BITS

#if _MSC_VER >= 1800
	const FString VSVersionFolderName = TEXT("VS2013");
#else
	const FString VSVersionFolderName = TEXT("VS2012");
#endif //_MSVC_VER

	// Windows requires support for 32/64 bit and also VC11 and VC12 runtimes.
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
		FString LibraryName = "lib" "icu" + Stem + LibraryNamePostfix + ".53.1" "." "dylib";
#endif //PLATFORM_*

		void* DLLHandle = FPlatformProcess::GetDllHandle(*(TargetSpecificPath / LibraryName));
		check(DLLHandle != nullptr);
		DLLHandles.Add(DLLHandle);
	}
}

void FInternationalization::UnloadDLLs()
{
	for( const auto DLLHandle : DLLHandles )
	{
		FPlatformProcess::FreeDllHandle(DLLHandle);
	}
	DLLHandles.Reset();
}
#endif //UE_ENABLE_ICU && (IS_PROGRAM || !IS_MONOLITHIC)

#if ENABLE_LOC_TESTING
namespace
{
	bool LeetifyInRange(FString& String, const int32 Begin, const int32 End)
	{
		bool Succeeded = false;
		for(int32 Index = Begin; Index < End; ++Index)
		{
			switch(String[Index])
			{
			case TEXT('A'): { String[Index] = TEXT('4'); Succeeded = true; } break;
			case TEXT('a'): { String[Index] = TEXT('@'); Succeeded = true; } break;
			case TEXT('B'): { String[Index] = TEXT('8'); Succeeded = true; } break;
			case TEXT('b'): { String[Index] = TEXT('8'); Succeeded = true; } break;
			case TEXT('E'): { String[Index] = TEXT('3'); Succeeded = true; } break;
			case TEXT('e'): { String[Index] = TEXT('3'); Succeeded = true; } break;
			case TEXT('G'): { String[Index] = TEXT('9'); Succeeded = true; } break;
			case TEXT('g'): { String[Index] = TEXT('9'); Succeeded = true; } break;
			case TEXT('I'): { String[Index] = TEXT('1'); Succeeded = true; } break;
			case TEXT('i'): { String[Index] = TEXT('!'); Succeeded = true; } break;
			case TEXT('O'): { String[Index] = TEXT('0'); Succeeded = true; } break;
			case TEXT('o'): { String[Index] = TEXT('0'); Succeeded = true; } break;
			case TEXT('S'): { String[Index] = TEXT('5'); Succeeded = true; } break;
			case TEXT('s'): { String[Index] = TEXT('$'); Succeeded = true; } break;
			case TEXT('T'): { String[Index] = TEXT('7'); Succeeded = true; } break;
			case TEXT('t'): { String[Index] = TEXT('7'); Succeeded = true; } break;
			case TEXT('Z'): { String[Index] = TEXT('2'); Succeeded = true; } break;
			case TEXT('z'): { String[Index] = TEXT('2'); Succeeded = true; } break;
			}
		}

		return Succeeded;
	}
}

FString& FInternationalization::Leetify(FString& SourceString)
{
	// Check that the string hasn't already been Leetified
	if( SourceString.IsEmpty() ||
		(SourceString[ 0 ] != 0x2021 && SourceString.Find( TEXT("\x00AB"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0 ) == -1) )
	{
		bool Succeeded = false;

		FString OpenBlock = TEXT("{");
		FString CloseBlock = TEXT("}");
		uint32 SanityLoopCheck=0xFFFF;

		int32 CurrentBlockBeginPos=-1;
		int32 CurrentBlockEndPos=0;
		int32 PrevBlockBeginPos=0;
		int32 PrevBlockEndPos=-1;
		int32 CurrArgBlock=0;

		struct FBlockRange
		{
			int32 BeginPos;
			int32 EndPos;
		};

		TArray<FBlockRange> BlockRanges;
		while(--SanityLoopCheck > 0)
		{
			//Find the start of the next block. Delimited with an open brace '{'
			++CurrentBlockBeginPos;
			while(true)
			{
				CurrentBlockBeginPos = SourceString.Find(OpenBlock,ESearchCase::CaseSensitive, ESearchDir::FromStart, CurrentBlockBeginPos);
				if( CurrentBlockBeginPos == -1 )
				{
					break;//No block open started so we've reached the end of the format string.
				}

				if( CurrentBlockBeginPos >= 0 && SourceString[CurrentBlockBeginPos+1] == OpenBlock[0] )
				{
					//Skip past {{ literals in the format
					CurrentBlockBeginPos += 2;
					continue;
				}

				break;
			}

			//No more block opening braces found so we're done.
			if( CurrentBlockBeginPos == -1 )
			{
				break;
			}

			//Find the end of the block. Delimited with a close brace '}'
			CurrentBlockEndPos=SourceString.Find(CloseBlock,ESearchCase::CaseSensitive, ESearchDir::FromStart,CurrentBlockBeginPos);

			FBlockRange NewRange = { CurrentBlockBeginPos, CurrentBlockEndPos };
			BlockRanges.Add(NewRange);


			// Insertion of double arrows causes block start and end to be move later in the string, adjust for that.
			++CurrentBlockBeginPos;
			++CurrentBlockEndPos;

			Succeeded = LeetifyInRange(SourceString, PrevBlockEndPos + 1, CurrentBlockBeginPos) || Succeeded;

			PrevBlockBeginPos=CurrentBlockBeginPos;
			PrevBlockEndPos=CurrentBlockEndPos;

			++CurrArgBlock;
		}

		Succeeded = LeetifyInRange(SourceString, CurrentBlockEndPos + 1, SourceString.Len()) || Succeeded;

		// Insert double arrows around parameter blocks.
		if(BlockRanges.Num() > 0)
		{
			FString ResultString;
			int32 i;
			for(i = 0; i < BlockRanges.Num(); ++i)
			{
				// Append intermediate part of string.
				int32 EndOfLastPart = (i - 1 >= 0) ? (BlockRanges[i - 1].EndPos + 1) : 0;
				ResultString += SourceString.Mid(EndOfLastPart, BlockRanges[i].BeginPos - EndOfLastPart);
				// Wrap block.
				ResultString += TEXT("\x00AB");
				// Append block.
				ResultString += SourceString.Mid(BlockRanges[i].BeginPos, BlockRanges[i].EndPos - BlockRanges[i].BeginPos + 1);
				// Wrap block.
				ResultString += TEXT("\x00BB");
			}
			ResultString += SourceString.Mid(BlockRanges[i - 1].EndPos + 1, SourceString.Len() - BlockRanges[i - 1].EndPos + 1);
			SourceString = ResultString;
		}

		if( !Succeeded )
		{
			// Failed to LEETify, add something to beginning and end just to help mark the string.
			SourceString = FString(TEXT("\x2021")) + SourceString + FString(TEXT("\x2021"));
		}
	}

	return SourceString;
}
#endif

void FInternationalization::GetCultureNames(TArray<FString>& CultureNames) const
{
	CultureNames.Reset();
	for(const auto& Culture : AllCultures)
	{
		CultureNames.Add(Culture->GetName());
	}
}

void FInternationalization::GetCulturesWithAvailableLocalization(const TArray<FString>& InLocalizationPaths, TArray< TSharedPtr<FCulture, ESPMode::ThreadSafe> >& OutAvailableCultures) const
{
	OutAvailableCultures.Reset();

	TArray<FString> AllLocalizationFolders;
	IPlatformFile& PlatformFile = IPlatformFile::GetPlatformPhysical();
	for(const auto& LocalizationPath : InLocalizationPaths)
	{
		/* Visitor class used to enumerate directories of culture */
		class FCultureEnumeratorVistor : public IPlatformFile::FDirectoryVisitor
		{
		public:
			FCultureEnumeratorVistor( TArray<FString>& OutLocalizationFolders )
				: LocalizationFolders(OutLocalizationFolders)
			{
			}

			virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
			{
				if(bIsDirectory)
				{
					// UE localization resource folders use "en-US" style while ICU uses "en_US"
					const FString LocalizationFolder = FPaths::GetCleanFilename(FilenameOrDirectory);
					const FString CanonicalName = GetCanonicalName(LocalizationFolder);
					LocalizationFolders.AddUnique(CanonicalName);
				}

				return true;
			}

			/** Array to fill with the names of the UE localization folders available at the given path */
			TArray<FString>& LocalizationFolders;
		};

		FCultureEnumeratorVistor CultureEnumeratorVistor(AllLocalizationFolders);
		PlatformFile.IterateDirectory(*LocalizationPath, CultureEnumeratorVistor);	
	}

	// Find any cultures that are a complete or partial match for the languages we have translations for
	for(const auto& Culture : AllCultures)
	{
		for(const FString& LocalizationFolder : AllLocalizationFolders)
		{
			// Check the full name, but if it doesn't match, see if the base language is present
			if(LocalizationFolder == Culture->GetName() || LocalizationFolder == Culture->GetTwoLetterISOLanguageName())
			{
				OutAvailableCultures.AddUnique(Culture);
			}
		}
	}
}

FInternationalization::FInternationalization()
	:	bIsInitialized(false)
	,	CurrentCultureIndex(INDEX_NONE)
{

}

void FInternationalization::PopulateAllCultures(void)
{
#if UE_ENABLE_ICU
	int32_t LocaleCount;
	const icu::Locale* const AvailableLocales = icu::Locale::getAvailableLocales(LocaleCount);

	// Get available locales and create cultures.
	AllCultures.Reset(LocaleCount);
	for(int32_t i = 0; i < LocaleCount; ++i)
	{
		AllCultures.Add( MakeShareable( new FCulture( AvailableLocales[i].getName() ) ) );
	}

	const FString DefaultLocaleName = FPlatformMisc::GetDefaultLocale();

	// Find the default locale as a culture, if present..
	auto FindDefaultPredicate = [DefaultLocaleName](const TSharedRef<FCulture, ESPMode::ThreadSafe>& Culture) -> bool
	{
		return Culture->GetName() == DefaultLocaleName;
	};
	TSharedRef<FCulture, ESPMode::ThreadSafe>* FoundDefaultCulture = AllCultures.FindByPredicate(FindDefaultPredicate);
	// If present, set default culture variable.
	if(FoundDefaultCulture)
	{
		DefaultCulture = *FoundDefaultCulture;
	}
	// Otherwise, create, set, and add default culture variable.
	else
	{
		DefaultCulture = MakeShareable(new FCulture(DefaultLocaleName));
		AllCultures.Add( DefaultCulture.ToSharedRef() );
	}

#else
	AllCultures.Add( FInvariantCulture::Create() );
#if ENABLE_LOC_TESTING
	AllCultures.Add( FLEETCulture::Create() );
#endif
	AllCultures.Add( FAmericanEnglishCulture::Create() );
	AllCultures.Add( FIndiaHindiCulture::Create() );
	AllCultures.Add( FJapaneseCulture::Create() );
	AllCultures.Add( FKoreanCulture::Create() );

	DefaultCulture = GetCulture(TEXT(""));
#endif 

	InvariantCulture = GetCulture(TEXT(""));
}

#undef LOCTEXT_NAMESPACE
