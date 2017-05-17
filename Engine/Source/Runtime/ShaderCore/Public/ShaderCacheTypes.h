// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
ShaderCacheTypes.h: Shader Cache Types
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "RHI.h"
#include "RHIDefinitions.h"

/**
 * FRWMutex - Read/Write Mutex
 *	- Provides Read/Write (or shared-exclusive) access.  
 *	- As a fallback default for non implemented platforms, using a single FCriticalSection to provide complete single mutual exclusion - no seperate Read/Write access.
 *
 *	TODOs:
 *		- Add Xbox and Windows specific lock structures/calls Ref: https://msdn.microsoft.com/en-us/library/windows/desktop/aa904937(v=vs.85).aspx
 *	Notes:
 *		- pThreads and Win32 API's don't provide a mechanism for upgrading a ownership of a read lock to a write lock - to get round that this system unlocks then acquires a write lock.
 *		- If trying to use system described here (https://en.wikipedia.org/wiki/Readers–writer_lock) to implement a read-write lock:
 *			- The two mutex solution relies on a seperate thread being able to unlock a mutex locked on a different thread (Needs more work than described to implement if this is not the case).
 *			- Doesn't seem to be a generic 'engine' conditional variable for the mutex+conditional solution.
 *		- Currently has an experimental thread_local lock level implementation (set DEVELOPER_TEST_RWLOCK_LEVELS to 1) for potential deadlock detection (see: http://www.drdobbs.com/parallel/use-lock-hierarchies-to-avoid-deadlock/204801163?pgno=4)
 *			- Define the locking order by passing an integer in the Mutex constructor
 *			- To avoid deadlocks lock in increasing sequence order (can miss ones out but can't go lower)
 *			- Asserts if trying to get a same or lower level lock as this could result in deadlock depending on situation/timing etc.
 */
 
#define DEVELOPER_TEST_RWLOCK_LEVELS 0
#define RWLOCK_DEBUG_CHECKS (DO_CHECK && PLATFORM_DESKTOP && !UE_BUILD_SHIPPING && !UE_BUILD_SHIPPING_WITH_EDITOR && DEVELOPER_TEST_RWLOCK_LEVELS)
 
class FRWMutex
{
	friend class FRWScopeLock;
	
public:

	// Default to level zero, we are not allowed to take another at the same level or lower - so no more than one 'default' mutex at once
	FRWMutex(uint32 Level = 0)
#if RWLOCK_DEBUG_CHECKS
	: MutexLevel(Level)
#endif
	{
#if PLATFORM_USE_PTHREADS
		int Err = pthread_rwlock_init(&Mutex, nullptr);
		checkf(Err == 0, TEXT("pthread_rwlock_init failed with error: %d"), Err);
#endif
	}

	~FRWMutex()
	{
#if PLATFORM_USE_PTHREADS
		int Err = pthread_rwlock_destroy(&Mutex);
		checkf(Err == 0, TEXT("pthread_rwlock_destroy failed with error: %d"), Err);
#endif
	}
	
private:

#if RWLOCK_DEBUG_CHECKS
	void CheckAndAddLockLevel()
	{
		if(LockLevels.Num())
		{
			uint32 Last = LockLevels.Last();
			check(MutexLevel > Last && "Trying to lock on an object with the same or lower level, options: [change lock order|change lock level values|disable this]");
		}
		
		LockLevels.Add(MutexLevel);
	}
#endif
	
	void ReadLock()
	{
#if RWLOCK_DEBUG_CHECKS
		CheckAndAddLockLevel();
#endif
	
#if PLATFORM_USE_PTHREADS
		int Err = pthread_rwlock_rdlock(&Mutex);
		checkf(Err == 0, TEXT("pthread_rwlock_rdlock failed with error: %d"), Err);
#else
		Mutex.Lock();
#endif
	}
	
	void WriteLock()
	{
#if RWLOCK_DEBUG_CHECKS
		CheckAndAddLockLevel();
#endif

#if PLATFORM_USE_PTHREADS
		int Err = pthread_rwlock_wrlock(&Mutex);
		checkf(Err == 0, TEXT("pthread_rwlock_wrlock failed with error: %d"), Err);
#else
		Mutex.Lock();
#endif
	}
	
	void Unlock()
	{
#if PLATFORM_USE_PTHREADS
		int Err = pthread_rwlock_unlock(&Mutex);
		checkf(Err == 0, TEXT("pthread_rwlock_unlock failed with error: %d"), Err);
#else
		Mutex.Unlock();
#endif

#if RWLOCK_DEBUG_CHECKS
		LockLevels.Pop();
#endif
	}
	
	void RaiseLockToWrite()
	{
#if PLATFORM_USE_PTHREADS
		int Err = pthread_rwlock_unlock(&Mutex);
		checkf(Err == 0, TEXT("pthread_rwlock_unlock failed with error: %d"), Err);
		Err = pthread_rwlock_wrlock(&Mutex);
		checkf(Err == 0, TEXT("pthread_rwlock_wrlock failed with error: %d"), Err);
#else
		//NOP
#endif		
	}

private:
#if PLATFORM_USE_PTHREADS
	pthread_rwlock_t Mutex;
#else
	FCriticalSection Mutex;
#endif

#if RWLOCK_DEBUG_CHECKS
	uint32 MutexLevel;
	static thread_local TArray<uint32> LockLevels;
#endif
};

//
// A scope lifetime controlled Read or Write lock of referenced mutex object
//
enum FRWScopeLockType
{
	SLT_ReadOnly = 0,
	SLT_Write,
};

class FRWScopeLock
{
public:
	FRWScopeLock(FRWMutex& InLockObject,FRWScopeLockType InLockType) 
	: LockObject(InLockObject)
	, LockType(InLockType)
	{
		if(LockType != SLT_ReadOnly)
		{
			LockObject.WriteLock();
		}
		else
		{
			LockObject.ReadLock();
		}
	}
	
	void RaiseLockToWrite()
	{	
		if(LockType == SLT_ReadOnly)
		{
			LockObject.RaiseLockToWrite();
			LockType = SLT_Write;
		}
	}

	~FRWScopeLock()
	{
		LockObject.Unlock();
	}

private:

	FRWScopeLock();
	FRWScopeLock(const FRWScopeLock&);
	FRWScopeLock& operator=(const FRWScopeLock&);
	
private:
	FRWMutex& LockObject;
	FRWScopeLockType LockType;
};


/** Texture type enum for shader cache draw keys */
enum EShaderCacheTextureType
{
	SCTT_Invalid,
	SCTT_Texture1D,
	SCTT_Texture2D,
	SCTT_Texture3D,
	SCTT_TextureCube,
	SCTT_Texture1DArray,
	SCTT_Texture2DArray,
	SCTT_TextureCubeArray,
	SCTT_Buffer, 
	SCTT_TextureExternal2D
};

/** The minimum texture state required for logging shader draw states */
struct SHADERCORE_API FShaderTextureKey
{
	mutable uint32 Hash;
	uint32 X;
	uint32 Y;
	uint32 Z;
	uint32 Flags; // ETextureCreateFlags
	uint32 MipLevels;
	uint32 Samples;
	uint8 Format;
	TEnumAsByte<EShaderCacheTextureType> Type;
	
	FShaderTextureKey()
	: Hash(0)
	, X(0)
	, Y(0)
	, Z(0)
	, Flags(0)
	, MipLevels(0)
	, Samples(0)
	, Format(PF_Unknown)
	, Type(SCTT_Invalid)
	{
	}
	
	friend bool operator ==(const FShaderTextureKey& A,const FShaderTextureKey& B)
	{
		return A.X == B.X && A.Y == B.Y && A.Z == B.Z && A.Flags == B.Flags && A.MipLevels == B.MipLevels && A.Samples == B.Samples && A.Format == B.Format && A.Type == B.Type;
	}
	
	friend uint32 GetTypeHash(const FShaderTextureKey &Key)
	{
		if(!Key.Hash)
		{
			Key.Hash = Key.X * 3;
			Key.Hash ^= Key.Y * 2;
			Key.Hash ^= Key.Z;
			Key.Hash ^= Key.Flags;
			Key.Hash ^= (Key.Format << 24);
			Key.Hash ^= (Key.MipLevels << 16);
			Key.Hash ^= (Key.Samples << 8);
			Key.Hash ^= Key.Type;
		}
		return Key.Hash;
	}
	
	friend FArchive& operator<<( FArchive& Ar, FShaderTextureKey& Info )
	{
		return Ar << Info.Format << Info.Type << Info.Samples << Info.MipLevels << Info.Flags << Info.X << Info.Y << Info.Z << Info.Hash;
	}
};

/** SRV state tracked by the shader-cache to properly predraw shaders */
struct SHADERCORE_API FShaderResourceKey
{
	FShaderTextureKey Tex;
	mutable uint32 Hash;
	uint32 BaseMip;
	uint32 MipLevels;
	uint8 Format;
	bool bSRV;
	
	FShaderResourceKey()
	: Hash(0)
	, BaseMip(0)
	, MipLevels(0)
	, Format(0)
	, bSRV(false)
	{
	}
	
	friend bool operator ==(const FShaderResourceKey& A,const FShaderResourceKey& B)
	{
		return A.BaseMip == B.BaseMip && A.MipLevels == B.MipLevels && A.Format == B.Format && A.bSRV == B.bSRV && A.Tex == B.Tex;
	}
	
	friend uint32 GetTypeHash(const FShaderResourceKey &Key)
	{
		if(!Key.Hash)
		{
			Key.Hash = GetTypeHash(Key.Tex);
			Key.Hash ^= (Key.BaseMip << 24);
			Key.Hash ^= (Key.MipLevels << 16);
			Key.Hash ^= (Key.Format << 8);
			Key.Hash ^= (uint32)Key.bSRV;
		}
		return Key.Hash;
	}
	
	friend FArchive& operator<<( FArchive& Ar, FShaderResourceKey& Info )
	{
		return Ar << Info.Tex << Info.BaseMip << Info.MipLevels << Info.Format << Info.bSRV << Info.Hash;
	}
};

/** Render target state tracked for predraw */
struct SHADERCORE_API FShaderRenderTargetKey
{
	FShaderTextureKey Texture;
	mutable uint32 Hash;
	uint32 MipLevel;
	uint32 ArrayIndex;
	
	FShaderRenderTargetKey()
	: Hash(0)
	, MipLevel(0)
	, ArrayIndex(0)
	{
		
	}
	
	friend bool operator ==(const FShaderRenderTargetKey& A,const FShaderRenderTargetKey& B)
	{
		return A.MipLevel == B.MipLevel && A.ArrayIndex == B.ArrayIndex && A.Texture == B.Texture;
	}
	
	friend uint32 GetTypeHash(const FShaderRenderTargetKey &Key)
	{
		if(!Key.Hash)
		{
			Key.Hash = GetTypeHash(Key.Texture);
			Key.Hash ^= (Key.MipLevel << 8);
			Key.Hash ^= Key.ArrayIndex;
		}
		return Key.Hash;
	}
	
	friend FArchive& operator<<( FArchive& Ar, FShaderRenderTargetKey& Info )
	{
		return Ar << Info.Texture << Info.MipLevel << Info.ArrayIndex << Info.Hash;
	}
};

struct SHADERCORE_API FShaderCacheKey
{
	FShaderCacheKey() : Platform(SP_NumPlatforms), Frequency(SF_NumFrequencies), Hash(0), bActive(false) {}
	
	FSHAHash SHAHash;
	EShaderPlatform Platform;
	EShaderFrequency Frequency;
	mutable uint32 Hash;
	bool bActive;
	
	friend bool operator ==(const FShaderCacheKey& A,const FShaderCacheKey& B)
	{
		return A.SHAHash == B.SHAHash && A.Platform == B.Platform && A.Frequency == B.Frequency && A.bActive == B.bActive;
	}
	
	friend uint32 GetTypeHash(const FShaderCacheKey &Key)
	{
		if(!Key.Hash)
		{
			uint32 TargetFrequency = Key.Frequency;
			uint32 TargetPlatform = Key.Platform;
			Key.Hash = FCrc::MemCrc_DEPRECATED((const void*)&Key.SHAHash, sizeof(Key.SHAHash)) ^ GetTypeHash(TargetPlatform) ^ (GetTypeHash(TargetFrequency) << 16) ^ GetTypeHash(Key.bActive);
		}
		return Key.Hash;
	}
	
	friend FArchive& operator<<( FArchive& Ar, FShaderCacheKey& Info )
	{
		uint32 TargetFrequency = Info.Frequency;
		uint32 TargetPlatform = Info.Platform;
		Ar << TargetFrequency << TargetPlatform;
		Info.Frequency = (EShaderFrequency)TargetFrequency;
		Info.Platform = (EShaderPlatform)TargetPlatform;
		return Ar << Info.SHAHash << Info.bActive << Info.Hash;
	}
};

struct SHADERCORE_API FShaderDrawKey
{
	enum
	{
		MaxNumSamplers = 16,
		MaxNumResources = 128,
		NullState = ~(0u),
		InvalidState = ~(1u)
	};
	
	FShaderDrawKey()
	: DepthStencilTarget(NullState)
	, Hash(0)
	, IndexType(0)
	{
		FMemory::Memzero(&BlendState, sizeof(BlendState));
		FMemory::Memzero(&RasterizerState, sizeof(RasterizerState));
		FMemory::Memzero(&DepthStencilState, sizeof(DepthStencilState));
		FMemory::Memset(RenderTargets, 255, sizeof(RenderTargets));
		FMemory::Memset(SamplerStates, 255, sizeof(SamplerStates));
		FMemory::Memset(Resources, 255, sizeof(Resources));
		check(GetMaxTextureSamplers() <= MaxNumSamplers);
	}
	
	struct FShaderRasterizerState
	{
		FShaderRasterizerState() { FMemory::Memzero(*this); }
		FShaderRasterizerState(FRasterizerStateInitializerRHI const& Other) { operator=(Other); }
		
		float DepthBias;
		float SlopeScaleDepthBias;
		TEnumAsByte<ERasterizerFillMode> FillMode;
		TEnumAsByte<ERasterizerCullMode> CullMode;
		bool bAllowMSAA;
		bool bEnableLineAA;
		
		FShaderRasterizerState& operator=(FRasterizerStateInitializerRHI const& Other)
		{
			DepthBias = Other.DepthBias;
			SlopeScaleDepthBias = Other.SlopeScaleDepthBias;
			FillMode = Other.FillMode;
			CullMode = Other.CullMode;
			bAllowMSAA = Other.bAllowMSAA;
			bEnableLineAA = Other.bEnableLineAA;
			return *this;
		}
		
		operator FRasterizerStateInitializerRHI () const
		{
			FRasterizerStateInitializerRHI Initializer = {FillMode, CullMode, DepthBias, SlopeScaleDepthBias, bAllowMSAA, bEnableLineAA};
			return Initializer;
		}
		
		friend FArchive& operator<<(FArchive& Ar,FShaderRasterizerState& RasterizerStateInitializer)
		{
			Ar << RasterizerStateInitializer.DepthBias;
			Ar << RasterizerStateInitializer.SlopeScaleDepthBias;
			Ar << RasterizerStateInitializer.FillMode;
			Ar << RasterizerStateInitializer.CullMode;
			Ar << RasterizerStateInitializer.bAllowMSAA;
			Ar << RasterizerStateInitializer.bEnableLineAA;
			return Ar;
		}
		
		friend uint32 GetTypeHash(const FShaderRasterizerState &Key)
		{
			uint32 KeyHash = (*((uint32*)&Key.DepthBias) ^ *((uint32*)&Key.SlopeScaleDepthBias));
			KeyHash ^= (Key.FillMode << 8);
			KeyHash ^= Key.CullMode;
			KeyHash ^= Key.bAllowMSAA ? 2 : 0;
			KeyHash ^= Key.bEnableLineAA ? 1 : 0;
			return KeyHash;
		}
	};
	
	static bool bTrackDrawResources;
	FBlendStateInitializerRHI BlendState;
	FShaderRasterizerState RasterizerState;
	FDepthStencilStateInitializerRHI DepthStencilState;
	uint32 RenderTargets[MaxSimultaneousRenderTargets];
	uint32 SamplerStates[SF_NumFrequencies][MaxNumSamplers];
	uint32 Resources[SF_NumFrequencies][MaxNumResources];
	uint32 DepthStencilTarget;
	mutable uint32 Hash;
	uint8 IndexType;
	
	friend bool operator ==(const FShaderDrawKey& A,const FShaderDrawKey& B)
	{
		bool Compare = A.IndexType == B.IndexType &&
		A.DepthStencilTarget == B.DepthStencilTarget &&
		(!FShaderDrawKey::bTrackDrawResources || FMemory::Memcmp(&A.Resources, &B.Resources, sizeof(A.Resources)) == 0) &&
		(!FShaderDrawKey::bTrackDrawResources || FMemory::Memcmp(&A.SamplerStates, &B.SamplerStates, sizeof(A.SamplerStates)) == 0) &&
		FMemory::Memcmp(&A.BlendState, &B.BlendState, sizeof(FBlendStateInitializerRHI)) == 0 &&
		FMemory::Memcmp(&A.RasterizerState, &B.RasterizerState, sizeof(FShaderRasterizerState)) == 0 &&
		FMemory::Memcmp(&A.DepthStencilState, &B.DepthStencilState, sizeof(FDepthStencilStateInitializerRHI)) == 0 &&
		FMemory::Memcmp(&A.RenderTargets, &B.RenderTargets, sizeof(A.RenderTargets)) == 0;
		return Compare;
	}
	
	friend uint32 GetTypeHash(const FShaderDrawKey &Key)
	{
		if(!Key.Hash)
		{
			Key.Hash ^= (Key.BlendState.bUseIndependentRenderTargetBlendStates ? (1 << 31) : 0);
			for( uint32 i = 0; i < MaxSimultaneousRenderTargets; i++ )
			{
				Key.Hash ^= (Key.BlendState.RenderTargets[i].ColorBlendOp << 24);
				Key.Hash ^= (Key.BlendState.RenderTargets[i].ColorSrcBlend << 16);
				Key.Hash ^= (Key.BlendState.RenderTargets[i].ColorDestBlend << 8);
				Key.Hash ^= (Key.BlendState.RenderTargets[i].ColorWriteMask << 0);
				Key.Hash ^= (Key.BlendState.RenderTargets[i].AlphaBlendOp << 24);
				Key.Hash ^= (Key.BlendState.RenderTargets[i].AlphaSrcBlend << 16);
				Key.Hash ^= (Key.BlendState.RenderTargets[i].AlphaDestBlend << 8);
				Key.Hash ^= Key.RenderTargets[i];
			}
			
			if (FShaderDrawKey::bTrackDrawResources)
			{
				for( uint32 i = 0; i < SF_NumFrequencies; i++ )
				{
					for( uint32 j = 0; j < MaxNumSamplers; j++ )
					{
						Key.Hash ^= Key.SamplerStates[i][j];
					}
					for( uint32 j = 0; j < MaxNumResources; j++ )
					{
						Key.Hash ^= Key.Resources[i][j];
					}
				}
			}
			
			Key.Hash ^= (Key.DepthStencilState.bEnableDepthWrite ? (1 << 31) : 0);
			Key.Hash ^= (Key.DepthStencilState.DepthTest << 24);
			Key.Hash ^= (Key.DepthStencilState.bEnableFrontFaceStencil ? (1 << 23) : 0);
			Key.Hash ^= (Key.DepthStencilState.FrontFaceStencilTest << 24);
			Key.Hash ^= (Key.DepthStencilState.FrontFaceStencilFailStencilOp << 16);
			Key.Hash ^= (Key.DepthStencilState.FrontFaceDepthFailStencilOp << 8);
			Key.Hash ^= (Key.DepthStencilState.FrontFacePassStencilOp);
			Key.Hash ^= (Key.DepthStencilState.bEnableBackFaceStencil ? (1 << 15) : 0);
			Key.Hash ^= (Key.DepthStencilState.BackFaceStencilTest << 24);
			Key.Hash ^= (Key.DepthStencilState.BackFaceStencilFailStencilOp << 16);
			Key.Hash ^= (Key.DepthStencilState.BackFaceDepthFailStencilOp << 8);
			Key.Hash ^= (Key.DepthStencilState.BackFacePassStencilOp);
			Key.Hash ^= (Key.DepthStencilState.StencilReadMask << 8);
			Key.Hash ^= (Key.DepthStencilState.StencilWriteMask);
			
			Key.Hash ^= GetTypeHash(Key.DepthStencilTarget);
			Key.Hash ^= GetTypeHash(Key.IndexType);
			
			Key.Hash ^= GetTypeHash(Key.RasterizerState);
		}
		return Key.Hash;
	}
	
	friend FArchive& operator<<( FArchive& Ar, FShaderDrawKey& Info )
	{
		if (FShaderDrawKey::bTrackDrawResources)
		{
			for ( uint32 i = 0; i < SF_NumFrequencies; i++ )
			{
				for ( uint32 j = 0; j < MaxNumSamplers; j++ )
				{
					Ar << Info.SamplerStates[i][j];
				}
				for ( uint32 j = 0; j < MaxNumResources; j++ )
				{
					Ar << Info.Resources[i][j];
				}
			}
		}
		for ( uint32 i = 0; i < MaxSimultaneousRenderTargets; i++ )
		{
			Ar << Info.RenderTargets[i];
		}
		return Ar << Info.BlendState << Info.RasterizerState << Info.DepthStencilState << Info.DepthStencilTarget << Info.IndexType << Info.Hash;
	}
};

struct SHADERCORE_API FShaderPipelineKey
{
	FShaderCacheKey VertexShader;
	FShaderCacheKey PixelShader;
	FShaderCacheKey GeometryShader;
	FShaderCacheKey HullShader;
	FShaderCacheKey DomainShader;
	mutable uint32 Hash;
	
	FShaderPipelineKey() : Hash(0) {}
	
	friend bool operator ==(const FShaderPipelineKey& A,const FShaderPipelineKey& B)
	{
		return A.VertexShader == B.VertexShader && A.PixelShader == B.PixelShader && A.GeometryShader == B.GeometryShader && A.HullShader == B.HullShader && A.DomainShader == B.DomainShader;
	}
	
	friend uint32 GetTypeHash(const FShaderPipelineKey &Key)
	{
		if(!Key.Hash)
		{
			Key.Hash ^= GetTypeHash(Key.VertexShader) ^ GetTypeHash(Key.PixelShader) ^ GetTypeHash(Key.GeometryShader) ^ GetTypeHash(Key.HullShader) ^ GetTypeHash(Key.DomainShader);
		}
		return Key.Hash;
	}
	
	friend FArchive& operator<<( FArchive& Ar, FShaderPipelineKey& Info )
	{
		return Ar << Info.VertexShader << Info.PixelShader << Info.GeometryShader << Info.HullShader << Info.DomainShader << Info.Hash;
	}
};

struct SHADERCORE_API FShaderCodeCache
{
	friend FArchive& operator<<( FArchive& Ar, FShaderCodeCache& Info );
	
	// Serialised
	TMap<FShaderCacheKey, TPair<uint32, TArray<uint8>>> Shaders;
	TMap<FShaderCacheKey, TSet<FShaderPipelineKey>> Pipelines;

	// Non-serialised
#if WITH_EDITORONLY_DATA
	TMap<FShaderCacheKey, TArray<TPair<int32, TArray<uint8>>>> Counts;
#endif

	friend FArchive& operator<<( FArchive& Ar, FShaderCodeCache& Info );
};

struct FShaderCacheBoundState
{
	FVertexDeclarationElementList VertexDeclaration;
	FShaderCacheKey VertexShader;
	FShaderCacheKey PixelShader;
	FShaderCacheKey GeometryShader;
	FShaderCacheKey HullShader;
	FShaderCacheKey DomainShader;
	mutable uint32 Hash;
	
	FShaderCacheBoundState() : Hash(0) {}
	
	friend bool operator ==(const FShaderCacheBoundState& A,const FShaderCacheBoundState& B)
	{
		if(A.VertexDeclaration.Num() == B.VertexDeclaration.Num())
		{
			for(int32 i = 0; i < A.VertexDeclaration.Num(); i++)
			{
				if(FMemory::Memcmp(&A.VertexDeclaration[i], &B.VertexDeclaration[i], sizeof(FVertexElement)))
				{
					return false;
				}
			}
			return A.VertexShader == B.VertexShader && A.PixelShader == B.PixelShader && A.GeometryShader == B.GeometryShader && A.HullShader == B.HullShader && A.DomainShader == B.DomainShader;
		}
		return false;
	}
	
	friend uint32 GetTypeHash(const FShaderCacheBoundState &Key)
	{
		if(!Key.Hash)
		{
			for(auto Element : Key.VertexDeclaration)
			{
				Key.Hash ^= FCrc::MemCrc_DEPRECATED(&Element, sizeof(FVertexElement));
			}
			
			Key.Hash ^= GetTypeHash(Key.VertexShader) ^ GetTypeHash(Key.PixelShader) ^ GetTypeHash(Key.GeometryShader) ^ GetTypeHash(Key.HullShader) ^ GetTypeHash(Key.DomainShader);
		}
		return Key.Hash;
	}
	
	friend FArchive& operator<<( FArchive& Ar, FShaderCacheBoundState& Info )
	{
		return Ar << Info.VertexDeclaration << Info.VertexShader << Info.PixelShader << Info.GeometryShader << Info.HullShader << Info.DomainShader << Info.Hash;
	}
};

struct FSamplerStateInitializerRHIKeyFuncs : TDefaultMapKeyFuncs<FSamplerStateInitializerRHI,int32,false>
{
	/**
	 * @return True if the keys match.
	 */
	static bool Matches(KeyInitType A,KeyInitType B);
	
	/** Calculates a hash index for a key. */
	static uint32 GetKeyHash(KeyInitType Key);
};

struct FShaderStreamingCache
{
	TMap<int32, TSet<int32>> ShaderDrawStates;
	
	friend FArchive& operator<<( FArchive& Ar, FShaderStreamingCache& Info )
	{
		return Ar << Info.ShaderDrawStates;
	}
};

template <class Type, typename KeyFuncs = TDefaultMapKeyFuncs<Type,int32,false>>
class TIndexedSet
{
	TMap<Type, int32, FDefaultSetAllocator, KeyFuncs> Map;
	TArray<Type> Data;
	
public:
#if PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS
	TIndexedSet() = default;
	TIndexedSet(TIndexedSet&&) = default;
	TIndexedSet(const TIndexedSet&) = default;
	TIndexedSet& operator=(TIndexedSet&&) = default;
	TIndexedSet& operator=(const TIndexedSet&) = default;
#else
	FORCEINLINE TIndexedSet() {}
	FORCEINLINE TIndexedSet(      TIndexedSet&& Other) : Map(MoveTemp(Other.Map)), Data(MoveTemp(Other.Data)) {}
	FORCEINLINE TIndexedSet(const TIndexedSet&  Other) : Map( Other.Map ), Data( Other.Data ) {}
	FORCEINLINE TIndexedSet& operator=(      TIndexedSet&& Other) { Map = MoveTemp(Other.Map); Data = MoveTemp(Other.Data); return *this; }
	FORCEINLINE TIndexedSet& operator=(const TIndexedSet&  Other) { Map = Other.Map; Data = Other.Data; return *this; }
#endif
	
	int32 Add(Type const& Object)
	{
		int32* Index = Map.Find(Object);
		if(Index)
		{
			return *Index;
		}
		else
		{
			int32 NewIndex = Data.Num();
			Data.Push(Object);
			Map.Add(Object, NewIndex);
			return NewIndex;
		}
	}
	
	int32 FindIndex(Type const& Object) const
	{
		int32 const* Index = Map.Find(Object);
		if(Index)
		{
			return *Index;
		}
		else
		{
			check(false);
			return -1;
		}
	}
	
	int32 FindIndexChecked(Type const& Object)
	{
		return Map.FindChecked(Object);
	}
	
	Type& operator[](int32 Index)
	{
		return Data[Index];
	}
	
	Type const& operator[](int32 Index) const
	{
		return Data[Index];
	}
	
	uint32 Num() const
	{
		return Data.Num();
	}
	
	friend FORCEINLINE FArchive& operator<<( FArchive& Ar, TIndexedSet& Set )
	{
		return Ar << Set.Map << Set.Data;
	}
	
	friend FORCEINLINE FArchive& operator<<( FArchive& Ar, const TIndexedSet& Set )
	{
		return Ar << Set.Map << Set.Data;
	}
};

struct FShaderPreDrawEntry
{
	int32 BoundStateIndex;
	int32 DrawKeyIndex;
	bool bPredrawn;
	
	FShaderPreDrawEntry() : BoundStateIndex(-1), DrawKeyIndex(-1), bPredrawn(false) {}
	
	friend bool operator ==(const FShaderPreDrawEntry& A,const FShaderPreDrawEntry& B)
	{
		return (A.BoundStateIndex == B.BoundStateIndex && A.DrawKeyIndex == B.DrawKeyIndex);
	}
	
	friend uint32 GetTypeHash(const FShaderPreDrawEntry &Key)
	{
		return (Key.BoundStateIndex ^ Key.DrawKeyIndex);
	}
	
	friend FArchive& operator<<( FArchive& Ar, FShaderPreDrawEntry& Info )
	{
		if(Ar.IsLoading())
		{
			Info.bPredrawn = false;
		}
		return Ar << Info.BoundStateIndex << Info.DrawKeyIndex;
	}
};

struct FShaderPlatformCache
{		
	friend FArchive& operator<<( FArchive& Ar, FShaderPlatformCache& Info )
	{
		return Ar << Info.Shaders << Info.BoundShaderStates << Info.DrawStates << Info.RenderTargets << Info.Resources << Info.SamplerStates << Info.PreDrawEntries << Info.ShaderStateMembership << Info.StreamingDrawStates;
	}

	TIndexedSet<FShaderCacheKey> Shaders;
	TIndexedSet<FShaderCacheBoundState> BoundShaderStates;
	TIndexedSet<FShaderDrawKey> DrawStates;
	TIndexedSet<FShaderRenderTargetKey> RenderTargets;
	TIndexedSet<FShaderResourceKey> Resources;
	TIndexedSet<FSamplerStateInitializerRHI, FSamplerStateInitializerRHIKeyFuncs> SamplerStates;
	TIndexedSet<FShaderPreDrawEntry> PreDrawEntries;
	
	TMap<int32, TSet<int32>> ShaderStateMembership;
	TMap<uint32, FShaderStreamingCache> StreamingDrawStates;
};

struct FShaderCaches
{
public:
	friend FArchive& operator<<( FArchive& Ar, FShaderCaches& Info );
	
	TMap<uint32, FShaderPlatformCache> PlatformCaches;
};

struct FShaderResourceViewBinding
{
	FShaderResourceViewBinding()
	: SRV(nullptr)
	, VertexBuffer(nullptr)
	, Texture(nullptr)
	{}
	
	FShaderResourceViewBinding(FShaderResourceViewRHIParamRef InSrv, FVertexBufferRHIParamRef InVb, FTextureRHIParamRef InTexture)
	: SRV(nullptr)
	, VertexBuffer(nullptr)
	, Texture(nullptr)
	{}
	
	FShaderResourceViewRHIParamRef SRV;
	FVertexBufferRHIParamRef VertexBuffer;
	FTextureRHIParamRef Texture;
};

struct FShaderTextureBinding
{
	FShaderTextureBinding() {}
	FShaderTextureBinding(FShaderResourceViewBinding const& Other)
	{
		operator=(Other);
	}
	
	FShaderTextureBinding& operator=(FShaderResourceViewBinding const& Other)
	{
		SRV = Other.SRV;
		VertexBuffer = Other.VertexBuffer;
		Texture = Other.Texture;
		return *this;
	}
	
	friend bool operator ==(const FShaderTextureBinding& A,const FShaderTextureBinding& B)
	{
		return A.SRV == B.SRV && A.VertexBuffer == B.VertexBuffer && A.Texture == B.Texture;
	}
	
	friend uint32 GetTypeHash(const FShaderTextureBinding &Key)
	{
		uint32 Hash = GetTypeHash(Key.SRV);
		Hash ^= GetTypeHash(Key.VertexBuffer);
		Hash ^= GetTypeHash(Key.Texture);
		return Hash;
	}
	
	FShaderResourceViewRHIRef SRV;
	FVertexBufferRHIRef VertexBuffer;
	FTextureRHIRef Texture;
};

//};
