// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#if WITH_LUA

extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

class FLuaContext : public FScriptContextBase
{
	bool bHasTick;
	bool bHasDestroy;
	bool bHasBeginPlay;
	lua_State* LuaState;
	
	void CloseLua();

public:

	FLuaContext();
	virtual ~FLuaContext();

	// FScriptContextBase interface
	virtual bool Initialize(const FString& Code, UObject* Owner) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Destroy() override;
	virtual bool CanTick() override;

	virtual bool CallFunction(const FString&  FunctionName) override;
	virtual bool SetFloatVariable(const FString&  GlobalVariable, float NewValue) override;
	virtual float GetFloatVariable(const FString&  GlobalVariable) override;
	virtual bool SetStringVariable(const FString&  GlobalVariable, const FString& NewValue) override;
	virtual FString GetStringVariable(const FString&  GlobalVariable) override;
	virtual void InvokeScriptFunction(FFrame& Stack, RESULT_DECL) override;

#if WITH_EDITOR
	virtual void GetScriptDefinedFields(TArray<FScriptField>& OutFields) override;
#endif
};

struct FLuaUtils
{
	static void RegisterLibrary(lua_State* LuaState, const luaL_Reg Lib[], const ANSICHAR* LibName);
	static bool CallFunction(lua_State* LuaState, const ANSICHAR* FunctionName);
	static bool DoesFunctionExist(lua_State* LuaState, const char* FunctionName);
};

struct FLuaVector2D
{
	static FVector2D Get(lua_State *LuaState, int ParamIndex);
	static void Return(lua_State *LuaState, const FVector2D& Vec);
};

struct FLuaVector
{
	static FVector Get(lua_State *LuaState, int ParamIndex);
	static void Return(lua_State *LuaState, const FVector& Vec);
};

struct FLuaVector4
{
	static FVector4 Get(lua_State *LuaState, int ParamIndex);
	static void Return(lua_State *LuaState, const FVector4& Vec);
};

struct FLuaQuat
{
	static FQuat Get(lua_State *LuaState, int ParamIndex);
	static void Return(lua_State *LuaState, const FQuat& Vec);
};

struct FLuaLinearColor
{
	static FLinearColor Get(lua_State *LuaState, int ParamIndex);
	static void Return(lua_State *LuaState, const FLinearColor& Vec);
};

struct FLuaTransform
{
	static FTransform Get(lua_State *LuaState, int ParamIndex);
	static void Return(lua_State *LuaState, const FTransform& Transform);
};

#endif // WITH_LUA
