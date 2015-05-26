#include "UnrealAudioPrivate.h"
#include "UnrealAudioSoundFile.h"
#include "UnrealAudioModule.h"
#include "ModuleManager.h"
#include "UnrealAudioSoundFile.h"
#include "UnrealAudioSoundFileInternal.h"

#if ENABLE_UNREAL_AUDIO


#define SOUND_IMPORT_CHECK(Result)							\
	if (Result != ESoundFileError::NONE)					\
	{														\
		((ISoundFileInternal*)SoundFile.Get())->SetError(Result);	\
		return;												\
	}


namespace UAudio
{
	/**
	FAsyncSoundFileImportTask
	Task which processes a sound file import request on a background thread.
	*/
	class FAsyncSoundFileImportTask
	{
	public:
		FAsyncSoundFileImportTask(FUnrealAudioModule* InAudioModule, FSoundFileImportSettings InImportSettings, TSharedPtr<ISoundFile> InSoundFile);
		~FAsyncSoundFileImportTask();

		void DoWork();
		bool CanAbandon() { return true; }
		void Abandon() {}

		FORCEINLINE TStatId GetStatId() const
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncSoundFileImportTask, STATGROUP_ThreadPoolAsyncTasks);
		}

		FUnrealAudioModule* AudioModule;
		FSoundFileImportSettings ImportSettings;
		TSharedPtr<ISoundFile> SoundFile;
		friend class FAsyncTask<FAsyncSoundFileImportTask>;
	};

	FAsyncSoundFileImportTask::FAsyncSoundFileImportTask(FUnrealAudioModule* InAudioModule, FSoundFileImportSettings InImportSettings, TSharedPtr<ISoundFile> InSoundFile)
		: AudioModule(InAudioModule)
		, ImportSettings(InImportSettings)
		, SoundFile(InSoundFile)
	{
		// make sure the audio module knows we're doing background task work so it doesn't shutdown while we're working here
		AudioModule->IncrementBackgroundTaskCount();
	}

	FAsyncSoundFileImportTask::~FAsyncSoundFileImportTask()
	{
		// Let the audio module know we're done doing this background task
		AudioModule->DecrementBackgroundTaskCount();
	}

	void FAsyncSoundFileImportTask::DoWork()
	{

		// Create a new sound file object
		TScopedPointer<FSoundFile> SoundFileInput = TScopedPointer<FSoundFile>(new FSoundFile());
		
		// Open the file
		ESoundFileError::Type Error = SoundFileInput->OpenFileForReading(ImportSettings.SoundFilePath);
		SOUND_IMPORT_CHECK(Error);

		TSharedPtr<ISoundFileData> SoundFileData;
		Error = SoundFileInput->GetSoundFileData(SoundFileData);
		SOUND_IMPORT_CHECK(Error);

		// Get the input file's description
		const FSoundFileDescription& InputDescription = SoundFileData->GetDescription();

		// Get the input file's channel map
		const TArray<ESoundFileChannelMap::Type>& InputChannelMap = SoundFileData->GetChannelMap();

		// Build a description for the new file
		FSoundFileDescription NewSoundFileDescription;
		NewSoundFileDescription.NumChannels = InputDescription.NumChannels;
		NewSoundFileDescription.NumFrames = InputDescription.NumFrames;
		NewSoundFileDescription.FormatFlags = ImportSettings.Format;
		NewSoundFileDescription.SampleRate = ImportSettings.SampleRate;
		NewSoundFileDescription.NumSections = 0;
		NewSoundFileDescription.bIsSeekable = 1;

		// Open it as an empty file for reading and writing
		ISoundFileInternal* SoundFileInternal = (ISoundFileInternal*)SoundFile.Get();
		Error = SoundFileInternal->OpenEmptyFileForImport(NewSoundFileDescription, InputChannelMap);
		SOUND_IMPORT_CHECK(Error);

		// Set the original description on the new sound file
		Error = SoundFileInternal->SetImportFileInfo(InputDescription, ImportSettings.SoundFilePath);
		SOUND_IMPORT_CHECK(Error);

		// Set the encoding quality (will only do anything if import target is Ogg-Vorbis)
		Error = SoundFileInternal->SetEncodingQuality(ImportSettings.EncodingQuality);
		SOUND_IMPORT_CHECK(Error);

		// Set the state of the sound file to be loading
		Error = SoundFileInternal->BeginImport();
		SOUND_IMPORT_CHECK(Error);

		// Create a buffer to do the processing 
		SoundFileCount ProcessBufferSamples = 1024 * NewSoundFileDescription.NumChannels;
		TArray<double> ProcessBuffer;
		ProcessBuffer.Init(0.0, ProcessBufferSamples);

		// Find the max value if we've been told to do peak normalization on import
		double MaxValue = 0.0;
		SoundFileCount SamplesRead = 0;
		bool bPerformPeakNormalization = ImportSettings.bPerformPeakNormalization;
		if (bPerformPeakNormalization)
		{
			Error = SoundFileInput->ReadSamples(ProcessBuffer.GetData(), ProcessBufferSamples, SamplesRead);
			SOUND_IMPORT_CHECK(Error);

			while (SamplesRead)
			{
				for (SoundFileCount Sample = 0; Sample < SamplesRead; ++Sample)
				{
					if (ProcessBuffer[Sample] > FMath::Abs(MaxValue))
					{
						MaxValue = ProcessBuffer[Sample];
					}
				}

				Error = SoundFileInput->ReadSamples(ProcessBuffer.GetData(), ProcessBufferSamples, SamplesRead);
				SOUND_IMPORT_CHECK(Error);
			}

			// If this happens, it means we have a totally silent file
			if (MaxValue == 0.0)
			{
				bPerformPeakNormalization = false;
			}

			// Seek the file back to the beginning
			SoundFileCount OutOffset;
			SoundFileInput->SeekFrames(0, ESoundFileSeekMode::FROM_START, OutOffset);
		}

		// Now perform the encoding to the target file
		Error = SoundFileInput->ReadSamples(ProcessBuffer.GetData(), ProcessBufferSamples, SamplesRead);
		SOUND_IMPORT_CHECK(Error);

		while (SamplesRead)
		{
			SOUND_IMPORT_CHECK(SoundFileInput->GetError());

			// Normalize the samples if we're told to
			if (bPerformPeakNormalization)
			{
				for (int32 Sample = 0; Sample < SamplesRead; ++Sample)
				{
					ProcessBuffer[Sample] /= MaxValue;
				}
			}

			SoundFileCount SamplesWritten;
			Error = SoundFileInternal->WriteSamples((const double*)ProcessBuffer.GetData(), SamplesRead, SamplesWritten);
			SOUND_IMPORT_CHECK(Error);

			Error = SoundFileInput->ReadSamples(ProcessBuffer.GetData(), ProcessBufferSamples, SamplesRead);
			SOUND_IMPORT_CHECK(Error);
		}

		SoundFileInput->ReleaseSoundFileHandle();
		SoundFileInternal->ReleaseSoundFileHandle();

		// We're done doing the encoding
		Error = SoundFileInternal->EndImport();
		SOUND_IMPORT_CHECK(Error);
	}

	TSharedPtr<ISoundFile> FUnrealAudioModule::ImportSound(const FSoundFileImportSettings& ImportSettings)
	{
		TSharedPtr<ISoundFile> ImportedSoundFile = TSharedPtr<ISoundFile>(new FSoundFile());

		// Make an auto-deleting task to perform the import work in the background task thread pool
		FAutoDeleteAsyncTask<FAsyncSoundFileImportTask>* Task = new FAutoDeleteAsyncTask<FAsyncSoundFileImportTask>(this, ImportSettings, ImportedSoundFile);
		Task->StartBackgroundTask();

		return ImportedSoundFile;
	}

}

#endif // #if ENABLE_UNREAL_AUDIO

