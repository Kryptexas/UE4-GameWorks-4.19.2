// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GenericPlatformFile.cpp: Generic implementations of platform file I/O functions
=============================================================================*/

#include "CorePrivate.h"
#include "ModuleManager.h"

int64 IFileHandle::Size()
{
	int64 Current = Tell();
	SeekFromEnd();
	int64 Result = Tell();
	Seek(Current);
	return Result;
}

const TCHAR* IPlatformFile::GetPhysicalTypeName()
{
	return TEXT("PhysicalFile");
}

bool IPlatformFile::IterateDirectoryRecursively(const TCHAR* Directory, FDirectoryVisitor& Visitor)
{
	class FRecurse : public FDirectoryVisitor
	{
	public:
		IPlatformFile&		PlatformFile;
		FDirectoryVisitor&	Visitor;
		FRecurse(IPlatformFile&	InPlatformFile, FDirectoryVisitor& InVisitor)
			: PlatformFile(InPlatformFile)
			, Visitor(InVisitor)
		{
		}
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
		{
			bool Result = Visitor.Visit(FilenameOrDirectory, bIsDirectory);
			if (Result && bIsDirectory)
			{
				Result = PlatformFile.IterateDirectory(FilenameOrDirectory, *this);
			}
			return Result;
		}
	};
	FRecurse Recurse(*this, Visitor);
	return IterateDirectory(Directory, Recurse);
}

bool IPlatformFile::DeleteDirectoryRecursively(const TCHAR* Directory)
{
	class FRecurse : public FDirectoryVisitor
	{
	public:
		IPlatformFile&		PlatformFile;
		FRecurse(IPlatformFile&	InPlatformFile)
			: PlatformFile(InPlatformFile)
		{
		}
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
		{
			if (bIsDirectory)
			{
				PlatformFile.IterateDirectory(FilenameOrDirectory, *this);
				PlatformFile.DeleteDirectory(FilenameOrDirectory);
			}
			else
			{
				PlatformFile.SetReadOnly(FilenameOrDirectory, false);
				PlatformFile.DeleteFile(FilenameOrDirectory);
			}
			return true; // continue searching
		}
	};
	FRecurse Recurse(*this);
	Recurse.Visit(Directory, true);
	return !DirectoryExists(Directory);
}


bool IPlatformFile::CopyFile(const TCHAR* To, const TCHAR* From)
{
	const int64 MaxBufferSize = 1024*1024;

	TAutoPtr<IFileHandle> FromFile(OpenRead(From));
	if (!FromFile.IsValid())
	{
		return false;
	}
	TAutoPtr<IFileHandle> ToFile(OpenWrite(To));
	if (!ToFile.IsValid())
	{
		return false;
	}
	int64 Size = FromFile->Size();
	if (Size < 1)
	{
		check(Size == 0);
		return true;
	}
	int64 AllocSize = FMath::Min<int64>(MaxBufferSize, Size);
	check(AllocSize);
	uint8* Buffer = (uint8*)FMemory::Malloc(int32(AllocSize));
	check(Buffer);
	while (Size)
	{
		int64 ThisSize = FMath::Min<int64>(AllocSize, Size);
		FromFile->Read(Buffer, ThisSize);
		ToFile->Write(Buffer, ThisSize);
		Size -= ThisSize;
		check(Size >= 0);
	}
	FMemory::Free(Buffer);
	return true;
}

bool IPlatformFile::CopyDirectoryTree(const TCHAR* DestinationDirectory, const TCHAR* Source, bool bOverwriteAllExisting)
{
	check(DestinationDirectory);
	check(Source);

	// TODO: can be implemented using existing functions, all bits seem to be there.
	return false;
}

FString IPlatformFile::ConvertToAbsolutePathForExternalAppForRead( const TCHAR* Filename )
{
	return FPaths::ConvertRelativePathToFull(Filename);
}

FString IPlatformFile::ConvertToAbsolutePathForExternalAppForWrite( const TCHAR* Filename )
{
	return FPaths::ConvertRelativePathToFull(Filename);
}

bool IPlatformFile::CreateDirectoryTree(const TCHAR* Directory)
{
	FString LocalFilename(Directory);
	FPaths::NormalizeDirectoryName(LocalFilename);
	const TCHAR* LocalPath = *LocalFilename;
	int32 CreateCount = 0;
	for (TCHAR Full[MAX_UNREAL_FILENAME_LENGTH]=TEXT( "" ), *Ptr=Full; ; *Ptr++=*LocalPath++ )
	{
		if (((*LocalPath) == TEXT('/')) || (*LocalPath== 0))
		{
			*Ptr = 0;
			if ((Ptr != Full) && !FPaths::IsDrive( Full ))
			{
				if (!CreateDirectory(Full) && !DirectoryExists(Full))
				{
					break;
				}
				CreateCount++;
			}
		}
		if (*LocalPath == 0)
		{
			break;
		}
	}
	return DirectoryExists(*LocalFilename);
}

bool IPhysicalPlatformFile::Initialize(IPlatformFile* Inner, const TCHAR* CmdLine)
{
	// Physical platform file should never wrap anything.
	check(Inner == NULL);
	return true;
}