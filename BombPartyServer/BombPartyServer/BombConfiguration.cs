using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace BombPartyServer
{
    class BombConfiguration
    {
        public long TimeInMs { get; set; }
        public List<Object> AllInstructions { get; set; }

        public BombConfiguration(long timeInMs, List<Object> allInstructions)
        {
            this.TimeInMs = timeInMs;
            this.AllInstructions = allInstructions;
        }
    }
}
