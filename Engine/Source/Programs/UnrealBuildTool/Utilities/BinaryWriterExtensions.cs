using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnrealBuildTool
{
	static class BinaryWriterExtensions
	{
		public static void Write<T>(this BinaryWriter Writer, T[] Items, Action<BinaryWriter, T> WriteElement)
		{
			if(Items == null)
			{
				Writer.Write(-1);
			}
			else
			{
				Writer.Write(Items.Length);
				for(int Idx = 0; Idx < Items.Length; Idx++)
				{
					WriteElement(Writer, Items[Idx]);
				}
			}
		}

		public static void Write<T>(this BinaryWriter Writer, List<T> Items, Action<BinaryWriter, T> WriteElement)
		{
			if (Items == null)
			{
				Writer.Write(-1);
			}
			else
			{
				Writer.Write(Items.Count);
				for (int Idx = 0; Idx < Items.Count; Idx++)
				{
					WriteElement(Writer, Items[Idx]);
				}
			}
		}
	}
}
