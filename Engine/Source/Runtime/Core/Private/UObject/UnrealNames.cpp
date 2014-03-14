// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnName.cpp: Unreal global name code.
=============================================================================*/

#include "CorePrivate.h"


DEFINE_LOG_CATEGORY_STATIC(LogUnrealNames, Log, All);

/*-----------------------------------------------------------------------------
	FName helpers.
-----------------------------------------------------------------------------*/

FNameEntry* AllocateNameEntry( const void* Name, NAME_INDEX Index, FNameEntry* HashNext, bool bIsPureAnsi );

/**
* Helper function that can be used inside the debuggers watch window. E.g. "DebugFName(Class->Name.Index)". 
*
* @param	Index	Name index to look up string for
* @return			Associated name
*/
const TCHAR* DebugFName(int32 Index)
{
	// Hardcoded static array. This function is only used inside the debugger so it should be fine to return it.
	static TCHAR TempName[256];
	FCString::Strcpy(TempName, *FName::SafeString((EName)Index));
	return TempName;
}

/**
* Helper function that can be used inside the debuggers watch window. E.g. "DebugFName(Class->Name.Index, Class->Name.Number)". 
*
* @param	Index	Name index to look up string for
* @param	Number	Internal instance number of the FName to print (which is 1 more than the printed number)
* @return			Associated name
*/
const TCHAR* DebugFName(int32 Index, int32 Number)
{
	// Hardcoded static array. This function is only used inside the debugger so it should be fine to return it.
	static TCHAR TempName[256];
	FCString::Strcpy(TempName, *FName::SafeString((EName)Index, Number));
	return TempName;
}

/**
 * Helper function that can be used inside the debuggers watch window. E.g. "DebugFName(Class->Name)". 
 *
 * @param	Name	Name to look up string for
 * @return			Associated name
 */
const TCHAR* DebugFName(FName& Name)
{
	// Hardcoded static array. This function is only used inside the debugger so it should be fine to return it.
	static TCHAR TempName[256];
	FCString::Strcpy(TempName, *FName::SafeString((EName)Name.Index, Name.Number));
	return TempName;
}


/*-----------------------------------------------------------------------------
	FNameEntry
-----------------------------------------------------------------------------*/


/**
 * @return FString of name portion minus number.
 */
FString FNameEntry::GetPlainNameString() const
{
	if( IsWide() )
	{
		return FString(WideName);
	}
	else
	{
		return FString(AnsiName);
	}
}

/**
 * Appends this name entry to the passed in string.
 *
 * @param	String	String to append this name to
 */
void FNameEntry::AppendNameToString( FString& String ) const
{
	if( IsWide() )
	{
		String += WideName;
	}
	else
	{
		String += AnsiName;
	}
}

/**
 * @return case insensitive hash of name
 */
uint32 FNameEntry::GetNameHash() const
{
	if( IsWide() )
	{
		return FCrc::Strihash_DEPRECATED(WideName);
	}
	else
	{
		return FCrc::Strihash_DEPRECATED(AnsiName);
	}
}

/**
 * @return length of name
 */
int32 FNameEntry::GetNameLength() const
{
	if( IsWide() )
	{
		return FCStringWide::Strlen( WideName );
	}
	else
	{
		return FCStringAnsi::Strlen( AnsiName );
	}
}

/**
 * Compares name without looking at case.
 *
 * @param	InName	Name to compare to
 * @return	true if equal, false otherwise
 */
bool FNameEntry::IsEqual( const ANSICHAR* InName ) const
{
	if( IsWide() )
	{
		// Matching wide-ness means strings are not equal.
		return false;
	}
	else
	{
		return FCStringAnsi::Stricmp( AnsiName, InName ) == 0;
	}
}

/**
 * Compares name without looking at case.
 *
 * @param	InName	Name to compare to
 * @return	true if equal, false otherwise
 */
bool FNameEntry::IsEqual( const WIDECHAR* InName ) const
{
	if( !IsWide() )
	{
		// Matching wide-ness means strings are not equal.
		return false;
	}
	else
	{
		return FCStringWide::Stricmp( WideName, InName ) == 0;
	}
}

/**
 * Returns the size in bytes for FNameEntry structure. This is != sizeof(FNameEntry) as we only allocated as needed.
 *
 * @param	Name	Name to determine size for
 * @return	required size of FNameEntry structure to hold this string (might be wide or ansi)
 */
int32 FNameEntry::GetSize( const TCHAR* Name )
{
	return FNameEntry::GetSize( FCString::Strlen( Name ), FCString::IsPureAnsi( Name ) );
}

/**
 * Returns the size in bytes for FNameEntry structure. This is != sizeof(FNameEntry) as we only allocated as needed.
 *
 * @param	Length			Length of name
 * @param	bIsPureAnsi		Whether name is pure ANSI or not
 * @return	required size of FNameEntry structure to hold this string (might be wide or ansi)
 */
int32 FNameEntry::GetSize( int32 Length, bool bIsPureAnsi )
{
	// Calculate base size without union array.
	int32 Size = sizeof(FNameEntry) - NAME_SIZE * sizeof(TCHAR);
	// Add size required for string.
	Size += (Length+1) * (bIsPureAnsi ? sizeof(ANSICHAR) : sizeof(TCHAR));
	return Size;
}

/*-----------------------------------------------------------------------------
	FName statics.
-----------------------------------------------------------------------------*/

TNameEntryArray& FName::GetNames()
{
	// NOTE: The reason we initialize to NULL here is due to an issue with static initialization of variables with
	// constructors/destructors across DLL boundaries, where a function called from a statically constructed object
	// calls a function in another module (such as this function) that creates a static variable.  A crash can occur
	// because the static initialization of this DLL has not yet happened, and the CRT's list of static destructors
	// cannot be written to because it has not yet been initialized fully.	(@todo UE4 DLL)
	static TNameEntryArray*	Names = NULL;
	if( Names == NULL )
	{
		check(IsInGameThread());
		Names = new TNameEntryArray();
	}
	return *Names;
}

//@todo, delete this in 2013, and clean up the visualizers
TArray<FNameEntry const*>* FName::GetNameTableForDebuggerVisualizers_ST()
{
	return NULL;
}

FNameEntry*** FName::GetNameTableForDebuggerVisualizers_MT()
{
	return GetNames().GetRootBlockForDebuggerVisualizers();
}


FCriticalSection* FName::GetCriticalSection()
{
	static FCriticalSection*	CriticalSection = NULL;
	if( CriticalSection == NULL )
	{
		check(IsInGameThread());
		CriticalSection = new FCriticalSection();
	}
	return CriticalSection;
}


// Static variables.
FNameEntry*						FName::NameHash[ FNameDefs::NameHashBucketCount ];
int32							FName::NameEntryMemorySize;
/** Number of ANSI names in name table.						*/
int32							FName::NumAnsiNames;			
/** Number of wide names in name table.					*/
int32							FName::NumWideNames;


/*-----------------------------------------------------------------------------
	FName implementation.
-----------------------------------------------------------------------------*/

/**
 * Create an FName. If FindType is FNAME_Find, and the string part of the name 
 * doesn't already exist, then the name will be NAME_None
 *
 * @param Name Value for the string portion of the name
 * @param FindType Action to take (see EFindName)
 */
FName::FName( const WIDECHAR* Name, EFindName FindType, bool )
{
	Init(Name, NAME_NO_NUMBER_INTERNAL, FindType);
}

FName::FName( const ANSICHAR* Name, EFindName FindType, bool )
{
	Init(Name, NAME_NO_NUMBER_INTERNAL, FindType);
}

/**
 * Create an FName. If FindType is FNAME_Find, and the string part of the name 
 * doesn't already exist, then the name will be NAME_None
 *
 * @param Name Value for the string portion of the name
 * @param Number Value for the number portion of the name
 * @param FindType Action to take (see EFindName)
 */
FName::FName( const TCHAR* Name, int32 InNumber, EFindName FindType )
{
	Init(Name, InNumber, FindType);
}


/**
 * Constructor used by ULinkerLoad when loading its name table; Creates an FName with an instance
 * number of 0 that does not attempt to split the FName into string and number portions.
 */
FName::FName( ELinkerNameTableConstructor, const WIDECHAR* Name )
{
	Init(Name, NAME_NO_NUMBER_INTERNAL, FNAME_Add, false);
}

/**
 * Constructor used by ULinkerLoad when loading its name table; Creates an FName with an instance
 * number of 0 that does not attempt to split the FName into string and number portions.
 */
FName::FName( ELinkerNameTableConstructor, const ANSICHAR* Name )
{
	Init(Name, NAME_NO_NUMBER_INTERNAL, FNAME_Add, false);
}

FName::FName( enum EName HardcodedIndex, const TCHAR* Name )
{
	check(HardcodedIndex >= 0);
	Init(Name, NAME_NO_NUMBER_INTERNAL, FNAME_Add, false, HardcodedIndex);
}

/**
 * Comparision operator.
 *
 * @param	Other	String to compare this name to
 * @return true if name matches the string, false otherwise
 */
bool FName::operator==( const TCHAR * Other ) const
{
	// Find name entry associated with this FName.
	check( Other );
	TNameEntryArray& Names = GetNames();
	check( Index < Names.Num() );
	FNameEntry const* Entry = Names[Index];
	check( Entry );

	// Temporary buffer to hold split name in case passed in name is of Name_Number format.
	WIDECHAR TempBuffer[NAME_SIZE];
	int32 InNumber	= NAME_NO_NUMBER_INTERNAL;
	int32 TempNumber	= NAME_NO_NUMBER_INTERNAL;

	// Check whether we need to split the passed in string into name and number portion.
	auto           WideOther    = StringCast<WIDECHAR>(Other);
	const WIDECHAR* WideOtherPtr = WideOther.Get();
	if( SplitNameWithCheck( WideOtherPtr, TempBuffer, ARRAY_COUNT(TempBuffer), TempNumber) )
	{
		WideOtherPtr	= TempBuffer;
		InNumber	= NAME_EXTERNAL_TO_INTERNAL(TempNumber);
	}

	// Report a match if both the number and string portion match.
	bool bAreNamesMatching = false;
	if( InNumber == Number && !FCStringWide::Stricmp( WideOtherPtr, Entry->IsWide() ? Entry->GetWideName() : StringCast<WIDECHAR>(Entry->GetAnsiName()).Get() ) )
	{
		bAreNamesMatching = true;
	}

	return bAreNamesMatching;
}

/**
 * Compares name to passed in one. Sort is alphabetical ascending.
 *
 * @param	Other	Name to compare this against
 * @return	< 0 is this < Other, 0 if this == Other, > 0 if this > Other
 */
int32 FName::Compare( const FName& Other ) const
{
	// Names match, check whether numbers match.
	if( GetIndex() == Other.GetIndex() )
	{
		return GetNumber() - Other.GetNumber();
	}
	// Names don't match. This means we don't even need to check numbers.
	else
	{
		TNameEntryArray& Names = GetNames();
		FNameEntry const* ThisEntry = Names[GetIndex()];
		FNameEntry const* OtherEntry = Names[Other.GetIndex()];

		// Ansi/Wide mismatch, convert to wide
		if( ThisEntry->IsWide() != OtherEntry->IsWide() )
		{
			return FCStringWide::Stricmp(	ThisEntry->IsWide() ? ThisEntry->GetWideName() : StringCast<WIDECHAR>(ThisEntry->GetAnsiName()).Get(),
								OtherEntry->IsWide() ? OtherEntry->GetWideName() : StringCast<WIDECHAR>(OtherEntry->GetAnsiName()).Get() );
		}
		// Both are wide.
		else if( ThisEntry->IsWide() )
		{
			return FCStringWide::Stricmp( ThisEntry->GetWideName(), OtherEntry->GetWideName() );
		}
		// Both are ansi.
		else
		{
			return FCStringAnsi::Stricmp( ThisEntry->GetAnsiName(), OtherEntry->GetAnsiName() );
		}		
	}
}

void FName::Init(const WIDECHAR* InName, int32 InNumber, EFindName FindType, bool bSplitName, int32 HardcodeIndex )
{
	check(FCString::Strlen(InName)<=NAME_SIZE);

	// initialize the name subsystem if necessary
	if (!GetIsInitialized())
	{
		StaticInit();
	}

	WIDECHAR TempBuffer[NAME_SIZE];
	int32 TempNumber;
	// if we were passed in a number, we can't split again, other wise, a_1_2_3_4 would change everytime
	// it was loaded in
	if (InNumber == NAME_NO_NUMBER_INTERNAL && bSplitName == true )
	{
		if (SplitNameWithCheck(InName, TempBuffer, ARRAY_COUNT(TempBuffer), TempNumber))
		{
			InName = TempBuffer;
			InNumber = NAME_EXTERNAL_TO_INTERNAL(TempNumber);
		}
	}

	check(InName);

	// If empty or invalid name was specified, return NAME_None.
	if( !InName[0] )
	{
		check(HardcodeIndex < 1); // if this is hardcoded, it better be zero 
		Index = NAME_None;
		Number = NAME_NO_NUMBER_INTERNAL;
		return;
	}


	//!!!! Caution, since these are set by static initializers from multiple threads, we must use local variables for this stuff until just before we return.

	int32 OutIndex = HardcodeIndex;

	// set the number
	int32 OutNumber = InNumber;

	// Hash value of string. Depends on whether the name is going to be ansi or wide.
	int32 iHash;

	// Figure out whether we have a pure ansi name or not.
	ANSICHAR AnsiName[NAME_SIZE];
	bool bIsPureAnsi = FCStringWide::IsPureAnsi( InName );
	if( bIsPureAnsi )
	{
		FCStringAnsi::Strncpy( AnsiName, StringCast<ANSICHAR>(InName).Get(), ARRAY_COUNT(AnsiName) );
		iHash = FCrc::Strihash_DEPRECATED( AnsiName ) & (ARRAY_COUNT(NameHash)-1);
	}
	else
	{
		iHash = FCrc::Strihash_DEPRECATED( InName ) & (ARRAY_COUNT(NameHash)-1);
	}

	if (OutIndex < 0)
	{
		// Try to find the name in the hash.
		for( FNameEntry* Hash=NameHash[iHash]; Hash; Hash=Hash->HashNext )
		{
			FPlatformMisc::Prefetch( Hash->HashNext );
			// Compare the passed in string, either ANSI or TCHAR.
			if( ( bIsPureAnsi && Hash->IsEqual( AnsiName )) 
			||  (!bIsPureAnsi && Hash->IsEqual( InName )) )
			{
				// Found it in the hash.
				OutIndex = Hash->GetIndex();

				// Check to see if the caller wants to replace the contents of the
				// FName with the specified value. This is useful for compiling
				// script classes where the file name is lower case but the class
				// was intended to be uppercase.
				if (FindType == FNAME_Replace_Not_Safe_For_Threading)
				{
					check(IsInGameThread());
					// This should be impossible due to the compare above
					// This *must* be true, or we'll overwrite memory when the
					// copy happens if it is longer
					check(FCString::Strlen(InName) == Hash->GetNameLength());
					// Can't rely on the template override for static arrays since the safe crt version of strcpy will fill in
					// the remainder of the array of NAME_SIZE with 0xfd.  So, we have to pass in the length of the dynamically allocated array instead.
					if( bIsPureAnsi )
					{
						FCStringAnsi::Strcpy(const_cast<ANSICHAR*>(Hash->GetAnsiName()),Hash->GetNameLength()+1,AnsiName);
					}
					else
					{
						FCStringWide::Strcpy(const_cast<WIDECHAR*>(Hash->GetWideName()),Hash->GetNameLength()+1,InName);
					}
				}
				check(OutIndex >= 0);
				Index = OutIndex;
				Number = OutNumber;
				return;
			}
		}

		// Didn't find name.
		if( FindType==FNAME_Find )
		{
			// Not found.
			Index = NAME_None;
			Number = NAME_NO_NUMBER_INTERNAL;
			return;
		}
	}
	// acquire the lock
	FScopeLock ScopeLock(GetCriticalSection());
	if (OutIndex < 0)
	{
		// Try to find the name in the hash. AGAIN...we might have been adding from a different thread and we just missed it
		for( FNameEntry* Hash=NameHash[iHash]; Hash; Hash=Hash->HashNext )
		{
			// Compare the passed in string, either ANSI or TCHAR.
			if( ( bIsPureAnsi && Hash->IsEqual( AnsiName )) 
				||  (!bIsPureAnsi && Hash->IsEqual( InName )) )
			{
				// Found it in the hash.
				OutIndex = Hash->GetIndex();
				check(FindType == FNAME_Add);  // if this was a replace, well it isn't safe for threading. Find should have already been handled
				Index = OutIndex;
				Number = OutNumber;
				return;
			}
		}
	}
	FNameEntry* OldHash=NameHash[iHash];
	TNameEntryArray& Names = GetNames();
	if (OutIndex < 0)
	{
		OutIndex = Names.AddZeroed(1);
	}
	else
	{
		Names.AddZeroed(OutIndex + 1 - Names.Num());
	}
	FNameEntry* NewEntry = AllocateNameEntry( bIsPureAnsi ? (void const*)AnsiName : (void const*)InName, OutIndex, OldHash, bIsPureAnsi );
	if (FPlatformAtomics::InterlockedCompareExchangePointer((void**)&Names[OutIndex], NewEntry, NULL) != NULL) // we use an atomic operation to check for unexpected concurrency, verify alignment, etc
	{
		UE_LOG(LogUnrealNames, Fatal, TEXT("Hardcoded name '%s' at index %i was duplicated (or unexpected concurrency). Existing entry is '%s'."), *NewEntry->GetPlainNameString(), NewEntry->GetIndex(), *Names[OutIndex]->GetPlainNameString() );
	}
	if (FPlatformAtomics::InterlockedCompareExchangePointer((void**)&NameHash[iHash], NewEntry, OldHash) != OldHash) // we use an atomic operation to check for unexpected concurrency, verify alignment, etc
	{
		check(0); // someone changed this while we were changing it
	}
	check(OutIndex >= 0);
	Index = OutIndex;
	Number = OutNumber;
}


FString FName::ToString() const
{
	FString Out;	
	ToString(Out);
	return Out;
}

void FName::ToString(FString& Out) const
{
	TNameEntryArray& Names = GetNames();
	// a version of ToString that saves at least one string copy
	checkName(Index < Names.Num());
	checkName(Names[Index]);
	FNameEntry const* NameEntry = Names[Index];
	Out.Empty( NameEntry->GetNameLength() + 6);
	AppendString(Out);
}

void FName::AppendString(FString& Out) const
{
	TNameEntryArray& Names = GetNames();
	checkName(Index < Names.Num());
	checkName(Names[Index]);
	FNameEntry const* NameEntry = Names[Index];
	NameEntry->AppendNameToString( Out );
	if (Number != NAME_NO_NUMBER_INTERNAL)
	{
		Out += TEXT("_");
		Out.AppendInt(NAME_INTERNAL_TO_EXTERNAL(Number));
	}
}


/*-----------------------------------------------------------------------------
	FName subsystem.
-----------------------------------------------------------------------------*/

void FName::StaticInit()
{
	check(IsInGameThread());
	/** Global instance used to initialize the GCRCTable. It used to be initialized in appInit() */
	//@todo: Massive workaround for static init order without needing to use a function call for every use of GCRCTable
	// This ASSUMES that fname::StaticINit is going to be called BEFORE ANY use of GCRCTable
	FCrc::Init();

	check(GetIsInitialized() == false);
	check((ARRAY_COUNT(NameHash)&(ARRAY_COUNT(NameHash)-1)) == 0);
	GetIsInitialized() = 1;


	// Init the name hash.
	for (int32 HashIndex = 0; HashIndex < ARRAY_COUNT(FName::NameHash); HashIndex++)
	{
		NameHash[HashIndex] = NULL;
	}

	{
		// Register all hardcoded names.
		#define REGISTER_NAME(num,namestr) FName Temp_##namestr(EName(num), TEXT(#namestr));
		#include "UObject/UnrealNames.h"
	}

#if DO_CHECK
	// Verify no duplicate names.
	for (int32 HashIndex = 0; HashIndex < ARRAY_COUNT(NameHash); HashIndex++)
	{
		for (FNameEntry* Hash = NameHash[HashIndex]; Hash; Hash = Hash->HashNext)
		{
			for (FNameEntry* Other = Hash->HashNext; Other; Other = Other->HashNext)
			{
				if (FCString::Stricmp(*Hash->GetPlainNameString(), *Other->GetPlainNameString()) == 0)
				{
					// we can't print out here because there may be no log yet if this happens before main starts
					if (FPlatformMisc::IsDebuggerPresent())
					{
						FPlatformMisc::DebugBreak();
					}
					else
					{
						FPlatformMisc::PromptForRemoteDebugging(false);
						FMessageDialog::Open(EAppMsgType::Ok, FText::Format( NSLOCTEXT("UnrealEd", "DuplicatedHardcodedName", "Duplicate hardcoded name: {0}"), FText::FromString( Hash->GetPlainNameString() ) ) );
						FPlatformMisc::RequestExit(false);
					}
				}
			}
		}
	}
	// check that the MAX_NETWORKED_HARDCODED_NAME define is correctly set
	if (GetMaxNames() <= MAX_NETWORKED_HARDCODED_NAME)
	{
		if (FPlatformMisc::IsDebuggerPresent())
		{
			FPlatformMisc::DebugBreak();
		}
		else
		{
			FPlatformMisc::PromptForRemoteDebugging(false);
			// can't use normal check()/UE_LOG(LogUnrealNames, Fatal,) here
			FMessageDialog::Open( EAppMsgType::Ok, FText::Format( NSLOCTEXT("UnrealEd", "MAX_NETWORKED_HARDCODED_NAME Incorrect", "MAX_NETWORKED_HARDCODED_NAME is incorrectly set! (Currently {0}, must be no greater than {1}"), FText::AsNumber( MAX_NETWORKED_HARDCODED_NAME ), FText::AsNumber( GetMaxNames() - 1 ) ) );
			FPlatformMisc::RequestExit(false);
		}
	}
#endif
}

bool& FName::GetIsInitialized()
{
	static bool bIsInitialized = false;
	return bIsInitialized;
}

void FName::DisplayHash( FOutputDevice& Ar )
{
	int32 UsedBins=0, NameCount=0, MemUsed = 0;
	for( int32 i=0; i<ARRAY_COUNT(NameHash); i++ )
	{
		if( NameHash[i] != NULL ) UsedBins++;
		for( FNameEntry *Hash = NameHash[i]; Hash; Hash=Hash->HashNext )
		{
			NameCount++;
			// Count how much memory this entry is using
			MemUsed += FNameEntry::GetSize( Hash->GetNameLength(), Hash->IsWide() );
		}
	}
	Ar.Logf( TEXT("Hash: %i names, %i/%i hash bins, Mem in bytes %i"), NameCount, UsedBins, ARRAY_COUNT(NameHash), MemUsed);
}

bool FName::SplitNameWithCheck(const WIDECHAR* OldName, WIDECHAR* NewName, int32 NewNameLen, int32& NewNumber)
{
	bool bSucceeded = false;
	const int32 OldNameLength = FCStringWide::Strlen(OldName);

	if(OldNameLength > 0)
	{
		// get string length
		const WIDECHAR* LastChar = OldName + (OldNameLength - 1);
		
		// if the last char is a number, then we will try to split
		const WIDECHAR* Ch = LastChar;
		if (*Ch >= '0' && *Ch <= '9')
		{
			// go backwards, looking an underscore or the start of the string
			// (we don't look at first char because '_9' won't split well)
			while (*Ch >= '0' && *Ch <= '9' && Ch > OldName)
			{
				Ch--;
			}

			// if the first non-number was an underscore (as opposed to a letter),
			// we can split
			if (*Ch == '_')
			{
				// check for the case where there are multiple digits after the _ and the first one
				// is a 0 ("Rocket_04"). Can't split this case. (So, we check if the first char
				// is not 0 or the length of the number is 1 (since ROcket_0 is valid)
				if (Ch[1] != '0' || LastChar - Ch == 1)
				{
					// attempt to convert what's following it to a number
					uint64 TempConvert = FCStringWide::Atoi64(Ch + 1);
					if (TempConvert <= MAX_int32)
					{
						NewNumber = (int32)TempConvert;
						// copy the name portion into the buffer
						FCStringWide::Strncpy(NewName, OldName, FMath::Min<int32>(Ch - OldName + 1, NewNameLen));

						// mark successful
						bSucceeded = true;
					}
				}
			}
		}
	}

	// return success code
	return bSucceeded;
}

bool FName::IsValidXName( FString InvalidChars/*=INVALID_NAME_CHARACTERS*/, FText* Reason/*=NULL*/ ) const
{
	FString Name = ToString();

	// See if the name contains invalid characters.
	FString Char;
	for( int32 x = 0; x < InvalidChars.Len() ; ++x )
	{
		Char = InvalidChars.Mid( x, 1 );

		if( Name.Contains( Char ) )
		{
			if ( Reason != NULL )
			{
				*Reason = FText::Format( NSLOCTEXT("Engine", "FNameContainsInvalidCharacter", "Name contains an invalid character : [{0}]"), FText::FromString( Char ) );
			}
			return false;
		}
	}

	return true;
}

void FName::AutoTest()
{
	FName AutoTest_1("AutoTest_1");
	FName autoTest_1("autoTest_1");
	FName AutoTest_2(TEXT("AutoTest_2"));
	FName AutoTestB_2(TEXT("AutoTestB_2"));

	check(AutoTest_1 != AutoTest_2);
	check(AutoTest_1 == autoTest_1);
	check(!FCString::Strcmp(*AutoTest_1.ToString(), TEXT("AutoTest_1")));
	check(!FCString::Strcmp(*autoTest_1.ToString(), TEXT("AutoTest_1")));
	check(!FCString::Strcmp(*AutoTestB_2.ToString(), TEXT("AutoTestB_2")));
	check(autoTest_1.GetIndex() == AutoTest_2.GetIndex());
	check(autoTest_1.GetPlainNameString() == AutoTest_1.GetPlainNameString());
	check(autoTest_1.GetPlainNameString() == AutoTest_2.GetPlainNameString());
	check(*AutoTestB_2.GetPlainNameString() != *AutoTest_2.GetPlainNameString());
	check(AutoTestB_2.GetNumber() == AutoTest_2.GetNumber());
	check(autoTest_1.GetNumber() != AutoTest_2.GetNumber());
}

/*-----------------------------------------------------------------------------
	FNameEntry implementation.
-----------------------------------------------------------------------------*/

FArchive& operator<<( FArchive& Ar, FNameEntry& E )
{
	if( Ar.IsLoading() )
	{
		// for optimization reasons, we want to keep pure Ansi strings as Ansi for initializing the name entry
		// (and later the FName) to stop copying in and out of TCHARs
		int32 StringLen;
		Ar << StringLen;

		// negative stringlen means it's a wide string
		if (StringLen < 0)
		{
			StringLen = -StringLen;

			// mark the name will be wide
			E.PreSetIsWideForSerialization(true);

			// get the pointer to the wide array 
			WIDECHAR* WideName = const_cast<WIDECHAR*>(E.GetWideName());

			// read in the UCS2CHAR string and byteswap it, etc
			auto Sink = StringMemoryPassthru<UCS2CHAR>(WideName, StringLen, StringLen);
			Ar.Serialize(Sink.Get(), StringLen * sizeof(UCS2CHAR));
			Sink.Apply();

			INTEL_ORDER_TCHARARRAY(WideName)
		}
		else
		{
			// mark the name will be ansi
			E.PreSetIsWideForSerialization(false);

			// ansi strings can go right into the AnsiBuffer
			ANSICHAR* AnsiName = const_cast<ANSICHAR*>(E.GetAnsiName());
			Ar.Serialize(AnsiName, StringLen);
		}
	}
	else
	{
		FString Str = E.GetPlainNameString();
		Ar << Str;
	}

	return Ar;
}

/**
 * Pooled allocator for FNameEntry structures. Doesn't have to worry about freeing memory as those
 * never go away. It simply uses 64K chunks and allocates new ones as space runs out. This reduces
 * allocation overhead significantly (only minor waste on 64k boundaries) and also greatly helps
 * with fragmentation as 50-100k allocations turn into tens of allocations.
 */
class FNameEntryPoolAllocator
{
public:
	/** Initializes all member variables. */
	FNameEntryPoolAllocator()
	{
		TotalAllocatedPages	= 0;
		CurrentPoolStart	= NULL;
		CurrentPoolEnd		= NULL;
		check(ThreadGuard.GetValue() == 0);
	}

	/**
	 * Allocates the requested amount of bytes and casts them to a FNameEntry pointer.
	 *
	 * @param   Size  Size in bytes to allocate
	 * @return  Allocation of passed in size cast to a FNameEntry pointer.
	 */
	FNameEntry* Allocate( int32 Size )
	{
		check(ThreadGuard.Increment() == 1);
		// Some platforms need all of the name entries to be aligned to 4 bytes, so by
		// aligning the size here the next allocation will be aligned to 4
		Size = Align( Size, ALIGNOF(FNameEntry) );

		// Allocate a new pool if current one is exhausted. We don't worry about a little bit
		// of waste at the end given the relative size of pool to average and max allocation.
		if( CurrentPoolEnd - CurrentPoolStart < Size )
		{
			AllocateNewPool();
		}
		check( CurrentPoolEnd - CurrentPoolStart >= Size );
		// Return current pool start as allocation and increment by size.
		FNameEntry* NameEntry = (FNameEntry*) CurrentPoolStart;
		CurrentPoolStart += Size;
		check(ThreadGuard.Decrement() == 0);
		return NameEntry;
	}

	/**
	 * Returns the amount of memory to allocate for each page pool.
	 *
	 * @return  Page pool size.
	 */
	FORCEINLINE int32 PoolSize()
	{
		// Allocate in 64k chunks as it's ideal for page size.
		return 64 * 1024;
	}

	/**
	 * Returns the number of pages that have been allocated so far for names.
	 *
	 * @return  The number of pages allocated.
	 */
	FORCEINLINE int32 PageCount()
	{
		return TotalAllocatedPages;
	}

private:
	/** Allocates a new pool. */
	void AllocateNewPool()
	{
		TotalAllocatedPages++;
		CurrentPoolStart = (uint8*) FMemory::Malloc(PoolSize());
		CurrentPoolEnd = CurrentPoolStart + PoolSize();
	}

	/** Beginning of pool. Allocated by AllocateNewPool, incremented by Allocate.	*/
	uint8* CurrentPoolStart;
	/** End of current pool. Set by AllocateNewPool and checked by Allocate.		*/
	uint8* CurrentPoolEnd;
	/** Total number of pages that have been allocated.								*/
	int32 TotalAllocatedPages;
	/** Threadsafe counter to test for unwanted concurrency							*/
	FThreadSafeCounter ThreadGuard;
};

/** Global allocator for name entries. */
FNameEntryPoolAllocator GNameEntryPoolAllocator;

FNameEntry* AllocateNameEntry( const void* Name, NAME_INDEX Index, FNameEntry* HashNext, bool bIsPureAnsi )
{
	const SIZE_T NameLen  = bIsPureAnsi ? FCStringAnsi::Strlen((ANSICHAR*)Name) : FCString::Strlen((TCHAR*)Name);
	int32 NameEntrySize	  = FNameEntry::GetSize( NameLen, bIsPureAnsi );
	FNameEntry* NameEntry = GNameEntryPoolAllocator.Allocate( NameEntrySize );
	FName::NameEntryMemorySize += NameEntrySize;
	NameEntry->Index      = (Index << NAME_INDEX_SHIFT) | (bIsPureAnsi ? 0 : 1);
	NameEntry->HashNext   = HashNext;
	// Can't rely on the template override for static arrays since the safe crt version of strcpy will fill in
	// the remainder of the array of NAME_SIZE with 0xfd.  So, we have to pass in the length of the dynamically allocated array instead.
	if( bIsPureAnsi )
	{
		FCStringAnsi::Strcpy( const_cast<ANSICHAR*>(NameEntry->GetAnsiName()), NameLen + 1, (ANSICHAR*) Name );
		FName::NumAnsiNames++;
	}
	else
	{
		FCStringWide::Strcpy( const_cast<WIDECHAR*>(NameEntry->GetWideName()), NameLen + 1, (WIDECHAR*) Name );
		FName::NumWideNames++;
	}
	return NameEntry;
}

#if !UE_BUILD_SHIPPING

#include "TaskGraphInterfaces.h"

/**
 Exec function for FNames, mostly for testing
**/
static class FFNameExec: private FSelfRegisteringExec
{
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) OVERRIDE
	{
		// Display information about the name table only rather than using HASH
		if( FParse::Command( &Cmd, TEXT("NAMEHASH") ) )
		{
			FName::DisplayHash(Ar);
			return true;
		}
#if !UE_BUILD_SHIPPING
		else if( FParse::Command( &Cmd, TEXT("FNAME") ) )
		{
			if( FParse::Command(&Cmd,TEXT("THREADTEST")) )
			{
				Ar.Logf( TEXT("Starting fname threading test."));
				struct FTest
				{
					enum 
					{
						NUM_TESTS = 400000,
						NUM_TASKS = 8
					};
					struct FTestTable
					{
						FString TheString;
						volatile int32 NameNumber;
						volatile int32 NameIndex;
						FTestTable(FString const& InString)
							: TheString(InString)
							, NameNumber(0)
							, NameIndex(0)
						{
						}
					};
					TArray<FTestTable> Table;
					FThreadSafeCounter TestCounter;
					FTest()
					{
						for (int32 Index = 0; Index < NUM_TESTS; Index++)
						{
							int32 Name = Index / 10;
							int32 Number = Index % 10;
							FString TestString = FString::Printf(TEXT("Test%dTest_%d"), Name, Number);
							new (Table) FTestTable(TestString);
						}
					}
					void Thread()
					{
						for (int32 Index = 0; Index < Table.Num(); Index++)
						{
							FName Temp(*Table[Index].TheString);
							check(Temp != NAME_None);
							check(Temp.ToString() == Table[Index].TheString);
							if (Table[Index].NameNumber == 0)
							{
								Table[Index].NameNumber = Temp.GetNumber();
							}
							check(Table[Index].NameNumber == Temp.GetNumber());
							if (Table[Index].NameIndex == 0)
							{
								Table[Index].NameIndex = Temp.GetIndex();
							}
							check(Table[Index].NameIndex == Temp.GetIndex());

							FName Temp2(*Table[Index].TheString);
							check(Temp2.ToString() == Table[Index].TheString);
							check(Table[Index].NameNumber == Temp2.GetNumber());
							check(Table[Index].NameIndex == Temp2.GetIndex());
							TestCounter.Increment();
						}

					}
				} Test;

				FGraphEventArray Handles;

				for (int32 ThreadIndex = 0; ThreadIndex < FTest::NUM_TASKS; ThreadIndex++)
				{
					new (Handles) FGraphEventRef(FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
						FSimpleDelegateGraphTask::FDelegate::CreateRaw(&Test, &FTest::Thread),
						TEXT("FName Test"),
						NULL,
						ENamedThreads::AnyThread
						));
				}
				FTaskGraphInterface::Get().WaitUntilTasksComplete(Handles, ENamedThreads::GameThread);
				check(Test.TestCounter.GetValue() == FTest::NUM_TESTS * FTest::NUM_TASKS);
				Ar.Logf( TEXT("Ran fname threading test."));
			}
			return true;
#endif // !UE_BUILD_SHIPPING
		}
		return false;
	}
} FNameExec;
#endif