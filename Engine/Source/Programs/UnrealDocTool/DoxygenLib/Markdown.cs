// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace DoxygenLib
{
	public static class Markdown
	{
		const bool bDocToolCanParseExternalLinks = true;

		public delegate string ResolveLinkDelegate(string Id);

		/** Parses markdown text out of an XML node 
		 * 
		 * @param child			xml node we want to parse into Markdown 
		 * @param Indent		indenting prefix for each newline of text in the converted markdown
		 * @param ResolveLink	delegate to call to resolve doxygen id's into link paths. may be null.
		 * @return				a string containing the text from the child node 
		 */
		public static string ParseXml(XmlNode child, string Indent, ResolveLinkDelegate ResolveLink)
		{
			string output = "";

			XmlNode parent = child.ParentNode;
			if (child.HasChildNodes)
			{
				switch (child.Name)
				{
					case "para":
						if (parent.Name != "listitem" && parent.Name != "parameterdescription" && parent.Name != "simplesect")
						{
							output += Environment.NewLine;
							output += ParseChildren(child, Indent, ResolveLink);
						}
						else if (parent.Name == "listitem")
						{
							output += Environment.NewLine;
							if (child == parent.FirstChild)
							{
								output += Indent + "* " + ParseChildren(child, Indent, ResolveLink);
							}
							else
							{
								output += Environment.NewLine + Indent + "\t" + ParseChildren(child, Indent, ResolveLink);
							}
						}
						else if (parent.Name == "parameterdescription" || parent.Name == "simplesect")
						{
							output += ParseChildren(child, Indent, ResolveLink);
						}
						break;
					case "ulink":
						if (bDocToolCanParseExternalLinks)
						{
							string LinkText = ParseChildren(child, Indent, ResolveLink);
							string LinkUrl = child.Attributes.GetNamedItem("url").InnerText;

							const string WebPrefix = "docs.unrealengine.com/latest/INT/";
							int WebPrefixIdx = LinkUrl.IndexOf(WebPrefix);
							if(WebPrefixIdx != -1 && LinkText.Contains("docs.unrealengine.com"))
							{
								LinkText = "";
								LinkUrl = LinkUrl.Substring(WebPrefixIdx + WebPrefix.Length);
								if(LinkUrl.EndsWith(".html"))
								{
									LinkUrl = LinkUrl.Substring(0, LinkUrl.LastIndexOf('/') + 1);
								}
							}

							output += Indent + String.Format("[{0}]({1})", LinkText, LinkUrl);
						}
						else
						{
							output += Indent + EscapeText(child.Attributes.GetNamedItem("url").InnerText);
						}
						break;
					case "ref":
						XmlAttribute RefAttribute = child.Attributes["refid"];
						if (RefAttribute != null && ResolveLink != null)
						{
							string LinkPath = ResolveLink(RefAttribute.Value);
							if(LinkPath != null)
							{
								output += String.Format("[{0}]({1})", ParseChildren(child, Indent, ResolveLink), LinkPath);
								break;
							}
						}
						output += ParseChildren(child, Indent, ResolveLink);
						break;
					case "bold":
						output += Indent + String.Format("**{0}**", ParseChildren(child, Indent, ResolveLink));
						break;
					case "emphasis":
						output += Indent + String.Format("_{0}_", ParseChildren(child, Indent, ResolveLink));
						break;
					case "computeroutput":
						output += Indent + String.Format("`{0}`", ParseChildren(child, Indent, ResolveLink));
						break;
					case "itemizedlist":
						if (parent.ParentNode.Name == "listitem")
						{
							output += ParseChildren(child, Indent + "\t", ResolveLink);
						}
						else
						{
							output += Environment.NewLine + ParseChildren(child, Indent, ResolveLink) + Environment.NewLine + Environment.NewLine;
						}
						break;
					case "parameterlist":
						break;
					case "parameteritem":
						break;
					case "parameternamelist":
						break;
					case "simplesect":
						XmlAttribute KindAttribute = child.Attributes["kind"];
						if (KindAttribute != null && KindAttribute.Value == "see")
						{
							output += "  \n  \nFor more information, see " + ParseChildren(child, Indent + "\t", ResolveLink) + ".";
						}
						break;
					case "xrefsect":
						break;
					default:
						output += ParseChildren(child, Indent, ResolveLink);
						break;
				}
			}
			else
			{
				output += Indent + EscapeText(child.InnerText);
			}

			return output;
		}

		/** Class ParseXML for all child nodes of the given child
		 * 
		 * @param child xml node we want to parse all children into Markdown 
		 * @return a string containing the text from the child node 
		 */
		static String ParseChildren(XmlNode child, string Indent, ResolveLinkDelegate ResolveLink)
		{
			String output = "";

			foreach (XmlNode childNode in child.ChildNodes)
			{
				if (output.Length > 0 && output[output.Length - 1] != ' ') output += " ";
				output += ParseXml(childNode, Indent, ResolveLink);
			}

			return output;
		}

		/** 
		 * Parses Markdown from a Doxygen code node
		 * 
		 * @param Node			Xml node we want to parse into Markdown 
		 * @param ResolveLink	Delegate to call to resolve doxygen id's into link paths. may be null.
		 * @return				Converted text
		 */
		public static string ParseXmlCodeLine(XmlNode Node, ResolveLinkDelegate ResolveLink)
		{
			StringBuilder Builder = new StringBuilder();
			ParseXmlCodeLine(Node, Builder, ResolveLink);
			return Builder.ToString();
		}

		/** 
		 * Parses Markdown from a Doxygen code node
		 * 
		 * @param Node			Xml node we want to parse into Markdown 
		 * @param Output		StringBuilder to receive the output
		 * @param ResolveLink	Delegate to call to resolve doxygen id's into link paths. may be null.
		 */
		static void ParseXmlCodeLine(XmlNode Node, StringBuilder Output, ResolveLinkDelegate ResolveLink)
		{
			switch(Node.Name)
			{
				case "sp":
					Output.Append(' ');
					return;
				case "ref":
					XmlAttribute RefAttribute = Node.Attributes["refid"];

					string LinkPath = (RefAttribute != null && ResolveLink != null)? ResolveLink(RefAttribute.Value) : null;
					if(LinkPath != null)
					{
						Output.Append("[");
					}

					ParseXmlCodeLineChildren(Node, Output, ResolveLink);

					if(LinkPath != null)
					{
						Output.AppendFormat("]({0})", LinkPath);
					}
					break;
				case "#text":
					Output.Append(EscapeText(Node.InnerText));
					break;
				default:
					ParseXmlCodeLineChildren(Node, Output, ResolveLink);
					break;
			}
		}

		/** 
		 * Parses Markdown from the children of a Doxygen code node
		 * 
		 * @param Node			Xml node we want to parse into Markdown 
		 * @param Output		StringBuilder to receive the output
		 * @param ResolveLink	Delegate to call to resolve doxygen id's into link paths. may be null.
		 */
		static void ParseXmlCodeLineChildren(XmlNode Node, StringBuilder Output, ResolveLinkDelegate ResolveLink)
		{
			foreach(XmlNode ChildNode in Node.ChildNodes)
			{
				ParseXmlCodeLine(ChildNode, Output, ResolveLink);
			}
		}

		/** 
		 * Escape a string to be valid markdown.
		 * 
		 * @param Text			String to escape
		 * @return				Escaped string
		 */
		public static string EscapeText(string Text)
		{
			StringBuilder Result = new StringBuilder();
			for (int Idx = 0; Idx < Text.Length; Idx++)
			{
				if (Text[Idx] == '&')
				{
					Result.Append("&amp;");
				}
				else if (Text[Idx] == '<')
				{
					Result.Append("&lt;");
				}
				else if (Text[Idx] == '>')
				{
					Result.Append("&gt;");
				}
				else if (Text[Idx] == '"')
				{
					Result.Append("&quot;");
				}
				else if (Text[Idx] == '\t')
				{
					Result.Append("    ");
				}
				else if(Char.IsLetterOrDigit(Text[Idx]) || Text[Idx] == '_' || Text[Idx] == ' ' || Text[Idx] == ',' || Text[Idx] == '.')
				{
					Result.Append(Text[Idx]);
				}
				else
				{
					Result.AppendFormat("&#{0:00};", (int)Text[Idx]);
				}
			}
			return Result.ToString();
		}

		/** 
		 * Truncates a line to a given width, respecting embedded links.
         * 
		 * @param Text			Text to truncate
         * @param MaxLength     Maximum number of characters in the output
         * @param Suffix        Suffix to append if the string is truncated
         * @return				Truncated string
		 */
        public static string Truncate(string Text, int MaxLength, string Suffix)
        {
            StringBuilder Output = new StringBuilder();
            
            int RemainingCharacters = MaxLength;
            for (int Idx = 0; Idx < Text.Length; )
            {
                int NextIdx = TruncateEscapedCharacter(Text, Idx, ref RemainingCharacters, Output);
                if(NextIdx == -1)
                {
                    NextIdx = TruncateLink(Text, Idx, ref RemainingCharacters, Output);
                    if(NextIdx == -1)
                    {
                        NextIdx = Idx + 1;
                        if(RemainingCharacters > 0)
                        {
                            Output.Append(Text[Idx]);
                        }
                        RemainingCharacters--;
                    }
                }
                Idx = NextIdx;
            }
            if(RemainingCharacters < 0)
            {
                Output.Append(Suffix);
            }

            return Output.ToString();
        }

        private static int TruncateEscapedCharacter(string Text, int Idx, ref int RemainingCharacters, StringBuilder Output)
        {
            // Check it starts with an ampersand
            if (Text[Idx] == '&')
            {
                // Skip past any sequence of letters
                int NewIdx = Idx + 1;
                while (NewIdx < Text.Length && Char.IsLetter(Text[NewIdx]))
                {
                    NewIdx++;
                }

                // Check the whole thing is valid
                if (NewIdx != Idx + 1 && NewIdx < Text.Length && Text[NewIdx] == ';')
                {
                    // Append the markdown
                    if (RemainingCharacters > 0)
                    {
                        Output.Append(Text.Substring(Idx, NewIdx + 1 - Idx));
                    }

                    // Move to the next character
                    RemainingCharacters--;
                    return NewIdx + 1;
                }
            }
            return -1;
        }

        private static int TruncateLink(string Text, int Idx, ref int RemainingCharacters, StringBuilder Output)
        {
            // Check if starts with a bracket
            if(Text[Idx] == '[')
            {
                // Find the end of the description
                int LabelStart = Idx + 1;
                int LabelEnd = Text.IndexOf(']', LabelStart);
                if(LabelEnd != -1 && LabelEnd + 1 < Text.Length && Text[LabelEnd + 1] == '(')
                {
                    // Find the end of the link
                    int LinkStart = LabelEnd + 2;
                    int LinkEnd = Text.IndexOf(')', LinkStart);
                    if(LinkEnd != -1)
                    {
                        if(RemainingCharacters > 0)
                        {
                            Output.AppendFormat("[{0}]({1})", Text.Substring(LabelStart, Math.Min(LabelEnd - LabelStart, RemainingCharacters)), Text.Substring(LinkStart, LinkEnd - LinkStart));
                        }
                        RemainingCharacters -= LabelEnd - LabelStart;
                        return LinkEnd + 1;
                    }
                }
            }
            return -1;
        }
	}
}
