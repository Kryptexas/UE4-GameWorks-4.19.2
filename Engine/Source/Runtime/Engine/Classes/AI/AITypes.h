// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AITypes.generated.h"

UENUM()
namespace EAIOptionFlag
{
	enum Type
	{
		Default,
		Enable UMETA(DisplayName = "Yes"),	// UHT was complaining when tried to use True as value instead of Enable
		Disable UMETA(DisplayName = "No"),

		MAX UMETA(Hidden)
	};
}

namespace EAIForceParam
{
	enum Type
	{
		Force,
		DoNotForce,

		MAX UMETA(Hidden)
	};
}

namespace FAIMoveFlag
{
	static const bool StopOnOverlap = true;
	static const bool UsePathfinding = true;
	static const bool IgnorePathfinding = false;
}

USTRUCT()
struct FGenericTeamId
{
	GENERATED_USTRUCT_BODY()

	enum EPredefinedId
	{
		NoTeam = 255
	};

	UPROPERTY(Category="TeamID", EditAnywhere, BlueprintReadWrite)
	uint8 TeamID;

	FGenericTeamId(uint8 InTeamID = NoTeam)  : TeamID(InTeamID)
	{}

	FORCEINLINE operator uint8() const { return TeamID; }
};

UENUM()
namespace EPawnActionAbortState
{
	enum Type
	{
		NotBeingAborted,
		MarkPendingAbort,	// this means waiting for child to abort before aborting self
		LatendAbortInProgress,
		AbortDone,

		MAX UMETA(Hidden)
	};
}

UENUM()
namespace EPawnActionResult
{
	enum Type
	{
		InProgress,
		Success,
		Failed,
		Aborted
	};
}

UENUM()
namespace EPawnActionEventType
{
	enum Type
	{
		Invalid,
		FinishedAborting,
		FinishedExecution,
		Push,
	};
}

UENUM()
namespace EAILockSource
{
	enum Type
	{
		Animation,
		Logic,
		Script,
		Gameplay,

		MAX UMETA(Hidden)
	};
}

/** structure used to define which subsystem requested locking of a specific AI resource (like movement, logic, etc.) */
struct FAIResourceLock
{
	uint8 Locks[EAILockSource::MAX];

	FAIResourceLock();

	FORCEINLINE void SetLock(EAILockSource::Type LockSource)
	{
		check(LockSource != EAILockSource::MAX);
		Locks[LockSource] = 1;
	}

	FORCEINLINE void ClearLock(EAILockSource::Type LockSource)
	{
		check(LockSource != EAILockSource::MAX);
		Locks[LockSource] = 0;
	}

	/** force-clears all locks */
	void ForceClearAllLocks();

	FORCEINLINE bool IsLocked() const
	{
		for (int32 LockLevel = 0; LockLevel < int32(EAILockSource::MAX); ++LockLevel)
		{
			if (Locks[LockLevel])
			{
				return true;
			}
		}
		return false;
	}

	FORCEINLINE bool IsLocked(EAILockSource::Type LockSource) const
	{
		check(LockSource != EAILockSource::MAX);
		return Locks[LockSource] > 0;
	}

	FString GetLockSourceName() const;
};

USTRUCT()
struct FNavAvoidanceMask
{
	GENERATED_USTRUCT_BODY()

#if CPP
	union
	{
		struct
		{
#endif
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup0 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup1 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup2 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup3 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup4 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup5 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup6 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup7 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup8 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup9 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup10 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup11 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup12 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup13 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup14 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup15 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup16 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup17 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup18 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup19 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup20 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup21 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup22 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup23 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup24 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup25 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup26 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup27 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup28 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup29 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup30 : 1;
		UPROPERTY(Category="Group", EditAnywhere, BlueprintReadOnly)
		uint32 bGroup31 : 1;
#if CPP
		};
		int32 Packed;
	};
#endif

	FORCEINLINE bool HasGroup(uint8 GroupID) const
	{
		return (Packed & (1 << GroupID)) != 0;
	}

	FORCEINLINE void SetGroup(uint8 GroupID)
	{
		Packed |= (1 << GroupID);
	}

	FORCEINLINE void ClearGroup(uint8 GroupID)
	{
		Packed &= ~(1 << GroupID);
	}

	FORCEINLINE void ClearAll()
	{
		Packed = 0;
	}

	FORCEINLINE void SetFlagsDirectly(uint32 NewFlagset)
	{
		Packed = NewFlagset;
	}
};

USTRUCT(BlueprintType)
struct ENGINE_API FAIRequestID
{
	GENERATED_USTRUCT_BODY()
		
private:
	static const uint32 AnyRequestID = 0;
	static const uint32 InvalidRequestID = uint32(-1);

	uint32 RequestID;

public:
	FAIRequestID(uint32 InRequestID = InvalidRequestID) : RequestID(InRequestID)
	{}

	/** returns true if given ID is identical to stored ID or any of considered
	 *	IDs is FAIRequestID::AnyRequest*/
	FORCEINLINE bool IsEquivalent(uint32 OtherID) const 
	{
		return RequestID == OtherID || (RequestID == AnyRequestID && OtherID != InvalidRequestID) || (OtherID == AnyRequestID && this->IsValid());
	}

	FORCEINLINE bool IsEquivalent(FAIRequestID Other) const
	{
		return IsEquivalent(Other.RequestID);
	}

	FORCEINLINE bool IsValid() const
	{
		return RequestID != InvalidRequestID;
	}

	FORCEINLINE uint32 GetID() const { return RequestID; }

	void operator=(uint32 OtherID)
	{
		RequestID = OtherID;
	}

	operator uint32() const
	{
		return RequestID;
	}

	static const FAIRequestID AnyRequest;
	static const FAIRequestID CurrentRequest;
	static const FAIRequestID InvalidRequest;
};

