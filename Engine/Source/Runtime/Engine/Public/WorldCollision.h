// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WorldCollision.h: WorldCollision related data structures/types 
=============================================================================*/

#pragma once 

#include "UniquePtr.h"

/**
 * Async Trace Data Structures 
 */

/** Trace Handle : unique ID that is returned once trace is requested **/
struct FTraceHandle
{
	union
	{
		uint64	_Handle;
		struct  
		{
			uint32 FrameNumber;
			uint32 Index;
		} _Data;
	};

	FTraceHandle() : _Handle(0) {};
	FTraceHandle(uint32 InFrameNumber, uint32 InIndex) 
	{
		_Data.FrameNumber = InFrameNumber;
		_Data.Index = InIndex;
	}
};

/** Trace Shape Types **/
namespace ECollisionShape
{
	enum Type
	{
		Line,
		Box,
		Sphere,
		Capsule
	};
};

struct FCollisionShape
{
	ECollisionShape::Type ShapeType;

	union
	{
		struct 
		{
			float HalfExtentX;
			float HalfExtentY;
			float HalfExtentZ;
		} Box;

		struct 
		{
			float Radius;
		} Sphere;

		struct 
		{
			float Radius;
			float HalfHeight;
		} Capsule;
	};

	FCollisionShape()
	{
		ShapeType = ECollisionShape::Line;
	}

	void SetBox(const FVector & HalfExtent)
	{
		ShapeType = ECollisionShape::Box;
		Box.HalfExtentX = HalfExtent.X;		
		Box.HalfExtentY = HalfExtent.Y;		
		Box.HalfExtentZ = HalfExtent.Z;		
	}

	void SetSphere(const float Radius)
	{
		ShapeType = ECollisionShape::Sphere;
		Sphere.Radius = Radius;
	}

	void SetCapsule(const float Radius, const float HalfHeight)
	{
		ShapeType = ECollisionShape::Capsule;
		Capsule.Radius = Radius;
		Capsule.HalfHeight = HalfHeight;
	}

	void SetCapsule(const FVector& Extent)
	{
		ShapeType = ECollisionShape::Capsule;
		Capsule.Radius = FMath::Min(Extent.X, Extent.Y);
		Capsule.HalfHeight = Extent.Z;
	}
	
	bool IsNearlyZero() const
	{
		switch (ShapeType)
		{
		case ECollisionShape::Box:
			{	
				return (Box.HalfExtentX <= KINDA_SMALL_NUMBER && Box.HalfExtentY <= KINDA_SMALL_NUMBER && Box.HalfExtentZ <= KINDA_SMALL_NUMBER);
			}
		case  ECollisionShape::Sphere:
			{
				return (Sphere.Radius <= KINDA_SMALL_NUMBER);
			}
		case ECollisionShape::Capsule:
			{
				// @Todo check height? It didn't check before, so I'm keeping this way for time being
				return (Capsule.Radius <= KINDA_SMALL_NUMBER);
			}
		}

		return true;
	}

	FVector GetExtent() const
	{
		switch(ShapeType)
		{
		case ECollisionShape::Box:
			{
				return FVector(Box.HalfExtentX, Box.HalfExtentY, Box.HalfExtentZ);
			}
		case  ECollisionShape::Sphere:
			{
				return FVector(Sphere.Radius, Sphere.Radius,  Sphere.Radius);
			}
		case ECollisionShape::Capsule:
			{
				// @Todo check height? It didn't check before, so I'm keeping this way for time being
				return FVector(Capsule.Radius, Capsule.Radius, Capsule.HalfHeight);
			}
		}

		return FVector::ZeroVector;
	}

	/** Get distance from center of capsule to center of sphere ends */
	float GetCapsuleAxisHalfLength() const
	{
		ensure (ShapeType == ECollisionShape::Capsule);
		return FMath::Max<float>(Capsule.HalfHeight - Capsule.Radius, 1.f);
	}

	FVector GetBox() const
	{
		return FVector(Box.HalfExtentX, Box.HalfExtentY, Box.HalfExtentZ);
	}

	const float GetSphereRadius() const
	{
		return Sphere.Radius;
	}

	const float GetCapsuleRadius() const
	{
		return Capsule.Radius;
	}

	const float GetCapsuleHalfHeight() const
	{
		return Capsule.HalfHeight;
	}

	static struct FCollisionShape LineShape;

	static FCollisionShape MakeBox(const FVector & BoxHalfExtent)
	{
		FCollisionShape BoxShape;
		BoxShape.SetBox(BoxHalfExtent);
		return BoxShape;
	}

	static FCollisionShape MakeSphere(const float SphereRadius)
	{
		FCollisionShape SphereShape;
		SphereShape.SetSphere(SphereRadius);
		return SphereShape;
	}

	static FCollisionShape MakeCapsule(const float CapsuleRadius, const float CapsuleHalfHeight)
	{
		FCollisionShape CapsuleShape;
		CapsuleShape.SetCapsule(CapsuleRadius, CapsuleHalfHeight);
		return CapsuleShape;
	}

	static FCollisionShape MakeCapsule(const FVector & Extent)
	{
		FCollisionShape CapsuleShape;
		CapsuleShape.SetCapsule(Extent);
		return CapsuleShape;
	}
};

/** Trace Shape Shapes - Params packs all possible shape types **/
struct FCollisionParameters
{
	/** Collision trance params **/
	struct FCollisionQueryParams		CollisionQueryParam;
	struct FCollisionResponseParams		ResponseParam;
	struct FCollisionObjectQueryParams	ObjectQueryParam;

	/** Shape data **/
	struct FCollisionShape CollisionShape;
};

/** Base Trace Data Struct **/
struct FBaseTraceDatum
{
	/* Physics World this trace will run in **/
	TWeakObjectPtr<UWorld> PhysWorld;

	/* Shape Data */
	FCollisionParameters	CollisionParams;

	/* Collision trace info*/
	ECollisionChannel TraceChannel;

	/* Framecount when requested is made*/
	uint32	FrameNumber; 

	/* User data*/
	uint32	UserData;

	/* single or multi - changed to uint32 if we get more booleans*/
	bool 	bIsMultiTrace;

	FBaseTraceDatum() {};

	/* Set functions for each Shape type */
	void Set(UWorld * World, const FCollisionShape & InCollisionShape, const FCollisionQueryParams & Param, const struct FCollisionResponseParams &InResponseParam, const struct FCollisionObjectQueryParams& InObjectQueryParam, 
		ECollisionChannel Channel, uint32 InUserData, bool bInIsMultiTrace, int32 FrameCounter)
	{
		ensure (World);
		CollisionParams.CollisionShape = InCollisionShape;
		CollisionParams.CollisionQueryParam = Param;
		CollisionParams.ResponseParam = InResponseParam;
		CollisionParams.ObjectQueryParam = InObjectQueryParam;
		TraceChannel = Channel;
		UserData = InUserData;
		bIsMultiTrace = bInIsMultiTrace;
		FrameNumber = FrameCounter;
		PhysWorld = World;
	}
};

struct FTraceDatum;
struct FOverlapDatum;

/*
 * Delegate for Trace 
 * @param	FTraceHandle	TraceHandle that is returned when requested
 * @param	FTraceDatum		TraceDatum that includes input/output
 */
DECLARE_DELEGATE_TwoParams( FTraceDelegate, const FTraceHandle&, FTraceDatum &);
/*
 * Delegate for Overlap
 * @param	FTHandle		TraceHandle that is returned when requestsed
 * @param	FOverlapDatum	OverlapDatum that includes input/output
 */
DECLARE_DELEGATE_TwoParams( FOverlapDelegate, const FTraceHandle&, FOverlapDatum &);


/** Trace Data Struct **/
struct FTraceDatum : public FBaseTraceDatum
{
	/** Start of the trace **/
	FVector Start;
	/** End of the trace **/
	FVector End;
	/** Delegate - optional - you can query **/
	FTraceDelegate Delegate;

	/** Output of the hits if made **/
	TArray<struct FHitResult> OutHits;

	FTraceDatum() {}

	/** Set Trace Datum for each shape type **/
	FTraceDatum(UWorld * World, const FCollisionShape & CollisionShape, const FCollisionQueryParams & Param, const struct FCollisionResponseParams &InResponseParam, const struct FCollisionObjectQueryParams& InObjectQueryParam, 
		ECollisionChannel Channel, uint32 InUserData, bool bInIsMultiTrace,	const FVector & InStart, const FVector & InEnd, FTraceDelegate * InDelegate, int32 FrameCounter)
	{
		Set(World, CollisionShape, Param, InResponseParam, InObjectQueryParam, Channel, InUserData, bInIsMultiTrace, FrameCounter);
		Start = InStart;
		End = InEnd;
		if (InDelegate)
		{
			Delegate = *InDelegate;
		}
		else
		{
			Delegate.Unbind();
		}
		OutHits.Reset();
	}
};

// overlap data struct
struct FOverlapDatum : FBaseTraceDatum
{
	//input
	FVector Pos;
	FQuat	Rot;
	FOverlapDelegate Delegate;

	//output 
	TArray<struct FOverlapResult> OutOverlaps;

	FOverlapDatum() {}

	FOverlapDatum(UWorld * World, const FCollisionShape & CollisionShape, const FCollisionQueryParams & Param, const struct FCollisionResponseParams &InResponseParam, const struct FCollisionObjectQueryParams& InObjectQueryParam, 
		ECollisionChannel Channel, uint32 InUserData, bool bInIsMultiTrace,
		const FVector& InPos, const FQuat& InRot, FOverlapDelegate * InDelegate, int32 FrameCounter)
	{
		Set(World, CollisionShape, Param, InResponseParam, InObjectQueryParam, Channel, InUserData, bInIsMultiTrace, FrameCounter);
		Pos = InPos;
		Rot = InRot;
		if (InDelegate)
		{
			Delegate = *InDelegate;
		}
		else
		{
			Delegate.Unbind();
		}
		OutOverlaps.Reset();
	}
};

#define ASYNC_TRACE_BUFFER_SIZE 64

template <typename T>
struct TTraceThreadData
{
	T Buffer[ASYNC_TRACE_BUFFER_SIZE];
};

/** We use double buffer for trace data pool. FrameNumber % 2 = is going to be the one collecting NEW data **/
struct AsyncTraceData : FNoncopyable
{
	// Data Buffer for each trace

	// FTraceThreadData is one atomic data size for thread
	// so once that's filled up, this will be sent to thread
	// if you need more, it will allocate new FTraceThreadData and add to TArray

	// however when we calculate index of buffer, we calculate continuously, 
	// meaning if TraceData(1).Buffer[50] will have 1*ASYNC_TRACE_BUFFER_SIZE + 50 as index
	// in order to give UNIQUE INDEX to every data in this buffer
	TArray<TUniquePtr<TTraceThreadData<FTraceDatum>>>			TraceData;
	TArray<TUniquePtr<TTraceThreadData<FOverlapDatum>>>			OverlapData;

	// if Execution is all done, set this to be true
	// when reinitialize bAsyncAllowed is true, once execution is done, this will set to be false
	// this is to find cases where execution is already done but another request is made again within the same frame. 
	bool					bAsyncAllowed; // are async calls allowed this frame

	// Thread completion event for batch
	FGraphEventArray		AsyncTraceCompletionEvent;

	AsyncTraceData() : bAsyncAllowed(false) {}
};


extern ECollisionChannel DefaultCollisionChannel;
ENGINE_API DECLARE_LOG_CATEGORY_EXTERN(LogCollision, Warning, All);
