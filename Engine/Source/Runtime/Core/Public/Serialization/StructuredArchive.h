// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "StructuredArchiveFormatter.h"
#include "Formatters/BinaryArchiveFormatter.h"
#include "Misc/Optional.h"
#include "Containers/Array.h"

/**
 * Class to contain a named value for serialization. Intended to be created as a temporary and passed to object serialization methods.
 */
template<typename T> struct TNamedValue
{
	FArchiveFieldName Name;
	T& Value;

	FORCEINLINE TNamedValue(FArchiveFieldName InName, T& InValue)
		: Name(InName)
		, Value(InValue)
	{
	}
};

/**
 * Helper function to construct a TNamedValue, deducing the value type.
 */
template<typename T> FORCEINLINE TNamedValue<T> MakeNamedValue(FArchiveFieldName Name, T& Value)
{
	return TNamedValue<T>(Name, Value);
}

/** Construct a TNamedValue given an ANSI string and value reference. */
#if WITH_TEXT_ARCHIVE_SUPPORT
	#define NAMED_ITEM(Name, Value) MakeNamedValue(FArchiveFieldName(TEXT(Name)), Value)
#else
	#define NAMED_ITEM(Name, Value) MakeNamedValue(FArchiveFieldName(), Value)
#endif

/** Construct a TNamedValue using the name of a field or variable. */
#define NAMED_FIELD(Field) NAMED_ITEM(#Field, Field)

/** Typedef for which formatter type to support */
#if WITH_TEXT_ARCHIVE_SUPPORT
	typedef FStructuredArchiveFormatter FArchiveFormatterType;
#else
	typedef FBinaryArchiveFormatter FArchiveFormatterType;
#endif

/**
 * Manages the state of an underlying FStructuredArchiveFormatter, and provides a consistent API for reading and writing to a structured archive.
 * 
 * Both reading and writing to the archive are *forward only* from an interface point of view. There is no point at which it is possible to 
 * require seeking.
 */
class CORE_API FStructuredArchive
{
public:
	class FSlot;
	class FRecord;
	class FArray;
	class FStream;
	class FMap;

	/**
	 * Constructor.
	 *
	 * @param InFormatter Formatter for the archive data
	 */
	FStructuredArchive(FArchiveFormatterType& InFormatter);

	/**
	 * Default destructor. Closes the archive.
	 */
	~FStructuredArchive();

	/**
	 * Start writing to the archive, and gets an interface to the root slot.
	 */
	FSlot Open();

	/**
	 * Flushes any remaining scope to the underlying formatter and closes the archive.
	 */
	void Close();

	/**
	 * Gets the serialization context from the underlying archive.
	 */
	FORCEINLINE FArchive& GetUnderlyingArchive()
	{
		return Formatter.GetUnderlyingArchive();
	}

	FStructuredArchive(const FStructuredArchive&) = delete;
	FStructuredArchive& operator=(const FStructuredArchive&) = delete;

public:
	/**
	 * Contains a value in the archive; either a field or array/map element. A slot does not know it's name or location,
	 * and can merely have a value serialized into it. That value may be a literal (eg. int, float) or compound object
	 * (eg. object, array, map).
	 */
	class CORE_API FSlot
	{
	public:
		FRecord EnterRecord();
		FArray EnterArray(int32& Num);
		FStream EnterStream();
		FMap EnterMap(int32& Num);

		// We don't support chaining writes to a single slot, so these return void.
		void operator << (char& Value);
		void operator << (uint8& Value);
		void operator << (uint16& Value);
		void operator << (uint32& Value);
		void operator << (uint64& Value);
		void operator << (int8& Value);
		void operator << (int16& Value);
		void operator << (int32& Value);
		void operator << (int64& Value);
		void operator << (float& Value);
		void operator << (double& Value);
		void operator << (bool& Value);
		void operator << (FString& Value);
		void operator << (FName& Value);
		void operator << (UObject*& Value);

		void Serialize(TArray<uint8>& Data);
		void Serialize(void* Data, uint64 DataSize);

		FORCEINLINE FArchive& GetUnderlyingArchive()
		{
			return Ar.GetUnderlyingArchive();
		}

	private:
		friend FStructuredArchive;

		FStructuredArchive& Ar;
#if WITH_TEXT_ARCHIVE_SUPPORT
		const int32 Depth;
		const int32 ElementId;

		FORCEINLINE FSlot(FStructuredArchive& InAr, int32 InDepth, int32 InElementId)
			: Ar(InAr)
			, Depth(InDepth)
			, ElementId(InElementId)
		{
		}
#else
		FORCEINLINE FSlot(FStructuredArchive& InAr)
			: Ar(InAr)
		{
		}
#endif
	};

	/**
	 * Represents a record in the structured archive. An object contains slots that are identified by FArchiveName,
	 * which may be compiled out with binary-only archives.
	 */
	class CORE_API FRecord
	{
	public:
		FSlot EnterField(FArchiveFieldName Name);
		FRecord EnterRecord(FArchiveFieldName Name);
		FArray EnterArray(FArchiveFieldName Name, int32& Num);
		FStream EnterStream(FArchiveFieldName Name);
		FMap EnterMap(FArchiveFieldName Name, int32& Num);

		TOptional<FSlot> TryEnterField(FArchiveFieldName Name, bool bEnterForSaving);

		template<typename T> FORCEINLINE FRecord& operator<<(TNamedValue<T> Item)
		{
			EnterField(Item.Name) << Item.Value;
			return *this;
		}

		FORCEINLINE FArchive& GetUnderlyingArchive()
		{
			return Ar.GetUnderlyingArchive();
		}

	private:
		friend FStructuredArchive;

		FStructuredArchive& Ar;
#if WITH_TEXT_ARCHIVE_SUPPORT
		const int32 Depth;
		const int32 ElementId;

		FORCEINLINE FRecord(FStructuredArchive& InAr, int32 InDepth, int32 InElementId)
			: Ar(InAr)
			, Depth(InDepth)
			, ElementId(InElementId)
		{
		}
#else
		FORCEINLINE FRecord(FStructuredArchive& InAr)
			: Ar(InAr)
		{
		}
#endif
	};

	/**
	 * Represents an array in the structured archive. An object contains slots that are identified by a FArchiveFieldName,
	 * which may be compiled out with binary-only archives.
	 */
	class CORE_API FArray
	{
	public:
		FSlot EnterElement();

		template<typename T> FORCEINLINE FArray& operator<<(T& Item)
		{
			EnterElement() << Item;
			return *this;
		}

		FORCEINLINE FArchive& GetUnderlyingArchive()
		{
			return Ar.GetUnderlyingArchive();
		}

	private:
		friend FStructuredArchive;

		FStructuredArchive& Ar;
#if WITH_TEXT_ARCHIVE_SUPPORT
		const int32 Depth;
		const int32 ElementId;

		FORCEINLINE FArray(FStructuredArchive& InAr, int32 InDepth, int32 InElementId)
			: Ar(InAr)
			, Depth(InDepth)
			, ElementId(InElementId)
		{
		}
#else
		FORCEINLINE FArray(FStructuredArchive& InAr)
			: Ar(InAr)
		{
		}
#endif
	};

	/**
	 * Represents an unsized sequence of slots in the structured archive (similar to an array, but without a known size).
	 */
	class CORE_API FStream
	{
	public:
		FSlot EnterElement();

		template<typename T> FORCEINLINE FStream& operator<<(T& Item)
		{
			EnterElement() << Item;
			return *this;
		}

		FORCEINLINE FArchive& GetUnderlyingArchive()
		{
			return Ar.GetUnderlyingArchive();
		}

	private:
		friend FStructuredArchive;

		FStructuredArchive& Ar;
#if WITH_TEXT_ARCHIVE_SUPPORT
		const int32 Depth;
		const int32 ElementId;

		FORCEINLINE FStream(FStructuredArchive& InAr, int32 InDepth, int32 InElementId)
			: Ar(InAr)
			, Depth(InDepth)
			, ElementId(InElementId)
		{
		}
#else
		FORCEINLINE FStream(FStructuredArchive& InAr)
			: Ar(InAr)
		{
		}
#endif
	};

	/**
	 * Represents a map in the structured archive. A map is similar to an object, but keys can be read back out from an archive.
	 * (This is an important distinction for binary archives).
	 */
	class CORE_API FMap
	{
	public:
		FSlot EnterElement(FString& Name);

		FORCEINLINE FArchive& GetUnderlyingArchive()
		{
			return Ar.GetUnderlyingArchive();
		}

	private:
		friend FStructuredArchive;

		FStructuredArchive& Ar;
#if WITH_TEXT_ARCHIVE_SUPPORT
		const int32 Depth;
		const int32 ElementId;

		FORCEINLINE FMap(FStructuredArchive& InAr, int32 InDepth, int32 InElementId)
			: Ar(InAr)
			, Depth(InDepth)
			, ElementId(InElementId)
		{
		}
#else
		FORCEINLINE FMap(FStructuredArchive& InAr)
			: Ar(InAr)
		{
		}
#endif
	};

private:
	/**
	* Reference to the formatter that actually writes out the data to the underlying archive
	*/
	FArchiveFormatterType& Formatter;

#if WITH_TEXT_ARCHIVE_SUPPORT
	enum class EElementType : unsigned char
	{
		Root,
		Record,
		Array,
		Stream,
		Map,
	};

	struct FElement
	{
		int Id;
		EElementType Type;

		FElement(int InId, EElementType InType)
			: Id(InId)
			, Type(InType)
		{
		}
	};

#if DO_GUARD_SLOW
	struct FContainer;
#endif

	/**
	 * First element ID, which is assigned to the root element.
	 */
	static const int RootElementId = 1000;

	/**
	 * The next element id to be assigned
	 */
	int NextElementId;

	/**
	 * The element ID assigned for the current slot. Slots are transient, and only exist as placeholders until something is written into them. This is reset to INDEX_NONE when something is created in a slot, and the created item can assume the element id.
	 */
	int CurrentSlotElementId;

	/**
	 * Tracks the current stack of objects being written. Used by SetScope() to ensure that scopes are always closed correctly in the underlying formatter,
	 * and to make sure that the archive is always written in a forwards-only way (ie. writing to an element id that is not in scope will assert)
	 */
	TArray<FElement> CurrentScope;

#if DO_GUARD_SLOW
	/**
	 * For arrays and maps, stores the loop counter and size of the container. Also stores key names for records and maps in builds with DO_GUARD_SLOW enabled.
	 */
	TArray<FContainer*> CurrentContainer;
#endif

	/**
	 * Enters the current slot for serializing a value. Asserts if the archive is not in a state about to write to an empty-slot.
	 */
	void EnterSlot(int32 ElementId);

	/**
	 * Enters the current slot, adding an element onto the stack. Asserts if the archive is not in a state about to write to an empty-slot.
	 */
	void EnterSlot(int32 ElementId, EElementType ElementType);

	/**
	 * Leaves slot at the top of the current scope
	 */
	void LeaveSlot();

	/**
	 * Switches to the scope at the given element id, updating the formatter state as necessary
	 */
	void SetScope(int32 Depth, int32 ElementId);
#endif
};

template <typename T>
FORCEINLINE_DEBUGGABLE void operator<<(FStructuredArchive::FSlot Slot, TArray<T>& InArray)
{
	int32 NumElements = InArray.Num();
	FStructuredArchive::FArray Array = Slot.EnterArray(NumElements);

	if (Slot.GetUnderlyingArchive().IsLoading())
	{
		InArray.SetNum(NumElements);
	}

	for (int32 ElementIndex = 0; ElementIndex < NumElements; ++ElementIndex)
	{
		FStructuredArchive::FSlot ElementSlot = Array.EnterElement();
		ElementSlot << InArray[ElementIndex];
	}
}

template <>
FORCEINLINE_DEBUGGABLE void operator<<(FStructuredArchive::FSlot Slot, TArray<uint8>& InArray)
{
	Slot.Serialize(InArray);
}

#if !WITH_TEXT_ARCHIVE_SUPPORT

	//////////// FStructuredArchive ////////////

	FORCEINLINE FStructuredArchive::FStructuredArchive(FArchiveFormatterType& InFormatter)
		: Formatter(InFormatter)
	{
	}

	FORCEINLINE FStructuredArchive::~FStructuredArchive()
	{
	}

	FORCEINLINE FStructuredArchive::FSlot FStructuredArchive::Open()
	{
		return FSlot(*this);
	}

	FORCEINLINE void FStructuredArchive::Close()
	{
	}

	//////////// FStructuredArchive::FSlot ////////////

	FORCEINLINE FStructuredArchive::FRecord FStructuredArchive::FSlot::EnterRecord()
	{
		return FRecord(Ar);
	}

	FORCEINLINE FStructuredArchive::FArray FStructuredArchive::FSlot::EnterArray(int32& Num)
	{
		Ar.Formatter.EnterArray(Num);
		return FArray(Ar);
	}

	FORCEINLINE FStructuredArchive::FStream FStructuredArchive::FSlot::EnterStream()
	{
		return FStream(Ar);
	}

	FORCEINLINE FStructuredArchive::FMap FStructuredArchive::FSlot::EnterMap(int32& Num)
	{
		Ar.Formatter.EnterMap(Num);
		return FMap(Ar);
	}

	FORCEINLINE void FStructuredArchive::FSlot::operator<< (char& Value)
	{
		int8 AsInt = Value;
		Ar.Formatter.Serialize(AsInt);
		Value = AsInt;
	}

	FORCEINLINE void FStructuredArchive::FSlot::operator<< (uint8& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchive::FSlot::operator<< (uint16& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchive::FSlot::operator<< (uint32& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchive::FSlot::operator<< (uint64& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchive::FSlot::operator<< (int8& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchive::FSlot::operator<< (int16& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchive::FSlot::operator<< (int32& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchive::FSlot::operator<< (int64& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchive::FSlot::operator<< (float& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchive::FSlot::operator<< (double& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchive::FSlot::operator<< (bool& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchive::FSlot::operator<< (FString& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchive::FSlot::operator<< (FName& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchive::FSlot::operator<< (UObject*& Value)
	{
		Ar.Formatter.Serialize(Value);
	}

	FORCEINLINE void FStructuredArchive::FSlot::Serialize(TArray<uint8>& Data)
	{
		Ar.Formatter.Serialize(Data);
	}

	FORCEINLINE void FStructuredArchive::FSlot::Serialize(void* Data, uint64 DataSize)
	{
		Ar.Formatter.Serialize(Data, DataSize);
	}

	//////////// FStructuredArchive::FObject ////////////

	FORCEINLINE FStructuredArchive::FSlot FStructuredArchive::FRecord::EnterField(FArchiveFieldName Name)
	{
		return FSlot(Ar);
	}

	FORCEINLINE FStructuredArchive::FRecord FStructuredArchive::FRecord::EnterRecord(FArchiveFieldName Name)
	{
		return EnterField(Name).EnterRecord();
	}

	FORCEINLINE FStructuredArchive::FArray FStructuredArchive::FRecord::EnterArray(FArchiveFieldName Name, int32& Num)
	{
		return EnterField(Name).EnterArray(Num);
	}

	FORCEINLINE FStructuredArchive::FStream FStructuredArchive::FRecord::EnterStream(FArchiveFieldName Name)
	{
		return EnterField(Name).EnterStream();
	}

	FORCEINLINE FStructuredArchive::FMap FStructuredArchive::FRecord::EnterMap(FArchiveFieldName Name, int32& Num)
	{
		return EnterField(Name).EnterMap(Num);
	}

	FORCEINLINE TOptional<FStructuredArchive::FSlot> FStructuredArchive::FRecord::TryEnterField(FArchiveFieldName Name, bool bEnterWhenWriting)
	{
		if (Ar.Formatter.TryEnterField(Name, bEnterWhenWriting))
		{
			return TOptional<FStructuredArchive::FSlot>(FSlot(Ar));
		}
		else
		{
			return TOptional<FStructuredArchive::FSlot>();
		}
	}

	//////////// FStructuredArchive::FArray ////////////

	FORCEINLINE FStructuredArchive::FSlot FStructuredArchive::FArray::EnterElement()
	{
		return FSlot(Ar);
	}

	//////////// FStructuredArchive::FStream ////////////

	FORCEINLINE FStructuredArchive::FSlot FStructuredArchive::FStream::EnterElement()
	{
		return FSlot(Ar);
	}

	//////////// FStructuredArchive::FMap ////////////

	FORCEINLINE FStructuredArchive::FSlot FStructuredArchive::FMap::EnterElement(FString& Name)
	{
		Ar.Formatter.EnterMapElement(Name);
		return FSlot(Ar);
	}

#endif

