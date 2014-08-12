// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ImageWrapperPrivatePCH.h"

#if WITH_UNREALEXR
/**
 * EXR image wrapper class.
 */

FExrImageWrapper::FExrImageWrapper()
	: FImageWrapperBase()
{
}

template <typename sourcetype> class FSourceImageRaw
{
public:
	FSourceImageRaw(const TArray<uint8>& SourceImageBitmapIN, uint32 ChannelsIN, uint32 WidthIN, uint32 HeightIN)
		: SourceImageBitmap(SourceImageBitmapIN),
			Width( WidthIN ),
			Height(HeightIN),
			Channels(ChannelsIN)
	{		
		check(SourceImageBitmap.Num() == Channels*Width*Height*sizeof(sourcetype));
	}

	uint32 GetXStride() const	{return sizeof(sourcetype) * Channels;}
	uint32 GetYStride() const	{return GetXStride() * Width;}

	const sourcetype* GetHorzLine(uint32 y, uint32 Channel)
	{
		return &SourceImageBitmap[(sizeof(sourcetype) * Channel) + (GetYStride() * y)];
	}

private:
	const TArray<uint8>& SourceImageBitmap;
	uint32 Width;
	uint32 Height;
	uint32 Channels;
};


class FMemFileOut : public Imf::OStream
{
public:
	//-------------------------------------------------------
	// A constructor that opens the file with the given name.
	// The destructor will close the file.
	//-------------------------------------------------------

	FMemFileOut(const char fileName[]) :
		Imf::OStream(fileName),
		Pos(0)
	{
	}

	virtual void write(const char c[/*n*/], int n)
	{
		for (int32 i = 0; i < n; ++i)
		{
			Data.Insert((uint8)c[i], Pos++);
		}
	}


	//---------------------------------------------------------
	// Get the current writing position, in bytes from the
	// beginning of the file.  If the next call to write() will
	// start writing at the beginning of the file, tellp()
	// returns 0.
	//---------------------------------------------------------

	virtual Imf::Int64 tellp()
	{
		return Pos;
	}


	//-------------------------------------------
	// Set the current writing position.
	// After calling seekp(i), tellp() returns i.
	//-------------------------------------------

	virtual void seekp(Imf::Int64 pos)
	{
		Pos = pos;
	}


	int64 Pos;
	TArray<uint8> Data;
};


namespace
{
	/////////////////////////////////////////
	// 8 bit per channel source
	float ToLinear(uint8 LDRValue)
	{
		return pow((float)(LDRValue) / 255.f, 2.2f);
	}

	void ExtractAndConvertChannel(const uint8*Src, uint32 SrcChannels, uint32 x, uint32 y, float* ChannelOUT)
	{
		for (uint32 i = 0; i < x*y; i++, Src += SrcChannels)
		{
			ChannelOUT[i] = ToLinear(*Src);
		}
	}

	void ExtractAndConvertChannel(const uint8*Src, uint32 SrcChannels, uint32 x, uint32 y, FFloat16* ChannelOUT)
	{
		for (uint32 i = 0; i < x*y; i++, Src += SrcChannels)
		{
			ChannelOUT[i] = FFloat16(ToLinear(*Src));
		}
	}

	/////////////////////////////////////////
	// 16 bit per channel source
	void ExtractAndConvertChannel(const FFloat16*Src, uint32 SrcChannels, uint32 x, uint32 y, float* ChannelOUT)
	{
		for (uint32 i = 0; i < x*y; i++, Src += SrcChannels)
		{
			ChannelOUT[i] = Src->GetFloat();
		}
	}

	void ExtractAndConvertChannel(const FFloat16*Src, uint32 SrcChannels, uint32 x, uint32 y, FFloat16* ChannelOUT)
	{
		for (uint32 i = 0; i < x*y; i++, Src += SrcChannels)
		{
			ChannelOUT[i] = *Src;
		}
	}

	/////////////////////////////////////////
	// 32 bit per channel source
	void ExtractAndConvertChannel(const float* Src, uint32 SrcChannels, uint32 x, uint32 y, float* ChannelOUT)
	{
		for (uint32 i = 0; i < x*y; i++, Src += SrcChannels)
		{
			ChannelOUT[i] = *Src;
		}
	}

	void ExtractAndConvertChannel(const float* Src, uint32 SrcChannels, uint32 x, uint32 y, FFloat16* ChannelOUT)
	{
		for (uint32 i = 0; i < x*y; i++, Src += SrcChannels)
		{
			ChannelOUT[i] = *Src;
		}
	}

	/////////////////////////////////////////
	int32 GetNumChannelsFromFormat(ERGBFormat::Type Format)
	{
		switch (Format)
		{
			case ERGBFormat::RGBA:
			case ERGBFormat::BGRA:
				return 4;
			case ERGBFormat::Gray:
				return 1;
		}
		checkNoEntry();
		return 1;
	}
}

const char* FExrImageWrapper::GetRawChannelName(int ChannelIndex) const
{
	const int32 MaxChannels = 4;
	static const char* RGBAChannelNames[] = { "R", "G", "B", "A" };
	static const char* BGRAChannelNames[] = { "B", "G", "R", "A" };
	static const char* GrayChannelNames[] = { "G" };
	check(ChannelIndex < MaxChannels);

	const char** ChannelNames = BGRAChannelNames;

	switch (RawFormat)
	{
		case ERGBFormat::RGBA:
		{
			ChannelNames = RGBAChannelNames;
		}
		break;
		case ERGBFormat::BGRA:
		{
			ChannelNames = BGRAChannelNames;
		}
		break;
		case ERGBFormat::Gray:
		{
			check(ChannelIndex < ARRAY_COUNT(GrayChannelNames));
			ChannelNames = GrayChannelNames;
		}
		break;
		default:
			checkNoEntry();
	}
	return ChannelNames[ChannelIndex];
}

template <Imf::PixelType OutputFormat, typename sourcetype>
void FExrImageWrapper::WriteFrameBufferChannel(Imf::FrameBuffer& ImfFrameBuffer, const char* ChannelName, const sourcetype* SrcData, TArray<uint8>& ChannelBuffer)
{
	const int32 OutputPixelSize = ((OutputFormat == Imf::HALF) ? 2 : 4);
	ChannelBuffer.AddUninitialized(Width*Height*OutputPixelSize);
	uint32 SrcChannels = GetNumChannelsFromFormat(RawFormat);
	switch (OutputFormat)
	{
		case Imf::HALF:
		{
			ExtractAndConvertChannel(SrcData, SrcChannels, Width, Height, (FFloat16*)&ChannelBuffer[0]);
		}
		break;
		case Imf::FLOAT:
		{
			ExtractAndConvertChannel(SrcData, SrcChannels, Width, Height, (float*)&ChannelBuffer[0]);
		}
		break;
	}

	Imf::Slice FrameChannel = Imf::Slice(OutputFormat, (char*)&ChannelBuffer[0], OutputPixelSize, Width*OutputPixelSize);
	ImfFrameBuffer.insert(ChannelName, FrameChannel);
}

template <Imf::PixelType OutputFormat, typename sourcetype>
void FExrImageWrapper::CompressRaw(const sourcetype* SrcData, bool bIgnoreAlpha)
{
	uint32 NumWriteComponents = GetNumChannelsFromFormat(RawFormat);
	if (bIgnoreAlpha && NumWriteComponents == 4)
	{
		NumWriteComponents = 3;
	}

	Imf::Header Header(Width, Height);

	for (uint32 Channel = 0; Channel < NumWriteComponents; Channel++)
	{
		Header.channels().insert(GetRawChannelName(Channel), Imf::Channel(OutputFormat));
	}

	FMemFileOut MemFile("");
	Imf::FrameBuffer ImfFrameBuffer;

	TArray<uint8> ChannelOutputBuffers[4];

	for (uint32 Channel = 0; Channel < NumWriteComponents; Channel++)
	{
		WriteFrameBufferChannel<OutputFormat>(ImfFrameBuffer, GetRawChannelName(Channel), SrcData + Channel, ChannelOutputBuffers[Channel]);
	}

	Imf::OutputFile ImfFile(MemFile, Header);
	ImfFile.setFrameBuffer(ImfFrameBuffer);
	ImfFile.writePixels(Height);

	CompressedData = MemFile.Data;
}

void FExrImageWrapper::Compress( int32 Quality )
{
	check(RawData.Num() != 0);
	check(Width > 0);
	check(Height > 0);
	check(RawBitDepth == 8 || RawBitDepth == 16 || RawBitDepth == 32);

	const int32 MaxComponents = 4;

	switch (RawBitDepth)
	{
		case 8:
			CompressRaw<Imf::HALF>(&RawData[0], false);
		break;
		case 16:
			CompressRaw<Imf::HALF>((const FFloat16*)&RawData[0], false);
		break;
		case 32:
			CompressRaw<Imf::FLOAT>((const float*)&RawData[0], false);
		break;
		default:
			checkNoEntry();
	}
}

void FExrImageWrapper::Uncompress( const ERGBFormat::Type InFormat, const int32 InBitDepth )
{
	checkf(false, TEXT("EXR decompression not supported"));
}

bool FExrImageWrapper::SetCompressed( const void* InCompressedData, int32 InCompressedSize )
{
	bool bResult = FImageWrapperBase::SetCompressed( InCompressedData, InCompressedSize );

	return bResult;
}

bool FExrImageWrapper::GetRaw( const ERGBFormat::Type InFormat, int32 InBitDepth, const TArray<uint8>*& OutRawData )
{
	LastError.Empty();
	Uncompress(InFormat, InBitDepth);

	if (LastError.IsEmpty())
	{
		OutRawData = &GetRawData();
	}

	return LastError.IsEmpty();
}

#endif // WITH_UNREALEXR