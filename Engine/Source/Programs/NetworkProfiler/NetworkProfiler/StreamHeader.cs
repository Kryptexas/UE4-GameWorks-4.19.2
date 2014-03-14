// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


using System;
using System.IO;

namespace NetworkProfiler
{
	/**
	 * Header written by capture tool
	 */
	public class StreamHeader
	{
		/** Magic number at beginning of data file.					*/
		public const UInt32 ExpectedMagic = 0x1DBF348A;

		/** Magic to ensure we're opening the right file.			*/
		public UInt32 Magic;
		/** Version number to detect version mismatches.			*/
		public UInt32 Version;

		/** Offset in file for name table.							*/
		public UInt32 NameTableOffset;
		/** Number of name table entries.							*/
		public UInt32 NameTableEntries;

		/** Tag, set via -networkprofiler=TAG						*/
		public string Tag;
		/** Game name, e.g. Example									*/
		public string GameName;
		/** URL used to open/ browse to the map.					*/
		public string URL;

		/**
		 * Reads the header information from the passed in stream and returns it. It also returns
		 * a BinaryReader that is endian-appropriate for the data stream. 
		 *
		 * @param	ParserStream		source stream of data to read from
		 * @param	BinaryStream [out]	binary reader used for reading from stream
		 * @return	serialized header
		 */
		public static StreamHeader ReadHeader( Stream ParserStream, out BinaryReader BinaryStream  )
		{
			// Create a binary stream for data, we might toss this later for are an endian swapping one.
			BinaryStream = new BinaryReader(ParserStream,System.Text.Encoding.ASCII);

			// Serialize header.
			StreamHeader Header = new StreamHeader(BinaryStream);

			// Determine whether read file has magic header. If no, try again byteswapped.
			if(Header.Magic != StreamHeader.ExpectedMagic)
			{
				// Seek back to beginning of stream before we retry.
				ParserStream.Seek(0,SeekOrigin.Begin);

				// Use big endian reader. It transparently endian swaps data on read.
				BinaryStream = new BinaryReaderBigEndian(ParserStream);
				
				// Serialize header a second time.
				Header = new StreamHeader(BinaryStream);
			}

			// At this point we should have a valid header. If no, throw an exception.
			if( Header.Magic != StreamHeader.ExpectedMagic )
			{
				throw new InvalidDataException();
			}

			return Header;
		}

		/**
		 * Constructor, serializing header from passed in stream.
		 * 
		 * @param	BinaryStream	Stream to serialize header from.
		 */
		public StreamHeader(BinaryReader BinaryStream)
		{
			// Serialize the file format magic first.
			Magic = BinaryStream.ReadUInt32();

			// Stop serializing data if magic number doesn't match. Most likely endian issue.
			if( Magic == ExpectedMagic )
			{
				// Version info for backward compatible serialization.
				Version = BinaryStream.ReadUInt32();

				// Name table offset in file and number of entries.
				NameTableOffset = BinaryStream.ReadUInt32();
				NameTableEntries = BinaryStream.ReadUInt32();

				// Serialize various fixed size strings.
				Tag = SerializeAnsiString( BinaryStream );
				GameName = SerializeAnsiString( BinaryStream );
				URL = SerializeAnsiString( BinaryStream );
			}
		}

		/**
		 * We serialize a fixed size string into the header so its size doesn't change later on. This means
		 * we need to handle loading slightly differently to trim the extra padding.
		 * 
		 * @param	BinaryStream	stream to read from
		 * @return	string being read
		 */
		private string SerializeAnsiString( BinaryReader BinaryStream )
		{
			UInt32 SerializedLength = BinaryStream.ReadUInt32();
			string ResultString = new string(BinaryStream.ReadChars((int)SerializedLength)); 
			// We serialize a fixed size string. Trim the null characters that make it in by converting char[] to string.
			int RealLength = 0;
			while( ( RealLength < ResultString.Length ) && ( ResultString[RealLength++] != '\0' ) ) 
			{
			}
			return ResultString.Remove(RealLength-1);
		}
	}
}
