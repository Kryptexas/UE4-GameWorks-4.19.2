// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PyWrapperMath.h"
#include "PyWrapperTypeRegistry.h"
#include "PyCore.h"
#include "PyUtil.h"
#include "PyConstant.h"
#include "PyConversion.h"

#if WITH_PYTHON

template <typename WrapperType>
void InitializeAndRegisterMathType(PyObject* PyModule, PyTypeObject* InPyType, FPyConstantDef* InPyConstants, const TFunctionRef<void(TArray<PyGenUtil::FGeneratedWrappedMethodParameter>&)>& InBuildInitParamsFunc)
{
	typedef typename WrapperType::WrappedType WrappedType;

	if (PyType_Ready(InPyType) == 0)
	{
		static TPyWrapperInlineStructMetaData<WrappedType> MetaData;
		MetaData.Struct = TBaseStructure<WrappedType>::Get();
		InBuildInitParamsFunc(MetaData.InitParams);
		TPyWrapperInlineStructMetaData<WrappedType>::SetMetaData(InPyType, &MetaData);

		if (InPyConstants)
		{
			FPyConstantDef::AddConstantsToType(InPyConstants, InPyType);
		}

		Py_INCREF(InPyType);
		PyModule_AddObject(PyModule, InPyType->tp_name, (PyObject*)InPyType);

		FPyWrapperTypeRegistry::Get().RegisterWrappedStructType(MetaData.Struct->GetFName(), InPyType);
	}
}

void InitializePyWrapperVector(PyObject* PyModule)
{
	struct FConstants
	{
		static PyObject* VectorGetter(const void* InValuePtr)
		{
			const FVector* VectorPtr = static_cast<const FVector*>(InValuePtr);
			return PyConversion::PythonizeStruct(*VectorPtr);
		}
	};

	static FPyConstantDef PyConstants[] = {
		{ &FVector::ZeroVector, &FConstants::VectorGetter, "ZERO_VECTOR", "A zero vector (0,0,0)" },
		{ &FVector::OneVector, &FConstants::VectorGetter, "ONE_VECTOR", "One vector (1,1,1)" },
		{ &FVector::UpVector, &FConstants::VectorGetter, "UP_VECTOR", "World up vector (0,0,1)" },
		{ &FVector::ForwardVector, &FConstants::VectorGetter, "FORWARD_VECTOR", "Unreal forward vector (1,0,0)" },
		{ &FVector::RightVector, &FConstants::VectorGetter, "RIGHT_VECTOR", "Unreal right vector (0,1,0)" },
		{ nullptr, nullptr, nullptr, nullptr }
	};
	
	InitializeAndRegisterMathType<FPyWrapperVector>(PyModule, &PyWrapperVectorType, PyConstants, [](TArray<PyGenUtil::FGeneratedWrappedMethodParameter>& OutInitParams)
	{
		PyGenUtil::AddStructInitParam(TEXT("X"), TEXT("x"), OutInitParams);
		PyGenUtil::AddStructInitParam(TEXT("Y"), TEXT("y"), OutInitParams);
		PyGenUtil::AddStructInitParam(TEXT("Z"), TEXT("z"), OutInitParams);
	});
}

void InitializePyWrapperVector2D(PyObject* PyModule)
{
	struct FConstants
	{
		static PyObject* Vector2DGetter(const void* InValuePtr)
		{
			const FVector2D* Vector2DPtr = static_cast<const FVector2D*>(InValuePtr);
			return PyConversion::PythonizeStruct(*Vector2DPtr);
		}
	};

	static FPyConstantDef PyConstants[] = {
		{ &FVector2D::ZeroVector, &FConstants::Vector2DGetter, "ZERO_VECTOR", "Global 2D zero vector constant (0,0)" },
		{ &FVector2D::UnitVector, &FConstants::Vector2DGetter, "UNIT_VECTOR", "Global 2D unit vector constant (1,1)" },
		{ nullptr, nullptr, nullptr, nullptr }
	};

	InitializeAndRegisterMathType<FPyWrapperVector2D>(PyModule, &PyWrapperVector2DType, PyConstants, [](TArray<PyGenUtil::FGeneratedWrappedMethodParameter>& OutInitParams)
	{
		PyGenUtil::AddStructInitParam(TEXT("X"), TEXT("x"), OutInitParams);
		PyGenUtil::AddStructInitParam(TEXT("Y"), TEXT("y"), OutInitParams);
	});
}

void InitializePyWrapperVector4(PyObject* PyModule)
{
	InitializeAndRegisterMathType<FPyWrapperVector4>(PyModule, &PyWrapperVector4Type, nullptr, [](TArray<PyGenUtil::FGeneratedWrappedMethodParameter>& OutInitParams)
	{
		PyGenUtil::AddStructInitParam(TEXT("X"), TEXT("x"), OutInitParams);
		PyGenUtil::AddStructInitParam(TEXT("Y"), TEXT("y"), OutInitParams);
		PyGenUtil::AddStructInitParam(TEXT("Z"), TEXT("z"), OutInitParams);
		PyGenUtil::AddStructInitParam(TEXT("W"), TEXT("w"), OutInitParams);
	});
}

void InitializePyWrapperQuat(PyObject* PyModule)
{
	struct FConstants
	{
		static PyObject* QuatGetter(const void* InValuePtr)
		{
			const FQuat* QuatPtr = static_cast<const FQuat*>(InValuePtr);
			return PyConversion::PythonizeStruct(*QuatPtr);
		}
	};

	static FPyConstantDef PyConstants[] = {
		{ &FQuat::Identity, &FConstants::QuatGetter, "IDENTITY", "Identity quaternion" },
		{ nullptr, nullptr, nullptr, nullptr }
	};

	InitializeAndRegisterMathType<FPyWrapperQuat>(PyModule, &PyWrapperQuatType, PyConstants, [](TArray<PyGenUtil::FGeneratedWrappedMethodParameter>& OutInitParams)
	{
		PyGenUtil::AddStructInitParam(TEXT("X"), TEXT("x"), OutInitParams);
		PyGenUtil::AddStructInitParam(TEXT("Y"), TEXT("y"), OutInitParams);
		PyGenUtil::AddStructInitParam(TEXT("Z"), TEXT("z"), OutInitParams);
		PyGenUtil::AddStructInitParam(TEXT("W"), TEXT("w"), OutInitParams);
	});
}

void InitializePyWrapperLinearColor(PyObject* PyModule)
{
	struct FConstants
	{
		static PyObject* LinearColorGetter(const void* InValuePtr)
		{
			const FLinearColor* LinearColorPtr = static_cast<const FLinearColor*>(InValuePtr);
			return PyConversion::PythonizeStruct(*LinearColorPtr);
		}
	};

	static FPyConstantDef PyConstants[] = {
		{ &FLinearColor::White, &FConstants::LinearColorGetter, "WHITE", "White color" },
		{ &FLinearColor::Gray, &FConstants::LinearColorGetter, "GRAY", "Gray color" },
		{ &FLinearColor::Black, &FConstants::LinearColorGetter, "BLACK", "Black color" },
		{ &FLinearColor::Transparent, &FConstants::LinearColorGetter, "TRANSPARENT", "Transparent color" },
		{ &FLinearColor::Red, &FConstants::LinearColorGetter, "RED", "Red color" },
		{ &FLinearColor::Green, &FConstants::LinearColorGetter, "GREEN", "Green color" },
		{ &FLinearColor::Blue, &FConstants::LinearColorGetter, "BLUE", "Blue color" },
		{ &FLinearColor::Yellow, &FConstants::LinearColorGetter, "YELLOW", "Yellow color" },
		{ nullptr, nullptr, nullptr, nullptr }
	};

	InitializeAndRegisterMathType<FPyWrapperLinearColor>(PyModule, &PyWrapperLinearColorType, PyConstants, [](TArray<PyGenUtil::FGeneratedWrappedMethodParameter>& OutInitParams)
	{
		PyGenUtil::AddStructInitParam(TEXT("R"), TEXT("r"), OutInitParams);
		PyGenUtil::AddStructInitParam(TEXT("G"), TEXT("g"), OutInitParams);
		PyGenUtil::AddStructInitParam(TEXT("B"), TEXT("b"), OutInitParams);
		PyGenUtil::AddStructInitParam(TEXT("A"), TEXT("a"), OutInitParams);
	});
}

void InitializePyWrapperMath(PyObject* PyModule)
{
	InitializePyWrapperVector(PyModule);
	InitializePyWrapperVector2D(PyModule);
	InitializePyWrapperVector4(PyModule);
	InitializePyWrapperQuat(PyModule);
	InitializePyWrapperLinearColor(PyModule);
}

/** Macro used to manage the boilerplate for calling a single parameter method of a type via Python */
#define PYMATH_METHOD_ONE_PARAM(WRAPPER_TYPE, FUNC_NAME, FUNC_PATTERN, PARAM_TYPE, PARAM_TYPE_STRING, PARAM_NAME, PARAM_DEFAULT, PARAM_CONV_FUNC, RET_CONV_FUNC)											\
	static PyObject* FUNC_NAME(WRAPPER_TYPE* InSelf, PyObject* InArgs, PyObject* InKwds)																													\
	{																																																		\
		PyObject* PyObj = nullptr;																																											\
																																																			\
		static const char *ArgsKwdList[] = { PARAM_NAME, nullptr };																																			\
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, FUNC_PATTERN, (char**)ArgsKwdList, &PyObj))																										\
		{																																																	\
			return nullptr;																																													\
		}																																																	\
																																																			\
		PARAM_TYPE Param = PARAM_DEFAULT;																																									\
		if (PyObj && !PARAM_CONV_FUNC(PyObj, Param))																																						\
		{																																																	\
			PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert '%s' (%s) to '%s'"), TEXT(PARAM_NAME), *PyUtil::GetFriendlyTypename(PyObj), TEXT(PARAM_TYPE_STRING))); \
			return nullptr;																																													\
		}																																																	\
																																																			\
		const auto Result = WRAPPER_TYPE::GetTypedStruct(InSelf).FUNC_NAME(Param);																															\
		return RET_CONV_FUNC(Result);																																										\
	}

/** Macro used to manage the boilerplate for calling a static function that takes two parameters (one the self, one passed) on a type via Python */
#define PYMATH_SFUNC_ONE_PARAM(WRAPPER_TYPE, FUNC_NAME, FUNC_PATTERN, PARAM_TYPE, PARAM_TYPE_STRING, PARAM_NAME, PARAM_DEFAULT, PARAM_CONV_FUNC, RET_CONV_FUNC)												\
	static PyObject* FUNC_NAME(WRAPPER_TYPE* InSelf, PyObject* InArgs, PyObject* InKwds)																													\
	{																																																		\
		PyObject* PyObj = nullptr;																																											\
																																																			\
		static const char *ArgsKwdList[] = { PARAM_NAME, nullptr };																																			\
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, FUNC_PATTERN, (char**)ArgsKwdList, &PyObj))																										\
		{																																																	\
			return nullptr;																																													\
		}																																																	\
																																																			\
		PARAM_TYPE Param = PARAM_DEFAULT;																																									\
		if (PyObj && !PARAM_CONV_FUNC(PyObj, Param))																																						\
		{																																																	\
			PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert '%s' (%s) to '%s'"), TEXT(PARAM_NAME), *PyUtil::GetFriendlyTypename(PyObj), TEXT(PARAM_TYPE_STRING))); \
			return nullptr;																																													\
		}																																																	\
																																																			\
		const auto Result = WRAPPER_TYPE::WrappedType::FUNC_NAME(WRAPPER_TYPE::GetTypedStruct(InSelf), Param);																								\
		return RET_CONV_FUNC(Result);																																										\
	}

namespace PyWrapperMathTypeNumberFuncs
{

#define DEFINE_OPERATION(NAME, OP)											\
	struct NAME																\
	{																		\
		template <typename ReturnType, typename LHSType, typename RHSType>	\
		static ReturnType Apply(const LHSType& InLHS, const RHSType& InRHS)	\
		{																	\
			return InLHS OP InRHS;											\
		}																	\
	}

DEFINE_OPERATION(FAddOperation, +);
DEFINE_OPERATION(FInlineAddOperation, +=);
DEFINE_OPERATION(FSubOperation, -);
DEFINE_OPERATION(FInlineSubOperation, -=);
DEFINE_OPERATION(FMulOperation, *);
DEFINE_OPERATION(FInlineMulOperation, *=);
DEFINE_OPERATION(FDivOperation, /);
DEFINE_OPERATION(FInlineDivOperation, /=);
DEFINE_OPERATION(FBitwiseOrOperation, |);
DEFINE_OPERATION(FBitwiseXorOperation, ^);

#undef DEFINE_OPERATION

/** Struct(op)Struct -> Struct */
template <typename WrapperType, typename OpType>
bool StructStructOp_ReturnStruct(PyTypeObject* InStructType, WrapperType* InLHS, PyObject* InRHS, PyObject*& OutResult)
{
	typedef typename WrapperType::WrappedType WrappedType;
	typedef TPyPtr<WrapperType> WrapperTypePtr;

	WrapperTypePtr RHS = WrapperTypePtr::StealReference(WrapperType::CastPyObject(InRHS, InStructType));
	if (!RHS)
	{
		return false;
	}

	WrappedType Result = OpType::template Apply<WrappedType, WrappedType, WrappedType>(WrapperType::GetTypedStruct(InLHS), WrapperType::GetTypedStruct(RHS));
	return PyConversion::PythonizeStruct(Result, OutResult);
}

/** Struct(op=)Struct -> Struct */
template <typename WrapperType, typename OpType>
bool StructStructInlineOp_ReturnStruct(PyTypeObject* InStructType, WrapperType* InLHS, PyObject* InRHS, PyObject*& OutResult)
{
	typedef typename WrapperType::WrappedType WrappedType;
	typedef TPyPtr<WrapperType> WrapperTypePtr;

	WrapperTypePtr RHS = WrapperTypePtr::StealReference(WrapperType::CastPyObject(InRHS, InStructType));
	if (!RHS)
	{
		return false;
	}

	OpType::template Apply<WrappedType, WrappedType, WrappedType>(WrapperType::GetTypedStruct(InLHS), WrapperType::GetTypedStruct(RHS));

	Py_INCREF(InLHS);
	OutResult = (PyObject*)InLHS;
	return true;
}

/** Struct(op)Struct -> Intrinsic */
template <typename WrapperType, typename IntrinsicType, typename OpType>
bool StructStructOp_ReturnIntrinsic(PyTypeObject* InStructType, WrapperType* InLHS, PyObject* InRHS, PyObject*& OutResult)
{
	typedef typename WrapperType::WrappedType WrappedType;
	typedef TPyPtr<WrapperType> WrapperTypePtr;

	WrapperTypePtr RHS = WrapperTypePtr::StealReference(WrapperType::CastPyObject(InRHS, InStructType));
	if (!RHS)
	{
		return false;
	}

	IntrinsicType Result = OpType::template Apply<IntrinsicType, WrappedType, WrappedType>(WrapperType::GetTypedStruct(InLHS), WrapperType::GetTypedStruct(RHS));
	return PyConversion::Pythonize(Result, OutResult);
}

/** Struct(op)Intrinsic -> Struct */
template <typename WrapperType, typename IntrinsicType, typename OpType>
bool StructIntrinsicOp_ReturnStruct(PyTypeObject* InStructType, WrapperType* InLHS, PyObject* InRHS, PyObject*& OutResult)
{
	typedef typename WrapperType::WrappedType WrappedType;

	IntrinsicType RHS;
	if (!PyConversion::Nativize(InRHS, RHS, PyConversion::ESetErrorState::No))
	{
		return false;
	}

	WrappedType Result = OpType::template Apply<WrappedType, WrappedType, IntrinsicType>(WrapperType::GetTypedStruct(InLHS), RHS);
	return PyConversion::PythonizeStruct(Result, OutResult);
}

/** Struct(op=)Intrinsic -> Struct */
template <typename WrapperType, typename IntrinsicType, typename OpType>
bool StructIntrinsicInlineOp_ReturnStruct(PyTypeObject* InStructType, WrapperType* InLHS, PyObject* InRHS, PyObject*& OutResult)
{
	typedef typename WrapperType::WrappedType WrappedType;
	
	IntrinsicType RHS;
	if (!PyConversion::Nativize(InRHS, RHS, PyConversion::ESetErrorState::No))
	{
		return false;
	}

	OpType::template Apply<WrappedType, WrappedType, IntrinsicType>(WrapperType::GetTypedStruct(InLHS), RHS);

	Py_INCREF(InLHS);
	OutResult = (PyObject*)InLHS;
	return true;
}

/** Struct(op)Intrinsic -> Intrinsic */
template <typename WrapperType, typename IntrinsicType, typename OpType>
bool StructIntrinsicOp_ReturnIntrinsic(PyTypeObject* InStructType, WrapperType* InLHS, PyObject* InRHS, PyObject*& OutResult)
{
	typedef typename WrapperType::WrappedType WrappedType;

	IntrinsicType RHS;
	if (!PyConversion::Nativize(InRHS, RHS, PyConversion::ESetErrorState::No))
	{
		return false;
	}

	IntrinsicType Result = OpType::template Apply<IntrinsicType, WrappedType, IntrinsicType>(WrapperType::GetTypedStruct(InLHS), RHS);
	return PyConversion::Pythonize(Result, OutResult);
}

typedef bool(*OpStackFuncPtr)(PyTypeObject*, PyObject*, PyObject*, PyObject*&);

template <typename WrapperType>
PyObject* EvaulateOperatorStack(PyObject* InLHS, PyObject* InRHS, OpStackFuncPtr* OpStackFuncPtrArray)
{
	typedef typename WrapperType::WrappedType WrappedType;
	typedef TPyPtr<WrapperType> WrapperTypePtr;

	PyTypeObject* PyStructType = FPyWrapperTypeRegistry::Get().GetWrappedStructType(TBaseStructure<WrappedType>::Get());
	if (PyObject_IsInstance(InLHS, (PyObject*)PyStructType) == 1)
	{
		while (*OpStackFuncPtrArray)
		{
			PyObject* Result = nullptr;
			if ((*OpStackFuncPtrArray)(PyStructType, InLHS, InRHS, Result))
			{
				return Result;
			}
			++OpStackFuncPtrArray;
		}
	}

	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}

template <typename WrapperType, typename OpType>
PyObject* EvaulateOperatorStack_Struct_ReturnStruct(PyObject* InLHS, PyObject* InRHS)
{
	static OpStackFuncPtr OpStack[] = {
		(OpStackFuncPtr)&StructStructOp_ReturnStruct<WrapperType, OpType>,
		nullptr
	};

	return EvaulateOperatorStack<WrapperType>(InLHS, InRHS, OpStack);
}

template <typename WrapperType, typename OpType>
PyObject* EvaulateOperatorStack_Float_ReturnStruct(PyObject* InLHS, PyObject* InRHS)
{
	static OpStackFuncPtr OpStack[] = {
		(OpStackFuncPtr)&StructIntrinsicOp_ReturnStruct<WrapperType, float, OpType>,
		nullptr
	};

	return EvaulateOperatorStack<WrapperType>(InLHS, InRHS, OpStack);
}

template <typename WrapperType, typename OpType>
PyObject* EvaulateOperatorStack_StructOrFloat_ReturnStruct(PyObject* InLHS, PyObject* InRHS)
{
	static OpStackFuncPtr OpStack[] = {
		(OpStackFuncPtr)&StructStructOp_ReturnStruct<WrapperType, OpType>,
		(OpStackFuncPtr)&StructIntrinsicOp_ReturnStruct<WrapperType, float, OpType>,
		nullptr
	};

	return EvaulateOperatorStack<WrapperType>(InLHS, InRHS, OpStack);
}

template <typename WrapperType, typename OpType>
PyObject* EvaulateOperatorStack_Struct_ReturnFloat(PyObject* InLHS, PyObject* InRHS)
{
	static OpStackFuncPtr OpStack[] = {
		(OpStackFuncPtr)&StructStructOp_ReturnIntrinsic<WrapperType, float, OpType>,
		nullptr
	};

	return EvaulateOperatorStack<WrapperType>(InLHS, InRHS, OpStack);
}

template <typename WrapperType, typename OpType>
PyObject* EvaulateOperatorStack_Inline_Struct_ReturnStruct(PyObject* InLHS, PyObject* InRHS)
{
	static OpStackFuncPtr OpStack[] = {
		(OpStackFuncPtr)&StructStructInlineOp_ReturnStruct<WrapperType, OpType>,
		nullptr
	};

	return EvaulateOperatorStack<WrapperType>(InLHS, InRHS, OpStack);
}

template <typename WrapperType, typename OpType>
PyObject* EvaulateOperatorStack_Inline_Float_ReturnStruct(PyObject* InLHS, PyObject* InRHS)
{
	static OpStackFuncPtr OpStack[] = {
		(OpStackFuncPtr)&StructIntrinsicInlineOp_ReturnStruct<WrapperType, float, OpType>,
		nullptr
	};

	return EvaulateOperatorStack<WrapperType>(InLHS, InRHS, OpStack);
}

template <typename WrapperType, typename OpType>
PyObject* EvaulateOperatorStack_Inline_StructOrFloat_ReturnStruct(PyObject* InLHS, PyObject* InRHS)
{
	static OpStackFuncPtr OpStack[] = {
		(OpStackFuncPtr)&StructStructInlineOp_ReturnStruct<WrapperType, OpType>,
		(OpStackFuncPtr)&StructIntrinsicInlineOp_ReturnStruct<WrapperType, float, OpType>,
		nullptr
	};

	return EvaulateOperatorStack<WrapperType>(InLHS, InRHS, OpStack);
}

}

template <typename WrapperType>
PyTypeObject InitializePyWrapperMathType_Common(const char* InTypeName, const char* InTypeDoc)
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)WrapperType::New(InType);
		}

		static void Dealloc(WrapperType* InSelf)
		{
			WrapperType::Free(InSelf);
		}

		static int Init(WrapperType* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			const int SuperResult = WrapperType::Init(InSelf);
			if (SuperResult != 0)
			{
				return SuperResult;
			}

			return FPyWrapperStruct::SetPropertyValues(InSelf, InArgs, InKwds);
		}
	};

	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		InTypeName, /* tp_name */
		sizeof(WrapperType), /* tp_basicsize */
	};

	PyType.tp_base = &PyWrapperStructType;
	PyType.tp_new = (newfunc)&FFuncs::New;
	PyType.tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType.tp_init = (initproc)&FFuncs::Init;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	PyType.tp_doc = InTypeDoc;

	return PyType;
}

PyTypeObject InitializePyWrapperMathType_Vector()
{
	PyTypeObject PyType = InitializePyWrapperMathType_Common<FPyWrapperVector>("Vector", "3D vector (X, Y, Z)");

#if PY_MAJOR_VERSION < 3
	PyType.tp_flags |= Py_TPFLAGS_CHECKTYPES;
#endif	// PY_MAJOR_VERSION < 3

	using namespace PyWrapperMathTypeNumberFuncs;

	struct FMethods
	{
		static PyObject* Set(FPyWrapperVector* InSelf, PyObject* InArgs)
		{
			PyObject* PyVectorObj = nullptr;

			if (!PyArg_ParseTuple(InArgs, "O:set", &PyVectorObj))
			{
				return nullptr;
			}

			FVector Other;
			if (!PyConversion::NativizeStruct(PyVectorObj, Other))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'vector' (%s) to 'Vector'"), *PyUtil::GetFriendlyTypename(PyVectorObj)));
				return nullptr;
			}

			FPyWrapperVector::GetTypedStruct(InSelf) = Other;
			Py_RETURN_NONE;
		}

		static PyObject* SetComponents(FPyWrapperVector* InSelf, PyObject* InArgs)
		{
			PyObject* PyXObj = nullptr;
			PyObject* PyYObj = nullptr;
			PyObject* PyZObj = nullptr;

			if (!PyArg_ParseTuple(InArgs, "OOO:set_components", &PyXObj, &PyYObj, &PyZObj))
			{
				return nullptr;
			}

			float X = 0.0f;
			if (!PyConversion::Nativize(PyXObj, X))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'x' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyXObj)));
				return nullptr;
			}

			float Y = 0.0f;
			if (!PyConversion::Nativize(PyYObj, Y))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'y' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyYObj)));
				return nullptr;
			}

			float Z = 0.0f;
			if (!PyConversion::Nativize(PyZObj, Z))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'z' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyZObj)));
				return nullptr;
			}

			FPyWrapperVector::GetTypedStruct(InSelf).Set(X, Y, Z);
			Py_RETURN_NONE;
		}

		static PyObject* Equals(FPyWrapperVector* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyVectorObj = nullptr;
			PyObject* PyToleranceObj = nullptr;
			
			static const char *ArgsKwdList[] = { "vector", "tolerance", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|O:equals", (char**)ArgsKwdList, &PyVectorObj, &PyToleranceObj))
			{
				return nullptr;
			}

			FVector Other;
			if (!PyConversion::NativizeStruct(PyVectorObj, Other))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'vector' (%s) to 'Vector'"), *PyUtil::GetFriendlyTypename(PyVectorObj)));
				return nullptr;
			}

			float Tolerance = KINDA_SMALL_NUMBER;
			if (PyToleranceObj && !PyConversion::Nativize(PyToleranceObj, Tolerance))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'tolerance' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyToleranceObj)));
				return nullptr;
			}

			const bool bResult = FPyWrapperVector::GetTypedStruct(InSelf).Equals(Other, Tolerance);
			return PyConversion::Pythonize(bResult);
		}

		static PyObject* AddBounded(FPyWrapperVector* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyVectorObj = nullptr;
			PyObject* PyRadiusObj = nullptr;

			static const char *ArgsKwdList[] = { "vector", "radius", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|O:add_bounded", (char**)ArgsKwdList, &PyVectorObj, &PyRadiusObj))
			{
				return nullptr;
			}

			FVector Vector;
			if (!PyConversion::NativizeStruct(PyVectorObj, Vector))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'vector' (%s) to 'Vector'"), *PyUtil::GetFriendlyTypename(PyVectorObj)));
				return nullptr;
			}

			float Radius = MAX_int16;
			if (PyRadiusObj && !PyConversion::Nativize(PyRadiusObj, Radius))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'radius' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyRadiusObj)));
				return nullptr;
			}

			FPyWrapperVector::GetTypedStruct(InSelf).AddBounded(Vector, Radius);
			Py_RETURN_NONE;
		}

		static PyObject* GetClampedToSize(FPyWrapperVector* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyMinObj = nullptr;
			PyObject* PyMaxObj = nullptr;

			static const char *ArgsKwdList[] = { "min", "max", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "OO:get_clamped_to_size", (char**)ArgsKwdList, &PyMinObj, &PyMaxObj))
			{
				return nullptr;
			}

			float Min = 0.0f;
			if (!PyConversion::Nativize(PyMinObj, Min))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'min' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyMinObj)));
				return nullptr;
			}

			float Max = 0.0f;
			if (!PyConversion::Nativize(PyMaxObj, Max))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'max' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyMaxObj)));
				return nullptr;
			}

			const FVector Result = FPyWrapperVector::GetTypedStruct(InSelf).GetClampedToSize(Min, Max);
			return PyConversion::PythonizeStruct(Result);
		}

		static PyObject* GetClampedToSize2D(FPyWrapperVector* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyMinObj = nullptr;
			PyObject* PyMaxObj = nullptr;

			static const char *ArgsKwdList[] = { "min", "max", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "OO:get_clamped_to_size_2d", (char**)ArgsKwdList, &PyMinObj, &PyMaxObj))
			{
				return nullptr;
			}

			float Min = 0.0f;
			if (!PyConversion::Nativize(PyMinObj, Min))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'min' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyMinObj)));
				return nullptr;
			}

			float Max = 0.0f;
			if (!PyConversion::Nativize(PyMaxObj, Max))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'max' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyMaxObj)));
				return nullptr;
			}

			const FVector Result = FPyWrapperVector::GetTypedStruct(InSelf).GetClampedToSize2D(Min, Max);
			return PyConversion::PythonizeStruct(Result);
		}

		static PyObject* RotateAngleAxis(FPyWrapperVector* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyAngleDegObj = nullptr;
			PyObject* PyAxisObj = nullptr;

			static const char *ArgsKwdList[] = { "angle_deg", "axis", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "OO:rotate_angle_axis", (char**)ArgsKwdList, &PyAngleDegObj, &PyAxisObj))
			{
				return nullptr;
			}

			float AngleDeg = 0.0f;
			if (!PyConversion::Nativize(PyAngleDegObj, AngleDeg))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'min' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyAngleDegObj)));
				return nullptr;
			}

			FVector Axis;
			if (!PyConversion::NativizeStruct(PyAxisObj, Axis))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'max' (%s) to 'Vector'"), *PyUtil::GetFriendlyTypename(PyAxisObj)));
				return nullptr;
			}

			const FVector Result = FPyWrapperVector::GetTypedStruct(InSelf).RotateAngleAxis(AngleDeg, Axis);
			return PyConversion::PythonizeStruct(Result);
		}

		static PyObject* ToDegrees(FPyWrapperVector* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			const FVector Result = FVector::RadiansToDegrees(FPyWrapperVector::GetTypedStruct(InSelf));
			return PyConversion::PythonizeStruct(Result);
		}

		static PyObject* ToRadians(FPyWrapperVector* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			const FVector Result = FVector::DegreesToRadians(FPyWrapperVector::GetTypedStruct(InSelf));
			return PyConversion::PythonizeStruct(Result);
		}

		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, AllComponentsEqual, "|O:all_components_equal", float, "float", "tolerance", KINDA_SMALL_NUMBER, PyConversion::Nativize, PyConversion::Pythonize);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, ComponentMin, "O:component_min", FVector, "Vector", "vector", FVector(), PyConversion::NativizeStruct, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, ComponentMax, "O:component_max", FVector, "Vector", "vector", FVector(), PyConversion::NativizeStruct, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, IsNearlyZero, "|O:is_nearly_zero", float, "float", "tolerance", KINDA_SMALL_NUMBER, PyConversion::Nativize, PyConversion::Pythonize);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, IsUniform, "|O:is_uniform", float, "float", "tolerance", KINDA_SMALL_NUMBER, PyConversion::Nativize, PyConversion::Pythonize);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, IsUnit, "|O:is_unit", float, "float", "tolerance", KINDA_SMALL_NUMBER, PyConversion::Nativize, PyConversion::Pythonize);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, Normalize, "|O:normalize", float, "float", "tolerance", SMALL_NUMBER, PyConversion::Nativize, PyConversion::Pythonize);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, GetSafeNormal, "|O:get_safe_normal", float, "float", "tolerance", SMALL_NUMBER, PyConversion::Nativize, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, GetSafeNormal2D, "|O:get_safe_normal_2d", float, "float", "tolerance", SMALL_NUMBER, PyConversion::Nativize, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, GridSnap, "O:grid_snap", float, "float", "grid_size", 0.0f, PyConversion::Nativize, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, BoundToCube, "O:bound_to_cube", float, "float", "radius", 0.0f, PyConversion::Nativize, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, GetClampedToMaxSize, "O:get_clamped_to_max_size", float, "float", "max_size", 0.0f, PyConversion::Nativize, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, GetClampedToMaxSize2D, "O:get_clamped_to_max_size_2d", float, "float", "max_size", 0.0f, PyConversion::Nativize, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, MirrorByVector, "O:mirror_by_vector", FVector, "Vector", "mirror_normal", FVector(), PyConversion::NativizeStruct, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, MirrorByPlane, "O:mirror_by_plane", FPlane, "Plane", "plane", FPlane(), PyConversion::NativizeStruct, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, ProjectOnTo, "O:project_on_to", FVector, "Vector", "vector", FVector(), PyConversion::NativizeStruct, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, ProjectOnToNormal, "O:project_on_to_normal", FVector, "Vector", "normal", FVector(), PyConversion::NativizeStruct, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector, CosineAngle2D, "O:cosine_angle_2d", FVector, "Vector", "vector", FVector(), PyConversion::NativizeStruct, PyConversion::Pythonize);

		PYMATH_SFUNC_ONE_PARAM(FPyWrapperVector, Dist, "O:dist", FVector, "Vector", "other", FVector(), PyConversion::NativizeStruct, PyConversion::Pythonize);
		PYMATH_SFUNC_ONE_PARAM(FPyWrapperVector, Dist2D, "O:dist_2d", FVector, "Vector", "other", FVector(), PyConversion::NativizeStruct, PyConversion::Pythonize);
		PYMATH_SFUNC_ONE_PARAM(FPyWrapperVector, DistSquared, "O:dist_squared", FVector, "Vector", "other", FVector(), PyConversion::NativizeStruct, PyConversion::Pythonize);
		PYMATH_SFUNC_ONE_PARAM(FPyWrapperVector, DistSquared2D, "O:dist_squared_2d", FVector, "Vector", "other", FVector(), PyConversion::NativizeStruct, PyConversion::Pythonize);
	};

	static PyMethodDef PyMethods[] = {
		{ "set", PyCFunctionCast(&FMethods::Set), METH_VARARGS, "x.set(vector) -- set the value of this vector" },
		{ "set_components", PyCFunctionCast(&FMethods::SetComponents), METH_VARARGS, "x.set_components(x, y, z) -- set the component values of this vector" },
		{ "equals", PyCFunctionCast(&FMethods::Equals), METH_VARARGS | METH_KEYWORDS, "x.equals(vector, tolerance=KINDA_SMALL_NUMBER) -> bool -- check against another vector for equality, within specified error limits" },
		{ "all_components_equal", PyCFunctionCast(&FMethods::AllComponentsEqual), METH_VARARGS | METH_KEYWORDS, "x.all_components_equal(tolerance=KINDA_SMALL_NUMBER) -> bool -- checks whether all components of this vector are the same, within a tolerance" },
		{ "component_min", PyCFunctionCast(&FMethods::ComponentMin), METH_VARARGS, "x.component_min(vector) -> Vector -- get the component-wise min of two vectors" },
		{ "component_max", PyCFunctionCast(&FMethods::ComponentMax), METH_VARARGS, "x.component_max(vector) -> Vector -- get the component-wise max of two vectors" },
		{ "is_zero", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_IntrinsicReturn<bool, &FVector::IsZero>)), METH_NOARGS, "x.is_zero() -> bool -- check whether all components of the vector are exactly zero" },
		{ "is_nearly_zero", PyCFunctionCast(&FMethods::IsNearlyZero), METH_VARARGS | METH_KEYWORDS, "x.is_nearly_zero(tolerance=KINDA_SMALL_NUMBER) -> bool -- checks whether vector is near to zero within a specified tolerance" },
		{ "is_uniform", PyCFunctionCast(&FMethods::IsUniform), METH_VARARGS | METH_KEYWORDS, "x.is_uniform(tolerance=KINDA_SMALL_NUMBER) -> bool -- checks whether X, Y and Z are nearly equal" },
		{ "is_unit", PyCFunctionCast(&FMethods::IsUnit), METH_VARARGS | METH_KEYWORDS, "x.is_unit(tolerance=KINDA_SMALL_NUMBER) -> bool -- check if the vector is of unit length, with specified tolerance" },
		{ "is_normalized", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_IntrinsicReturn<bool, &FVector::IsNormalized>)), METH_NOARGS, "x.is_normalized() -> bool -- check whether the vector is normalized" },
		{ "normalize", PyCFunctionCast(&FMethods::Normalize), METH_VARARGS | METH_KEYWORDS, "x.normalize(tolerance=SMALL_NUMBER) -> bool -- normalize this vector in-place if it is larger than a given tolerance. Leaves it unchanged if not" },
		{ "get_unsafe_normal", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_StructReturn<FVector, &FVector::GetUnsafeNormal>)), METH_NOARGS, "x.get_unsafe_normal() -> Vector -- calculates normalized version of vector without checking for zero length" },
		{ "get_safe_normal", PyCFunctionCast(&FMethods::GetSafeNormal), METH_VARARGS | METH_KEYWORDS, "x.get_safe_normal(tolerance=SMALL_NUMBER) -> Vector -- get a normalized copy of the vector, checking it is safe to do so based on the length, or zero vector if vector length is too small to safely normalize" },
		{ "get_safe_normal_2d", PyCFunctionCast(&FMethods::GetSafeNormal2D), METH_VARARGS | METH_KEYWORDS, "x.get_safe_normal_2d(tolerance=SMALL_NUMBER) -> Vector -- get a normalized copy of the 2D components of the vector, checking it is safe to do so, or a zero vector if vector length is too small to normalize (Z is set to zero)" },
		{ "get_max", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector::GetMax>)), METH_NOARGS, "x.get_max() -> float -- get the maximum value of the vector's components" },
		{ "get_abs_max", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector::GetAbsMax>)), METH_NOARGS, "x.get_abs_max() -> float -- get the maximum absolute value of the vector's components" },
		{ "get_min", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector::GetMin>)), METH_NOARGS, "x.get_min() -> float -- get the minimum value of the vector's components" },
		{ "get_abs_min", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector::GetAbsMin>)), METH_NOARGS, "x.get_abs_min() -> float -- get the minimum absolute value of the vector's components" },
		{ "get_abs", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_StructReturn<FVector, &FVector::GetAbs>)), METH_NOARGS, "x.get_abs() -> Vector -- get a copy of this vector with absolute value of each component" },
		{ "size", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector::Size>)), METH_NOARGS, "x.size() -> float -- get the length (magnitude) of this vector" },
		{ "size_squared", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector::SizeSquared>)), METH_NOARGS, "x.size_squared() -> float -- get the squared length of this vector" },
		{ "size_2d", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector::Size2D>)), METH_NOARGS, "x.size_2d() -> float -- get the length (magnitude) of the 2D components of this vector" },
		{ "size_squared_2d", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector::SizeSquared2D>)), METH_NOARGS, "x.size_squared_2d() -> float -- get the squared length of the 2D components of this vector" },
		{ "get_sign_vector", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_StructReturn<FVector, &FVector::GetSignVector>)), METH_NOARGS, "x.get_sign_vector() -> Vector -- get a copy of this vector as sign only (each component is set to + 1 or -1, with the sign of zero treated as + 1)" },
		{ "projection", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_StructReturn<FVector, &FVector::Projection>)), METH_NOARGS, "x.projection() -> Vector -- projects 2D components of vector based on Z" },
		{ "reciprocal", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_StructReturn<FVector, &FVector::Reciprocal>)), METH_NOARGS, "x.reciprocal() -> Vector -- get the reciprocal of this vector, avoiding division by zero (zero components are set to BIG_NUMBER)" },
		{ "to_degrees", PyCFunctionCast(&FMethods::ToDegrees), METH_NOARGS, "x.to_degrees() -> Vector -- get a vector containing degree values from this vector (containing radian values)" },
		{ "to_radians", PyCFunctionCast(&FMethods::ToRadians), METH_NOARGS, "x.to_radians() -> Vector -- get a vector containing radian values from this vector (containing degree values)" },
		{ "to_orientation_rotator", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_StructReturn<FRotator, &FVector::ToOrientationRotator>)), METH_NOARGS, "x.to_orientation_rotator() -> Rotator -- get the Rotator orientation corresponding to the direction in which the vector points" },
		{ "to_orientation_quat", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_StructReturn<FQuat, &FVector::ToOrientationQuat>)), METH_NOARGS, "x.to_orientation_quat() -> Quat -- get the Quat orientation corresponding to the direction in which the vector points" },
		{ "unit_cartesian_to_spherical", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_StructReturn<FVector2D, &FVector::UnitCartesianToSpherical>)), METH_NOARGS, "x.unit_cartesian_to_spherical() -> Vector2D -- converts a Cartesian unit vector into spherical coordinates on the unit sphere (output Theta will be in the range [0, PI], and output Phi will be in the range [-PI, PI])" },
		{ "heading_angle", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector::HeadingAngle>)), METH_NOARGS, "x.heading_angle() -> float -- convert a direction vector into a 'heading' angle (between +/-PI. 0 is pointing down +X)" },
		{ "contains_nan", PyCFunctionCast((&FPyWrapperVector::CallConstFunc_NoParams_IntrinsicReturn<bool, &FVector::ContainsNaN>)), METH_NOARGS, "x.contains_nan() -> bool -- check if there are any non-finite values (NaN or Inf) in this vector" },
		{ "unwind_euler", PyCFunctionCast((&FPyWrapperVector::CallFunc_NoParams_NoReturn<&FVector::UnwindEuler>)), METH_NOARGS, "x.unwind_euler() -- when this vector contains Euler angles (degrees), ensure that angles are between +/-180" },
		{ "grid_snap", PyCFunctionCast(&FMethods::GridSnap), METH_VARARGS | METH_KEYWORDS, "x.grid_snap(grid_size) -> Vector -- get a copy of this vector snapped to a grid" },
		{ "bound_to_cube", PyCFunctionCast(&FMethods::BoundToCube), METH_VARARGS | METH_KEYWORDS, "x.bound_to_cube(radius) -> Vector -- get a copy of this vector, clamped inside of a cube" },
		{ "add_bounded", PyCFunctionCast(&FMethods::AddBounded), METH_VARARGS | METH_KEYWORDS, "x.add_bounded(vector, radius=MAX_int16) -- add a vector to this and clamp the result in a cube" },
		{ "get_clamped_to_size", PyCFunctionCast(&FMethods::GetClampedToSize), METH_VARARGS | METH_KEYWORDS, "x.get_clamped_to_size(min, max) -> Vector -- get a copy of this vector, with its magnitude clamped between min and max" },
		{ "get_clamped_to_size_2d", PyCFunctionCast(&FMethods::GetClampedToSize), METH_VARARGS | METH_KEYWORDS, "x.get_clamped_to_size_2d(min, max) -> Vector -- get a copy of this vector, with the 2D magnitude clamped between min and max" },
		{ "get_clamped_to_max_size", PyCFunctionCast(&FMethods::GetClampedToMaxSize), METH_VARARGS | METH_KEYWORDS, "x.get_clamped_to_max_size(max_size) -> Vector -- get a copy of this vector, with its maximum magnitude clamped between min and max" },
		{ "get_clamped_to_max_size_2d", PyCFunctionCast(&FMethods::GetClampedToMaxSize2D), METH_VARARGS | METH_KEYWORDS, "x.get_clamped_to_max_size_2d(max_size) -> Vector -- get a copy of this vector, with the maximum 2D magnitude clamped between min and max" },
		{ "mirror_by_vector", PyCFunctionCast(&FMethods::MirrorByVector), METH_VARARGS | METH_KEYWORDS, "x.mirror_by_vector(mirror_normal) -> Vector -- mirror a vector about a normal vector" },
		{ "mirror_by_plane", PyCFunctionCast(&FMethods::MirrorByPlane), METH_VARARGS | METH_KEYWORDS, "x.mirror_by_plane(plane) -> Vector -- mirror a vector about a plane" },
		{ "project_on_to", PyCFunctionCast(&FMethods::ProjectOnTo), METH_VARARGS | METH_KEYWORDS, "x.project_on_to(vector) -> Vector -- get a copy of this vector projected onto the input vector" },
		{ "project_on_to_normal", PyCFunctionCast(&FMethods::ProjectOnToNormal), METH_VARARGS | METH_KEYWORDS, "x.project_on_to_normal(vector) -> Vector -- get a copy of this vector projected onto the input vector, which is assumed to be unit length" },
		{ "cosine_angle_2d", PyCFunctionCast(&FMethods::CosineAngle2D), METH_VARARGS | METH_KEYWORDS, "x.cosine_angle_2d(vector) -> float -- get the cosine of the angle between this vector and another projected onto the XY plane (no Z)" },
		{ "rotate_angle_axis", PyCFunctionCast(&FMethods::RotateAngleAxis), METH_VARARGS | METH_KEYWORDS, "x.rotate_angle_axis(angle_deg, axis) -> Vector -- rotates around Axis (assumes Axis.Size() == 1)" },
		{ "dist", PyCFunctionCast(&FMethods::Dist), METH_VARARGS | METH_KEYWORDS, "x.dist(other) -> float -- euclidean distance between this vector and another" },
		{ "dist_2d", PyCFunctionCast(&FMethods::Dist2D), METH_VARARGS | METH_KEYWORDS, "x.dist_2d(other) -> float -- euclidean distance between this vector and another in the XY plane (ignoring Z)" },
		{ "dist_squared", PyCFunctionCast(&FMethods::DistSquared), METH_VARARGS | METH_KEYWORDS, "x.dist_squared(other) -> float -- squared distance between this vector and another" },
		{ "dist_squared_2d", PyCFunctionCast(&FMethods::DistSquared2D), METH_VARARGS | METH_KEYWORDS, "x.dist_squared_2d(other) -> float -- squared distance between this vector and another in the XY plane (ignoring Z)" },
		{ nullptr, nullptr, 0, nullptr }
	};

	static PyGetSetDef PyGetSets[] = {
		{ PyCStrCast("x"), (getter)&FPyWrapperVector::IntrinsicFieldGetter<float, &FVector::X>, (setter)&FPyWrapperVector::IntrinsicFieldSetter<float, &FVector::X>, PyCStrCast("X"), nullptr },
		{ PyCStrCast("y"), (getter)&FPyWrapperVector::IntrinsicFieldGetter<float, &FVector::Y>, (setter)&FPyWrapperVector::IntrinsicFieldSetter<float, &FVector::Y>, PyCStrCast("Y"), nullptr },
		{ PyCStrCast("z"), (getter)&FPyWrapperVector::IntrinsicFieldGetter<float, &FVector::Z>, (setter)&FPyWrapperVector::IntrinsicFieldSetter<float, &FVector::Z>, PyCStrCast("Z"), nullptr },
		{ nullptr, nullptr, nullptr, nullptr, nullptr }
	};

	static PyNumberMethods PyNumber;
	PyNumber.nb_add = (binaryfunc)&EvaulateOperatorStack_StructOrFloat_ReturnStruct<FPyWrapperVector, FAddOperation>;
	PyNumber.nb_inplace_add = (binaryfunc)&EvaulateOperatorStack_Inline_Struct_ReturnStruct<FPyWrapperVector, FAddOperation>;
	PyNumber.nb_subtract = (binaryfunc)&EvaulateOperatorStack_StructOrFloat_ReturnStruct<FPyWrapperVector, FSubOperation>;
	PyNumber.nb_inplace_subtract = (binaryfunc)&EvaulateOperatorStack_Inline_Struct_ReturnStruct<FPyWrapperVector, FSubOperation>;
	PyNumber.nb_multiply = (binaryfunc)&EvaulateOperatorStack_StructOrFloat_ReturnStruct<FPyWrapperVector, FMulOperation>;
	PyNumber.nb_inplace_multiply = (binaryfunc)&EvaulateOperatorStack_Inline_StructOrFloat_ReturnStruct<FPyWrapperVector, FMulOperation>;
#if PY_MAJOR_VERSION >= 3
	PyNumber.nb_true_divide = (binaryfunc)&EvaulateOperatorStack_StructOrFloat_ReturnStruct<FPyWrapperVector, FDivOperation>;
	PyNumber.nb_inplace_true_divide = (binaryfunc)&EvaulateOperatorStack_Inline_StructOrFloat_ReturnStruct<FPyWrapperVector, FDivOperation>;
#else	// PY_MAJOR_VERSION >= 3
	PyNumber.nb_divide = (binaryfunc)&EvaulateOperatorStack_StructOrFloat_ReturnStruct<FPyWrapperVector, FDivOperation>;
	PyNumber.nb_inplace_divide = (binaryfunc)&EvaulateOperatorStack_Inline_StructOrFloat_ReturnStruct<FPyWrapperVector, FDivOperation>;
#endif	// PY_MAJOR_VERSION >= 3
	PyNumber.nb_or = (binaryfunc)&EvaulateOperatorStack_Struct_ReturnFloat<FPyWrapperVector, FBitwiseOrOperation>;
	PyNumber.nb_xor = (binaryfunc)&EvaulateOperatorStack_Struct_ReturnStruct<FPyWrapperVector, FBitwiseXorOperation>;

	PyType.tp_methods = PyMethods;
	PyType.tp_getset = PyGetSets;
	PyType.tp_as_number = &PyNumber;

	return PyType;
}

PyTypeObject InitializePyWrapperMathType_Vector2D()
{
	PyTypeObject PyType = InitializePyWrapperMathType_Common<FPyWrapperVector>("Vector2D", "2D vector (X, Y)");

#if PY_MAJOR_VERSION < 3
	PyType.tp_flags |= Py_TPFLAGS_CHECKTYPES;
#endif	// PY_MAJOR_VERSION < 3

	using namespace PyWrapperMathTypeNumberFuncs;

	struct FMethods
	{
		static PyObject* Set(FPyWrapperVector2D* InSelf, PyObject* InArgs)
		{
			PyObject* PyVectorObj = nullptr;

			if (!PyArg_ParseTuple(InArgs, "O:set", &PyVectorObj))
			{
				return nullptr;
			}

			FVector2D Other;
			if (!PyConversion::NativizeStruct(PyVectorObj, Other))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'vector' (%s) to 'Vector2D'"), *PyUtil::GetFriendlyTypename(PyVectorObj)));
				return nullptr;
			}

			FPyWrapperVector2D::GetTypedStruct(InSelf) = Other;
			Py_RETURN_NONE;
		}

		static PyObject* SetComponents(FPyWrapperVector2D* InSelf, PyObject* InArgs)
		{
			PyObject* PyXObj = nullptr;
			PyObject* PyYObj = nullptr;

			if (!PyArg_ParseTuple(InArgs, "OO:set_components", &PyXObj, &PyYObj))
			{
				return nullptr;
			}

			float X = 0.0f;
			if (!PyConversion::Nativize(PyXObj, X))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'x' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyXObj)));
				return nullptr;
			}

			float Y = 0.0f;
			if (!PyConversion::Nativize(PyYObj, Y))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'y' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyYObj)));
				return nullptr;
			}

			FPyWrapperVector2D::GetTypedStruct(InSelf).Set(X, Y);
			Py_RETURN_NONE;
		}

		static PyObject* Equals(FPyWrapperVector2D* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyVectorObj = nullptr;
			PyObject* PyToleranceObj = nullptr;

			static const char *ArgsKwdList[] = { "vector", "tolerance", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|O:equals", (char**)ArgsKwdList, &PyVectorObj, &PyToleranceObj))
			{
				return nullptr;
			}

			FVector2D Other;
			if (!PyConversion::NativizeStruct(PyVectorObj, Other))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'vector' (%s) to 'Vector2D'"), *PyUtil::GetFriendlyTypename(PyVectorObj)));
				return nullptr;
			}

			float Tolerance = KINDA_SMALL_NUMBER;
			if (PyToleranceObj && !PyConversion::Nativize(PyToleranceObj, Tolerance))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'tolerance' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyToleranceObj)));
				return nullptr;
			}

			const bool bResult = FPyWrapperVector2D::GetTypedStruct(InSelf).Equals(Other, Tolerance);
			return PyConversion::Pythonize(bResult);
		}

		static PyObject* Normalize(FPyWrapperVector2D* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyToleranceObj = nullptr;

			static const char *ArgsKwdList[] = { "tolerance", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "|O:normalize", (char**)ArgsKwdList, &PyToleranceObj))
			{
				return nullptr;
			}

			float Tolerance = SMALL_NUMBER;
			if (PyToleranceObj && !PyConversion::Nativize(PyToleranceObj, Tolerance))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'tolerance' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyToleranceObj)));
				return nullptr;
			}

			FPyWrapperVector2D::GetTypedStruct(InSelf).Normalize(Tolerance);
			Py_RETURN_NONE;
		}

		static PyObject* ClampAxes(FPyWrapperVector2D* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyMinObj = nullptr;
			PyObject* PyMaxObj = nullptr;

			static const char *ArgsKwdList[] = { "min", "max", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "OO:clamp_axes", (char**)ArgsKwdList, &PyMinObj, &PyMaxObj))
			{
				return nullptr;
			}

			float Min = 0.0f;
			if (!PyConversion::Nativize(PyMinObj, Min))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'min' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyMinObj)));
				return nullptr;
			}

			float Max = 0.0f;
			if (!PyConversion::Nativize(PyMaxObj, Max))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'max' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyMaxObj)));
				return nullptr;
			}

			const FVector2D Result = FPyWrapperVector2D::GetTypedStruct(InSelf).ClampAxes(Min, Max);
			return PyConversion::PythonizeStruct(Result);
		}

		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector2D, IsNearlyZero, "|O:is_nearly_zero", float, "float", "tolerance", KINDA_SMALL_NUMBER, PyConversion::Nativize, PyConversion::Pythonize);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector2D, GetSafeNormal, "|O:get_safe_normal", float, "float", "tolerance", SMALL_NUMBER, PyConversion::Nativize, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector2D, GetRotated, "O:get_rotated", float, "float", "angle_deg", 0.0f, PyConversion::Nativize, PyConversion::PythonizeStruct);

		PYMATH_SFUNC_ONE_PARAM(FPyWrapperVector2D, Distance, "O:dist", FVector2D, "Vector2D", "other", FVector2D(), PyConversion::NativizeStruct, PyConversion::Pythonize);
		PYMATH_SFUNC_ONE_PARAM(FPyWrapperVector2D, DistSquared, "O:dist_squared", FVector2D, "Vector2D", "other", FVector2D(), PyConversion::NativizeStruct, PyConversion::Pythonize);
	};

	static PyMethodDef PyMethods[] = {
		{ "set", PyCFunctionCast(&FMethods::Set), METH_VARARGS, "x.set(vector) -- set the value of this vector" },
		{ "set_components", PyCFunctionCast(&FMethods::SetComponents), METH_VARARGS, "x.set_components(x, y) -- set the component values of this vector" },
		{ "equals", PyCFunctionCast(&FMethods::Equals), METH_VARARGS | METH_KEYWORDS, "x.equals(vector, tolerance=KINDA_SMALL_NUMBER) -> bool -- check against another vector for equality, within specified error limits" },
		{ "is_zero", PyCFunctionCast((&FPyWrapperVector2D::CallConstFunc_NoParams_IntrinsicReturn<bool, &FVector2D::IsZero>)), METH_NOARGS, "x.is_zero() -> bool -- check whether all components of the vector are exactly zero" },
		{ "is_nearly_zero", PyCFunctionCast(&FMethods::IsNearlyZero), METH_VARARGS | METH_KEYWORDS, "x.is_nearly_zero(tolerance=KINDA_SMALL_NUMBER) -> bool -- checks whether vector is near to zero within a specified tolerance" },
		{ "get_max", PyCFunctionCast((&FPyWrapperVector2D::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector2D::GetMax>)), METH_NOARGS, "x.get_max() -> float -- get the maximum value of the vector's components" },
		{ "get_min", PyCFunctionCast((&FPyWrapperVector2D::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector2D::GetMin>)), METH_NOARGS, "x.get_min() -> float -- get the minimum value of the vector's components" },
		{ "get_abs_max", PyCFunctionCast((&FPyWrapperVector2D::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector2D::GetAbsMax>)), METH_NOARGS, "x.get_abs_max() -> float -- get the maximum absolute value of the vector's components" },
		{ "get_abs", PyCFunctionCast((&FPyWrapperVector2D::CallConstFunc_NoParams_StructReturn<FVector2D, &FVector2D::GetAbs>)), METH_NOARGS, "x.get_abs() -> Vector2D -- get a copy of this vector with absolute value of each component" },
		{ "size", PyCFunctionCast((&FPyWrapperVector2D::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector2D::Size>)), METH_NOARGS, "x.size() -> float -- get the length (magnitude) of this vector" },
		{ "size_squared", PyCFunctionCast((&FPyWrapperVector2D::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector2D::SizeSquared>)), METH_NOARGS, "x.size_squared() -> float -- get the squared length of this vector" },
		{ "get_sign_vector", PyCFunctionCast((&FPyWrapperVector2D::CallConstFunc_NoParams_StructReturn<FVector2D, &FVector2D::GetSignVector>)), METH_NOARGS, "x.get_sign_vector() -> Vector2D -- get a copy of this vector as sign only (each component is set to + 1 or -1, with the sign of zero treated as + 1)" },
		{ "get_rotated", PyCFunctionCast(&FMethods::GetRotated), METH_VARARGS | METH_KEYWORDS, "x.get_rotated() -> Vector2D -- rotate around axis (0,0,1)" },
		{ "round_to_vector", PyCFunctionCast((&FPyWrapperVector2D::CallConstFunc_NoParams_StructReturn<FVector2D, &FVector2D::RoundToVector>)), METH_NOARGS, "x.round_to_vector() -> Vector2D -- get a copy of this vector as where each component has been rounded to the nearest int" },
		{ "normalize", PyCFunctionCast(&FMethods::Normalize), METH_VARARGS | METH_KEYWORDS, "x.normalize(tolerance=SMALL_NUMBER) -> bool -- normalize this vector in-place if it is larger than a given tolerance, set it to (0,0) otherwise" },
		{ "get_safe_normal", PyCFunctionCast(&FMethods::GetSafeNormal), METH_VARARGS | METH_KEYWORDS, "x.get_safe_normal(tolerance=SMALL_NUMBER) -> Vector2D -- get a normalized copy of the vector, checking it is safe to do so based on the length, or zero vector if vector length is too small to safely normalize" },
		{ "dist", PyCFunctionCast(&FMethods::Distance), METH_VARARGS | METH_KEYWORDS, "x.dist(other) -> float -- distance between this vector and another" },
		{ "dist_squared", PyCFunctionCast(&FMethods::DistSquared), METH_VARARGS | METH_KEYWORDS, "x.dist_squared(other) -> float -- squared distance between this vector and another" },
		{ "clamp_axes", PyCFunctionCast(&FMethods::ClampAxes), METH_VARARGS | METH_KEYWORDS, "x.clamp_axes(min, max) -> Vector2D -- get a copy of this vector with both axes clamped to the given range" },
		{ nullptr, nullptr, 0, nullptr }
	};

	static PyGetSetDef PyGetSets[] = {
		{ PyCStrCast("x"), (getter)&FPyWrapperVector2D::IntrinsicFieldGetter<float, &FVector2D::X>, (setter)&FPyWrapperVector2D::IntrinsicFieldSetter<float, &FVector2D::X>, PyCStrCast("X"), nullptr },
		{ PyCStrCast("y"), (getter)&FPyWrapperVector2D::IntrinsicFieldGetter<float, &FVector2D::Y>, (setter)&FPyWrapperVector2D::IntrinsicFieldSetter<float, &FVector2D::Y>, PyCStrCast("Y"), nullptr },
		{ nullptr, nullptr, nullptr, nullptr, nullptr }
	};

	static PyNumberMethods PyNumber;
	PyNumber.nb_add = (binaryfunc)&EvaulateOperatorStack_StructOrFloat_ReturnStruct<FPyWrapperVector2D, FAddOperation>;
	PyNumber.nb_inplace_add = (binaryfunc)&EvaulateOperatorStack_Inline_Struct_ReturnStruct<FPyWrapperVector2D, FAddOperation>;
	PyNumber.nb_subtract = (binaryfunc)&EvaulateOperatorStack_StructOrFloat_ReturnStruct<FPyWrapperVector2D, FSubOperation>;
	PyNumber.nb_inplace_subtract = (binaryfunc)&EvaulateOperatorStack_Inline_Struct_ReturnStruct<FPyWrapperVector2D, FSubOperation>;
	PyNumber.nb_multiply = (binaryfunc)&EvaulateOperatorStack_StructOrFloat_ReturnStruct<FPyWrapperVector2D, FMulOperation>;
	PyNumber.nb_inplace_multiply = (binaryfunc)&EvaulateOperatorStack_Inline_StructOrFloat_ReturnStruct<FPyWrapperVector2D, FMulOperation>;
#if PY_MAJOR_VERSION >= 3
	PyNumber.nb_true_divide = (binaryfunc)&EvaulateOperatorStack_StructOrFloat_ReturnStruct<FPyWrapperVector2D, FDivOperation>;
	PyNumber.nb_inplace_true_divide = (binaryfunc)&EvaulateOperatorStack_Inline_StructOrFloat_ReturnStruct<FPyWrapperVector2D, FDivOperation>;
#else	// PY_MAJOR_VERSION >= 3
	PyNumber.nb_divide = (binaryfunc)&EvaulateOperatorStack_StructOrFloat_ReturnStruct<FPyWrapperVector2D, FDivOperation>;
	PyNumber.nb_inplace_divide = (binaryfunc)&EvaulateOperatorStack_Inline_StructOrFloat_ReturnStruct<FPyWrapperVector2D, FDivOperation>;
#endif	// PY_MAJOR_VERSION >= 3
	PyNumber.nb_or = (binaryfunc)&EvaulateOperatorStack_Struct_ReturnFloat<FPyWrapperVector2D, FBitwiseOrOperation>;
	PyNumber.nb_xor = (binaryfunc)&EvaulateOperatorStack_Struct_ReturnFloat<FPyWrapperVector2D, FBitwiseXorOperation>;

	PyType.tp_methods = PyMethods;
	PyType.tp_getset = PyGetSets;
	PyType.tp_as_number = &PyNumber;

	return PyType;
}

PyTypeObject InitializePyWrapperMathType_Vector4()
{
	PyTypeObject PyType = InitializePyWrapperMathType_Common<FPyWrapperVector>("Vector4", "4D vector (X, Y, Z, W)");

#if PY_MAJOR_VERSION < 3
	PyType.tp_flags |= Py_TPFLAGS_CHECKTYPES;
#endif	// PY_MAJOR_VERSION < 3

	using namespace PyWrapperMathTypeNumberFuncs;

	struct FMethods
	{
		static PyObject* Set(FPyWrapperVector4* InSelf, PyObject* InArgs)
		{
			PyObject* PyVectorObj = nullptr;

			if (!PyArg_ParseTuple(InArgs, "O:set", &PyVectorObj))
			{
				return nullptr;
			}

			FVector4 Other;
			if (!PyConversion::NativizeStruct(PyVectorObj, Other))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'vector' (%s) to 'Vector4'"), *PyUtil::GetFriendlyTypename(PyVectorObj)));
				return nullptr;
			}

			FPyWrapperVector4::GetTypedStruct(InSelf) = Other;
			Py_RETURN_NONE;
		}

		static PyObject* SetComponents(FPyWrapperVector4* InSelf, PyObject* InArgs)
		{
			PyObject* PyXObj = nullptr;
			PyObject* PyYObj = nullptr;
			PyObject* PyZObj = nullptr;
			PyObject* PyWObj = nullptr;

			if (!PyArg_ParseTuple(InArgs, "OOOO:set_components", &PyXObj, &PyYObj, &PyZObj, &PyWObj))
			{
				return nullptr;
			}

			float X = 0.0f;
			if (!PyConversion::Nativize(PyXObj, X))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'x' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyXObj)));
				return nullptr;
			}

			float Y = 0.0f;
			if (!PyConversion::Nativize(PyYObj, Y))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'y' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyYObj)));
				return nullptr;
			}

			float Z = 0.0f;
			if (!PyConversion::Nativize(PyZObj, Z))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'z' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyZObj)));
				return nullptr;
			}

			float W = 0.0f;
			if (!PyConversion::Nativize(PyWObj, W))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'w' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyWObj)));
				return nullptr;
			}

			FPyWrapperVector4::GetTypedStruct(InSelf).Set(X, Y, Z, W);
			Py_RETURN_NONE;
		}

		static PyObject* Equals(FPyWrapperVector4* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyVectorObj = nullptr;
			PyObject* PyToleranceObj = nullptr;

			static const char *ArgsKwdList[] = { "vector", "tolerance", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|O:equals", (char**)ArgsKwdList, &PyVectorObj, &PyToleranceObj))
			{
				return nullptr;
			}

			FVector4 Other;
			if (!PyConversion::NativizeStruct(PyVectorObj, Other))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'vector' (%s) to 'Vector4'"), *PyUtil::GetFriendlyTypename(PyVectorObj)));
				return nullptr;
			}

			float Tolerance = KINDA_SMALL_NUMBER;
			if (PyToleranceObj && !PyConversion::Nativize(PyToleranceObj, Tolerance))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'tolerance' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyToleranceObj)));
				return nullptr;
			}

			const bool bResult = FPyWrapperVector4::GetTypedStruct(InSelf).Equals(Other, Tolerance);
			return PyConversion::Pythonize(bResult);
		}

		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector4, IsNearlyZero3, "|O:is_nearly_zero3", float, "float", "tolerance", KINDA_SMALL_NUMBER, PyConversion::Nativize, PyConversion::Pythonize);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector4, IsUnit3, "|O:is_unit3", float, "float", "tolerance", KINDA_SMALL_NUMBER, PyConversion::Nativize, PyConversion::Pythonize);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector4, GetSafeNormal, "|O:get_safe_normal", float, "float", "tolerance", SMALL_NUMBER, PyConversion::Nativize, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperVector4, Reflect3, "O:reflect3", FVector4, "Vector4", "normal", FVector4(), PyConversion::NativizeStruct, PyConversion::PythonizeStruct);
	};

	static PyMethodDef PyMethods[] = {
		{ "set", PyCFunctionCast(&FMethods::Set), METH_VARARGS, "x.set(vector) -- set the value of this vector" },
		{ "set_components", PyCFunctionCast(&FMethods::SetComponents), METH_VARARGS, "x.set_components(x, y, z, w) -- set the component values of this vector" },
		{ "equals", PyCFunctionCast(&FMethods::Equals), METH_VARARGS | METH_KEYWORDS, "x.equals(vector, tolerance=KINDA_SMALL_NUMBER) -> bool -- check against another vector for equality, within specified error limits" },
		{ "is_nearly_zero3", PyCFunctionCast(&FMethods::IsNearlyZero3), METH_VARARGS | METH_KEYWORDS, "x.is_nearly_zero3(tolerance=KINDA_SMALL_NUMBER) -> bool -- checks whether vector is near to zero within a specified tolerance" },
		{ "is_unit3", PyCFunctionCast(&FMethods::IsUnit3), METH_VARARGS | METH_KEYWORDS, "x.is_unit3(tolerance=KINDA_SMALL_NUMBER) -> bool -- check if the vector is of unit length, with specified tolerance" },
		{ "get_unsafe_normal3", PyCFunctionCast((&FPyWrapperVector4::CallConstFunc_NoParams_StructReturn<FVector4, &FVector4::GetUnsafeNormal3>)), METH_NOARGS, "x.get_unsafe_normal3() -> Vector4 -- calculates normalized version of vector without checking for zero length" },
		{ "get_safe_normal", PyCFunctionCast(&FMethods::GetSafeNormal), METH_VARARGS | METH_KEYWORDS, "x.get_safe_normal(tolerance=SMALL_NUMBER) -> Vector4 -- get a normalized copy of the vector, checking it is safe to do so based on the length, or zero vector if vector length is too small to safely normalize" },
		{ "size", PyCFunctionCast((&FPyWrapperVector4::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector4::Size>)), METH_NOARGS, "x.size() -> float -- get the length (magnitude) of this vector taking the W component into account" },
		{ "size_squared", PyCFunctionCast((&FPyWrapperVector4::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector4::SizeSquared>)), METH_NOARGS, "x.size_squared() -> float -- get the squared length of this vector taking the W component into account" },
		{ "size3", PyCFunctionCast((&FPyWrapperVector4::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector4::Size3>)), METH_NOARGS, "x.size_2d() -> float -- get the length (magnitude) of this vector not taking the W component into account" },
		{ "size_squared3", PyCFunctionCast((&FPyWrapperVector4::CallConstFunc_NoParams_IntrinsicReturn<float, &FVector4::SizeSquared3>)), METH_NOARGS, "x.size_squared_2d() -> float -- get the squared length of this vector not taking the W component into account" },
		{ "to_orientation_rotator", PyCFunctionCast((&FPyWrapperVector4::CallConstFunc_NoParams_StructReturn<FRotator, &FVector4::ToOrientationRotator>)), METH_NOARGS, "x.to_orientation_rotator() -> Rotator -- get the Rotator orientation corresponding to the direction in which the vector points" },
		{ "to_orientation_quat", PyCFunctionCast((&FPyWrapperVector4::CallConstFunc_NoParams_StructReturn<FQuat, &FVector4::ToOrientationQuat>)), METH_NOARGS, "x.to_orientation_quat() -> Quat -- get the Quat orientation corresponding to the direction in which the vector points" },
		{ "contains_nan", PyCFunctionCast((&FPyWrapperVector4::CallConstFunc_NoParams_IntrinsicReturn<bool, &FVector4::ContainsNaN>)), METH_NOARGS, "x.contains_nan() -> bool -- check if there are any non-finite values (NaN or Inf) in this vector" },
		{ "reflect3", PyCFunctionCast(&FMethods::Reflect3), METH_VARARGS, "x.reflect3(normal) -> Vector4 -- reflect vector around normal" },
		{ nullptr, nullptr, 0, nullptr }
	};

	static PyGetSetDef PyGetSets[] = {
		{ PyCStrCast("x"), (getter)&FPyWrapperVector4::IntrinsicFieldGetter<float, &FVector4::X>, (setter)&FPyWrapperVector4::IntrinsicFieldSetter<float, &FVector4::X>, PyCStrCast("X"), nullptr },
		{ PyCStrCast("y"), (getter)&FPyWrapperVector4::IntrinsicFieldGetter<float, &FVector4::Y>, (setter)&FPyWrapperVector4::IntrinsicFieldSetter<float, &FVector4::Y>, PyCStrCast("Y"), nullptr },
		{ PyCStrCast("z"), (getter)&FPyWrapperVector4::IntrinsicFieldGetter<float, &FVector4::Z>, (setter)&FPyWrapperVector4::IntrinsicFieldSetter<float, &FVector4::Z>, PyCStrCast("Z"), nullptr },
		{ PyCStrCast("w"), (getter)&FPyWrapperVector4::IntrinsicFieldGetter<float, &FVector4::W>, (setter)&FPyWrapperVector4::IntrinsicFieldSetter<float, &FVector4::W>, PyCStrCast("W"), nullptr },
		{ nullptr, nullptr, nullptr, nullptr, nullptr }
	};

	static PyNumberMethods PyNumber;
	PyNumber.nb_add = (binaryfunc)&EvaulateOperatorStack_Struct_ReturnStruct<FPyWrapperVector4, FAddOperation>;
	PyNumber.nb_inplace_add = (binaryfunc)&EvaulateOperatorStack_Inline_Struct_ReturnStruct<FPyWrapperVector4, FAddOperation>;
	PyNumber.nb_subtract = (binaryfunc)&EvaulateOperatorStack_Struct_ReturnStruct<FPyWrapperVector4, FSubOperation>;
	PyNumber.nb_inplace_subtract = (binaryfunc)&EvaulateOperatorStack_Inline_Struct_ReturnStruct<FPyWrapperVector4, FSubOperation>;
	PyNumber.nb_multiply = (binaryfunc)&EvaulateOperatorStack_StructOrFloat_ReturnStruct<FPyWrapperVector4, FMulOperation>;
	PyNumber.nb_inplace_multiply = (binaryfunc)&EvaulateOperatorStack_Inline_StructOrFloat_ReturnStruct<FPyWrapperVector4, FMulOperation>;
#if PY_MAJOR_VERSION >= 3
	PyNumber.nb_true_divide = (binaryfunc)&EvaulateOperatorStack_StructOrFloat_ReturnStruct<FPyWrapperVector4, FDivOperation>;
	PyNumber.nb_inplace_true_divide = (binaryfunc)&EvaulateOperatorStack_Inline_Struct_ReturnStruct<FPyWrapperVector4, FDivOperation>;
#else	// PY_MAJOR_VERSION >= 3
	PyNumber.nb_divide = (binaryfunc)&EvaulateOperatorStack_StructOrFloat_ReturnStruct<FPyWrapperVector4, FDivOperation>;
	PyNumber.nb_inplace_divide = (binaryfunc)&EvaulateOperatorStack_Inline_Struct_ReturnStruct<FPyWrapperVector4, FDivOperation>;
#endif	// PY_MAJOR_VERSION >= 3
	//PyNumber.nb_or = (binaryfunc)&EvaulateOperatorStack_Struct_ReturnStruct<FPyWrapperVector2D, FBitwiseOrOperation>;
	PyNumber.nb_xor = (binaryfunc)&EvaulateOperatorStack_Struct_ReturnStruct<FPyWrapperVector4, FBitwiseXorOperation>;

	PyType.tp_methods = PyMethods;
	PyType.tp_getset = PyGetSets;
	PyType.tp_as_number = &PyNumber;

	return PyType;
}

PyTypeObject InitializePyWrapperMathType_Quat()
{
	PyTypeObject PyType = InitializePyWrapperMathType_Common<FPyWrapperQuat>("Quat", "Floating point quaternion that can represent a rotation about an axis in 3-D space (X, Y, Z, W)");

#if PY_MAJOR_VERSION < 3
	PyType.tp_flags |= Py_TPFLAGS_CHECKTYPES;
#endif	// PY_MAJOR_VERSION < 3

	using namespace PyWrapperMathTypeNumberFuncs;

	struct FMethods
	{
		static PyObject* Set(FPyWrapperQuat* InSelf, PyObject* InArgs)
		{
			PyObject* PyQuatObj = nullptr;

			if (!PyArg_ParseTuple(InArgs, "O:set", &PyQuatObj))
			{
				return nullptr;
			}

			FQuat Other;
			if (!PyConversion::NativizeStruct(PyQuatObj, Other))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'quat' (%s) to 'Quat'"), *PyUtil::GetFriendlyTypename(PyQuatObj)));
				return nullptr;
			}

			FPyWrapperQuat::GetTypedStruct(InSelf) = Other;
			Py_RETURN_NONE;
		}

		static PyObject* SetComponents(FPyWrapperQuat* InSelf, PyObject* InArgs)
		{
			PyObject* PyXObj = nullptr;
			PyObject* PyYObj = nullptr;
			PyObject* PyZObj = nullptr;
			PyObject* PyWObj = nullptr;

			if (!PyArg_ParseTuple(InArgs, "OOOO:set_components", &PyXObj, &PyYObj, &PyZObj, &PyWObj))
			{
				return nullptr;
			}

			float X = 0.0f;
			if (!PyConversion::Nativize(PyXObj, X))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'x' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyXObj)));
				return nullptr;
			}

			float Y = 0.0f;
			if (!PyConversion::Nativize(PyYObj, Y))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'y' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyYObj)));
				return nullptr;
			}

			float Z = 0.0f;
			if (!PyConversion::Nativize(PyZObj, Z))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'z' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyZObj)));
				return nullptr;
			}

			float W = 0.0f;
			if (!PyConversion::Nativize(PyWObj, W))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'w' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyWObj)));
				return nullptr;
			}

			FPyWrapperQuat::GetTypedStruct(InSelf) = FQuat(X, Y, Z, W);
			Py_RETURN_NONE;
		}

		static PyObject* SetFromEuler(FPyWrapperQuat* InSelf, PyObject* InArgs)
		{
			PyObject* PyVectorObj = nullptr;

			if (!PyArg_ParseTuple(InArgs, "O:set_from_euler", &PyVectorObj))
			{
				return nullptr;
			}

			FVector Euler;
			if (!PyConversion::NativizeStruct(PyVectorObj, Euler))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'euler' (%s) to 'Vector'"), *PyUtil::GetFriendlyTypename(PyVectorObj)));
				return nullptr;
			}

			FPyWrapperQuat::GetTypedStruct(InSelf) = FQuat::MakeFromEuler(Euler);
			Py_RETURN_NONE;
		}

		static PyObject* Equals(FPyWrapperQuat* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyQuatObj = nullptr;
			PyObject* PyToleranceObj = nullptr;

			static const char *ArgsKwdList[] = { "quat", "tolerance", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|O:equals", (char**)ArgsKwdList, &PyQuatObj, &PyToleranceObj))
			{
				return nullptr;
			}

			FQuat Other;
			if (!PyConversion::NativizeStruct(PyQuatObj, Other))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'quat' (%s) to 'Quat'"), *PyUtil::GetFriendlyTypename(PyQuatObj)));
				return nullptr;
			}

			float Tolerance = KINDA_SMALL_NUMBER;
			if (PyToleranceObj && !PyConversion::Nativize(PyToleranceObj, Tolerance))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'tolerance' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyToleranceObj)));
				return nullptr;
			}

			const bool bResult = FPyWrapperQuat::GetTypedStruct(InSelf).Equals(Other, Tolerance);
			return PyConversion::Pythonize(bResult);
		}

		static PyObject* Normalize(FPyWrapperQuat* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyToleranceObj = nullptr;

			static const char *ArgsKwdList[] = { "tolerance", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "|O:normalize", (char**)ArgsKwdList, &PyToleranceObj))
			{
				return nullptr;
			}

			float Tolerance = SMALL_NUMBER;
			if (PyToleranceObj && !PyConversion::Nativize(PyToleranceObj, Tolerance))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'tolerance' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyToleranceObj)));
				return nullptr;
			}

			FPyWrapperQuat::GetTypedStruct(InSelf).Normalize(Tolerance);
			Py_RETURN_NONE;
		}

		static PyObject* EnforceShortestArcWith(FPyWrapperQuat* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyQuatObj = nullptr;

			static const char *ArgsKwdList[] = { "quat", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O:enforce_shortest_arc_with", (char**)ArgsKwdList, &PyQuatObj))
			{
				return nullptr;
			}

			FQuat Quat;
			if (!PyConversion::NativizeStruct(PyQuatObj, Quat))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'quat' (%s) to 'Quat'"), *PyUtil::GetFriendlyTypename(PyQuatObj)));
				return nullptr;
			}

			FPyWrapperQuat::GetTypedStruct(InSelf).EnforceShortestArcWith(Quat);
			Py_RETURN_NONE;
		}

		PYMATH_METHOD_ONE_PARAM(FPyWrapperQuat, IsIdentity, "|O:is_identity", float, "float", "tolerance", SMALL_NUMBER, PyConversion::Nativize, PyConversion::Pythonize);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperQuat, GetNormalized, "|O:get_normalized", float, "float", "tolerance", SMALL_NUMBER, PyConversion::Nativize, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperQuat, RotateVector, "O:rotate_vector", FVector, "Vector", "vector", FVector(), PyConversion::NativizeStruct, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperQuat, UnrotateVector, "O:unrotate_vector", FVector, "Vector", "vector", FVector(), PyConversion::NativizeStruct, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperQuat, AngularDistance, "O:angular_distance", FQuat, "Quat", "quat", FQuat(), PyConversion::NativizeStruct, PyConversion::Pythonize);
	};

	static PyMethodDef PyMethods[] = {
		{ "set", PyCFunctionCast(&FMethods::Set), METH_VARARGS, "x.set(quat) -- set the value of this quaternion" },
		{ "set_components", PyCFunctionCast(&FMethods::SetComponents), METH_VARARGS, "x.set_components(x, y, z, w) -- set the component values of this quaternion" },
		{ "set_from_euler", PyCFunctionCast(&FMethods::SetFromEuler), METH_VARARGS, "x.set_from_euler(euler) -- set from a vector of floating-point Euler angles (in degrees)" },
		{ "equals", PyCFunctionCast(&FMethods::Equals), METH_VARARGS | METH_KEYWORDS, "x.equals(quat, tolerance=KINDA_SMALL_NUMBER) -> bool -- check against another quaternion for equality, within specified error limits" },
		{ "is_identity", PyCFunctionCast(&FMethods::IsIdentity), METH_VARARGS | METH_KEYWORDS, "x.is_identity(tolerance=SMALL_NUMBER) -> bool -- check whether this quaternion is an identity quaternion, within specified error limits" },
		{ "is_normalized", PyCFunctionCast((&FPyWrapperQuat::CallConstFunc_NoParams_IntrinsicReturn<bool, &FQuat::IsNormalized>)), METH_NOARGS, "x.is_normalized() -> bool -- check whether the quaternion is normalized" },
		{ "normalize", PyCFunctionCast(&FMethods::Normalize), METH_VARARGS | METH_KEYWORDS, "x.normalize(tolerance=SMALL_NUMBER) -> bool -- normalize this quaternion in-place if it is larger than a given tolerance, or an identity quaternion if not" },
		{ "get_normalized", PyCFunctionCast(&FMethods::GetNormalized), METH_VARARGS | METH_KEYWORDS, "x.get_normalized(tolerance=SMALL_NUMBER) -> Quat -- get a normalized copy of this quaternion if it is larger than a given tolerance, or an an identity quaternion if not" },
		{ "size", PyCFunctionCast((&FPyWrapperQuat::CallConstFunc_NoParams_IntrinsicReturn<float, &FQuat::Size>)), METH_NOARGS, "x.size() -> float -- get the length of this quaternion" },
		{ "size_squared", PyCFunctionCast((&FPyWrapperQuat::CallConstFunc_NoParams_IntrinsicReturn<float, &FQuat::SizeSquared>)), METH_NOARGS, "x.size_squared() -> float -- get the squared length of this quaternion" },
		{ "get_angle", PyCFunctionCast((&FPyWrapperQuat::CallConstFunc_NoParams_IntrinsicReturn<float, &FQuat::GetAngle>)), METH_NOARGS, "x.get_angle() -> float -- get the angle of this quaternion" },
		{ "log", PyCFunctionCast((&FPyWrapperQuat::CallConstFunc_NoParams_StructReturn<FQuat, &FQuat::Log>)), METH_NOARGS, "x.log() -> Quat -- get quaternion with W=0 and V=theta*v" },
		{ "exp", PyCFunctionCast((&FPyWrapperQuat::CallConstFunc_NoParams_StructReturn<FQuat, &FQuat::Exp>)), METH_NOARGS, "x.exp() -> Quat -- exp(q) = (sin(theta)*v, cos(theta)) (assumes a quaternion with W=0 and V=theta*v (where |v| = 1), and should really only be used after log)" },
		{ "inverse", PyCFunctionCast((&FPyWrapperQuat::CallConstFunc_NoParams_StructReturn<FQuat, &FQuat::Inverse>)), METH_NOARGS, "x.inverse() -> Quat -- get the inverse of this quaternion" },
		{ "get_axis_x", PyCFunctionCast((&FPyWrapperQuat::CallConstFunc_NoParams_StructReturn<FVector, &FQuat::GetAxisX>)), METH_NOARGS, "x.get_axis_x() -> Vector -- get the forward direction (X axis) after it has been rotated by this quaternion" },
		{ "get_axis_y", PyCFunctionCast((&FPyWrapperQuat::CallConstFunc_NoParams_StructReturn<FVector, &FQuat::GetAxisY>)), METH_NOARGS, "x.get_axis_y() -> Vector -- get the right direction (Y axis) after it has been rotated by this quaternion" },
		{ "get_axis_z", PyCFunctionCast((&FPyWrapperQuat::CallConstFunc_NoParams_StructReturn<FVector, &FQuat::GetAxisZ>)), METH_NOARGS, "x.get_axis_z() -> Vector -- get the up direction (Z axis) after it has been rotated by this quaternion" },
		{ "get_forward_vector", PyCFunctionCast((&FPyWrapperQuat::CallConstFunc_NoParams_StructReturn<FVector, &FQuat::GetForwardVector>)), METH_NOARGS, "x.get_forward_vector() -> Vector -- get the forward direction (X axis) after it has been rotated by this quaternion" },
		{ "get_right_vector", PyCFunctionCast((&FPyWrapperQuat::CallConstFunc_NoParams_StructReturn<FVector, &FQuat::GetRightVector>)), METH_NOARGS, "x.get_right_vector() -> Vector -- get the right direction (Y axis) after it has been rotated by this quaternion" },
		{ "get_up_vector", PyCFunctionCast((&FPyWrapperQuat::CallConstFunc_NoParams_StructReturn<FVector, &FQuat::GetUpVector>)), METH_NOARGS, "x.get_up_vector() -> Vector -- get the up direction (Z axis) after it has been rotated by this quaternion" },
		{ "euler", PyCFunctionCast((&FPyWrapperQuat::CallConstFunc_NoParams_StructReturn<FVector, &FQuat::Euler>)), METH_NOARGS, "x.euler() -> Vector -- get the quaternion as floating-point Euler angles (in degrees)" },
		{ "rotator", PyCFunctionCast((&FPyWrapperQuat::CallConstFunc_NoParams_StructReturn<FRotator, &FQuat::Rotator>)), METH_NOARGS, "x.rotator() -> Rotator -- get the rotator representation of this quaternion" },
		{ "get_rotation_axis", PyCFunctionCast((&FPyWrapperQuat::CallConstFunc_NoParams_StructReturn<FVector, &FQuat::GetRotationAxis>)), METH_NOARGS, "x.get_rotation_axis() -> Vector -- get the axis of rotation of the quaternion" },
		{ "rotate_vector", PyCFunctionCast(&FMethods::RotateVector), METH_VARARGS | METH_KEYWORDS, "x.rotate_vector(vector) -> Vector -- rotate a vector by this quaternion" },
		{ "unrotate_vector", PyCFunctionCast(&FMethods::UnrotateVector), METH_VARARGS | METH_KEYWORDS, "x.unrotate_vector(vector) -> Vector -- rotate a vector by the inverse of this quaternion" },
		{ "angular_distance", PyCFunctionCast(&FMethods::AngularDistance), METH_VARARGS | METH_KEYWORDS, "x.angular_distance(quat) -> float -- find the angular distance between two rotation quaternions (in radians)" },
		{ "enforce_shortest_arc_with", PyCFunctionCast(&FMethods::EnforceShortestArcWith), METH_VARARGS | METH_KEYWORDS, "x.enforce_shortest_arc_with(quat) -- enforce that the delta between this quaternion and another one represent the shortest possible rotation angle" },
		{ "contains_nan", PyCFunctionCast((&FPyWrapperQuat::CallConstFunc_NoParams_IntrinsicReturn<bool, &FQuat::ContainsNaN>)), METH_NOARGS, "x.contains_nan() -> bool -- check if there are any non-finite values (NaN or Inf) in this quaternion" },
		{ nullptr, nullptr, 0, nullptr }
	};

	static PyGetSetDef PyGetSets[] = {
		{ PyCStrCast("x"), (getter)&FPyWrapperQuat::IntrinsicFieldGetter<float, &FQuat::X>, (setter)&FPyWrapperQuat::IntrinsicFieldSetter<float, &FQuat::X>, PyCStrCast("X"), nullptr },
		{ PyCStrCast("y"), (getter)&FPyWrapperQuat::IntrinsicFieldGetter<float, &FQuat::Y>, (setter)&FPyWrapperQuat::IntrinsicFieldSetter<float, &FQuat::Y>, PyCStrCast("Y"), nullptr },
		{ PyCStrCast("z"), (getter)&FPyWrapperQuat::IntrinsicFieldGetter<float, &FQuat::Z>, (setter)&FPyWrapperQuat::IntrinsicFieldSetter<float, &FQuat::Z>, PyCStrCast("Z"), nullptr },
		{ PyCStrCast("w"), (getter)&FPyWrapperQuat::IntrinsicFieldGetter<float, &FQuat::W>, (setter)&FPyWrapperQuat::IntrinsicFieldSetter<float, &FQuat::W>, PyCStrCast("W"), nullptr },
		{ nullptr, nullptr, nullptr, nullptr, nullptr }
	};

	static PyNumberMethods PyNumber;
	PyNumber.nb_add = (binaryfunc)&EvaulateOperatorStack_Struct_ReturnStruct<FPyWrapperQuat, FAddOperation>;
	PyNumber.nb_inplace_add = (binaryfunc)&EvaulateOperatorStack_Inline_Struct_ReturnStruct<FPyWrapperQuat, FAddOperation>;
	PyNumber.nb_subtract = (binaryfunc)&EvaulateOperatorStack_Struct_ReturnStruct<FPyWrapperQuat, FSubOperation>;
	PyNumber.nb_inplace_subtract = (binaryfunc)&EvaulateOperatorStack_Inline_Struct_ReturnStruct<FPyWrapperQuat, FSubOperation>;
	PyNumber.nb_multiply = (binaryfunc)&EvaulateOperatorStack_StructOrFloat_ReturnStruct<FPyWrapperQuat, FMulOperation>;
	PyNumber.nb_inplace_multiply = (binaryfunc)&EvaulateOperatorStack_Inline_Struct_ReturnStruct<FPyWrapperQuat, FMulOperation>;
#if PY_MAJOR_VERSION >= 3
	PyNumber.nb_true_divide = (binaryfunc)&EvaulateOperatorStack_Float_ReturnStruct<FPyWrapperQuat, FDivOperation>;
	PyNumber.nb_inplace_true_divide = (binaryfunc)&EvaulateOperatorStack_Inline_Float_ReturnStruct<FPyWrapperQuat, FDivOperation>;
#else	// PY_MAJOR_VERSION >= 3
	PyNumber.nb_divide = (binaryfunc)&EvaulateOperatorStack_Float_ReturnStruct<FPyWrapperQuat, FDivOperation>;
	PyNumber.nb_inplace_divide = (binaryfunc)&EvaulateOperatorStack_Inline_Float_ReturnStruct<FPyWrapperQuat, FDivOperation>;
#endif	// PY_MAJOR_VERSION >= 3

	PyType.tp_methods = PyMethods;
	PyType.tp_getset = PyGetSets;
	PyType.tp_as_number = &PyNumber;

	return PyType;
}

PyTypeObject InitializePyWrapperMathType_LinearColor()
{
	PyTypeObject PyType = InitializePyWrapperMathType_Common<FPyWrapperLinearColor>("LinearColor", " Linear color (R, G, B, A)");

#if PY_MAJOR_VERSION < 3
	PyType.tp_flags |= Py_TPFLAGS_CHECKTYPES;
#endif	// PY_MAJOR_VERSION < 3

	using namespace PyWrapperMathTypeNumberFuncs;

	struct FMethods
	{
		static PyObject* Set(FPyWrapperLinearColor* InSelf, PyObject* InArgs)
		{
			PyObject* PyLinearColorObj = nullptr;

			if (!PyArg_ParseTuple(InArgs, "O:set", &PyLinearColorObj))
			{
				return nullptr;
			}

			FLinearColor Other;
			if (!PyConversion::NativizeStruct(PyLinearColorObj, Other))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'linear_color' (%s) to 'LinearColor'"), *PyUtil::GetFriendlyTypename(PyLinearColorObj)));
				return nullptr;
			}

			FPyWrapperLinearColor::GetTypedStruct(InSelf) = Other;
			Py_RETURN_NONE;
		}

		static PyObject* SetComponents(FPyWrapperLinearColor* InSelf, PyObject* InArgs)
		{
			PyObject* PyRObj = nullptr;
			PyObject* PyGObj = nullptr;
			PyObject* PyBObj = nullptr;
			PyObject* PyAObj = nullptr;

			if (!PyArg_ParseTuple(InArgs, "OOO|O:set_components", &PyRObj, &PyGObj, &PyBObj, &PyAObj))
			{
				return nullptr;
			}

			float R = 0.0f;
			if (!PyConversion::Nativize(PyRObj, R))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'r' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyRObj)));
				return nullptr;
			}

			float G = 0.0f;
			if (!PyConversion::Nativize(PyGObj, G))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'g' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyGObj)));
				return nullptr;
			}

			float B = 0.0f;
			if (!PyConversion::Nativize(PyBObj, B))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'b' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyBObj)));
				return nullptr;
			}

			float A = 1.0f;
			if (PyAObj && !PyConversion::Nativize(PyAObj, A))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'a' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyAObj)));
				return nullptr;
			}

			FPyWrapperLinearColor::GetTypedStruct(InSelf) = FLinearColor(R, G, B, A);
			Py_RETURN_NONE;
		}

		static PyObject* SetFromHSV(FPyWrapperLinearColor* InSelf, PyObject* InArgs)
		{
			PyObject* PyHObj = nullptr;
			PyObject* PySObj = nullptr;
			PyObject* PyVObj = nullptr;

			if (!PyArg_ParseTuple(InArgs, "OOO:set_from_hsv", &PyHObj, &PySObj, &PyVObj))
			{
				return nullptr;
			}

			uint8 H = 0;
			if (!PyConversion::Nativize(PyHObj, H))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'h' (%s) to 'uint8'"), *PyUtil::GetFriendlyTypename(PyHObj)));
				return nullptr;
			}

			uint8 S = 0;
			if (!PyConversion::Nativize(PySObj, S))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 's' (%s) to 'uint8'"), *PyUtil::GetFriendlyTypename(PySObj)));
				return nullptr;
			}

			uint8 V = 0;
			if (!PyConversion::Nativize(PyVObj, V))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'v' (%s) to 'uint8'"), *PyUtil::GetFriendlyTypename(PyVObj)));
				return nullptr;
			}

			FPyWrapperLinearColor::GetTypedStruct(InSelf) = FLinearColor::FGetHSV(H, S, V);
			Py_RETURN_NONE;
		}

		static PyObject* SetFromSRGB(FPyWrapperLinearColor* InSelf, PyObject* InArgs)
		{
			PyObject* PyColorObj = nullptr;

			if (!PyArg_ParseTuple(InArgs, "O:set_from_srgb", &PyColorObj))
			{
				return nullptr;
			}

			FColor Color;
			if (!PyConversion::NativizeStruct(PyColorObj, Color))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'color' (%s) to 'Color'"), *PyUtil::GetFriendlyTypename(PyColorObj)));
				return nullptr;
			}

			FPyWrapperLinearColor::GetTypedStruct(InSelf) = FLinearColor::FromSRGBColor(Color);
			Py_RETURN_NONE;
		}

		static PyObject* SetFromPow22(FPyWrapperLinearColor* InSelf, PyObject* InArgs)
		{
			PyObject* PyColorObj = nullptr;

			if (!PyArg_ParseTuple(InArgs, "O:set_from_pow22", &PyColorObj))
			{
				return nullptr;
			}

			FColor Color;
			if (!PyConversion::NativizeStruct(PyColorObj, Color))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'color' (%s) to 'Color'"), *PyUtil::GetFriendlyTypename(PyColorObj)));
				return nullptr;
			}

			FPyWrapperLinearColor::GetTypedStruct(InSelf) = FLinearColor::FromPow22Color(Color);
			Py_RETURN_NONE;
		}

		static PyObject* SetFromTemperature(FPyWrapperLinearColor* InSelf, PyObject* InArgs)
		{
			PyObject* PyTempObj = nullptr;

			if (!PyArg_ParseTuple(InArgs, "O:set_from_temperature", &PyTempObj))
			{
				return nullptr;
			}

			float Temp = 0.0f;
			if (!PyConversion::Nativize(PyTempObj, Temp))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'temp' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyTempObj)));
				return nullptr;
			}

			FPyWrapperLinearColor::GetTypedStruct(InSelf) = FLinearColor::MakeFromColorTemperature(Temp);
			Py_RETURN_NONE;
		}

		static PyObject* Equals(FPyWrapperLinearColor* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyLinearColorObj = nullptr;
			PyObject* PyToleranceObj = nullptr;

			static const char *ArgsKwdList[] = { "linear_color", "tolerance", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|O:equals", (char**)ArgsKwdList, &PyLinearColorObj, &PyToleranceObj))
			{
				return nullptr;
			}

			FLinearColor Other;
			if (!PyConversion::NativizeStruct(PyLinearColorObj, Other))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'linear_color' (%s) to 'LinearColor'"), *PyUtil::GetFriendlyTypename(PyLinearColorObj)));
				return nullptr;
			}

			float Tolerance = KINDA_SMALL_NUMBER;
			if (PyToleranceObj && !PyConversion::Nativize(PyToleranceObj, Tolerance))
			{
				PyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'tolerance' (%s) to 'float'"), *PyUtil::GetFriendlyTypename(PyToleranceObj)));
				return nullptr;
			}

			const bool bResult = FPyWrapperLinearColor::GetTypedStruct(InSelf).Equals(Other, Tolerance);
			return PyConversion::Pythonize(bResult);
		}

		PYMATH_METHOD_ONE_PARAM(FPyWrapperLinearColor, ToFColor, "O:to_color", bool, "bool", "is_srgb", false, PyConversion::Nativize, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperLinearColor, Desaturate, "O:desaturate", float, "float", "desaturation", 0.0f, PyConversion::Nativize, PyConversion::PythonizeStruct);
		PYMATH_METHOD_ONE_PARAM(FPyWrapperLinearColor, CopyWithNewOpacity, "O:copy_with_new_opacity", float, "float", "opacicty", 0.0f, PyConversion::Nativize, PyConversion::PythonizeStruct);

		PYMATH_SFUNC_ONE_PARAM(FPyWrapperLinearColor, Dist, "O:dist", FLinearColor, "LinearColor", "other", FLinearColor(), PyConversion::NativizeStruct, PyConversion::Pythonize);
	};

	static PyMethodDef PyMethods[] = {
		{ "set", PyCFunctionCast(&FMethods::Set), METH_VARARGS, "x.set(linear_color) -- set the value of this linear color" },
		{ "set_components", PyCFunctionCast(&FMethods::SetComponents), METH_VARARGS, "x.set_components(r, g, b, a=1.0) -- set the component values of this linear color" },
		{ "set_from_hsv", PyCFunctionCast(&FMethods::SetFromHSV), METH_VARARGS, "x.set_from_hsv(h, s, v) -- set from byte hue-saturation-brightness" },
		{ "set_from_srgb", PyCFunctionCast(&FMethods::SetFromSRGB), METH_VARARGS, "x.set_from_srgb(color) -- set from a color coming from an observed sRGB output" },
		{ "set_from_pow22", PyCFunctionCast(&FMethods::SetFromPow22), METH_VARARGS, "x.set_from_pow22(color) -- set from a color coming from an observed Pow(1/2.2) output" },
		{ "set_from_temperature", PyCFunctionCast(&FMethods::SetFromTemperature), METH_VARARGS, "x.set_from_temperature(temp) -- set from a temperature in Kelvins of a black body radiator to RGB chromaticity" },
		{ "linear_rgb_to_hsv", PyCFunctionCast((&FPyWrapperLinearColor::CallConstFunc_NoParams_StructReturn<FLinearColor, &FLinearColor::LinearRGBToHSV>)), METH_NOARGS, "x.linear_rgb_to_hsv() -> LinearColor -- get a linear space RGB color as a HSV color" },
		{ "hsv_to_linear_rgb", PyCFunctionCast((&FPyWrapperLinearColor::CallConstFunc_NoParams_StructReturn<FLinearColor, &FLinearColor::HSVToLinearRGB>)), METH_NOARGS, "x.hsv_to_linear_rgb() -> LinearColor -- get a HSV color as a linear space RGB color" },
		{ "to_rgbe", PyCFunctionCast((&FPyWrapperLinearColor::CallConstFunc_NoParams_StructReturn<FColor, &FLinearColor::ToRGBE>)), METH_NOARGS, "x.to_rgbe() -> Color -- get a RGBE color from a floating point color" },
		{ "to_color", PyCFunctionCast(&FMethods::ToFColor), METH_VARARGS | METH_KEYWORDS, "x.to_color(is_srgb) -> Color -- quantize the linear color with optional sRGB conversion and quality as goal" },
		{ "quantize", PyCFunctionCast((&FPyWrapperLinearColor::CallConstFunc_NoParams_StructReturn<FColor, &FLinearColor::Quantize>)), METH_NOARGS, "x.quantize() -> Color -- quantize the linear color bypassing the sRGB conversion" },
		{ "quantize_round", PyCFunctionCast((&FPyWrapperLinearColor::CallConstFunc_NoParams_StructReturn<FColor, &FLinearColor::QuantizeRound>)), METH_NOARGS, "x.quantize_round() -> Color -- quantize the linear color with rounding and bypassing the sRGB conversion" },
		{ "desaturate", PyCFunctionCast(&FMethods::Desaturate), METH_VARARGS | METH_KEYWORDS, "x.desaturate(desaturation) -> LinearColor -- get a desaturated color, with 0 meaning no desaturation and 1 == full desaturation" },
		{ "copy_with_new_opacity", PyCFunctionCast(&FMethods::CopyWithNewOpacity), METH_VARARGS | METH_KEYWORDS, "x.copy_with_new_opacity(opacicty) -> LinearColor -- get a copy of this RGB components from this linear color and the A of the given opacity" },
		{ "equals", PyCFunctionCast(&FMethods::Equals), METH_VARARGS | METH_KEYWORDS, "x.equals(linear_color, tolerance=KINDA_SMALL_NUMBER) -> bool -- check against another linear color for equality, within specified error limits" },
		{ "get_max", PyCFunctionCast((&FPyWrapperLinearColor::CallConstFunc_NoParams_IntrinsicReturn<float, &FLinearColor::GetMax>)), METH_NOARGS, "x.get_max() -> float -- get the maximum value in this color" },
		{ "get_min", PyCFunctionCast((&FPyWrapperLinearColor::CallConstFunc_NoParams_IntrinsicReturn<float, &FLinearColor::GetMin>)), METH_NOARGS, "x.get_min() -> float -- get the minimum value in this color" },
		{ "get_luminance", PyCFunctionCast((&FPyWrapperLinearColor::CallConstFunc_NoParams_IntrinsicReturn<float, &FLinearColor::GetLuminance>)), METH_NOARGS, "x.get_luminance() -> float -- get the luminance of this color" },
		{ "dist", PyCFunctionCast(&FMethods::Dist), METH_VARARGS | METH_KEYWORDS, "x.dist(other) -> float -- get the Euclidean distance between this color and another" },
		{ nullptr, nullptr, 0, nullptr }
	};

	static PyGetSetDef PyGetSets[] = {
		{ PyCStrCast("r"), (getter)&FPyWrapperLinearColor::IntrinsicFieldGetter<float, &FLinearColor::R>, (setter)&FPyWrapperLinearColor::IntrinsicFieldSetter<float, &FLinearColor::R>, PyCStrCast("R"), nullptr },
		{ PyCStrCast("g"), (getter)&FPyWrapperLinearColor::IntrinsicFieldGetter<float, &FLinearColor::G>, (setter)&FPyWrapperLinearColor::IntrinsicFieldSetter<float, &FLinearColor::G>, PyCStrCast("G"), nullptr },
		{ PyCStrCast("b"), (getter)&FPyWrapperLinearColor::IntrinsicFieldGetter<float, &FLinearColor::B>, (setter)&FPyWrapperLinearColor::IntrinsicFieldSetter<float, &FLinearColor::B>, PyCStrCast("B"), nullptr },
		{ PyCStrCast("a"), (getter)&FPyWrapperLinearColor::IntrinsicFieldGetter<float, &FLinearColor::A>, (setter)&FPyWrapperLinearColor::IntrinsicFieldSetter<float, &FLinearColor::A>, PyCStrCast("A"), nullptr },
		{ nullptr, nullptr, nullptr, nullptr, nullptr }
	};

	static PyNumberMethods PyNumber;
	PyNumber.nb_add = (binaryfunc)&EvaulateOperatorStack_Struct_ReturnStruct<FPyWrapperLinearColor, FAddOperation>;
	PyNumber.nb_inplace_add = (binaryfunc)&EvaulateOperatorStack_Inline_Struct_ReturnStruct<FPyWrapperLinearColor, FAddOperation>;
	PyNumber.nb_subtract = (binaryfunc)&EvaulateOperatorStack_Struct_ReturnStruct<FPyWrapperLinearColor, FSubOperation>;
	PyNumber.nb_inplace_subtract = (binaryfunc)&EvaulateOperatorStack_Inline_Struct_ReturnStruct<FPyWrapperLinearColor, FSubOperation>;
	PyNumber.nb_multiply = (binaryfunc)&EvaulateOperatorStack_StructOrFloat_ReturnStruct<FPyWrapperLinearColor, FMulOperation>;
	PyNumber.nb_inplace_multiply = (binaryfunc)&EvaulateOperatorStack_Inline_Struct_ReturnStruct<FPyWrapperLinearColor, FMulOperation>;
#if PY_MAJOR_VERSION >= 3
	PyNumber.nb_true_divide = (binaryfunc)&EvaulateOperatorStack_StructOrFloat_ReturnStruct<FPyWrapperLinearColor, FDivOperation>;
	PyNumber.nb_inplace_true_divide = (binaryfunc)&EvaulateOperatorStack_Inline_StructOrFloat_ReturnStruct<FPyWrapperLinearColor, FDivOperation>;
#else	// PY_MAJOR_VERSION >= 3
	PyNumber.nb_divide = (binaryfunc)&EvaulateOperatorStack_StructOrFloat_ReturnStruct<FPyWrapperLinearColor, FDivOperation>;
	PyNumber.nb_inplace_divide = (binaryfunc)&EvaulateOperatorStack_Inline_StructOrFloat_ReturnStruct<FPyWrapperLinearColor, FDivOperation>;
#endif	// PY_MAJOR_VERSION >= 3

	PyType.tp_methods = PyMethods;
	PyType.tp_getset = PyGetSets;
	PyType.tp_as_number = &PyNumber;

	return PyType;
}

#undef PYMATH_METHOD_ONE_PARAM
#undef PYMATH_SFUNC_ONE_PARAM

PyTypeObject PyWrapperVectorType = InitializePyWrapperMathType_Vector();
PyTypeObject PyWrapperVector2DType = InitializePyWrapperMathType_Vector2D();
PyTypeObject PyWrapperVector4Type = InitializePyWrapperMathType_Vector4();
PyTypeObject PyWrapperQuatType = InitializePyWrapperMathType_Quat();
PyTypeObject PyWrapperLinearColorType = InitializePyWrapperMathType_LinearColor();

#endif	// WITH_PYTHON
