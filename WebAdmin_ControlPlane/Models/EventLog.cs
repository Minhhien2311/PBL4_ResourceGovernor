using System;
using System.ComponentModel.DataAnnotations;

namespace WebAdmin_ControlPlane.Models
{
    public class EventLog
    {
        [Key]
        public int Id { get; set; }

        [Required]
        public string NodeType { get; set; } = ""; // "OSAgent" hoặc "NetworkProxy"

        [Required]
        public string EventType { get; set; } = ""; // "PolicyApplied", "ProcessKilled", "Error", ...

        public string? Message { get; set; }

        public DateTime Timestamp { get; set; }
    }
}