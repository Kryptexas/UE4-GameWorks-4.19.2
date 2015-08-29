using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace DoxygenInputFilter
{
	class Program
	{
		static void Main(string[] args)
		{
			StreamReader reader = File.OpenText(args[0]);
			string line;
			while ((line = reader.ReadLine()) != null)
			{
				int posIgnore = DetectFirstSubstring(line, "//~", "/*~");
				int posHarvest = DetectFirstSubstring(line, "//", "/*");
				if (posHarvest < posIgnore)
				{
					// This line does not contain an ignore code, or has a comment section starter before it. It should be harvested.
					int posDoxygenHarvest = DetectFirstSubstring(line, "///", "/**");
					if (posHarvest < posDoxygenHarvest)
					{
						// This line would normally be ignored by Doxygen, so modify it into something Doxygen will harvest.
						Console.Out.WriteLine(ReplaceFirstSubstring(ReplaceFirstSubstring(line, "/*", "/**"), "//", "///"));
						continue;
					}
				}
				Console.Out.WriteLine(line);
			}
		}

		static int DetectFirstSubstring(string sourceLine, string substring1, string substring2)
		{
			int len = sourceLine.Length;
			int pos1 = sourceLine.IndexOf(substring1);
			int pos2 = sourceLine.IndexOf(substring2);
			if (pos1 < 0)
			{
				pos1 = len;
			}
			if (pos2 < 0)
			{
				pos2 = len;
			}
			return Math.Min(pos1, pos2);
		}

		static string ReplaceFirstSubstring(string sourceString, string searchSubstring, string replacementSubstring)
		{
			int pos = sourceString.IndexOf(searchSubstring);
			if (pos < 0)
			{
				// Not found
				return sourceString;
			}
			// Manually cut the first instance of searchString out and replace it.
			return (sourceString.Substring(0, pos) + replacementSubstring + sourceString.Substring(pos + searchSubstring.Length));
		}
	}
}
