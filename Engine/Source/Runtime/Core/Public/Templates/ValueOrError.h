// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

template<typename ArgType>
struct TValueOrError_ValueProxy
{
	TValueOrError_ValueProxy(ArgType&& InArg) : Arg(InArg) {}
	ArgType& Arg;
};

template<typename ArgType>
struct TValueOrError_ErrorProxy
{
	TValueOrError_ErrorProxy(ArgType&& InArg) : Arg(InArg) {}
	ArgType& Arg;
};

template <typename A>
FORCEINLINE TValueOrError_ValueProxy<A> MakeValue(A&& arg)
{
    return TValueOrError_ValueProxy<A>(Forward<A>(arg));
}

template <typename A>
FORCEINLINE TValueOrError_ErrorProxy<A> MakeError(A&& arg)
{
    return TValueOrError_ErrorProxy<A>(Forward<A>(arg));
}

/** Type used to return either some data, or an error */
template<typename ValueType, typename ErrorType>
class TValueOrError
{
public:
	/** Construct the result from a value, or an error (See MakeValue and MakeError) */
	template<typename A>
	TValueOrError(TValueOrError_ValueProxy<A>&& Proxy) 		: Value(Forward<A>(Proxy.Arg)) {}

	template<typename A>
	TValueOrError(TValueOrError_ErrorProxy<A>&& Proxy)		: Error(Forward<A>(Proxy.Arg)) {}

	/** Copy construction/Assignment */
	TValueOrError(const TValueOrError& In) 					: Error(In.Error), Value(In.Value) {}
	TValueOrError& operator=(const TValueOrError& In) 		{ Error = In.Error; Value = In.Value; return *this; }

	/** Move construction/assignment */
	TValueOrError(TValueOrError&& In) 						: Error(MoveTemp(In.Error)), Value(MoveTemp(In.Value)) {}
	TValueOrError& operator=(TValueOrError&& In) 			{ Error = MoveTemp(In.Error); Value = MoveTemp(In.Value); return *this; }

	/** Check whether this value is valid */
	bool IsValid() const				{ return Value.IsSet() && !Error.IsSet(); }

	/** Get the error, if set */
	ErrorType& GetError()				{ return Error.GetValue(); }
	const ErrorType& GetError() const	{ return Error.GetValue(); }
	
	/** Steal this result's error, if set, causing it to become unset */
	ErrorType StealError()				{ ErrorType Temp = MoveTemp(Error.GetValue()); Error.Reset(); return Temp; }

	/** Access the value contained in this result */
	ValueType& GetValue()				{ return Value.GetValue(); }
	const ValueType& GetValue() const	{ return Value.GetValue(); }

	/** Steal this result's value, causing it to become unset */
	ValueType StealValue()				{ ValueType Temp = MoveTemp(Value.GetValue()); Value.Reset(); return Temp; }

private:

	/** The error reported by the procedure, if any */
	TOptional<ErrorType> Error;
	/** Optional value to return as part of the result */
	TOptional<ValueType> Value;	
};