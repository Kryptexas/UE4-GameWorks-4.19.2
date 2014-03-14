// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	JsonMessageSerializer.cpp: Implements the FJsonMessageSerializer structure.
=============================================================================*/

#include "MessagingPrivatePCH.h"


struct FJsonStructReadState
{
	// Holds a pointer to the property's data.
	void* Data;

	// Holds the property's meta data.
	UProperty* Property;

	// Holds a pointer to the UStruct describing the data.
	UStruct* TypeInfo;
};

struct FJsonStructWriteState
{
	// Holds a pointer to the property's data.
	const void* Data;

	// Holds a flag indicating whether the property has been processed.
	bool HasBeenProcessed;

	// Holds the property's meta data.
	UProperty* Property;

	// Holds a pointer to the UStruct describing the data.
	UStruct* TypeInfo;
};


// @todo gmp - refactor this and SlateStyleSerializer into generic UStruct serializer
namespace Obsolete
{
	// Finds the class for the given stack state.
	UStruct* FindClass( const FJsonStructReadState& State )
	{
		UStruct* Class = nullptr;

		if (State.Property != nullptr)
		{
			UProperty* ParentProperty = State.Property;
			UArrayProperty* ArrayProperty = Cast<UArrayProperty>(ParentProperty);

			if (ArrayProperty != nullptr)
			{
				ParentProperty = ArrayProperty->Inner;
			}

			UStructProperty* StructProperty = Cast<UStructProperty>(ParentProperty);
			UObjectPropertyBase* ObjectProperty = Cast<UObjectPropertyBase>(ParentProperty);

			if (StructProperty != nullptr)
			{
				Class = StructProperty->Struct;
			}
			else if (ObjectProperty != nullptr)
			{
				Class = ObjectProperty->PropertyClass;
			}
		}
		else
		{
			UObject* RootObject = static_cast<UObject*>(State.Data);
			Class = RootObject->GetClass();
		}

		return Class;
	}


	// Finds the specified property for the given stack state.
	UProperty* FindProperty( const FJsonStructReadState& State, const FString& Identifier )
	{
		UArrayProperty* ArrayProperty = Cast<UArrayProperty>( State.Property );
		if ( ArrayProperty )
		{
			return ArrayProperty->Inner;
		}

		return FindField<UProperty>( State.TypeInfo, *Identifier );
	}


	// Gets the value from the given property.
	template< typename UPropertyType, typename PropertyType >
	PropertyType* GetPropertyValue( const FJsonStructWriteState& State, UProperty* Property )
	{
		PropertyType* ValuePtr = nullptr;
		UArrayProperty* ArrayProperty = Cast<UArrayProperty>(State.Property);

		if (ArrayProperty)
		{
			check(ArrayProperty->Inner == Property);

			FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->template ContainerPtrToValuePtr<void>(State.Data));
			int32 Index = ArrayHelper.AddValue();
		
			ValuePtr = (PropertyType*)ArrayHelper.GetRawPtr( Index );
		}
		else
		{
			UPropertyType* TypedProperty = Cast<UPropertyType>( Property );
			check( TypedProperty )

			ValuePtr = TypedProperty->template ContainerPtrToValuePtr<PropertyType>(State.Data);
		}

		return ValuePtr;
	}


	// Gets the value from the given property.
	template< typename UPropertyType, typename PropertyType >
	PropertyType* GetPropertyValue( const FJsonStructReadState& State, UProperty* Property )
	{
		PropertyType* ValuePtr = nullptr;
		UArrayProperty* ArrayProperty = Cast<UArrayProperty>(State.Property);

		if (ArrayProperty)
		{
			check(ArrayProperty->Inner == Property);

			FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->template ContainerPtrToValuePtr<void>(State.Data));
			int32 Index = ArrayHelper.AddValue();
		
			ValuePtr = (PropertyType*)ArrayHelper.GetRawPtr( Index );
		}
		else
		{
			UPropertyType* TypedProperty = Cast<UPropertyType>( Property );
			check( TypedProperty )

			ValuePtr = TypedProperty->template ContainerPtrToValuePtr<PropertyType>(State.Data);
		}

		return ValuePtr;
	}


	// Sets the value of the given property.
	template< typename UPropertyType, typename PropertyType >
	void SetPropertyValue( const FJsonStructReadState& State, UProperty* Property, const PropertyType& Value )
	{
		PropertyType* ValuePtr = GetPropertyValue<UPropertyType, PropertyType>(State , Property);
		*ValuePtr = Value;
	}


	bool ReadExpectedArrayValue( const TSharedRef< TJsonReader<UCS2CHAR> >& Reader, const EJsonNotation::Type ExpectNotation, const FString& ErrorMessage )
	{
		bool ReadCorrectValue = true;

		EJsonNotation::Type Notation;
		if ( !Reader->ReadNext( Notation ) || Notation != ExpectNotation )
		{
			ReadCorrectValue = false;

			UE_LOG( LogJson, Log, TEXT("%s Line: %u Ch: %u"), *ErrorMessage, Reader->GetLineNumber(), Reader->GetCharacterNumber() );
		
			if ( Notation != EJsonNotation::ArrayEnd )
			{
				Reader->SkipArray();
			}
		}

		return ReadCorrectValue;
	}


	// Writes a property value to the serialization output. Array elements will be written without their name.
	template<typename ValueType>
	void SerializeProperty( const TSharedRef<TJsonWriter<UCS2CHAR, TPrettyJsonPrintPolicy<UCS2CHAR> > > Writer, UProperty* Property, const ValueType& Value)
	{
		if ((Property == nullptr) || (Property->GetOuter()->GetClass() == UArrayProperty::StaticClass()))
		{
			Writer->WriteValue(Value);
		}
		else
		{
			Writer->WriteValue(Property->GetName(), Value);
		}
	}
}


/* FJsonMessageSerializer implementation
 *****************************************************************************/

bool FJsonMessageSerializer::DeserializeMessage( FArchive& Archive, IMutableMessageContextRef& OutContext )
{
	// deserialize context
	FMessageAddress Sender;
	Archive << Sender;
	OutContext->SetSender(Sender);

	TArray<FMessageAddress> Recipients;
	Archive << Recipients;

	for (int32 RecipientIndex = 0; RecipientIndex < Recipients.Num(); ++RecipientIndex)
	{
		OutContext->AddRecipient(Recipients[RecipientIndex]);
	}

	TEnumAsByte<EMessageScope::Type> Scope;
	Archive << Scope;
	OutContext->SetScope(Scope);

	FDateTime TimeSent;
	Archive << TimeSent;
	OutContext->SetTimeSent(TimeSent);

	FDateTime Expiration;
	Archive << Expiration;
	OutContext->SetExpiration(Expiration);

	TMap<FName, FString> Headers;
	Archive << Headers;

	for (TMap<FName, FString>::TConstIterator It(Headers); It; ++It)
	{
		OutContext->SetHeader(It.Key(), It.Value());
	}

	// serialize message
	TSharedRef<TJsonReader<UCS2CHAR> > Reader = TJsonReader<UCS2CHAR>::Create(&Archive);

	return DeserializeStruct(OutContext, Reader);
}


bool FJsonMessageSerializer::DeserializeStruct( IMutableMessageContextRef& OutContext, const TSharedRef<TJsonReader<UCS2CHAR> >& Reader ) const
{
	bool bFirstObjectStartNotation = true;

	TArray<FJsonStructReadState> StateStack;
	FJsonStructReadState CurrentState;

	CurrentState.Data = nullptr;
	CurrentState.Property = nullptr;
	CurrentState.TypeInfo = nullptr;

	EJsonNotation::Type Notation;

	while (Reader->ReadNext(Notation))
	{
		FString Identifier = Reader->GetIdentifier();

		switch (Notation)
		{
		// arrays
		case EJsonNotation::ArrayEnd:
			{
				CurrentState = StateStack.Pop();
			}

			break;

		case EJsonNotation::ArrayStart:
			{
				check( bFirstObjectStartNotation == false );
				StateStack.Push( CurrentState );

				FJsonStructReadState NewState;
				NewState.Data = CurrentState.Data;

				//TODO: Handle arrays of arrays
				// This does not handle arrays of arrays, merely arrays of objects or structures
				NewState.Property = FindField<UProperty>( CurrentState.TypeInfo, *Identifier );

				if ( NewState.Property == nullptr )
				{
					CurrentState = StateStack.Pop();
					Reader->SkipArray();

					UE_LOG( LogJson, Verbose, TEXT("Array member '%s' was unhandled. Line: %u Ch: %u"), *Identifier, Reader->GetLineNumber(), Reader->GetCharacterNumber() );
				}
				else
				{
					NewState.TypeInfo = Obsolete::FindClass( NewState );
					CurrentState = NewState;
				}
			}
			break;

		// booleans
		case EJsonNotation::Boolean:
			{
				UProperty* Property = Obsolete::FindProperty(CurrentState, *Identifier);

				if (Property == nullptr)
				{
					UE_LOG(LogJson, Verbose, TEXT("The property %s does not exist (Line: %u, Ch: %u)"), *Identifier, Reader->GetLineNumber(), Reader->GetCharacterNumber());
				}
				else
				{
					bool BoolValue = Reader->GetValueAsBoolean();

					if (Property->GetClass() == UBoolProperty::StaticClass())
					{
						Obsolete::SetPropertyValue<UBoolProperty, bool>(CurrentState, Property, BoolValue);
					}
					else
					{
						UE_LOG(LogJson, Verbose, TEXT("Boolean field %s with value '%s' is not supported in UProperty type %s (Line: %u, Ch: %u)"), *Identifier, BoolValue ? *(GTrue.ToString()) : *(GFalse.ToString()), *Property->GetClass()->GetName(), Reader->GetLineNumber(), Reader->GetCharacterNumber());
					}
				}
			}

			break;

		// numeric values
		case EJsonNotation::Number:
			{
				UProperty* Property = Obsolete::FindProperty(CurrentState, *Identifier);

				if (Property == nullptr)
				{
					UE_LOG(LogJson, Verbose, TEXT("The property %s does not exist (Line: %u, Ch: %u)"), *Identifier, Reader->GetLineNumber(), Reader->GetCharacterNumber());
				}
				else
				{
					float NumericValue = Reader->GetValueAsNumber();

					if (Property->GetClass() == UByteProperty::StaticClass())
					{
						Obsolete::SetPropertyValue<UByteProperty, int8>( CurrentState, Property, (int8)NumericValue);
					}
					else if (Property->GetClass() == UFloatProperty::StaticClass())
					{
						Obsolete::SetPropertyValue<UFloatProperty, float>(CurrentState, Property, NumericValue);
					}
					else if (Property->GetClass() == UDoubleProperty::StaticClass())
					{
						Obsolete::SetPropertyValue<UDoubleProperty, double>(CurrentState, Property, NumericValue);
					}
					else if (Property->GetClass() == UIntProperty::StaticClass())
					{
						Obsolete::SetPropertyValue<UIntProperty, int32>( CurrentState, Property, (int32)NumericValue);
					}
					else if (Property->GetClass() == UUInt32Property::StaticClass())
					{
						Obsolete::SetPropertyValue<UUInt32Property, uint32>( CurrentState, Property, (uint32)NumericValue);
					}
					else if (Property->GetClass() == UInt16Property::StaticClass())
					{
						Obsolete::SetPropertyValue<UInt16Property, int16>( CurrentState, Property, (int16)NumericValue);
					}
					else if (Property->GetClass() == UUInt16Property::StaticClass())
					{
						Obsolete::SetPropertyValue<UUInt16Property, uint16>( CurrentState, Property, (uint16)NumericValue);
					}
					else if (Property->GetClass() == UInt64Property::StaticClass())
					{
						Obsolete::SetPropertyValue<UInt64Property, int64>( CurrentState, Property, (int64)NumericValue);
					}
					else if (Property->GetClass() == UUInt64Property::StaticClass())
					{
						Obsolete::SetPropertyValue<UUInt64Property, uint64>( CurrentState, Property, (uint64)NumericValue);
					}
					else if (Property->GetClass() == UInt8Property::StaticClass())
					{
						Obsolete::SetPropertyValue<UInt8Property, int8>( CurrentState, Property, (int8)NumericValue);
					}
					else
					{
						UE_LOG(LogJson, Verbose, TEXT("Numeric field %s with value '%f' is not supported in UProperty type %s (Line: %u, Ch: %u)"), *Identifier, NumericValue, *Property->GetClass()->GetName(), Reader->GetLineNumber(), Reader->GetCharacterNumber());
					}
				}
			}

			break;

		// objects & structures
		case EJsonNotation::ObjectEnd:
			{
				if (StateStack.Num() > 0)
				{
					CurrentState = StateStack.Pop();
				}
			}

			break;

		case EJsonNotation::ObjectStart:
			{
				if (bFirstObjectStartNotation)
				{
					if (Reader->ReadNext(Notation) && (Notation == EJsonNotation::String) && (Reader->GetIdentifier() == TEXT("__type")))
					{
						const FString& TypeName = Reader->GetValueAsString();				
						UScriptStruct* ScriptStruct = FMessageTypeMap::MessageTypeMap.Find(*TypeName);

						if (ScriptStruct != nullptr)
						{
							CurrentState.Data = FMemory::Malloc(ScriptStruct->PropertiesSize);
							CurrentState.Property = nullptr;
							CurrentState.TypeInfo = ScriptStruct;
								
							ScriptStruct->InitializeScriptStruct(CurrentState.Data);

							bFirstObjectStartNotation = false;
						}
						else
						{
							//UE_LOG(LogJson, Verbose, TEXT("The message type %s could not be found"), *TypeName);

							return false;
						}
					}
					else
					{
						UE_LOG(LogJson, Verbose, TEXT("Invalid Json object"));

						return false;
					}
				}
				else
				{
					StateStack.Push(CurrentState);

					FJsonStructReadState NewState;

					if (Identifier.IsEmpty())
					{
						UArrayProperty* ArrayProperty = Cast<UArrayProperty>( CurrentState.Property );

						check(ArrayProperty);

						FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(CurrentState.Data));
						const int32 ArrayIndex = ArrayHelper.AddValue();
						NewState.Property = CurrentState.Property;
						NewState.Data = ArrayHelper.GetRawPtr(ArrayIndex);
					}
					else
					{
						NewState.Property = FindField<UProperty>(CurrentState.TypeInfo, *Identifier );

						if (NewState.Property != nullptr)
						{
							NewState.Data = NewState.Property->ContainerPtrToValuePtr<void>(CurrentState.Data);
						}
					}

					if (NewState.Property == nullptr)
					{
						CurrentState = StateStack.Pop();
						Reader->SkipObject();

						UE_LOG(LogJson, Verbose, TEXT("Json object member '%s' was unhandled. Line: %u Ch: %u"), *Identifier, Reader->GetLineNumber(), Reader->GetCharacterNumber());
					}
					else
					{
						NewState.TypeInfo = Obsolete::FindClass(NewState);
						CurrentState = NewState;
					}
				}
			}
			break;

		// strings, names & enumerations
		case EJsonNotation::String:
			{
				UProperty* Property = Obsolete::FindProperty(CurrentState, *Identifier);

				if (Property == nullptr)
				{
					UE_LOG(LogJson, Verbose, TEXT("The property %s does not exist (Line: %u, Ch: %u)"), *Identifier, Reader->GetLineNumber(), Reader->GetCharacterNumber());
				}
				else
				{
					const FString& StringValue = Reader->GetValueAsString();

					if (Property->GetClass() == UStrProperty::StaticClass())
					{
						Obsolete::SetPropertyValue<UStrProperty, FString>(CurrentState, Property, StringValue);
					}
					else if (Property->GetClass() == UNameProperty::StaticClass())
					{
						Obsolete::SetPropertyValue<UNameProperty, FName>(CurrentState, Property, *StringValue);
					}
					else if (Property->GetClass() == UByteProperty::StaticClass())
					{
						UByteProperty* ByteProperty = Cast<UByteProperty>(Property);
						int32 Index = ByteProperty->Enum->FindEnumIndex(*StringValue);

						if (Index != INDEX_NONE)
						{
							Obsolete::SetPropertyValue<UByteProperty, uint8>(CurrentState, Property, (uint8)Index);
						}
					}
					else if (Property->GetClass() == UArrayProperty::StaticClass())
					{
						UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property);

						if (ArrayProperty->Inner->GetClass() == UByteProperty::StaticClass())
						{
							int32 ArrayNum = StringValue.Len() / 3;

							if (ArrayNum > 0)
							{
								FScriptArrayHelper_InContainer Helper(ArrayProperty, CurrentState.Data);
								Helper.EmptyAndAddValues(ArrayNum);
						
								FString::ToBlob(StringValue, Helper.GetRawPtr(), ArrayNum);
							}
						}
						else
						{
							// strings can only be decoded into byte arrays
							return false;
						}
					}
					else
					{
						UStructProperty* StructProperty = Cast<UStructProperty>(Property);

						if ((StructProperty != nullptr) && (StructProperty->Struct->GetName() == TEXT("Guid")))
						{
							FGuid ParsedGuid;
								
							if (FGuid::Parse(StringValue, ParsedGuid))
							{
								FGuid* Guid = StructProperty->ContainerPtrToValuePtr<FGuid>((void*)CurrentState.Data);
								*Guid = ParsedGuid;
							}
							else
							{
								UE_LOG(LogJson, Verbose, TEXT("GUID field %s has a malformed value '%s'"), *Identifier, *StringValue);
							}
						}
						else
						{
							UE_LOG(LogJson, Verbose, TEXT("String field %s with value '%s' is not supported in UProperty type %s (Line: %u, Ch: %u)"), *Identifier, *StringValue, *Property->GetClass()->GetName(), Reader->GetLineNumber(), Reader->GetCharacterNumber());
						}							
					}
				}
			}

			break;

		// null values & errors
		case EJsonNotation::Error:
			{
				UE_LOG(LogJson, Verbose, TEXT("Format error: %s"), *Reader->GetErrorMessage());

				return false;
			}

		case EJsonNotation::Null:
			{
				return false;
			}

		default:

			continue;
		}
	}

	if (!Reader->GetErrorMessage().IsEmpty())
	{
		return false;
	}

	OutContext->SetMessage(CurrentState.Data, (UScriptStruct*)CurrentState.TypeInfo);

	return (CurrentState.Data != nullptr);
}


bool FJsonMessageSerializer::SerializeMessage( const IMessageContextRef& Context, FArchive& Archive )
{
	if (Context->IsValid())
	{
		// serialize context
		FMessageAddress Sender = Context->GetSender();
		Archive << Sender;

		TArray<FMessageAddress> Recipients = Context->GetRecipients();
		Archive << Recipients;

		TEnumAsByte<EMessageScope::Type> Scope = Context->GetScope();
		Archive << Scope;

		FDateTime TimeSent = Context->GetTimeSent();
		Archive << TimeSent;

		FDateTime Expiration = Context->GetExpiration();
		Archive << Expiration;

		TMap<FName, FString> Headers = Context->GetHeaders();
		Archive << Headers;

		// serialize message
		TSharedRef<TJsonWriter<UCS2CHAR> > Writer = TJsonWriter<UCS2CHAR>::Create(&Archive);

		return SerializeStruct(Context->GetMessage(), *Context->GetMessageTypeInfo(), Writer);
	}

	return false;
}


bool FJsonMessageSerializer::SerializeStruct( const void* Data, UStruct& TypeInfo, const TSharedRef<TJsonWriter<UCS2CHAR> >& Writer )
{
	// initialize the state stack
	TArray<FJsonStructWriteState> StateStack;
	FJsonStructWriteState StructState;

	StructState.Data = Data;
	StructState.Property = nullptr;
	StructState.TypeInfo = &TypeInfo;
	StructState.HasBeenProcessed = false;

	StateStack.Push(StructState);

	// process state stack
	while (StateStack.Num() > 0)
	{
		FJsonStructWriteState CurrentState = StateStack.Pop();

		const void* CurrentData = CurrentState.Data;
		UProperty* CurrentProperty = CurrentState.Property;
		UStruct* CurrentTypeInfo = CurrentState.TypeInfo;

		// booleans
		if (CurrentTypeInfo == UBoolProperty::StaticClass())
		{
			Obsolete::SerializeProperty(Writer, CurrentProperty, Cast<UBoolProperty>(CurrentProperty)->GetPropertyValue_InContainer(CurrentData));
		}

		// bytes & enumerations
		else if (CurrentTypeInfo == UByteProperty::StaticClass())
		{
			UByteProperty* ByteProperty = Cast<UByteProperty>(CurrentProperty);

			if (ByteProperty->IsEnum())
			{
				Obsolete::SerializeProperty(Writer, CurrentProperty, ByteProperty->Enum->GetEnumName(ByteProperty->GetPropertyValue_InContainer(CurrentData)));
			}
			else
			{
				Obsolete::SerializeProperty(Writer, CurrentProperty, (double)Cast<UByteProperty>(CurrentProperty)->GetPropertyValue_InContainer(CurrentData));
			}			
		}

		// floats
		else if (CurrentTypeInfo == UFloatProperty::StaticClass())
		{
			Obsolete::SerializeProperty(Writer, CurrentProperty, Cast<UFloatProperty>(CurrentProperty)->GetPropertyValue_InContainer(CurrentData));
		}
		else if (CurrentTypeInfo == UDoubleProperty::StaticClass())
		{
			double value = Cast<UDoubleProperty>(CurrentProperty)->GetPropertyValue_InContainer(CurrentData);
			Obsolete::SerializeProperty(Writer, CurrentProperty, value);
		}

		// signed integers
		else if (CurrentTypeInfo == UIntProperty::StaticClass())
		{
			Obsolete::SerializeProperty(Writer, CurrentProperty, (double)Cast<UIntProperty>(CurrentProperty)->GetPropertyValue_InContainer(CurrentData));
		}
		else if (CurrentTypeInfo == UInt8Property::StaticClass())
		{
			Obsolete::SerializeProperty(Writer, CurrentProperty, (double)Cast<UInt8Property>(CurrentProperty)->GetPropertyValue_InContainer(CurrentData));
		}
		else if (CurrentTypeInfo == UInt16Property::StaticClass())
		{
			Obsolete::SerializeProperty(Writer, CurrentProperty, (double)Cast<UInt16Property>(CurrentProperty)->GetPropertyValue_InContainer(CurrentData));
		}
		else if (CurrentTypeInfo == UInt64Property::StaticClass())
		{
			Obsolete::SerializeProperty(Writer, CurrentProperty, (double)Cast<UInt64Property>(CurrentProperty)->GetPropertyValue_InContainer(CurrentData));
		}

		// unsigned integers
		else if (CurrentTypeInfo == UUInt16Property::StaticClass())
		{
			Obsolete::SerializeProperty(Writer, CurrentProperty, (double)Cast<UUInt16Property>(CurrentProperty)->GetPropertyValue_InContainer(CurrentData));
		}
		else if (CurrentTypeInfo == UUInt32Property::StaticClass())
		{
			Obsolete::SerializeProperty(Writer, CurrentProperty, (double)Cast<UUInt32Property>(CurrentProperty)->GetPropertyValue_InContainer(CurrentData));
		}
		else if (CurrentTypeInfo == UUInt64Property::StaticClass())
		{
			Obsolete::SerializeProperty(Writer, CurrentProperty, (double)Cast<UUInt64Property>(CurrentProperty)->GetPropertyValue_InContainer(CurrentData));
		}

		// names & strings
		else if (CurrentTypeInfo == UNameProperty::StaticClass())
		{
			Obsolete::SerializeProperty(Writer, CurrentProperty, Cast<UNameProperty>(CurrentProperty)->GetPropertyValue_InContainer(CurrentData).ToString());
		}
		else if (CurrentTypeInfo == UStrProperty::StaticClass())
		{
			Obsolete::SerializeProperty(Writer, CurrentProperty, Cast<UStrProperty>(CurrentProperty)->GetPropertyValue_InContainer(CurrentData));
		}

		// single & multi-dimensional arrays
		else if (CurrentTypeInfo == UArrayProperty::StaticClass())
		{
			UArrayProperty* ArrayProperty = Cast<UArrayProperty>(CurrentProperty);
			UProperty* Inner = ArrayProperty->Inner;

			if (Inner->GetClass() == UInt8Property::StaticClass() || Inner->GetClass() == UByteProperty::StaticClass())
			{
				const TArray<uint8>* ArrayPtr = ArrayProperty->ContainerPtrToValuePtr<TArray<uint8> >(CurrentData);
				const uint8* Buffer = ArrayPtr->GetData();
				UObject* Outer = CurrentProperty->GetOuter();

				if ((Outer != nullptr) && (Outer->GetClass() == UArrayProperty::StaticClass()))
				{
					Writer->WriteValue(FString::FromBlob(Buffer, ArrayPtr->Num()));
				}
				else
				{
					Writer->WriteValue(CurrentProperty->GetName(), FString::FromBlob(Buffer, ArrayPtr->Num()));
				}
			}
			else if (CurrentState.HasBeenProcessed)
			{
				Writer->WriteArrayEnd();
			}
			else
			{
				// write array start
				UObject* Outer = CurrentProperty->GetOuter();

				if ((Outer != nullptr) && (Outer->GetClass() == UArrayProperty::StaticClass()))
				{
					Writer->WriteArrayStart();
				}
				else
				{
					Writer->WriteArrayStart(CurrentProperty->GetName());
				}

				CurrentState.HasBeenProcessed = true;
				StateStack.Push(CurrentState);

				// serialize elements
				FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(CurrentData));

				for (int Index = ArrayHelper.Num() - 1; Index >= 0; --Index)
				{
					FJsonStructWriteState ElementState;

					ElementState.Data = ArrayHelper.GetRawPtr(Index);
					ElementState.Property = Inner;
					ElementState.TypeInfo = Inner->GetClass();
					ElementState.HasBeenProcessed = false;

					StateStack.Push(ElementState);
				}
			}
		}

		// objects & structures
		else
		{
			if (CurrentState.HasBeenProcessed)
			{
				Writer->WriteObjectEnd();
			}
			else
			{
				const void* NewData = CurrentData;

				// write object start
				if (CurrentProperty == nullptr)
				{
					Writer->WriteObjectStart();
					Writer->WriteValue(TEXT("__type"), CurrentTypeInfo->GetFName().ToString());
				}
				else
				{
					UStructProperty* StructProperty = Cast<UStructProperty>(CurrentProperty);

					if ((StructProperty != nullptr) && (StructProperty->Struct->GetName() == TEXT("Guid")))
					{
						const FGuid* Guid = StructProperty->ContainerPtrToValuePtr<FGuid>(CurrentData);
						Writer->WriteValue(StructProperty->GetName(), Guid->ToString(EGuidFormats::DigitsWithHyphens));

						continue;
					}
					else
					{
						UObject* Outer = CurrentProperty->GetOuter();

						if ((Outer != nullptr) && (Outer->GetClass() == UArrayProperty::StaticClass()))
						{
							Writer->WriteObjectStart();
						}
						else
						{
							Writer->WriteObjectStart(CurrentProperty->GetName());
							NewData = CurrentProperty->ContainerPtrToValuePtr<void>(CurrentData);
						}
					}
				}

				CurrentState.HasBeenProcessed = true;
				StateStack.Push(CurrentState);

				// serialize fields
				if (CurrentProperty != nullptr)
				{
					UStructProperty* StructProperty = Cast<UStructProperty>(CurrentProperty);

					// @todo json: allow for custom serialization of specific types

					if (StructProperty != nullptr)
					{
						CurrentTypeInfo = StructProperty->Struct;
					}
					else
					{
						UObjectPropertyBase* ObjectProperty = Cast<UObjectPropertyBase>(CurrentProperty);

						if (ObjectProperty != nullptr)
						{
							CurrentTypeInfo = ObjectProperty->PropertyClass;
						}
					}
				}

				TArray<FJsonStructWriteState> FieldStates;

				for (TFieldIterator<UProperty> It(CurrentTypeInfo, EFieldIteratorFlags::IncludeSuper); It; ++It)
				{
					FJsonStructWriteState FieldState;

					FieldState.Data = NewData;
					FieldState.Property = *It;
					FieldState.TypeInfo = It->GetClass();
					FieldState.HasBeenProcessed = false;

					FieldStates.Add(FieldState);
				}

				// push backwards for readability
				for (int Index = FieldStates.Num() - 1; Index >= 0; --Index)
				{
					StateStack.Push(FieldStates[Index]);
				}
			}
		}
	}

	return Writer->Close();
}
