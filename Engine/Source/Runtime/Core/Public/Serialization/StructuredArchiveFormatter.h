// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "CoreFwd.h"

// Define a structure to encapsulate a field name, which compiles to an empty object if WITH_TEXT_ARCHIVE_SUPPORT = 0
#if WITH_TEXT_ARCHIVE_SUPPORT
	struct FArchiveFieldName
	{
		const TCHAR* Name;

		explicit FArchiveFieldName(const TCHAR* InName) : Name(InName){ }
	};

	#define FIELD_NAME(x) FArchiveFieldName(TEXT(x))
#else
	struct FArchiveFieldName
	{
	};

	#define FIELD_NAME(x) FArchiveFieldName()
#endif

/**
 * Specifies the type of a value in a slot. Used by FContextFreeArchiveFormatter for introspection.
 */
enum class EArchiveValueType
{
	None,
	Record,
	Array,
	Stream,
	Map,
	Int8,
	Int16,
	Int32,
	Int64,
	UInt8,
	UInt16,
	UInt32,
	UInt64,
	Float,
	Double,
	Bool,
	String,
	Name,
	Object,
	RawData,
};

/**
 * Interface to format data to and from an underlying archive. Methods on this class are validated to be correct
 * with the current archive state (eg. EnterObject/LeaveObject calls are checked to be matching), and do not need
 * to be validated by implementations.
 */
class CORE_API FStructuredArchiveFormatter
{
public:
	virtual ~FStructuredArchiveFormatter();
	
	virtual FArchive& GetUnderlyingArchive() = 0;

	virtual void EnterRecord() = 0;
	virtual void LeaveRecord() = 0;
	virtual void EnterField(FArchiveFieldName Name) = 0;
	virtual void LeaveField() = 0;
	virtual bool TryEnterField(FArchiveFieldName Name, bool bEnterWhenWriting) = 0;

	virtual void EnterArray(int32& NumElements) = 0;
	virtual void LeaveArray() = 0;
	virtual void EnterArrayElement() = 0;
	virtual void LeaveArrayElement() = 0;

	virtual void EnterStream() = 0;
	virtual void LeaveStream() = 0;
	virtual void EnterStreamElement() = 0;
	virtual void LeaveStreamElement() = 0;

	virtual void EnterMap(int32& NumElements) = 0;
	virtual void LeaveMap() = 0;
	virtual void EnterMapElement(FString& Name) = 0;
	virtual void LeaveMapElement() = 0;

	virtual void Serialize(uint8& Value) = 0;
	virtual void Serialize(uint16& Value) = 0;
	virtual void Serialize(uint32& Value) = 0;
	virtual void Serialize(uint64& Value) = 0;
	virtual void Serialize(int8& Value) = 0;
	virtual void Serialize(int16& Value) = 0;
	virtual void Serialize(int32& Value) = 0;
	virtual void Serialize(int64& Value) = 0;
	virtual void Serialize(float& Value) = 0;
	virtual void Serialize(double& Value) = 0;
	virtual void Serialize(bool& Value) = 0;
	virtual void Serialize(FString& Value) = 0;
	virtual void Serialize(FName& Value) = 0;
	virtual void Serialize(UObject*& Value) = 0;
	virtual void Serialize(TArray<uint8>& Value) = 0;
	virtual void Serialize(void* Data, uint64 DataSize) = 0;
};

/**
 * Base class for formatters that include context-free annotations for typing while reading.
 * Text-based formats implement this to allow context-free conversions between archives without being dependent on C++ serialization code.
 */
class CORE_API FAnnotatedStructuredArchiveFormatter : public FStructuredArchiveFormatter
{
public:
	using FStructuredArchiveFormatter::EnterRecord;
	virtual void EnterRecord(TArray<FString>& Keys) = 0;

	using FStructuredArchiveFormatter::EnterStream;
	virtual void EnterStream(int32& NumElements) = 0;

	using FStructuredArchiveFormatter::EnterField;
	virtual void EnterField(FArchiveFieldName Name, EArchiveValueType& OutType) = 0;

	using FStructuredArchiveFormatter::EnterArrayElement;
	virtual void EnterArrayElement(EArchiveValueType& OutType) = 0;

	using FStructuredArchiveFormatter::EnterStreamElement;
	virtual void EnterStreamElement(EArchiveValueType& OutType) = 0;

	using FStructuredArchiveFormatter::EnterMapElement;
	virtual void EnterMapElement(FString& OutName, EArchiveValueType& OutType) = 0;
};

#if WITH_TEXT_ARCHIVE_SUPPORT
	/**
	 * Copies formatted data from one place to another. Useful for conversion functions or visitor patterns.
	 */
	CORE_API void CopyFormattedData(FAnnotatedStructuredArchiveFormatter& InputFormatter, FStructuredArchiveFormatter& OutputFormatter);
#endif
