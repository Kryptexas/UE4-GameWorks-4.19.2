// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	DelegateInstancesImpl.inl: Inline implementation of delegate instance classes
================================================================================*/


// Only designed to be included directly by Delegate.h
#if !defined( __Delegate_h__ ) || !defined( FUNC_INCLUDING_INLINE_IMPL )
	#error "This inline header must only be included by Delegate.h"
#endif


// NOTE: This file in re-included for EVERY delegate signature that we support.  That is, every combination
//		 of parameter count, return value presence or other function modifier will include this file to
//		 generate a delegate interface type and implementation type for that signature.


#define DELEGATE_INSTANCE_INTERFACE_CLASS FUNC_COMBINE( IBaseDelegateInstance_, FUNC_SUFFIX )
#define SP_METHOD_DELEGATE_INSTANCE_CLASS FUNC_COMBINE( TBaseSPMethodDelegateInstance_, FUNC_COMBINE( FUNC_PAYLOAD_SUFFIX, FUNC_CONST_SUFFIX ) )
#define RAW_METHOD_DELEGATE_INSTANCE_CLASS FUNC_COMBINE( TBaseRawMethodDelegateInstance_, FUNC_COMBINE( FUNC_PAYLOAD_SUFFIX, FUNC_CONST_SUFFIX ) )
#define UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS FUNC_COMBINE( TBaseUObjectMethodDelegateInstance_, FUNC_COMBINE( FUNC_PAYLOAD_SUFFIX, FUNC_CONST_SUFFIX ) )
#define STATIC_DELEGATE_INSTANCE_CLASS FUNC_COMBINE( TBaseStaticDelegateInstance_, FUNC_COMBINE( FUNC_PAYLOAD_SUFFIX, FUNC_CONST_SUFFIX ) )

#if FUNC_IS_CONST
	#define DELEGATE_QUALIFIER const
#else
	#define DELEGATE_QUALIFIER
#endif

#if FUNC_HAS_PAYLOAD
	#define DELEGATE_COMMA_PAYLOAD_LIST             , FUNC_PAYLOAD_LIST
	#define DELEGATE_COMMA_PAYLOAD_PASSTHRU         , FUNC_PAYLOAD_PASSTHRU
	#define DELEGATE_COMMA_PAYLOAD_INITIALIZER_LIST , FUNC_PAYLOAD_INITIALIZER_LIST
	#define DELEGATE_COMMA_PAYLOAD_PASSIN           , FUNC_PAYLOAD_PASSIN
	#if FUNC_HAS_PARAMS
		#define DELEGATE_PARAM_PASSTHRU_COMMA_PAYLOAD_PASSIN FUNC_PARAM_PASSTHRU, FUNC_PAYLOAD_PASSIN
		#define DELEGATE_PARAM_LIST_COMMA_PAYLOAD_LIST       FUNC_PARAM_LIST, FUNC_PAYLOAD_LIST
	#else
		#define DELEGATE_PARAM_PASSTHRU_COMMA_PAYLOAD_PASSIN FUNC_PAYLOAD_PASSIN
		#define DELEGATE_PARAM_LIST_COMMA_PAYLOAD_LIST       FUNC_PAYLOAD_LIST
	#endif
#else
	#define DELEGATE_COMMA_PAYLOAD_LIST
	#define DELEGATE_COMMA_PAYLOAD_PASSTHRU
	#define DELEGATE_COMMA_PAYLOAD_INITIALIZER_LIST
	#define DELEGATE_COMMA_PAYLOAD_PASSIN
	#define DELEGATE_PARAM_PASSTHRU_COMMA_PAYLOAD_PASSIN FUNC_PARAM_PASSTHRU
	#define DELEGATE_PARAM_LIST_COMMA_PAYLOAD_LIST       FUNC_PARAM_LIST
#endif


template< class UserClass, FUNC_PAYLOAD_TEMPLATE_DECL_TYPENAME > class UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS;
template< class UserClass, FUNC_PAYLOAD_TEMPLATE_DECL_TYPENAME > class RAW_METHOD_DELEGATE_INSTANCE_CLASS;

/**
 * Shared pointer-based delegate instance class for methods.  For internal use only.
 */
template< class UserClass, FUNC_PAYLOAD_TEMPLATE_DECL_TYPENAME, ESPMode::Type SPMode >
class SP_METHOD_DELEGATE_INSTANCE_CLASS
	: public DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS >
{

	/**
	 * Internal, inlined and non-virtual version of GetRawUserObject interface.
	 */
	FORCEINLINE const void* GetRawUserObjectInternal() const
	{
		return UserObject.Pin().Get();
	}

	/**
	 * Internal, inlined and non-virtual version of GetRawMethod interface.
	 */
	FORCEINLINE const void* GetRawMethodPtrInternal() const
	{
		return *(const void**)&MethodPtr;
	}

public:

	/** Pointer-to-member typedef */
	typedef RetValType ( UserClass::*FMethodPtr )( DELEGATE_PARAM_LIST_COMMA_PAYLOAD_LIST ) DELEGATE_QUALIFIER;


	/**
	 * Static helper function that returns a newly-created delegate object for
	 * the specified user class and class method
	 *
	 * @param  InUserObjectRef  Shared reference to the user's object that contains the class method
	 * @param  InFunc  Member function pointer to your class method
	 *
	 * @return  Returns an instance of the new delegate
	 */
	FORCEINLINE static DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS >* Create( const TSharedPtr< UserClass, SPMode >& InUserObjectRef, FMethodPtr InFunc DELEGATE_COMMA_PAYLOAD_LIST )
	{
		return new SP_METHOD_DELEGATE_INSTANCE_CLASS< UserClass, FUNC_PAYLOAD_TEMPLATE_ARGS, SPMode >( InUserObjectRef, InFunc DELEGATE_COMMA_PAYLOAD_PASSTHRU );
	}

	
	/**
	 * Static helper function that returns a newly-created delegate object for
	 * the specified user class and class method.  Requires that the supplied object derives from TSharedFromThis.
	 *
	 * @param  InUserObject  The user's object that contains the class method.  Must derive from TSharedFromThis.
	 * @param  InFunc  Member function pointer to your class method
	 *
	 * @return  Returns an instance of the new delegate
	 */
	FORCEINLINE static DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS >* Create( UserClass* InUserObject, FMethodPtr InFunc DELEGATE_COMMA_PAYLOAD_LIST )
	{
		/* We expect the incoming InUserObject to derived from TSharedFromThis */
		TSharedRef< UserClass > UserObjectRef( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ) );
		return Create( UserObjectRef, InFunc DELEGATE_COMMA_PAYLOAD_PASSTHRU );
	}


	/**
	 * Constructor
	 *
	 * @param  InUserObject  A shared reference to an arbitrary object (templated) that hosts the member function
	 * @param  InMethodPtr  C++ member function pointer for the method to bind
	 */
	SP_METHOD_DELEGATE_INSTANCE_CLASS( const TSharedPtr< UserClass, SPMode >& InUserObject, FMethodPtr InMethodPtr DELEGATE_COMMA_PAYLOAD_LIST )
		: UserObject( InUserObject ),
		  MethodPtr( InMethodPtr )
		  DELEGATE_COMMA_PAYLOAD_INITIALIZER_LIST
	{
		// NOTE: Shared pointer delegates are allowed to have a null incoming object pointer.  Weak pointers can expire,
		//       an it is possible for a copy of a delegate instance to end up with a null pointer.
		checkSlow( MethodPtr != NULL );
	}


	/**
	 * Returns the type of delegate instance
	 *
	 * @return  Delegate instance type
	 */
	virtual EDelegateInstanceType::Type GetType() const OVERRIDE
	{
		return SPMode == ESPMode::ThreadSafe ? EDelegateInstanceType::ThreadSafeSharedPointerMethod : EDelegateInstanceType::SharedPointerMethod;
	}


	/**
	 * Creates a copy of the delegate instance
	 *
	 * @return	The newly created copy
	 */
	virtual DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS >* CreateCopy() OVERRIDE
	{
		return Create( UserObject.Pin(), MethodPtr DELEGATE_COMMA_PAYLOAD_PASSIN );
	}


	/**
	 * Returns true if this delegate points to exactly the same object and method as the specified delegate,
	 * even if the delegate objects themselves are different.  Also, the delegate types *must* be compatible.
	 *
	 * @param  InOtherDelegate
	 *
	 * @return  True if delegates match
	 */
	virtual bool IsSameFunction( const DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS >& InOtherDelegate ) const OVERRIDE
	{
		// NOTE: Payload data is not currently considered when comparing delegate instances.  See the comment in
		// multi-cast delegate's Remove() method for more information.

		if ( InOtherDelegate.GetType() == EDelegateInstanceType::SharedPointerMethod || 
			   InOtherDelegate.GetType() == EDelegateInstanceType::ThreadSafeSharedPointerMethod ||
				 InOtherDelegate.GetType() == EDelegateInstanceType::RawMethod ||
				 InOtherDelegate.GetType() == EDelegateInstanceType::UObjectMethod )
		{
			return GetRawMethodPtrInternal() == InOtherDelegate.GetRawMethodPtr() && UserObject.HasSameObject( InOtherDelegate.GetRawUserObject() );
		}

		return false;
	}


	/**
	 * Returns true if this delegate is bound to the specified UserObject,
	 *
	 * @param  InUserObject
	 *
	 * @return  True if delegate is bound to the specified UserObject
	 */
	virtual bool HasSameObject( const void* InUserObject ) const OVERRIDE
	{
		return UserObject.HasSameObject( InUserObject );
	}

	virtual const void* GetRawUserObject() const OVERRIDE
	{
		return GetRawUserObjectInternal();
	}

	virtual const void* GetRawMethodPtr() const OVERRIDE
	{
		return GetRawMethodPtrInternal();
	}

	/**
	 * Execute the delegate.  If the function pointer is not valid, an error will occur.
	 */
	virtual RetValType Execute( FUNC_PARAM_LIST ) const OVERRIDE
	{
		// Verify that the user object is still valid.  We only have a weak reference to it.
		TSharedPtr< UserClass, SPMode > SharedUserObject( UserObject.Pin() );
		checkSlow( SharedUserObject.IsValid() );

		// Safely remove const to work around a compiler issue with instantiating template permutations for 
		// overloaded functions that take a function pointer typedef as a member of a templated class.  In
		// all cases where this code is actually invoked, the UserClass will already be a const pointer.
		typedef typename TRemoveConst<UserClass>::Type MutableUserClass;
		MutableUserClass* MutableUserObject = const_cast<MutableUserClass*>( SharedUserObject.Get() );

		// Call the member function on the user's object.  And yes, this is the correct C++ syntax for calling a
		// pointer-to-member function.
		checkSlow( MethodPtr != NULL );

		return ( MutableUserObject->*MethodPtr )( DELEGATE_PARAM_PASSTHRU_COMMA_PAYLOAD_PASSIN );
	}


	/**
	 * Checks to see if the user object bound to this delegate is still valid
	 *
	 * @return  True if the object is still valid and it's safe to execute the function call
	 */
	virtual bool IsSafeToExecute() const OVERRIDE
	{
		return UserObject.IsValid();
	}


#if FUNC_IS_VOID
	/**
	 * Execute the delegate, but only if the function pointer is still valid
	 *
	 * @return  Returns true if the function was executed
	 */
	virtual bool ExecuteIfSafe( FUNC_PARAM_LIST ) const OVERRIDE
	{
		// Verify that the user object is still valid.  We only have a weak reference to it.
		TSharedPtr< UserClass, SPMode > SharedUserObject( UserObject.Pin() );
		if( SharedUserObject.IsValid() )
		{
			Execute( FUNC_PARAM_PASSTHRU );
			return true;
		}
		return false;
	}
#endif


private:

	// Declare ourselves as a friend so we can access other template permutations in IsSameFunction()
	template< class UserClassNoShadow, FUNC_PAYLOAD_TEMPLATE_DECL_NO_SHADOW, ESPMode::Type SPModeNoShadow > friend class SP_METHOD_DELEGATE_INSTANCE_CLASS;

	// Declare other pointer-based delegates as a friend so IsSameFunction() can compare members
	template< class UserClassNoShadow, FUNC_PAYLOAD_TEMPLATE_DECL_NO_SHADOW > friend class UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS;
	template< class UserClassNoShadow, FUNC_PAYLOAD_TEMPLATE_DECL_NO_SHADOW > friend class RAW_METHOD_DELEGATE_INSTANCE_CLASS;

	/** Weak reference to an instance of the user's class which contains a method we would like to call. */
	TWeakPtr< UserClass, SPMode > UserObject;

	/** C++ member function pointer */
	FMethodPtr MethodPtr;

	/** Payload member variables (if any) */
	FUNC_PAYLOAD_MEMBERS
};



/**
 * Delegate type implementation class for raw C++ methods.  For internal use only.
 */
template< class UserClass, FUNC_PAYLOAD_TEMPLATE_DECL_TYPENAME >
class RAW_METHOD_DELEGATE_INSTANCE_CLASS
	: public DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS >
{
	// Raw method delegates to UObjects are illegal.
	checkAtCompileTime((!CanConvertPointerFromTo<UserClass, UObjectBase>::Result), You_Cannot_Use_Raw_Method_Delegates_With_UObjects);

	/**
	 * Internal, inlined and non-virtual version of GetRawUserObject interface.
	 */
	FORCEINLINE const void* GetRawUserObjectInternal() const
	{
		return UserObject;
	}

	/**
	 * Internal, inlined and non-virtual version of GetRawMethodPtr interface.
	 */
	FORCEINLINE const void* GetRawMethodPtrInternal() const
	{
		return *(const void**)&MethodPtr;
	}

public:

	/** Pointer-to-member typedef */
	typedef RetValType ( UserClass::*FMethodPtr )( DELEGATE_PARAM_LIST_COMMA_PAYLOAD_LIST ) DELEGATE_QUALIFIER;


	/**
	 * Static helper function that returns a delegate object for the specified user class and class method
	 *
	 * @param  InUserObject  User's object that contains the class method
	 * @param  InFunc  Member function pointer to your class method
	 *
	 * @return  Returns an interface to the new delegate
	 */
	FORCEINLINE static DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS >* Create( UserClass* InUserObject, FMethodPtr InFunc DELEGATE_COMMA_PAYLOAD_LIST )
	{
		return new RAW_METHOD_DELEGATE_INSTANCE_CLASS< UserClass, FUNC_PAYLOAD_TEMPLATE_ARGS >( InUserObject, InFunc DELEGATE_COMMA_PAYLOAD_PASSTHRU );
	}


	/**
	 * Constructor
	 *
	 * @param  InUserObject  An arbitrary object (templated) that hosts the member function
	 * @param  InMethodPtr  C++ member function pointer for the method to bind
	 */
	RAW_METHOD_DELEGATE_INSTANCE_CLASS( UserClass* InUserObject, FMethodPtr InMethodPtr DELEGATE_COMMA_PAYLOAD_LIST )
		: UserObject( InUserObject ),
		  MethodPtr( InMethodPtr )
		  DELEGATE_COMMA_PAYLOAD_INITIALIZER_LIST
	{
		// Non-expirable delegates must always have a non-null object pointer on creation (otherwise they could never execute.)
		check( InUserObject != NULL && MethodPtr != NULL );
	}


	/**
	 * Returns the type of delegate instance
	 *
	 * @return  Delegate instance type
	 */
	virtual EDelegateInstanceType::Type GetType() const OVERRIDE
	{
		return EDelegateInstanceType::RawMethod;
	}


	/**
	 * Creates a copy of the delegate instance
	 *
	 * @return	The newly created copy
	 */
	virtual DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS >* CreateCopy() OVERRIDE
	{
		return Create( UserObject, MethodPtr DELEGATE_COMMA_PAYLOAD_PASSIN );
	}


	/**
	 * Returns true if this delegate points to exactly the same object and method as the specified delegate,
	 * even if the delegate objects themselves are different.  Also, the delegate types *must* be compatible.
	 *
	 * @param  InOtherDelegate
	 *
	 * @return  True if delegates match
	 */
	virtual bool IsSameFunction( const DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS >& InOtherDelegate ) const OVERRIDE
	{
		// NOTE: Payload data is not currently considered when comparing delegate instances.  See the comment in
		// multi-cast delegate's Remove() method for more information.

		if ( InOtherDelegate.GetType() == EDelegateInstanceType::RawMethod || 
			   InOtherDelegate.GetType() == EDelegateInstanceType::UObjectMethod ||
				 InOtherDelegate.GetType() == EDelegateInstanceType::SharedPointerMethod ||
				 InOtherDelegate.GetType() == EDelegateInstanceType::ThreadSafeSharedPointerMethod )
		{
			return GetRawMethodPtrInternal() == InOtherDelegate.GetRawMethodPtr() && UserObject == InOtherDelegate.GetRawUserObject();
		}

		return false;
	}


	/**
	 * Returns true if this delegate is bound to the specified UserObject,
	 *
	 * @param  InUserObject
	 *
	 * @return  True if delegate is bound to the specified UserObject
	 */
	virtual bool HasSameObject( const void* InUserObject ) const OVERRIDE
	{
		return UserObject == InUserObject;
	}

	virtual const void* GetRawUserObject() const OVERRIDE
	{
		return GetRawUserObjectInternal();
	}

	virtual const void* GetRawMethodPtr() const OVERRIDE
	{
		return GetRawMethodPtrInternal();
	}

	/**
	 * Execute the delegate.  If the function pointer is not valid, an error will occur.
	 */
	virtual RetValType Execute( FUNC_PARAM_LIST ) const OVERRIDE
	{
		// Safely remove const to work around a compiler issue with instantiating template permutations for 
		// overloaded functions that take a function pointer typedef as a member of a templated class.  In
		// all cases where this code is actually invoked, the UserClass will already be a const pointer.
		typedef typename TRemoveConst<UserClass>::Type MutableUserClass;
		MutableUserClass* MutableUserObject = const_cast<MutableUserClass*>( UserObject );

		// Call the member function on the user's object.  And yes, this is the correct C++ syntax for calling a
		// pointer-to-member function.
		checkSlow( MethodPtr != NULL );

		return ( MutableUserObject->*MethodPtr )( DELEGATE_PARAM_PASSTHRU_COMMA_PAYLOAD_PASSIN );
	}


	/**
	 * Checks to see if the user object bound to this delegate is still valid
	 *
	 * @return  True if the object is still valid and it's safe to execute the function call
	 */
	virtual bool IsSafeToExecute() const OVERRIDE
	{
		// We never know whether or not it is safe to deference a C++ pointer, but we have to
		// trust the user in this case.  Prefer using a shared-pointer based delegate type instead!
		return true;
	}


#if FUNC_IS_VOID
	/**
	 * Execute the delegate, but only if the function pointer is still valid
	 *
	 * @return  Returns true if the function was executed
	 */
	virtual bool ExecuteIfSafe( FUNC_PARAM_LIST ) const OVERRIDE
	{
		// We never know whether or not it is safe to deference a C++ pointer, but we have to
		// trust the user in this case.  Prefer using a shared-pointer based delegate type instead!
		Execute( FUNC_PARAM_PASSTHRU );
		return true;
	}
#endif


private:

	// Declare other pointer-based delegates as a friend so IsSameFunction() can compare members
	template< class UserClassNoShadow, FUNC_PAYLOAD_TEMPLATE_DECL_NO_SHADOW, ESPMode::Type SPModeNoShadow > friend class SP_METHOD_DELEGATE_INSTANCE_CLASS;
	template< class UserClassNoShadow, FUNC_PAYLOAD_TEMPLATE_DECL_NO_SHADOW > friend class UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS;

	/** Pointer to the user's class which contains a method we would like to call. */
	UserClass* UserObject;

	/** C++ member function pointer */
	FMethodPtr MethodPtr;

	/** Payload member variables (if any) */
	FUNC_PAYLOAD_MEMBERS
};



/**
 * Delegate type implementation class for UObject methods.  For internal use only.
 */
template< class UserClass, FUNC_PAYLOAD_TEMPLATE_DECL_TYPENAME >
class UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS
	: public DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS >
{
		// UObject delegates can't be used with non-UObject pointers.
	checkAtCompileTime((CanConvertPointerFromTo<UserClass, UObjectBase>::Result), You_Cannot_Use_UObject_Method_Delegates_With_Raw_Pointers);

	/**
	 * Internal, inlined and non-virtual version of GetRawUserObject interface.
	 */
	FORCEINLINE const void* GetRawUserObjectInternal() const
	{
		return UserObject.Get();
	}

	/**
	 * Internal, inlined and non-virtual version of GetRawMethodPtr interface.
	 */
	FORCEINLINE const void* GetRawMethodPtrInternal() const
	{
		return *(const void**)&MethodPtr;
	}

public:

	/** Pointer-to-member typedef */
	typedef RetValType ( UserClass::*FMethodPtr )( DELEGATE_PARAM_LIST_COMMA_PAYLOAD_LIST ) DELEGATE_QUALIFIER;


	/**
	 * Static helper function that returns a delegate object for the specified user class and class method
	 *
	 * @param  InUserObject  User's object that contains the class method
	 * @param  InFunc  Member function pointer to your class method
	 *
	 * @return  Returns an interface to the new delegate
	 */
	FORCEINLINE static DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS >* Create( UserClass* InUserObject, FMethodPtr InFunc DELEGATE_COMMA_PAYLOAD_LIST )
	{
		return new UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS< UserClass, FUNC_PAYLOAD_TEMPLATE_ARGS >( InUserObject, InFunc DELEGATE_COMMA_PAYLOAD_PASSTHRU );
	}


	/**
	 * Constructor
	 *
	 * @param  InUserObject  An arbitrary object (templated) that hosts the member function
	 * @param  InMethodPtr  C++ member function pointer for the method to bind
	 */
	UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS( UserClass* InUserObject, FMethodPtr InMethodPtr DELEGATE_COMMA_PAYLOAD_LIST )
		: UserObject( InUserObject ),
		  MethodPtr( InMethodPtr )
		  DELEGATE_COMMA_PAYLOAD_INITIALIZER_LIST
	{
		// NOTE: UObject delegates are allowed to have a null incoming object pointer.  UObject weak pointers can expire,
		//       an it is possible for a copy of a delegate instance to end up with a null pointer.
		checkSlow( MethodPtr != NULL );
	}


	/**
	 * Returns the type of delegate instance
	 *
	 * @return  Delegate instance type
	 */
	virtual EDelegateInstanceType::Type GetType() const OVERRIDE
	{
		return EDelegateInstanceType::UObjectMethod;
	}


	/**
	 * Creates a copy of the delegate instance
	 *
	 * @return	The newly created copy
	 */
	virtual DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS >* CreateCopy() OVERRIDE
	{
		return Create( UserObject.Get(), MethodPtr DELEGATE_COMMA_PAYLOAD_PASSIN );
	}


	/**
	 * Returns true if this delegate points to exactly the same object and method as the specified delegate,
	 * even if the delegate objects themselves are different.  Also, the delegate types *must* be compatible.
	 *
	 * @param  InOtherDelegate
	 *
	 * @return  True if delegates match
	 */
	virtual bool IsSameFunction( const DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS >& InOtherDelegate ) const OVERRIDE
	{
		// NOTE: Payload data is not currently considered when comparing delegate instances.  See the comment in
		// multi-cast delegate's Remove() method for more information.

		if ( InOtherDelegate.GetType() == EDelegateInstanceType::UObjectMethod || 
			   InOtherDelegate.GetType() == EDelegateInstanceType::RawMethod ||
				 InOtherDelegate.GetType() == EDelegateInstanceType::SharedPointerMethod ||
				 InOtherDelegate.GetType() == EDelegateInstanceType::ThreadSafeSharedPointerMethod )
		{
			return GetRawMethodPtrInternal() == InOtherDelegate.GetRawMethodPtr() && UserObject.Get() == InOtherDelegate.GetRawUserObject();
		}

		return false;
	}

	/**
	 * Returns true if this delegate is bound to the specified UserObject,
	 *
	 * @param  InUserObject
	 *
	 * @return  True if delegate is bound to the specified UserObject
	 */
	virtual bool HasSameObject( const void* InUserObject ) const OVERRIDE
	{
		return UserObject.Get() == InUserObject;
	}

	virtual const void* GetRawUserObject() const OVERRIDE
	{
		return GetRawUserObjectInternal();
	}

	virtual const void* GetRawMethodPtr() const OVERRIDE
	{
		return GetRawMethodPtrInternal();
	}

	/**
	 * Execute the delegate.  If the function pointer is not valid, an error will occur.
	 */
	virtual RetValType Execute( FUNC_PARAM_LIST ) const OVERRIDE
	{
		// Verify that the user object is still valid.  We only have a weak reference to it.
		checkSlow( UserObject.IsValid() );

		// Safely remove const to work around a compiler issue with instantiating template permutations for 
		// overloaded functions that take a function pointer typedef as a member of a templated class.  In
		// all cases where this code is actually invoked, the UserClass will already be a const pointer.
		typedef typename TRemoveConst<UserClass>::Type MutableUserClass;
		MutableUserClass* MutableUserObject = const_cast<MutableUserClass*>( UserObject.Get() );

		// Call the member function on the user's object.  And yes, this is the correct C++ syntax for calling a
		// pointer-to-member function.
		checkSlow( MethodPtr != NULL );

		return ( MutableUserObject->*MethodPtr )( DELEGATE_PARAM_PASSTHRU_COMMA_PAYLOAD_PASSIN );
	}


	/**
	 * Checks to see if the user object bound to this delegate is still valid
	 *
	 * @return  True if the object is still valid and it's safe to execute the function call
	 */
	virtual bool IsSafeToExecute() const OVERRIDE
	{
		return !!UserObject.Get();
	}

	virtual bool IsCompactable() const OVERRIDE
	{
		return !UserObject.Get(true);
	}

#if FUNC_IS_VOID
	/**
	 * Execute the delegate, but only if the function pointer is still valid
	 *
	 * @return  Returns true if the function was executed
	 */
	virtual bool ExecuteIfSafe( FUNC_PARAM_LIST ) const OVERRIDE
	{
		// Verify that the user object is still valid.  We only have a weak reference to it.
		UserClass* ActualUserObject = UserObject.Get();
		if( ActualUserObject != NULL )
		{
			Execute( FUNC_PARAM_PASSTHRU );
			return true;
		}
		return false;
	}
#endif


private:

	// Declare other pointer-based delegates as a friend so IsSameFunction() can compare members
	template< class UserClassNoShadow, FUNC_PAYLOAD_TEMPLATE_DECL_NO_SHADOW, ESPMode::Type SPModeNoShadow > friend class SP_METHOD_DELEGATE_INSTANCE_CLASS;
	template< class UserClassNoShadow, FUNC_PAYLOAD_TEMPLATE_DECL_NO_SHADOW > friend class RAW_METHOD_DELEGATE_INSTANCE_CLASS;

	/** Pointer to the user's class which contains a method we would like to call. */
	TWeakObjectPtr< UserClass > UserObject;

	/** C++ member function pointer */
	FMethodPtr MethodPtr;

	/** Payload member variables (if any) */
	FUNC_PAYLOAD_MEMBERS
};



/**
 * Delegate type implementation class for global functions.  For internal use only.
 */
template< FUNC_PAYLOAD_TEMPLATE_DECL_TYPENAME >
class STATIC_DELEGATE_INSTANCE_CLASS
	: public DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS >
{

public:

	/** Static function pointer typedef */
	// NOTE: Static C++ functions can never be const, so 'const' is always omitted here.  That means there is
	//       usually an extra static delegate class created with a 'const' name, even though it doesn't use const.
	typedef RetValType ( *FFuncPtr )( DELEGATE_PARAM_LIST_COMMA_PAYLOAD_LIST );


	/**
	 * Static helper function that returns a newly-created delegate object for a
	 * static/global function pointer.
	 *
	 * @param  InFunc  Static function pointer
	 *
	 * @return  Returns a new instance of the delegate
	 */
	FORCEINLINE static DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS >* Create( FFuncPtr InFunc DELEGATE_COMMA_PAYLOAD_LIST )
	{
		return new STATIC_DELEGATE_INSTANCE_CLASS< FUNC_PAYLOAD_TEMPLATE_ARGS >( InFunc DELEGATE_COMMA_PAYLOAD_PASSTHRU );
	}


	/**
	 * Constructor
	 *
	 * @param  InStaticFuncPtr  C++ function pointer
	 */
	STATIC_DELEGATE_INSTANCE_CLASS( FFuncPtr InStaticFuncPtr DELEGATE_COMMA_PAYLOAD_LIST )
		: StaticFuncPtr( InStaticFuncPtr )
		  DELEGATE_COMMA_PAYLOAD_INITIALIZER_LIST
	{
		check( StaticFuncPtr != NULL );
	}


	/**
	 * Returns the type of delegate instance
	 *
	 * @return  Delegate instance type
	 */
	virtual EDelegateInstanceType::Type GetType() const OVERRIDE
	{
		return EDelegateInstanceType::Raw;
	}


	/**
	 * Creates a copy of the delegate instance
	 *
	 * @return	The newly created copy
	 */
	virtual DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS >* CreateCopy() OVERRIDE
	{
		return Create( StaticFuncPtr DELEGATE_COMMA_PAYLOAD_PASSIN );
	}


	/**
	 * Returns true if this delegate points to exactly the same object and method as the specified delegate,
	 * even if the delegate objects themselves are different.  However, the delegate types *must* match.
	 *
	 * @param  InOtherDelegate
	 *
	 * @return  True if delegates match
	 */
	virtual bool IsSameFunction( const DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS >& InOtherDelegate ) const OVERRIDE
	{
		// NOTE: Payload data is not currently considered when comparing delegate instances.  See the comment in
		// multi-cast delegate's Remove() method for more information.

		if( InOtherDelegate.GetType() == EDelegateInstanceType::Raw )
		{
			// Downcast to our delegate type and compare
			const STATIC_DELEGATE_INSTANCE_CLASS< FUNC_PAYLOAD_TEMPLATE_ARGS >& OtherStaticDelegate =
				static_cast< const STATIC_DELEGATE_INSTANCE_CLASS< FUNC_PAYLOAD_TEMPLATE_ARGS >& >( InOtherDelegate );
			if( StaticFuncPtr == OtherStaticDelegate.StaticFuncPtr )
			{
				return true;
			}
		}

		return false;
	}


	/**
	 * Returns true if this delegate is bound to the specified UserObject,
	 *
	 * @param  InUserObject
	 *
	 * @return  True if delegate is bound to the specified UserObject
	 */
	virtual bool HasSameObject( const void* UserObject ) const OVERRIDE
	{
		// Raw Delegates aren't bound to an object so they can never match
		return false;
	}

	virtual const void* GetRawUserObject() const OVERRIDE
	{
		return NULL;
	}

	virtual const void* GetRawMethodPtr() const OVERRIDE
	{
		return *(const void**)&StaticFuncPtr;
	}

	/**
	 * Checks to see if the user object bound to this delegate is still valid
	 *
	 * @return  True if the object is still valid and it's safe to execute the function call
	 */
	virtual bool IsSafeToExecute() const OVERRIDE
	{
		// Static functions are always safe to execute!
		return true;
	}


	/**
	 * Execute the delegate.  If the function pointer is not valid, an error will occur.
	 */
	virtual RetValType Execute( FUNC_PARAM_LIST ) const OVERRIDE
	{
		// Call the member function on the user's object.  And yes, this is the correct C++ syntax for calling a
		// pointer-to-member function.
		checkSlow( StaticFuncPtr != NULL );

		return StaticFuncPtr( DELEGATE_PARAM_PASSTHRU_COMMA_PAYLOAD_PASSIN );
	}


#if FUNC_IS_VOID
	/**
	 * Execute the delegate, but only if the function pointer is still valid
	 *
	 * @return  Returns true if the function was executed
	 */
	virtual bool ExecuteIfSafe( FUNC_PARAM_LIST ) const OVERRIDE
	{
		Execute( FUNC_PARAM_PASSTHRU );
		return true;
	}
#endif


private:

	/** C++ function pointer */
	FFuncPtr StaticFuncPtr;

	/** Payload member variables (if any) */
	FUNC_PAYLOAD_MEMBERS
};



#undef SP_METHOD_DELEGATE_INSTANCE_CLASS
#undef RAW_METHOD_DELEGATE_INSTANCE_CLASS
#undef UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS
#undef STATIC_DELEGATE_INSTANCE_CLASS
#undef DELEGATE_INSTANCE_INTERFACE_CLASS
#undef DELEGATE_QUALIFIER
#undef DELEGATE_COMMA_PAYLOAD_LIST
#undef DELEGATE_COMMA_PAYLOAD_PASSTHRU
#undef DELEGATE_COMMA_PAYLOAD_INITIALIZER_LIST
#undef DELEGATE_COMMA_PAYLOAD_PASSIN
#undef DELEGATE_PARAM_PASSTHRU_COMMA_PAYLOAD_PASSIN
#undef DELEGATE_PARAM_LIST_COMMA_PAYLOAD_LIST
