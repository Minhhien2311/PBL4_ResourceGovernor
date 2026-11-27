//namespace WebAdmin_ControlPlane.Models
//{
//    public class ControllerResponse
//    {
//        public string? Status { get; set; }
//        public string? Message { get; set; }
//        public string? JobId { get; set; }
//        public string? State { get; set; }
//        public List<ControllerServerInfo>? Servers { get; set; }
//    }

//    public class ControllerServerInfo
//    {
//        public string? NodeId { get; set; }
//        public string? Ip { get; set; }
//        public string? State { get; set; }
//        public double CPUUsagePercent { get; set; }
//        public long TotalRAM { get; set; }
//        public long UsedRAM { get; set; }
//        public int ProcessCount { get; set; }
//        public long LastSeenSeconds { get; set; }
//        public string? Note { get; set; }
//    }
//}

namespace WebAdmin_ControlPlane.Models
{
    public class ControllerResponse
    {
        public string? Status { get; set; }
        public string? Message { get; set; }
        public string? JobId { get; set; }
        public string? State { get; set; }
        public List<ControllerServerInfo>? Servers { get; set; }
    }

    public class ControllerServerInfo
    {
        public string? NodeId { get; set; }
        public string? Ip { get; set; }
        public string? State { get; set; }
        public double CPUUsagePercent { get; set; }
        public long TotalRAM { get; set; }
        public long UsedRAM { get; set; }
        public int ProcessCount { get; set; }
        public long LastSeenSeconds { get; set; }
        public string? Note { get; set; }
    }

    // Phan hoi cua lenh GetProcesses (Controller -> WebAdmin)
    public class ProcessListResponse
    {
        public string? Status { get; set; }
        public string? Message { get; set; }
        public string? NodeId { get; set; }
        public string? State { get; set; }
        public List<ProcessInfo>? Processes { get; set; }
    }
}