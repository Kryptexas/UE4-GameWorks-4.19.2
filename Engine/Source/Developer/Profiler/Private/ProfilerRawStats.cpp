// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ProfilerPrivatePCH.h"


/*-----------------------------------------------------------------------------
	Sort helpers
-----------------------------------------------------------------------------*/

/** Sorts allocations by size. */
struct FAllocationInfoSequenceTagLess
{
	FORCEINLINE bool operator()( const FAllocationInfo& A, const FAllocationInfo& B ) const
	{
		return A.SequenceTag < B.SequenceTag;
	}
};

/** Sorts allocations by size. */
struct FAllocationInfoSizeGreater
{
	FORCEINLINE bool operator()( const FAllocationInfo& A, const FAllocationInfo& B ) const
	{
		return B.Size < A.Size;
	}
};

/** Sorts combined allocations by size. */
struct FCombinedAllocationInfoSizeGreater
{
	FORCEINLINE bool operator()( const FCombinedAllocationInfo& A, const FCombinedAllocationInfo& B ) const
	{
		return B.Size < A.Size;
	}
};


/** Sorts node allocations by size. */
struct FNodeAllocationInfoSizeGreater
{
	FORCEINLINE bool operator()( const FNodeAllocationInfo& A, const FNodeAllocationInfo& B ) const
	{
		return B.Size < A.Size;
	}
};

/*-----------------------------------------------------------------------------
	Callstack decoding/encoding
-----------------------------------------------------------------------------*/

/** Helper struct used to manipulate stats based callstacks. */
struct FStatsCallstack
{
	/** Separator. */
	static const TCHAR* CallstackSeparator;

	/** Encodes decoded callstack a string, to be like '45+656+6565'. */
	static FString Encode( const TArray<FName>& Callstack )
	{
		FString Result;
		for (const auto& Name : Callstack)
		{
			Result += TTypeToString<int32>::ToString( (int32)Name.GetComparisonIndex() );
			Result += CallstackSeparator;
		}
		return Result;
	}

	/** Decodes encoded callstack to an array of FNames. */
	static void DecodeToNames( const FName& EncodedCallstack, TArray<FName>& out_DecodedCallstack )
	{
		TArray<FString> DecodedCallstack;
		DecodeToStrings( EncodedCallstack, DecodedCallstack );

		// Convert back to FNames
		for (const auto& It : DecodedCallstack)
		{
			NAME_INDEX NameIndex = 0;
			TTypeFromString<NAME_INDEX>::FromString( NameIndex, *It );
			const FName LongName = FName( NameIndex, NameIndex, 0 );

			out_DecodedCallstack.Add( LongName );
		}
	}

	/** Converts the encoded callstack into human readable callstack. */
	static FString GetHumanReadable( const FName& EncodedCallstack )
	{
		TArray<FName> DecodedCallstack;
		DecodeToNames( EncodedCallstack, DecodedCallstack );
		const FString Result = GetHumanReadable( DecodedCallstack );
		return Result;
	}

	/** Converts the encoded callstack into human readable callstack. */
	static FString GetHumanReadable( const TArray<FName>& DecodedCallstack )
	{
		FString Result;

		const int32 NumEntries = DecodedCallstack.Num();
		//for (int32 Index = DecodedCallstack.Num() - 1; Index >= 0; --Index)
		for (int32 Index = 0; Index < NumEntries; ++Index)
		{
			const FName LongName = DecodedCallstack[Index];
			const FString ShortName = FStatNameAndInfo::GetShortNameFrom( LongName ).ToString();
			//const FString Group = FStatNameAndInfo::GetGroupNameFrom( LongName ).ToString();
			FString Desc = FStatNameAndInfo::GetDescriptionFrom( LongName );
			Desc.Trim();

			if (Desc.Len() == 0)
			{
				Result += ShortName;
			}
			else
			{
				Result += Desc;
			}

			if (Index != NumEntries - 1)
			{
				Result += TEXT( " -> " );
			}
		}

		Result.ReplaceInline( TEXT( "STAT_" ), TEXT( "" ), ESearchCase::CaseSensitive );
		return Result;
	}

protected:
	/** Decodes encoded callstack to an array of strings. Where each string is the index of the FName. */
	static void DecodeToStrings( const FName& EncodedCallstack, TArray<FString>& out_DecodedCallstack )
	{
		EncodedCallstack.ToString().ParseIntoArray( out_DecodedCallstack, CallstackSeparator, true );
	}
};

const TCHAR* FStatsCallstack::CallstackSeparator = TEXT( "+" );

/*-----------------------------------------------------------------------------
	Allocation info
-----------------------------------------------------------------------------*/

FAllocationInfo::FAllocationInfo( uint64 InOldPtr, uint64 InPtr, int64 InSize, const TArray<FName>& InCallstack, uint32 InSequenceTag, EMemoryOperation InOp, bool bInHasBrokenCallstack ) 
	: OldPtr( InOldPtr )
	, Ptr( InPtr )
	, Size( InSize )
	, EncodedCallstack( *FStatsCallstack::Encode( InCallstack ) )
	, SequenceTag( InSequenceTag )
	, Op( InOp )
	, bHasBrokenCallstack( bInHasBrokenCallstack )
{

}

FAllocationInfo::FAllocationInfo( const FAllocationInfo& Other ) 
	: OldPtr( Other.OldPtr )
	, Ptr( Other.Ptr )
	, Size( Other.Size )
	, EncodedCallstack( Other.EncodedCallstack )
	, SequenceTag( Other.SequenceTag )
	, Op( Other.Op )
	, bHasBrokenCallstack( Other.bHasBrokenCallstack )
{

}

/*-----------------------------------------------------------------------------
	FNodeAllocationInfo
-----------------------------------------------------------------------------*/

void FNodeAllocationInfo::SortBySize()
{
	ChildNodes.ValueSort( FNodeAllocationInfoSizeGreater() );
	for (auto& It : ChildNodes)
	{
		It.Value->SortBySize();
	}
}


void FNodeAllocationInfo::PrepareCallstackData( const TArray<FName>& InDecodedCallstack )
{
	DecodedCallstack = InDecodedCallstack;
	EncodedCallstack = *FStatsCallstack::Encode( DecodedCallstack );
	HumanReadableCallstack = FStatsCallstack::GetHumanReadable( DecodedCallstack );
}

/*-----------------------------------------------------------------------------
	Stats stack helpers
-----------------------------------------------------------------------------*/

/** Holds stats stack state, used to preserve continuity when the game frame has changed. */
struct FStackState
{
	/** Default constructor. */
	FStackState()
		: bIsBrokenCallstack( false )
	{}

	/** Call stack. */
	TArray<FName> Stack;

	/** Current function name. */
	FName Current;

	/** Whether this callstack is marked as broken due to mismatched start and end scope cycles. */
	bool bIsBrokenCallstack;
};