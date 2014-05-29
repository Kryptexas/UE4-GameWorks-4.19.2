// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

// Intended categories:
//	Log - This happened. What gameplay programmers may care about to debug
//	Verbose - This is why this happened. What you may turn on to debug the skill system code.
//  VeryVerbose - This is what didn't happen, and why. Extreme printf debugging
//


SKILLSYSTEM_API DECLARE_LOG_CATEGORY_EXTERN(LogSkillSystem, Warning, All);

#define SKILL_LOG(Verbosity, Format, ...) \
{ \
	FString Str = SkillSystemLog::Log(ELogVerbosity::Verbosity, FString::Printf(Format, ##__VA_ARGS__)); \
	UE_LOG(LogSkillSystem, Verbosity, TEXT("%s"), *Str); \
}

#define SKILL_LOG_TOKENPASTE_INNER(x,y) x##y
#define SKILL_LOG_TOKENPASTE(x,y) SKILL_LOG_TOKENPASTE_INNER(x,y)
#define SKILL_LOG_SCOPE( Format, ... ) SkillSystemLogScope SKILL_LOG_TOKENPASTE(LogScope,__LINE__)( FString::Printf(Format, ##__VA_ARGS__));

struct SkillSystemLogScope
{
	SkillSystemLogScope()
	{
		Init();
	}

	SkillSystemLogScope(FString InScopeName) :
		ScopeName(InScopeName)
	{
		Init();
	}

	~SkillSystemLogScope();
	
	FString ScopeName;
	bool PrintedInThisScope;

private:

	void Init();
};

class SkillSystemLog
{
public:

	SkillSystemLog()
	{
		Indent = 0;
		NeedNewLine = false;
	}

	static FString Log(ELogVerbosity::Type, FString Str);

	static void PushScope(SkillSystemLogScope* Scope);

	static void PopScope();	

	int32	Indent;
	TArray<SkillSystemLogScope*>	ScopeStack;
	bool NeedNewLine;
};