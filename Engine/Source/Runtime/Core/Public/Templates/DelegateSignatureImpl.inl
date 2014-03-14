// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

// Only designed to be included directly by Delegate.h
#if !defined( __Delegate_h__ ) || !defined( FUNC_INCLUDING_INLINE_IMPL )
	#error "This inline header must only be included by Delegate.h"
#endif


// NOTE: This file in re-included for EVERY delegate signature that we support.  That is, every combination
//		 of parameter count, return value presence or other function modifier will include this file to
//		 generate a delegate interface type and implementation type for that signature.


#define DELEGATE_INSTANCE_INTERFACE_CLASS FUNC_COMBINE( IBaseDelegateInstance_, FUNC_SUFFIX )

#define SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars FUNC_COMBINE( TBaseSPMethodDelegateInstance_, FUNC_SUFFIX )
#define SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars_Const FUNC_COMBINE( SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars, _Const )
#define SP_METHOD_DELEGATE_INSTANCE_CLASS_OneVar FUNC_COMBINE( SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars, _OneVar )
#define SP_METHOD_DELEGATE_INSTANCE_CLASS_OneVar_Const FUNC_COMBINE( SP_METHOD_DELEGATE_INSTANCE_CLASS_OneVar, _Const )
#define SP_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars FUNC_COMBINE( SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars, _TwoVars )
#define SP_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars_Const FUNC_COMBINE( SP_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars, _Const )
#define SP_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars FUNC_COMBINE( SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars, _ThreeVars )
#define SP_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars_Const FUNC_COMBINE( SP_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars, _Const )
#define SP_METHOD_DELEGATE_INSTANCE_CLASS_FourVars FUNC_COMBINE( SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars, _FourVars )
#define SP_METHOD_DELEGATE_INSTANCE_CLASS_FourVars_Const FUNC_COMBINE( SP_METHOD_DELEGATE_INSTANCE_CLASS_FourVars, _Const )

#define RAW_METHOD_DELEGATE_INSTANCE_CLASS_NoVars FUNC_COMBINE( TBaseRawMethodDelegateInstance_, FUNC_SUFFIX )
#define RAW_METHOD_DELEGATE_INSTANCE_CLASS_NoVars_Const FUNC_COMBINE( RAW_METHOD_DELEGATE_INSTANCE_CLASS_NoVars, _Const )
#define RAW_METHOD_DELEGATE_INSTANCE_CLASS_OneVar FUNC_COMBINE( RAW_METHOD_DELEGATE_INSTANCE_CLASS_NoVars, _OneVar )
#define RAW_METHOD_DELEGATE_INSTANCE_CLASS_OneVar_Const FUNC_COMBINE( RAW_METHOD_DELEGATE_INSTANCE_CLASS_OneVar, _Const )
#define RAW_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars FUNC_COMBINE( RAW_METHOD_DELEGATE_INSTANCE_CLASS_NoVars, _TwoVars )
#define RAW_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars_Const FUNC_COMBINE( RAW_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars, _Const )
#define RAW_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars FUNC_COMBINE( RAW_METHOD_DELEGATE_INSTANCE_CLASS_NoVars, _ThreeVars )
#define RAW_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars_Const FUNC_COMBINE( RAW_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars, _Const )
#define RAW_METHOD_DELEGATE_INSTANCE_CLASS_FourVars FUNC_COMBINE( RAW_METHOD_DELEGATE_INSTANCE_CLASS_NoVars, _FourVars )
#define RAW_METHOD_DELEGATE_INSTANCE_CLASS_FourVars_Const FUNC_COMBINE( RAW_METHOD_DELEGATE_INSTANCE_CLASS_FourVars, _Const )

#define UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_NoVars FUNC_COMBINE( TBaseUObjectMethodDelegateInstance_, FUNC_SUFFIX )
#define UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_NoVars_Const FUNC_COMBINE( UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_NoVars, _Const )
#define UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_OneVar FUNC_COMBINE( UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_NoVars, _OneVar )
#define UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_OneVar_Const FUNC_COMBINE( UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_OneVar, _Const )
#define UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars FUNC_COMBINE( UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_NoVars, _TwoVars )
#define UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars_Const FUNC_COMBINE( UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars, _Const )
#define UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars FUNC_COMBINE( UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_NoVars, _ThreeVars )
#define UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars_Const FUNC_COMBINE( UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars, _Const )
#define UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_FourVars FUNC_COMBINE( UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_NoVars, _FourVars )
#define UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_FourVars_Const FUNC_COMBINE( UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_FourVars, _Const )

#define STATIC_DELEGATE_INSTANCE_CLASS_NoVars FUNC_COMBINE( TBaseStaticDelegateInstance_, FUNC_SUFFIX )
#define STATIC_DELEGATE_INSTANCE_CLASS_NoVars_Const FUNC_COMBINE( STATIC_DELEGATE_INSTANCE_CLASS_NoVars, _Const )
#define STATIC_DELEGATE_INSTANCE_CLASS_OneVar FUNC_COMBINE( STATIC_DELEGATE_INSTANCE_CLASS_NoVars, _OneVar )
#define STATIC_DELEGATE_INSTANCE_CLASS_OneVar_Const FUNC_COMBINE( STATIC_DELEGATE_INSTANCE_CLASS_OneVar, _Const )
#define STATIC_DELEGATE_INSTANCE_CLASS_TwoVars FUNC_COMBINE( STATIC_DELEGATE_INSTANCE_CLASS_NoVars, _TwoVars )
#define STATIC_DELEGATE_INSTANCE_CLASS_TwoVars_Const FUNC_COMBINE( STATIC_DELEGATE_INSTANCE_CLASS_TwoVars, _Const )
#define STATIC_DELEGATE_INSTANCE_CLASS_ThreeVars FUNC_COMBINE( STATIC_DELEGATE_INSTANCE_CLASS_NoVars, _ThreeVars )
#define STATIC_DELEGATE_INSTANCE_CLASS_ThreeVars_Const FUNC_COMBINE( STATIC_DELEGATE_INSTANCE_CLASS_ThreeVars, _Const )
#define STATIC_DELEGATE_INSTANCE_CLASS_FourVars FUNC_COMBINE( STATIC_DELEGATE_INSTANCE_CLASS_NoVars, _FourVars )
#define STATIC_DELEGATE_INSTANCE_CLASS_FourVars_Const FUNC_COMBINE( STATIC_DELEGATE_INSTANCE_CLASS_FourVars, _Const )

#define DELEGATE_CLASS FUNC_COMBINE( TBaseDelegate_, FUNC_SUFFIX )
#define BASE_MULTICAST_DELEGATE_CLASS FUNC_COMBINE( TBaseMulticastDelegate_, FUNC_SUFFIX )
#define MULTICAST_DELEGATE_CLASS FUNC_COMBINE( TMulticastDelegate_, FUNC_SUFFIX )
#define EVENT_CLASS FUNC_COMBINE( TEvent_, FUNC_SUFFIX )
#define DYNAMIC_DELEGATE_CLASS FUNC_COMBINE( TBaseDynamicDelegate_, FUNC_SUFFIX )
#define DYNAMIC_MULTICAST_DELEGATE_CLASS FUNC_COMBINE( TBaseDynamicMulticastDelegate_, FUNC_SUFFIX )

#define PAYLOAD_TEMPLATE_DECL_OneVar typename Var1Type
#define PAYLOAD_TEMPLATE_LIST_OneVar Var1Type
#define PAYLOAD_TEMPLATE_ARGS_OneVar Var1Type Var1
#define PAYLOAD_TEMPLATE_PASSIN_OneVar Var1

#define PAYLOAD_TEMPLATE_DECL_TwoVars PAYLOAD_TEMPLATE_DECL_OneVar, typename Var2Type
#define PAYLOAD_TEMPLATE_LIST_TwoVars PAYLOAD_TEMPLATE_LIST_OneVar, Var2Type
#define PAYLOAD_TEMPLATE_ARGS_TwoVars PAYLOAD_TEMPLATE_ARGS_OneVar, Var2Type Var2
#define PAYLOAD_TEMPLATE_PASSIN_TwoVars PAYLOAD_TEMPLATE_PASSIN_OneVar, Var2

#define PAYLOAD_TEMPLATE_DECL_ThreeVars PAYLOAD_TEMPLATE_DECL_TwoVars, typename Var3Type
#define PAYLOAD_TEMPLATE_LIST_ThreeVars PAYLOAD_TEMPLATE_LIST_TwoVars, Var3Type
#define PAYLOAD_TEMPLATE_ARGS_ThreeVars PAYLOAD_TEMPLATE_ARGS_TwoVars, Var3Type Var3
#define PAYLOAD_TEMPLATE_PASSIN_ThreeVars PAYLOAD_TEMPLATE_PASSIN_TwoVars, Var3

#define PAYLOAD_TEMPLATE_DECL_FourVars PAYLOAD_TEMPLATE_DECL_ThreeVars, typename Var4Type
#define PAYLOAD_TEMPLATE_LIST_FourVars PAYLOAD_TEMPLATE_LIST_ThreeVars, Var4Type
#define PAYLOAD_TEMPLATE_ARGS_FourVars PAYLOAD_TEMPLATE_ARGS_ThreeVars, Var4Type Var4
#define PAYLOAD_TEMPLATE_PASSIN_FourVars PAYLOAD_TEMPLATE_PASSIN_ThreeVars, Var4



/**
 * Single-cast delegate base object.   You'll use the various DECLARE_DELEGATE macros to create the
 * actual delegate type, templated to the function signature the delegate is compatible with.  Then,
 * you can create an instance of that class when you want to bind a function to the delegate.
 */
template< FUNC_TEMPLATE_DECL_TYPENAME >
class DELEGATE_CLASS
{

public:

	/** Declare the user's delegate interface (e.g. IMyDelegate) */
	typedef DELEGATE_INSTANCE_INTERFACE_CLASS< FUNC_TEMPLATE_ARGS > IDelegate;


	/** Declare the user's "fast" shared pointer-based delegate object (e.g. TSPMyDelegate) */
	template< class UserClass                                  > class TSPMethodDelegate                 : public SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars         < UserClass, FUNC_TEMPLATE_ARGS,                                  ESPMode::Fast > { public: TSPMethodDelegate                ( const TSharedRef< UserClass, ESPMode::Fast >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars         < UserClass, FUNC_TEMPLATE_ARGS,                                  ESPMode::Fast >::FMethodPtr InMethodPtr                                  ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars         < UserClass, FUNC_TEMPLATE_ARGS,                                  ESPMode::Fast >( InUserObject, InMethodPtr                                    ) {} };
	template< class UserClass                                  > class TSPMethodDelegate_Const           : public SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars_Const   < UserClass, FUNC_TEMPLATE_ARGS,                                  ESPMode::Fast > { public: TSPMethodDelegate_Const          ( const TSharedRef< UserClass, ESPMode::Fast >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars_Const   < UserClass, FUNC_TEMPLATE_ARGS,                                  ESPMode::Fast >::FMethodPtr InMethodPtr                                  ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars_Const   < UserClass, FUNC_TEMPLATE_ARGS,                                  ESPMode::Fast >( InUserObject, InMethodPtr                                    ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar    > class TSPMethodDelegate_OneVar          : public SP_METHOD_DELEGATE_INSTANCE_CLASS_OneVar         < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar,    ESPMode::Fast > { public: TSPMethodDelegate_OneVar         ( const TSharedRef< UserClass, ESPMode::Fast >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_OneVar         < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar,    ESPMode::Fast >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_OneVar    ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_OneVar         < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar,    ESPMode::Fast >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_OneVar    ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar    > class TSPMethodDelegate_OneVar_Const    : public SP_METHOD_DELEGATE_INSTANCE_CLASS_OneVar_Const   < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar,    ESPMode::Fast > { public: TSPMethodDelegate_OneVar_Const   ( const TSharedRef< UserClass, ESPMode::Fast >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_OneVar_Const   < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar,    ESPMode::Fast >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_OneVar    ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_OneVar_Const   < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar,    ESPMode::Fast >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_OneVar    ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars   > class TSPMethodDelegate_TwoVars         : public SP_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars        < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars,   ESPMode::Fast > { public: TSPMethodDelegate_TwoVars        ( const TSharedRef< UserClass, ESPMode::Fast >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars        < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars,   ESPMode::Fast >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_TwoVars   ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars        < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars,   ESPMode::Fast >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_TwoVars   ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars   > class TSPMethodDelegate_TwoVars_Const   : public SP_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars_Const  < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars,   ESPMode::Fast > { public: TSPMethodDelegate_TwoVars_Const  ( const TSharedRef< UserClass, ESPMode::Fast >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars_Const  < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars,   ESPMode::Fast >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_TwoVars   ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars_Const  < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars,   ESPMode::Fast >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_TwoVars   ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars > class TSPMethodDelegate_ThreeVars       : public SP_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars      < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars, ESPMode::Fast > { public: TSPMethodDelegate_ThreeVars      ( const TSharedRef< UserClass, ESPMode::Fast >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars      < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars, ESPMode::Fast >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_ThreeVars ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars      < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars, ESPMode::Fast >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars > class TSPMethodDelegate_ThreeVars_Const : public SP_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars_Const< UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars, ESPMode::Fast > { public: TSPMethodDelegate_ThreeVars_Const( const TSharedRef< UserClass, ESPMode::Fast >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars_Const< UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars, ESPMode::Fast >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_ThreeVars ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars_Const< UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars, ESPMode::Fast >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars  > class TSPMethodDelegate_FourVars        : public SP_METHOD_DELEGATE_INSTANCE_CLASS_FourVars       < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars,  ESPMode::Fast > { public: TSPMethodDelegate_FourVars       ( const TSharedRef< UserClass, ESPMode::Fast >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_FourVars       < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars,  ESPMode::Fast >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_FourVars  ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_FourVars       < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars,  ESPMode::Fast >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_FourVars  ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars  > class TSPMethodDelegate_FourVars_Const  : public SP_METHOD_DELEGATE_INSTANCE_CLASS_FourVars_Const < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars,  ESPMode::Fast > { public: TSPMethodDelegate_FourVars_Const ( const TSharedRef< UserClass, ESPMode::Fast >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_FourVars_Const < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars,  ESPMode::Fast >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_FourVars  ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_FourVars_Const < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars,  ESPMode::Fast >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_FourVars  ) {} };

	/** Declare the user's "thread-safe" shared pointer-based delegate object (e.g. TThreadSafeSPMyDelegate) */
	template< class UserClass                                  > class TThreadSafeSPMethodDelegate                 : public SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars         < UserClass, FUNC_TEMPLATE_ARGS,                                  ESPMode::ThreadSafe > { public: TThreadSafeSPMethodDelegate                ( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars         < UserClass, FUNC_TEMPLATE_ARGS,                                  ESPMode::ThreadSafe >::FMethodPtr InMethodPtr                                  ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars         < UserClass, FUNC_TEMPLATE_ARGS,                                  ESPMode::ThreadSafe >( InUserObject, InMethodPtr                                    ) {} };
	template< class UserClass                                  > class TThreadSafeSPMethodDelegate_Const           : public SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars_Const   < UserClass, FUNC_TEMPLATE_ARGS,                                  ESPMode::ThreadSafe > { public: TThreadSafeSPMethodDelegate_Const          ( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars_Const   < UserClass, FUNC_TEMPLATE_ARGS,                                  ESPMode::ThreadSafe >::FMethodPtr InMethodPtr                                  ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars_Const   < UserClass, FUNC_TEMPLATE_ARGS,                                  ESPMode::ThreadSafe >( InUserObject, InMethodPtr                                    ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar    > class TThreadSafeSPMethodDelegate_OneVar          : public SP_METHOD_DELEGATE_INSTANCE_CLASS_OneVar         < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar,    ESPMode::ThreadSafe > { public: TThreadSafeSPMethodDelegate_OneVar         ( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_OneVar         < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar,    ESPMode::ThreadSafe >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_OneVar    ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_OneVar         < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar,    ESPMode::ThreadSafe >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_OneVar    ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar    > class TThreadSafeSPMethodDelegate_OneVar_Const    : public SP_METHOD_DELEGATE_INSTANCE_CLASS_OneVar_Const   < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar,    ESPMode::ThreadSafe > { public: TThreadSafeSPMethodDelegate_OneVar_Const   ( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_OneVar_Const   < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar,    ESPMode::ThreadSafe >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_OneVar    ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_OneVar_Const   < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar,    ESPMode::ThreadSafe >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_OneVar    ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars   > class TThreadSafeSPMethodDelegate_TwoVars         : public SP_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars        < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars,   ESPMode::ThreadSafe > { public: TThreadSafeSPMethodDelegate_TwoVars        ( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars        < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars,   ESPMode::ThreadSafe >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_TwoVars   ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars        < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars,   ESPMode::ThreadSafe >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_TwoVars   ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars   > class TThreadSafeSPMethodDelegate_TwoVars_Const   : public SP_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars_Const  < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars,   ESPMode::ThreadSafe > { public: TThreadSafeSPMethodDelegate_TwoVars_Const  ( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars_Const  < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars,   ESPMode::ThreadSafe >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_TwoVars   ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars_Const  < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars,   ESPMode::ThreadSafe >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_TwoVars   ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars > class TThreadSafeSPMethodDelegate_ThreeVars       : public SP_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars      < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars, ESPMode::ThreadSafe > { public: TThreadSafeSPMethodDelegate_ThreeVars      ( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars      < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars, ESPMode::ThreadSafe >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_ThreeVars ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars      < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars, ESPMode::ThreadSafe >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars > class TThreadSafeSPMethodDelegate_ThreeVars_Const : public SP_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars_Const< UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars, ESPMode::ThreadSafe > { public: TThreadSafeSPMethodDelegate_ThreeVars_Const( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars_Const< UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars, ESPMode::ThreadSafe >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_ThreeVars ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars_Const< UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars, ESPMode::ThreadSafe >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars  > class TThreadSafeSPMethodDelegate_FourVars        : public SP_METHOD_DELEGATE_INSTANCE_CLASS_FourVars       < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars,  ESPMode::ThreadSafe > { public: TThreadSafeSPMethodDelegate_FourVars       ( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_FourVars       < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars,  ESPMode::ThreadSafe >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_FourVars  ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_FourVars       < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars,  ESPMode::ThreadSafe >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_FourVars  ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars  > class TThreadSafeSPMethodDelegate_FourVars_Const  : public SP_METHOD_DELEGATE_INSTANCE_CLASS_FourVars_Const < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars,  ESPMode::ThreadSafe > { public: TThreadSafeSPMethodDelegate_FourVars_Const ( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObject, typename SP_METHOD_DELEGATE_INSTANCE_CLASS_FourVars_Const < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars,  ESPMode::ThreadSafe >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_FourVars  ) : SP_METHOD_DELEGATE_INSTANCE_CLASS_FourVars_Const < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars,  ESPMode::ThreadSafe >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_FourVars  ) {} };

	/** Declare the user's C++ pointer-based delegate object (e.g. TRawMyDelegate) */
	template< class UserClass                                  > class TRawMethodDelegate                 : public RAW_METHOD_DELEGATE_INSTANCE_CLASS_NoVars         < UserClass, FUNC_TEMPLATE_ARGS                                  > { public: TRawMethodDelegate                ( UserClass* InUserObject, typename RAW_METHOD_DELEGATE_INSTANCE_CLASS_NoVars         < UserClass, FUNC_TEMPLATE_ARGS                                  >::FMethodPtr InMethodPtr                                  ) : RAW_METHOD_DELEGATE_INSTANCE_CLASS_NoVars         < UserClass, FUNC_TEMPLATE_ARGS                                  >( InUserObject, InMethodPtr                                    ) {} };
	template< class UserClass                                  > class TRawMethodDelegate_Const           : public RAW_METHOD_DELEGATE_INSTANCE_CLASS_NoVars_Const   < UserClass, FUNC_TEMPLATE_ARGS                                  > { public: TRawMethodDelegate_Const          ( UserClass* InUserObject, typename RAW_METHOD_DELEGATE_INSTANCE_CLASS_NoVars_Const   < UserClass, FUNC_TEMPLATE_ARGS                                  >::FMethodPtr InMethodPtr                                  ) : RAW_METHOD_DELEGATE_INSTANCE_CLASS_NoVars_Const   < UserClass, FUNC_TEMPLATE_ARGS                                  >( InUserObject, InMethodPtr                                    ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar    > class TRawMethodDelegate_OneVar          : public RAW_METHOD_DELEGATE_INSTANCE_CLASS_OneVar         < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar    > { public: TRawMethodDelegate_OneVar         ( UserClass* InUserObject, typename RAW_METHOD_DELEGATE_INSTANCE_CLASS_OneVar         < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar    >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_OneVar    ) : RAW_METHOD_DELEGATE_INSTANCE_CLASS_OneVar         < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar    >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_OneVar    ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar    > class TRawMethodDelegate_OneVar_Const    : public RAW_METHOD_DELEGATE_INSTANCE_CLASS_OneVar_Const   < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar    > { public: TRawMethodDelegate_OneVar_Const   ( UserClass* InUserObject, typename RAW_METHOD_DELEGATE_INSTANCE_CLASS_OneVar_Const   < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar    >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_OneVar    ) : RAW_METHOD_DELEGATE_INSTANCE_CLASS_OneVar_Const   < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar    >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_OneVar    ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars   > class TRawMethodDelegate_TwoVars         : public RAW_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars        < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars   > { public: TRawMethodDelegate_TwoVars        ( UserClass* InUserObject, typename RAW_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars        < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars   >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_TwoVars   ) : RAW_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars        < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars   >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_TwoVars   ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars   > class TRawMethodDelegate_TwoVars_Const   : public RAW_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars_Const  < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars   > { public: TRawMethodDelegate_TwoVars_Const  ( UserClass* InUserObject, typename RAW_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars_Const  < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars   >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_TwoVars   ) : RAW_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars_Const  < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars   >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_TwoVars   ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars > class TRawMethodDelegate_ThreeVars       : public RAW_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars      < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars > { public: TRawMethodDelegate_ThreeVars      ( UserClass* InUserObject, typename RAW_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars      < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_ThreeVars ) : RAW_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars      < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars > class TRawMethodDelegate_ThreeVars_Const : public RAW_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars_Const< UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars > { public: TRawMethodDelegate_ThreeVars_Const( UserClass* InUserObject, typename RAW_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars_Const< UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_ThreeVars ) : RAW_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars_Const< UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars  > class TRawMethodDelegate_FourVars        : public RAW_METHOD_DELEGATE_INSTANCE_CLASS_FourVars       < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars  > { public: TRawMethodDelegate_FourVars       ( UserClass* InUserObject, typename RAW_METHOD_DELEGATE_INSTANCE_CLASS_FourVars       < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars  >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_FourVars  ) : RAW_METHOD_DELEGATE_INSTANCE_CLASS_FourVars       < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars  >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_FourVars  ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars  > class TRawMethodDelegate_FourVars_Const  : public RAW_METHOD_DELEGATE_INSTANCE_CLASS_FourVars_Const < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars  > { public: TRawMethodDelegate_FourVars_Const ( UserClass* InUserObject, typename RAW_METHOD_DELEGATE_INSTANCE_CLASS_FourVars_Const < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars  >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_FourVars  ) : RAW_METHOD_DELEGATE_INSTANCE_CLASS_FourVars_Const < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars  >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_FourVars  ) {} };
	
	/** Declare the user's UObject-based delegate object (e.g. TObjMyDelegate) */
	template< class UserClass                                  > class TUObjectMethodDelegate                 : public UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_NoVars         < UserClass, FUNC_TEMPLATE_ARGS                                  > { public: TUObjectMethodDelegate                ( UserClass* InUserObject, typename UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_NoVars         < UserClass, FUNC_TEMPLATE_ARGS                                  >::FMethodPtr InMethodPtr                                  ) : UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_NoVars         < UserClass, FUNC_TEMPLATE_ARGS                                  >( InUserObject, InMethodPtr                                    ) {} };
	template< class UserClass                                  > class TUObjectMethodDelegate_Const           : public UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_NoVars_Const   < UserClass, FUNC_TEMPLATE_ARGS                                  > { public: TUObjectMethodDelegate_Const          ( UserClass* InUserObject, typename UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_NoVars_Const   < UserClass, FUNC_TEMPLATE_ARGS                                  >::FMethodPtr InMethodPtr                                  ) : UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_NoVars_Const   < UserClass, FUNC_TEMPLATE_ARGS                                  >( InUserObject, InMethodPtr                                    ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar    > class TUObjectMethodDelegate_OneVar          : public UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_OneVar         < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar    > { public: TUObjectMethodDelegate_OneVar         ( UserClass* InUserObject, typename UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_OneVar         < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar    >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_OneVar    ) : UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_OneVar         < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar    >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_OneVar    ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar    > class TUObjectMethodDelegate_OneVar_Const    : public UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_OneVar_Const   < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar    > { public: TUObjectMethodDelegate_OneVar_Const   ( UserClass* InUserObject, typename UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_OneVar_Const   < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar    >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_OneVar    ) : UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_OneVar_Const   < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar    >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_OneVar    ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars   > class TUObjectMethodDelegate_TwoVars         : public UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars        < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars   > { public: TUObjectMethodDelegate_TwoVars        ( UserClass* InUserObject, typename UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars        < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars   >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_TwoVars   ) : UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars        < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars   >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_TwoVars   ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars   > class TUObjectMethodDelegate_TwoVars_Const   : public UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars_Const  < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars   > { public: TUObjectMethodDelegate_TwoVars_Const  ( UserClass* InUserObject, typename UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars_Const  < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars   >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_TwoVars   ) : UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars_Const  < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars   >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_TwoVars   ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars > class TUObjectMethodDelegate_ThreeVars       : public UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars      < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars > { public: TUObjectMethodDelegate_ThreeVars      ( UserClass* InUserObject, typename UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars      < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_ThreeVars ) : UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars      < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars > class TUObjectMethodDelegate_ThreeVars_Const : public UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars_Const< UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars > { public: TUObjectMethodDelegate_ThreeVars_Const( UserClass* InUserObject, typename UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars_Const< UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_ThreeVars ) : UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars_Const< UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars  > class TUObjectMethodDelegate_FourVars        : public UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_FourVars       < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars  > { public: TUObjectMethodDelegate_FourVars       ( UserClass* InUserObject, typename UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_FourVars       < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars  >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_FourVars  ) : UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_FourVars       < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars  >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_FourVars  ) {} };
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars  > class TUObjectMethodDelegate_FourVars_Const  : public UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_FourVars_Const < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars  > { public: TUObjectMethodDelegate_FourVars_Const ( UserClass* InUserObject, typename UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_FourVars_Const < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars  >::FMethodPtr InMethodPtr, PAYLOAD_TEMPLATE_ARGS_FourVars  ) : UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_FourVars_Const < UserClass, FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars  >( InUserObject, InMethodPtr, PAYLOAD_TEMPLATE_PASSIN_FourVars  ) {} };
	
	/** Declare the user's static function pointer delegate object (e.g. FMyDelegate) */
	typedef STATIC_DELEGATE_INSTANCE_CLASS_NoVars< FUNC_TEMPLATE_ARGS > FStaticDelegate;
	template< PAYLOAD_TEMPLATE_DECL_OneVar    > class TStaticDelegate_OneVar    : public STATIC_DELEGATE_INSTANCE_CLASS_OneVar   < FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar    > { public: TStaticDelegate_OneVar   ( typename STATIC_DELEGATE_INSTANCE_CLASS_OneVar   < FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar    >::FFuncPtr InFuncPtr, PAYLOAD_TEMPLATE_ARGS_OneVar    ) : STATIC_DELEGATE_INSTANCE_CLASS_OneVar   < FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_OneVar    >( InFuncPtr, PAYLOAD_TEMPLATE_PASSIN_OneVar    ) {} };
	template< PAYLOAD_TEMPLATE_DECL_TwoVars   > class TStaticDelegate_TwoVars   : public STATIC_DELEGATE_INSTANCE_CLASS_TwoVars  < FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars   > { public: TStaticDelegate_TwoVars  ( typename STATIC_DELEGATE_INSTANCE_CLASS_TwoVars  < FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars   >::FFuncPtr InFuncPtr, PAYLOAD_TEMPLATE_ARGS_TwoVars   ) : STATIC_DELEGATE_INSTANCE_CLASS_TwoVars  < FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_TwoVars   >( InFuncPtr, PAYLOAD_TEMPLATE_PASSIN_TwoVars   ) {} };
	template< PAYLOAD_TEMPLATE_DECL_ThreeVars > class TStaticDelegate_ThreeVars : public STATIC_DELEGATE_INSTANCE_CLASS_ThreeVars< FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars > { public: TStaticDelegate_ThreeVars( typename STATIC_DELEGATE_INSTANCE_CLASS_ThreeVars< FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FFuncPtr InFuncPtr, PAYLOAD_TEMPLATE_ARGS_ThreeVars ) : STATIC_DELEGATE_INSTANCE_CLASS_ThreeVars< FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_ThreeVars >( InFuncPtr, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) {} };
	template< PAYLOAD_TEMPLATE_DECL_FourVars  > class TStaticDelegate_FourVars  : public STATIC_DELEGATE_INSTANCE_CLASS_FourVars < FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars  > { public: TStaticDelegate_FourVars ( typename STATIC_DELEGATE_INSTANCE_CLASS_FourVars < FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars  >::FFuncPtr InFuncPtr, PAYLOAD_TEMPLATE_ARGS_FourVars  ) : STATIC_DELEGATE_INSTANCE_CLASS_FourVars < FUNC_TEMPLATE_ARGS, PAYLOAD_TEMPLATE_LIST_FourVars  >( InFuncPtr, PAYLOAD_TEMPLATE_PASSIN_FourVars  ) {} };


	/**
	 * Static: Creates a raw C++ pointer global function delegate
	 */
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateStatic( typename FStaticDelegate::FFuncPtr InFunc )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( FStaticDelegate::Create( InFunc ) );
	}
	template< PAYLOAD_TEMPLATE_DECL_OneVar >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateStatic( typename TStaticDelegate_OneVar< PAYLOAD_TEMPLATE_LIST_OneVar >::FFuncPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TStaticDelegate_OneVar< PAYLOAD_TEMPLATE_LIST_OneVar >::Create( InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateStatic( typename TStaticDelegate_TwoVars< PAYLOAD_TEMPLATE_LIST_TwoVars >::FFuncPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TStaticDelegate_TwoVars< PAYLOAD_TEMPLATE_LIST_TwoVars >::Create( InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateStatic( typename TStaticDelegate_ThreeVars< PAYLOAD_TEMPLATE_LIST_ThreeVars >::FFuncPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TStaticDelegate_ThreeVars< PAYLOAD_TEMPLATE_LIST_ThreeVars >::Create( InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< PAYLOAD_TEMPLATE_DECL_FourVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateStatic( typename TStaticDelegate_FourVars< PAYLOAD_TEMPLATE_LIST_FourVars >::FFuncPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TStaticDelegate_FourVars< PAYLOAD_TEMPLATE_LIST_FourVars >::Create( InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}

	
	/**
	 * Static: Creates a raw C++ pointer member function delegate.  Raw pointer doesn't use any sort of reference, so may be unsafe to call if the object was
	 * deleted out from underneath your delegate.  Be careful when calling Execute()!
	 */
	template< class UserClass >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateRaw( UserClass* InUserObject, typename TRawMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TRawMethodDelegate< UserClass >::Create( InUserObject, InFunc ) );
	}
	template< class UserClass >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateRaw( UserClass* InUserObject, typename TRawMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TRawMethodDelegate_Const< UserClass >::Create( InUserObject, InFunc ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateRaw( UserClass* InUserObject, typename TRawMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TRawMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::Create( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateRaw( UserClass* InUserObject, typename TRawMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TRawMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::Create( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateRaw( UserClass* InUserObject, typename TRawMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TRawMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::Create( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateRaw( UserClass* InUserObject, typename TRawMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TRawMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::Create( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateRaw( UserClass* InUserObject, typename TRawMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TRawMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::Create( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateRaw( UserClass* InUserObject, typename TRawMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TRawMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::Create( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateRaw( UserClass* InUserObject, typename TRawMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TRawMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::Create( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateRaw( UserClass* InUserObject, typename TRawMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TRawMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::Create( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}

	
	/**
	 * Static: Creates a shared pointer-based (fast, not thread-safe) member function delegate.  Shared pointer delegates keep a weak reference to your object.  You can use ExecuteIfBound() to call them.
	 */
	template< class UserClass >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TSPMethodDelegate< UserClass >::Create( InUserObjectRef, InFunc ) );
	}
	template< class UserClass >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TSPMethodDelegate_Const< UserClass >::Create( InUserObjectRef, InFunc ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TSPMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::Create( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TSPMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::Create( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TSPMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::Create( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TSPMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::Create( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TSPMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::Create( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TSPMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::Create( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TSPMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::Create( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TSPMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::Create( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}


	/**
	 * Static: Creates a shared pointer-based (fast, not thread-safe) member function delegate.  Shared pointer delegates keep a weak reference to your object.  You can use ExecuteIfBound() to call them.
	 */
	template< class UserClass >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( UserClass* InUserObject, typename TSPMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		return CreateSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc );
	}
	template< class UserClass >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( UserClass* InUserObject, typename TSPMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		return CreateSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( UserClass* InUserObject, typename TSPMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		return CreateSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( UserClass* InUserObject, typename TSPMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		return CreateSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( UserClass* InUserObject, typename TSPMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		return CreateSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( UserClass* InUserObject, typename TSPMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		return CreateSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( UserClass* InUserObject, typename TSPMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		return CreateSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( UserClass* InUserObject, typename TSPMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		return CreateSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( UserClass* InUserObject, typename TSPMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		return CreateSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateSP( UserClass* InUserObject, typename TSPMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		return CreateSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars );
	}

	
	/**
	 * Static: Creates a shared pointer-based (slower, conditionally thread-safe) member function delegate.  Shared pointer delegates keep a weak reference to your object.  You can use ExecuteIfBound() to call them.
	 */
	template< class UserClass >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TThreadSafeSPMethodDelegate< UserClass >::Create( InUserObjectRef, InFunc ) );
	}
	template< class UserClass >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TThreadSafeSPMethodDelegate_Const< UserClass >::Create( InUserObjectRef, InFunc ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TThreadSafeSPMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::Create( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TThreadSafeSPMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::Create( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TThreadSafeSPMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::Create( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TThreadSafeSPMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::Create( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TThreadSafeSPMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::Create( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TThreadSafeSPMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::Create( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TThreadSafeSPMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::Create( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TThreadSafeSPMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::Create( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}


	/**
	 * Static: Creates a shared pointer-based (slower, conditionally thread-safe) member function delegate.  Shared pointer delegates keep a weak reference to your object.  You can use ExecuteIfBound() to call them.
	 */
	template< class UserClass >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		return CreateThreadSafeSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc );
	}
	template< class UserClass >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		return CreateThreadSafeSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		return CreateThreadSafeSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		return CreateThreadSafeSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		return CreateThreadSafeSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		return CreateThreadSafeSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		return CreateThreadSafeSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		return CreateThreadSafeSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		return CreateThreadSafeSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		return CreateThreadSafeSP( StaticCastSharedRef< UserClass >( InUserObject->AsShared() ), InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars );
	}


	/**
	 * Static: Creates a UObject-based member function delegate.  UObject delegates keep a weak reference to your object.  You can use ExecuteIfBound() to call them.
	 */
	template< class UserClass >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateUObject( UserClass* InUserObject, typename TUObjectMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TUObjectMethodDelegate< UserClass >::Create( InUserObject, InFunc ) );
	}
	template< class UserClass >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TUObjectMethodDelegate_Const< UserClass >::Create( InUserObject, InFunc ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TUObjectMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::Create( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TUObjectMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::Create( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TUObjectMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::Create( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TUObjectMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::Create( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TUObjectMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::Create( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TUObjectMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::Create( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TUObjectMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::Create( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline static DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > CreateUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		return DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >( TUObjectMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::Create( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}


	/**
	 * Default constructor
	 */
	inline DELEGATE_CLASS()
		: Delegate( NULL )
	{
	}


	/**
	 * Delegate instance destructor
	 */
	inline ~DELEGATE_CLASS()
	{
		Unbind();
	}


	/**
	 * Construct from a delegate instance pointer.  The delegate will assume ownership of the incoming delegate instance!
	 * IMPORTANT: This is a system-internal function and you should never be using this in regular C++ code
	 */
	inline DELEGATE_CLASS( IDelegate* InitDelegate )
		: Delegate( InitDelegate )
	{
	}


	/**
	 * Construct from existing delegate object
	 *
	 * @param	OtherDelegate	Delegate object to copy from
	 */
	inline DELEGATE_CLASS( const DELEGATE_CLASS& OtherDelegate )
		: Delegate( NULL )
	{
		*this = OtherDelegate;
	}


	/**
	 * Assignment operator
	 *
	 * @param	OtherDelegate	Delegate object to copy from
	 */
	inline DELEGATE_CLASS& operator=( const DELEGATE_CLASS& OtherDelegate )
	{
		if( &OtherDelegate != this )
		{
			IDelegate* NewDelegate = NULL;
			if( OtherDelegate.Delegate != NULL )
			{
				NewDelegate = OtherDelegate.Delegate->CreateCopy();
			}

			Unbind();
			Delegate = NewDelegate;
		}
		return *this;
	}


public:


	/**
	 * Creates a raw C++ pointer global function delegate
	 */
	inline void BindStatic( typename FStaticDelegate::FFuncPtr InFunc )
	{
		*this = CreateStatic( InFunc );
	}
	template< PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void BindStatic( typename TStaticDelegate_OneVar< PAYLOAD_TEMPLATE_LIST_OneVar >::FFuncPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		*this = CreateStatic( InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar );
	}
	template< PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void BindStatic( typename TStaticDelegate_TwoVars< PAYLOAD_TEMPLATE_LIST_TwoVars >::FFuncPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		*this = CreateStatic( InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars );
	}
	template< PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void BindStatic( typename TStaticDelegate_ThreeVars< PAYLOAD_TEMPLATE_LIST_ThreeVars >::FFuncPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		*this = CreateStatic( InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars );
	}
	template< PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void BindStatic( typename TStaticDelegate_FourVars< PAYLOAD_TEMPLATE_LIST_FourVars >::FFuncPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		*this = CreateStatic( InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars );
	}


	/**
	 * Binds a raw C++ pointer delegate.  Raw pointer doesn't use any sort of reference, so may be unsafe to call if the object was
	 * deleted out from underneath your delegate.  Be careful when calling Execute()!
	 */
	template< class UserClass >
	inline void BindRaw( UserClass* InUserObject, typename TRawMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		*this = CreateRaw( InUserObject, InFunc );
	}
	template< class UserClass >
	inline void BindRaw( UserClass* InUserObject, typename TRawMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		*this = CreateRaw( InUserObject, InFunc );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void BindRaw( UserClass* InUserObject, typename TRawMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		*this = CreateRaw( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void BindRaw( UserClass* InUserObject, typename TRawMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		*this = CreateRaw( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void BindRaw( UserClass* InUserObject, typename TRawMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		*this = CreateRaw( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void BindRaw( UserClass* InUserObject, typename TRawMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		*this = CreateRaw( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void BindRaw( UserClass* InUserObject, typename TRawMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		*this = CreateRaw( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void BindRaw( UserClass* InUserObject, typename TRawMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		*this = CreateRaw( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void BindRaw( UserClass* InUserObject, typename TRawMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		*this = CreateRaw( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void BindRaw( UserClass* InUserObject, typename TRawMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		*this = CreateRaw( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars );
	}


	/**
	 * Binds a shared pointer-based (fast, not thread-safe) member function delegate.  Shared pointer delegates keep a weak reference to your object.  You can use ExecuteIfBound() to call them.
	 */
	template< class UserClass >
	inline void BindSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		*this = CreateSP( InUserObjectRef, InFunc );
	}
	template< class UserClass >
	inline void BindSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		*this = CreateSP( InUserObjectRef, InFunc );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void BindSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		*this = CreateSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void BindSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		*this = CreateSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void BindSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		*this = CreateSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void BindSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		*this = CreateSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void BindSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		*this = CreateSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void BindSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		*this = CreateSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void BindSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		*this = CreateSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void BindSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename TSPMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		*this = CreateSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars );
	}


	/**
	 * Creates a shared pointer-based (fast, not thread-safe) member function delegate.  Shared pointer delegates keep a weak reference to your object.  You can use ExecuteIfBound() to call them.
	 */
	template< class UserClass >
	inline void BindSP( UserClass* InUserObject, typename TSPMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		*this = CreateSP( InUserObject, InFunc );
	}
	template< class UserClass >
	inline void BindSP( UserClass* InUserObject, typename TSPMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		*this = CreateSP( InUserObject, InFunc );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void BindSP( UserClass* InUserObject, typename TSPMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		*this = CreateSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void BindSP( UserClass* InUserObject, typename TSPMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		*this = CreateSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void BindSP( UserClass* InUserObject, typename TSPMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		*this = CreateSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void BindSP( UserClass* InUserObject, typename TSPMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		*this = CreateSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void BindSP( UserClass* InUserObject, typename TSPMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		*this = CreateSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void BindSP( UserClass* InUserObject, typename TSPMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		*this = CreateSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void BindSP( UserClass* InUserObject, typename TSPMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		*this = CreateSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void BindSP( UserClass* InUserObject, typename TSPMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		*this = CreateSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars );
	}


	/**
	 * Binds a shared pointer-based (slower, conditionally thread-safe) member function delegate.  Shared pointer delegates keep a weak reference to your object.  You can use ExecuteIfBound() to call them.
	 */
	template< class UserClass >
	inline void BindThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		*this = CreateThreadSafeSP( InUserObjectRef, InFunc );
	}
	template< class UserClass >
	inline void BindThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		*this = CreateThreadSafeSP( InUserObjectRef, InFunc );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void BindThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		*this = CreateThreadSafeSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void BindThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		*this = CreateThreadSafeSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void BindThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		*this = CreateThreadSafeSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void BindThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		*this = CreateThreadSafeSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void BindThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		*this = CreateThreadSafeSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void BindThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		*this = CreateThreadSafeSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void BindThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		*this = CreateThreadSafeSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void BindThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename TThreadSafeSPMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		*this = CreateThreadSafeSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars );
	}


	/**
	 * Creates a shared pointer-based (slower, conditionally thread-safe) member function delegate.  Shared pointer delegates keep a weak reference to your object.  You can use ExecuteIfBound() to call them.
	 */
	template< class UserClass >
	inline void BindThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		*this = CreateThreadSafeSP( InUserObject, InFunc );
	}
	template< class UserClass >
	inline void BindThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		*this = CreateThreadSafeSP( InUserObject, InFunc );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void BindThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		*this = CreateThreadSafeSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void BindThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		*this = CreateThreadSafeSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void BindThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		*this = CreateThreadSafeSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void BindThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		*this = CreateThreadSafeSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void BindThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		*this = CreateThreadSafeSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void BindThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		*this = CreateThreadSafeSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void BindThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		*this = CreateThreadSafeSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void BindThreadSafeSP( UserClass* InUserObject, typename TThreadSafeSPMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		*this = CreateThreadSafeSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars );
	}


	/**
	 * Creates a UObject-based member function delegate.  UObject delegates keep a weak reference to your object.  You can use ExecuteIfBound() to call them.
	 */
	template< class UserClass >
	inline void BindUObject( UserClass* InUserObject, typename TUObjectMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		*this = CreateUObject( InUserObject, InFunc );
	}
	template< class UserClass >
	inline void BindUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		*this = CreateUObject( InUserObject, InFunc );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void BindUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		*this = CreateUObject( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void BindUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		*this = CreateUObject( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void BindUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		*this = CreateUObject( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void BindUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		*this = CreateUObject( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void BindUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		*this = CreateUObject( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void BindUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		*this = CreateUObject( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void BindUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		*this = CreateUObject( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void BindUObject( UserClass* InUserObject, typename TUObjectMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		*this = CreateUObject( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars );
	}



	/**
	 * Unbinds this delegate
	 */
	inline void Unbind()
	{
		if( Delegate != NULL )
		{
			delete Delegate;
			Delegate = NULL;
		}
	}


	/**
	 * Checks to see if the user object bound to this delegate is still valid
	 *
	 * @return  True if the user object is still valid and it's safe to execute the function call
	 */
	inline bool IsBound() const
	{
		return Delegate != NULL && Delegate->IsSafeToExecute();
	}

	/** 
	 * Checks to see if this delegate is bound to the given user object.
	 *
	 * @return	True if this delegate is bound to InUserObject, false otherwise.
	 */
	inline bool IsBoundToObject(void const* InUserObject) const
	{
		return InUserObject && Delegate != NULL && Delegate->HasSameObject(InUserObject);
	}

	/**
	 * Checks to see if the user object bound to this delegate will ever be valid again
	 *
	 * @return  True if the user object could be valid in the future or is valid now.
	 */
	inline bool IsCompactable() const
	{
		return Delegate != NULL && Delegate->IsCompactable();
	}

	/**
	 * Execute the delegate.  If the function pointer is not valid, an error will occur.
	 */
	inline RetValType Execute( FUNC_PARAM_LIST ) const
	{
		// If this assert goes off, Execute() was called before a function was bound to the delegate.  Consider using ExecuteIfSafe() instead.
		checkSlow( Delegate != NULL );
		return Delegate->Execute( FUNC_PARAM_PASSTHRU );
	}

	/**
	 * If this is a UObject delegate, return the UObject
	 *
	 * @return  the object associated with this delegate if there is one
	 */
	inline class UObject* GetUObject() const
	{
		if (Delegate && Delegate->GetType() == EDelegateInstanceType::UObjectMethod)
		{
			return (class UObject*)Delegate->GetRawUserObject();
		}
		return NULL;
	}

#if FUNC_IS_VOID
	/**
	 * Execute the delegate, but only if the function pointer is still valid
	 *
	 * @return  Returns true if the function was executed
	 */
	// NOTE: Currently only delegates with no return value support ExecuteIfBound() 
	inline bool ExecuteIfBound( FUNC_PARAM_LIST ) const
	{
		if( IsBound() )
		{
			return Delegate->ExecuteIfSafe( FUNC_PARAM_PASSTHRU );
		}
		return false;
	}
#endif


	/**
	 * Equality operator
	 */
	bool operator==( const DELEGATE_CLASS< FUNC_TEMPLATE_ARGS >& Other ) const
	{
		// The function these delegates point to must be the same
		if( Delegate != NULL && Other.Delegate != NULL )
		{
			return Delegate->IsSameFunction( *Other.Delegate );
		}
		else
		{
			// If neither delegate is initialized to anything yet, then we treat them as equal
			if( Delegate == NULL && Other.Delegate == NULL )
			{
				return true;
			}


			// No match!
			return false;
		}
	}


private:

	// Declare ourselves as a friend to our multi-cast counterpart, so that it can access our interface directly
	template< FUNC_TEMPLATE_DECL_NO_SHADOW > friend class BASE_MULTICAST_DELEGATE_CLASS;

	/** Pointer to the actual delegate object instance.  We need to use a pointer because the actual type of delegate class
	    that's created will depend on the type of binding (raw, shared pointer, etc) as well as the user's object type */
	IDelegate* Delegate;

};



// NOTE: Only delegates with no return value are valid for multi-cast
#if FUNC_IS_VOID

/**
 * Multi-cast delegate base object.   You'll use the various DECLARE_MULTICAST_DELEGATE macros to create
 * the actual delegate type, templated to the function signature the delegate is compatible with.  Then,
 * you can create an instance of that class when you want to bind a function to the delegate.
 */
template< FUNC_TEMPLATE_DECL_TYPENAME >
class BASE_MULTICAST_DELEGATE_CLASS
{

public:

	/** The actual single-cast delegate class for this multi-cast delegate */
	typedef DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > FDelegate;

	/**
	 * Adds a function delegate to this multi-cast delegate's invocation list
	 *
	 * @param	InDelegate	Delegate to add
	 */
	void Add( const FDelegate& InDelegate )
	{
		// Add the delegate
		AddInternal( InDelegate );

		// Then check and see if there are any stale delegates and remove them
		CompactInvocationList();
	}


	/**
	 * Adds a raw C++ pointer global function delegate
	 *
	 * @param	InFunc	Function pointer
	 */
	inline void AddStatic( typename FDelegate::FStaticDelegate::FFuncPtr InFunc )
	{
		Add( FDelegate::CreateStatic( InFunc ) );
	}
	template< PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void AddStatic( typename FDelegate::template TStaticDelegate_OneVar< PAYLOAD_TEMPLATE_LIST_OneVar >::FFuncPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		Add( FDelegate::CreateStatic( InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void AddStatic( typename FDelegate::template TStaticDelegate_TwoVars< PAYLOAD_TEMPLATE_LIST_TwoVars >::FFuncPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		Add( FDelegate::CreateStatic( InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void AddStatic( typename FDelegate::template TStaticDelegate_ThreeVars< PAYLOAD_TEMPLATE_LIST_ThreeVars >::FFuncPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		Add( FDelegate::CreateStatic( InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void AddStatic( typename FDelegate::template TStaticDelegate_FourVars< PAYLOAD_TEMPLATE_LIST_FourVars >::FFuncPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		Add( FDelegate::CreateStatic( InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}


	/**
	 * Adds a raw C++ pointer delegate.  Raw pointer doesn't use any sort of reference, so may be unsafe to call if the object was
	 * deleted out from underneath your delegate.  Be careful when calling Execute()!
	 *
	 * @param	InUserObject	User object to bind to
	 * @param	InFunc			Class method function address
	 */
	template< class UserClass >
	inline void AddRaw( UserClass* InUserObject, typename FDelegate::template TRawMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		Add( FDelegate::CreateRaw( InUserObject, InFunc ) );
	}
	template< class UserClass >
	inline void AddRaw( UserClass* InUserObject, typename FDelegate::template TRawMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		Add( FDelegate::CreateRaw( InUserObject, InFunc ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void AddRaw( UserClass* InUserObject, typename FDelegate::template TRawMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		Add( FDelegate::CreateRaw( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void AddRaw( UserClass* InUserObject, typename FDelegate::template TRawMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		Add( FDelegate::CreateRaw( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void AddRaw( UserClass* InUserObject, typename FDelegate::template TRawMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		Add( FDelegate::CreateRaw( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void AddRaw( UserClass* InUserObject, typename FDelegate::template TRawMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		Add( FDelegate::CreateRaw( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void AddRaw( UserClass* InUserObject, typename FDelegate::template TRawMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		Add( FDelegate::CreateRaw( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void AddRaw( UserClass* InUserObject, typename FDelegate::template TRawMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		Add( FDelegate::CreateRaw( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void AddRaw( UserClass* InUserObject, typename FDelegate::template TRawMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		Add( FDelegate::CreateRaw( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void AddRaw( UserClass* InUserObject, typename FDelegate::template TRawMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		Add( FDelegate::CreateRaw( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}


	/**
	 * Adds a shared pointer-based (fast, not thread-safe) member function delegate.  Shared pointer delegates keep a weak reference to your object.
	 *
	 * @param	InUserObjectRef	User object to bind to
	 * @param	InFunc			Class method function address
	 */
	template< class UserClass >
	inline void AddSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename FDelegate::template TSPMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		Add( FDelegate::CreateSP( InUserObjectRef, InFunc ) );
	}
	template< class UserClass >
	inline void AddSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename FDelegate::template TSPMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		Add( FDelegate::CreateSP( InUserObjectRef, InFunc ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void AddSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename FDelegate::template TSPMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		Add( FDelegate::CreateSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void AddSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename FDelegate::template TSPMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		Add( FDelegate::CreateSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void AddSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename FDelegate::template TSPMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		Add( FDelegate::CreateSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void AddSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename FDelegate::template TSPMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		Add( FDelegate::CreateSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void AddSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename FDelegate::template TSPMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		Add( FDelegate::CreateSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void AddSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename FDelegate::template TSPMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		Add( FDelegate::CreateSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void AddSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename FDelegate::template TSPMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		Add( FDelegate::CreateSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void AddSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename FDelegate::template TSPMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		Add( FDelegate::CreateSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}


	/**
	 * Adds a shared pointer-based (fast, not thread-safe) member function delegate.  Shared pointer delegates keep a weak reference to your object.
	 *
	 * @param	InUserObject	User object to bind to
	 * @param	InFunc			Class method function address
	 */
	template< class UserClass >
	inline void AddSP( UserClass* InUserObject, typename FDelegate::template TSPMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		Add( FDelegate::CreateSP( InUserObject, InFunc ) );
	}
	template< class UserClass >
	inline void AddSP( UserClass* InUserObject, typename FDelegate::template TSPMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		Add( FDelegate::CreateSP( InUserObject, InFunc ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void AddSP( UserClass* InUserObject, typename FDelegate::template TSPMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		Add( FDelegate::CreateSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void AddSP( UserClass* InUserObject, typename FDelegate::template TSPMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		Add( FDelegate::CreateSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void AddSP( UserClass* InUserObject, typename FDelegate::template TSPMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		Add( FDelegate::CreateSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void AddSP( UserClass* InUserObject, typename FDelegate::template TSPMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		Add( FDelegate::CreateSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void AddSP( UserClass* InUserObject, typename FDelegate::template TSPMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		Add( FDelegate::CreateSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void AddSP( UserClass* InUserObject, typename FDelegate::template TSPMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		Add( FDelegate::CreateSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void AddSP( UserClass* InUserObject, typename FDelegate::template TSPMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		Add( FDelegate::CreateSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void AddSP( UserClass* InUserObject, typename FDelegate::template TSPMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		Add( FDelegate::CreateSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}


	/**
	 * Adds a shared pointer-based (slower, conditionally thread-safe) member function delegate.  Shared pointer delegates keep a weak reference to your object.
	 *
	 * @param	InUserObjectRef	User object to bind to
	 * @param	InFunc			Class method function address
	 */
	template< class UserClass >
	inline void AddThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename FDelegate::template TThreadSafeSPMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObjectRef, InFunc ) );
	}
	template< class UserClass >
	inline void AddThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename FDelegate::template TThreadSafeSPMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObjectRef, InFunc ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void AddThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename FDelegate::template TThreadSafeSPMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void AddThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename FDelegate::template TThreadSafeSPMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void AddThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename FDelegate::template TThreadSafeSPMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void AddThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename FDelegate::template TThreadSafeSPMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void AddThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename FDelegate::template TThreadSafeSPMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void AddThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename FDelegate::template TThreadSafeSPMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void AddThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename FDelegate::template TThreadSafeSPMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void AddThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename FDelegate::template TThreadSafeSPMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObjectRef, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}


	/**
	 * Adds a shared pointer-based (slower, conditionally thread-safe) member function delegate.  Shared pointer delegates keep a weak reference to your object.
	 *
	 * @param	InUserObject	User object to bind to
	 * @param	InFunc			Class method function address
	 */
	template< class UserClass >
	inline void AddThreadSafeSP( UserClass* InUserObject, typename FDelegate::template TThreadSafeSPMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObject, InFunc ) );
	}
	template< class UserClass >
	inline void AddThreadSafeSP( UserClass* InUserObject, typename FDelegate::template TThreadSafeSPMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObject, InFunc ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void AddThreadSafeSP( UserClass* InUserObject, typename FDelegate::template TThreadSafeSPMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void AddThreadSafeSP( UserClass* InUserObject, typename FDelegate::template TThreadSafeSPMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void AddThreadSafeSP( UserClass* InUserObject, typename FDelegate::template TThreadSafeSPMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void AddThreadSafeSP( UserClass* InUserObject, typename FDelegate::template TThreadSafeSPMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void AddThreadSafeSP( UserClass* InUserObject, typename FDelegate::template TThreadSafeSPMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void AddThreadSafeSP( UserClass* InUserObject, typename FDelegate::template TThreadSafeSPMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void AddThreadSafeSP( UserClass* InUserObject, typename FDelegate::template TThreadSafeSPMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void AddThreadSafeSP( UserClass* InUserObject, typename FDelegate::template TThreadSafeSPMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		Add( FDelegate::CreateThreadSafeSP( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}


	/**
	 * Adds a UObject-based member function delegate.  UObject delegates keep a weak reference to your object.
	 *
	 * @param	InUserObject	User object to bind to
	 * @param	InFunc			Class method function address
	 */
	template< class UserClass >
	inline void AddUObject( UserClass* InUserObject, typename FDelegate::template TUObjectMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		Add( FDelegate::CreateUObject( InUserObject, InFunc ) );
	}
	template< class UserClass >
	inline void AddUObject( UserClass* InUserObject, typename FDelegate::template TUObjectMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		Add( FDelegate::CreateUObject( InUserObject, InFunc ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void AddUObject( UserClass* InUserObject, typename FDelegate::template TUObjectMethodDelegate_OneVar< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		Add( FDelegate::CreateUObject( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_OneVar >
	inline void AddUObject( UserClass* InUserObject, typename FDelegate::template TUObjectMethodDelegate_OneVar_Const< UserClass, PAYLOAD_TEMPLATE_LIST_OneVar >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_OneVar )
	{
		Add( FDelegate::CreateUObject( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_OneVar ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void AddUObject( UserClass* InUserObject, typename FDelegate::template TUObjectMethodDelegate_TwoVars< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		Add( FDelegate::CreateUObject( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_TwoVars >
	inline void AddUObject( UserClass* InUserObject, typename FDelegate::template TUObjectMethodDelegate_TwoVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_TwoVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_TwoVars )
	{
		Add( FDelegate::CreateUObject( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_TwoVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void AddUObject( UserClass* InUserObject, typename FDelegate::template TUObjectMethodDelegate_ThreeVars< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		Add( FDelegate::CreateUObject( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_ThreeVars >
	inline void AddUObject( UserClass* InUserObject, typename FDelegate::template TUObjectMethodDelegate_ThreeVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_ThreeVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_ThreeVars )
	{
		Add( FDelegate::CreateUObject( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_ThreeVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void AddUObject( UserClass* InUserObject, typename FDelegate::template TUObjectMethodDelegate_FourVars< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		Add( FDelegate::CreateUObject( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}
	template< class UserClass, PAYLOAD_TEMPLATE_DECL_FourVars >
	inline void AddUObject( UserClass* InUserObject, typename FDelegate::template TUObjectMethodDelegate_FourVars_Const< UserClass, PAYLOAD_TEMPLATE_LIST_FourVars >::FMethodPtr InFunc, PAYLOAD_TEMPLATE_ARGS_FourVars )
	{
		Add( FDelegate::CreateUObject( InUserObject, InFunc, PAYLOAD_TEMPLATE_PASSIN_FourVars ) );
	}


	/**
	 *	Removes all functions from this multi-cast delegate's invocation list that are bound to the specified UserObject.
	 *	Note that the order of the delegates may not be preserved!
	 */
	void RemoveAll( const void* InUserObject )
	{
		// Remove the delegate
		RemoveByObjectInternal( InUserObject );

		// Check for any delegates that may have expired
		CompactInvocationList();
	}


	/**
	 * Removes a function from this multi-cast delegate's invocation list (performance is O(N)).  Note that the
	 * order of the delegates may not be preserved!
	 *
	 * @param	InDelegate	Delegate to remove
	 */
	void Remove( const FDelegate& InDelegate )
	{
		// NOTE: For delegates with PAYLOAD, the payload variables are not considered when comparing delegate objects!
		// This means that two delegate instances bound to the same object/method, but with different payload data,
		// cannot be bound to the same single multi-cast delegate.  We could support this by changing IsSameFunction()
		// to do comparison of payload data, but there are some trade-offs if we do that.

		// Remove the delegate
		RemoveByDelegateInternal( InDelegate );

		// Check for any delegates that may have expired
		CompactInvocationList();
	}


	// NOTE: These direct Remove methods are only supported for multi-cast delegates with no payload attached.
	// See the comment in the multi-cast delegate Remove() method above for more details.

	/**
	 * Removes a raw C++ pointer global function delegate (performance is O(N)).  Note that the order of the
	 * delegates may not be preserved!
	 *
	 * @param	InFunc	Function pointer
	 */
	inline void RemoveStatic( typename FDelegate::FStaticDelegate::FFuncPtr InFunc )
	{
		Remove( FDelegate::CreateStatic( InFunc ) );
	}


	/**
	 * Removes a raw C++ pointer delegate (performance is O(N)).  Note that the order of the delegates may not
	 * be preserved!
	 *
	 * @param	InUserObject	User object to unbind from
	 * @param	InFunc			Class method function address
	 */
	template< class UserClass >
	inline void RemoveRaw( UserClass* InUserObject, typename FDelegate::template TRawMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		Remove( FDelegate::CreateRaw( InUserObject, InFunc ) );
	}
	template< class UserClass >
	inline void RemoveRaw( UserClass* InUserObject, typename FDelegate::template TRawMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		Remove( FDelegate::CreateRaw( InUserObject, InFunc ) );
	}


	/**
	 * Removes a shared pointer-based member function delegate (performance is O(N)).  Note that the order of
	 * the delegates may not be preserved!
	 *
	 * @param	InUserObjectRef	User object to unbind from
	 * @param	InFunc			Class method function address
	 */
	template< class UserClass >
	inline void RemoveSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename FDelegate::template TSPMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		Remove( FDelegate::CreateSP( InUserObjectRef, InFunc ) );
	}
	template< class UserClass >
	inline void RemoveSP( const TSharedRef< UserClass, ESPMode::Fast >& InUserObjectRef, typename FDelegate::template TSPMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		Remove( FDelegate::CreateSP( InUserObjectRef, InFunc ) );
	}


	/**
	 * Removes a shared pointer-based member function delegate (performance is O(N)).  Note that the order of
	 * the delegates may not be preserved!
	 *
	 * @param	InUserObject	User object to unbind from
	 * @param	InFunc			Class method function address
	 */
	template< class UserClass >
	inline void RemoveSP( UserClass* InUserObject, typename FDelegate::template TSPMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		Remove( FDelegate::CreateSP( InUserObject, InFunc ) );
	}
	template< class UserClass >
	inline void RemoveSP( UserClass* InUserObject, typename FDelegate::template TSPMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		Remove( FDelegate::CreateSP( InUserObject, InFunc ) );
	}


	/**
	 * Removes a shared pointer-based member function delegate (performance is O(N)).  Note that the order of
	 * the delegates may not be preserved!
	 *
	 * @param	InUserObjectRef	User object to unbind from
	 * @param	InFunc			Class method function address
	 */
	template< class UserClass >
	inline void RemoveThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename FDelegate::template TThreadSafeSPMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		Remove( FDelegate::CreateThreadSafeSP( InUserObjectRef, InFunc ) );
	}
	template< class UserClass >
	inline void RemoveThreadSafeSP( const TSharedRef< UserClass, ESPMode::ThreadSafe >& InUserObjectRef, typename FDelegate::template TThreadSafeSPMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		Remove( FDelegate::CreateThreadSafeSP( InUserObjectRef, InFunc ) );
	}


	/**
	 * Removes a shared pointer-based member function delegate (performance is O(N)).  Note that the order of
	 * the delegates may not be preserved!
	 *
	 * @param	InUserObject	User object to unbind from
	 * @param	InFunc			Class method function address
	 */
	template< class UserClass >
	inline void RemoveThreadSafeSP( UserClass* InUserObject, typename FDelegate::template TThreadSafeSPMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		Remove( FDelegate::CreateThreadSafeSP( InUserObject, InFunc ) );
	}
	template< class UserClass >
	inline void RemoveThreadSafeSP( UserClass* InUserObject, typename FDelegate::template TThreadSafeSPMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		Remove( FDelegate::CreateThreadSafeSP( InUserObject, InFunc ) );
	}


	/**
	 * Removes a UObject-based member function delegate (performance is O(N)).  Note that the order of the
	 * delegates may not be preserved!
	 *
	 * @param	InUserObject	User object to unbind from
	 * @param	InFunc			Class method function address
	 */
	template< class UserClass >
	inline void RemoveUObject( UserClass* InUserObject, typename FDelegate::template TUObjectMethodDelegate< UserClass >::FMethodPtr InFunc )
	{
		Remove( FDelegate::CreateUObject( InUserObject, InFunc ) );
	}
	template< class UserClass >
	inline void RemoveUObject( UserClass* InUserObject, typename FDelegate::template TUObjectMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		Remove( FDelegate::CreateUObject( InUserObject, InFunc ) );
	}

	/** 
	 * Checks to see if any functions are bound to the given user object.
	 *
	 * @return	True if any functions are bound to InUserObject, false otherwise.
	 */
	inline bool IsBoundToObject(void const* InUserObject) const
	{
		const int32 NumFunctions = InvocationList.Num();
		for( int32 CurFunctionIndex = 0; CurFunctionIndex < NumFunctions; ++CurFunctionIndex )
		{
			if (InvocationList[ CurFunctionIndex ].Delegate->HasSameObject( InUserObject ) )
			{
				return true;
			}
		}

		return false;
	}

protected:

	/**
	 * Default constructor
	 */
	inline BASE_MULTICAST_DELEGATE_CLASS()
		: ExpirableObjectCount( 0 )
	{
	}


	/**
	 * Checks to see if any functions are bound to this multi-cast delegate. Private to avoid assumptions about only one delegate existing
	 *
	 * @return	True if any functions are bound
	 */
	inline bool IsBound() const
	{
		return InvocationList.Num() > 0;
	}


	/**
	 * Removes all functions from this delegate's invocation list
	 */
	void Clear()
	{
		InvocationList.Empty();
		ExpirableObjectCount = 0;
	}


	/**
	 * Broadcasts this delegate to all bound objects, except to those that may have expired
	 */
	void Broadcast( FUNC_PARAM_LIST ) const
	{
		if( InvocationList.Num() > 0 )
		{
			// Create a copy of the invocation list, just in case the list is modified by one of the callbacks
			// during the broadcast
			typedef TArray< FDelegate, TInlineAllocator< 4 > > FInlineInvocationList;
			FInlineInvocationList InvocationListCopy = FInlineInvocationList(InvocationList);

			// Invoke each bound function
			for( typename FInlineInvocationList::TConstIterator FunctionIt( InvocationListCopy ); FunctionIt; ++FunctionIt )
			{
				if( !FunctionIt->ExecuteIfBound( FUNC_PARAM_PASSTHRU ) )
				{
					if( FunctionIt->IsCompactable() )
					{
						// Function couldn't be executed and it is compactable, so remove it.  Note that because the 
						// original list could have been modified by one of the callbacks, we have to search for the 
						// function to remove here.
						RemoveByDelegateInternal( *FunctionIt );
					}
				}
			}
		}
	}


private:

	/**
	 * Adds a function delegate to this multi-cast delegate's invocation list
	 *
	 * @param	InDelegate	Delegate to add
	 */
	void AddInternal( const FDelegate& InDelegate )
	{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
		// Verify same function isn't already bound.
		const int32 NumFunctions = InvocationList.Num();
		for( int32 CurFunctionIndex = 0; CurFunctionIndex < NumFunctions; ++CurFunctionIndex )
		{
			// NOTE: Multi-cast delegates currently don't support binding two delegates with the same object/function
			// that have different payload variables.  See the comment in the Remove() method for more information.
			check( !InvocationList[ CurFunctionIndex ].Delegate->IsSameFunction( *InDelegate.Delegate ) );
		}
#endif
		InvocationList.Add( InDelegate );

		// Keep track of whether we have objects bound that may expire out from underneath us.
		const EDelegateInstanceType::Type DelegateType = InDelegate.Delegate->GetType();
		if( DelegateType == EDelegateInstanceType::SharedPointerMethod ||
			DelegateType == EDelegateInstanceType::ThreadSafeSharedPointerMethod ||
			DelegateType == EDelegateInstanceType::UObjectMethod )
		{
			++ExpirableObjectCount;
		}
	}


	/**
	 * Removes a function from this multi-cast delegate's invocation list (performance is O(N)).  Note that the
	 * order of the delegates may not be preserved!
	 *
	 * @param	InDelegate	Delegate to remove
	 */
	void RemoveByDelegateInternal( const FDelegate& InDelegate ) const
	{
		const int32 NumFunctions = InvocationList.Num();
		for( int32 CurFunctionIndex = 0; CurFunctionIndex < NumFunctions; ++CurFunctionIndex )
		{
			// NOTE: We must do a deep compare here, not just compare delegate pointers, because multiple
			//       delegate pointers can refer to the exact same object and method
			if( InvocationList[ CurFunctionIndex ].Delegate->IsSameFunction( *InDelegate.Delegate ) )
			{
				// Keep track of whether we have objects bound that may expire out from underneath us.
				const EDelegateInstanceType::Type DelegateType = InvocationList[ CurFunctionIndex ].Delegate->GetType();
				if( DelegateType == EDelegateInstanceType::SharedPointerMethod || 
					DelegateType == EDelegateInstanceType::ThreadSafeSharedPointerMethod ||
					DelegateType == EDelegateInstanceType::UObjectMethod )
				{
					--ExpirableObjectCount;
				}

				InvocationList.RemoveAtSwap( CurFunctionIndex );

				// No need to continue, as we never allow the same delegate to be bound twice
				break;
			}
		}

		// NOTE: We don't want to assert that the specified delegate was removed, because delegates that are
		//       bound by weak pointer may vanish when the object goes out of scope during cleanup, which
		//       could create unpredictable assertions.  It's best to always safely allow a delegate remove attempt.
	}

	/**
	 *	Removes all functions from this multi-cast delegate's invocation list that are bound to the specified UserObject.
	 *	Note that the order of the delegates may not be preserved!
	 */
	void RemoveByObjectInternal( const void* InUserObject )
	{
		const int32 NumFunctions = InvocationList.Num();
		for( int CurFunctionIndex = NumFunctions - 1; CurFunctionIndex >= 0; --CurFunctionIndex )
		{
			if( InvocationList[CurFunctionIndex].Delegate->HasSameObject( InUserObject ) )
			{
				// Keep track of whether we have objects bound that may expire out from underneath us.
				const EDelegateInstanceType::Type DelegateType = InvocationList[CurFunctionIndex].Delegate->GetType();
				if( DelegateType == EDelegateInstanceType::SharedPointerMethod ||
					DelegateType == EDelegateInstanceType::ThreadSafeSharedPointerMethod ||
					DelegateType == EDelegateInstanceType::UObjectMethod )
				{
					--ExpirableObjectCount;
				}

				InvocationList.RemoveAtSwap( CurFunctionIndex );
			}
		}
	}


	/** Cleans up any delegates in our invocation list that have expired (performance is O(N)) */
	void CompactInvocationList() const
	{
		// We only need to compact if any objects were added whose lifetime is known to us
		if( ExpirableObjectCount > 0 )
		{
			for( int32 CurFunctionIndex = 0; CurFunctionIndex < InvocationList.Num(); ++CurFunctionIndex )
			{
				if( InvocationList[CurFunctionIndex].IsCompactable() )
				{
					// Remove it, and decrement loop counter so that we don't skip over an element
					InvocationList.RemoveAtSwap( CurFunctionIndex-- );

					checkSlow( ExpirableObjectCount > 0 );
					--ExpirableObjectCount;
				}
			}
		}
	}


private:

	/** Ordered list functions to invoke when the Broadcast function is called */
	typedef TArray< FDelegate > FInvocationList;
	mutable FInvocationList InvocationList;		// Mutable so that we can housekeep list even with 'const' broadcasts

	/** Tracks the number of objects in our invocation list whose lifetime is known to us.  That is,
	    these objects could potentially expire out from underneath us. */
	mutable int32 ExpirableObjectCount;		// Mutable so that we can housekeep list even with 'const' broadcasts
};


template< FUNC_TEMPLATE_DECL_TYPENAME >
class MULTICAST_DELEGATE_CLASS : public BASE_MULTICAST_DELEGATE_CLASS< FUNC_TEMPLATE_DECL >
{
private:  // Prevents erroneous use by other classes.
	typedef BASE_MULTICAST_DELEGATE_CLASS< FUNC_TEMPLATE_DECL > Super;

public:

	/**
	 * Checks to see if any functions are bound to this multi-cast delegate
	 *
	 * @return	True if any functions are bound
	 */
	inline bool IsBound() const
	{
		return Super::IsBound();
	}


	/**
	 * Removes all functions from this delegate's invocation list
	 */
	inline void Clear()
	{
		return Super::Clear();
	}


	/**
	 * Broadcasts this delegate to all bound objects, except to those that may have expired
	 */
	inline void Broadcast( FUNC_PARAM_LIST ) const
	{
		return Super::Broadcast( FUNC_PARAM_PASSTHRU );
	}
};
#endif		// FUNC_IS_VOID



/**
 * Dynamic delegate base object (UObject-based, serializable).  You'll use the various DECLARE_DYNAMIC_DELEGATE
 * macros to create the actual delegate type, templated to the function signature the delegate is compatible with.
 * Then, you can create an instance of that class when you want to assign functions to the delegate.
 */
template< FUNC_TEMPLATE_DECL_TYPENAME, typename TWeakPtr = FWeakObjectPtr >
class DYNAMIC_DELEGATE_CLASS
	: public TScriptDelegate<TWeakPtr>
{

public:

	/**
	 * Default constructor
	 */
	DYNAMIC_DELEGATE_CLASS()
	{
	}


	/**
	 * Construction from an FScriptDelegate must be explicit.  This is really only used by UObject system internals.
	 *
	 * @param	InScriptDelegate	The delegate to construct from by copying
	 */
	explicit DYNAMIC_DELEGATE_CLASS( const TScriptDelegate<TWeakPtr>& InScriptDelegate )
		: TScriptDelegate<TWeakPtr>( InScriptDelegate )
	{
	}


	/**
	 * Templated helper class to define a typedef for user's method pointer, then used below
	 */
	template< class UserClass >
	class TMethodPtrResolver
	{

	public:

		typedef RetValType ( UserClass::*FMethodPtr )( FUNC_PARAM_LIST );
	};


	/**
	 * Binds a UObject instance and a UObject method address to this delegate.
	 *
	 * @param	InUserObject			UObject instance
	 * @param	InMethodPtr				Member function address pointer
	 * @param	InMacroFunctionName		Name of member function, including class name (generated by a macro)
	 *
	 * NOTE:  Do not call this function directly.  Instead, call BindDynamic() which is a macro proxy function that
	 *        automatically sets the function name string for the caller.
	 */
	template< class UserClass >
	void __Internal_BindDynamic( UserClass* InUserObject, typename TMethodPtrResolver< UserClass >::FMethodPtr InMethodPtr, const FString& InMacroFunctionName )
	{
		check( InUserObject != NULL && InMethodPtr != NULL && !InMacroFunctionName.IsEmpty() );

		// NOTE: We're not actually storing the incoming method pointer or calling it.  We simply require it for type-safety reasons.

		// NOTE: If you hit a compile error on the following line, it means you're trying to use a non-UObject type
		//       with this delegate, which is not supported
		this->Object = InUserObject;

		// Store the function name.  The incoming function name was generated by a macro and includes the method's class name.
		// We'll need to strip off that class prefix and just store the function name by itself.
		this->FunctionName = *InMacroFunctionName.Mid( InMacroFunctionName.Find( TEXT( "::" ), ESearchCase::CaseSensitive, ESearchDir::FromEnd) + 2 );

		ensureMsgf(this->IsBound(), TEXT("Unable to bind delegate to '%s' (function might not be marked as a UFUNCTION)"), *InMacroFunctionName);
	}

	friend uint32 GetTypeHash(const DYNAMIC_DELEGATE_CLASS& Key)
	{
		return FCrc::MemCrc_DEPRECATED(&Key,sizeof(Key));
	}

	// NOTE:  Execute() method must be defined in derived classes

	// NOTE:  ExecuteIfBound() method must be defined in derived classes

};



/**
 * Dynamic multi-cast delegate base object (UObject-based, serializable).  You'll use the various
 * DECLARE_DYNAMIC_MULTICAST_DELEGATE macros to create the actual delegate type, templated to the function
 * signature the delegate is compatible with.   Then, you can create an instance of that class when you
 * want to assign functions to the delegate.
 */
template< FUNC_TEMPLATE_DECL_TYPENAME, typename TWeakPtr = FWeakObjectPtr >
class DYNAMIC_MULTICAST_DELEGATE_CLASS
	: public TMulticastScriptDelegate<TWeakPtr>
{

public:

	/** The actual single-cast delegate class for this multi-cast delegate */
	typedef DYNAMIC_DELEGATE_CLASS< FUNC_TEMPLATE_ARGS > FDelegate;


	/**
	 * Default constructor
	 */
	DYNAMIC_MULTICAST_DELEGATE_CLASS()
	{
	}


	/**
	 * Construction from an FMulticastScriptDelegate must be explicit.  This is really only used by UObject system internals.
	 *
	 * @param	InScriptDelegate	The delegate to construct from by copying
	 */
	explicit DYNAMIC_MULTICAST_DELEGATE_CLASS( const TMulticastScriptDelegate<TWeakPtr>& InMulticastScriptDelegate )
		: TMulticastScriptDelegate<TWeakPtr>( InMulticastScriptDelegate )
	{
	}


	/**
	 * Tests if a UObject instance and a UObject method address pair are already bound to this multi-cast delegate.
	 *
	 * @param	InUserObject			UObject instance
	 * @param	InMethodPtr				Member function address pointer
	 * @param	InMacroFunctionName		Name of member function, including class name (generated by a macro)
	 * @return	True if the instance/method is already bound.
	 *
	 * NOTE:  Do not call this function directly.  Instead, call IsAlreadyBound() which is a macro proxy function that
	 *        automatically sets the function name string for the caller.
	 */
	template< class UserClass >
	bool __Internal_IsAlreadyBound( UserClass* InUserObject, typename FDelegate::template TMethodPtrResolver< UserClass >::FMethodPtr InMethodPtr, const FString& InMacroFunctionName ) const
	{
		check( InUserObject != NULL && InMethodPtr != NULL && !InMacroFunctionName.IsEmpty() );

		// NOTE: We're not actually using the incoming method pointer or calling it.  We simply require it for type-safety reasons.

		FDelegate NewDelegate;
		NewDelegate.__Internal_BindDynamic( InUserObject, InMethodPtr, InMacroFunctionName );

		return this->Contains( NewDelegate );
	}


	/**
	 * Binds a UObject instance and a UObject method address to this multi-cast delegate.
	 *
	 * @param	InUserObject			UObject instance
	 * @param	InMethodPtr				Member function address pointer
	 * @param	InMacroFunctionName		Name of member function, including class name (generated by a macro)
	 *
	 * NOTE:  Do not call this function directly.  Instead, call AddDynamic() which is a macro proxy function that
	 *        automatically sets the function name string for the caller.
	 */
	template< class UserClass >
	void __Internal_AddDynamic( UserClass* InUserObject, typename FDelegate::template TMethodPtrResolver< UserClass >::FMethodPtr InMethodPtr, const FString& InMacroFunctionName )
	{
		check( InUserObject != NULL && InMethodPtr != NULL && !InMacroFunctionName.IsEmpty() );

		// NOTE: We're not actually storing the incoming method pointer or calling it.  We simply require it for type-safety reasons.

		FDelegate NewDelegate;
		NewDelegate.__Internal_BindDynamic( InUserObject, InMethodPtr, InMacroFunctionName );

		this->Add( NewDelegate );
	}


	/**
	 * Binds a UObject instance and a UObject method address to this multi-cast delegate, but only if it hasn't been bound before.
	 *
	 * @param	InUserObject			UObject instance
	 * @param	InMethodPtr				Member function address pointer
	 * @param	InMacroFunctionName		Name of member function, including class name (generated by a macro)
	 *
	 * NOTE:  Do not call this function directly.  Instead, call AddUniqueDynamic() which is a macro proxy function that
	 *        automatically sets the function name string for the caller.
	 */
	template< class UserClass >
	void __Internal_AddUniqueDynamic( UserClass* InUserObject, typename FDelegate::template TMethodPtrResolver< UserClass >::FMethodPtr InMethodPtr, const FString& InMacroFunctionName )
	{
		check( InUserObject != NULL && InMethodPtr != NULL && !InMacroFunctionName.IsEmpty() );

		// NOTE: We're not actually storing the incoming method pointer or calling it.  We simply require it for type-safety reasons.

		FDelegate NewDelegate;
		NewDelegate.__Internal_BindDynamic( InUserObject, InMethodPtr, InMacroFunctionName );

		this->AddUnique( NewDelegate );
	}


	/**
	 * Unbinds a UObject instance and a UObject method address from this multi-cast delegate.
	 *
	 * @param	InUserObject			UObject instance
	 * @param	InMethodPtr				Member function address pointer
	 * @param	InMacroFunctionName		Name of member function, including class name (generated by a macro)
	 *
	 * NOTE:  Do not call this function directly.  Instead, call RemoveDynamic() which is a macro proxy function that
	 *        automatically sets the function name string for the caller.
	 */
	template< class UserClass >
	void __Internal_RemoveDynamic( UserClass* InUserObject, typename FDelegate::template TMethodPtrResolver< UserClass >::FMethodPtr InMethodPtr, const FString& InMacroFunctionName )
	{
		check( InUserObject != NULL && InMethodPtr != NULL && !InMacroFunctionName.IsEmpty() );

		// NOTE: We're not actually storing the incoming method pointer or calling it.  We simply require it for type-safety reasons.

		FDelegate NewDelegate;
		NewDelegate.__Internal_BindDynamic( InUserObject, InMethodPtr, InMacroFunctionName );

		this->Remove( NewDelegate );
	}


	// NOTE:  Broadcast() method must be defined in derived classes

};



#undef PAYLOAD_TEMPLATE_DECL_OneVar
#undef PAYLOAD_TEMPLATE_LIST_OneVar
#undef PAYLOAD_TEMPLATE_ARGS_OneVar
#undef PAYLOAD_TEMPLATE_PASSIN_OneVar

#undef PAYLOAD_TEMPLATE_DECL_TwoVars
#undef PAYLOAD_TEMPLATE_LIST_TwoVars
#undef PAYLOAD_TEMPLATE_ARGS_TwoVars
#undef PAYLOAD_TEMPLATE_PASSIN_TwoVars

#undef PAYLOAD_TEMPLATE_DECL_ThreeVars
#undef PAYLOAD_TEMPLATE_LIST_ThreeVars
#undef PAYLOAD_TEMPLATE_ARGS_ThreeVars
#undef PAYLOAD_TEMPLATE_PASSIN_ThreeVars

#undef PAYLOAD_TEMPLATE_DECL_FourVars
#undef PAYLOAD_TEMPLATE_LIST_FourVars
#undef PAYLOAD_TEMPLATE_ARGS_FourVars
#undef PAYLOAD_TEMPLATE_PASSIN_FourVars

#undef DYNAMIC_MULTICAST_DELEGATE_CLASS
#undef DYNAMIC_DELEGATE_CLASS
#undef EVENT_CLASS
#undef MULTICAST_DELEGATE_CLASS
#undef DELEGATE_CLASS

#undef SP_METHOD_DELEGATE_INSTANCE_CLASS_NoVars
#undef SP_METHOD_DELEGATE_INSTANCE_CLASS_OneVar
#undef SP_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars
#undef SP_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars
#undef SP_METHOD_DELEGATE_INSTANCE_CLASS_FourVars

#undef RAW_METHOD_DELEGATE_INSTANCE_CLASS_NoVars
#undef RAW_METHOD_DELEGATE_INSTANCE_CLASS_OneVar
#undef RAW_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars
#undef RAW_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars
#undef RAW_METHOD_DELEGATE_INSTANCE_CLASS_FourVars

#undef UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_NoVars
#undef UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_OneVar
#undef UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_TwoVars
#undef UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_ThreeVars
#undef UOBJECT_METHOD_DELEGATE_INSTANCE_CLASS_FourVars

#undef STATIC_DELEGATE_INSTANCE_CLASS_NoVars
#undef STATIC_DELEGATE_INSTANCE_CLASS_OneVar
#undef STATIC_DELEGATE_INSTANCE_CLASS_TwoVars
#undef STATIC_DELEGATE_INSTANCE_CLASS_ThreeVars
#undef STATIC_DELEGATE_INSTANCE_CLASS_FourVars

#undef DELEGATE_INSTANCE_INTERFACE_CLASS


