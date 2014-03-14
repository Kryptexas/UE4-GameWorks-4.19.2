// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UniformBuffer.h: Uniform buffer declarations.
=============================================================================*/

#pragma once

#include "RenderCore.h"
#include "RenderResource.h"
#include "RHI.h"

/** Uniform buffer structs must be aligned to 16-byte boundaries. */
#define UNIFORM_BUFFER_STRUCT_ALIGNMENT 16

namespace EShaderPrecisionModifier
{
	enum Type
	{
		Float,
		Half,
		Fixed
	};
};

/** A uniform buffer resource. */
template<typename TBufferStruct>
class TUniformBuffer : public FRenderResource
{
public:

	TUniformBuffer()
	{
		Contents = (uint8*)FMemory::Malloc(sizeof(TBufferStruct),UNIFORM_BUFFER_STRUCT_ALIGNMENT);
	}

	~TUniformBuffer()
	{
		FMemory::Free(Contents);
	}

	/** Sets the contents of the uniform buffer. */
	void SetContents(const TBufferStruct& NewContents)
	{
		check(IsInRenderingThread());
		FMemory::Memcpy(Contents,&NewContents,sizeof(TBufferStruct));
		UpdateRHI();
	}
	
	// FRenderResource interface.
	virtual void InitDynamicRHI()
	{
		UniformBufferRHI = RHICreateUniformBuffer(Contents,sizeof(TBufferStruct),UniformBuffer_MultiUse);
	}
	virtual void ReleaseDynamicRHI()
	{
		UniformBufferRHI.SafeRelease();
	}

	// Accessors.
	FUniformBufferRHIParamRef GetUniformBufferRHI() const { return UniformBufferRHI; }

private:

	FUniformBufferRHIRef UniformBufferRHI;
	uint8* Contents;
};

/** A reference to a uniform buffer RHI resource with a specific structure. */
template<typename TBufferStruct>
class TUniformBufferRef : public FUniformBufferRHIRef
{
public:

	/** Initializes the reference to null. */
	TUniformBufferRef()
	{}

	/** Initializes the reference to point to a buffer. */
	TUniformBufferRef(const TUniformBuffer<TBufferStruct>& InBuffer)
	: FUniformBufferRHIRef(InBuffer.GetUniformBufferRHI())
	{}

	/** Creates a uniform buffer with the given value, and returns a structured reference to it. */
	static TUniformBufferRef<TBufferStruct> CreateUniformBufferImmediate(const TBufferStruct& Value, EUniformBufferUsage Usage)
	{
		check(IsInRenderingThread());
		return TUniformBufferRef<TBufferStruct>(RHICreateUniformBuffer(&Value,sizeof(Value),Usage));
	}

private:

	/** A private constructor used to coerce an arbitrary RHI uniform buffer reference to a structured reference. */
	TUniformBufferRef(FUniformBufferRHIParamRef InRHIRef)
	: FUniformBufferRHIRef(InRHIRef)
	{}
};

ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER_DECLARE_TEMPLATE(
	SetUniformBufferContents,TBufferStruct,
	TUniformBuffer<TBufferStruct>*,UniformBuffer,&UniformBuffer,
	TBufferStruct,Struct,Struct,
	{
		UniformBuffer->SetContents(Struct);
	});

/** Sends a message to the rendering thread to set the contents of a uniform buffer.  Called by the game thread. */
template<typename TBufferStruct>
void BeginSetUniformBufferContents(
	TUniformBuffer<TBufferStruct>& UniformBuffer,
	const TBufferStruct& Struct
	)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER_CREATE_TEMPLATE(
		SetUniformBufferContents,TBufferStruct,
		TUniformBuffer<TBufferStruct>*,&UniformBuffer,
		TBufferStruct,Struct);
}

/** The base type of a value in a uniform buffer. */
enum EUniformBufferBaseType
{
	UBMT_INVALID,
	UBMT_BOOL,
	UBMT_INT32,
	UBMT_UINT32,
	UBMT_FLOAT32,
	UBMT_STRUCT
};

/** A uniform buffer struct. */
class RENDERCORE_API FUniformBufferStruct
{
public:
	
	/** A member of a uniform buffer type. */
	class FMember
	{
	public:

		/** Initialization constructor. */
		FMember(
			const TCHAR* InName,
			uint32 InOffset,
			EUniformBufferBaseType InBaseType,
			EShaderPrecisionModifier::Type InPrecision,
			uint32 InNumRows,
			uint32 InNumColumns,
			uint32 InNumElements,
			const FUniformBufferStruct* InStruct
			)
		:	Name(InName)
		,	Offset(InOffset)
		,	BaseType(InBaseType)
		,	Precision(InPrecision)
		,	NumRows(InNumRows)
		,	NumColumns(InNumColumns)
		,	NumElements(InNumElements)
		,	Struct(InStruct)
		{}
	
		const TCHAR* GetName() const { return Name; }
		uint32 GetOffset() const { return Offset; }
		EUniformBufferBaseType GetBaseType() const { return BaseType; }
		EShaderPrecisionModifier::Type GetPrecision() const { return Precision; }
		uint32 GetNumRows() const { return NumRows; }
		uint32 GetNumColumns() const { return NumColumns; }
		uint32 GetNumElements() const { return NumElements; }
		const FUniformBufferStruct* GetStruct() const { return Struct; }

	private:

		const TCHAR* Name;
		uint32 Offset;
		EUniformBufferBaseType BaseType;
		EShaderPrecisionModifier::Type Precision;
		uint32 NumRows;
		uint32 NumColumns;
		uint32 NumElements;
		const FUniformBufferStruct* Struct;
	};

	typedef class FShaderUniformBufferParameter* (*ConstructUniformBufferParameterType)();

	/** Initialization constructor. */
	FUniformBufferStruct(const TCHAR* InStructTypeName, const TCHAR* InShaderVariableName, ConstructUniformBufferParameterType InConstructRef, uint32 InSize, const TArray<FMember>& InMembers, bool bRegisterForAutoBinding)
	:	StructTypeName(InStructTypeName)
	,	ShaderVariableName(InShaderVariableName)
	,	ConstructUniformBufferParameterRef(InConstructRef)
	,	Size(InSize)
	,	Members(InMembers)
	,	GlobalListLink(this)
	{
		if (bRegisterForAutoBinding)
		{
			GlobalListLink.Link(GetStructList());
		}
	}

	virtual ~FUniformBufferStruct()
	{
		GlobalListLink.Unlink();
	}

	const TCHAR* GetStructTypeName() const { return StructTypeName; }
	const TCHAR* GetShaderVariableName() const { return ShaderVariableName; }
	const uint32 GetSize() const { return Size; }
	const TArray<FMember>& GetMembers() const { return Members; }
	FShaderUniformBufferParameter* ConstructTypedParameter() const { return (*ConstructUniformBufferParameterRef)(); }

	static TLinkedList<FUniformBufferStruct*>*& GetStructList();

private:
	const TCHAR* StructTypeName;
	const TCHAR* ShaderVariableName;
	ConstructUniformBufferParameterType ConstructUniformBufferParameterRef;
	uint32 Size;
	TArray<FMember> Members;
	TLinkedList<FUniformBufferStruct*> GlobalListLink;
};

//
// Uniform buffer alignment tools (should only be used by the uniform buffer type infos below).
//

	template<typename T,uint32 Alignment>
	class TAlignedTypedef;

	#define IMPLEMENT_ALIGNED_TYPE(Alignment) \
	template<typename T> \
	class TAlignedTypedef<T,Alignment> \
	{ \
	public: \
		typedef MS_ALIGN(Alignment) T TAlignedType GCC_ALIGN(Alignment); \
	};

	IMPLEMENT_ALIGNED_TYPE(1);
	IMPLEMENT_ALIGNED_TYPE(2);
	IMPLEMENT_ALIGNED_TYPE(4);
	IMPLEMENT_ALIGNED_TYPE(8);
	IMPLEMENT_ALIGNED_TYPE(16);

//
// Template specializations used to map C++ types to uniform buffer member types.
//

	template<typename>
	class TUniformBufferTypeInfo
	{
	public:
		enum { BaseType = UBMT_INVALID };
		enum { NumRows = 0 };
		enum { NumColumns = 0 };
		enum { NumElements = 0 };
		enum { Alignment = 1 };
		static const FUniformBufferStruct* GetStruct() { return NULL; }
	};

	template<>
	class TUniformBufferTypeInfo<bool>
	{
	public:
		enum { BaseType = UBMT_BOOL };
		enum { NumRows = 1 };
		enum { NumColumns = 1 };
		enum { NumElements = 0 };
		enum { Alignment = 4 };
		typedef TAlignedTypedef<bool,Alignment>::TAlignedType TAlignedType;
		static const FUniformBufferStruct* GetStruct() { return NULL; }
	};

	template<>
	class TUniformBufferTypeInfo<uint32>
	{
	public:
		enum { BaseType = UBMT_UINT32 };
		enum { NumRows = 1 };
		enum { NumColumns = 1 };
		enum { NumElements = 0 };
		enum { Alignment = 4 };
		typedef uint32 TAlignedType;
		static const FUniformBufferStruct* GetStruct() { return NULL; }
	};
	

	template<>
	class TUniformBufferTypeInfo<int32>
	{
	public:
		enum { BaseType = UBMT_INT32 };
		enum { NumRows = 1 };
		enum { NumColumns = 1 };
		enum { NumElements = 0 };
		enum { Alignment = 4 };
		typedef int32 TAlignedType;
		static const FUniformBufferStruct* GetStruct() { return NULL; }
	};

	template<>
	class TUniformBufferTypeInfo<float>
	{
	public:
		enum { BaseType = UBMT_FLOAT32 };
		enum { NumRows = 1 };
		enum { NumColumns = 1 };
		enum { NumElements = 0 };
		enum { Alignment = 4 };
		typedef float TAlignedType;
		static const FUniformBufferStruct* GetStruct() { return NULL; }
	};

	template<>
	class TUniformBufferTypeInfo<FVector2D>
	{
	public:
		enum { BaseType = UBMT_FLOAT32 };
		enum { NumRows = 1 };
		enum { NumColumns = 2 };
		enum { NumElements = 0 };
		enum { Alignment = 8 };
		typedef TAlignedTypedef<FVector2D,Alignment>::TAlignedType TAlignedType;
		static const FUniformBufferStruct* GetStruct() { return NULL; }
	};

	template<>
	class TUniformBufferTypeInfo<FVector>
	{
	public:
		enum { BaseType = UBMT_FLOAT32 };
		enum { NumRows = 1 };
		enum { NumColumns = 3 };
		enum { NumElements = 0 };
		enum { Alignment = 16 };
		typedef TAlignedTypedef<FVector,Alignment>::TAlignedType TAlignedType;
		static const FUniformBufferStruct* GetStruct() { return NULL; }
	};

	template<>
	class TUniformBufferTypeInfo<FVector4>
	{
	public:
		enum { BaseType = UBMT_FLOAT32 };
		enum { NumRows = 1 };
		enum { NumColumns = 4 };
		enum { NumElements = 0 };
		enum { Alignment = 16 };
		typedef TAlignedTypedef<FVector4,Alignment>::TAlignedType TAlignedType;
		static const FUniformBufferStruct* GetStruct() { return NULL; }
	};
	
	template<>
	class TUniformBufferTypeInfo<FLinearColor>
	{
	public:
		enum { BaseType = UBMT_FLOAT32 };
		enum { NumRows = 1 };
		enum { NumColumns = 4 };
		enum { NumElements = 0 };
		enum { Alignment = 16 };
		typedef TAlignedTypedef<FLinearColor,Alignment>::TAlignedType TAlignedType;
		static const FUniformBufferStruct* GetStruct() { return NULL; }
	};
	
	template<>
	class TUniformBufferTypeInfo<FIntPoint>
	{
	public:
		enum { BaseType = UBMT_INT32 };
		enum { NumRows = 1 };
		enum { NumColumns = 2 };
		enum { NumElements = 0 };
		enum { Alignment = 8 };
		typedef TAlignedTypedef<FIntPoint,Alignment>::TAlignedType TAlignedType;
		static const FUniformBufferStruct* GetStruct() { return NULL; }
	};

	template<>
	class TUniformBufferTypeInfo<FIntVector>
	{
	public:
		enum { BaseType = UBMT_INT32 };
		enum { NumRows = 1 };
		enum { NumColumns = 3 };
		enum { NumElements = 0 };
		enum { Alignment = 16 };
		typedef TAlignedTypedef<FIntVector,Alignment>::TAlignedType TAlignedType;
		static const FUniformBufferStruct* GetStruct() { return NULL; }
	};

	template<>
	class TUniformBufferTypeInfo<FIntRect>
	{
	public:
		enum { BaseType = UBMT_INT32 };
		enum { NumRows = 1 };
		enum { NumColumns = 4 };
		enum { NumElements = 0 };
		enum { Alignment = 16 };
		typedef TAlignedTypedef<FIntRect,Alignment>::TAlignedType TAlignedType;
		static const FUniformBufferStruct* GetStruct() { return NULL; }
	};

	template<>
	class TUniformBufferTypeInfo<FMatrix>
	{
	public:
		enum { BaseType = UBMT_FLOAT32 };
		enum { NumRows = 4 };
		enum { NumColumns = 4 };
		enum { NumElements = 0 };
		enum { Alignment = 16 };
		typedef TAlignedTypedef<FMatrix,Alignment>::TAlignedType TAlignedType;
		static const FUniformBufferStruct* GetStruct() { return NULL; }
	};
	
	template<typename T,size_t InNumElements>
	class TUniformBufferTypeInfo<T[InNumElements]>
	{
	public:
		enum { BaseType = TUniformBufferTypeInfo<T>::BaseType };
		enum { NumRows = TUniformBufferTypeInfo<T>::NumRows };
		enum { NumColumns = TUniformBufferTypeInfo<T>::NumColumns };
		enum { NumElements = InNumElements };
		enum { Alignment = TUniformBufferTypeInfo<T>::Alignment };
		typedef TStaticArray<T,InNumElements,16> TAlignedType;
		static const FUniformBufferStruct* GetStruct() { return TUniformBufferTypeInfo<T>::GetStruct(); }
	};
	
	template<typename T,size_t InNumElements,uint32 IgnoredAlignment>
	class TUniformBufferTypeInfo<TStaticArray<T,InNumElements,IgnoredAlignment> >
	{
	public:
		enum { BaseType = TUniformBufferTypeInfo<T>::BaseType };
		enum { NumRows = TUniformBufferTypeInfo<T>::NumRows };
		enum { NumColumns = TUniformBufferTypeInfo<T>::NumColumns };
		enum { NumElements = InNumElements };
		enum { Alignment = TUniformBufferTypeInfo<T>::Alignment };
		typedef TStaticArray<T,InNumElements,16> TAlignedType;
		static const FUniformBufferStruct* GetStruct() { return TUniformBufferTypeInfo<T>::GetStruct(); }
	};

//
// Macros for declaring uniform buffer structures.
//

#define IMPLEMENT_UNIFORM_BUFFER_STRUCT(StructTypeName,ShaderVariableName) \
	FUniformBufferStruct StructTypeName::StaticStruct( \
	TEXT(#StructTypeName), \
	ShaderVariableName, \
	StructTypeName::ConstructUniformBufferParameter, \
	sizeof(StructTypeName), \
	StructTypeName::zzGetMembers(), \
	true);
	
/** Begins a uniform buffer struct declaration. */
#define BEGIN_UNIFORM_BUFFER_STRUCT(StructTypeName,PrefixKeywords) \
	MS_ALIGN(UNIFORM_BUFFER_STRUCT_ALIGNMENT) class PrefixKeywords StructTypeName \
	{ \
	public: \
		static FUniformBufferStruct StaticStruct; \
		static FShaderUniformBufferParameter* ConstructUniformBufferParameter() { return new TShaderUniformBufferParameter<StructTypeName>(); } \
	private: \
		typedef StructTypeName zzTThisStruct; \
		struct zzFirstMemberId {}; \
		static TArray<FUniformBufferStruct::FMember> zzGetMembersBefore(zzFirstMemberId) \
		{ \
			return TArray<FUniformBufferStruct::FMember>(); \
		} \
		typedef zzFirstMemberId

/** Declares a member of a uniform buffer struct. */
#define DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY_EX(MemberType,MemberName,ArrayDecl,Precision) \
		zzMemberId##MemberName; \
	public: \
		typedef MemberType zzA##MemberName ArrayDecl; \
		typedef TUniformBufferTypeInfo<zzA##MemberName>::TAlignedType zzT##MemberName; \
		zzT##MemberName MemberName; \
		checkAtCompileTime(TUniformBufferTypeInfo<zzT##MemberName>::BaseType != UBMT_INVALID,InvalidMemberType##MemberName); \
	private: \
		struct zzNextMemberId##MemberName {}; \
		static TArray<FUniformBufferStruct::FMember> zzGetMembersBefore(zzNextMemberId##MemberName) \
		{ \
			/* Route the member enumeration on to the function for the member following this. */ \
			TArray<FUniformBufferStruct::FMember> OutMembers = zzGetMembersBefore(zzMemberId##MemberName()); \
			/* Add this member. */ \
			OutMembers.Add(FUniformBufferStruct::FMember( \
				TEXT(#MemberName), \
				STRUCT_OFFSET(zzTThisStruct,MemberName), \
				(EUniformBufferBaseType)TUniformBufferTypeInfo<zzT##MemberName>::BaseType, \
				Precision, \
				TUniformBufferTypeInfo<zzT##MemberName>::NumRows, \
				TUniformBufferTypeInfo<zzT##MemberName>::NumColumns, \
				TUniformBufferTypeInfo<zzT##MemberName>::NumElements, \
				TUniformBufferTypeInfo<zzT##MemberName>::GetStruct() \
				)); \
			checkAtCompileTime( \
				(STRUCT_OFFSET(zzTThisStruct,MemberName) & (TUniformBufferTypeInfo<zzT##MemberName>::Alignment - 1)) == 0, \
				MisalignedUniformBufferStructMember##MemberName \
				); \
			return OutMembers; \
		} \
		typedef zzNextMemberId##MemberName

#define DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY(MemberType,MemberName,ArrayDecl) DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY_EX(MemberType,MemberName,ArrayDecl,EShaderPrecisionModifier::Float)

#define DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(MemberType,MemberName) DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY_EX(MemberType,MemberName,,EShaderPrecisionModifier::Float)

#define DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(MemberType,MemberName,Precision) DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY_EX(MemberType,MemberName,,Precision)

/** Ends a uniform buffer struct declaration. */
#define END_UNIFORM_BUFFER_STRUCT(Name) \
			zzLastMemberId; \
		static TArray<FUniformBufferStruct::FMember> zzGetMembers() { return zzGetMembersBefore(zzLastMemberId()); } \
	} GCC_ALIGN(UNIFORM_BUFFER_STRUCT_ALIGNMENT); \
	template<> class TUniformBufferTypeInfo<Name> \
	{ \
	public: \
		enum { BaseType = UBMT_STRUCT }; \
		enum { NumRows = 1 }; \
		enum { NumColumns = 1 }; \
		enum { Alignment = UNIFORM_BUFFER_STRUCT_ALIGNMENT }; \
		static const FUniformBufferStruct* GetStruct() { return &Name::StaticStruct; } \
	};


/** Finds the FUniformBufferStruct corresponding to the given name, or NULL if not found. */
extern RENDERCORE_API FUniformBufferStruct* FindUniformBufferStructByName(const TCHAR* StructName);