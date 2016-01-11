// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"
#include <jni.h>

/*
Wrappers for Java classes.
TODO: make function look up static? It doesn't matter for our case here (we only create one instance of the class ever)
*/
struct FJavaClassMethod
{
	FName		Name;
	FName		Signature;
	jmethodID	Method;
};

class FJavaClassObject
{
public:
	// !!  All Java objects returned by JNI functions are local references.
	FJavaClassObject(FName ClassName, const char* CtorSig, ...);

	~FJavaClassObject();

	FJavaClassMethod GetClassMethod(const char* MethodName, const char* FuncSig) const;

	// TODO: Define this for extra cases
	template<typename ReturnType>
	ReturnType CallMethod(FJavaClassMethod Method, ...) const;

	FORCEINLINE jobject GetJObject() const
	{
		return Object;
	}

	static jstring GetJString(const FString& String);

	void VerifyException() const;

protected:

	jobject			Object;
	jclass			Class;

private:
	FJavaClassObject(const FJavaClassObject& rhs);
	FJavaClassObject& operator = (const FJavaClassObject& rhs);
};

template<>
void FJavaClassObject::CallMethod<void>(FJavaClassMethod Method, ...) const;

template<>
bool FJavaClassObject::CallMethod<bool>(FJavaClassMethod Method, ...) const;

template<>
int FJavaClassObject::CallMethod<int>(FJavaClassMethod Method, ...) const;

template<>
jobject FJavaClassObject::CallMethod<jobject>(FJavaClassMethod Method, ...) const;

template<>
int64 FJavaClassObject::CallMethod<int64>(FJavaClassMethod Method, ...) const;

template<>
FString FJavaClassObject::CallMethod<FString>(FJavaClassMethod Method, ...) const;
