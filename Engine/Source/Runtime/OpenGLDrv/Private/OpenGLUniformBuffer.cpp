// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	OpenGLUniformBuffer.cpp: OpenGL Uniform buffer RHI implementation.
=============================================================================*/

#include "OpenGLDrvPrivate.h"

#define NUM_POOL_BUCKETS 45

#define NUM_SAFE_FRAMES 3

const uint32 UniformBufferSizeBuckets[NUM_POOL_BUCKETS] = {
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

	0xFFFFFFFF
};

static uint32 GetPoolBucketIndex(uint32 NumBytes)
{
	unsigned long lower = 0;
	unsigned long upper = NUM_POOL_BUCKETS;
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
	uint32 FrameFreed;
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

	// Clean a limited number of old entries to reduce hitching when leaving a large level
	const bool bIsES2 = IsES2Platform(GRHIShaderPlatform);
	for( int32 StreamedIndex = 0; StreamedIndex < 2; ++StreamedIndex)
	{
		for (int32 BucketIndex = 0; BucketIndex < NUM_POOL_BUCKETS; BucketIndex++)
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

	// Index of the bucket that is now old enough to be reused
	const int32 SafeFrameIndex = GFrameNumberRenderThread % NUM_SAFE_FRAMES;

	// Merge the bucket into the free pool array
	for( int32 StreamedIndex = 0; StreamedIndex < 2; ++StreamedIndex)
	{
		for (int32 BucketIndex = 0; BucketIndex < NUM_POOL_BUCKETS; BucketIndex++)
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


static FUniformBufferRHIRef CreateUniformBuffer(uint32 InStride,uint32 InSize,uint32 InUsage, const void *InData = NULL, bool bStreamedDraw = false, GLuint ResourceToUse = 0, uint32 ResourceSize = 0)
{
	if (IsES2Platform(GRHIShaderPlatform))
	{
		return new FOpenGLEUniformBuffer(InStride, InSize, InUsage, InData, bStreamedDraw, ResourceToUse, ResourceSize);
	}
	else
	{
		return new FOpenGLUniformBuffer(InStride, InSize, InUsage, InData, bStreamedDraw, ResourceToUse, ResourceSize);
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

			return CreateUniformBuffer(0,NumBytes,0,Contents,bStreamDraw,FreeBufferEntry.Buffer,FreeBufferEntry.CreatedSize);
		}
		else
		{
			// Nothing usable was found in the free pool, create a new uniform buffer
			uint32 BufferSize = UniformBufferSizeBuckets[BucketIndex];
			return CreateUniformBuffer(0,NumBytes,0,Contents,bStreamDraw,0,BufferSize);
		}
	}
	else
	{
		return CreateUniformBuffer(0, NumBytes, 0, Contents, bStreamDraw);
	}
}

void AddNewlyFreedBufferToUniformBufferPool( GLuint Buffer, uint32 BufferSize, bool bStreamDraw )
{
	if(IsPoolingEnabled())
	{
		check(Buffer);
		FPooledGLUniformBuffer NewEntry;
		NewEntry.Buffer = Buffer;
		NewEntry.FrameFreed = GFrameNumberRenderThread;
		NewEntry.CreatedSize = BufferSize;

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
