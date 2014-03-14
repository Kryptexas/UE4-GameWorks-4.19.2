// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	ApplePlatformFile.h: Apple platform File functions
==============================================================================================*/

#pragma once

/**
 * Mac File I/O implementation
**/
class CORE_API FApplePlatformFile : public IPhysicalPlatformFile
{
protected:
	virtual FString NormalizeFilename(const TCHAR* Filename);
	virtual FString NormalizeDirectory(const TCHAR* Directory);
public:
	virtual bool FileExists(const TCHAR* Filename) OVERRIDE;
	virtual int64 FileSize(const TCHAR* Filename) OVERRIDE;
	virtual bool DeleteFile(const TCHAR* Filename) OVERRIDE;
	virtual bool IsReadOnly(const TCHAR* Filename) OVERRIDE;
	virtual bool MoveFile(const TCHAR* To, const TCHAR* From) OVERRIDE;
	virtual bool SetReadOnly(const TCHAR* Filename, bool bNewReadOnlyValue) OVERRIDE;


	virtual FDateTime GetTimeStamp(const TCHAR* Filename) OVERRIDE;

	virtual void SetTimeStamp(const TCHAR* Filename, const FDateTime DateTime) OVERRIDE;

	virtual FDateTime GetAccessTimeStamp(const TCHAR* Filename) OVERRIDE;

	virtual IFileHandle* OpenRead(const TCHAR* Filename) OVERRIDE;
	virtual IFileHandle* OpenWrite(const TCHAR* Filename, bool bAppend = false, bool bAllowRead = false) OVERRIDE;
	virtual bool DirectoryExists(const TCHAR* Directory) OVERRIDE;
	virtual bool CreateDirectory(const TCHAR* Directory) OVERRIDE;
	virtual bool DeleteDirectory(const TCHAR* Directory) OVERRIDE;
	bool IterateDirectory(const TCHAR* Directory, FDirectoryVisitor& Visitor);
private:
	int32 Stat(const TCHAR* Filename, struct stat* OutFileInfo, ANSICHAR* OutFixedPath = NULL, SIZE_T FixedPathSize = 0);
};
