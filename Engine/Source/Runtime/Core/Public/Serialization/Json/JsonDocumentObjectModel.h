// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class FJsonObject;

/**
 * A Json Value is a structure that can be any of the Json Types.
 * It should never be used on its, only its derived types should be used.
 */
class CORE_API FJsonValue
{
public:
	/** Returns this value as a double, logging an error and returning zero if this is not an Json Number */
	double AsNumber() const;
	
	/** Returns this value as a number, logging an error and returning an empty string if not possible */
	FString AsString() const;

	/** Returns this value as a boolean, logging an error and returning false if not possible */
	bool AsBool() const;

	/** Returns this value as an array, logging an error and returning an empty array reference if not possible */
	const TArray< TSharedPtr<FJsonValue> >& AsArray() const;

	/** Returns this value as an object, throwing an error if this is not an Json Object */
	virtual const TSharedPtr<FJsonObject>& AsObject() const;

	/** Tries to convert this value to a number, returning false if not possible */
	virtual bool TryGetNumber(double& OutDouble) const { return false; }

	/** Tries to convert this value to an integer, returning false if not possible */
	bool TryGetNumber(int32& OutNumber) const;

	/** Tries to convert this value to an integer, returning false if not possible */
	bool TryGetNumber(uint32& OutNumber) const;

	/** Tries to convert this value to a string, returning false if not possible */
	virtual bool TryGetString(FString& OutString) const { return false; }

	/** Tries to convert this value to a bool, returning false if not possible */
	virtual bool TryGetBool(bool& OutBool) const { return false; }

	/** Tries to convert this value to an array, returning false if not possible */
	virtual bool TryGetArray(const TArray< TSharedPtr<FJsonValue> >*& OutArray) const { return false; }

	/** Tries to convert this value to an object, returning false if not possible */
	virtual bool TryGetObject(const TSharedPtr<FJsonObject>*& Object) const { return false; }

	/** Returns true if this value is a 'null' */
	bool IsNull() const { return Type == EJson::Null || Type == EJson::None; }

	/** Get a field of the same type as the argument */
	void AsArgumentType(double                          & Value) { Value = AsNumber(); }
	void AsArgumentType(FString                         & Value) { Value = AsString(); }
	void AsArgumentType(bool                            & Value) { Value = AsBool  (); }
	void AsArgumentType(TArray< TSharedPtr<FJsonValue> >& Value) { Value = AsArray (); }
	void AsArgumentType(TSharedPtr<FJsonObject>         & Value) { Value = AsObject(); }

	EJson::Type Type;

	static bool CompareEqual(const FJsonValue& Lhs, const FJsonValue& Rhs);
	bool operator==(const FJsonValue& Rhs) const { return CompareEqual(*this, Rhs); }

protected:

	static const TArray< TSharedPtr<FJsonValue> > EMPTY_ARRAY;
	static const TSharedPtr<FJsonObject> EMPTY_OBJECT;

	FJsonValue() : Type(EJson::None) {}
	virtual ~FJsonValue() {}

	virtual FString GetType() const = 0;

	void ErrorMessage(const FString& InType) const {UE_LOG(LogJson, Error, TEXT("Json Value of type '%s' used as a '%s'."), *GetType(), *InType);}
};

/** A Json String Value. */
class CORE_API FJsonValueString : public FJsonValue
{
public:
	FJsonValueString(const FString& InString) : Value(InString) {Type = EJson::String;}

	virtual bool TryGetString(FString& OutString) const override	{ OutString = Value; return true; }
	virtual bool TryGetNumber(double& OutDouble) const override		{ if(Value.IsNumeric()){ OutDouble = FCString::Atod(*Value); return true; } else { return false; } }
	virtual bool TryGetBool(bool& OutBool) const override			{ OutBool = Value.ToBool(); return true; }

protected:
	FString Value;

	virtual FString GetType() const {return TEXT("String");}
};

/** A Json Number Value. */
class CORE_API FJsonValueNumber : public FJsonValue
{
public:
	FJsonValueNumber(double InNumber) : Value(InNumber) {Type = EJson::Number;}
	virtual bool TryGetNumber(double& OutNumber) const override		{ OutNumber = Value; return true; }
	virtual bool TryGetBool(bool& OutBool) const override			{ OutBool = (Value != 0.0); return true; }
	virtual bool TryGetString(FString& OutString) const override	{ OutString = FString::SanitizeFloat(Value); return true; }
	
protected:
	double Value;

	virtual FString GetType() const {return TEXT("Number");}
};

/** A Json Boolean Value. */
class CORE_API FJsonValueBoolean : public FJsonValue
{
public:
	FJsonValueBoolean(bool InBool) : Value(InBool) {Type = EJson::Boolean;}
	virtual bool TryGetNumber(double& OutNumber) const override		{ OutNumber = Value ? 1 : 0; return true; }
	virtual bool TryGetBool(bool& OutBool) const override			{ OutBool = Value; return true; }
	virtual bool TryGetString(FString& OutString) const override	{ OutString = Value ? TEXT("true") : TEXT("false"); return true; }
	
protected:
	bool Value;

	virtual FString GetType() const {return TEXT("Boolean");}
};

/** A Json Array Value. */
class CORE_API FJsonValueArray : public FJsonValue
{
public:
	FJsonValueArray(const TArray< TSharedPtr<FJsonValue> >& InArray) : Value(InArray) {Type = EJson::Array;}
	virtual bool TryGetArray(const TArray< TSharedPtr<FJsonValue> >*& OutArray) const override	{ OutArray = &Value; return true; }
	
protected:
	TArray< TSharedPtr<FJsonValue> > Value;

	virtual FString GetType() const {return TEXT("Array");}
};

/** A Json Object Value. */
class CORE_API FJsonValueObject : public FJsonValue
{
public:
	FJsonValueObject(TSharedPtr<FJsonObject> InObject) : Value(InObject) {Type = EJson::Object;}
	virtual bool TryGetObject(const TSharedPtr<FJsonObject>*& OutObject) const override			{ OutObject = &Value; return true; }
	
protected:
	TSharedPtr<FJsonObject> Value;

	virtual FString GetType() const {return TEXT("Object");}
};

/** A Json Null Value. */
class CORE_API FJsonValueNull : public FJsonValue
{
public:
	FJsonValueNull() {Type = EJson::Null;}

protected:
	virtual FString GetType() const {return TEXT("Null");}
};


/**
 * A Json Object is a structure holding an unordered set of name/value pairs.
 * In a Json file, it is represented by everything between curly braces {}.
 */
class CORE_API FJsonObject
{
public:
	TMap< FString, TSharedPtr<FJsonValue> > Values;

	template<EJson::Type JsonType>
	TSharedPtr<FJsonValue> GetField( const FString& FieldName ) const
	{
		const TSharedPtr<FJsonValue>* Field = Values.Find(FieldName);
		if ( Field != NULL && Field->IsValid() )
		{
			if (JsonType == EJson::None || (*Field)->Type == JsonType)
			{
				return (*Field);
			}
			else
			{
				UE_LOG(LogJson, Warning, TEXT("Field %s is of the wrong type."), *FieldName);
			}
		}
		else
		{
			UE_LOG(LogJson, Warning, TEXT("Field %s was not found."), *FieldName);
		}

		return TSharedPtr<FJsonValue>(new FJsonValueNull());
	}

	TSharedPtr<FJsonValue> TryGetField( const FString& FieldName ) const
	{
		const TSharedPtr<FJsonValue>* Field = Values.Find(FieldName);
		return (Field != NULL && Field->IsValid()) ? *Field : TSharedPtr<FJsonValue>();
	}

	/** Checks to see if the FieldName exists in the object. */
	bool HasField( const FString& FieldName) const
	{
		const TSharedPtr<FJsonValue>* Field = Values.Find(FieldName);
		if(Field && Field->IsValid())
		{
			return true;
		}

		return false;
	}
	
	/** Checks to see if the FieldName exists in the object, and has the specified type. */
	template<EJson::Type JsonType>
	bool HasTypedField(const FString& FieldName) const
	{
		const TSharedPtr<FJsonValue>* Field = Values.Find(FieldName);
		if(Field && Field->IsValid() && ((*Field)->Type == JsonType))
		{
			return true;
		}

		return false;
	}

	void SetField( const FString& FieldName, const TSharedPtr<FJsonValue>& Value );

	void RemoveField(const FString& FieldName);

	/** Get the field named FieldName as a number. Ensures that the field is present and is of type Json number. */
	double GetNumberField(const FString& FieldName) const;

	/** Get the field named FieldName as a number. Returns false if it cannot be converted. */
	bool TryGetNumberField(const FString& FieldName, double& OutNumber) const;

	/** Get the field named FieldName as a number, and makes sure it's within int32 range. Returns false if it cannot be converted. */
	bool TryGetNumberField(const FString& FieldName, int32& OutNumber) const;

	/** Get the field named FieldName as a number, and makes sure it's within uint32 range. Returns false if it cannot be converted.  */
	bool TryGetNumberField(const FString& FieldName, uint32& OutNumber) const;

	/** Add a field named FieldName with Number as value */
	void SetNumberField( const FString& FieldName, double Number );

	/** Get the field named FieldName as a string. */
	FString GetStringField(const FString& FieldName) const;

	/** Get the field named FieldName as a string. Returns false if it cannot be converted. */
	bool TryGetStringField(const FString& FieldName, FString& OutString) const;

	/** Add a field named FieldName with value of StringValue */
	void SetStringField( const FString& FieldName, const FString& StringValue );

	/** Get the field named FieldName as a boolean. */
	bool GetBoolField(const FString& FieldName) const;

	/** Get the field named FieldName as a string. Returns false if it cannot be converted. */
	bool TryGetBoolField(const FString& FieldName, bool& OutBool) const;

	/** Set a boolean field named FieldName and value of InValue */
	void SetBoolField( const FString& FieldName, bool InValue );

	/** Get the field named FieldName as an array. */
	const TArray< TSharedPtr<FJsonValue> >& GetArrayField(const FString& FieldName) const;

	/** Try to get the field named FieldName as an array, or return false if it's another type */
	bool TryGetArrayField(const FString& FieldName, const TArray< TSharedPtr<FJsonValue> >*& OutArray) const;

	/** Set an array field named FieldName and value of Array */
	void SetArrayField( const FString& FieldName, const TArray< TSharedPtr<FJsonValue> >& Array );

	/** Get the field named FieldName as a Json object. */
	const TSharedPtr<FJsonObject>& GetObjectField(const FString& FieldName) const;

	/** Try to get the field named FieldName as an object, or return false if it's another type */
	bool TryGetObjectField(const FString& FieldName, const TSharedPtr<FJsonObject>*& OutObject) const;

	/** Set an ObjectField named FieldName and value of JsonObject */
	void SetObjectField( const FString& FieldName, const TSharedPtr<FJsonObject>& JsonObject );
};
