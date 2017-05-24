// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ApplePlatformMemory.h: Apple platform memory functions common across all Apple OSes
=============================================================================*/

#include "ApplePlatformMemory.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformMath.h"
#include "HAL/UnrealMemory.h"
#include "Templates/AlignmentTemplates.h"

#include <stdlib.h>
#include <objc/runtime.h>
#include <CoreFoundation/CFBase.h>

NS_ASSUME_NONNULL_BEGIN

/** 
 * Zombie object implementation so that we can implement NSZombie behaviour for our custom allocated objects.
 * Will leak memory - just like Cocoa's NSZombie - but allows for debugging of invalid usage of the pooled types.
 */
@interface FApplePlatformObjectZombie : NSObject 
{
	@public
	Class OriginalClass;
}
@end

@implementation FApplePlatformObjectZombie
-(id)init
{
	id Self = [super init];
	if (Self)
	{
		OriginalClass = nil;
	}
	return Self;
}

-(void)dealloc
{
	// Denied!
	return;
	
	[super dealloc];
}

- (nullable NSMethodSignature *)methodSignatureForSelector:(SEL)sel
{
	NSLog(@"Selector %@ sent to deallocated instance %p of class %@", NSStringFromSelector(sel), self, OriginalClass);
	abort();
}
@end

@implementation FApplePlatformObject

+ (OSQueueHead*)classAllocator
{
	return nullptr;
}

+ (id)allocClass: (Class)NewClass
{
	static bool NSZombieEnabled = (getenv("NSZombieEnabled") != nullptr);
	
	// Allocate the correct size & zero it
	// All allocations must be 16 byte aligned
	SIZE_T Size = Align(FPlatformMath::Max(class_getInstanceSize(NewClass), class_getInstanceSize([FApplePlatformObjectZombie class])), 16);
	void* Mem = nullptr;
	
	OSQueueHead* Alloc = [NewClass classAllocator];
	if (Alloc && !NSZombieEnabled)
	{
		Mem = OSAtomicDequeue(Alloc, 0);
		if (!Mem)
		{
			static uint8 BlocksPerChunk = 32;
			char* Chunk = (char*)FMemory::Malloc(Size * BlocksPerChunk);
			Mem = Chunk;
			Chunk += Size;
			for (uint8 i = 0; i < (BlocksPerChunk - 1); i++, Chunk += Size)
			{
				OSAtomicEnqueue(Alloc, Chunk, 0);
			}
		}
	}
	else
	{
		Mem = FMemory::Malloc(Size);
	}
	FMemory::Memzero(Mem, Size);
	
	// Construction assumes & requires zero-initialised memory
	FApplePlatformObject* Obj = (FApplePlatformObject*)objc_constructInstance(NewClass, Mem);
	object_setClass(Obj, NewClass);
	Obj->AllocatorPtr = !NSZombieEnabled ? Alloc : nullptr;
	return Obj;
}

- (void)dealloc
{
	static bool NSZombieEnabled = (getenv("NSZombieEnabled") != nullptr);
	
	// First call the destructor and then release the memory - like C++ placement new/delete
	objc_destructInstance(self);
	if (AllocatorPtr)
	{
		check(!NSZombieEnabled);
		OSAtomicEnqueue(AllocatorPtr, self, 0);
	}
	else if (NSZombieEnabled)
	{
		Class CurrentClass = self.class;
		object_setClass(self, [FApplePlatformObjectZombie class]);
		FApplePlatformObjectZombie* ZombieSelf = (FApplePlatformObjectZombie*)self;
		ZombieSelf->OriginalClass = CurrentClass;
	}
	else
	{
		FMemory::Free(self);
	}
	return;
	
	// Deliberately unreachable code to silence clang's error about not calling super - which in all other
	// cases will be correct.
	[super dealloc];
}

@end

static void* FApplePlatformAllocatorAllocate(CFIndex AllocSize, CFOptionFlags Hint, void* Info)
{
	void* Mem = FMemory::Malloc(AllocSize, 16);
	return Mem;
}

static void* FApplePlatformAllocatorReallocate(void* Ptr, CFIndex Newsize, CFOptionFlags Hint, void* Info)
{
	void* Mem = FMemory::Realloc(Ptr, Newsize, 16);
	return Mem;
}

static void FApplePlatformAllocatorDeallocate(void* Ptr, void* Info)
{
	return FMemory::Free(Ptr);
}

static CFIndex FApplePlatformAllocatorPreferredSize(CFIndex Size, CFOptionFlags Hint, void* Info)
{
	return FMemory::QuantizeSize(Size);
}

void FApplePlatformMemory::ConfigureDefaultCFAllocator(void)
{
	// Configure CoreFoundation's default allocator to use our allocation routines too.
	CFAllocatorContext AllocatorContext;
	AllocatorContext.version = 0;
	AllocatorContext.info = nullptr;
	AllocatorContext.retain = nullptr;
	AllocatorContext.release = nullptr;
	AllocatorContext.copyDescription = nullptr;
	AllocatorContext.allocate = &FApplePlatformAllocatorAllocate;
	AllocatorContext.reallocate = &FApplePlatformAllocatorReallocate;
	AllocatorContext.deallocate = &FApplePlatformAllocatorDeallocate;
	AllocatorContext.preferredSize = &FApplePlatformAllocatorPreferredSize;
	
	CFAllocatorRef Alloc = CFAllocatorCreate(kCFAllocatorDefault, &AllocatorContext);
	CFAllocatorSetDefault(Alloc);
}

NS_ASSUME_NONNULL_END
