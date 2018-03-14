// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UObjectMarks.cpp: Unreal save marks annotation
=============================================================================*/

#include "UObject/UObjectMarks.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectIterator.h"

#include "UObject/UObjectAnnotation.h"


struct FObjectMark
{
	/**
	 * default contructor
	 * Default constructor must be the default item
	 */
	FObjectMark() :
		Marks(OBJECTMARK_NOMARKS)
	{
	}
	/**
	 * Intilialization constructor
	 * @param InMarks marks to initialize to
	 */
	FObjectMark(EObjectMark InMarks) :
		Marks(InMarks)
	{
	}
	/**
	 * Determine if this annotation is the default
	 * @return true is this is a default pair. We only check the linker because CheckInvariants rules out bogus combinations
	 */
	FORCEINLINE bool IsDefault()
	{
		return Marks == OBJECTMARK_NOMARKS;
	}

	/**
	 * Marks associated with an object
	 */
	EObjectMark				Marks; 

};

template <> struct TIsPODType<FObjectMark> { enum { Value = true }; };


/**
 * Annotation to relate objects with object marks
 */
class FThreadMarkAnnotation : public TThreadSingleton<FThreadMarkAnnotation>
{
	friend class TThreadSingleton<FThreadMarkAnnotation>;

	FThreadMarkAnnotation()	{}

public:

	FUObjectAnnotationSparse<FObjectMark, true> MarkAnnotation;
};


/**
 * Adds marks to an object
 *
 * @param	Object	Object to add marks to
 * @param	Marks	Logical OR of OBJECTMARK_'s to apply 
 */
void MarkObject(const class UObjectBase* Object, EObjectMark Marks)
{
	FUObjectAnnotationSparse<FObjectMark, true>& ThreadMarkAnnotation = FThreadMarkAnnotation::Get().MarkAnnotation;
	ThreadMarkAnnotation.AddAnnotation(Object,FObjectMark(EObjectMark(ThreadMarkAnnotation.GetAnnotation(Object).Marks | Marks)));
}

/**
 * Removes marks from and object
 *
 * @param	Object	Object to remove marks from
 * @param	Marks	Logical OR of OBJECTMARK_'s to remove 
 */
void UnMarkObject(const class UObjectBase* Object, EObjectMark Marks)
{
	FUObjectAnnotationSparse<FObjectMark, true>& ThreadMarkAnnotation = FThreadMarkAnnotation::Get().MarkAnnotation;
	FObjectMark Annotation = ThreadMarkAnnotation.GetAnnotation(Object);
	if(Annotation.Marks & Marks)
	{
		ThreadMarkAnnotation.AddAnnotation(Object,FObjectMark(EObjectMark(Annotation.Marks & ~Marks)));
	}
}

void MarkAllObjects(EObjectMark Marks)
{
	for( FObjectIterator It; It; ++It )
	{
		MarkObject(*It,Marks);
	}
}

void UnMarkAllObjects(EObjectMark Marks)
{
	FUObjectAnnotationSparse<FObjectMark, true>& ThreadMarkAnnotation = FThreadMarkAnnotation::Get().MarkAnnotation;
	if (Marks == OBJECTMARK_ALLMARKS)
	{
		ThreadMarkAnnotation.RemoveAllAnnotations();
	}
	else
	{
		const TMap<const UObjectBase *, FObjectMark>& Map = ThreadMarkAnnotation.GetAnnotationMap();
		for (TMap<const UObjectBase *, FObjectMark>::TConstIterator It(Map); It; ++It)
		{
			if(It.Value().Marks & Marks)
			{
				ThreadMarkAnnotation.AddAnnotation((UObject*)It.Key(),FObjectMark(EObjectMark(It.Value().Marks & ~Marks)));
			}
		}
	}
}

bool ObjectHasAnyMarks(const class UObjectBase* Object, EObjectMark Marks)
{
	return !!(FThreadMarkAnnotation::Get().MarkAnnotation.GetAnnotation(Object).Marks & Marks);
}

bool ObjectHasAllMarks(const class UObjectBase* Object, EObjectMark Marks)
{
	return (FThreadMarkAnnotation::Get().MarkAnnotation.GetAnnotation(Object).Marks & Marks) == Marks;
}

EObjectMark ObjectGetAllMarks(const class UObjectBase* Object)
{
	return FThreadMarkAnnotation::Get().MarkAnnotation.GetAnnotation(Object).Marks;
}

void GetObjectsWithAllMarks(TArray<UObject *>& Results, EObjectMark Marks)
{
	// We don't want to return any objects that are currently being background loaded unless we're using the object iterator during async loading.
	EInternalObjectFlags ExclusionFlags = EInternalObjectFlags::Unreachable;
	if (!IsInAsyncLoadingThread())
	{
		ExclusionFlags |= EInternalObjectFlags::AsyncLoading;
	}
	const TMap<const UObjectBase *, FObjectMark>& Map = FThreadMarkAnnotation::Get().MarkAnnotation.GetAnnotationMap();
	Results.Empty(Map.Num());
	for (TMap<const UObjectBase *, FObjectMark>::TConstIterator It(Map); It; ++It)
	{
		if ((It.Value().Marks & Marks) == Marks)
		{
			UObject* Item = (UObject*)It.Key();
			if (!Item->HasAnyInternalFlags(ExclusionFlags))
			{
				Results.Add(Item);
			}
		}
	}
}

void GetObjectsWithAnyMarks(TArray<UObject *>& Results, EObjectMark Marks)
{
	// We don't want to return any objects that are currently being background loaded unless we're using the object iterator during async loading.
	EInternalObjectFlags ExclusionFlags = EInternalObjectFlags::Unreachable;
	if (!IsInAsyncLoadingThread())
	{
		ExclusionFlags |= EInternalObjectFlags::AsyncLoading;
	}
	const TMap<const UObjectBase *, FObjectMark>& Map = FThreadMarkAnnotation::Get().MarkAnnotation.GetAnnotationMap();
	Results.Empty(Map.Num());
	for (TMap<const UObjectBase *, FObjectMark>::TConstIterator It(Map); It; ++It)
	{
		if (It.Value().Marks & Marks)
		{
			UObject* Item = (UObject*)It.Key();
			if (!Item->HasAnyInternalFlags(ExclusionFlags))
			{
				Results.Add(Item);
			}
		}
	}
}

