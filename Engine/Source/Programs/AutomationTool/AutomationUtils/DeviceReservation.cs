// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Xml.Serialization;
using System.Net;
using UnrealBuildTool;
using System.Text;

namespace AutomationTool.DeviceReservation
{
	/// <summary>
	/// Co-operatively reserves remote devices for build automation.
	/// 
	/// The constructor blocks until the specified type and number of devices are available.
	/// The reservation is automatically renewed for the lifetime of this object, and released
	/// when the object is disposed or garbage collected.
	/// </summary>
	public sealed class DeviceReservationAutoRenew : IDisposable
	{
		private Uri ReservationBaseUri;

		private static readonly TimeSpan ReserveTime = TimeSpan.FromMinutes(10);
		private static readonly TimeSpan RenewTime = TimeSpan.FromMinutes(5);

		private Thread RenewThread;
		private AutoResetEvent WaitEvent = new AutoResetEvent(false);

		private Reservation ActiveReservation;
		private List<Device> ReservedDevices;

		public IReadOnlyList<Device> Devices
		{
			get
			{
				// Return a copy so our list can't be modified.
				return ReservedDevices.Select(d => d.Clone()).ToList();
			}
		}

		/// <summary>
		/// Creates a device reservation for the specified type and number of devices.
		/// Blocks until the devices are available.
		/// </summary>
		/// <param name="InWorkingDirectory">Working directory which contains the devices.xml and reservations.xml files. Usually a network share.</param>
		/// <param name="InDeviceTypes">An array of device types to reserve, one for each device requested. These must match the device types listed in devices.xml.</param>
		public DeviceReservationAutoRenew(string InReservationBaseUri, params string[] InDeviceTypes)
		{
			ReservationBaseUri = new Uri(InReservationBaseUri);

			// Make a device reservation for all the required device types.
			// This blocks until the reservation is successful.
			ActiveReservation = Reservation.Create(ReservationBaseUri, InDeviceTypes, ReserveTime);

			// Resolve the device IPs
			ReservedDevices = new List<Device>();
			foreach (var DeviceName in ActiveReservation.DeviceNames)
			{
				ReservedDevices.Add(Device.Get(ReservationBaseUri, DeviceName));
			}

			RenewThread = new Thread(DoAutoRenew);
			RenewThread.Start();
		}

		private void DoAutoRenew()
		{
			while (!WaitEvent.WaitOne(RenewTime))
			{
				ActiveReservation.Renew(ReservationBaseUri, ReserveTime);
			}

			ActiveReservation.Delete(ReservationBaseUri);
		}

		private void StopAutoRenew()
		{
			if (RenewThread != null)
			{
				WaitEvent.Set();
				RenewThread.Join();
				RenewThread = null;
			}
		}

		~DeviceReservationAutoRenew()
		{
			StopAutoRenew();
		}

		public void Dispose()
		{
			StopAutoRenew();
			GC.SuppressFinalize(this);
		}
	}

	public static class Utils
	{
		public static Uri AppendPath(this Uri BaseUri, string NewPath)
		{
			var Builder = new UriBuilder(BaseUri);
			Builder.Path += NewPath;
			return Builder.Uri;
		}

		public static T InvokeAPI<T>(Uri UriToRequest, string Method, object ObjectToSerialize)
		{
			fastJSON.JSON.Instance.RegisterCustomType(typeof(TimeSpan), (o) => o.ToString(), (s) => TimeSpan.Parse(s));

			var Request = (HttpWebRequest)WebRequest.Create(UriToRequest);
			Request.UseDefaultCredentials = true;
			Request.Method = Method;

			if (ObjectToSerialize != null)
			{
				Request.ContentType = "application/json";

				using (var RequestStream = Request.GetRequestStream())
				{
					var JsonString = fastJSON.JSON.Instance.ToJSON(ObjectToSerialize, new fastJSON.JSONParameters() { UseExtensions = false });
					var Writer = new StreamWriter(RequestStream);
					Writer.Write(JsonString);
					Writer.Flush();
					RequestStream.Flush();
				}
			}

			using (var Response = (HttpWebResponse)Request.GetResponse())
			using (var ResponseStream = Response.GetResponseStream())
			{
				string JsonString = Encoding.UTF8.GetString(ResponseStream.ReadIntoMemoryStream().ToArray());
				return fastJSON.JSON.Instance.ToObject<T>(JsonString);
			}
		}

		public static void InvokeAPI(Uri UriToRequest, string Method)
		{
			var Request = (HttpWebRequest)WebRequest.Create(UriToRequest);
			Request.UseDefaultCredentials = true;
			Request.Method = Method;

			using (var Response = (HttpWebResponse)Request.GetResponse())
			using (var ResponseStream = Response.GetResponseStream())
			{
				// Nothing to do here. Error codes throw exceptions.
			}
		}
	}

	public sealed class Reservation
	{
		public string[] DeviceNames { get; set; }
		public string HostName { get; set; }
		public DateTime StartDateTime { get; set; }
		public TimeSpan Duration { get; set; }
		public Guid Guid { get; set; }

		private void Copy(Reservation Other)
		{
			DeviceNames = Other.DeviceNames.ToArray();
			HostName = Other.HostName;
			StartDateTime = Other.StartDateTime;
			Duration = Other.Duration;
			Guid = Other.Guid;
		}

		private sealed class CreateReservationData
		{
			public string[] DeviceTypes;
			public string Hostname;
			public TimeSpan Duration;
		}

		public static Reservation Create(Uri BaseUri, string[] DeviceTypes, TimeSpan Duration)
		{
			bool bFirst = true;
			while (true)
			{
				if (!bFirst)
				{
					TimeSpan RetryTime = TimeSpan.FromMinutes(1);
					Console.WriteLine("Retrying in {0} seconds ...", RetryTime.TotalSeconds);
					Thread.Sleep(RetryTime);
				}
				bFirst = false;

				Console.WriteLine("Requesting device reservation...");

				Exception UnknownException;

				try
				{
					return Utils.InvokeAPI<Reservation>(BaseUri.AppendPath("api/v1/reservations"), "POST", new CreateReservationData()
					{
						DeviceTypes = DeviceTypes,
						Hostname = Environment.MachineName,
						Duration = Duration
					});
				}
				catch (WebException WebEx)
				{
					if ((WebEx.Response as HttpWebResponse).StatusCode == HttpStatusCode.Conflict)
					{
						Console.WriteLine("No devices currently available.");
						continue; // Retry 
					}

					UnknownException = WebEx;
				}
				catch (Exception Ex)
				{
					UnknownException = Ex;
				}

				Console.WriteLine("Warning: Device reservation unsuccessful: {0}", UnknownException.Message);
			}
		}

		public void Renew(Uri BaseUri, TimeSpan NewDuration)
		{
			try
			{
				var NewReservation = Utils.InvokeAPI<Reservation>(BaseUri.AppendPath("api/v1/reservations/" + Guid.ToString()), "PUT", NewDuration);
				Copy(NewReservation);
			}
			catch (Exception ex)
			{
				throw new AutomationException(ex, "Failed to renew device reservation.");
			}
		}
		
		public void Delete(Uri BaseUri)
		{
			try
			{
				Utils.InvokeAPI(BaseUri.AppendPath("api/v1/reservations/" + Guid.ToString()), "DELETE");
				Console.WriteLine("Successfully deleted device reservation \"{0}\".", Guid);
			}
			catch (Exception ex)
			{
				Console.WriteLine("Failed to delete device reservation: {0}", ex.Message);
			}
		}	
	}

	public sealed class Device
	{
		public string Name { get; set; }
		public string Type { get; set; }
		public string IPOrHostName { get; set; }
		public TimeSpan AvailableStartTime { get; set; }
		public TimeSpan AvailableEndTime { get; set; }
		public bool Enabled { get; set; }

		public Device Clone() { return (Device)MemberwiseClone(); }

		public static Device Get(Uri BaseUri, string DeviceName)
		{
			return Utils.InvokeAPI<Device>(BaseUri.AppendPath("api/v1/devices/" + DeviceName), "GET", null);
		}
	}
}
