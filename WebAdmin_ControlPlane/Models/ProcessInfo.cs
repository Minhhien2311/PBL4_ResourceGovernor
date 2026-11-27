namespace WebAdmin_ControlPlane.Models
{
    public class ProcessInfo
    {
        public string Name { get; set; } = string.Empty;
        public int Pid { get; set; }
        public string User { get; set; } = string.Empty;
        public double RamMB { get; set; }
        public double? CpuPercent { get; set; }
    }
}