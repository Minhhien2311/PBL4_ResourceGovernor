using System.Collections.Generic;

namespace WebAdmin_ControlPlane.Models
{
    public class HeartbeatPayload
    {
        public string NodeType { get; set; } = string.Empty;
        public string NodeId { get; set; } = string.Empty;
        public string Status { get; set; } = string.Empty;
        public double CPUUsagePercent { get; set; }
        public long TotalRAM { get; set; }
        public long UsedRAM { get; set; }
        public int ProcessCount { get; set; }

        public List<ProcessInfo> Processes { get; set; } = new List<ProcessInfo>();
    }
}