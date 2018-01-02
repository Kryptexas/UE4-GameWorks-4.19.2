// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "StructuredArchive.h"
#include "Containers/Set.h"
#include "Containers/UnrealString.h"

#if WITH_TEXT_ARCHIVE_SUPPORT

//////////// FStructuredArchive::FContainer ////////////

#if DO_GUARD_SLOW
struct FStructuredArchive::FContainer
{
	int Index;
	int Count;
	TSet<FString> KeyNames;

	FContainer(int InCount)
		: Index(0)
		, Count(InCount)
	{
	}
};
#endif

//////////// FStructuredArchive ////////////

FStructuredArchive::FStructuredArchive(FArchiveFormatterType& InFormatter)
	: Formatter(InFormatter)
	, NextElementId(RootElementId + 1)
	, CurrentSlotElementId(INDEX_NONE)
{
	CurrentScope.Reserve(32);
#if DO_GUARD_SLOW
	CurrentContainer.Reserve(32);
#endif
}

FStructuredArchive::~FStructuredArchive()
{
	Close();

#if DO_GUARD_SLOW
	while(CurrentContainer.Num() > 0)
	{
		delete CurrentContainer.Pop();
	}
#endif
}

FStructuredArchive::FSlot FStructuredArchive::Open()
{
	check(CurrentScope.Num() == 0);
	check(NextElementId == RootElementId + 1);
	check(CurrentSlotElementId == INDEX_NONE);

	CurrentScope.Add(FElement(RootElementId, EElementType::Root));

	CurrentSlotElementId = NextElementId++;

	return FSlot(*this, 0, CurrentSlotElementId);
}

void FStructuredArchive::Close()
{
	SetScope(0, RootElementId);
}

void FStructuredArchive::EnterSlot(int32 ElementId)
{
	checkf(ElementId == CurrentSlotElementId, TEXT("Attempt to serialize data into an invalid slot"));

	CurrentSlotElementId = INDEX_NONE;
}

void FStructuredArchive::EnterSlot(int32 ElementId, EElementType ElementType)
{
	checkf(ElementId == CurrentSlotElementId, TEXT("Attempt to serialize data into an invalid slot"));

	CurrentSlotElementId = INDEX_NONE;

	CurrentScope.Add(FElement(ElementId, ElementType));
}

void FStructuredArchive::LeaveSlot()
{
	switch (CurrentScope.Top().Type)
	{
	case EElementType::Record:
		Formatter.LeaveField();
		break;
	case EElementType::Array:
		Formatter.LeaveArrayElement();
#if DO_GUARD_SLOW
		CurrentContainer.Top()->Index++;
#endif
		break;
	case EElementType::Stream:
		Formatter.LeaveStreamElement();
		break;
	case EElementType::Map:
		Formatter.LeaveMapElement();
#if DO_GUARD_SLOW
		CurrentContainer.Top()->Index++;
#endif
		break;
	}
}

void FStructuredArchive::SetScope(int32 Depth, int32 ElementId)
{
	// Make sure the scope is valid
	checkf(Depth < CurrentScope.Num() && CurrentScope[Depth].Id == ElementId, TEXT("Invalid scope for writing to archive"));
	checkf(CurrentSlotElementId == INDEX_NONE, TEXT("Cannot change scope until having written a value to the current slot"));

	// Roll back to the correct scope
	for(int32 CurrentDepth = CurrentScope.Num() - 1; CurrentDepth > Depth; CurrentDepth--)
	{
		// Leave the current element
		const FElement& Element = CurrentScope[CurrentDepth];
		switch (Element.Type)
		{
		case EElementType::Record:
			Formatter.LeaveRecord();
#if DO_GUARD_SLOW
			delete CurrentContainer.Pop(false);
#endif
			break;
		case EElementType::Array:
#if DO_GUARD_SLOW
			checkf(CurrentContainer.Top()->Index == CurrentContainer.Top()->Count, TEXT("Incorrect number of elements serialized in array"));
#endif
			Formatter.LeaveArray();
#if DO_GUARD_SLOW
			delete CurrentContainer.Pop(false);
#endif
			break;
		case EElementType::Stream:
			Formatter.LeaveStream();
			break;
		case EElementType::Map:
#if DO_GUARD_SLOW
			checkf(CurrentContainer.Top()->Index == CurrentContainer.Top()->Count, TEXT("Incorrect number of elements serialized in map"));
#endif
			Formatter.LeaveMap();
#if DO_GUARD_SLOW
			delete CurrentContainer.Pop(false);
#endif
			break;
		}

		// Remove the element from the stack
		CurrentScope.RemoveAt(CurrentDepth, 1, false);

		// Leave the slot containing it
		LeaveSlot();
	}
}

//////////// FStructuredArchive::FSlot ////////////

FStructuredArchive::FRecord FStructuredArchive::FSlot::EnterRecord()
{
	Ar.EnterSlot(ElementId, EElementType::Record);

#if DO_GUARD_SLOW
	Ar.CurrentContainer.Add(new FContainer(0));
#endif

	Ar.Formatter.EnterRecord();

	return FRecord(Ar, Depth + 1, ElementId);
}

FStructuredArchive::FArray FStructuredArchive::FSlot::EnterArray(int32& Num)
{
	Ar.EnterSlot(ElementId, EElementType::Array);

	Ar.Formatter.EnterArray(Num);

#if DO_GUARD_SLOW
	Ar.CurrentContainer.Add(new FContainer(Num));
#endif

	return FArray(Ar, Depth + 1, ElementId);
}

FStructuredArchive::FStream FStructuredArchive::FSlot::EnterStream()
{
	Ar.EnterSlot(ElementId, EElementType::Stream);

	Ar.Formatter.EnterStream();

	return FStream(Ar, Depth + 1, ElementId);
}

FStructuredArchive::FMap FStructuredArchive::FSlot::EnterMap(int32& Num)
{
	Ar.EnterSlot(ElementId, EElementType::Map);

	Ar.Formatter.EnterMap(Num);

#if DO_GUARD_SLOW
	Ar.CurrentContainer.Add(new FContainer(Num));
#endif

	return FMap(Ar, Depth + 1, ElementId);
}

void FStructuredArchive::FSlot::operator<< (char& Value)
{
	Ar.EnterSlot(ElementId);

	int8 AsInt = Value;
	Ar.Formatter.Serialize(AsInt);
	Value = AsInt;

	Ar.LeaveSlot();
}

void FStructuredArchive::FSlot::operator<< (uint8& Value)
{
	Ar.EnterSlot(ElementId);
	Ar.Formatter.Serialize(Value);
	Ar.LeaveSlot();
}

void FStructuredArchive::FSlot::operator<< (uint16& Value)
{
	Ar.EnterSlot(ElementId);
	Ar.Formatter.Serialize(Value);
	Ar.LeaveSlot();
}

void FStructuredArchive::FSlot::operator<< (uint32& Value)
{
	Ar.EnterSlot(ElementId);
	Ar.Formatter.Serialize(Value);
	Ar.LeaveSlot();
}

void FStructuredArchive::FSlot::operator<< (uint64& Value)
{
	Ar.EnterSlot(ElementId);
	Ar.Formatter.Serialize(Value);
	Ar.LeaveSlot();
}

void FStructuredArchive::FSlot::operator<< (int8& Value)
{
	Ar.EnterSlot(ElementId);
	Ar.Formatter.Serialize(Value);
	Ar.LeaveSlot();
}

void FStructuredArchive::FSlot::operator<< (int16& Value)
{
	Ar.EnterSlot(ElementId);
	Ar.Formatter.Serialize(Value);
	Ar.LeaveSlot();
}

void FStructuredArchive::FSlot::operator<< (int32& Value)
{
	Ar.EnterSlot(ElementId);
	Ar.Formatter.Serialize(Value);
	Ar.LeaveSlot();
}

void FStructuredArchive::FSlot::operator<< (int64& Value)
{
	Ar.EnterSlot(ElementId);
	Ar.Formatter.Serialize(Value);
	Ar.LeaveSlot();
}

void FStructuredArchive::FSlot::operator<< (float& Value)
{
	Ar.EnterSlot(ElementId);
	Ar.Formatter.Serialize(Value);
	Ar.LeaveSlot();
}

void FStructuredArchive::FSlot::operator<< (double& Value)
{
	Ar.EnterSlot(ElementId);
	Ar.Formatter.Serialize(Value);
	Ar.LeaveSlot();
}

void FStructuredArchive::FSlot::operator<< (bool& Value)
{
	Ar.EnterSlot(ElementId);
	Ar.Formatter.Serialize(Value);
	Ar.LeaveSlot();
}

void FStructuredArchive::FSlot::operator<< (FString& Value)
{
	Ar.EnterSlot(ElementId);
	Ar.Formatter.Serialize(Value);
	Ar.LeaveSlot();
}

void FStructuredArchive::FSlot::operator<< (FName& Value)
{
	Ar.EnterSlot(ElementId);
	Ar.Formatter.Serialize(Value);
	Ar.LeaveSlot();
}

void FStructuredArchive::FSlot::operator<< (UObject*& Value)
{
	Ar.EnterSlot(ElementId);
	Ar.Formatter.Serialize(Value);
	Ar.LeaveSlot();
}

void FStructuredArchive::FSlot::Serialize(TArray<uint8>& Data)
{
	Ar.EnterSlot(ElementId);
	Ar.Formatter.Serialize(Data);
	Ar.LeaveSlot();
}

void FStructuredArchive::FSlot::Serialize(void* Data, uint64 DataSize)
{
	Ar.EnterSlot(ElementId);
	Ar.Formatter.Serialize(Data, DataSize);
	Ar.LeaveSlot();
}

//////////// FStructuredArchive::FObject ////////////

FStructuredArchive::FSlot FStructuredArchive::FRecord::EnterField(FArchiveFieldName Name)
{
	Ar.SetScope(Depth, ElementId);

	Ar.CurrentSlotElementId = Ar.NextElementId++;

#if DO_GUARD_SLOW
	FContainer& Container = *Ar.CurrentContainer.Top();
	checkf(!Container.KeyNames.Contains(Name.Name), TEXT("Multiple keys called '%s' serialized into record"), Name.Name);
	Container.KeyNames.Add(Name.Name);
#endif

	Ar.Formatter.EnterField(Name);

	return FSlot(Ar, Depth, Ar.CurrentSlotElementId);
}

FStructuredArchive::FRecord FStructuredArchive::FRecord::EnterRecord(FArchiveFieldName Name)
{
	return EnterField(Name).EnterRecord();
}

FStructuredArchive::FArray FStructuredArchive::FRecord::EnterArray(FArchiveFieldName Name, int32& Num)
{
	return EnterField(Name).EnterArray(Num);
}

FStructuredArchive::FStream FStructuredArchive::FRecord::EnterStream(FArchiveFieldName Name)
{
	return EnterField(Name).EnterStream();
}

FStructuredArchive::FMap FStructuredArchive::FRecord::EnterMap(FArchiveFieldName Name, int32& Num)
{
	return EnterField(Name).EnterMap(Num);
}

TOptional<FStructuredArchive::FSlot> FStructuredArchive::FRecord::TryEnterField(FArchiveFieldName Name, bool bEnterWhenWriting)
{
	Ar.SetScope(Depth, ElementId);

#if DO_GUARD_SLOW
	FContainer& Container = *Ar.CurrentContainer.Top();
	checkf(!Container.KeyNames.Contains(Name.Name), TEXT("Multiple keys called '%s' serialized into record"), Name.Name);
	Container.KeyNames.Add(Name.Name);
#endif

	if (Ar.Formatter.TryEnterField(Name, bEnterWhenWriting))
	{
		Ar.CurrentSlotElementId = Ar.NextElementId++;
		return TOptional<FStructuredArchive::FSlot>(FSlot(Ar, Depth, Ar.CurrentSlotElementId));
	}
	else
	{
		return TOptional<FStructuredArchive::FSlot>();
	}
}

//////////// FStructuredArchive::FArray ////////////

FStructuredArchive::FSlot FStructuredArchive::FArray::EnterElement()
{
	Ar.SetScope(Depth, ElementId);

#if DO_GUARD_SLOW
	checkf(Ar.CurrentContainer.Top()->Index < Ar.CurrentContainer.Top()->Count, TEXT("Serialized too many array elements"));
#endif

	Ar.CurrentSlotElementId = Ar.NextElementId++;

	Ar.Formatter.EnterArrayElement();

	return FSlot(Ar, Depth, Ar.CurrentSlotElementId);
}

//////////// FStructuredArchive::FStream ////////////

FStructuredArchive::FSlot FStructuredArchive::FStream::EnterElement()
{
	Ar.SetScope(Depth, ElementId);

	Ar.CurrentSlotElementId = Ar.NextElementId++;

	Ar.Formatter.EnterStreamElement();

	return FSlot(Ar, Depth, Ar.CurrentSlotElementId);
}

//////////// FStructuredArchive::FMap ////////////

FStructuredArchive::FSlot FStructuredArchive::FMap::EnterElement(FString& Name)
{
	Ar.SetScope(Depth, ElementId);

#if DO_GUARD_SLOW
	checkf(Ar.CurrentContainer.Top()->Index < Ar.CurrentContainer.Top()->Count, TEXT("Serialized too many map elements"));
#endif

	Ar.CurrentSlotElementId = Ar.NextElementId++;

#if DO_GUARD_SLOW
	if(Ar.GetUnderlyingArchive().IsSaving())
	{
		FContainer& Container = *Ar.CurrentContainer.Top();
		checkf(!Container.KeyNames.Contains(Name), TEXT("Multiple keys called '%s' serialized into record"), *Name);
		Container.KeyNames.Add(Name);
	}
#endif

	Ar.Formatter.EnterMapElement(Name);

#if DO_GUARD_SLOW
	if(Ar.GetUnderlyingArchive().IsLoading())
	{
		FContainer& Container = *Ar.CurrentContainer.Top();
		checkf(!Container.KeyNames.Contains(Name), TEXT("Multiple keys called '%s' serialized into record"), *Name);
		Container.KeyNames.Add(Name);
	}
#endif

	return FSlot(Ar, Depth, Ar.CurrentSlotElementId);
}

#endif
