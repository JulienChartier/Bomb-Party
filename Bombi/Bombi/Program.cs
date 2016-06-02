using Constellation;
using Constellation.Package;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Bombi
{
    public class Program : PackageBase
    {
        private string MacAddress { get; set; }
        private int Rssi { get; set; }
        private string Status { get; set; }
        private long TimerInMs { get; set; }
        private List<Object> Components { get; set; }
        private List<Object> Instructions { get; set; }

        private static string[] allStatus = { "Idle", "Activated", "Paused", "Exploded" };
        private static string ALL_MAC_ADDRESSES = "00:00:00:00:00";

        static void Main(string[] args)
        {
            PackageHost.Start<Program>(args);
        }

        public override void OnStart()
        {
            PackageHost.SubscribeStateObjects("*", "BombPartyServer", "*");

            PackageHost.StateObjectUpdated += PackageHost_StateObjectUpdated;

            Random random = new Random();

            this.MacAddress = macPart(random.Next(0, 16)) + ":" + macPart(random.Next(0, 16)) + ":" +
                macPart(random.Next(0, 16)) + ":" + macPart(random.Next(0, 16)) + ":" + macPart(random.Next(0, 16));
            this.Rssi = -random.Next(20, 90);
            this.Status = allStatus[random.Next(0, allStatus.Length)];
            this.TimerInMs = random.Next(100000, 500000);
            this.Components = new List<Object>();
            this.Instructions = new List<Object>();

            this.Components.Add(new
            {
                Name = "Right_Button",
                Type = "Button",
                State = "Up"
            });

            this.Components.Add(new
            {
                Name = "Left_Button",
                Type = "Button",
                State = "Up"
            });

            this.Components.Add(new
            {
                Name = "Up_Button",
                Type = "Button",
                State = "Up"
            });

            this.Instructions.Add(new
                {
                    Component = this.Components[1],
                    MinDuration = 0,
                    MaxDuration = 0
                });

            this.Instructions.Add(new
            {
                Component = this.Components[0],
                MinDuration = 1000,
                MaxDuration = 0
            });

            Task.Factory.StartNew(() =>
            {
                while (PackageHost.IsRunning)
                {
                    PackageHost.PushStateObject("BombInfo", new
                    {
                        MacAddress = this.MacAddress,
                        Rssi = this.Rssi,
                        Status = this.Status,
                        TimerInMs = this.TimerInMs,
                        Components = this.Components,
                        Instructions = this.Instructions
                    });

                    Thread.Sleep(1000);

                    if (this.Status == "Activated")
                    {
                        this.TimerInMs -= 1000;

                        if (this.TimerInMs <= 0)
                        {
                            this.Status = "Exploded";
                            this.TimerInMs = 0;
                        }
                    }
                }
            });
        }

        private string macPart(int i)
        {
            if (i >= 10)
            {
                return "0" + ((char) ('a' + i - 10));
            }

            return "0" + i;
        }

        void PackageHost_StateObjectUpdated(object sender, StateObjectUpdatedEventArgs e)
        {
            string destinationMacAddress = e.StateObject.DynamicValue.MacAddress;

            if ((destinationMacAddress != ALL_MAC_ADDRESSES) &&
                (destinationMacAddress != this.MacAddress))
            {
                return;
            }

            PackageHost.WriteInfo("StateObject received: {0}", e.StateObject.Name);

            switch (e.StateObject.Name)
            {
                case "ActivateBomb":
                    {
                        this.Status = "Activated";
                        break;
                    }
                case "PauseBomb":
                    {
                        this.Status = "Paused";
                        break;
                    }
                case "ResumeBomb":
                    {
                        this.Status = "Activated";
                        break;
                    }
                case "ResetBomb":
                    {
                        this.Status = "Idle";
                        break;
                    }
                default:
                    break;
            }
        }
    }
}
