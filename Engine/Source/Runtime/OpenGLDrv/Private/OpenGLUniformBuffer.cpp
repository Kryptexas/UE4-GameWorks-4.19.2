// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	OpenGLUniformBuffer.cpp: OpenGL Uniform buffer RHI implementation.
=============================================================================*/

#include "OpenGLDrvPrivate.h"


namespace OpenGLConsoleVariables
{
#if (PLATFORM_WINDOWS || PLATFORM_ANDROIDGL4)
	int32 RequestedUBOPoolSize = 1024*1024*16;
#else
	int32 RequestedUBOPoolSize = 0;
#endif

	static FAutoConsoleVariableRef CVarUBOPoolSize(
		TEXT("OpenGL.UBOPoolSize"),
		RequestedUBOPoolSize,
		TEXT("Size of the UBO pool, 0 disables UBO Pool"),
		ECVF_ReadOnly
		);

	int32 bUBODirectWrite = 1;

	static FAutoConsoleVariableRef CVarUBODirectWrite(
		TEXT("OpenGL.UBODirectWrite"),
		bUBODirectWrite,
		TEXT("Enables direct writes to the UBO via Buffer Storage"),
		ECVF_ReadOnly
		);
};

#define NUM_POOL_BUCKETS 45

#define NUM_SAFE_FRAMES 3

const uint32 RequestedUniformBufferSizeBuckets[NUM_POOL_BUCKETS] = {
	16,32,48,64,80,96,112,128,	// 16-byte increments
	160,192,224,256,			// 32-byte increments
	320,384,448,512,			// 64-byte increments
	640,768,896,1024,			// 128-byte increments
	1280,1536,1792,2048,		// 256-byte increments
	2560,3072,3584,4096,		// 512-byte increments
	5120,6144,7168,8192,		// 1024-byte increments
	10240,12288,14336,16384,	// 2048-byte increments
	20480,24576,28672,32768,	// 4096-byte increments
	40960,49152,57344,65536,	// 8192-byte increments

	// 65536 is current max uniform buffer size for Mac OS X.

	0xFFFF0000 // Not max uint32 to allow rounding
};

// Maps desired size buckets to aligment actually 
TArray<uint32> UniformBufferSizeBuckets;


static inline bool IsSuballocatingUBOs()
{
#if SUBALLOCATED_CONSTANT_BUFFER
	if (!IsES2Platform(GRHIShaderPlatform))
	{
		return OpenGLConsoleVariables::RequestedUBOPoolSize != 0;
	}
#endif
	return false;
}

static inline uint32 GetUBOPoolSize()
{
	static uint32 UBOPoolSize = 0xFFFFFFFF;

	if ( UBOPoolSize == 0xFFFFFFFF )
	{
		GLint Alignment;
		glGetIntegerv( GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &Alignment);

		UBOPoolSize = (( OpenGLConsoleVariables::RequestedUBOPoolSize + Alignment - 1) / Alignment ) * Alignment;
	}

	return UBOPoolSize;
}

// Convert bucket sizes to cbe compatible with present device
static void RemapBuckets()
{
	if (!IsSuballocatingUBOs())
	{
		for (int32 Count = 0; Count < NUM_POOL_BUCKETS; Count++)
		{
			UniformBufferSizeBuckets.Push(RequestedUniformBufferSizeBuckets[Count]);
		}
	}
	else
	{
		GLint Alignment;
		glGetIntegerv( GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &Alignment);

		for (int32 Count = 0; Count < NUM_POOL_BUCKETS; Count++)
		{
			uint32 AlignedSize = ((RequestedUniformBufferSizeBuckets[Count] + Alignment - 1) / Alignment ) * Alignment;
			if (!UniformBufferSizeBuckets.Contains(AlignedSize))
			{
				UniformBufferSizeBuckets.Push(AlignedSize);
			}
		}
		UE_LOG(LogRHI,Log,TEXT("Configured UBO bucket pool to %d buckets based on alignment of %d bytes"), UniformBufferSizeBuckets.Num(), Alignment);
	}
}

static uint32 GetPoolBucketIndex(uint32 NumBytes)
{
	if (UniformBufferSizeBuckets.Num() == 0)
	{
		RemapBuckets();
	}

	check( UniformBufferSizeBuckets.Num() > 0);

	unsigned long lower = 0;
	unsigned long upper = UniformBufferSizeBuckets.Num();
	unsigned long middle;

	do
	{
		middle = ( upper + lower ) >> 1;
		if( NumBytes <= UniformBufferSizeBuckets[middle-1] )
		{
			upper = middle;
		}
		else
		{
			lower = middle;
		}
	}
	while( upper - lower > 1 );

	check( NumBytes <= UniformBufferSizeBuckets[lower] );
	check( (lower == 0 ) || ( NumBytes > UniformBufferSizeBuckets[lower-1] ) );

	return lower;
}

static inline uint32 GetPoolBucketSize(uint32 NumBytes)
{
	return UniformBufferSizeBuckets[GetPoolBucketIndex(NumBytes)];
}


uint32 FOpenGLEUniformBuffer::UniqueIDCounter = 1;


struct FUniformBufferDataFactory
{
	TMap<GLuint, FOpenGLEUniformBufferDataRef> Entries;

	FOpenGLEUniformBufferDataRef Create(uint32 Size, GLuint& OutResource)
	{
		static GLuint TempCounter = 0;
		OutResource = ++TempCounter;

		FOpenGLEUniformBufferDataRef Buffer = new FOpenGLEUniformBufferData(Size);
		Entries.Add(OutResource, Buffer);
		return Buffer;
	}

	FOpenGLEUniformBufferDataRef Get(GLuint Resource)
	{
		FOpenGLEUniformBufferDataRef* Buffer = Entries.Find(Resource);
		check(Buffer);
		return *Buffer;
	}

	void Destroy(GLuint Resource)
	{
		Entries.Remove(Resource);
	}
};

static FUniformBufferDataFactory UniformBufferDataFactory;

FOpenGLEUniformBufferDataRef AllocateOpenGLEUniformBufferData(uint32 Size, GLuint& Resource)
{
	SCOPE_CYCLE_COUNTER(STAT_OpenGLEmulatedUniformBufferTime);
	if (Resource == 0)
	{
		return UniformBufferDataFactory.Create(Size, Resource);
	}

	return UniformBufferDataFactory.Get(Resource);
}

void FreeOpenGLEUniformBufferData(GLuint Resource)
{
	SCOPE_CYCLE_COUNTER(STAT_OpenGLEmulatedUniformBufferTime);
	UniformBufferDataFactory.Destroy(Resource);
}

// Describes a uniform buffer in the free pool.
struct FPooledGLUniformBuffer
{
	GLuint Buffer;
	uint32 CreatedSize;
	uint32 Offset;
	uint32 FrameFreed;
	uint8* Pointer;
};

// Pool of free uniform buffers, indexed by bucket for constant size search time.
TArray<FPooledGLUniformBuffer> GLUniformBufferPool[NUM_POOL_BUCKETS][2];

// Uniform buffers that have been freed more recently than NumSafeFrames ago.
TArray<FPooledGLUniformBuffer> SafeGLUniformBufferPools[NUM_SAFE_FRAMES][NUM_POOL_BUCKETS][2];

// Does per-frame global updating for the uniform buffer pool.
void BeginFrame_UniformBufferPoolCleanup()
{
	check(IsInRenderingThread());

	int32 NumToCleanThisFrame = 10;

	SCOPE_CYCLE_COUNTER(STAT_OpenGLUniformBufferCleanupTime);

	if (!IsSuballocatingUBOs())
	{
		// Clean a limited number of old entries to reduce hitching when leaving a large level
		const bool bIsES2 = IsES2Platform(GRHIShaderPlatform);
		for( int32 StreamedIndex = 0; StreamedIndex < 2; ++StreamedIndex)
		{
			for (int32 BucketIndex = 0; BucketIndex < UniformBufferSizeBuckets.Num(); BucketIndex++)
			{
				for (int32 EntryIndex = GLUniformBufferPool[BucketIndex][StreamedIndex].Num() - 1; EntryIndex >= 0; EntryIndex--)
				{
					FPooledGLUniformBuffer& PoolEntry = GLUniformBufferPool[BucketIndex][StreamedIndex][EntryIndex];

					check(PoolEntry.Buffer);

					// Clean entries that are unlikely to be reused
					if (GFrameNumberRenderThread - PoolEntry.FrameFreed > 30)
					{
						DEC_DWORD_STAT(STAT_OpenGLNumFreeUniformBuffers);
						DEC_MEMORY_STAT_BY(STAT_OpenGLFreeUniformBufferMemory, PoolEntry.CreatedSize);
						if (bIsES2)
						{
							UniformBufferDataFactory.Destroy(PoolEntry.Buffer);
						}
						else
						{
							glDeleteBuffers( 1, &PoolEntry.Buffer );
						}
						GLUniformBufferPool[BucketIndex][StreamedIndex].RemoveAtSwap(EntryIndex);

						--NumToCleanThisFrame;
						if (NumToCleanThisFrame == 0)
						{
							break;
						}
					}
				}

				if (NumToCleanThisFrame == 0)
				{
					break;
				}
			}

			if (NumToCleanThisFrame == 0)
			{
				break;
			}
		}
	}

	// Index of the bucket that is now old enough to be reused
	const int32 SafeFrameIndex = GFrameNumberRenderThread % NUM_SAFE_FRAMES;

	// Merge the bucket into the free pool array
	for( int32 StreamedIndex = 0; StreamedIndex < 2; ++StreamedIndex)
	{
		for (int32 BucketIndex = 0; BucketIndex < UniformBufferSizeBuckets.Num(); BucketIndex++)
		{
			GLUniformBufferPool[BucketIndex][StreamedIndex].Append(SafeGLUniformBufferPools[SafeFrameIndex][BucketIndex][StreamedIndex]);
			SafeGLUniformBufferPools[SafeFrameIndex][BucketIndex][StreamedIndex].Reset();
		}
	}
}

static bool IsPoolingEnabled()
{
	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.UniformBufferPooling"));
	int32 CVarValue = CVar->GetValueOnRenderThread();
	return CVarValue != 0;
};

struct TUBOPoolBuffer
{
	GLuint Resource;
	uint32 ConsumedSpace;
	uint32 AllocatedSpace;
	uint8* Pointer;
};

TArray<TUBOPoolBuffer> UBOPool;

static void SuballocateUBO( uint32 Size, GLuint& Resource, uint32& Offset, uint8*& Pointer)
{
	check( Size <= GetUBOPoolSize());
	// Find space in previously allocated pool buffers
	for ( int32 Buffer = 0; Buffer < UBOPool.Num(); Buffer++)
	{
		TUBOPoolBuffer &Pool = UBOPool[Buffer];
		if ( Size < (Pool.AllocatedSpace - Pool.ConsumedSpace))
		{
			Resource = Pool.Resource;
			Offset = Pool.ConsumedSpace;
			Pointer = Pool.Pointer ? Pool.Pointer + Offset : 0;
			Pool.ConsumedSpace += Size;
			return;
		}
	}

	// No space was found to use, create a new Pool buffer
	TUBOPoolBuffer Pool;

	FOpenGL::GenBuffers( 1, &Pool.Resource);
	
	CachedBindUniformBuffer(Pool.Resource);

	if (FOpenGL::SupportsBufferStorage() && OpenGLConsoleVariables::bUBODirectWrite)
	{
		FOpenGL::BufferStorage( GL_UNIFORM_BUFFER, GetUBOPoolSize(), NULL, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT );
		Pool.Pointer = (uint8*)FOpenGL::MapBufferRange(GL_UNIFORM_BUFFER, 0, GetUBOPoolSize(), FOpenGL::RLM_WriteOnlyPersistent);
	}
	else
	{
		glBufferData( GL_UNIFORM_BUFFER, GetUBOPoolSize(), 0, GL_DYNAMIC_DRAW);
		Pool.Pointer = 0;
	}

	Pointer = Pool.Pointer;
	
	INC_MEMORY_STAT_BY(STAT_OpenGLFreeUniformBufferMemory, GetUBOPoolSize());
	
	Pool.ConsumedSpace = Size;
	Pool.AllocatedSpace = GetUBOPoolSize();
	
	Resource = Pool.Resource;
	Offset = 0;
	
	UBOPool.Push(Pool);

	UE_LOG(LogRHI,Log,TEXT("Allocated new buffer for uniform Pool %d buffers with %d bytes"),UBOPool.Num(), UBOPool.Num()*GetUBOPoolSize());
}

static FUniformBufferRHIRef CreateUniformBuffer(uint32 InStride,uint32 InSize,uint32 InUsage, const void *InData = NULL, bool bStreamedDraw = false, GLuint ResourceToUse = 0, uint32 ResourceSize = 0, uint32 Offset = 0, uint8 *Pointer = 0)
{
	if (IsES2Platform(GRHIShaderPlatform))
	{
		return new FOpenGLEUniformBuffer(InStride, InSize, InUsage, InData, bStreamedDraw, ResourceToUse, ResourceSize);
	}
	else
	{
#if !SUBALLOCATED_CONSTANT_BUFFER
		return new FOpenGLUniformBuffer(InStride, InSize, InUsage, InData, bStreamedDraw, ResourceToUse, ResourceSize);
#else
		return new FOpenGLUniformBuffer(InStride, InSize, InUsage, InData, bStreamedDraw, ResourceToUse, ResourceSize, Offset, Pointer);
#endif
	}
}

FUniformBufferRHIRef FOpenGLDynamicRHI::RHICreateUniformBuffer(const void* Contents,uint32 NumBytes,EUniformBufferUsage Usage)
{
	check(IsInRenderingThread());

	// This should really be synchronized, if there's a chance it'll be used from more than one buffer. Luckily, uniform buffers
	// are only used for drawing/shader usage, not for loading resources or framebuffer blitting, so no synchronization primitives for now.

	// Explicitly check that the size is nonzero before allowing CreateBuffer to opaquely fail.
	check(NumBytes > 0);

	VERIFY_GL_SCOPE();

	bool bStreamDraw = (Usage==UniformBuffer_SingleUse);

	if(IsPoolingEnabled())
	{
		int StreamedIndex = bStreamDraw ? 1 : 0;

		// Find the appropriate bucket based on size
		const uint32 BucketIndex = GetPoolBucketIndex(NumBytes);
		TArray<FPooledGLUniformBuffer>& PoolBucket = GLUniformBufferPool[BucketIndex][StreamedIndex];
		if (PoolBucket.Num() > 0)
		{
			// Reuse the last entry in this size bucket
			FPooledGLUniformBuffer FreeBufferEntry = PoolBucket.Pop();
			DEC_DWORD_STAT(STAT_OpenGLNumFreeUniformBuffers);
			DEC_MEMORY_STAT_BY(STAT_OpenGLFreeUniformBufferMemory, FreeBufferEntry.CreatedSize);

			return CreateUniformBuffer(0,NumBytes,0,Contents,bStreamDraw,FreeBufferEntry.Buffer,FreeBufferEntry.CreatedSize,FreeBufferEntry.Offset,FreeBufferEntry.Pointer);
		}
		else
		{
			// Nothing usable was found in the free pool, create a new uniform buffer
			uint32 BufferSize = UniformBufferSizeBuckets[BucketIndex];
			if (IsSuballocatingUBOs())
			{
				GLuint Resource = 0;
				uint32 Offset = 0;
				uint8* Pointer = 0;
				SuballocateUBO( BufferSize, Resource, Offset, Pointer);
				return CreateUniformBuffer(0,NumBytes,0,Contents,bStreamDraw,Resource,BufferSize,Offset,Pointer);
			}
			else
			{
				return CreateUniformBuffer(0,NumBytes,0,Contents,bStreamDraw,0,BufferSize);
			}
		}
	}
	else
	{
		return CreateUniformBuffer(0, NumBytes, 0, Contents, bStreamDraw);
	}
}

void AddNewlyFreedBufferToUniformBufferPool( GLuint Buffer, uint32 BufferSize, bool bStreamDraw, uint32 Offset, uint8* Pointer )
{
	if(IsPoolingEnabled())
	{
		check(Buffer);
		FPooledGLUniformBuffer NewEntry;
		NewEntry.Buffer = Buffer;
		NewEntry.FrameFreed = GFrameNumberRenderThread;
		NewEntry.CreatedSize = BufferSize;
		NewEntry.Offset = Offset;
		NewEntry.Pointer = Pointer;

		int StreamedIndex = bStreamDraw ? 1 : 0;

		// Add to this frame's array of free uniform buffers
		const int32 SafeFrameIndex = GFrameNumberRenderThread % NUM_SAFE_FRAMES;
		const uint32 BucketIndex = GetPoolBucketIndex(BufferSize);

		check(BufferSize == UniformBufferSizeBuckets[BucketIndex]);
		// this might fail with sizes > 65536; handle it then by extending the range? sizes > 65536 are presently unsupported on Mac OS X.

		SafeGLUniformBufferPools[SafeFrameIndex][BucketIndex][StreamedIndex].Add(NewEntry);
		INC_DWORD_STAT(STAT_OpenGLNumFreeUniformBuffers);
		INC_MEMORY_STAT_BY(STAT_OpenGLFreeUniformBufferMemory, BufferSize);
	}
}
