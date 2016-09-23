// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "JsonStructDeserializerBackend.h"
#include "Core.h"

class FAndroidJSStructDeserializerBackend
	: public FJsonStructDeserializerBackend
{
public:
	FAndroidJSStructDeserializerBackend(FAndroidJSScriptingRef InScripting, const FString& JsonString);

	virtual bool ReadProperty( UProperty* Property, UProperty* Outer, void* Data, int32 ArrayIndex ) override;

private:
	FAndroidJSScriptingRef Scripting;
	TArray<uint8> JsonData;
	FMemoryReader Reader;
};
