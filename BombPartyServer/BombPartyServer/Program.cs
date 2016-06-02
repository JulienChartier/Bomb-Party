using Constellation;
using Constellation.Package;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;

namespace BombPartyServer
{
    public class Program : PackageBase
    {
        private static string ALL_MAC_ADDRESSES = "00:00:00:00:00";

        static void Main(string[] args)
        {
            PackageHost.Start<Program>(args);
        }

        public override void OnStart()
        {
            PackageHost.WriteInfo("PARTY HARD!!");
        }

        [MessageCallback]
        public void ActivateBomb(string macAddress)
        {
            PackageHost.PushStateObject("ActivateBomb", new {MacAddress = macAddress});
        }

        [MessageCallback]
        public void PauseBomb(string macAddress)
        {
            PackageHost.PushStateObject("PauseBomb", new { MacAddress = macAddress });
        }

        [MessageCallback]
        public void ResumeBomb(string macAddress)
        {
            PackageHost.PushStateObject("ResumeBomb", new { MacAddress = macAddress });
        }

        [MessageCallback]
        public void ResetBomb(string macAddress)
        {
            PackageHost.PushStateObject("ResetBomb", new { MacAddress = macAddress });
        }
    }
}
