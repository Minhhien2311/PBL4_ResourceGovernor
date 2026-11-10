using System;
using System.ComponentModel.DataAnnotations;

namespace WebAdmin_ControlPlane.Models
{
    public class NodeStatus
    {
        [Key]
        public string NodeId { get; set; } = string.Empty;
        public string Status { get; set; } = string.Empty;
        public double CPUUsagePercent { get; set; }
        public long TotalRAM { get; set; }
        public long UsedRAM { get; set; }
        public DateTime LastHeartbeat { get; set; } = DateTime.Now;
    }

    public class ProcessInfo
    {
        public string Name { get; set; } = string.Empty;
        public int Pid { get; set; }
        public string User { get; set; } = string.Empty;
        public double RamMB { get; set; }
    }
}