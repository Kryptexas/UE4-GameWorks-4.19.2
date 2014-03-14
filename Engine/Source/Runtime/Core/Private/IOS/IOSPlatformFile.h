// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class IFileHandle;
class FApplePlatformFile;

/** 
 * iOS file handle implementation
**/
class CORE_API FFileHandleIOS : public IFileHandle
{
	enum {READWRITE_SIZE = 1024 * 1024};
	int32 FileHandle;

	FORCEINLINE bool IsValid()
	{
		return FileHandle != -1;
	}

public:
	FFileHandleIOS(int32 InFileHandle = -1);
	virtual ~FFileHandleIOS();

	virtual int64 Tell() OVERRIDE;
	virtual bool Seek(int64 NewPosition) OVERRIDE;
	virtual bool SeekFromEnd(int64 NewPositionRelativeToEnd = 0) OVERRIDE;
	virtual bool Read(uint8* Destination, int64 BytesToRead) OVERRIDE;
	virtual bool Write(const uint8* Source, int64 BytesToWrite) OVERRIDE;
};

/**
 * iOS File I/O implementation
**/
class CORE_API FIOSPlatformFile : public FApplePlatformFile
{
protected:
	virtual FString NormalizeFilename(const TCHAR* Filename);
	virtual FString NormalizeDirectory(const TCHAR* Directory);

public:
	//virtual bool Initialize(IPlatformFile* Inner, const TCHAR* CommandLineParam) OVERRIDE;

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
	FString ConvertToIOSPath(const FString& Filename, bool bForWrite);

	FString ReadDirectory;
	FString WriteDirectory;
};