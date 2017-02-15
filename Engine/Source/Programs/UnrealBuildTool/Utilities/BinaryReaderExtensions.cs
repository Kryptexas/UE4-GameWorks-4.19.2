using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnrealBuildTool
{
	static class BinaryReaderExtensions
	{
		public static T[] ReadArray<T>(this BinaryReader Reader, Func<BinaryReader, T> ReadElement)
		{
			int NumItems = Reader.ReadInt32();
			if(NumItems < 0)
			{
				return null;
			}

			T[] Items = new T[NumItems];
			for(int Idx = 0; Idx < NumItems; Idx++)
			{
				Items[Idx] = ReadElement(Reader);
			}
			return Items;
		}

		public static List<T> ReadList<T>(this BinaryReader Reader, Func<BinaryReader, T> ReadElement)
		{
			T[] Items = Reader.ReadArray<T>(ReadElement);
			return (Items == null)? null : new List<T>(Items);
		}
	}
}
