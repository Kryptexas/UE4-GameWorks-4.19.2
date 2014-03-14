// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
ConsoleManager.cpp: console command handling
=============================================================================*/

#include "CorePrivate.h"
#include "ConsoleManager.h"
#include "ModuleManager.h"

DEFINE_LOG_CATEGORY(LogConsoleResponse);
DEFINE_LOG_CATEGORY_STATIC(LogConsoleManager, Log, All);

static inline bool IsWhiteSpace(TCHAR Value) { return Value == TCHAR(' '); }

class FConsoleVariableBase : public IConsoleVariable
{
public:
	/*
	 * Constructor
	 * @param InHelp must not be 0, must not be empty
	 */
	FConsoleVariableBase(const TCHAR* InHelp, EConsoleVariableFlags InFlags)
		:Help(InHelp), Flags(InFlags), bWarnedAboutThreadSafety(false)
	{
		check(InHelp);
		//check(*Help != 0); for now disabled as there callstack when we crash early during engine init
	}

	// interface IConsoleVariable -----------------------------------

	virtual const TCHAR* GetHelp() const
	{
		return *Help;
	}
	virtual void SetHelp(const TCHAR* Value)
	{
		check(Value);
		//check(*Value != 0);

		Help = Value;
	}
	virtual EConsoleVariableFlags GetFlags() const
	{
		return Flags;
	}
	virtual void SetFlags(const EConsoleVariableFlags Value)
	{
		Flags = Value;
	}

	virtual class IConsoleVariable* AsVariable()
	{
		return this;
	}

	virtual void SetOnChangedCallback(const FConsoleVariableDelegate& Callback) { OnChangedCallback = Callback; }

	// ------

	void OnChanged()
	{
		// only change on main thread

		Flags = (EConsoleVariableFlags)((uint32)Flags | ECVF_Changed);

		OnChangedCallback.ExecuteIfBound(this);
	}

	
protected: // -----------------------------------------

	// not using TCHAR* to allow chars support reloading of modules (otherwise we would keep a pointer into the module)
	FString Help;
	//
	EConsoleVariableFlags Flags;
	/** User function to call when the console variable is changed */
	FConsoleVariableDelegate OnChangedCallback;

	/** True if this console variable has been used on the wrong thread and we have warned about it. */
	mutable bool bWarnedAboutThreadSafety;

	// @return 0:main thread, 1: render thread, later more
	uint32 GetShadowIndex() const
	{
		if((uint32)Flags & ECVF_RenderThreadSafe)
		{
			return IsInGameThread() ? 0 : 1;
		}
		else
		{
			FConsoleManager& ConsoleManager = (FConsoleManager&)IConsoleManager::Get();
			if(ConsoleManager.IsThreadPropagationThread() && FPlatformProcess::SupportsMultithreading())
			{
				if(!bWarnedAboutThreadSafety)
				{
					FString CVarName = ConsoleManager.FindConsoleObjectName(this);
					UE_LOG(LogConsoleManager, Warning,
						TEXT("Console variable '%s' used in the render thread. Rendering artifacts could happen. Use ECVF_RenderThreadSafe or don't use in render thread."),
						CVarName.IsEmpty() ? TEXT("unknown?") : *CVarName
						);
					bWarnedAboutThreadSafety = true;
				}
			}
			// other threads are not handled at the moment (e.g. sound)
		}
	
		return 0;
	}
};



class FConsoleCommandBase : public IConsoleCommand
{
public:
	/*
	 * Constructor
	 * @param InHelp must not be 0, must not be empty
	 */
	FConsoleCommandBase(const TCHAR* InHelp, EConsoleVariableFlags InFlags)
		: Help(InHelp), Flags(InFlags)
	{
		check(InHelp);
		//check(*Help != 0); for now disabled as there callstack when we crash early during engine init
	}

	// interface IConsoleVariable -----------------------------------

	virtual const TCHAR* GetHelp() const
	{
		return *Help;
	}
	virtual void SetHelp(const TCHAR* InValue)
	{
		check(InValue);
		check(*InValue != 0);

		Help = InValue;
	}
	virtual EConsoleVariableFlags GetFlags() const
	{
		return Flags;
	}
	virtual void SetFlags(const EConsoleVariableFlags Value)
	{
		Flags = Value;
	}

	virtual struct IConsoleCommand* AsCommand()
	{
		return this;
	}

private: // -----------------------------------------

	// not using TCHAR* to allow chars support reloading of modules (otherwise we would keep a pointer into the module)
	FString Help;

	EConsoleVariableFlags Flags;
};

template <class T>
void OnCVarChange(T& Dst, const T& Src, EConsoleVariableFlags Flags)
{
	FConsoleManager& ConsoleManager = (FConsoleManager&)IConsoleManager::Get();

	if(IsInGameThread())
	{
		if((Flags & ECVF_RenderThreadSafe) && ConsoleManager.GetThreadPropagationCallback())
		{
			// defer the change to be in order with other rendering commands
			ConsoleManager.GetThreadPropagationCallback()->OnCVarChange(Dst, Src);
		}
		else
		{
			// propagate the change right away
			Dst = Src;
		}
	}
	else
	{
		// CVar Changes can only be initiated from the main thread
		check(0);
	}

	ConsoleManager.OnCVarChanged();
}

// T: int32, float, FString
template <class T>
class FConsoleVariable : public FConsoleVariableBase
{
public:
	FConsoleVariable(T DefaultValue, const TCHAR* Help, EConsoleVariableFlags Flags) 
		: FConsoleVariableBase(Help, Flags), Data(DefaultValue)
	{
	}

	// interface IConsoleVariable -----------------------------------

	virtual void Release()
	{
		delete this; 
	} 
	virtual void Set(const TCHAR* InValue)
	{
		TTypeFromString<T>::FromString(Data.ShadowedValue[0], InValue);
		OnChanged();
	}

	virtual int32 GetInt() const;
	virtual float GetFloat() const;
	virtual FString GetString() const;
	virtual class TConsoleVariableData<int32>* AsVariableInt() { return 0; }
	virtual class TConsoleVariableData<float>* AsVariableFloat() { return 0; }

private: // ----------------------------------------------------

	TConsoleVariableData<T> Data;

	const T &Value() const
	{
		// remove const
		FConsoleVariable<T>* This = (FConsoleVariable<T>*)this;
		return This->Data.GetReferenceOnAnyThread();
	}

	void OnChanged()
	{
		// propagate from main thread to render thread
		OnCVarChange(Data.ShadowedValue[1], Data.ShadowedValue[0], Flags);
		FConsoleVariableBase::OnChanged();
	}
};


// specialization for int32

template<> int32 FConsoleVariable<int32>::GetInt() const
{
	return Value();
}
template<> float FConsoleVariable<int32>::GetFloat() const
{
	return (float)Value();
}
template<> FString FConsoleVariable<int32>::GetString() const
{
	return FString::Printf(TEXT("%d"), Value());
}
template<> TConsoleVariableData<int32>* FConsoleVariable<int32>::AsVariableInt()
{
	return &Data; 
}

// specialization for float

template<> int32 FConsoleVariable<float>::GetInt() const
{
	return (int32)Value();
}
template<> float FConsoleVariable<float>::GetFloat() const
{
	return Value();
}
template<> FString FConsoleVariable<float>::GetString() const
{
	return FString::Printf(TEXT("%g"), Value());
}
template<> TConsoleVariableData<float>* FConsoleVariable<float>::AsVariableFloat()
{
	return &Data; 
}

// specialization for FString

template<> void FConsoleVariable<FString>::Set(const TCHAR* InValue)
{
	Data.ShadowedValue[0] = InValue;
	OnChanged();
}
template<> int32 FConsoleVariable<FString>::GetInt() const
{
	return FCString::Atoi(*Value());
}
template<> float FConsoleVariable<FString>::GetFloat() const
{
	return FCString::Atof(*Value());
}
template<> FString FConsoleVariable<FString>::GetString() const
{
	return Value();
}

// ----

// T: int32, float
template <class T>
class FConsoleVariableRef : public FConsoleVariableBase
{
public:
	FConsoleVariableRef(T& InRefValue, const TCHAR* Help, EConsoleVariableFlags Flags) 
		: FConsoleVariableBase(Help, Flags), RefValue(InRefValue), MainValue(InRefValue)
	{
	}

	// interface IConsoleVariable -----------------------------------

	virtual void Release()
	{
		delete this; 
	} 
	virtual void Set(const TCHAR* InValue)
	{
		TTypeFromString<T>::FromString(MainValue, InValue);
		OnChanged();
	}
	virtual int32 GetInt() const
	{
		return (int32)MainValue;
	}
	virtual float GetFloat() const
	{
		return (float)MainValue;
	}
	virtual FString GetString() const
	{
		return TTypeToString<T>::ToString(MainValue);
	}

private: // ----------------------------------------------------

	// reference the the value (should not be changed from outside), if ECVF_RenderThreadSafe this is the render thread version, otherwise same as MainValue
	T& RefValue;
	//  main thread version 
	T MainValue;
	
	const T &Value() const
	{
		uint32 Index = GetShadowIndex();
		checkSlow(Index < 2);
		return (Index == 0) ? MainValue : RefValue;
	}

	void OnChanged()
	{
		// propagate from main thread to render thread or to reference
		OnCVarChange(RefValue, MainValue, Flags);
		FConsoleVariableBase::OnChanged();
	}
};

// specialization for float

template <>
FString FConsoleVariableRef<float>::GetString() const
{
	// otherwise we get 2.1f would become "2.100000"
	return FString::SanitizeFloat(RefValue);
}

// ----

class FConsoleVariableBitRef : public FConsoleVariableBase
{
public:
	FConsoleVariableBitRef(const TCHAR* FlagName, uint32 InBitNumber, uint8* InForce0MaskPtr, uint8* InForce1MaskPtr, const TCHAR* Help, EConsoleVariableFlags Flags) 
		: FConsoleVariableBase(Help, Flags), Force0MaskPtr(InForce0MaskPtr), Force1MaskPtr(InForce1MaskPtr), BitNumber(InBitNumber)
	{
	}

	// interface IConsoleVariable -----------------------------------

	virtual void Release()
	{
		delete this; 
	} 
	virtual void Set(const TCHAR* InValue)
	{
		int32 Value = FCString::Atoi(InValue);

		check(IsInGameThread());

		FMath::SetBoolInBitField(Force0MaskPtr, BitNumber, Value == 0);
		FMath::SetBoolInBitField(Force1MaskPtr, BitNumber, Value == 1);
	}
	virtual int32 GetInt() const
	{
		// we apply the bitmask on game thread (showflags) so we don't have to do any special thread handling
		check(IsInGameThread());

		bool Force0 = FMath::ExtractBoolFromBitfield(Force0MaskPtr, BitNumber);
		bool Force1 = FMath::ExtractBoolFromBitfield(Force1MaskPtr, BitNumber);
		
		if(!Force0 && !Force1)
		{
			// not enforced to be 0 or 1
			return 2;
		}

		return Force1 ? 1 : 0;
	}
	virtual float GetFloat() const
	{
		return (float)GetInt();
	}
	virtual FString GetString() const
	{
		return FString::Printf(TEXT("%d"), GetInt());
	}

private: // ----------------------------------------------------

	uint8* Force0MaskPtr;
	uint8* Force1MaskPtr;
	uint32 BitNumber;
};

IConsoleVariable* FConsoleManager::RegisterConsoleVariableBitRef(const TCHAR* CVarName, const TCHAR* FlagName, uint32 BitNumber, uint8* Force0MaskPtr, uint8* Force1MaskPtr, const TCHAR* Help, uint32 Flags)
{
	return AddConsoleObject(CVarName, new FConsoleVariableBitRef(FlagName, BitNumber, Force0MaskPtr, Force1MaskPtr, Help, (EConsoleVariableFlags)Flags))->AsVariable();
}

void FConsoleManager::CallAllConsoleVariableSinks()
{
	if(bCallAllConsoleVariableSinks)
	{
		for(uint32 i = 0; i < (uint32)ConsoleVariableChangeSinks.Num(); ++i)
		{
			ConsoleVariableChangeSinks[i].ExecuteIfBound();
		}

		bCallAllConsoleVariableSinks = false;
	}
}

void FConsoleManager::RegisterConsoleVariableSink(const FConsoleCommandDelegate& Command)
{
	ConsoleVariableChangeSinks.Add(Command);
}

void FConsoleManager::UnregisterConsoleVariableSink(const FConsoleCommandDelegate& Command)
{
	ConsoleVariableChangeSinks.Remove(Command);
}

class FConsoleCommand : public FConsoleCommandBase
{

public:
	FConsoleCommand( const FConsoleCommandDelegate& InitDelegate, const TCHAR* InitHelp, const EConsoleVariableFlags InitFlags )
		: FConsoleCommandBase( InitHelp, InitFlags ),
		  Delegate( InitDelegate )
	{
	}

	// interface IConsoleCommand -----------------------------------

	virtual void Release() OVERRIDE
	{
		delete this; 
	} 

	virtual bool Execute( const TArray< FString > Args, UWorld* InWorld, FOutputDevice& OutputDevice ) OVERRIDE
	{
		// NOTE: Args are ignored for FConsoleCommand.  Use FConsoleCommandWithArgs if you need parameters.
		return Delegate.ExecuteIfBound();
	}

private:

	/** User function to call when the console command is executed */
	FConsoleCommandDelegate Delegate;

};


class FConsoleCommandWithArgs : public FConsoleCommandBase
{

public:
	FConsoleCommandWithArgs( const FConsoleCommandWithArgsDelegate& InitDelegate, const TCHAR* InitHelp, const EConsoleVariableFlags InitFlags )
		: FConsoleCommandBase( InitHelp, InitFlags ),
		  Delegate( InitDelegate )
	{
	}

	// interface IConsoleCommand -----------------------------------

	virtual void Release() OVERRIDE
	{
		delete this; 
	} 

	virtual bool Execute( const TArray< FString > Args, UWorld* InWorld, FOutputDevice& OutputDevice ) OVERRIDE
	{
		return Delegate.ExecuteIfBound( Args );
	}

private:

	/** User function to call when the console command is executed */
	FConsoleCommandWithArgsDelegate Delegate;
};

/* Console command that can be given a world parameter. */
class FConsoleCommandWithWorld : public FConsoleCommandBase
{

public:
	FConsoleCommandWithWorld( const FConsoleCommandWithWorldDelegate& InitDelegate, const TCHAR* InitHelp, const EConsoleVariableFlags InitFlags )
		: FConsoleCommandBase( InitHelp, InitFlags ),
		Delegate( InitDelegate )
	{
	}

	// interface IConsoleCommand -----------------------------------

	virtual void Release() OVERRIDE
	{
		delete this; 
	} 

	virtual bool Execute( const TArray< FString > Args, UWorld* InWorld, FOutputDevice& OutputDevice ) OVERRIDE
	{
		return Delegate.ExecuteIfBound( InWorld );
	}

private:

	/** User function to call when the console command is executed */
	FConsoleCommandWithWorldDelegate Delegate;
};

/* Console command that can be given a world parameter and args. */
class FConsoleCommandWithWorldAndArgs : public FConsoleCommandBase
{

public:
	FConsoleCommandWithWorldAndArgs( const FConsoleCommandWithWorldAndArgsDelegate& InitDelegate, const TCHAR* InitHelp, const EConsoleVariableFlags InitFlags )
		: FConsoleCommandBase( InitHelp, InitFlags ),
		Delegate( InitDelegate )
	{
	}

	// interface IConsoleCommand -----------------------------------

	virtual void Release() OVERRIDE
	{
		delete this; 
	} 

	virtual bool Execute( const TArray< FString > Args, UWorld* InWorld, FOutputDevice& OutputDevice ) OVERRIDE
	{
		return Delegate.ExecuteIfBound( Args, InWorld );
	}

private:

	/** User function to call when the console command is executed */
	FConsoleCommandWithWorldAndArgsDelegate Delegate;
};

/* Console command that can be given an output device. */
class FConsoleCommandWithOutputDevice : public FConsoleCommandBase
{

public:
	FConsoleCommandWithOutputDevice( const FConsoleCommandWithOutputDeviceDelegate& InitDelegate, const TCHAR* InitHelp, const EConsoleVariableFlags InitFlags )
		: FConsoleCommandBase( InitHelp, InitFlags ),
		Delegate( InitDelegate )
	{
	}

	// interface IConsoleCommand -----------------------------------

	virtual void Release() OVERRIDE
	{
		delete this; 
	} 

	virtual bool Execute( const TArray< FString > Args, UWorld* InWorld, FOutputDevice& OutputDevice ) OVERRIDE
	{
		return Delegate.ExecuteIfBound( OutputDevice );
	}

private:

	/** User function to call when the console command is executed */
	FConsoleCommandWithOutputDeviceDelegate Delegate;
};

// only needed for auto completion of Exec commands
class FConsoleCommandExec : public FConsoleCommandBase
{

public:
	FConsoleCommandExec( const TCHAR* InitHelp, const EConsoleVariableFlags InitFlags )
		: FConsoleCommandBase( InitHelp, InitFlags )
	{
	}

	// interface IConsoleCommand -----------------------------------

	virtual void Release() OVERRIDE
	{
		delete this; 
	} 

	virtual bool Execute( const TArray< FString > Args, UWorld* InCmdWorld, FOutputDevice& OutputDevice ) OVERRIDE
	{
		return false;
	}
};

IConsoleVariable* FConsoleManager::RegisterConsoleVariable(const TCHAR* Name, int32 DefaultValue, const TCHAR* Help, uint32 Flags)
{
	return AddConsoleObject(Name, new FConsoleVariable<int32>(DefaultValue, Help, (EConsoleVariableFlags)Flags))->AsVariable();
}

IConsoleVariable* FConsoleManager::RegisterConsoleVariable(const TCHAR* Name, float DefaultValue, const TCHAR* Help, uint32 Flags)
{
	return AddConsoleObject(Name, new FConsoleVariable<float>(DefaultValue, Help, (EConsoleVariableFlags)Flags))->AsVariable();
}

IConsoleVariable* FConsoleManager::RegisterConsoleVariable(const TCHAR* Name, const TCHAR *DefaultValue, const TCHAR* Help, uint32 Flags)
{
	// not supported
	check((Flags & (uint32)ECVF_RenderThreadSafe) == 0);
	return AddConsoleObject(Name, new FConsoleVariable<FString>(DefaultValue, Help, (EConsoleVariableFlags)Flags))->AsVariable();
}

IConsoleVariable* FConsoleManager::RegisterConsoleVariableRef(const TCHAR* Name, int32& RefValue, const TCHAR* Help, uint32 Flags)
{
	return AddConsoleObject(Name, new FConsoleVariableRef<int32>(RefValue, Help, (EConsoleVariableFlags)Flags))->AsVariable();
}

IConsoleVariable* FConsoleManager::RegisterConsoleVariableRef(const TCHAR* Name, float& RefValue, const TCHAR* Help, uint32 Flags)
{
	return AddConsoleObject(Name, new FConsoleVariableRef<float>(RefValue, Help, (EConsoleVariableFlags)Flags))->AsVariable();
}

IConsoleCommand* FConsoleManager::RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandDelegate& Command, uint32 Flags)
{
	return AddConsoleObject(Name, new FConsoleCommand(Command, Help, (EConsoleVariableFlags)Flags))->AsCommand();
}

IConsoleCommand* FConsoleManager::RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, uint32 Flags)
{
	return AddConsoleObject(Name, new FConsoleCommandExec(Help, (EConsoleVariableFlags)Flags))->AsCommand();
}

IConsoleCommand* FConsoleManager::RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithArgsDelegate& Command, uint32 Flags)
{
	return AddConsoleObject(Name, new FConsoleCommandWithArgs(Command, Help, (EConsoleVariableFlags)Flags))->AsCommand();
}

IConsoleCommand* FConsoleManager::RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldDelegate& Command, uint32 Flags)
{
	return AddConsoleObject(Name, new FConsoleCommandWithWorld(Command, Help, (EConsoleVariableFlags)Flags))->AsCommand();
}

IConsoleCommand* FConsoleManager::RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldAndArgsDelegate& Command, uint32 Flags)
{
	return AddConsoleObject(Name, new FConsoleCommandWithWorldAndArgs(Command, Help, (EConsoleVariableFlags)Flags))->AsCommand();
}

IConsoleCommand* FConsoleManager::RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithOutputDeviceDelegate& Command, uint32 Flags)
{
	return AddConsoleObject(Name, new FConsoleCommandWithOutputDevice(Command, Help, (EConsoleVariableFlags)Flags))->AsCommand();
}


IConsoleVariable* FConsoleManager::FindConsoleVariable(const TCHAR* Name) const
{
	IConsoleObject* Obj = FindConsoleObject(Name);

	if(Obj)
	{
		if(Obj->TestFlags(ECVF_Unregistered))
		{
			return 0;
		}

		return Obj->AsVariable();
	}

	return 0;
}

IConsoleObject* FConsoleManager::FindConsoleObject(const TCHAR* Name) const
{
	IConsoleObject* CVar = FindConsoleObjectUnfiltered(Name);

	if(CVar && CVar->TestFlags(ECVF_CreatedFromIni))
	{
		return 0;
	}

	return CVar;
}

IConsoleObject* FConsoleManager::FindConsoleObjectUnfiltered(const TCHAR* Name) const
{
	FScopeLock ScopeLock( &ConsoleObjectsSynchronizationObject );
	IConsoleObject* Var = ConsoleObjects.FindRef(Name);
	return Var;
}

void FConsoleManager::UnregisterConsoleObject(IConsoleObject* CVar, bool bKeepState)
{
	if(!CVar)
	{
		return;
	}

	// Slow search for console object
	const FString ObjName = FindConsoleObjectName( CVar );
	if( !ObjName.IsEmpty() )
	{
		UnregisterConsoleObject(*ObjName, bKeepState);
	}
}


void FConsoleManager::UnregisterConsoleObject(const TCHAR* Name, bool bKeepState)
{
	IConsoleObject* Object = FindConsoleObject(Name);

	if(Object)
	{
		FScopeLock ScopeLock(&ConsoleObjectsSynchronizationObject);

		IConsoleVariable* CVar = Object->AsVariable();

		if(CVar && bKeepState)
		{
			// to be able to restore the value if we just recompile a module
			CVar->SetFlags(ECVF_Unregistered);
		}
		else
		{
			ConsoleObjects.Remove(Name);
			Object->Release();
		}
	}
}

void FConsoleManager::ForEachConsoleObject(const FConsoleObjectVisitor& Visitor, const TCHAR* ThatStartsWith) const
{
	check(Visitor.IsBound());
	check(ThatStartsWith);

	//@caution, potential deadlock if the visitor tries to call back into the cvar system. Best not to do this, but we could capture and array of them, then release the lock, then dispatch the visitor.
	FScopeLock ScopeLock( &ConsoleObjectsSynchronizationObject );
	for(TMap<FString, IConsoleObject*>::TConstIterator PairIt(ConsoleObjects); PairIt; ++PairIt)
	{
		const FString& Name = PairIt.Key();
		IConsoleObject* CVar = PairIt.Value();

		if(MatchPartialName(*Name, ThatStartsWith))
		{
			Visitor.Execute(*Name, CVar);
		}
	}
}

bool FConsoleManager::ProcessUserConsoleInput(const TCHAR* InInput, FOutputDevice& Ar, UWorld* InWorld)
{
	check(InInput);

	const TCHAR* It = InInput;

	FString Param1 = GetTextSection(It);
	if(Param1.IsEmpty())
	{
		return false;
	}

	IConsoleObject* CObj = FindConsoleObject(*Param1);
	if(!CObj)
	{
		return false;
	}

#if (UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if(CObj->TestFlags(ECVF_Cheat))
	{
		return false;
	}
#endif // (UE_BUILD_SHIPPING || UE_BUILD_TEST)

	if(CObj->TestFlags(ECVF_Unregistered))
	{
		return false;
	}

	// fix case for nicer printout
	Param1 = FindConsoleObjectName(CObj);

	IConsoleCommand* CCmd = CObj->AsCommand();
	IConsoleVariable* CVar = CObj->AsVariable();

	if( CCmd )
	{
		// Process command
		// Build up argument list
		TArray< FString > Args;
		FString( It ).ParseIntoArrayWS( &Args );

		const bool bShowHelp = Args.Num() == 1 && Args[0] == TEXT("?") ? true : false;
		if( bShowHelp )
		{
			// get help
			Ar.Logf(TEXT("HELP for '%s':\n%s"), *Param1, CCmd->GetHelp());
		}
		else
		{
			// if a delegate was bound, we execute it and it should return true,
			// otherwise it was a Exec console command and this returns FASLE
			return CCmd->Execute( Args, InWorld, Ar );
		}
	}
	else if( CVar )
	{
		// Process variable

		if(*It == 0)
		{
			// get current state
			Ar.Logf(TEXT("%s = \"%s\""), *Param1, *CVar->GetString());
		}
		else
		{
			FString Param2 = FString(It).Trim().TrimTrailing();

			if(Param2.Len() >= 2)
			{
				if(Param2[0] == (TCHAR)'\"' && Param2[Param2.Len() - 1] == (TCHAR)'\"')
				{
					Param2 = Param2.Mid(1, Param2.Len() - 2);
				}
			}

			bool bReadOnly = CVar->TestFlags(ECVF_ReadOnly);

			if(Param2 == TEXT("?"))
			{
				// get help
				Ar.Logf(TEXT("HELP for '%s'%s:\n%s"), *Param1, bReadOnly ? TEXT("(ReadOnly)") : TEXT(""), CVar->GetHelp());
			}
			else
			{
				if(bReadOnly)
				{
					Ar.Logf(TEXT("Error: %s is read only!"), *Param1, *CVar->GetString());
				}
				else
				{
					// set value
					CVar->Set(*Param2);

					Ar.Logf(TEXT("%s = \"%s\""), *Param1, *CVar->GetString());

					CallAllConsoleVariableSinks();
				}
			}
		}
	}

	return true;
}

IConsoleObject* FConsoleManager::AddConsoleObject(const TCHAR* Name, IConsoleObject* Obj)
{
	check(Name);
	check(*Name != 0);
	check(Obj);

	FScopeLock ScopeLock( &ConsoleObjectsSynchronizationObject ); // we will lock on the entire add process
	IConsoleObject* ExistingObj = ConsoleObjects.FindRef(Name);

	if(Obj->GetFlags() & ECVF_Scalability)
	{
		// Scalability options cannot be cheats - otherwise using the options menu would mean cheating
		check(!(Obj->GetFlags() & ECVF_Cheat));
		// Scalability options cannot be read only - otherwise the options menu cannot work 
		check(!(Obj->GetFlags() & ECVF_ReadOnly));
	}

	if(Obj->GetFlags() & ECVF_RenderThreadSafe)
	{
		if(Obj->AsCommand())
		{
			// This feature is not supported for console commands
			check(0);
		}
	}

	if(ExistingObj)
	{
		// An existing console object was found that has the same name as the object being registered.
		// In most cases this is not allowed, but if there is a variable with the same name and is
		// in an 'unregistered' state or we're hot reloading dlls, we may be able to replace or update that variable.
#if !IS_MONOLITHIC
		const bool bCanUpdateOrReplaceObj = (ExistingObj->AsVariable()||ExistingObj->AsCommand()) && (GIsHotReload || ExistingObj->TestFlags(ECVF_Unregistered));
#else
		const bool bCanUpdateOrReplaceObj = ExistingObj->AsVariable() && ExistingObj->TestFlags(ECVF_Unregistered);
#endif
		if( !bCanUpdateOrReplaceObj )
		{
			// NOTE: The reason we don't assert here is because when using HotReload, locally-initialized static console variables will be
			//       re-registered, and it's desirable for the new variables to clobber the old ones.  Because this happen outside of the
			//       hot reload stack frame (GIsHotReload=true), we can't detect and handle only those cases, so we opt to warn instead.
			UE_LOG(LogConsoleManager, Warning, TEXT( "Console object named '%s' already exists but is being registered again, but we weren't expected it to be! (FConsoleManager::AddConsoleObject)"), Name );
		}

		IConsoleVariable* ExistingVar = ExistingObj->AsVariable();
		IConsoleCommand* ExistingCmd = ExistingObj->AsCommand();
		const int ExistingType = ExistingVar ? ExistingCmd ? 3 : 2 : 1;

		IConsoleVariable* Var = Obj->AsVariable();
		IConsoleCommand* Cmd = Obj->AsCommand();
		const int NewType = Var ? Cmd ? 3 : 2 : 1;

		// Validate that we have the same type for the existing console object and for the new one, because
		// never allowed to replace a command with a variable or vice-versa
		if( ExistingType != NewType )
		{
			UE_LOG(LogConsoleManager, Fatal, TEXT( "Console object named '%s' can't be replaced with the new one of different type!"), Name );
		}

		if( ExistingVar )
		{
			if(ExistingVar->TestFlags(ECVF_CreatedFromIni))
			{
				// The existing one came from the ini, get the value and destroy the existing one (no need to call sink because that will happen after all ini setting have been loaded)
				Var->Set(*ExistingVar->GetString());
				ExistingVar->Release();

				ConsoleObjects.Add(Name, Var);
				return Var;
			}
			else
			{
				// Copy data over from the new variable,
				// but keep the value from the existing one.
				// This way references to the old variable are preserved (no crash).
				// Changing the type of a variable however is not possible with this.
				ExistingVar->SetFlags(Var->GetFlags());
				ExistingVar->SetHelp(Var->GetHelp());

				// Name was already registered but got unregistered
				Var->Release();

				return ExistingVar;
			}
		}
		else if( ExistingCmd )
		{
			// Replace console command with the new one and release the existing one.
			// This should be safe, because we don't have FindConsoleVariable equivalent for commands.
			ConsoleObjects.Add( Name, Cmd );
			ExistingCmd->Release();

			return Cmd;
		}

		// Should never happen
		return nullptr;
	}
	else
	{
		ConsoleObjects.Add(Name, Obj);
		return Obj;
	}
}

FString FConsoleManager::GetTextSection(const TCHAR* &It)
{
	FString ret;

	while(*It)
	{
		if(IsWhiteSpace(*It))
		{
			break;
		}

		ret += *It++;
	}

	while(IsWhiteSpace(*It))
	{
		++It;
	}

	return ret;
}

FString FConsoleManager::FindConsoleObjectName(const IConsoleObject* InVar) const
{
	check(InVar);

	FScopeLock ScopeLock( &ConsoleObjectsSynchronizationObject );
	for(TMap<FString, IConsoleObject*>::TConstIterator PairIt(ConsoleObjects); PairIt; ++PairIt)
	{
		IConsoleObject* Var = PairIt.Value();

		if(Var == InVar)
		{
			const FString& Name = PairIt.Key();

			return Name;
		}
	}

	return FString();
}

bool FConsoleManager::MatchPartialName(const TCHAR* Stream, const TCHAR* Pattern)
{
	while(*Pattern)
	{
		if(FChar::ToLower(*Stream) != FChar::ToLower(*Pattern))
		{
			return false;
		}

		++Stream;
		++Pattern;
	}

	return true;
}

void CreateConsoleVariables();

IConsoleManager* IConsoleManager::Singleton;

void IConsoleManager::SetupSingleton()
{
	check(!Singleton);
	if (!Singleton)
	{
		Singleton = new FConsoleManager; // we will leak this
		CreateConsoleVariables();
	}
	check(Singleton);
}

void FConsoleManager::AddConsoleHistoryEntry(const TCHAR* Input)
{
	HistoryEntries.Add(FString(Input));
}

void FConsoleManager::GetConsoleHistory(TArray<FString>& Out) const
{
	Out = HistoryEntries;
}

bool FConsoleManager::IsNameRegistered(const TCHAR* Name) const
{
	FScopeLock ScopeLock(&ConsoleObjectsSynchronizationObject);
	return ConsoleObjects.Contains(Name);
}

void FConsoleManager::RegisterThreadPropagation(uint32 ThreadId, IConsoleThreadPropagation* InCallback)
{
	if(InCallback)
	{
		// at the moment we only support one thread besides the main thread
		check(!ThreadPropagationCallback);
	}
	else
	{
		// bad input parameters
		check(!ThreadId);
	}

	ThreadPropagationCallback = InCallback;
	ThreadPropagationThreadId = ThreadId;
}

IConsoleThreadPropagation* FConsoleManager::GetThreadPropagationCallback()
{
	return ThreadPropagationCallback;
}

bool FConsoleManager::IsThreadPropagationThread()
{
	return FPlatformTLS::GetCurrentThreadId() == ThreadPropagationThreadId;
}

void FConsoleManager::OnCVarChanged()
{
	bCallAllConsoleVariableSinks = true;
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
uint32 GConsoleManagerSinkTestCounter = 0;
void TestSinkCallback()
{
	++GConsoleManagerSinkTestCounter;
}
uint32 GConsoleVariableCallbackTestCounter = 0;
void TestConsoleVariableCallback(IConsoleVariable* Var)
{
	check(Var);

	float Value = Var->GetFloat();
	check(FMath::IsNearlyEqual(Value, 3.1f, KINDA_SMALL_NUMBER));

	++GConsoleVariableCallbackTestCounter;
}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

void FConsoleManager::Test()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	check(IsInGameThread());

	// at this time we don't want to test with threading
	check(GetThreadPropagationCallback() == 0);

	// init ---------

	GConsoleManagerSinkTestCounter = 0;
	IConsoleManager::Get().CallAllConsoleVariableSinks();

	// setup ---------

	RegisterConsoleVariableSink(FConsoleCommandDelegate::CreateStatic(&TestSinkCallback));

	// start tests ---------

	// no change should be triggered
	IConsoleManager::Get().CallAllConsoleVariableSinks();
	check(GConsoleManagerSinkTestCounter == 0);

	for(uint32 Pass = 0; Pass < 2; ++Pass)
	{
		// we only test the main thread side of ECVF_RenderThreadSafe so we expect the same results
		uint32 Flags = Pass ? ECVF_Default : ECVF_RenderThreadSafe;

		int32 RefD = 2;
		float RefE = 2.1f;

		IConsoleVariable* VarA = IConsoleManager::Get().RegisterConsoleVariable(TEXT("TestNameA"), 1, TEXT("TestHelpA"), Flags);
		IConsoleVariable* VarB = IConsoleManager::Get().RegisterConsoleVariable(TEXT("TestNameB"), 1.2f, TEXT("TestHelpB"), Flags);
		IConsoleVariable* VarD = IConsoleManager::Get().RegisterConsoleVariableRef(TEXT("TestNameD"), RefD, TEXT("TestHelpD"), Flags);
		IConsoleVariable* VarE = IConsoleManager::Get().RegisterConsoleVariableRef(TEXT("TestNameE"), RefE, TEXT("TestHelpE"), Flags);

		GConsoleVariableCallbackTestCounter = 0;
		VarB->SetOnChangedCallback(FConsoleVariableDelegate::CreateStatic(&TestConsoleVariableCallback));
		check(GConsoleVariableCallbackTestCounter == 0);

		// make sure the vars are there

		check(VarA == IConsoleManager::Get().FindConsoleVariable(TEXT("TestNameA")));
		check(VarB == IConsoleManager::Get().FindConsoleVariable(TEXT("TestNameB")));
		check(VarD == IConsoleManager::Get().FindConsoleVariable(TEXT("TestNameD")));
		check(VarE == IConsoleManager::Get().FindConsoleVariable(TEXT("TestNameE")));

		// test Get()

		check(VarA->GetInt() == 1);
		check(VarA->GetFloat() == 1);
		check(VarA->GetString() == FString(TEXT("1")));

		check(VarB->GetInt() == 1);
		check(FMath::IsNearlyEqual(VarB->GetFloat(), 1.2f, KINDA_SMALL_NUMBER));
		check(VarB->GetString() == FString(TEXT("1.2")));

		check(RefD == 2);
		check(VarD->GetInt() == 2);
		check(VarD->GetFloat() == (float)2);
		check(VarD->GetString() == FString(TEXT("2")));

		check(FMath::IsNearlyEqual(RefE, 2.1f, KINDA_SMALL_NUMBER));
		check(VarE->GetInt() == (int32)RefE);
		check(VarE->GetFloat() == RefE);
		check(VarE->GetString() == FString(TEXT("2.1")));

		// test changed

		check(!VarA->TestFlags(ECVF_Changed));
		check(!VarB->TestFlags(ECVF_Changed));
		check(!VarD->TestFlags(ECVF_Changed));
		check(!VarE->TestFlags(ECVF_Changed));

		// call Set(string)

		VarA->Set(TEXT("3.1"));
		VarB->Set(TEXT("3.1"));
		VarD->Set(TEXT("3.1"));
		VarE->Set(TEXT("3.1"));

		check(GConsoleVariableCallbackTestCounter == 1);

		// test changed

		check(VarA->TestFlags(ECVF_Changed));
		check(VarB->TestFlags(ECVF_Changed));
		check(VarD->TestFlags(ECVF_Changed));
		check(VarE->TestFlags(ECVF_Changed));

		// verify Set()

		check(VarA->GetString() == FString(TEXT("3")));
		check(VarB->GetString() == FString(TEXT("3.1")));
		check(VarD->GetString() == FString(TEXT("3")));
		check(VarE->GetString() == FString(TEXT("3.1")));
		check(RefD == 3);
		check(RefE == 3.1f);

		VarB->Set(TEXT("3.1"));
		check(GConsoleVariableCallbackTestCounter == 2);

		// unregister

		IConsoleManager::Get().UnregisterConsoleObject(VarA);
		IConsoleManager::Get().UnregisterConsoleObject(VarB, false);
		UnregisterConsoleObject(TEXT("TestNameD"), false);
		UnregisterConsoleObject(TEXT("TestNameE"), false);

		check(!IConsoleManager::Get().FindConsoleVariable(TEXT("TestNameA")));
		check(!IConsoleManager::Get().FindConsoleVariable(TEXT("TestNameB")));
		check(!IConsoleManager::Get().FindConsoleVariable(TEXT("TestNameD")));
		check(!IConsoleManager::Get().FindConsoleVariable(TEXT("TestNameE")));

		// re-register but maintain state
		IConsoleVariable* SecondVarA = IConsoleManager::Get().RegisterConsoleVariable(TEXT("TestNameA"), 1234, TEXT("TestHelpSecondA"), Flags);
		check(SecondVarA == VarA);
		check(SecondVarA->GetInt() == 3);
		check(IConsoleManager::Get().FindConsoleVariable(TEXT("TestNameA")));

		UnregisterConsoleObject(TEXT("TestNameA"), false);
		check(!IConsoleManager::Get().FindConsoleVariable(TEXT("TestNameA")));

		if((Flags & ECVF_RenderThreadSafe) == 0)
		{
			// string is not supported with the flag ECVF_RenderThreadSafe
			IConsoleVariable* VarC = IConsoleManager::Get().RegisterConsoleVariable(TEXT("TestNameC"), TEXT("1.23"), TEXT("TestHelpC"), Flags);
			check(VarC == IConsoleManager::Get().FindConsoleVariable(TEXT("TestNameC")));
			check(VarC->GetInt() == 1);
			// note: exact comparison fails in Win32 release
			check(FMath::IsNearlyEqual(VarC->GetFloat(), 1.23f, KINDA_SMALL_NUMBER));
			check(VarC->GetString() == FString(TEXT("1.23")));
			check(!VarC->TestFlags(ECVF_Changed));
			VarC->Set(TEXT("3.1"));
			check(VarC->TestFlags(ECVF_Changed));
			check(VarC->GetString() == FString(TEXT("3.1")));
			UnregisterConsoleObject(TEXT("TestNameC"), false);
			check(!IConsoleManager::Get().FindConsoleVariable(TEXT("TestNameC")));
		}
	}

	// this should trigger the callback
	IConsoleManager::Get().CallAllConsoleVariableSinks();
	check(GConsoleManagerSinkTestCounter == 1);

	// this should not trigger the callback
	IConsoleManager::Get().CallAllConsoleVariableSinks();
	check(GConsoleManagerSinkTestCounter == 1);

	// this should also trigger the callback
	TestSinkCallback();
	check(GConsoleManagerSinkTestCounter == 2);

	UnregisterConsoleVariableSink(FConsoleCommandDelegate::CreateStatic(&TestSinkCallback));

#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

// These don't belong here, but they belong here more than they belong in launch engine loop.
void CreateConsoleVariables()
{
	// Naming conventions:
	//
	// Console variable should start with (suggestion):
	//
	// r.      Renderer / 3D Engine / graphical feature
	// RHI.    Low level RHI (rendering platform) specific
	// s. 	   Sound / Music
	// n.      Network
	// ai.     Artificial intelligence
	// i.      Input e.g. mouse/keyboard
	// p.      Physics
	// t.      Timer
	// g.      Game
	// Compat.
	// FX.     Particle effects
	// sg.     scalability group (used by scalability system, ini load/save or using SCALABILITY console command)

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.PreViewTranslation"),
		1,
		TEXT("To limit issues with float world space positions we offset the world by the\n")
		TEXT("PreViewTranslation vector. This command allows to disable updating this vector.\n")
		TEXT(" 0: disable update\n")
		TEXT(" 1: update the offset is each frame (default)"),
		ECVF_Cheat);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.FogDensity"),
		-1.0f,
		TEXT("Allows to override the FogDensity setting (needs ExponentialFog in the level).\n")
		TEXT("Using a strong value allows to quickly see which pixel are affected by fog.\n")
		TEXT("Using a start distance allows to cull pixels are can speed up rendering.\n")
		TEXT(" <0: use default settings (default: -1)\n")
		TEXT(">=0: override settings by the given value (0:off, 1=very dense fog)"),
		ECVF_Cheat | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.FogStartDistance"),
		-1.0f,
		TEXT("Allows to override the FogStartDistance setting (needs ExponentialFog in the level).\n")
		TEXT(" <0: use default settings (default: -1)\n")
		TEXT(">=0: override settings by the given value (in world units)"),
		ECVF_Cheat | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.RenderTimeFrozen"),
		0,
		TEXT("Allows to freeze time based effects in order to provide more deterministic render profiling.\n")
		TEXT(" 0: off\n")
		TEXT(" 1: on"),
		ECVF_Cheat);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("FreezeAtPosition"),
		TEXT(""),	// default value is empty
		TEXT("This console variable stores the position and rotation for the FreezeAt command which allows\n")
		TEXT("to lock the camera in order to provide more deterministic render profiling.\n")
		TEXT("The FreezeAtPosition can be set in the ConsoleVariables.ini (start the map with MAPNAME?bTourist=1).\n")
		TEXT("Also see the FreezeAt command console command.\n")
		TEXT("The number syntax if the same as the one used by the BugIt command:\n")
		TEXT(" The first three values define the position, the next three define the rotation.\n")
		TEXT("Example:\n")
		TEXT(" FreezeAtPosition 2819.5520 416.2633 75.1500 65378 -25879 0"),
		ECVF_Cheat);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.VisualizeTexturePool"),
		0,
		TEXT("Allows to enable the visualize the texture pool (currently only on console).\n")
		TEXT(" 0: off\n")
		TEXT(" 1: on"),
		ECVF_Cheat | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.D3DCompilerPath"),
		TEXT(""),	// default
		TEXT("Allows to specify a HLSL compiler version that is different from the one the code was compiled.\n")
		TEXT("No path (\"\") means the default one is used.\n")
		TEXT("If the compiler cannot be found an error is reported and it will compile further with the default one.\n")
		TEXT("This console variable works with ShaderCompileWorker (with multi threading) and without multi threading.\n")
		TEXT("This variable can be set in ConsoleVariables.ini to be defined at startup.\n")
		TEXT("e.g. c:/temp/d3dcompiler_44.dll or \"\""),
		ECVF_Cheat);
	
	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.RenderTargetPoolTest"),
		0,
		TEXT("Clears the texture returned by the rendertarget pool with a special color\n")
		TEXT("so we can see better which passes would need to clear. Doesn't work on volume textures and non rendertargets yet.\n")
		TEXT(" 0:off (default), 1:on"),
		ECVF_Cheat | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.ReflectionEnvironment"),
		1,	
		TEXT("0:off, 1:on and blend with scene, 2:on and overwrite scene.\n")
		TEXT("Whether to render the reflection environment feature, which implements local reflections through Reflection Capture actors."),
		ECVF_Cheat | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.GPUBusyWait"),
		0.0f,	
		TEXT("<=0:off, >0: keep the GPU busy with n units of some fixed amount of work, independent on the resolution\n")
		TEXT("This can be useful to make GPU timing experiments. The value should roughly represent milliseconds.\n")
		TEXT("Clamped at 500."),
		ECVF_Cheat | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("t.OverrideFPS"),
		0.0f,
		TEXT("This allows to override the frame time measurement with a fixed fps number (game can run faster or slower).\n")
		TEXT("<=0:off, in frames per second, e.g. 60"),
		ECVF_Cheat);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.LimitRenderingFeatures"),
		0,
		TEXT("Allows to quickly reduce render feature to increase render performance.\n")
		TEXT("This is just a quick way to alter multiple show flags and console variables in the game\n")
		TEXT("Disabled more feature the higher the number\n")
		TEXT(" <=0:off, order is defined in code (can be documented here when we settled on an order)"),
		ECVF_Cheat | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariableRef(TEXT("r.DumpingMovie"),
		GIsDumpingMovie,
		TEXT("Allows to dump each rendered frame to disk (slow fps, names MovieFrame..).\n")
		TEXT("<=0:off (default), <0:remains on, >0:remains on for n frames (n is the number specified)"),
		ECVF_Cheat);

	// ---------------------------------------
	// physics related
	// ---------------------------------------

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.ShowInitialOverlaps"),
		0,
		TEXT("Show initial overlaps when moving a component, including estimated 'exit' direction.\n")
		TEXT(" 0:off, otherwise on"),
		ECVF_Cheat);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.NetShowCorrections"),
		0,
		TEXT("Whether to draw client position corrections (red is incorrect, green is corrected).\n")
		TEXT("0: Disable, 1: Enable"),
		ECVF_Cheat);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.NetCorrectionLifetime"),
		4.f,
		TEXT("How long a visualized network correction persists.\n")
		TEXT("Time in seconds each visualized network correction persists."),
		ECVF_Cheat);

	// ---------------------------------------

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("g.DebugCameraTraceComplex"),
		1,
		TEXT("Whether DebugCamera should use complex or simple collision for the line trace.\n")
		TEXT("1: complex collision, 0: simple collision "),
		ECVF_Cheat);

	// ---------------------------------------

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.ExposureOffset"),
		0.0f,
		TEXT("For adjusting the exposure on top of post process settings and eye adaptation. For developers only. 0:default"),
		ECVF_Cheat);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.Shadow.FreezeCamera"),
		0,
		TEXT("Debug the shadow methods by allowing to observe the system from outside.\n")
		TEXT("0: default\n")
		TEXT("1: freeze camera at current location"),
		ECVF_Cheat);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.ForceLOD"),
		-1,
		TEXT("LOD level to force, -1 is off."),
		ECVF_Cheat | ECVF_RenderThreadSafe
		);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.MotionBlurFiltering"),
		0,
		TEXT("Useful developer variable\n")
		TEXT("0: off (default, expected by the shader for better quality)\n")
		TEXT("1: on"),
		ECVF_Cheat | ECVF_RenderThreadSafe);

	// ---------------------------------------

	// the following commands are common exec commands that should be added to auto completion (todo: read UnConsole list in ini, discover all exec commands)
	IConsoleManager::Get().RegisterConsoleCommand(TEXT("VisualizeTexture"),	TEXT("To visualize internal textures"), ECVF_Cheat);
	IConsoleManager::Get().RegisterConsoleCommand(TEXT("Vis"),	TEXT("short version of visualizetexture"), ECVF_Cheat);
	IConsoleManager::Get().RegisterConsoleCommand(TEXT("VisRT"),	TEXT("GUI for visualizetexture"), ECVF_Cheat);
	IConsoleManager::Get().RegisterConsoleCommand(TEXT("HighResShot"),	TEXT("High resolution screenshots [Magnification = 2..]"), ECVF_Cheat);
	IConsoleManager::Get().RegisterConsoleCommand(TEXT("DumpConsoleCommands"),	TEXT("Dumps all console vaiables and commands and all exec that can be discovered to the log/console"), ECVF_Default);
	IConsoleManager::Get().RegisterConsoleCommand(TEXT("DumpUnbuiltLightInteractions"),	TEXT("Logs all lights and primitives that have an unbuilt interaction."), ECVF_Cheat);

#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.UniformBufferPooling"),
		1,
		TEXT("If we pool object in RHICreateUniformBuffer to have less real API calls to creat buffers\n")
		TEXT(" 0: off (for debugging)\n")
		TEXT(" 1: on (optimization)"),
		ECVF_RenderThreadSafe);

	// The following console variable should never be compiled out ------------------
	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.ClearWithExcludeRects"),
		2,
		TEXT("Control the use of exclude rects when using RHIClear\n")
		TEXT(" 0: Force off (can be faster on hardware that has fast clears)\n")
		TEXT(" 1: Use exclude rect if supplied\n")
		TEXT(" 2: Auto (default is 2, pick what is considered best on this hardware)"),
		ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.SimpleDynamicLighting"),
		0,	
		TEXT("Whether to use simple dynamic lighting, which just renders an unshadowed dynamic directional light and a skylight.\n")
		TEXT("All other lighting features are disabled when true.  This is useful for supporting very low end hardware.\n")
		TEXT("0:off, 1:on"),
		ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.MobileHDR"),
		1,	
		TEXT("0: Mobile renders in LDR gamma space. (suggested for unlit games targeting low-end phones)\n")
		TEXT("1: Mobile renders in HDR linear space. (default)"),
		ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.MobileHDR32bpp"),
		0,	
		TEXT("0: Mobile HDR renders to an FP16 render target.\n")
		TEXT("1: Mobile HDR renders to an RGBA8 target."),
		ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.ClearSceneMethod"),
		1,
		TEXT("Select how scene rendertarget clears are handled\n")
		TEXT(" 0: No clear\n")
		TEXT(" 1: RHIClear (default)\n")
		TEXT(" 2: Quad at max z"),
		ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.SeparateTranslucency"),
		1,
		TEXT("Allows to disable the separate translucency feature (all translucency is rendered in separate RT and composited\n")
		TEXT("after DOF, if not specified otherwise in the material).\n")
		TEXT(" 0: off (translucency is affected by depth of field)\n")
		TEXT(" 1: on costs GPU performance and memory but keeps translucency unaffected by Depth of Fieled. (default)"),
		ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.LensFlareQuality"),
		2,
		TEXT(" 0: off but best for performance\n")
		TEXT(" 1: low quality with good performance\n")
		TEXT(" 2: good quality (default)\n")
		TEXT(" 3: very good quality but bad performance"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.BloomQuality"),
		5,
		TEXT(" 0: off, no performance impact.\n")
		TEXT(" 1: average quality, least performance impact.\n")
		TEXT(" 2: average quality, least performance impact.\n")
		TEXT(" 3: good quality.\n")
		TEXT(" 4: good quality.\n")
		TEXT(" 5: Best quality, most significant performance impact. (default)\n")
		TEXT(">5: force experimental higher quality on mobile (can be quite slow on some hardware)"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.SceneColorFringeQuality"),
		1,
		TEXT(" 0: off but best for performance\n")
		TEXT(" 1: 3 texture samples (default)n"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.UpsampleQuality"),
		3,
		TEXT(" 0: Nearest filtering\n")
		TEXT(" 1: Simple Bilinear\n")
		TEXT(" 2: 4 tap bilinear\n")
		TEXT(" 3: Directional blur with unsharp mask upsample. (default)"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	// ---------------------------------------
	// physics related
	// ---------------------------------------

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.InitialOverlapTolerance"),
		0.0f,
		TEXT("Tolerance for initial overlapping test in PrimitiveComponent movement.\n")
		TEXT("Normals within this tolerance are ignored if moving out of the object.\n")
		TEXT("Dot product of movement direction and surface normal."),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.PenetrationPullbackDistance"),
		0.125f,
		TEXT("Pull out from penetration of an object by this extra distance.\n")
		TEXT("Distance added to penetration fix-ups."),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.EncroachEpsilon"),
		0.15f,
		TEXT("Epsilon value encroach checking for capsules\n")
		TEXT("0: use full sized capsule. > 0: shrink capsule size by this amount (world units)"),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.NetEnableMoveCombining"),
		1,
		TEXT("Whether to enable move combining on the client to reduce bandwidth by combining similar moves.\n")
		TEXT("0: Disable, 1: Enable"),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.NetProxyShrinkRadius"),
		0.01f,
		TEXT("Shrink simulated proxy capsule radius by this amount, to account for network rounding that may cause encroachment.\n")
		TEXT("Changing this value at runtime may require the proxy to re-join for correct behavior.\n")
		TEXT("<= 0: disabled, > 0: shrink by this amount."),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.NetProxyShrinkHalfHeight"),
		0.01f,
		TEXT("Shrink simulated proxy capsule half height by this amount, to account for network rounding that may cause encroachment.\n")
		TEXT("Changing this value at runtime may require the proxy to re-join for correct behavior.\n")
		TEXT("<= 0: disabled, > 0: shrink by this amount."),
		ECVF_Default);



	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.ToleranceScale_Length"),
		100.f,
		TEXT("The approximate size of objects in the simulation. Default: 100"),
		ECVF_ReadOnly);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.ToleranceScale_Mass"),
		100.f,
		TEXT("The approximate mass of a length * length * length block. Default: 100"),
		ECVF_ReadOnly);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.ToleranceScale_Speed"),
		1000.f,
		TEXT("The typical magnitude of velocities of objects in simulation. Default: 1000"),
		ECVF_ReadOnly);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.ConstraintDampingScale"),
		100000.f,
		TEXT("The multiplier of constraint damping in simulation. Default: 100000"),
		ECVF_ReadOnly);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.ConstraintStiffnessScale"),
		100000.f,
		TEXT("The multiplier of constraint stiffness in simulation. Default: 100000"),
		ECVF_ReadOnly);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.ContactOffsetFactor"),
		0.01f,
		TEXT("Multiplied by min dimension of object to calculate how close objects get before generating contacts. Default: 0.01"),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.MaxContactOffset"),
		1.f,
		TEXT("Max value of contact offset, which controls how close objects get before generating contacts. Default: 1.0"),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.BounceThresholdVelocity"),
		200.f,
		TEXT("A contact with a relative velocity below this will not bounce. Default: 200"),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.EnableAsyncScene"),
		1,
		TEXT("If enabled, an additional async scene is created."),
		ECVF_Default);

	// ---------------------------------------
	// APEX

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.APEXMaxDestructibleDynamicChunkIslandCount"),
		2000,
		TEXT("APEX Max Destructilbe Dynamic Chunk Island Count."),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.APEXMaxDestructibleDynamicChunkCount"),
		2000,
		TEXT("APEX Max Destructible dynamic Chunk Count."),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("p.bAPEXSortDynamicChunksByBenefit"),
		1,
		TEXT("True if APEX should sort dynamic chunks by benefit."),
		ECVF_Default);

	// ---------------------------------------

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.AmbientOcclusionLevels"),
		3,
		TEXT("Defines how many mip levels are using during the ambient occlusion calculation. This is useful when tweaking the algorithm.\n")
		TEXT(" 0:none, 1:one, 2:two, 3:three(default), 4:four(larger at little cost but can flicker)"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.AmbientOcclusionRadiusScale"),
		1.0f,
		TEXT("Allows to scale the ambient occlusion radius (SSAO).\n")
		TEXT(" 0:off, 1.0:normal, <1:smaller, >1:larger"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.AtmosphereRender"),
		1,
		TEXT("Defines atmosphere will render or not. Only changed by r.Atmosphere console command\n")
		TEXT(" 0: off\n")
		TEXT(" 1: on"),
		ECVF_ReadOnly);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.ShadowQuality"),
		5,
		TEXT("Defines the shadow method which allows to adjust for quality or performance.\n")
		TEXT(" 0:off, 1:low(unfiltered), 2:low .. 5:max (default)"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.MotionBlurQuality"),
		4,
		TEXT("Defines the motion blur method which allows to adjust for quality or performance.\n")
		TEXT(" 0:off, 1:low, 2:medium, 3:high (default), 4: very high"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.PostProcessAAQuality"),
		4,
		TEXT("Defines the postprocess anti aliasing method which allows to adjust for quality or performance.\n")
		TEXT(" 0:off, 1:very low, 2:low, 3:medium, 4:high (default), 4:very high, 6:max"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.MotionBlurSoftEdgeSize"),
		1.0f,
		TEXT("Defines how lange object motion blur is blurred (percent of screen width) to allow soft edge motion blur.\n")
		TEXT("This scales linearly with the size (up to a maximum of 32 samples, 2.5 is about 18 samples) and with screen resolution\n")
		TEXT("Smaller values are better for performance and provide more accurate motion vectors but the blurring outside the object is reduced.\n")
		TEXT("If needed this can be exposed like the other motionblur settings.\n")
		TEXT(" 0:off (not free and does never completely disable), >0, 1.0 (default)"),
		ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.Color.Min"),
		0.0f,
		TEXT("Allows to define where the value 0 in the color channels is mapped to after color grading.\n")
		TEXT("The value should be around 0, positive: a gray scale is added to the darks, negative: more dark values become black, Default: 0"),
		ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.Color.Mid"),
		0.5f,
		TEXT("Allows to define where the value 0.5 in the color channels is mapped to after color grading (This is similar to a gamma correction).\n")
		TEXT("Value should be around 0.5, smaller values darken the mid tones, larger values brighten the mid tones, Default: 0.5"),
		ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.Color.Max"),
		1.0f,
		TEXT("Allows to define where the value 1.0 in the color channels is mapped to after color grading.\n")
		TEXT("Value should be around 1, smaller values darken the highlights, larger values move more colors towards white, Default: 1"),
		ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.FullScreenMode"),
		2,
		TEXT("Defines how we do full screen when requested (e.g. command line option -fullscreen or in ini [SystemSettings] fullscreen=true)\n")
		TEXT(" 0: normal full screen (renders faster, more control over vsync, less GPU memory, 10bit color if possible)\n")
		TEXT(" 1: windowed full screen, desktop resolution (quick switch between applications and window mode, full quality)\n")
		TEXT(" 2: windowed full screen, specified resolution (like 1 but no unintuitive performance cliff, can be blurry, default)\n")
		TEXT(" any other number behaves like 0"),
		ECVF_Scalability);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.GBufferFormat"),
		1,
		TEXT("Defines the memory layout used for the GBuffer.\n")
		TEXT("(affects performance, mostly through bandwidth, quality of normals and material attributes).\n")
		TEXT(" 0: lower precision (8bit per component, for profiling)\n")
		TEXT(" 1: low precision (default)\n")
		TEXT(" 5: high precision"),
		ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.SceneColorFormat"),
		4,
		TEXT("Defines the memory layout (RGBA) used for the scene color\n")
		TEXT("(affects performance, mostly through bandwidth, quality especially with translucency).\n")
		TEXT(" 0: PF_B8G8R8A8 32Bit (mostly for testing, likely to unusable with HDR)\n")
		TEXT(" 1: PF_A2B10G10R10 32Bit\n")
		TEXT(" 2: PF_FloatR11G11B10 32Bit\n")
		TEXT(" 3: PF_FloatRGB 32Bit\n")
		TEXT(" 4: PF_FloatRGBA 64Bit (default, might be overkill, especially if translucency is mostly using SeparateTranslucency)\n")
		TEXT(" 5: PF_A32B32G32R32F 128Bit (unreasonable but good for testing)"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.AmbientOcclusionStaticFraction"),
		-1.0f,
		TEXT("Allows to override the Ambient Occlusion Static Fraction (see post process volume). Fractions are between 0 and 1.\n")
		TEXT("<0: use default setting (default -1)\n")
		TEXT(" 0: no effect on static lighting, 0 is free meaning no extra rendering pass\n")
		TEXT(" 1: AO affects the stat lighting"),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.DepthOfFieldQuality"),
		2,
		TEXT("Allows to adjust the depth of field quality. Currently only fully affects BokehDOF. GaussianDOF is either 0 for off, otherwise on.\n")
		TEXT(" 0: Off\n")
		TEXT(" 1: Low\n")
		TEXT(" 2: high quality (default, adaptive, can be 4x slower)"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.DepthOfFieldNearBlurSizeThreshold"),
		0.01f,
		TEXT("Sets the minimum near blur size before the effect is forcably disabled. Currently only affects Gaussian DOF.\n")
		TEXT(" (default = 0.01f)"),
		ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.ScreenPercentage"),
		-1.0f,
		TEXT("To render in lower resolution and upscale for better performance.\n")
		TEXT("70 is a good value for low aliasing and performance, can be verified with 'show TestImage'\n")
		TEXT("in percent, >0 and <=100, <0 means the post process volume settings are used"),
		ECVF_Scalability | ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.ScreenPercentageSoftness"),
		0.3f,
		TEXT("To scale up with higher quality loosing some sharpness\n")
		TEXT(" 0..1 (0.3 is good for ScreenPercentage 90"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.BlackBorders"),
		1,	
		TEXT("To draw black borders around the rendered image\n")
		TEXT("(prevents artifacts from post processing passes that read outside of the image e.g. PostProcessAA)\n")
		TEXT("in pixels, 0:off"),
		ECVF_Default);

	// Changing this causes a full shader recompile
	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.UseDiffuseSpecularMaterialInputs"),
		0,	
		TEXT("Whether to use DiffuseColor, SpecularColor material inputs, otherwise BaseColor, Metallic, Specular will be used. Cannot be changed at runtime."),
		ECVF_ReadOnly);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.MaterialQualityLevel"),
		1,	
		TEXT("0 corresponds to low quality materials, as defined by quality switches in materials, 1 corresponds to high."),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("Compat.UseDXT5NormalMaps"),
		0,	
		TEXT("Whether to use DXT5 for normal maps, otherwise BC5 will be used, which is not supported on all hardware.\n")
		TEXT("Both formats require the same amount of memory (if driver doesn't emulate the format).\n")
		TEXT("Changing this will cause normal maps to be recompressed on next load (or when using recompile shaders)\n")
		TEXT(" 0: Use BC5 texture format (default)\n")
		TEXT(" 1: Use DXT5 texture format (lower quality)"),
		// 
		// Changing this causes a full shader recompile

		ECVF_ReadOnly);

	// Changing this is currently unsupported after content has been chunked with the previous setting
	// Changing this causes a full shader recompile
	IConsoleManager::Get().RegisterConsoleVariable(TEXT("Compat.MAX_GPUSKIN_BONES"),
		256,	
		TEXT("Max number of bones that can be skinned on the GPU in a single draw call. Cannot be changed at runtime."),
		ECVF_ReadOnly);

	// Changing this causes a full shader recompile
	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.AllowStaticLighting"),
		1,	
		TEXT("Whether to allow any static lighting to be generated and used, like lightmaps and shadowmaps.\n")
		TEXT("Games that only use dynamic lighting should set this to 0 to save some static lighting overhead."),
		ECVF_ReadOnly | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("con.MinLogVerbosity"),
		0,
		TEXT("Allows to see the log in the in game console (by default deactivated to avoid spam and minor performance loss).\n")
		TEXT(" 0: no logging other than console response (default)\n")
		TEXT(" 1: Only fatal errors (no that useful)\n")
		TEXT(" 2: additionally errors\n")
		TEXT(" 3: additionally warnings\n")
		TEXT(" 4: additionally display\n")
		TEXT(" 5: additionally log\n")
		TEXT("..\n")
		TEXT(">=7: all"),
		ECVF_Default);
	
	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.MSAA.CompositingSampleCount"),
		4,
		TEXT("Affects the render quality of the editor 3d objects.\n")
		TEXT(" 1: no MSAA, lowest quality\n")
		TEXT(" 2: 2x MSAA, medium quality (medium GPU memory consumption)\n")
		TEXT(" 4: 4x MSAA, high quality (high GPU memory consumption)\n")
		TEXT(" 8: 8x MSAA, very high quality (insane GPU memory consumption)"),
		ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("net.DormancyEnable"),
		1,	
		TEXT("Enables Network Dormancy System for reducing CPU and bandwidth overhead of infrequently updated actors\n")
		TEXT("1 Enables network dormancy. 0 disables network dormancy."),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("net.DormancyDraw"),
		0,
		TEXT("Draws debug information for network dormancy\n")
		TEXT("1 Enables network dormancy debugging. 0 disables."),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("net.DormancyDrawCullDistance"),
		5000.f,
		TEXT("Cull distance for net.DormancyDraw. World Units")
		TEXT("Max world units an actor can be away from the local view to draw its dormancy status"),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("net.DormancyValidate"),
		0,
		TEXT("Validates that dormant actors do not change state while in a dormant state (on server only)")
		TEXT("0: Dont validate. 1: Validate on wake up. 2: Validate on each net update"),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("net.PackageMap.DebugObject"),
		TEXT(""),
		TEXT("Debugs PackageMap serialization of object")
		TEXT("Partial name of object to debug"),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("net.Replication.DebugProperty"),
		TEXT(""),
		TEXT("Debugs Replication of property by name")
		TEXT("Partial name of property to debug"),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("net.Reliable.Debug"),
		0,
		TEXT("Print all reliable bunches sent over the network\n")
		TEXT(" 0: no print.\n")
		TEXT(" 1: Print bunches as they are sent.\n")
		TEXT(" 2: Print reliable bunch buffer each net update"),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("net.RPC.Debug"),
		0,
		TEXT("Print all RPC bunches sent over the network\n")
		TEXT(" 0: no print.\n")
		TEXT(" 1: Print bunches as they are sent."),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("net.Montage.Debug"),
		0,
		TEXT("Prints Replication information about AnimMontages\n")
		TEXT(" 0: no print.\n")
		TEXT(" 1: Print AnimMontage info on client side as they are played."),
		ECVF_Cheat);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.RenderTargetPoolMin"),
		400,
		TEXT("If the render target pool size (in MB) is below this number there is no deallocation of rendertargets")
		TEXT("Default is 200 MB."),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.Shadow.DistanceFieldPenumbraSize"),
		0.05f,
		TEXT("Controls the size of the uniform penumbra produced by static shadowing.\n")
		TEXT("This is a fraction of MaxTransitionDistanceWorldSpace in BaseLightmass.ini."),
		ECVF_RenderThreadSafe);

	static TAutoConsoleVariable<int32> CVarIdleWhenNotForeground(
		TEXT("t.IdleWhenNotForeground"),0,
		TEXT("Prevents the engine from taking any CPU or GPU time while not the foreground app."),
		ECVF_Cheat);
	
	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.CustomDepth"),
		1,
		TEXT("0: feature is disabled\n")
		TEXT("1: feature is enabled, texture is created on demand\n")
		TEXT("2: feature is enabled, texture is not released until required (should be the project setting if the feature should not stall)"),
		ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.TemporalAASamples"),
		8,
		TEXT("Number of jittered positions for temporal AA (4, 8=default, 16, 32, 64)."),
		ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.VSync"),
		0,
		TEXT("0: VSync is disabled.(default)\n")
		TEXT("1: VSync is enabled."),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("R.FinishCurrentFrame"),
		0,
		TEXT("If on, the current frame will be forced to finish and render to the screen instead of being buffered.  This will improve latency, but slow down overall performance."),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.MaxAnisotropy"),
		4,
		TEXT("MaxAnisotropy should range from 1 to 16. Higher values mean better texure quality when using anisotropic filtering but at a cost to performance."),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.Shadow.MaxResolution"),
		1024,
		TEXT("Max square dimensions (in texels) allowed for rendering shadow depths. Range 4 to hardware limit. Higher = better quality shadows but at a performance cost."),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.Shadow.CSM.TransitionScale"),
		1.0f,
		TEXT("Allows to scale the cascaded shadow map transition region. Clamped within 0..2.\n")
		TEXT("0: no transition (fastest)\n")
		TEXT("1: as specific in the light settings (default)\n")
		TEXT("2: 2x larger than what was specified in the light"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.MobileContentScaleFactor"),
		1.0f,
		TEXT("Content scale multiplier (equates to iOS's contentScaleFactor to support Retina displays"),
		ECVF_Default);

#if PLATFORM_SUPPORTS_TEXTURE_STREAMING
	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.TextureStreaming"),
		1,
		TEXT("Allows to define if texture streaming is enabled, can be changed at run time.\n")
		TEXT("0: off\n")
		TEXT("1: on (default)"),
		ECVF_Default | ECVF_RenderThreadSafe);
#endif

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.Streaming.PoolSize"),
		-1,
		TEXT("-1: Default texture pool size, otherwise the size in MB\n"),
		ECVF_Scalability );

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.Streaming.UseFixedPoolSize"),
		0,
		TEXT("If non-zero, do not allow the pool size to change at run time."),
		ECVF_ReadOnly );

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.ShaderDevelopmentMode"),
		0,
		TEXT("0: Default, 1: Enable various shader development utilities, such as the ability to retry on failed shader compile, and extra logging as shaders are compiled."),
		ECVF_Default);

	// this cvar can be removed in shipping to not compile shaders for development (faster)
	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.CompileShadersForDevelopment"),
		1,
		TEXT("Setting this to 0 allows to ship a game with more optimized shaders as some\n")
		TEXT("editor and development features are not longer compiled into the shaders.\n")
		TEXT(" Note: This should be done when shipping but it's not done automatically yet (feature need to mature\n")
		TEXT("       and shaders will compile slower as shader caching from development isn't shared).\n")
		TEXT("Cannot be changed at runtime - can be put into BaseEngine.ini\n")
		TEXT(" 0: off, shader can run a bit faster\n")
		TEXT(" 1: on (Default)"),
		ECVF_ReadOnly);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.ViewDistanceScale"),
		1.0f,
		TEXT("Controls the view distance scale. A primitive's MaxDrawDistance is scaled by this value.\n")
		TEXT("Higher values will increase view distance but at a performance cost.\n")
		TEXT("Default = 1. Value should be in the range [0.0f, 1.0f]."),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.LightFunctionQuality"),
		2,
		TEXT("Defines the light function quality which allows to adjust for quality or performance.\n")
		TEXT("<=0: off (fastest)\n")
		TEXT("  1: low quality (e.g. half res with blurring, not yet implemented)\n")
		TEXT("  2: normal quality (default)\n")
		TEXT("  3: high quality (e.g. super-sampled or colored, not yet implemented)"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.EyeAdaptationQuality"),
		2,
		TEXT("Defines the eye adaptation quality which allows to adjust for quality or performance.\n")
		TEXT("<=0: off (fastest)\n")
		TEXT("  1: low quality (e.g. non histogram based, not yet implemented)\n")
		TEXT("  2: normal quality (default)\n")
		TEXT("  3: high quality (e.g. screen position localized, not yet implemented)"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.Shadow.DistanceScale"),
		1.0f,
		TEXT("Scalability option to trade shadow distance versus performance for directional lights (clamped within a reasonable range).\n")
		TEXT("<1: shorter distance\n")
		TEXT(" 1: normal (default)\n")
		TEXT(">1: larger distance"),
		ECVF_Scalability | ECVF_RenderThreadSafe);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.BokehDOFIndexStyle"),
#if PLATFORM_MAC // Avoid a driver bug on OSX/NV cards that causes driver to generate an unwound index buffer
		1,
#else
		0,
#endif
		TEXT("Controls whether to use a packed or unwound index buffer for Bokeh DOF.\n")
		TEXT("0: Use packed index buffer (faster) (default)\n")
		TEXT("1: Use unwound index buffer (slower)"),
		ECVF_ReadOnly | ECVF_RenderThreadSafe);



	IConsoleManager::Get().RegisterConsoleVariable(TEXT("r.FreeSkeletalMeshBuffers"),
		0,
		TEXT("Controls whether skeletal mesh buffers are kept in CPU memory to support merging of skeletal meshes.\n")
		TEXT("0: Keep buffers(default)\n")
		TEXT("1: Free buffers"),
		ECVF_RenderThreadSafe );

	// testing code
	{
		FConsoleManager& ConsoleManager = (FConsoleManager&)IConsoleManager::Get();

		ConsoleManager.Test();
	}
}


static TAutoConsoleVariable<int32> CVarTonemapperQuality(
	TEXT("r.TonemapperQuality"),
	1,
	TEXT("0: low (minor performance benefit)\n")
	TEXT("1: high (default, with high frequency pixel pattern to fight 8 bit color quantization)"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarDetailMode(
	TEXT("r.DetailMode"),
	2,
	TEXT("Current detail mode; determines whether components of actors should be updated/ ticked.\n")
	TEXT(" 0: low, show only object with DetailMode low or higher\n")
	TEXT(" 1: medium, show all object with DetailMode medium or higher\n")
	TEXT(" 2: high, show all objects (default)"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarRefractionQuality(
	TEXT("r.RefractionQuality"),
	2,
	TEXT("Defines the distorion/refraction quality which allows to adjust for quality or performance.\n")
	TEXT("<=0: off (fastest)\n")
	TEXT("  1: low quality (not yet implemented)\n")
	TEXT("  2: normal quality (default)\n")
	TEXT("  3: high quality (e.g. color fringe, not yet implemented)"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarSSSSS(
	TEXT("r.SSSSS"),
	0.0f,
	TEXT("Very experimental screen space subsurface scattering on non unlit/lit materials\n")
	TEXT("0: off\n")
	TEXT("x: SSS radius in world space e.g. 10"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarTriangleOrderOptimization(
	TEXT("r.TriangleOrderOptimization"),
	1,
	TEXT("Controls the algorithm to use when optimizing the triangle order for the post-transform cache.\n")
	TEXT("0: Use NVTriStrip (slower)\n")
	TEXT("1: Use Forsyth algorithm (fastest)(default)"),
	ECVF_ReadOnly);	

static TAutoConsoleVariable<float> CVarFastBlurThreshold(
	TEXT("r.FastBlurThreshold"),
	7.0f,
	TEXT("Defines at what radius the Gaussian blur optimization kicks in (estimated 25% - 40% faster).\n")
	TEXT("The optimzation uses slightly less memory and has a quality loss on smallblur radius.\n")
	TEXT("  0: use the optimization always (fastest, lowest quality)\n")
	TEXT("  3: use the optimization starting at a 3 pixel radius (quite fast)\n")
	TEXT("  7: use the optimization starting at a 7 pixel radius (default)\n")
	TEXT(">15: barely ever use the optimization (high quality)"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarUIBlurRadius(
	TEXT("UI.BlurRadius"),
	1.2f,
	TEXT("Gaussian blur radius for post process UIBlur operation\n")
	TEXT("as a percent of screen width\n")
	TEXT("0 to turn off gaussian blur. (Default 1.2)"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarDrawRectangleOptimization(
	TEXT("r.DrawRectangleOptimization"),
	1,
	TEXT("Controls an optimization for DrawRectangle(). When enabled a triangle can be used to draw a quad in certain situations (viewport sized quad).\n")
	TEXT("Using a triangle allows for slightly faster post processing in lower resolutions but can not always be used.\n")
	TEXT(" 0: Optimization is disabled, DrawDenormalizedQuad always render with quad\n")
	TEXT(" 1: Optimization is enabled, a triangle can be rendered where specified (default)"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarDBuffer(
	TEXT("r.DBuffer"),
	0,
	TEXT("Experimental DBuffer feature: Generating deferred decals before the BasePass.\n")
	TEXT("Allows decals to be correctly lit by baked lighting.\n")
	TEXT(" 0: off\n")
	TEXT(" 1: on (needs early pass rendering on all decal receivers and base pass lookups into the DBuffer, costs GPU memory, allows GBuffer compression)"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarSkeletalMeshLODRadiusScale(
	TEXT("r.SkeletalMeshLODRadiusScale"),
	1.0f,
	TEXT("Scale factor for the screen radius used in computing discrete LOD for skeletal meshes. (0.25-1)"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarStaticMeshLODDistanceScale(
	TEXT("r.StaticMeshLODDistanceScale"),
	1.0f,
	TEXT("Scale factor for the distance used in computing discrete LOD for static meshes. (0.25-1)"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarOnlyStreamInTextures(
	TEXT("r.OnlyStreamInTextures"),
	0,
	TEXT("If set to 1, texture will only be streamed in, not out"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarPreTileTextures(
	TEXT("r.PreTileTextures"),
	1,
	TEXT("If set to 1, textures will be tiled during cook and are expected to be cooked at runtime"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarAllowHighQualityLightMaps(
	TEXT("r.HighQualityLightMaps"),
	1,
	TEXT("If set to 1, allow high quality lightmaps which don't bake in direct lighting of stationary lights"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarSetMipMapLODBias(
	TEXT("r.MipMapLODBias"),
	0.0f,
	TEXT("Apply additional mip map bias for all 2D textures, range of -15.0 to 15.0"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarVirtualTextureEnabled(
	TEXT("r.VirtualTexture"),
	1,
	TEXT("If set to 1, textures will use virtual memory to optimize texture mip streaming"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarVirtualTextureReducedMemoryEnabled(
	TEXT("r.VirtualTextureReducedMemory"),
	1,
	TEXT("If set to 1, the cost of virtual memory will be reduced by transforming the texture between streamed & resident"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarPrecomputedVisibilityWarning(
	TEXT("r.PrecomputedVisibilityWarning"),
	0,
	TEXT("If set to 1, a warning will be displayed when rendering a scene from a view point without precomputed visibility."),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarDemosaicVposOffset(
	TEXT("r.DemosaicVposOffset"),
	0.0f,
	TEXT("This offset is added to the rasterized position used for demosaic in the ES2 tonemapping shader. It exists to workaround driver bugs on some Android devices that have a half-pixel offset."),
	ECVF_RenderThreadSafe);
