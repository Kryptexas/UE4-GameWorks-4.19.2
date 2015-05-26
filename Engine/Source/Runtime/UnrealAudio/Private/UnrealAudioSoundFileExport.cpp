#include "UnrealAudioPrivate.h"
#include "UnrealAudioSoundFile.h"
#include "UnrealAudioModule.h"
#include "ModuleManager.h"
#include "UnrealAudioSoundFile.h"
#include "UnrealAudioSoundFileInternal.h"

#if ENABLE_UNREAL_AUDIO

#define SOUND_EXPORT_CHECK(Result)			\
	if (Result != ESoundFileError::NONE)	\
	{										\
		return;								\
	}

namespace UAudio
{
	/**
	FAsyncSoundFileExportTask
	Task which processes a sound file export request on a background thread.
	*/
	class FAsyncSoundFileExportTask
	{
	public:
		FAsyncSoundFileExportTask(FUnrealAudioModule* InAudioModule, TSharedPtr<ISoundFileData> InSoundFileData, const FString* InExportPath);
		~FAsyncSoundFileExportTask();

		void DoWork();
		bool CanAbandon() { return true; }
		void Abandon() {}

		FORCEINLINE TStatId GetStatId() const
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncSoundFileExportTask, STATGROUP_ThreadPoolAsyncTasks);
		}

		FUnrealAudioModule* AudioModule;
		TSharedPtr<ISoundFileData> SoundFileData;
		FString ExportPath;
		friend class FAsyncTask<FAsyncSoundFileExportTask>;
	};

	FAsyncSoundFileExportTask::FAsyncSoundFileExportTask(FUnrealAudioModule* InAudioModule, TSharedPtr<ISoundFileData> InSoundFileData, const FString* InExportPath)
		: AudioModule(InAudioModule)
		, SoundFileData(InSoundFileData)
		, ExportPath(*InExportPath)
	{
		// make sure the audio module knows we're doing background task work so it doesn't shutdown while we're working here
		AudioModule->IncrementBackgroundTaskCount();
	}

	FAsyncSoundFileExportTask::~FAsyncSoundFileExportTask()
	{
		// Let the audio module know we're done doing this background task
		AudioModule->DecrementBackgroundTaskCount();
	}

	void FAsyncSoundFileExportTask::DoWork()
	{
		// Create a new sound file object which we will pass our data to so we can read from it
		FSoundFile SoundFileInput;

		ESoundFileError::Type Error = SoundFileInput.Initialize(SoundFileData);
		SOUND_EXPORT_CHECK(Error);

		// Create a new sound file object which will be written to disk
		FSoundFile SoundFileOutput;

		FSoundFileDescription OriginalDescription = SoundFileData->GetOriginalDescription();
		const TArray<ESoundFileChannelMap::Type>& ChannelMap = SoundFileData->GetChannelMap();

		Error = SoundFileOutput.OpenEmptyFileForExport(ExportPath, OriginalDescription, ChannelMap);
		SOUND_EXPORT_CHECK(Error);

		// Create a buffer to do the processing 
		SoundFileCount ProcessBufferSamples = 1024 * OriginalDescription.NumChannels;
		TArray<double> ProcessBuffer;
		ProcessBuffer.Init(0.0, ProcessBufferSamples);

		// Now perform the encoding to the target file
		SoundFileCount SamplesRead = 0;
		Error = SoundFileInput.ReadSamples(ProcessBuffer.GetData(), ProcessBufferSamples, SamplesRead);
		SOUND_EXPORT_CHECK(Error);
		
		while (SamplesRead)
		{
			SOUND_EXPORT_CHECK(SoundFileInput.GetError());
			SoundFileCount SamplesWritten;

			Error = SoundFileOutput.WriteSamples(ProcessBuffer.GetData(), SamplesRead, SamplesWritten);
			SOUND_EXPORT_CHECK(Error);

			Error = SoundFileInput.ReadSamples(ProcessBuffer.GetData(), ProcessBufferSamples, SamplesRead);
			SOUND_EXPORT_CHECK(Error);
		}

		SoundFileInput.ReleaseSoundFileHandle();
		SoundFileOutput.ReleaseSoundFileHandle();
	}
	
	void FUnrealAudioModule::ExportSound(TSharedPtr<ISoundFileData> SoundFileData, const FString& ExportPath)
	{
		// Make an auto-deleting task to perform the import work in the background task thread pool
		FAutoDeleteAsyncTask<FAsyncSoundFileExportTask>* Task = new FAutoDeleteAsyncTask<FAsyncSoundFileExportTask>(this, SoundFileData, &ExportPath);
		Task->StartBackgroundTask();
	}

}

#endif // #if ENABLE_UNREAL_AUDIO
