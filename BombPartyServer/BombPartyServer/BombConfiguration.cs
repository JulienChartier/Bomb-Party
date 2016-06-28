using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace BombPartyServer
{
    class BombConfiguration
    {
        public long TimeInMs { get; set; }
        public string Type { get; set; }
        public List<Object> AllInstructions { get; set; }
        public string Puzzle { get; set; }


        public BombConfiguration(long timeInMs, string type, List<Object> allInstructions, string puzzle)
        {
            this.TimeInMs = timeInMs;
            this.Type = type;
            this.AllInstructions = allInstructions;
            this.Puzzle = puzzle;
        }
    }
}
