/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#ifndef DIR_ENTRY_H
#define DIR_ENTRY_H

#include "foundation/PxPreprocessor.h"

#if defined PX_NX
#	include "nn/fs.h"
#	include "nn/fs/fs_Directory.h"
#else
#	error Unsupported platform
#endif

namespace physx
{
	class DirEntry
	{
	public:

		DirEntry()
		{
			mDirEntries = NULL;
			mIdx = 0;
			mCount = 0;
		}

		~DirEntry()
		{
			release();
		}
		
		void release()
		{
			if(mDirEntries)
			{
				delete[] mDirEntries;
				mDirEntries = NULL;
				
				CloseDirectory(mDirHandle);
			}
			
			mCount = 0;
		}

		// Get successive element of directory.
		// Returns true on success, error otherwise.
		bool next()
		{
			if (mIdx < mCount - 1)
			{
				++mIdx;
			}
			else
			{
				release();
				return false;
			}
			return true;
		}

		// No more entries in directory?
		bool isDone() const
		{
			return mIdx >= mCount;

		}

		// Is this entry a directory?
		bool isDirectory() const
		{
			if (mIdx < mCount)
			{
				return mDirEntries[mIdx].directoryEntryType == nn::fs::DirectoryEntryType_Directory;
			}
			else
			{
				return false;
			}
		}

		// Get name of this entry.
		const char* getName() const
		{
			if (mIdx < mCount)
			{
				return mDirEntries[mIdx].name;;
			}
			else
			{
				return NULL;
			}
		}

		// Get first entry in directory.
		static bool GetFirstEntry(const char* path, DirEntry& dentry)
		{
			//!!!Odin check that nn::fs::SetAllocator was called before
			
			dentry.release();
			
			nn::Result res = OpenDirectory(&dentry.mDirHandle, path, nn::fs::OpenDirectoryMode::OpenDirectoryMode_All);
			if(res.IsFailure())
			{
				return false;
			}
			
			int64_t count;
			GetDirectoryEntryCount(&count, dentry.mDirHandle);
			dentry.mDirEntries = new nn::fs::DirectoryEntry[static_cast<uint32_t>(count)];
			ReadDirectory(&dentry.mCount, dentry.mDirEntries, dentry.mDirHandle, count);
			dentry.mIdx = 0;
			
			return true;
		}

	private:

		nn::fs::DirectoryHandle mDirHandle;
		nn::fs::DirectoryEntry* mDirEntries;
		int64_t mCount;
		int64_t mIdx;
	};
}

#endif
