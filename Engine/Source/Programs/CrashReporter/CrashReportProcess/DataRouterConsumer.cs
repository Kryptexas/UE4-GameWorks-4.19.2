using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ProtoBuf;
using Tools.CrashReporter.CrashReportCommon;

namespace Tools.CrashReporter.CrashReportProcess
{
	class DataRouterConsumer
	{
		[ProtoContract]
		public class ProtocolBufferRecord
		{
			[ProtoMember(1)]
			public string RecordId { get; set; }
			[ProtoMember(2)]
			public string AppId { get; set; }
			[ProtoMember(3)]
			public string AppVersion { get; set; }
			[ProtoMember(4)]
			public string Environment { get; set; }
			[ProtoMember(5)]
			public string UserId { get; set; }
			[ProtoMember(6)]
			public string UserAgent { get; set; }
			[ProtoMember(7)]
			public string UploadType { get; set; }
			[ProtoMember(8)]
			public string Meta { get; set; }
			[ProtoMember(9)]
			public string IpAddress { get; set; }
			[ProtoMember(10)]
			public string FilePath { get; set; }
			[ProtoMember(11)]
			public string ReceivedTimestamp { get; set; }
			[ProtoMember(12)]
			public byte[] Payload { get; set; }
			[ProtoMember(13)]
			public string Geo { get; set; }
			[ProtoMember(14)]
			public string SessionId { get; set; }

			public bool HasPayload { get { return Payload != null && Payload.Length > 0; } }
		}

		public string LastError { get; private set; }

		public bool TryParse(Stream InStream, out ProtocolBufferRecord Message)
		{
			try
			{
				Int64 Size = GetVarInt(InStream);
				Int64 MessageStart = InStream.Position;

				Message = Serializer.Deserialize<ProtocolBufferRecord>(InStream);

				Int64 MessageEnd = InStream.Position;
				if (MessageEnd - MessageStart != Size)
				{
					throw new CrashReporterException(string.Format("DataRouterConsumer size mismatch in TryParse() - Embedded Size {0}  Actual Size {1}", Size, MessageEnd - MessageStart));
				}

				return Message.HasPayload;
			}
			catch (Exception ex)
			{
				LastError = ex.ToString();
				Message = new ProtocolBufferRecord();
				return false;
			}
		}

		private static Int64 GetVarInt(Stream InStream)
		{
			Int64 VarInt = 0;
			for (int ByteIndex = 0, LeftShift = 0; ByteIndex < 9; ByteIndex++, LeftShift += 7)
			{
				int Byte = InStream.ReadByte();
				if (Byte < 0)
				{
					throw new CrashReporterException("DataRouterConsumer encountered unexpected end of stream in GetVarInt()");
				}
				VarInt |= (Int64) (Byte & 0x7F) << LeftShift;
				if ((Byte & 0x80) == 0)
				{
					break;
				}
			}
			return VarInt;
		}
	}
}
