// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DirectoryWatcherPrivatePCH.h"

FDirectoryWatchRequestLinux::FDirectoryWatchRequestLinux()
:	bRunning(false)
,	bEndWatchRequestInvoked(false)
{
    NotifyFilter = IN_CREATE | IN_MOVE | IN_MODIFY | IN_DELETE;
}

FDirectoryWatchRequestLinux::~FDirectoryWatchRequestLinux()
{
    Shutdown();
}

void FDirectoryWatchRequestLinux::Shutdown()
{
    free(WatchDescriptor);
    close(FileDescriptor);
}

bool FDirectoryWatchRequestLinux::Init(const FString& InDirectory)
{
	if (InDirectory.Len() == 0)
	{
		// Verify input
		return false;
	}

	if (bRunning)
	{
		Shutdown();
	}

	bEndWatchRequestInvoked = false;

	// Make sure the path is absolute
	const FString FullPath = FPaths::ConvertRelativePathToFull(InDirectory);

    FileDescriptor = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);

    if (FileDescriptor == -1)
    {
		UE_LOG(LogDirectoryWatcher, Error, TEXT("Failed to init inotify"));
    	return false;
    }

    IFileManager::Get().FindFilesRecursive(AllFiles, *FullPath, TEXT("*"), false, true);

    WatchDescriptor = (int*)FMemory::Malloc((AllFiles.Num()+1) * sizeof(int));
    if (WatchDescriptor == NULL) 
    {
		UE_LOG(LogDirectoryWatcher, Error, TEXT("Failed to allocate memory for WatchDescriptor"));
    	return false;
    }

    for (int32 FileIdx = 0; FileIdx < AllFiles.Num(); ++FileIdx)
    {
		const FString& FolderName = AllFiles[FileIdx];
		WatchDescriptor[FileIdx] = inotify_add_watch(FileDescriptor, TCHAR_TO_UTF8(*FolderName), NotifyFilter); //FileIdx+1
		if (WatchDescriptor[FileIdx] == -1) 
		{
			UE_LOG(LogDirectoryWatcher, Error, TEXT("inotify_add_watch cannot watch folder %s"), *FolderName);
			return false;
		}
    }

  	bRunning = true;

  	return true;
}

void FDirectoryWatchRequestLinux::AddDelegate( const IDirectoryWatcher::FDirectoryChanged& InDelegate )
{
	Delegates.Add(InDelegate);
}

bool FDirectoryWatchRequestLinux::RemoveDelegate( const IDirectoryWatcher::FDirectoryChanged& InDelegate )
{
	if ( Delegates.Contains(InDelegate) )
	{
		Delegates.Remove(InDelegate);
		return true;
	}

	return false;
}

bool FDirectoryWatchRequestLinux::HasDelegates() const
{
	return Delegates.Num() > 0;
}

void FDirectoryWatchRequestLinux::EndWatchRequest()
{
	bEndWatchRequestInvoked = true;
}


void FDirectoryWatchRequestLinux::ProcessPendingNotifications()
{
    ProcessChanges();

	// Trigger all listening delegates with the files that have changed
	if ( FileChanges.Num() > 0 )
	{
		for (int32 DelegateIdx = 0; DelegateIdx < Delegates.Num(); ++DelegateIdx)
		{
			Delegates[DelegateIdx].Execute(FileChanges);
		}

		FileChanges.Empty();
	}
}

void FDirectoryWatchRequestLinux::ProcessChanges()
{
    uint8 Buf[BUF_LEN] __attribute__ ((aligned(__alignof__(inotify_event))));

	for (;;)
	{
		ssize_t Len = read(FileDescriptor, Buf, sizeof(Buf));
		if (Len == -1 && errno != EAGAIN)
		{
			// TODO: handle errors
			return;
		}

		if (Len <= 0)
		{
			break;
		}
		else
		{
			for (const uint8 * Ptr = Buf; Ptr < Buf + Len;)
			{
				const inotify_event* Event = reinterpret_cast<const inotify_event *>(Ptr);
				FFileChangeData::EFileChangeAction Action = FFileChangeData::FCA_Unknown;

				// not sure if "else" is correct here, but we cannot OR actions that way
				if ((Event->mask & IN_CREATE) || (Event->mask & IN_MOVED_TO))
				{
					Action = FFileChangeData::FCA_Added;
				}
				else if (Event->mask & IN_MODIFY)
				{
					Action = FFileChangeData::FCA_Modified;
				}
				else if ((Event->mask & IN_DELETE) || (Event->mask & IN_MOVED_FROM))
				{
					Action = FFileChangeData::FCA_Removed;
				}

				Ptr += sizeof(inotify_event)+Event->len;

				// find the file
				FString Filename;
				for (int32 i = 0; i < AllFiles.Num(); ++i)
				{
					if (WatchDescriptor[i] == Event->wd)
					{
						Filename = AllFiles[i];
						break;
					}
				}

				new (FileChanges)FFileChangeData(Filename, Action);
			}
		}
	}
}
