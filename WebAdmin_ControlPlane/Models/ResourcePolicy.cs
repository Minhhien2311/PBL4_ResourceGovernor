using System.ComponentModel.DataAnnotations;

namespace WebAdmin_ControlPlane.Models
{
    public enum PolicyType
    {
        OsResource = 1,
        NetworkQoS = 2
    }

    public class ResourcePolicy
    {
        [Key]
        public int Id { get; set; }

        [Required]
        [Display(Name = "Loại Luật")]
        public PolicyType Type { get; set; }

        [Display(Name = "Trạng thái")]
        public bool IsActive { get; set; } = true;

        // --- Dành cho OS Agent ---
        [Display(Name = "Tên Tiến trình (App)")]
        public string? TargetProcess { get; set; }

        [Display(Name = "Giới hạn RAM (MB)")]
        public int? MaxRamMB { get; set; }

        [Display(Name = "Hành động (Kill/Notify)")]
        public string? OsAction { get; set; } = "Kill";

        // --- Dành cho Network Proxy ---
        [Display(Name = "Cổng mạng (Port)")]
        public int? TargetPort { get; set; }

        [Display(Name = "Giới hạn Băng thông (Kbps)")]
        public int? MaxBandwidthKbps { get; set; }

        [Display(Name = "Áp dụng cho IP")]
        public string? ApplyToIp { get; set; } = "All";
    }
}