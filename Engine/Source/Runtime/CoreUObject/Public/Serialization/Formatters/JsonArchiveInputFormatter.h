// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Serialization/StructuredArchiveFormatter.h"

class FJsonObject;
class FJsonValue;

#if WITH_TEXT_ARCHIVE_SUPPORT

class FJsonArchiveInputFormatter : public FAnnotatedStructuredArchiveFormatter
{
public:
	FJsonArchiveInputFormatter(FArchive& InInner, TFunction<UObject* (const FString&)> InResolveObjectName = nullptr);
	virtual ~FJsonArchiveInputFormatter();

	virtual FArchive& GetUnderlyingArchive() override;

	virtual void EnterRecord() override;
	virtual void EnterRecord(TArray<FString>& OutKeys) override;
	virtual void LeaveRecord() override;
	virtual void EnterField(FArchiveFieldName Name) override;
	virtual void EnterField(FArchiveFieldName Name, EArchiveValueType& OutType) override;
	virtual void LeaveField() override;
	virtual bool TryEnterField(FArchiveFieldName Name, bool bEnterWhenSaving) override;

	virtual void EnterArray(int32& NumElements) override;
	virtual void LeaveArray() override;
	virtual void EnterArrayElement() override;
	virtual void EnterArrayElement(EArchiveValueType& OutType) override;
	virtual void LeaveArrayElement() override;

	virtual void EnterStream() override;
	virtual void EnterStream(int32& NumElements) override;
	virtual void LeaveStream() override;
	virtual void EnterStreamElement() override;
	virtual void EnterStreamElement(EArchiveValueType& OutType) override;
	virtual void LeaveStreamElement() override;

	virtual void EnterMap(int32& NumElements) override;
	virtual void LeaveMap() override;
	virtual void EnterMapElement(FString& Name) override;
	virtual void EnterMapElement(FString& OutName, EArchiveValueType& OutType) override;
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

	TFunction<UObject* (const FString&)> ResolveObjectName;
	TArray<TSharedPtr<FJsonObject>> ObjectStack;
	TArray<TSharedPtr<FJsonValue>> ValueStack;
	TArray<TMap<FString, TSharedPtr<FJsonValue>>::TIterator> MapIteratorStack;

	static FString EscapeFieldName(const TCHAR* Name);
	static FString UnescapeFieldName(const TCHAR* Name);

	static EArchiveValueType GetValueType(const FJsonValue &Value);
};

#endif
