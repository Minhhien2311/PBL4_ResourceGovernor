using System.ComponentModel.DataAnnotations;
namespace WebAdmin_ControlPlane.Models
{
    public class ResourcePolicy
    {
        [Key]
        public int Id { get; set; }

        [Display(Name = "Tên Tiến trình")]
        public string? TargetProcess { get; set; }

        [Display(Name = "User sở hữu (TargetUser)")]
        public string? TargetUser { get; set; } // Thêm mới

        [Display(Name = "Giới hạn RAM (MB)")]
        public int? MaxRamMB { get; set; }

        [Display(Name = "Hành động")]
        public string? OsAction { get; set; }

        [Display(Name = "Áp dụng cho Server")]
        public string? NodeId { get; set; }
    }
}