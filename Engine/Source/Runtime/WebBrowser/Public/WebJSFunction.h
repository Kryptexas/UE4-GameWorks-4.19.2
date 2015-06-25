// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreUObject.h"
#include "WebJSFunction.generated.h"

class FWebJSScripting;

struct WEBBROWSER_API FWebJSParam
{
	enum { PTYPE_NULL, PTYPE_BOOL, PTYPE_INT, PTYPE_DOUBLE, PTYPE_STRING, PTYPE_OBJECT, PTYPE_STRUCT, } Tag;
	union
	{
		bool BoolValue;
		double DoubleValue;
		int32 IntValue;
		UObject* ObjectValue;
		FString* StringValue;
		struct {
			UStruct* TypeInfo;
			void* StructPtr;
		} StructValue;
	};

	FWebJSParam() : Tag(PTYPE_NULL) {}
	FWebJSParam(bool Value) : Tag(PTYPE_BOOL), BoolValue(Value) {}
	FWebJSParam(int32 Value) : Tag(PTYPE_INT), IntValue(Value) {}
	FWebJSParam(double Value) : Tag(PTYPE_DOUBLE), DoubleValue(Value) {}
	FWebJSParam(const FString& Value) : Tag(PTYPE_STRING), StringValue(new FString(Value)) {}
	FWebJSParam(const TCHAR* Value) : Tag(PTYPE_STRING), StringValue(new FString(Value)) {}
	FWebJSParam(UObject* Value) : Tag(PTYPE_OBJECT), ObjectValue(Value) {}
	FWebJSParam(UStruct* Struct, void* Value) : Tag(PTYPE_STRUCT), StructValue({Struct, Value}) {}
	FWebJSParam(const FWebJSParam& Other);
	~FWebJSParam();

};

/** Add this macro to a USTRUCT definition to be able to pass instances to FWebJSFunctions */
#define DECLARE_JSPARAM_STRUCT() \
	explicit operator FWebJSParam() \
	{ \
		return FWebJSParam(StaticStruct(), (void*)this);\
	}

USTRUCT()
struct WEBBROWSER_API FWebJSFunction
{
	GENERATED_USTRUCT_BODY()

	FWebJSFunction()
		: ScriptingPtr()
		, FunctionId()
	{}

	FWebJSFunction(TSharedPtr<FWebJSScripting> InScripting, const FGuid& InFunctionId)
		: ScriptingPtr(InScripting)
		, FunctionId(InFunctionId)
	{}

#if PLATFORM_COMPILER_HAS_VARIADIC_TEMPLATES
	template<typename ...ArgTypes> void operator()(ArgTypes... Args) const
	{
		FWebJSParam ArgArray[sizeof...(Args)] = {FWebJSParam(Args)...};
		Invoke(sizeof...(Args), ArgArray);
	}
#else
	void operator()() const
	{
		Invoke(0, nullptr);
	}

	template<typename T1>
	void operator()(T1 Arg1) const
	{
		FWebJSParam ArgArray[1] = {FWebJSParam(Arg1)};
		Invoke(1, ArgArray);
	}

	template<typename T1, typename T2>
	void operator()(T1 Arg1, T2 Arg2) const
	{
		FWebJSParam ArgArray[2] = {FWebJSParam(Arg1), FWebJSParam(Arg2)};
		Invoke(2, ArgArray);
	}

	template<typename T1, typename T2, typename T3>
	void operator()(T1 Arg1, T2 Arg2, T3 Arg3) const
	{
		FWebJSParam ArgArray[3] = {FWebJSParam(Arg1), FWebJSParam(Arg2), FWebJSParam(Arg3)};
		Invoke(3, ArgArray);
	}

	template<typename T1, typename T2, typename T3, typename T4>
	void operator()(T1 Arg1, T2 Arg2, T3 Arg3, T4 Arg4) const
	{
		FWebJSParam ArgArray[4] = {FWebJSParam(Arg1), FWebJSParam(Arg2), FWebJSParam(Arg3), FWebJSParam(Arg4)};
		Invoke(4, ArgArray);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5>
	void operator()(T1 Arg1, T2 Arg2, T3 Arg3, T4 Arg4, T5 Arg5) const
	{
		FWebJSParam ArgArray[5] = {FWebJSParam(Arg1), FWebJSParam(Arg2), FWebJSParam(Arg3), FWebJSParam(Arg4), FWebJSParam(Arg5)};
		Invoke(5, ArgArray);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
	void operator()(T1 Arg1, T2 Arg2, T3 Arg3, T4 Arg4, T5 Arg5, T6 Arg6) const
	{
		FWebJSParam ArgArray[6] = {FWebJSParam(Arg1), FWebJSParam(Arg2), FWebJSParam(Arg3), FWebJSParam(Arg4), FWebJSParam(Arg5), FWebJSParam(Arg6)};
		Invoke(6, ArgArray);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
	void operator()(T1 Arg1, T2 Arg2, T3 Arg3, T4 Arg4, T5 Arg5, T6 Arg6, T7 Arg7) const
	{
		FWebJSParam ArgArray[7] = {FWebJSParam(Arg1), FWebJSParam(Arg2), FWebJSParam(Arg3), FWebJSParam(Arg4), FWebJSParam(Arg5), FWebJSParam(Arg6), FWebJSParam(Arg7)};
		Invoke(7, ArgArray);
	}
#endif

private:

	void Invoke(int32 ArgCount, FWebJSParam Arguments[]) const;

	TWeakPtr<FWebJSScripting> ScriptingPtr;
	FGuid FunctionId;
};