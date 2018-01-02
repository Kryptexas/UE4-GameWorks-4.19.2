// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ArchiveFromStructuredArchive.h"

FArchiveFromStructuredArchive::FArchiveFromStructuredArchive(FStructuredArchive::FSlot Slot)
	: FArchiveProxy(Slot.GetUnderlyingArchive())
	, bPendingSerialize(true)
	, Pos(0)
{
	if (IsTextFormat())
	{
		Record = Slot.EnterRecord();
		if (IsLoading())
		{
			SerializeInternal();
		}
	}
	else
	{
		Slot.EnterStream();
	}
}

FArchiveFromStructuredArchive::~FArchiveFromStructuredArchive()
{
	SerializeInternal();
}

void FArchiveFromStructuredArchive::Flush()
{
	SerializeInternal();
	FArchive::Flush();
}

bool FArchiveFromStructuredArchive::Close()
{
	SerializeInternal();
	return FArchive::Close();
}

int64 FArchiveFromStructuredArchive::Tell()
{
	if (IsTextFormat())
	{
		return Pos;
	}
	else
	{
		return InnerArchive.Tell();
	}
}

int64 FArchiveFromStructuredArchive::TotalSize()
{
	checkf(false, TEXT("FArchiveFromStructuredArchive does not support TotalSize()"));
	return FArchive::TotalSize();
}

void FArchiveFromStructuredArchive::Seek(int64 InPos)
{
	if (IsTextFormat())
	{
		check(Pos >= 0 && Pos <= Buffer.Num());
		Pos = InPos;
	}
	else
	{
		InnerArchive.Seek(InPos);
	}
}

bool FArchiveFromStructuredArchive::AtEnd()
{
	if (IsTextFormat())
	{
		return Pos == Buffer.Num();
	}
	else
	{
		return InnerArchive.AtEnd();
	}
}

FArchive& FArchiveFromStructuredArchive::operator<<(class FName& Value)
{
	if (IsTextFormat())
	{
		if (IsLoading())
		{
			int32 NameIdx = 0;
			Serialize(&NameIdx, sizeof(NameIdx));
			Value = Names[NameIdx];
		}
		else
		{
			int32* NameIdxPtr = NameToIndex.Find(Value);
			if (NameIdxPtr == nullptr)
			{
				NameIdxPtr = &(NameToIndex.Add(Value));
				*NameIdxPtr = Names.Add(Value);
			}
			Serialize(NameIdxPtr, sizeof(*NameIdxPtr));
		}
	}
	else
	{
		InnerArchive << Value;
	}
	return *this;
}

FArchive& FArchiveFromStructuredArchive::operator<<(class UObject*& Value)
{
	if(IsTextFormat())
	{
		if (IsLoading())
		{
			int32 ObjectIdx = 0;
			Serialize(&ObjectIdx, sizeof(ObjectIdx));
			Value = Objects[ObjectIdx];
		}
		else
		{
			int32* ObjectIdxPtr = ObjectToIndex.Find(Value);
			if (ObjectIdxPtr == nullptr)
			{
				ObjectIdxPtr = &(ObjectToIndex.Add(Value));
				*ObjectIdxPtr = Objects.Add(Value);
			}
			Serialize(ObjectIdxPtr, sizeof(*ObjectIdxPtr));
		}
	}
	else
	{
		InnerArchive << Value;
	}
	return *this;
}

void FArchiveFromStructuredArchive::Serialize(void* V, int64 Length)
{
	if (IsTextFormat())
	{
		if (IsLoading())
		{
			if (Pos + Length > Buffer.Num())
			{
				checkf(false, TEXT("Attempt to read past end of archive"));
			}
			FMemory::Memcpy(V, Buffer.GetData() + Pos, Length);
			Pos += Length;
		}
		else
		{
			if (Pos + Length > Buffer.Num())
			{
				Buffer.AddUninitialized(Pos + Length - Buffer.Num());
			}
			FMemory::Memcpy(Buffer.GetData() + Pos, V, Length);
			Pos += Length;
		}
	}
	else
	{
		InnerArchive.Serialize(V, Length);
	}
}

void FArchiveFromStructuredArchive::SerializeInternal()
{
	if (bPendingSerialize && IsTextFormat())
	{
		FStructuredArchive::FSlot DataSlot = Record.GetValue().EnterField(FIELD_NAME("Data"));
		DataSlot.Serialize(Buffer);

		TOptional<FStructuredArchive::FSlot> ObjectsSlot = Record.GetValue().TryEnterField(FIELD_NAME("Objects"), Objects.Num() > 0);
		if (ObjectsSlot.IsSet())
		{
			ObjectsSlot.GetValue() << Objects;
		}

		TOptional<FStructuredArchive::FSlot> NamesSlot = Record.GetValue().TryEnterField(FIELD_NAME("Names"), Names.Num() > 0);
		if (NamesSlot.IsSet())
		{
			NamesSlot.GetValue() << Names;
		}

		bPendingSerialize = false;
	}
}
