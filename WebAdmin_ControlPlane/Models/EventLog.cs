using System;
using System.ComponentModel.DataAnnotations;

namespace WebAdmin_ControlPlane.Models
{
    public class EventLog
    {
        [Key]
        public int Id { get; set; }

        [Display(Name = "Server Áp Dụng")]
        public string NodeId { get; set; } = string.Empty;

        [Display(Name = "Hành động")]
        public string Action { get; set; } = string.Empty; // VD: Tạo mới, Cập nhật, Xóa

        [Display(Name = "Tiến trình")]
        public string TargetProcess { get; set; } = string.Empty;

        [Display(Name = "Chi tiết")]
        public string Message { get; set; } = string.Empty; // Ghi rõ thành công hay thất bại

        public DateTime Timestamp { get; set; } = DateTime.Now;
    }
}