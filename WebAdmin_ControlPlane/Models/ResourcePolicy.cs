using System.ComponentModel.DataAnnotations;

namespace WebAdmin_ControlPlane.Models
{
    public class ResourcePolicy
    {
        [Key]
        public int Id { get; set; }

        [Display(Name = "Server Áp Dụng")]
        public string? NodeId { get; set; }

        [Display(Name = "Loại Chính Sách")]
        public string PolicyType { get; set; } = "OS"; // "OS" hoặc "NETWORK"

        // --- OS (RAM/CPU) ---
        [Display(Name = "Tên Tiến trình")]
        public string? TargetProcess { get; set; }

        [Display(Name = "User sở hữu")]
        public string? TargetUser { get; set; }

        [Display(Name = "Giới hạn RAM (MB)")]
        public int? MaxRamMB { get; set; }

        [Display(Name = "Giới hạn CPU (%)")]
        public int? MaxCpuPercent { get; set; }

        [Display(Name = "Thu hồi RAM (Reclaim)")]
        public bool ReclaimRam { get; set; } = true;

        // --- NETWORK (Băng thông) ---
        [Display(Name = "Cổng mạng (Port)")]
        public int? TargetPort { get; set; }

        [Display(Name = "Giới hạn Băng thông (Kbps)")]
        public int? MaxBandwidthKbps { get; set; }

        [Display(Name = "Áp dụng cho IP")]
        public string? ApplyToIp { get; set; } = "All";
    }
}