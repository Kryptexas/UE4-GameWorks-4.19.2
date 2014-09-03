using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace APIDocTool
{
	class APIAction : APIPage
	{
		public readonly string Tooltip;
		private List<APIActionPin> Pins;

		public APIAction(APIPage InParent, string InName, Dictionary<string, object> ActionProperties)
			: base(InParent, InName)
		{
			object Value;
			
			if (ActionProperties.TryGetValue("Tooltip", out Value))
			{
				Tooltip = String.Concat(((string)Value).Split('\n').Select(Line => Line.Trim() + '\n'));
//				Tooltip = new string(Array.FindAll<char>(((string)Value).ToCharArray(), c => c != '\t'));
			}
			else
			{
				Tooltip = "";
			}

			if (ActionProperties.TryGetValue("Pins", out Value))
			{
				Pins = new List<APIActionPin>();

				foreach (var Pin in (Dictionary<string, object>)Value)
				{
					Pins.Add(new APIActionPin(this, APIActionPin.GetPinName((Dictionary<string, object>)Pin.Value), (Dictionary<string, object>)Pin.Value));
				}
			}
		}

		public override void WritePage(UdnManifest Manifest, string OutputPath)
		{
			using (UdnWriter Writer = new UdnWriter(OutputPath))
			{
				Writer.WritePageHeader(Name, PageCrumbs, Name);

				// Tooltip/description
				Writer.WriteLine(Tooltip);

				if (Pins != null && Pins.Count() > 0)
				{
					// Inputs
					Writer.EnterSection("inputs", "Inputs");
					Writer.WriteList(Pins.Where(x => x.bInputPin).Select(x => x.GetListItem()));
					Writer.LeaveSection();

					// Outputs
					Writer.EnterSection("outputs", "Outputs");
					Writer.WriteList(Pins.Where(x => !x.bInputPin).Select(x => x.GetListItem()));
					Writer.LeaveSection();
				}
			}
		}

		public override void GatherReferencedPages(List<APIPage> Pages)
		{
			/*if (Pins != null)
			{
				Pages.AddRange(Pins);
			}*/
		}

		public override void AddToManifest(UdnManifest Manifest)
		{
			Manifest.Add(Name, this);
			/*if (Pins != null)
			{
				foreach (APIPage Pin in Pins)
				{
					Pin.AddToManifest(Manifest);
				}
			}*/
		}

		public UdnListItem GetListItem()
		{
			return new UdnListItem(Name, Tooltip, LinkPath);
		}
	}
}
