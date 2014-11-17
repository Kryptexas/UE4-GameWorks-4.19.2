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

	virtual int64 Tell() override;
	virtual bool Seek(int64 NewPosition) override;
	virtual bool SeekFromEnd(int64 NewPositionRelativeToEnd = 0) override;
	virtual bool Read(uint8* Destination, int64 BytesToRead) override;
	virtual bool Write(const uint8* Source, int64 BytesToWrite) override;
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
	//virtual bool Initialize(IPlatformFile* Inner, const TCHAR* CommandLineParam) override;

	virtual bool FileExists(const TCHAR* Filename) override;
	virtual int64 FileSize(const TCHAR* Filename) override;
	virtual bool DeleteFile(const TCHAR* Filename) override;
	virtual bool IsReadOnly(const TCHAR* Filename) override;
	virtual bool MoveFile(const TCHAR* To, const TCHAR* From) override;
	virtual bool SetReadOnly(const TCHAR* Filename, bool bNewReadOnlyValue) override;

	virtual FDateTime GetTimeStamp(const TCHAR* Filename) override;
	virtual void SetTimeStamp(const TCHAR* Filename, const FDateTime DateTime) override;
	virtual FDateTime GetAccessTimeStamp(const TCHAR* Filename) override;

	virtual IFileHandle* OpenRead(const TCHAR* Filename) override;
	virtual IFileHandle* OpenWrite(const TCHAR* Filename, bool bAppend = false, bool bAllowRead = false) override;

	virtual bool DirectoryExists(const TCHAR* Directory) override;
	virtual bool CreateDirectory(const TCHAR* Directory) override;
	virtual bool DeleteDirectory(const TCHAR* Directory) override;
	bool IterateDirectory(const TCHAR* Directory, FDirectoryVisitor& Visitor);

private:
	FString ConvertToIOSPath(const FString& Filename, bool bForWrite);

	FString ReadDirectory;
	FString WriteDirectory;
};