// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Serialization/StructuredArchiveFormatter.h"
#include "Archive.h"
#include "Containers/Array.h"

class CORE_API FBinaryArchiveFormatter final : public FStructuredArchiveFormatter
{
public:
	FBinaryArchiveFormatter(FArchive& InInner);
	virtual ~FBinaryArchiveFormatter();

	virtual FArchive& GetUnderlyingArchive() override;

	virtual void EnterRecord() override;
	virtual void LeaveRecord() override;
	virtual void EnterField(FArchiveFieldName Name) override;
	virtual void LeaveField() override;
	virtual bool TryEnterField(FArchiveFieldName Name, bool bEnterIfWriting);

	virtual void EnterArray(int32& NumElements) override;
	virtual void LeaveArray() override;
	virtual void EnterArrayElement() override;
	virtual void LeaveArrayElement() override;

	virtual void EnterStream() override;
	virtual void LeaveStream() override;
	virtual void EnterStreamElement() override;
	virtual void LeaveStreamElement() override;

	virtual void EnterMap(int32& NumElements) override;
	virtual void LeaveMap() override;
	virtual void EnterMapElement(FString& Name) override;
	virtual void LeaveMapElement() override;

	virtual void Serialize(uint8& Value) override;
	virtual void Serialize(uint16& Value) override;
	virtual void Serialize(uint32& Value) override;
	virtual void Serialize(uint64& Value) override;
	virtual void Serialize(int8& Value) override;
	virtual void Serialize(int16& Value) override;
	virtual void Serialize(int32& Value) override;
	virtual void Serialize(int64& Value) override;
	virtual void Serialize(float& Value) override;
	virtual void Serialize(double& Value) override;
	virtual void Serialize(bool& Value) override;
	virtual void Serialize(FString& Value) override;
	virtual void Serialize(FName& Value) override;
	virtual void Serialize(UObject*& Value) override;
	virtual void Serialize(TArray<uint8>& Value) override;
	virtual void Serialize(void* Data, uint64 DataSize) override;

private:

	FArchive& Inner;
};

inline FArchive& FBinaryArchiveFormatter::GetUnderlyingArchive()
{
	return Inner;
}

inline void FBinaryArchiveFormatter::EnterRecord()
{
}

inline void FBinaryArchiveFormatter::LeaveRecord()
{
}

inline void FBinaryArchiveFormatter::EnterField(FArchiveFieldName Name)
{
}

inline void FBinaryArchiveFormatter::LeaveField()
{
}

inline bool FBinaryArchiveFormatter::TryEnterField(FArchiveFieldName Name, bool bEnterWhenWriting)
{
	bool bValue = bEnterWhenWriting;
	Inner << bValue;
	if (bValue)
	{
		EnterField(Name);
	}
	return bValue;
}

inline void FBinaryArchiveFormatter::EnterArray(int32& NumElements)
{
	Inner << NumElements;
}

inline void FBinaryArchiveFormatter::LeaveArray()
{
}

inline void FBinaryArchiveFormatter::EnterArrayElement()
{
}

inline void FBinaryArchiveFormatter::LeaveArrayElement()
{
}

inline void FBinaryArchiveFormatter::EnterStream()
{
}

inline void FBinaryArchiveFormatter::LeaveStream()
{
}

inline void FBinaryArchiveFormatter::EnterStreamElement()
{
}

inline void FBinaryArchiveFormatter::LeaveStreamElement()
{
}

inline void FBinaryArchiveFormatter::EnterMap(int32& NumElements)
{
	Inner << NumElements;
}

inline void FBinaryArchiveFormatter::LeaveMap()
{
}

inline void FBinaryArchiveFormatter::EnterMapElement(FString& Name)
{
	Inner << Name;
}

inline void FBinaryArchiveFormatter::LeaveMapElement()
{
}

inline void FBinaryArchiveFormatter::Serialize(uint8& Value)
{
	Inner << Value;
}

inline void FBinaryArchiveFormatter::Serialize(uint16& Value)
{
	Inner << Value;
}

inline void FBinaryArchiveFormatter::Serialize(uint32& Value)
{
	Inner << Value;
}

inline void FBinaryArchiveFormatter::Serialize(uint64& Value)
{
	Inner << Value;
}

inline void FBinaryArchiveFormatter::Serialize(int8& Value)
{
	Inner << Value;
}

inline void FBinaryArchiveFormatter::Serialize(int16& Value)
{
	Inner << Value;
}

inline void FBinaryArchiveFormatter::Serialize(int32& Value)
{
	Inner << Value;
}

inline void FBinaryArchiveFormatter::Serialize(int64& Value)
{
	Inner << Value;
}

inline void FBinaryArchiveFormatter::Serialize(float& Value)
{
	Inner << Value;
}

inline void FBinaryArchiveFormatter::Serialize(double& Value)
{
	Inner << Value;
}

inline void FBinaryArchiveFormatter::Serialize(bool& Value)
{
	Inner << Value;
}

inline void FBinaryArchiveFormatter::Serialize(FString& Value)
{
	Inner << Value;
}

inline void FBinaryArchiveFormatter::Serialize(FName& Value)
{
	Inner << Value;
}

inline void FBinaryArchiveFormatter::Serialize(UObject*& Value)
{
	Inner << Value;
}

inline void FBinaryArchiveFormatter::Serialize(TArray<uint8>& Data)
{
	Inner << Data;
}

inline void FBinaryArchiveFormatter::Serialize(void* Data, uint64 DataSize)
{
	Inner.Serialize(Data, DataSize);
}
