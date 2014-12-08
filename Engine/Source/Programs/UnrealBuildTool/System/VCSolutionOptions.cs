// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;
using System.Xml.XPath;
using System.Xml.Linq;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Diagnostics;

using STATSTG = System.Runtime.InteropServices.ComTypes.STATSTG;
using FILETIME = System.Runtime.InteropServices.ComTypes.FILETIME;

namespace UnrealBuildTool
{
	public class VCBinarySetting
	{
		public enum ValueType
		{
			Bool = 3,
			String = 8,
		}

		public string Name;
		public ValueType Type;
		public bool BoolValue;
		public string StringValue;

		public VCBinarySetting()
		{
		}

		public VCBinarySetting(string InName, bool InValue)
		{
			Name = InName;
			Type = ValueType.Bool;
			BoolValue = InValue;
		}

		public VCBinarySetting(string InName, string InValue)
		{
			Name = InName;
			Type = ValueType.String;
			StringValue = InValue;
		}

		public static VCBinarySetting Read(BinaryReader Reader)
		{
			VCBinarySetting Setting = new VCBinarySetting();

			// Read the key
			Setting.Name = new string(Reader.ReadChars(Reader.ReadInt32())).TrimEnd('\0');

			// Read an equals sign
			if (Reader.ReadChar() != '=')
			{
				throw new InvalidDataException("Expected equals symbol");
			}

			// Read the value
			Setting.Type = (ValueType)Reader.ReadUInt16();
			if (Setting.Type == ValueType.Bool)
			{
				Setting.BoolValue = (Reader.ReadUInt32() != 0);
			}
			else if (Setting.Type == ValueType.String)
			{
				Setting.StringValue = new string(Reader.ReadChars(Reader.ReadInt32()));
			}
			else
			{
				throw new InvalidDataException("Unknown value type");
			}

			// Read a semicolon
			if (Reader.ReadChar() != ';')
			{
				throw new InvalidDataException("Expected semicolon");
			}

			return Setting;
		}

		public void Write(BinaryWriter Writer)
		{
			// Write the name
			Writer.Write(Name.Length + 1);
			Writer.Write(Name.ToCharArray());
			Writer.Write('\0');

			// Write an equals sign
			Writer.Write('=');

			// Write the value type
			Writer.Write((ushort)Type);
			if (Type == ValueType.Bool)
			{
				Writer.Write(BoolValue ? (uint)0xffff0000 : 0);
			}
			else if (Type == ValueType.String)
			{
				Writer.Write(StringValue.Length);
				Writer.Write(StringValue.ToCharArray());
			}
			else
			{
				throw new InvalidDataException("Unknown value type");
			}

			// Write a semicolon
			Writer.Write(';');
		}

		public override string ToString()
		{
			switch (Type)
			{
				case ValueType.Bool:
					return String.Format("{0} = {1}", Name, BoolValue);
				case ValueType.String:
					return String.Format("{0} = {1}", Name, StringValue);
			}
			return String.Format("{0} = ???", Name);
		}
	}

	class VCSolutionOptions
	{
		[Flags]
		enum STGC : int
		{
			Default = 0,
			Overwrite = 1,
			OnlyIfCurrent = 2,
			DangerouslyCommitMerelyToDiskCache = 4,
			Consolidate = 8,
		}

		[Flags]
		public enum STGM : int
		{
			Direct = 0x00000000,
			Transacted = 0x00010000,
			Simple = 0x08000000,
			Read = 0x00000000,
			Write = 0x00000001,
			ReadWrite = 0x00000002,
			ShareDenyNone = 0x00000040,
			ShareDenyRead = 0x00000030,
			ShareDenyWrite = 0x00000020,
			ShareExclusive = 0x00000010,
			Priority = 0x00040000,
			DeleteOnRelease = 0x04000000,
			NoScratch = 0x00100000,
			Create = 0x00001000,
			Convert = 0x00020000,
			FailIfThere = 0x00000000,
			NoSnapshot = 0x00200000,
			DirectSWMR = 0x00400000,
		}

		[ComImport, Guid("0000000c-0000-0000-C000-000000000046"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
		interface IOleStream
		{
			void Read([Out, MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 1)] byte[] pv, uint cb, out uint pcbRead);
			void Write([MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 1)] byte[] pv, uint cb, out uint pcbWritten);
			void Seek(long dlibMove, uint dwOrigin, out long plibNewPosition);
			void SetSize(long libNewSize);
			void CopyTo(IOleStream pstm, long cb, out long pcbRead, out long pcbWritten);
			void Commit(STGC grfCommitFlags);
			void Revert();
			void LockRegion(long libOffset, long cb, uint dwLockType);
			void UnlockRegion(long libOffset, long cb, uint dwLockType);
			void Stat(out STATSTG pstatstg, uint grfStatFlag);
			void Clone(out IOleStream ppstm);
		}

		[ComImport, Guid("0000000b-0000-0000-C000-000000000046"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
		interface IOleStorage
		{
			void CreateStream(string pwcsName, STGM grfMode, uint reserved1, uint reserved2, out IOleStream ppstm);
			void OpenStream(string pwcsName, IntPtr reserved1, STGM grfMode, uint reserved2, out IOleStream ppstm);
			void CreateStorage(string pwcsName, STGM grfMode, uint reserved1, uint reserved2, out IOleStorage ppstg);
			void OpenStorage(string pwcsName, IOleStorage pstgPriority, uint grfMode, IntPtr snbExclude, uint reserved, out IOleStorage ppstg);
			void CopyTo(uint ciidExclude, Guid rgiidExclude, IntPtr snbExclude, IOleStorage pstgDest);
			void MoveElementTo(string pwcsName, IOleStorage pstgDest, string pwcsNewName, uint grfFlags);
			void Commit(STGC grfCommitFlags);
			void Revert();
			void EnumElements(uint reserved1, IntPtr reserved2, uint reserved3, out IOleEnumSTATSTG ppenum);
			void DestroyElement(string pwcsName);
			void RenameElement(string pwcsOldName, string pwcsNewName);
			void SetElementTimes(string pwcsName, FILETIME pctime, FILETIME patime, FILETIME pmtime);
			void SetClass(Guid clsid);
			void SetStateBits(uint grfStateBits, uint grfMask);
			void Stat(out STATSTG pstatstg, uint grfStatFlag);
		}

		[ComImport, Guid("0000000d-0000-0000-C000-000000000046"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
		interface IOleEnumSTATSTG
		{
			void Next(uint celt, [Out, MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 0)] STATSTG[] rgelt, out uint pceltFetched);
			void RemoteNext(uint celt, [Out, MarshalAs(UnmanagedType.LPArray)] STATSTG[] rgelt, out uint pceltFetched);
			void Skip(out uint celt);
			void Reset();
			void Clone(out IOleEnumSTATSTG ppenum);
		}

		[DllImport("ole32.dll")]
		static extern int StgCreateDocfile([MarshalAs(UnmanagedType.LPWStr)]string pwcsName, STGM grfMode, uint reserved, out IOleStorage ppstgOpen);

		[DllImport("ole32.dll")]
		static extern int StgOpenStorage([MarshalAs(UnmanagedType.LPWStr)] string pwcsName, IOleStorage pstgPriority, STGM grfMode, IntPtr snbExclude, uint reserved, out IOleStorage ppstgOpen);

		public List<VCBinarySetting> SolutionConfiguration = new List<VCBinarySetting>();

		public VCSolutionOptions()
		{
		}

		public static VCSolutionOptions Read(string InputFileName)
		{
			IOleStorage Storage = null;
			StgOpenStorage(InputFileName, null, STGM.Direct | STGM.Read | STGM.ShareExclusive, IntPtr.Zero, 0, out Storage);
			try
			{
				IOleEnumSTATSTG Enumerator = null;
				Storage.EnumElements(0, IntPtr.Zero, 0, out Enumerator);
				try
				{
					uint Fetched;
					STATSTG[] Stats = new STATSTG[200];
					Enumerator.Next((uint)Stats.Length, Stats, out Fetched);

					VCSolutionOptions Options = new VCSolutionOptions();
					foreach (STATSTG Stat in Stats)
					{
						if (Stat.pwcsName == "SolutionConfiguration")
						{
							IOleStream OleStream;
							Storage.OpenStream(Stat.pwcsName, IntPtr.Zero, STGM.Read | STGM.ShareExclusive, 0, out OleStream);
							try
							{
								uint SizeRead;
								byte[] Buffer = new byte[Stat.cbSize];
								OleStream.Read(Buffer, (uint)Stat.cbSize, out SizeRead);

								using (MemoryStream InputStream = new MemoryStream(Buffer, false))
								{
									BinaryReader Reader = new BinaryReader(InputStream, Encoding.Unicode);
									while (InputStream.Position < InputStream.Length)
									{
										Options.SolutionConfiguration.Add(VCBinarySetting.Read(Reader));
									}
								}
							}
							finally
							{
								Marshal.ReleaseComObject(OleStream);
							}
						}
					}
					return Options;
				}
				finally
				{
					Marshal.ReleaseComObject(Enumerator);
				}
			}
			finally
			{
				Marshal.ReleaseComObject(Storage);
			}
		}

		public void Write(string OutputFileName)
		{
			IOleStorage OleStorage = null;
			StgCreateDocfile(OutputFileName, STGM.Direct | STGM.Create | STGM.Write | STGM.ShareExclusive, 0, out OleStorage);
			try
			{
				if (SolutionConfiguration.Count > 0)
				{
					using (MemoryStream OutputStream = new MemoryStream())
					{
						// Write all the settings to the memory stream
						BinaryWriter Writer = new BinaryWriter(OutputStream, Encoding.Unicode);
						foreach (VCBinarySetting Setting in SolutionConfiguration)
						{
							Setting.Write(Writer);
						}

						// Create a named stream in the compound document and write the binary data out
						IOleStream OleStream = null;
						OleStorage.CreateStream("SolutionConfiguration", STGM.Write | STGM.ShareExclusive, 0, 0, out OleStream);
						try
						{
							uint Written;
							OleStream.Write(OutputStream.GetBuffer(), (uint)OutputStream.Length, out Written);
							OleStream.Commit(STGC.Overwrite);
						}
						finally
						{
							Marshal.ReleaseComObject(OleStream);
						}
					}
				}
			}
			finally
			{
				Marshal.ReleaseComObject(OleStorage);
			}
		}
	}
}
