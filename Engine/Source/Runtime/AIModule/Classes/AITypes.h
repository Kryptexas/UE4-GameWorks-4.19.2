// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AITypes.generated.h"

namespace FAISystem
{
	static const FVector InvalidLocation(FLT_MAX);
	static const FRotator InvalidRotation(FLT_MAX);

	FORCEINLINE bool IsValidLocation(const FVector& TestLocation)
	{
		return TestLocation != InvalidLocation;
	}

	FORCEINLINE bool IsValidRotation(const FRotator& TestRotation)
	{
		return TestRotation != InvalidRotation;
	}
}

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

namespace EAILogicResuming
{
	enum Type
	{
		Continue,
		RestartedInstead,
	};
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

USTRUCT(BlueprintType)
struct AIMODULE_API FAIRequestID
{
	GENERATED_USTRUCT_BODY()
		
private:
	static const uint32 AnyRequestID = 0;
	static const uint32 InvalidRequestID = uint32(-1);

	UPROPERTY()
	uint32 RequestID;

public:
	FAIRequestID(uint32 InRequestID = InvalidRequestID) : RequestID(InRequestID)
	{}

	/** returns true if given ID is identical to stored ID or any of considered
	 *	IDs is FAIRequestID::AnyRequest*/
	FORCEINLINE bool IsEquivalent(uint32 OtherID) const 
	{
		return OtherID != InvalidRequestID && this->IsValid() && (RequestID == OtherID || RequestID == AnyRequestID || OtherID == AnyRequestID);
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

