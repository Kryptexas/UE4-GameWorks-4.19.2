// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ScriptPluginPrivatePCH.h"
#include "LuaIntegration.h"

#if WITH_LUA

// Forward declaration - definition is in generated inl function.
void LuaRegisterExportedClasses(lua_State* InScriptContext);

void FLuaUtils::RegisterLibrary(lua_State* LuaState, const luaL_Reg Lib[], const ANSICHAR* LibName)
{
	lua_getglobal(LuaState, LibName);
	if (lua_isnil(LuaState, -1))
	{
		lua_pop(LuaState, 1);
		lua_newtable(LuaState);
	}
	luaL_setfuncs(LuaState, Lib, 0);
	lua_setglobal(LuaState, LibName);
}

bool FLuaUtils::CallFunction(lua_State* LuaState, const ANSICHAR* FunctionName)
{
	if (FunctionName != NULL)
	{
		lua_getglobal(LuaState, FunctionName);
	}

	bool bResult = true;
	const int NumArgs = 0;
	const int NumResults = 0;
	if (lua_pcall(LuaState, NumArgs, NumResults, 0) != 0)
	{
		UE_LOG(LogScriptPlugin, Warning, TEXT("Cannot call Lua function %s: %s"), ANSI_TO_TCHAR(FunctionName), ANSI_TO_TCHAR(lua_tostring(LuaState, -1)));
		bResult = false;
	}
	return bResult;
}

bool FLuaUtils::DoesFunctionExist(lua_State* LuaState, const char* FunctionName)
{
	bool bExists = true;
	lua_getglobal(LuaState, FunctionName);
	if (!lua_isfunction(LuaState, lua_gettop(LuaState)))
	{
		bExists = false;
	}
	lua_pop(LuaState, 1);
	return bExists;
}

struct FLuaUObject
{
	/**
	 * FindObject
	 *
	 * @param Class
	 * @param Package
	 * @param Name
	 * @param bExactClass
	 */
	static int32 LuaStaticFindObject(lua_State* LuaState)
	{
		UClass* Class = (UClass*)lua_touserdata(LuaState, 1);
		UObject* Package = (UObject*)lua_touserdata(LuaState, 2);
		FString Name = ANSI_TO_TCHAR(luaL_checkstring(LuaState, 3));
		bool bExactClass = !!(lua_toboolean(LuaState, 4));
		UObject* Result = StaticFindObject(Class, Package, *Name, bExactClass);
		lua_pushlightuserdata(LuaState, Result);
		return 1;
	}

	/**
	* FindObjectFast
	*
	* @param Class
	* @param Package
	* @param Name
	* @param bExactClass
	*/
	static int32 LuaStaticFindObjectFast(lua_State* LuaState)
	{
		UClass* Class = (UClass*)lua_touserdata(LuaState, 1);
		UObject* Package = (UObject*)lua_touserdata(LuaState, 2);
		FString Name = ANSI_TO_TCHAR(luaL_checkstring(LuaState, 3));
		bool bExactClass = !!(lua_toboolean(LuaState, 4));
		UObject* Result = StaticFindObjectFast(Class, Package, *Name, bExactClass);
		lua_pushlightuserdata(LuaState, Result);
		return 1;
	}

	/**
	* LoadObject
	*
	* @param Class
	* @param LongPackageName
	*/
	static int32 LuaStaticLoadObject(lua_State* LuaState)
	{
		UClass* Class = (UClass*)lua_touserdata(LuaState, 1);
		FString LongPackageName = ANSI_TO_TCHAR(luaL_checkstring(LuaState, 2));
		UObject* Result = StaticLoadObject(Class, NULL, *LongPackageName);
		lua_pushlightuserdata(LuaState, Result);
		return 1;
	}

	/**
	* IsA
	*
	* @param Object
	* @param Class
	*/
	static int32 LuaIsA(lua_State* LuaState)
	{
		UObject* Obj = (UObject*)lua_touserdata(LuaState, 1);
		UClass* Class = (UClass*)lua_touserdata(LuaState, 2);
		bool bResult = Obj->IsA(Class);
		lua_pushboolean(LuaState, bResult);
		return 1;
	}

	// Library
	static luaL_Reg UObject_Lib[];
};

luaL_Reg FLuaUObject::UObject_Lib[] =
{
	{ "IsA", FLuaUObject::LuaIsA },
	{ "FindObject", FLuaUObject::LuaStaticFindObject },
	{ "FindObjectFast", FLuaUObject::LuaStaticFindObjectFast },
	{ "LoadObject", FLuaUObject::LuaStaticLoadObject },
	{ NULL, NULL }
};

FVector FLuaVector::Get(lua_State* LuaState, int ParamIndex)
{
	FVector Result;

	luaL_checktype(LuaState, ParamIndex, LUA_TTABLE);

	lua_rawgeti(LuaState, ParamIndex, 1); 
	int Top = lua_gettop(LuaState);
	Result.X = (float)lua_tonumber(LuaState, Top);
	lua_pop(LuaState, 1);

	lua_rawgeti(LuaState, ParamIndex, 2);
	Top = lua_gettop(LuaState);
	Result.Y = (float)lua_tonumber(LuaState, Top);
	lua_pop(LuaState, 1);

	lua_rawgeti(LuaState, ParamIndex, 3);
	Top = lua_gettop(LuaState);
	Result.Z = (float)lua_tonumber(LuaState, Top);
	lua_pop(LuaState, 1);

	return Result;
}

void FLuaVector::Return(lua_State *LuaState, const FVector& Value)
{
	lua_newtable(LuaState);

	lua_pushnumber(LuaState, Value.X);
	lua_rawseti(LuaState, -2, 1);

	lua_pushnumber(LuaState, Value.Y);
	lua_rawseti(LuaState, -2, 2);

	lua_pushnumber(LuaState, Value.Z);
	lua_rawseti(LuaState, -2, 3);
}

FVector4 FLuaVector4::Get(lua_State *LuaState, int ParamIndex)
{
	FVector4 Result;

	luaL_checktype(LuaState, ParamIndex, LUA_TTABLE);

	lua_rawgeti(LuaState, ParamIndex, 1); 
	int Top = lua_gettop(LuaState);
	Result.X = (float)lua_tonumber(LuaState, Top);
	lua_pop(LuaState, 1);

	lua_rawgeti(LuaState, ParamIndex, 2);
	Top = lua_gettop(LuaState);
	Result.Y = (float)lua_tonumber(LuaState, Top);
	lua_pop(LuaState, 1);

	lua_rawgeti(LuaState, ParamIndex, 3);
	Top = lua_gettop(LuaState);
	Result.Z = (float)lua_tonumber(LuaState, Top);
	lua_pop(LuaState, 1);

	lua_rawgeti(LuaState, ParamIndex, 4);
	Top = lua_gettop(LuaState);
	Result.W = (float)lua_tonumber(LuaState, Top);
	lua_pop(LuaState, 1);

	return Result;
}

void FLuaVector4::Return(lua_State* LuaState, const FVector4& Value)
{
	lua_newtable(LuaState);

	lua_pushnumber(LuaState, Value.X);
	lua_rawseti(LuaState, -2, 1);

	lua_pushnumber(LuaState, Value.Y);
	lua_rawseti(LuaState, -2, 2);

	lua_pushnumber(LuaState, Value.Z);
	lua_rawseti(LuaState, -2, 3);

	lua_pushnumber(LuaState, Value.W);
	lua_rawseti(LuaState, -2, 4);
}

FQuat FLuaQuat::Get(lua_State* LuaState, int ParamIndex)
{
	FQuat Result;

	luaL_checktype(LuaState, ParamIndex, LUA_TTABLE);

	lua_rawgeti(LuaState, ParamIndex, 1); 
	int Top = lua_gettop(LuaState);
	Result.X = (float)lua_tonumber(LuaState, Top);
	lua_pop(LuaState, 1);

	lua_rawgeti(LuaState, ParamIndex, 2);
	Top = lua_gettop(LuaState);
	Result.Y = (float)lua_tonumber(LuaState, Top);
	lua_pop(LuaState, 1);

	lua_rawgeti(LuaState, ParamIndex, 3);
	Top = lua_gettop(LuaState);
	Result.Z = (float)lua_tonumber(LuaState, Top);
	lua_pop(LuaState, 1);

	lua_rawgeti(LuaState, ParamIndex, 4);
	Top = lua_gettop(LuaState);
	Result.W = (float)lua_tonumber(LuaState, Top);
	lua_pop(LuaState, 1);

	return Result;
}

void FLuaQuat::Return(lua_State* LuaState, const FQuat& Quat)
{
	lua_newtable(LuaState);

	lua_pushnumber(LuaState, Quat.X);
	lua_rawseti(LuaState, -2, 1);

	lua_pushnumber(LuaState, Quat.Y);
	lua_rawseti(LuaState, -2, 2);

	lua_pushnumber(LuaState, Quat.Z);
	lua_rawseti(LuaState, -2, 3);

	lua_pushnumber(LuaState, Quat.W);
	lua_rawseti(LuaState, -2, 4);
}

template <typename T, typename R>
static R GetVectorFromTable(lua_State *LuaState, int Index, const ANSICHAR* Key)
{
	lua_getfield(LuaState, Index, Key);
	int Top = lua_gettop(LuaState);
	if (lua_isnil(LuaState, Top))
	{
		lua_pop(LuaState, 1);
		return R();
	}

	Top = lua_gettop(LuaState);
	return T::Get(LuaState, Top);
}

FTransform FLuaTransform::Get(lua_State *LuaState, int ParamIndex)
{
	luaL_checktype(LuaState, ParamIndex, LUA_TTABLE);

	FQuat Rotation = GetVectorFromTable<FLuaQuat, FQuat>(LuaState, ParamIndex, "Rotation");
	FVector Translation = GetVectorFromTable<FLuaVector, FVector>(LuaState, ParamIndex, "Translation");
	FVector Scale3D = GetVectorFromTable<FLuaVector, FVector>(LuaState, ParamIndex, "Scale3D");

	return FTransform(Rotation, Translation, Scale3D);
}

void FLuaTransform::Return(lua_State *LuaState, const FTransform& Value)
{
	lua_newtable(LuaState);
	lua_pushstring(LuaState, "Rotation");
	FLuaQuat::Return(LuaState, Value.GetRotation());
	lua_settable(LuaState, -3);

	lua_pushstring(LuaState, "Translation");
	FLuaVector::Return(LuaState, Value.GetTranslation());
	lua_settable(LuaState, -3);

	lua_pushstring(LuaState, "Scale3D");
	FLuaVector::Return(LuaState, Value.GetScale3D());
	lua_settable(LuaState, -3);
}

static int32 LuaTransformNew(lua_State* LuaState)
{
	FTransform Result = FTransform::Identity;
	FLuaTransform::Return(LuaState, Result);
	return 1;
}

luaL_Reg FTransform_Lib[] =
{
	{ "New", LuaTransformNew },
	{ NULL, NULL }
};

static int32 LuaUnrealLog(lua_State* LuaState)
{
	int ArgCount = lua_gettop(LuaState);
	FString Message;

	for (int ArgIndex = 1; ArgIndex <= ArgCount; ++ArgIndex)
	{
		if (lua_isstring(LuaState, ArgIndex))
		{
			Message += ANSI_TO_TCHAR(lua_tostring(LuaState, ArgIndex));
		}
	}
	UE_LOG(LogScriptPlugin, Log, TEXT("%s"), *Message);

	return 0;
}

static void LuaOverridePrint(lua_State* LuaState)
{
	static const luaL_Reg PrintOverride[] =
	{
		{ "print", LuaUnrealLog },
		{ NULL, NULL }
	};

	lua_getglobal(LuaState, "_G");
	luaL_setfuncs(LuaState, PrintOverride, 0);
	lua_pop(LuaState, 1);
}

static void* LuaAlloc(void *Ud, void *Ptr, size_t OldSize, size_t NewSize) 
{
	if (NewSize != 0)
	{
		return FMemory::Realloc(Ptr, NewSize);
	}
	else
	{
		FMemory::Free(Ptr);
		return NULL;
	}
}

static int32 LuaPanic(lua_State *lua_State)
{
	UE_LOG(LogScriptPlugin, Error, TEXT("PANIC: unprotected error in call to Lua API(%s)"), ANSI_TO_TCHAR(lua_tostring(lua_State, -1)));
	return 0;
}


static lua_State* LuaNewState() 
{
	lua_State* LuaState = lua_newstate(LuaAlloc, NULL);
	if (LuaState)
	{
		lua_atpanic(LuaState, &LuaPanic);
	}
	return LuaState;
}

static void LuaRegisterUnrealUtilities(lua_State* LuaState)
{
	LuaOverridePrint(LuaState);
	FLuaUtils::RegisterLibrary(LuaState, FLuaUObject::UObject_Lib, "UE");
	FLuaUtils::RegisterLibrary(LuaState, FTransform_Lib, "Transform");
}

FLuaContext::FLuaContext()
: bHasTick(false)
, bHasDestroy(false)
, LuaState(NULL)
{
};

bool FLuaContext::Initialize(UScriptComponent* Owner)
{
	bool bResult = false;
	LuaState = LuaNewState();
	luaL_openlibs(LuaState);
	LuaRegisterExportedClasses(LuaState);
	LuaRegisterUnrealUtilities(LuaState);

	if (luaL_loadstring(LuaState, TCHAR_TO_ANSI(*Owner->Script->SourceCode)) == 0)
	{
		// Set this pointer
		lua_pushlightuserdata(LuaState, Owner);
		lua_setglobal(LuaState, "this");

		bResult = FLuaUtils::CallFunction(LuaState, NULL);
		if (bResult)
		{
			bHasTick = FLuaUtils::DoesFunctionExist(LuaState, "Tick");
			bHasDestroy = FLuaUtils::DoesFunctionExist(LuaState, "Destroy");
		}
	}
	else
	{
		UE_LOG(LogScriptPlugin, Warning, TEXT("Cannot load Lua script %s for %s: %s"), *Owner->Script->GetName(), *Owner->GetPathName(), ANSI_TO_TCHAR(lua_tostring(LuaState, -1)));
	}
	return bResult;
}

void FLuaContext::Tick(float DeltaTime)
{
	check(LuaState && bHasTick);
	FLuaUtils::CallFunction(LuaState, "Tick");
}

void FLuaContext::Destroy()
{
	if (LuaState)
	{
		if (bHasDestroy)
		{
			FLuaUtils::CallFunction(LuaState, "Destroy");
		}

		lua_close(LuaState);
		LuaState = NULL;
	}
}

bool FLuaContext::CanTick()
{
	return bHasTick;
}


#endif // WITH_LUA
