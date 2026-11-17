using System;
using System.ComponentModel.DataAnnotations;

namespace WebAdmin_ControlPlane.Models
{
    public class NodeStatus
    {
        [Key]
        public int Id { get; set; }

        [Required]
        public string NodeType { get; set; } = ""; // "OSAgent" hoặc "NetworkProxy"

        [Required]
        public string Status { get; set; } = ""; // "Running", "Stopped", "Error"

        public double CPUUsagePercent { get; set; }
        public double TotalRAM { get; set; }
        public double UsedRAM { get; set; }
        public int ProcessCount { get; set; }

        // Dành cho Network Proxy
        public double BandwidthInUse { get; set; }
        public int ActiveConnections { get; set; }

        public DateTime LastHeartbeat { get; set; }
    }
}