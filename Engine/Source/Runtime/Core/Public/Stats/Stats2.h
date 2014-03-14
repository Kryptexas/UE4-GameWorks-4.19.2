// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnStats.h: Performance stats framework.
=============================================================================*/
#pragma once

/**
* This is thread-private information about the thread idle stats, which we always collect, even in final builds
*/
class CORE_API FThreadIdleStats : public FThreadSingleton<FThreadIdleStats>
{
	friend class FThreadSingleton<FThreadIdleStats>;

	FThreadIdleStats()
		: Waits(0)
	{}

public:

	/** Total cycles we waited for sleep or event. **/
	uint32 Waits;

	struct FScopeIdle
	{
		uint32 Start;
		FScopeIdle()
			: Start(FPlatformTime::Cycles())
		{
		}
		~FScopeIdle()
		{
			FThreadIdleStats::Get().Waits += FPlatformTime::Cycles() - Start;
		}
	};
};


#if STATS

struct TStatId
{
	FORCEINLINE TStatId()
		: StatIdPtr(&TStatId_NAME_None)
	{
	}
	FORCEINLINE TStatId(FName const* InStatIdPtr)
		: StatIdPtr(InStatIdPtr)
	{
	}
	FORCEINLINE FName operator*() const
	{
		return *StatIdPtr;
	}
	FORCEINLINE FName const* operator->() const
	{
		return StatIdPtr;
	}
	FORCEINLINE bool IsValidStat() const
	{
		return StatIdPtr != &TStatId_NAME_None;
	}
	FORCEINLINE FName const* GetRawPointer() const
	{
		return StatIdPtr;
	}
private:
	CORE_API static FName TStatId_NAME_None;
	FName const* StatIdPtr;
};

/**
 * For packet messages, this indicated what sort of thread timing we use. 
 * Game and Other use CurrentGameFrame, Renderer is CurrentRenderFrame
 */
namespace EThreadType
{
	enum Type
	{
		Invalid,
		Game,
		Renderer,
		Other,
	};
}


/**
 * What the type of the payload is
 */
struct EStatDataType
{
	enum Type
	{
		Invalid,
		ST_None,
		ST_int64,
		ST_double,
		ST_FName,

		Num,
		Mask = 0x7,
		Shift = 0,
		NumBits = 3
	};
};

/**
 * The operation being performed by this message
 */
struct EStatOperation
{
	enum Type
	{
		Invalid,
		SetLongName,
		AdvanceFrameEventGameThread,    
		AdvanceFrameEventRenderThread,  
		CycleScopeStart,
		CycleScopeEnd,
		CycleSetToNow,
		Set, 
		Clear,
		Add,
		Subtract,

		// these are special ones for processed data
		ChildrenStart,
		ChildrenEnd,
		Leaf,
		MaxVal,

		Num,
		Mask = 0xf,
		Shift = EStatDataType::Shift + EStatDataType::NumBits,
		NumBits = 4
	};
};

/**
 * Message flags
 */
struct EStatMetaFlags
{
	enum Type
	{
		Invalid							= 0x0,
		DummyAlwaysOne					= 0x1,  // this bit is always one and is used for error checking
		HasLongNameAndMetaInfo			= 0x2,  // if true, then this message contains all meta data as well and the name is long
		IsCycle							= 0x4,	// if true, then this message contains and int64 cycle or IsPackedCCAndDuration
		IsMemory						= 0x8,  // if true, then this message contains a memory stat
		IsPackedCCAndDuration			= 0x10, // if true, then this is actually two uint32s, the cycle count and the call count, see FromPackedCallCountDuration_Duration
		ShouldClearEveryFrame			= 0x20, // if true, then this stat is cleared every frame
		SendingFName					= 0x40, // used only on disk on on the wire, indicates that we serialized the fname string.

		Num								= 0x80,
		Mask							= 0xff,
		Shift = EStatOperation::Shift + EStatOperation::NumBits,
		NumBits = 8
	};
};

//@todo merge these two after we have redone the user end
/**
 * Wrapper for memory region
 */
struct EMemoryRegion
{
	enum Type
	{
		Invalid = FPlatformMemory::MCR_Invalid,

		Num = FPlatformMemory::MCR_MAX,
		Mask = 0xf,
		Shift = EStatMetaFlags::Shift + EStatMetaFlags::NumBits,
		NumBits = 4
	};
	checkAtCompileTime(FPlatformMemory::MCR_MAX < (1 << NumBits), need_to_expand_memoryregion_field);
};


/**
 * A few misc final bit packing computations
 */
namespace EStatAllFields
{
	enum Type
	{
		NumBits = EMemoryRegion::Shift + EMemoryRegion::NumBits,
		StartShift = 28 - NumBits,
	};
}

checkAtCompileTime(EStatAllFields::StartShift > 0, too_many_stat_fields);

// Pass a console command directly to the stats system, return true if it is known command, false means it might be a stats command
CORE_API bool DirectStatsCommand(const TCHAR* Cmd, bool bBlockForCompletion = false, FOutputDevice* Ar = nullptr);

FORCEINLINE int64 ToPackedCallCountDuration(uint32 CallCount, uint32 Duration)
{
	return (int64(CallCount) << 32) | Duration;
}

FORCEINLINE uint32 FromPackedCallCountDuration_CallCount(int64 Both)
{
	return uint32(Both >> 32);
}

FORCEINLINE uint32 FromPackedCallCountDuration_Duration(int64 Both)
{
	return uint32(Both & 0xffffffff);
}

/**
 * Helper class that stores an FName and all meta information in 8 bytes. Kindof icky.
 */
class FStatNameAndInfo
{
	/**
	 * An FName, but the high bits of the Number are used for other fields.
	 */
	FName NameAndInfo;
public:
	FORCEINLINE_STATS FStatNameAndInfo()
	{
	}

	/**
	 * Copy constructor
	 */
	FORCEINLINE_STATS FStatNameAndInfo(FStatNameAndInfo const& Other)
		: NameAndInfo(Other.NameAndInfo)
	{
		checkAtCompileTime(EStatAllFields::StartShift >= 0, too_many__fields);
		CheckInvariants();
	}

	/**
	 * Build from a raw FName
	 */
	FORCEINLINE_STATS FStatNameAndInfo(FName Other, bool bAlreadyHasMeta)
		: NameAndInfo(Other)
	{
		if (!bAlreadyHasMeta)
		{
			int32 Number = NameAndInfo.GetNumber();
			// ok, you can't have numbered stat FNames too large
			checkStats(!(Number >> EStatAllFields::StartShift));
			Number |= EStatMetaFlags::DummyAlwaysOne << (EStatMetaFlags::Shift + EStatAllFields::StartShift);
			NameAndInfo.SetNumber(Number);
		}
		CheckInvariants();
	}

	/**
	 * Build with stat metadata
	 */
	FORCEINLINE_STATS FStatNameAndInfo(FName InStatName, char const* InGroup, TCHAR const* InDescription, EStatDataType::Type InStatType, bool bShouldClearEveryFrame, bool bCycleStat, FPlatformMemory::EMemoryCounterRegion MemoryRegion = FPlatformMemory::MCR_Invalid)
		: NameAndInfo(ToLongName(InStatName, InGroup, InDescription))
	{
		int32 Number = NameAndInfo.GetNumber();
		// ok, you can't have numbered stat FNames too large
		checkStats(!(Number >> EStatAllFields::StartShift));
		Number |= (EStatMetaFlags::DummyAlwaysOne | EStatMetaFlags::HasLongNameAndMetaInfo) << (EStatMetaFlags::Shift + EStatAllFields::StartShift);
		NameAndInfo.SetNumber(Number);

		SetField<EStatDataType>(InStatType);
		SetFlag(EStatMetaFlags::ShouldClearEveryFrame, bShouldClearEveryFrame);
		SetFlag(EStatMetaFlags::IsCycle, bCycleStat);
		if (MemoryRegion != FPlatformMemory::MCR_Invalid)
		{
			SetFlag(EStatMetaFlags::IsMemory, true);
			SetField<EMemoryRegion>(EMemoryRegion::Type(MemoryRegion));
		}

		CheckInvariants();
	}

	/**
	 * Internal use, used by the deserializer
	 */
	FORCEINLINE_STATS void SetNumberDirect(int32 Number)
	{
		NameAndInfo.SetNumber(Number);
	}

	/**
	* Internal use, used by the serializer
	 */
	FORCEINLINE_STATS int32 GetRawNumber() const
	{
		CheckInvariants();
		int32 Number = NameAndInfo.GetNumber();
		return Number;
	}

	/**
	 * Internal use by FStatsThreadState to force an update to the long name
	 */
	FORCEINLINE_STATS void SetRawName(FName RawName)
	{
		// ok, you can't have numbered stat FNames too large
		checkStats(!(RawName.GetNumber() >> EStatAllFields::StartShift));
		CheckInvariants();
		int32 Number = NameAndInfo.GetNumber();
		Number &= ~((1 << EStatAllFields::StartShift) - 1);
		NameAndInfo = RawName;
		NameAndInfo.SetNumber(Number | RawName.GetNumber());
	}

	/**
	 * The encoded FName with the correct, original Number
	 */
	FORCEINLINE_STATS FName GetRawName() const
	{
		CheckInvariants();
		FName Result(NameAndInfo);
		int32 Number = NameAndInfo.GetNumber();
		Number &= ((1 << EStatAllFields::StartShift) - 1);
		Result.SetNumber(Number);
		return Result;
	}

	/**
	 * The encoded FName with the correct, original Number
	 */
	FORCEINLINE_STATS FName GetEncodedName() const
	{
		CheckInvariants();
		return NameAndInfo;
	}

	/**
	 * Expensive! Extracts the shortname if this is a long name or just returns the name
	 */
	FORCEINLINE_STATS FName GetShortName() const
	{
		CheckInvariants();
		return GetShortNameFrom(GetRawName());
	}

	/**
	 * Expensive! Extracts the group name if this is a long name or just returns none
	 */
	FORCEINLINE_STATS FName GetGroupName() const
	{
		CheckInvariants();
		return GetGroupNameFrom(GetRawName());
	}

	/**
	 * Expensive! Extracts the description if this is a long name or just returns the empty string
	 */
	FORCEINLINE_STATS void GetDescription(class FString& OutDescription) const
	{
		CheckInvariants();
		GetDescriptionFrom(GetRawName(), OutDescription);
	}

	/**
	 * Makes sure this object is in good shape
	 */
	FORCEINLINE_STATS void CheckInvariants() const
	{
		checkStats((NameAndInfo.GetNumber() & (EStatMetaFlags::DummyAlwaysOne << (EStatAllFields::StartShift + EStatMetaFlags::Shift)))
			&& NameAndInfo.GetIndex());
	}

	/**
	 * returns an encoded field
	 * @return the field
	 */
	template<typename TField>
	typename TField::Type GetField() const
	{
		CheckInvariants();
		int32 Number = NameAndInfo.GetNumber();
		Number = (Number >> (EStatAllFields::StartShift + TField::Shift)) & TField::Mask;
		checkStats(Number != TField::Invalid && Number < TField::Num);
		return typename TField::Type(Number);
	}

	/**
	 * sets an encoded field
	 * @param Value, value to set
	 */
	template<typename TField>
	void SetField(typename TField::Type Value)
	{
		int32 Number = NameAndInfo.GetNumber();
		CheckInvariants();
		checkStats(Value < TField::Num && Value != TField::Invalid);
		Number &= ~(TField::Mask << (EStatAllFields::StartShift + TField::Shift));
		Number |= Value << (EStatAllFields::StartShift + TField::Shift);
		NameAndInfo.SetNumber(Number);
		CheckInvariants();
	}

	/**
	 * returns an encoded flag
	 * @param Bit, flag to read
	 */
	bool GetFlag(EStatMetaFlags::Type Bit) const
	{
		int32 Number = NameAndInfo.GetNumber();
		CheckInvariants();
		checkStats(Bit < EStatMetaFlags::Num && Bit != EStatMetaFlags::Invalid);
		return !!((Number >> (EStatAllFields::StartShift + EStatMetaFlags::Shift)) & Bit);
	}

	/**
	 * sets an encoded flag
	 * @param Bit, flag to set
	 * @param Value, value to set
	 */
	void SetFlag(EStatMetaFlags::Type Bit, bool Value)
	{
		int32 Number = NameAndInfo.GetNumber();
		CheckInvariants();
		checkStats(Bit < EStatMetaFlags::Num && Bit != EStatMetaFlags::Invalid);
		if (Value)
		{
			Number |= (Bit << (EStatAllFields::StartShift + EStatMetaFlags::Shift));
		}
		else
		{
			Number &= ~(Bit << (EStatAllFields::StartShift + EStatMetaFlags::Shift));
		}
		NameAndInfo.SetNumber(Number);
		CheckInvariants();
	}


	/**
	 * Builds a long name from the three parts
	 * @param InStatName, Short name
	 * @param InGroup, Group name
	 * @param InDescription, Description
	 * @return the packed FName
	 */
	CORE_API static FName ToLongName(FName InStatName, char const* InGroup, TCHAR const* InDescription);
	CORE_API static FName GetShortNameFrom(FName InLongName);
	CORE_API static FName GetGroupNameFrom(FName InLongName);
	CORE_API static void GetDescriptionFrom(FName InLongName, class FString& OutDescription);
};


/** Union for easier debugging. */
union UStatData
{
private:
	/** For ST_double. */
	double	Float;
	/** For ST_int64 and IsCycle or IsMemory. */
	int64	Cycles;
	/** ST_int64 and IsPackedCCAndDuration. */
	int32	CCAndDuration[2];
};

/**
* 16 byte stat message. Everything is a message
*/
struct FStatMessage
{
	/**
	* Generic payload
	*/
	enum
	{
		DATA_SIZE=8,
		DATA_ALIGN=8,
	};
	union
	{
#if	UE_BUILD_DEBUG
		UStatData								DebugStatData;
#endif // UE_BUILD_DEBUG
		TAlignedBytes<DATA_SIZE, DATA_ALIGN>	StatData;
	};

	/**
	* Name and the meta info.
	*/
	FStatNameAndInfo						NameAndInfo;

	FStatMessage()
	{
	}

	/**
	* Copy constructor
	*/
	FORCEINLINE_STATS FStatMessage(FStatMessage const& Other)
		: StatData(Other.StatData)
		, NameAndInfo(Other.NameAndInfo)
	{
	}
	
	/**
	* Build a meta data message
	*/
	FStatMessage(FName InStatName, EStatDataType::Type InStatType, char const* InGroup, TCHAR const* InDescription, bool bShouldClearEveryFrame, bool bCycleStat, FPlatformMemory::EMemoryCounterRegion MemoryRegion = FPlatformMemory::MCR_Invalid)
		: NameAndInfo(InStatName, InGroup, InDescription, InStatType, bShouldClearEveryFrame, bCycleStat, MemoryRegion)
	{
		NameAndInfo.SetField<EStatOperation>(EStatOperation::SetLongName);
	}

	FORCEINLINE_STATS FStatMessage(FStatNameAndInfo InStatName)
		: NameAndInfo(InStatName)
	{
	}

	/**
	* Clock operation
	*/
	FORCEINLINE_STATS FStatMessage(FName InStatName, EStatOperation::Type InStatOperation)
		: NameAndInfo(InStatName, true)
	{
		NameAndInfo.SetField<EStatOperation>(InStatOperation);
		checkStats(NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64);
		checkStats(NameAndInfo.GetFlag(EStatMetaFlags::IsCycle) == true);

		// these branches are FORCEINLINE_STATS of constants in almost all cases, so they disappear
		if (InStatOperation == EStatOperation::CycleScopeStart || InStatOperation == EStatOperation::CycleScopeEnd || InStatOperation == EStatOperation::CycleSetToNow)
		{
			GetValue_int64()= int64(FPlatformTime::Cycles());
		}
		else
		{
			checkStats(0);
		}
	}

	/**
	* int64 operation
	*/
	explicit FORCEINLINE_STATS FStatMessage(FName InStatName, EStatOperation::Type InStatOperation, int64 Value, bool bIsCycle)
		: NameAndInfo(InStatName, true)
	{
		NameAndInfo.SetField<EStatOperation>(InStatOperation);
		checkStats(NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64);
		checkStats(NameAndInfo.GetFlag(EStatMetaFlags::IsCycle) == bIsCycle);
		GetValue_int64() = Value;
	}

	/**
	* double operation
	*/
	explicit FORCEINLINE_STATS FStatMessage(FName InStatName, EStatOperation::Type InStatOperation, double Value, bool)
		: NameAndInfo(InStatName, true)
	{
		NameAndInfo.SetField<EStatOperation>(InStatOperation);
		checkStats(NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_double);
		checkStats(NameAndInfo.GetFlag(EStatMetaFlags::IsCycle) == false);
		GetValue_double() = Value;
	}

	/**
	* name operation
	*/
	explicit FORCEINLINE_STATS FStatMessage(FName InStatName, EStatOperation::Type InStatOperation, FName Value, bool)
		: NameAndInfo(InStatName, true)
	{
		NameAndInfo.SetField<EStatOperation>(InStatOperation);
		checkStats(NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_FName);
		checkStats(NameAndInfo.GetFlag(EStatMetaFlags::IsCycle) == false);
		GetValue_FName() = Value;
	}

	/**
	* Clear any data type
	*/
	FORCEINLINE_STATS void Clear()
	{
		checkAtCompileTime(sizeof(uint64) == DATA_SIZE, bad_clear);
		*(int64*)&StatData = 0;
	}

	/**
	* Payload retrieval and setting methods
	*/
	FORCEINLINE_STATS int64& GetValue_int64()
	{
		checkAtCompileTime(sizeof(int64) <= DATA_SIZE && ALIGNOF(int64) <= DATA_ALIGN, bad_data_for_stat_message );
		checkStats(NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64);
		return *(int64*)&StatData;
	}
	FORCEINLINE_STATS int64 GetValue_int64() const
	{
		checkStats(NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64);
		return *(int64 const*)&StatData;
	}

	FORCEINLINE_STATS int64 GetValue_Duration() const
	{
		checkStats(NameAndInfo.GetFlag(EStatMetaFlags::IsCycle) && NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64);
		if (NameAndInfo.GetFlag(EStatMetaFlags::IsPackedCCAndDuration))
		{
			return FromPackedCallCountDuration_Duration(*(int64 const*)&StatData);
		}
		return *(int64 const*)&StatData;
	}

	FORCEINLINE_STATS uint32 GetValue_CallCount() const
	{
		checkStats(NameAndInfo.GetFlag(EStatMetaFlags::IsPackedCCAndDuration) && NameAndInfo.GetFlag(EStatMetaFlags::IsCycle) && NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64);
		return FromPackedCallCountDuration_CallCount(*(int64 const*)&StatData);
	}

	FORCEINLINE_STATS double& GetValue_double()
	{
		checkAtCompileTime(sizeof(double) <= DATA_SIZE && ALIGNOF(double) <= DATA_ALIGN, bad_data_for_stat_message );
		checkStats(NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_double);
		return *(double*)&StatData;
	}

	FORCEINLINE_STATS double GetValue_double() const
	{
		checkStats(NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_double);
		return *(double const*)&StatData;
	}

	FORCEINLINE_STATS FName& GetValue_FName()
	{
		checkAtCompileTime(sizeof(FName) <= DATA_SIZE && ALIGNOF(FName) <= DATA_ALIGN, bad_data_for_stat_message );
		checkStats(NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_FName);
		return *(FName*)&StatData;
	}

	FORCEINLINE_STATS FName GetValue_FName() const
	{
		checkStats(NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_FName);
		return *(FName const*)&StatData;
	}
};
template<> struct TIsPODType<FStatMessage> { enum { Value = true }; };

CORE_API void GetPermanentStats(TArray<FStatMessage>& OutStats);

/**
 *	Based on FStatMessage, but supports more than 8 bytes of stat data storage.
 */
template< typename TEnum >
struct TStatMessage
{
	typedef TEnum TStructEnum;
	static const int32 EnumCount = TEnum::Num;

	/**
	* Generic payload
	*/
	enum
	{
		DATA_SIZE=8*EnumCount,
		DATA_ALIGN=8,
	};

	union
	{
#if	UE_BUILD_DEBUG
		UStatData								DebugStatData[EnumCount];
#endif // UE_BUILD_DEBUG
		TAlignedBytes<DATA_SIZE, DATA_ALIGN>	StatData;
	};

	/**
	* Name and the meta info.
	*/
	FStatNameAndInfo							NameAndInfo;

	TStatMessage()
	{}

	/**
	* Copy constructor from FStatMessage
	*/
	explicit FORCEINLINE_STATS TStatMessage(const FStatMessage& Other)
		: NameAndInfo(Other.NameAndInfo)
	{
		// Reset data type and clear all fields.
		NameAndInfo.SetField<EStatDataType>( EStatDataType::ST_None );
		Clear();
	}

	/**
	* Copy constructor
	*/
	explicit FORCEINLINE_STATS TStatMessage(const TStatMessage& Other)
		: NameAndInfo(Other.NameAndInfo)
	{
		// Copy all fields.
		for( int32 FieldIndex = 0; FieldIndex < EnumCount; ++FieldIndex )
		{
			*((int64*)&StatData+FieldIndex) = *((int64*)&Other.StatData+FieldIndex);
		}
	}

	/** Assignment operator for FStatMessage. */
	TStatMessage& operator=(const FStatMessage& Other)
	{
		NameAndInfo = Other.NameAndInfo;
		// Reset data type and clear all fields.
		NameAndInfo.SetField<EStatDataType>( EStatDataType::ST_None );
		Clear();
		return *this;
	}

	/** Fixes stat data type for all fields. */
	void FixStatData( const EStatDataType::Type NewType )
	{
		const EStatDataType::Type OldType = NameAndInfo.GetField<EStatDataType>();
		if( OldType != NewType )
		{
			// Convert from the old type to the new type.
			if( OldType == EStatDataType::ST_int64 && NewType == EStatDataType::ST_double )
			{
				// Get old values.
				int64 OldValues[TEnum::Num];
				for( int32 FieldIndex = 0; FieldIndex < EnumCount; ++FieldIndex )
				{
					OldValues[FieldIndex] = GetValue_int64((typename TEnum::Type)FieldIndex);
				}
				
				// Set new stat data type
				NameAndInfo.SetField<EStatDataType>(NewType);
				for( int32 FieldIndex = 0; FieldIndex < EnumCount; ++FieldIndex )
				{
					GetValue_double((typename TEnum::Type)FieldIndex) = (double)OldValues[FieldIndex];
				}
			}
			else if( OldType == EStatDataType::ST_double && NewType == EStatDataType::ST_int64 )
			{
				// Get old values.
				double OldValues[TEnum::Num];
				for( int32 FieldIndex = 0; FieldIndex < EnumCount; ++FieldIndex )
				{
					OldValues[FieldIndex] = GetValue_double((typename TEnum::Type)FieldIndex);
				}

				// Set new stat data type
				NameAndInfo.SetField<EStatDataType>(NewType);
				for( int32 FieldIndex = 0; FieldIndex < EnumCount; ++FieldIndex )
				{
					GetValue_int64((typename TEnum::Type)FieldIndex) = (int64)OldValues[FieldIndex];
				}
			}
		}
	}

	/**
	* Clear any data type
	*/
	FORCEINLINE_STATS void Clear()
	{
		checkAtCompileTime(sizeof(uint64) == DATA_SIZE/EnumCount, bad_clear);

		for( int32 FieldIndex = 0; FieldIndex < EnumCount; ++FieldIndex )
		{
			*((int64*)&StatData+FieldIndex) = 0;
		}
	}

	/**
	* Payload retrieval and setting methods
	*/
	FORCEINLINE_STATS int64& GetValue_int64( typename TEnum::Type Index )
	{
		checkAtCompileTime(sizeof(int64) <= DATA_SIZE && ALIGNOF(int64) <= DATA_ALIGN, bad_data_for_stat_message );
		checkStats(NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64);
		checkStats(Index<EnumCount);
		int64& Value = *((int64*)&StatData+(uint32)Index);
		return Value;
	}
	FORCEINLINE_STATS int64 GetValue_int64( typename TEnum::Type Index ) const
	{
		checkStats(NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64);
		checkStats(Index<EnumCount);
		const int64 Value = *((int64*)&StatData+(uint32)Index);
		return Value;
	}

	FORCEINLINE_STATS int64 GetValue_Duration( typename TEnum::Type Index ) const
	{
		checkStats(NameAndInfo.GetFlag(EStatMetaFlags::IsCycle) && NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64);
		checkStats(Index<EnumCount);
		if (NameAndInfo.GetFlag(EStatMetaFlags::IsPackedCCAndDuration))
		{
			const uint32 Value = FromPackedCallCountDuration_Duration(*((int64 const*)&StatData+(uint32)Index));
			return Value;
		}
		const int64 Value = *((int64 const*)&StatData+(uint32)Index);
		return Value;
	}

	FORCEINLINE_STATS uint32 GetValue_CallCount( typename TEnum::Type Index ) const
	{
		checkStats(NameAndInfo.GetFlag(EStatMetaFlags::IsPackedCCAndDuration) && NameAndInfo.GetFlag(EStatMetaFlags::IsCycle) && NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64);
		checkStats(Index<EnumCount);
		const uint32 Value = FromPackedCallCountDuration_CallCount(*((int64 const*)&StatData+(uint32)Index));
		return Value;
	}

	FORCEINLINE_STATS double& GetValue_double( typename TEnum::Type Index )
	{
		checkAtCompileTime(sizeof(double) <= DATA_SIZE && ALIGNOF(double) <= DATA_ALIGN, bad_data_for_stat_message );
		checkStats(Index<EnumCount);
		checkStats(NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_double);
		double& Value = *((double*)&StatData+(uint32)Index);
		return Value;
	}

	FORCEINLINE_STATS double GetValue_double( typename TEnum::Type Index ) const
	{
		checkStats(NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_double);
		checkStats(Index<EnumCount);
		const double Value = *((double const*)&StatData+(uint32)Index);
		return Value;
	}

	FORCEINLINE_STATS FString GetDescription() const
	{
		FString Result;			
		NameAndInfo.GetDescription(Result);
		return Result;
	}
};

/** Enumerates fields of the FComplexStatMessage. */
struct EComplexStatField
{
	enum Type
	{
		/** Summed inclusive time. */
		IncSum,
		/** Average inclusive time. */
		IncAve,
		/** Maximum inclusive time. */
		IncMax,
		/** Summed exclusive time. */
		ExcSum,
		/** Average exclusive time. */
		ExcAve,
		/** Maximum exclusive time. */
		ExcMax,
		/** Number of enumerates. */
		Num,
	};
};

/**
 *	This type of stat message holds data defined by associated enumeration @see EComplexStatField
 *	By default any of these messages contain a valid data, so check before accessing the data.
 */
typedef TStatMessage<EComplexStatField> FComplexStatMessage;

template<> struct TIsPODType<FComplexStatMessage> { enum { Value = true }; };

/**
* A stats packet. Sent between threads. Includes and array of messages and some information about the thread. 
*/
struct FStatPacket
{
	/** Assigned later, this is the frame number this packet is for **/
	int64 Frame;
	/** ThreadId this packet came from **/
	uint32 ThreadId;
	/** type of thread this packet came from **/
	EThreadType::Type ThreadType;
	/** true if this packet has broken callstacks **/
	bool bBrokenCallstacks;
	/** messages in this packet **/
	TArray<FStatMessage> StatMessages;
	/** Size we presize the message buffer to, currently the max of what we have seen so far. **/
	TArray<int32> StatMessagesPresize;

	/** constructor **/
	FStatPacket()
		: Frame(0)
		, ThreadId(0)
		, ThreadType(EThreadType::Invalid)
		, bBrokenCallstacks(false)
	{
	}

	/** Copy constructor. !!!CAUTION!!! does not copy the data **/
	FStatPacket(FStatPacket const& Other)
		: Frame(Other.Frame)
		, ThreadId(Other.ThreadId)
		, ThreadType(Other.ThreadType)
		, bBrokenCallstacks(false)
		, StatMessagesPresize(Other.StatMessagesPresize)
	{
	}
};

/**
* This is thread-private information about the stats we are acquiring. Pointers to these objects are held in TLS.
*/
class FThreadStats
{
	/** Used to control when we are collecting stats. User of the stats system increment and decrement this counter as they need data. **/
	CORE_API static FThreadSafeCounter MasterEnableCounter;
	/** Every time bMasterEnable changes, we update this. This is used to determine frames that have complete data. **/
	CORE_API static FThreadSafeCounter MasterEnableUpdateNumber;
	/** while bMasterEnable (or other things affecting stat collection) is chaning, we lock this. This is used to determine frames that have complete data. **/
	CORE_API static FThreadSafeCounter MasterDisableChangeTagLock;
	/** Computed by CheckEnable, the current "master control" for stats collection, based on MasterEnableCounter and a few other things. **/
	CORE_API static bool bMasterEnable;
	/** Set to permanently disable the stats system. **/
	CORE_API static bool bMasterDisableForever;
	/** TLS slot that holds a FThreadStats. **/
	CORE_API static uint32 TlsSlot;

	friend class FStatsThread;

	/** Tracks current stack depth for cycle counters. **/
	uint32 ScopeCount;
	/** Tracks current stack depth for cycle counters. **/
	bool bSawExplicitFlush;
	/** Tracks current stack depth for cycle counters. **/
	uint32 bWaitForExplicitFlush;
	/** The data we are eventually going to send to the stats thread. **/
	FStatPacket Packet;

	/** Gathers information about the current thread and sets upt eh TLS value. **/
	CORE_API FThreadStats();
	/** Copys a packet for sending. !!!CAUTION!!! does not copy packets.**/
	CORE_API FThreadStats(FThreadStats const& Other);

	/** Checks the TLS for a thread packet and if it isn't found, it makes a new one. **/
	static FORCEINLINE_STATS FThreadStats* GetThreadStats()
	{
		FThreadStats* Stats = (FThreadStats*)FPlatformTLS::GetTlsValue(TlsSlot);
		if (!Stats)
		{
			Stats = new FThreadStats();
		}
		return Stats;
	}

	/** This should be called when conditions have changed such that stat collection may now be enabled or not **/
	static CORE_API void CheckEnable();

	/** Send any outstanding packets to the stats thread **/
	CORE_API void Flush(bool bHasBrokenCallstacks = false);

public:
	/** This should be called when a thread exits, this deletes FThreadStats from the heap and TLS. **/
	static void Shutdown()
	{
		FThreadStats* Stats = (FThreadStats*)FPlatformTLS::GetTlsValue(TlsSlot);
		if (Stats)
		{
			FPlatformTLS::SetTlsValue(TlsSlot, NULL);
			delete Stats;
		}
	}

	/** Used to send messages that came in before the thread was ready. Adds em and flushes **/
	static void AddMessages(TArray<FStatMessage> const& Messages)
	{
		if (WillEverCollectData())
		{
			FThreadStats* ThreadStats = GetThreadStats();
			ThreadStats->Packet.StatMessages += Messages;
			ThreadStats->Flush();
		}
	}

	/** Clock operation. **/
	static FORCEINLINE_STATS void AddMessage(FName InStatName, EStatOperation::Type InStatOperation)
	{
		checkStats((InStatOperation == EStatOperation::CycleScopeStart || InStatOperation == EStatOperation::CycleScopeEnd || InStatOperation == EStatOperation::CycleSetToNow));
		FThreadStats* ThreadStats = GetThreadStats();
		// these branches are handled by the optimizer
		if (InStatOperation == EStatOperation::CycleScopeStart)
		{
			ThreadStats->ScopeCount++;
			new (ThreadStats->Packet.StatMessages) FStatMessage(InStatName, InStatOperation);

			// Emit named event for active cycle stat.
			if( GCycleStatsShouldEmitNamedEvents )
			{
				FPlatformMisc::BeginNamedEvent(FColor(0), InStatName.GetPlainANSIString());
			}
		}
		else if (InStatOperation == EStatOperation::CycleScopeEnd)
		{
			if (ThreadStats->ScopeCount > ThreadStats->bWaitForExplicitFlush)
			{
				new (ThreadStats->Packet.StatMessages) FStatMessage(InStatName, InStatOperation);
				ThreadStats->ScopeCount--;
				if (!ThreadStats->ScopeCount)
				{
					ThreadStats->Flush();
				}
			}
			// else we dumped this frame without closing scope, so we just drop the closes on the floor

			// End named event for active cycle stat.
			if( GCycleStatsShouldEmitNamedEvents )
			{
				FPlatformMisc::EndNamedEvent();
			}
		}
	}

	/** Any non-clock operation with an ordinary payload. **/
	template<typename TValue>
	static FORCEINLINE_STATS void AddMessage(FName InStatName, EStatOperation::Type InStatOperation, TValue Value, bool bIsCycle = false)
	{
		if (!InStatName.IsNone() && WillEverCollectData())
		{
			FThreadStats* ThreadStats = GetThreadStats();
			new (ThreadStats->Packet.StatMessages) FStatMessage(InStatName, InStatOperation, Value, bIsCycle);
			if (!ThreadStats->ScopeCount) // we can't guarantee other threads will ever be flushed, so we need to flush ever counter item!
			{
				ThreadStats->Flush();
			}
		}
	}

	/** 
	 * Used to force a flush at the next available opportunity. This is not useful for threads other than the main and render thread. 
	 * if DiscardCallstack is true, we also dump call stacks, making the next available opportunity at the next stat or stat close.
	**/
	static CORE_API void ExplicitFlush(bool DiscardCallstack = false);

	/** Return true if we are currently collecting data **/
	static FORCEINLINE_STATS bool IsCollectingData()
	{
		return bMasterEnable;
	}
	static FORCEINLINE_STATS bool IsCollectingData(TStatId StatId)
	{
		// we don't test StatId for NULL here because we assume it is non-null. If it is NULL, that indicates a problem with higher level code.
		return !StatId->IsNone() && IsCollectingData();
	}

	/** Return true if we are currently collecting data **/
	static FORCEINLINE_STATS bool WillEverCollectData()
	{
		return !bMasterDisableForever;
	}

	/** Return true if the threading is ready **/
	static FORCEINLINE_STATS bool IsThreadingReady()
	{
		return !!TlsSlot;
	}

	/** Indicate that you would like the system to begin collecting data, if it isn't already collecting data. Think reference count. **/
	static FORCEINLINE_STATS void MasterEnableAdd(int32 Value = 1)
	{
		MasterEnableCounter.Add(Value);
		CheckEnable();
	}

	/** Indicate that you no longer need stat data, if nobody else needs stat data, then no stat data will be collected. Think reference count. **/
	static FORCEINLINE_STATS void MasterEnableSubtract(int32 Value = 1)
	{
		MasterEnableCounter.Subtract(Value);
		CheckEnable();
	}

	/** Indicate that you no longer need stat data, forever. **/
	static FORCEINLINE_STATS void MasterDisableForever()
	{
		bMasterDisableForever = true;
		CheckEnable();
	}

	/** This is called before we start to change something that will invalidate. **/
	static FORCEINLINE_STATS void MasterDisableChangeTagLockAdd(int32 Value = 1)
	{
		MasterDisableChangeTagLock.Add(Value);
		FPlatformMisc::MemoryBarrier();
		MasterEnableUpdateNumber.Increment();
	}

	/** Indicate that you no longer need stat data, if nobody else needs stat data, then no stat data will be collected. Think reference count. **/
	static FORCEINLINE_STATS void MasterDisableChangeTagLockSubtract(int32 Value = 1)
	{
		FPlatformMisc::MemoryBarrier();
		MasterEnableUpdateNumber.Increment();
		FPlatformMisc::MemoryBarrier();
		MasterDisableChangeTagLock.Subtract(Value);
	}

	/** Everytime master enable changes, this number increases. This is used to determine full frames. **/
	static FORCEINLINE_STATS int32 MasterDisableChangeTag()
	{
		if (MasterDisableChangeTagLock.GetValue())
		{
			// while locked we are continually invalid, so we will just keep giving unique numbers
			return MasterEnableUpdateNumber.Increment();
		}
		return MasterEnableUpdateNumber.GetValue();
	}

	/** Call this if something disrupts data gathering. For example when the render thread is killed, data is abandoned.**/
	static FORCEINLINE_STATS void FrameDataIsIncomplete()
	{
		FPlatformMisc::MemoryBarrier();
		MasterEnableUpdateNumber.Increment();
		FPlatformMisc::MemoryBarrier();
	}

	/** Called by launch engine loop to start the stats thread **/
	static CORE_API void StartThread();
	/** Called by launch engine loop to stop the stats thread **/
	static CORE_API void StopThread();
	/** Called by the engine loop to make sure the stats thread isn't getting too far behind. **/
	static CORE_API void WaitForStats();
};


/**
 * This is a utility class for counting the number of cycles during the
 * lifetime of the object. It creates messages for the stats thread.
 */
class FCycleCounter
{
	/** Name of the stat, usually a short name **/
	FName StatId;

public:

	/**
	 * Pushes the specified stat onto the hierarchy for this thread. Starts
	 * the timing of the cycles used
	 */
	FORCEINLINE_STATS void Start(FName InStatId)
	{
		StatId = InStatId;
		FThreadStats::AddMessage(InStatId, EStatOperation::CycleScopeStart);
	}

	/**
	 * Stops the capturing and stores the result
	 */
	FORCEINLINE_STATS void Stop()
	{
		if (!StatId.IsNone())
		{
			FThreadStats::AddMessage(StatId, EStatOperation::CycleScopeEnd);
		}
	}
};


/**
* Single interface to control high performance stat disable
*/
class IStatGroupEnableManager
{
public:
	/** Return the singleton, must be called from the main thread. **/
	CORE_API static IStatGroupEnableManager& Get();

	/** virtual destructor. **/
	virtual ~IStatGroupEnableManager() 
	{
	}

	/**
	 * Returns a pointer to a bool (valid forever) that determines if this group is active
	 * This should be CACHED. We will get a few calls from different stats and different threads and stuff, but once things are "warmed up", this should NEVER be called.
	 * @param Group, group to look up
	 * @param bDefaultEnable, If this is the first time this group has been set up, this sets the default enable value for this group.
	 * @param bCanBeDisabled, If this is true, this is a memory counter or something and can never be disabled
	 * @return a pointer to a FName (valid forever) that determines if this group is active
	 */
	virtual TStatId GetHighPerformanceEnableForStat(FName StatShortName, const char* InGroup, bool bDefaultEnable, bool bCanBeDisabled, EStatDataType::Type InStatType, TCHAR const* InDescription, bool bCycleStat, FPlatformMemory::EMemoryCounterRegion MemoryRegion = FPlatformMemory::MCR_Invalid)=0;

	/**
	 * Enables or disabled a particular group of stats
	 * Disabling a memory group, ever, is usually a bad idea
	 * @param Group, group to look up
	 * @param Enable, this should be true if we want to collect stats for this group
	 */
	virtual void SetHighPerformanceEnableForGroup(FName Group, bool Enable)=0;

	/**
	 * Enables or disabled all groups of stats
	 * Disabling a memory group, ever, is usually a bad idea. SO if you disable all groups, you will wreck memory stats usually.
	 * @param Enable, this should be true if we want to collect stats for all groups
	 */
	virtual void SetHighPerformanceEnableForAllGroups(bool Enable)=0;

	/**
	 * Resets all stats to their default collection state, which was set when they were looked up intially
	 */
	virtual void ResetHighPerformanceEnableForAllGroups()=0;

	/**
	 * Runs a group command
	 * @param Cmd, Command to run
	 */
	virtual void StatGroupEnableManagerCommand(FString const& Cmd)=0;
};


/**
**********************************************************************************
*/
struct FThreadSafeStaticStatBase
{
protected:
	mutable FName* HighPerformanceEnable; // must be uninitialized, because we need atomic initialization
	CORE_API void DoSetup(const char* InStatName, const TCHAR* InStatDesc, const char* GroupName, const TCHAR* InGroupDesc, bool bDefaultEnable, bool bCanBeDisabled, EStatDataType::Type InStatType, bool IsCycleStat, FPlatformMemory::EMemoryCounterRegion InMemoryRegion) const;
};

template<class TStatData, bool TCompiledIn>
struct FThreadSafeStaticStatInner : public FThreadSafeStaticStatBase
{
	FORCEINLINE TStatId GetStatId() const
	{
		checkAtCompileTime(sizeof(HighPerformanceEnable) == sizeof(TStatId), unsafe_cast_requires_these_to_be_the_same_thing);
		if (!HighPerformanceEnable)
		{
			DoSetup(TStatData::GetStatName(), TStatData::GetDescription(), TStatData::TGroup::GetGroupName(), TStatData::TGroup::GetDescription(), TStatData::TGroup::IsDefaultEnabled(), TStatData::IsClearEveryFrame(), TStatData::GetStatType(), TStatData::IsCycleStat(), TStatData::GetMemoryRegion() );
		}
		return *(TStatId*)(&HighPerformanceEnable);
	}
	FORCEINLINE FName GetStatFName() const
	{
		return *GetStatId();
	}
};

template<class TStatData>
struct FThreadSafeStaticStatInner<TStatData, false>
{
	FORCEINLINE TStatId GetStatId()
	{
		return TStatId();
	}
	FORCEINLINE FName GetStatFName() const
	{
		return FName();
	}
};

template<class TStatData>
struct FThreadSafeStaticStat : public FThreadSafeStaticStatInner<TStatData, TStatData::TGroup::CompileTimeEnable>
{
};

#define DECLARE_STAT_GROUP(Description, StatName, InDefaultEnable, InCompileTimeEnable) \
struct FStatGroup_##StatName\
{ \
	enum \
	{ \
		DefaultEnable = InDefaultEnable, \
		CompileTimeEnable = InCompileTimeEnable \
	}; \
	static FORCEINLINE const char* GetGroupName() \
	{ \
		return #StatName; \
	} \
	static FORCEINLINE const TCHAR* GetDescription() \
	{ \
		return Description; \
	} \
	static FORCEINLINE bool IsDefaultEnabled() \
	{ \
		return (bool)DefaultEnable; \
	} \
	static FORCEINLINE bool IsCompileTimeEnable() \
	{ \
		return (bool)CompileTimeEnable; \
	} \
};

#define DECLARE_STAT(Description, StatName, GroupName, StatType, bShouldClearEveryFrame, bCycleStat, MemoryRegion) \
struct FStat_##StatName\
{ \
	typedef FStatGroup_##GroupName TGroup; \
	static FORCEINLINE const char* GetStatName() \
	{ \
		return #StatName; \
	} \
	static FORCEINLINE const TCHAR* GetDescription() \
	{ \
		return Description; \
	} \
	static FORCEINLINE EStatDataType::Type GetStatType() \
	{ \
		return StatType; \
	} \
	static FORCEINLINE bool IsClearEveryFrame() \
	{ \
		return bShouldClearEveryFrame; \
	} \
	static FORCEINLINE bool IsCycleStat() \
	{ \
		return bCycleStat; \
	} \
	static FORCEINLINE FPlatformMemory::EMemoryCounterRegion GetMemoryRegion() \
	{ \
		return MemoryRegion; \
	} \
};

#define GET_STATID(Stat) (StatPtr_##Stat.GetStatId())
#define GET_STATFNAME(Stat) (StatPtr_##Stat.GetStatFName())

#define STAT_GROUP_TO_FStatGroup(Group) FStatGroup_##Group

#define STAT_IS_COLLECTING(Stat) (FThreadStats::IsCollectingData(GET_STATID(Stat)))

#define DEFINE_STAT(Stat) \
	struct FThreadSafeStaticStat<FStat_##Stat> StatPtr_##Stat;

#define RETURN_QUICK_DECLARE_CYCLE_STAT(StatId,GroupId) \
	DECLARE_STAT(TEXT(#StatId),StatId,GroupId,EStatDataType::ST_int64, true, true, FPlatformMemory::MCR_Invalid); \
	static DEFINE_STAT(StatId) \
	return GET_STATID(StatId);

#define DECLARE_CYCLE_STAT(CounterName,StatId,GroupId) \
	DECLARE_STAT(CounterName,StatId,GroupId,EStatDataType::ST_int64, true, true, FPlatformMemory::MCR_Invalid); \
	static DEFINE_STAT(StatId)
#define DECLARE_FLOAT_COUNTER_STAT(CounterName,StatId,GroupId) \
	DECLARE_STAT(CounterName,StatId,GroupId,EStatDataType::ST_double, true, false, FPlatformMemory::MCR_Invalid); \
	static DEFINE_STAT(StatId)
#define DECLARE_DWORD_COUNTER_STAT(CounterName,StatId,GroupId) \
	DECLARE_STAT(CounterName,StatId,GroupId,EStatDataType::ST_int64, true, false, FPlatformMemory::MCR_Invalid); \
	static DEFINE_STAT(StatId)
#define DECLARE_FLOAT_ACCUMULATOR_STAT(CounterName,StatId,GroupId) \
	DECLARE_STAT(CounterName,StatId,GroupId,EStatDataType::ST_double, false, false, FPlatformMemory::MCR_Invalid); \
	static DEFINE_STAT(StatId)
#define DECLARE_DWORD_ACCUMULATOR_STAT(CounterName,StatId,GroupId) \
	DECLARE_STAT(CounterName,StatId,GroupId,EStatDataType::ST_int64, false, false, FPlatformMemory::MCR_Invalid); \
	static DEFINE_STAT(StatId)
#define DECLARE_MEMORY_STAT(CounterName,StatId,GroupId) \
	DECLARE_STAT(CounterName,StatId,GroupId,EStatDataType::ST_int64, false, false, FPlatformMemory::MCR_Physical); \
	static DEFINE_STAT(StatId)
#define DECLARE_MEMORY_STAT_POOL(CounterName,StatId,GroupId,Pool) \
	DECLARE_STAT(CounterName,StatId,GroupId,EStatDataType::ST_int64, false, false, Pool); \
	static DEFINE_STAT(StatId)

#define DECLARE_CYCLE_STAT_EXTERN(CounterName,StatId,GroupId, APIX) \
	DECLARE_STAT(CounterName,StatId,GroupId,EStatDataType::ST_int64, true, true, FPlatformMemory::MCR_Invalid); \
	extern APIX DEFINE_STAT(StatId);
#define DECLARE_FLOAT_COUNTER_STAT_EXTERN(CounterName,StatId,GroupId, API) \
	DECLARE_STAT(CounterName,StatId,GroupId,EStatDataType::ST_double, true, false, FPlatformMemory::MCR_Invalid); \
	extern API DEFINE_STAT(StatId);
#define DECLARE_DWORD_COUNTER_STAT_EXTERN(CounterName,StatId,GroupId, API) \
	DECLARE_STAT(CounterName,StatId,GroupId,EStatDataType::ST_int64, true, false, FPlatformMemory::MCR_Invalid); \
	extern API DEFINE_STAT(StatId);
#define DECLARE_FLOAT_ACCUMULATOR_STAT_EXTERN(CounterName,StatId,GroupId, API) \
	DECLARE_STAT(CounterName,StatId,GroupId,EStatDataType::ST_double, false, false, FPlatformMemory::MCR_Invalid); \
	extern API DEFINE_STAT(StatId);
#define DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(CounterName,StatId,GroupId, API) \
	DECLARE_STAT(CounterName,StatId,GroupId,EStatDataType::ST_int64, false, false, FPlatformMemory::MCR_Invalid); \
	extern API DEFINE_STAT(StatId);
#define DECLARE_MEMORY_STAT_EXTERN(CounterName,StatId,GroupId, API) \
	DECLARE_STAT(CounterName,StatId,GroupId,EStatDataType::ST_int64, false, false, FPlatformMemory::MCR_Physical); \
	extern API DEFINE_STAT(StatId);
#define DECLARE_MEMORY_STAT_POOL_EXTERN(CounterName,StatId,GroupId,Pool, API) \
	DECLARE_STAT(CounterName,StatId,GroupId,EStatDataType::ST_int64, false, false, Pool); \
	extern API DEFINE_STAT(StatId);

/** Macro for declaring group factory instances */
#define DECLARE_STATS_GROUP(GroupDesc,GroupId) \
	DECLARE_STAT_GROUP(GroupDesc, GroupId, true, true);

#define DECLARE_STATS_GROUP_VERBOSE(GroupDesc, GroupId) \
	DECLARE_STAT_GROUP(GroupDesc, GroupId, false, true);

#define DECLARE_STATS_GROUP_MAYBE_COMPILED_OUT(GroupDesc, GroupId, CompileIn) \
	DECLARE_STAT_GROUP(GroupDesc, GroupId, false, CompileIn);


#define DECLARE_SCOPE_CYCLE_COUNTER(CounterName,StatId,GroupId) \
	DECLARE_STAT(CounterName,StatId,GroupId,EStatDataType::ST_int64, true, true, FPlatformMemory::MCR_Invalid); \
	static DEFINE_STAT(StatId) \
	FScopeCycleCounter CycleCount_##StatId(GET_STATID(StatId));

#define QUICK_SCOPE_CYCLE_COUNTER(Stat) \
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT(#Stat),Stat,STATGROUP_Quick)

#define SCOPE_CYCLE_COUNTER(Stat) \
	FScopeCycleCounter CycleCount_##Stat(GET_STATID(Stat));

#define CONDITIONAL_SCOPE_CYCLE_COUNTER(Stat,bCondition) \
	FScopeCycleCounter CycleCount_##Stat(bCondition ? GET_STATID(Stat) : TStatId());



#define SET_CYCLE_COUNTER(Stat,Cycles) \
{\
	FThreadStats::AddMessage(GET_STATFNAME(Stat), EStatOperation::Set, int64(Cycles), true);\
}

#define INC_DWORD_STAT(Stat) \
{\
	FThreadStats::AddMessage(GET_STATFNAME(Stat), EStatOperation::Add, int64(1));\
}
#define INC_FLOAT_STAT_BY(Stat, Amount) \
{\
	FThreadStats::AddMessage(GET_STATFNAME(Stat), EStatOperation::Add, double(Amount));\
}
#define INC_DWORD_STAT_BY(Stat, Amount) \
{\
	FThreadStats::AddMessage(GET_STATFNAME(Stat), EStatOperation::Add, int64(Amount));\
}
#define INC_DWORD_STAT_FNAME_BY(StatFName, Amount) \
{\
	FThreadStats::AddMessage(StatFName, EStatOperation::Add, int64(Amount));\
}
#define INC_MEMORY_STAT_BY(Stat, Amount) \
{\
	FThreadStats::AddMessage(GET_STATFNAME(Stat), EStatOperation::Add, int64(Amount));\
}
#define DEC_DWORD_STAT(Stat) \
{\
	FThreadStats::AddMessage(GET_STATFNAME(Stat), EStatOperation::Subtract, int64(1));\
}
#define DEC_FLOAT_STAT_BY(Stat,Amount) \
{\
	FThreadStats::AddMessage(GET_STATFNAME(Stat), EStatOperation::Subtract, double(Amount));\
}
#define DEC_DWORD_STAT_BY(Stat,Amount) \
{\
	FThreadStats::AddMessage(GET_STATFNAME(Stat), EStatOperation::Subtract, int64(Amount));\
}
#define DEC_DWORD_STAT_FNAME_BY(StatFName,Amount) \
{\
	FThreadStats::AddMessage(StatFName, EStatOperation::Subtract, int64(Amount));\
}
#define DEC_MEMORY_STAT_BY(Stat,Amount) \
{\
	FThreadStats::AddMessage(GET_STATFNAME(Stat), EStatOperation::Subtract, int64(Amount));\
}
#define SET_MEMORY_STAT(Stat,Value) \
{\
	FThreadStats::AddMessage(GET_STATFNAME(Stat), EStatOperation::Set, int64(Value));\
}
#define SET_DWORD_STAT(Stat,Value) \
{\
	FThreadStats::AddMessage(GET_STATFNAME(Stat), EStatOperation::Set, int64(Value));\
}
#define SET_FLOAT_STAT(Stat,Value) \
{\
	FThreadStats::AddMessage(GET_STATFNAME(Stat), EStatOperation::Set, double(Value));\
}


#define SET_CYCLE_COUNTER_FName(Stat,Cycles) \
{\
	FThreadStats::AddMessage(Stat, EStatOperation::Set, int64(Cycles), true);\
}

#define INC_DWORD_STAT_FName(Stat) \
{\
	FThreadStats::AddMessage(Stat, EStatOperation::Add, int64(1));\
}
#define INC_FLOAT_STAT_BY_FName(Stat, Amount) \
{\
	FThreadStats::AddMessage(Stat, EStatOperation::Add, double(Amount));\
}
#define INC_DWORD_STAT_BY_FName(Stat, Amount) \
{\
	FThreadStats::AddMessage(Stat, EStatOperation::Add, int64(Amount));\
}
#define INC_MEMORY_STAT_BY_FName(Stat, Amount) \
{\
	FThreadStats::AddMessage(Stat, EStatOperation::Add, int64(Amount));\
}
#define DEC_DWORD_STAT_FName(Stat) \
{\
	FThreadStats::AddMessage(Stat, EStatOperation::Subtract, int64(1));\
}
#define DEC_FLOAT_STAT_BY_FName(Stat,Amount) \
{\
	FThreadStats::AddMessage(Stat, EStatOperation::Subtract, double(Amount));\
}
#define DEC_DWORD_STAT_BY_FName(Stat,Amount) \
{\
	FThreadStats::AddMessage(Stat, EStatOperation::Subtract, int64(Amount));\
}
#define DEC_MEMORY_STAT_BY_FName(Stat,Amount) \
{\
	FThreadStats::AddMessage(Stat, EStatOperation::Subtract, int64(Amount));\
}
#define SET_MEMORY_STAT_FName(Stat,Value) \
{\
	FThreadStats::AddMessage(Stat, EStatOperation::Set, int64(Value));\
}
#define SET_DWORD_STAT_FName(Stat,Value) \
{\
	FThreadStats::AddMessage(Stat, EStatOperation::Set, int64(Value));\
}
#define SET_FLOAT_STAT_FName(Stat,Value) \
{\
	FThreadStats::AddMessage(Stat, EStatOperation::Set, double(Value));\
}


/**
 * Unique group identifiers. Note these don't have to defined in this header
 * but they do have to be unique. You're better off defining these in your
 * own headers/cpp files
 */
DECLARE_STATS_GROUP(TEXT("DefaultStatGroup"),STATGROUP_Default);
DECLARE_STATS_GROUP(TEXT("Anim"),STATGROUP_Anim);
DECLARE_STATS_GROUP(TEXT("AsyncIO"),STATGROUP_AsyncIO);
DECLARE_STATS_GROUP(TEXT("Audio"), STATGROUP_Audio);
DECLARE_STATS_GROUP(TEXT("BeamParticles"),STATGROUP_BeamParticles);
DECLARE_STATS_GROUP(TEXT("Canvas"),STATGROUP_Canvas);
DECLARE_STATS_GROUP(TEXT("Collision"),STATGROUP_Collision);
DECLARE_STATS_GROUP(TEXT("Engine"),STATGROUP_Engine);
DECLARE_STATS_GROUP(TEXT("FPSChart"),STATGROUP_FPSChart);
DECLARE_STATS_GROUP(TEXT("Game"),STATGROUP_Game);
DECLARE_STATS_GROUP(TEXT("UObjects"),STATGROUP_UObjects);
DECLARE_STATS_GROUP(TEXT("Memory"),STATGROUP_Memory);
DECLARE_STATS_GROUP(TEXT("MemoryStaticMesh"),STATGROUP_MemoryStaticMesh);
DECLARE_STATS_GROUP(TEXT("MemoryChurn"),STATGROUP_MemoryChurn);
DECLARE_STATS_GROUP(TEXT("MeshParticles"),STATGROUP_MeshParticles);
DECLARE_STATS_GROUP(TEXT("Net"),STATGROUP_Net);
DECLARE_STATS_GROUP(TEXT("Object"),STATGROUP_Object);
DECLARE_STATS_GROUP(TEXT("Particles"),STATGROUP_Particles);
DECLARE_STATS_GROUP(TEXT("GPUParticles"),STATGROUP_GPUParticles);
DECLARE_STATS_GROUP(TEXT("ParticleMem"),STATGROUP_ParticleMem);
DECLARE_STATS_GROUP(TEXT("Physics"),STATGROUP_Physics);
DECLARE_STATS_GROUP(TEXT("D3D11RHI"),STATGROUP_D3D11RHI);
DECLARE_STATS_GROUP(TEXT("Gnm"),STATGROUP_PS4RHI);
DECLARE_STATS_GROUP(TEXT("SceneRendering"),STATGROUP_SceneRendering);
DECLARE_STATS_GROUP(TEXT("InitViews"),STATGROUP_InitViews);
DECLARE_STATS_GROUP(TEXT("ShadowRendering"),STATGROUP_ShadowRendering);
DECLARE_STATS_GROUP(TEXT("LightRendering"),STATGROUP_LightRendering);
DECLARE_STATS_GROUP(TEXT("SceneUpdate"),STATGROUP_SceneUpdate);
DECLARE_STATS_GROUP(TEXT("ShaderCompiling"),STATGROUP_ShaderCompiling);
DECLARE_STATS_GROUP(TEXT("ShaderCompression"),STATGROUP_Shaders);
DECLARE_STATS_GROUP(TEXT("StatSystem"),STATGROUP_StatSystem);
DECLARE_STATS_GROUP(TEXT("Streaming"),STATGROUP_Streaming);
DECLARE_STATS_GROUP(TEXT("StreamingDetails"),STATGROUP_StreamingDetails);
DECLARE_STATS_GROUP(TEXT("Threading"),STATGROUP_Threading);
DECLARE_STATS_GROUP(TEXT("TrailParticles"),STATGROUP_TrailParticles);
DECLARE_STATS_GROUP(TEXT("UI"),STATGROUP_UI);
DECLARE_STATS_GROUP(TEXT("Navigation"),STATGROUP_Navigation);
DECLARE_STATS_GROUP(TEXT("Morph"),STATGROUP_MorphTarget);
DECLARE_STATS_GROUP(TEXT("RenderThread"),STATGROUP_RenderThreadProcessing);
DECLARE_STATS_GROUP(TEXT("OpenGLRHI"),STATGROUP_OpenGLRHI);
DECLARE_STATS_GROUP(TEXT("Slate"), STATGROUP_Slate );
DECLARE_STATS_GROUP(TEXT("Slate Memory"), STATGROUP_SlateMemory );
DECLARE_STATS_GROUP(TEXT("SceneMemory"),STATGROUP_SceneMemory);
DECLARE_STATS_GROUP(TEXT("Landscape"),STATGROUP_Landscape);
DECLARE_STATS_GROUP(TEXT("DDC"),STATGROUP_DDC);
DECLARE_STATS_GROUP(TEXT("CrashTracker"),STATGROUP_CrashTracker);
DECLARE_STATS_GROUP(TEXT("GraphTasks"),STATGROUP_GraphTasks);
DECLARE_STATS_GROUP(TEXT("Profiler"), STATGROUP_Profiler);
DECLARE_STATS_GROUP(TEXT("User"),STATGROUP_User);
DECLARE_STATS_GROUP(TEXT("Tickables"),STATGROUP_Tickables);
DECLARE_STATS_GROUP(TEXT("ServerCPU"),STATGROUP_ServerCPU);
DECLARE_STATS_GROUP(TEXT("Niagara"),STATGROUP_Niagara);
DECLARE_STATS_GROUP(TEXT("Text"),STATGROUP_Text);
DECLARE_STATS_GROUP(TEXT("CPUStalls"), STATGROUP_CPUStalls);
DECLARE_STATS_GROUP(TEXT("Quick"), STATGROUP_Quick);
DECLARE_STATS_GROUP(TEXT("Load Time"), STATGROUP_LoadTime);
DECLARE_STATS_GROUP(TEXT("AI"),STATGROUP_AI);
DECLARE_STATS_GROUP(TEXT("RHI"),STATGROUP_RHI);

DECLARE_CYCLE_STAT_EXTERN(TEXT("Root"),STAT_Root,STATGROUP_Default, CORE_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("FrameTime"),STAT_FrameTime,STATGROUP_Engine, CORE_API);
DECLARE_FLOAT_COUNTER_STAT_EXTERN(TEXT("StatUnit FPS"), STAT_FPS, STATGROUP_Engine, CORE_API);


/** Stats for the stat system */

DECLARE_CYCLE_STAT_EXTERN(TEXT("DrawStats"),STAT_DrawStats,STATGROUP_StatSystem, CORE_API);

#endif
