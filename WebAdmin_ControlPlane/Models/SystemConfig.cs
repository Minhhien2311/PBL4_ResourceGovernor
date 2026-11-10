using System.ComponentModel.DataAnnotations;
namespace WebAdmin_ControlPlane.Models
{
    public class SystemConfig
    {
        [Key]
        public int Id { get; set; }

        [Display(Name = "Controller IP")]
        public string ControllerIP { get; set; } = "127.0.0.1";
        [Display(Name = "Controller Port")]
        public int ControllerPort { get; set; } = 9100;
        [Display(Name = "Controller Auth Token")]
        public string ControllerToken { get; set; } = "ctl-token";

        // Cấu hình để gọi trực tiếp Agent
        [Display(Name = "OS Agent IP")]
        public string AgentIP { get; set; } = "127.0.0.1";
        [Display(Name = "OS Agent Port")]
        public int AgentPort { get; set; } = 9001;
        [Display(Name = "OS Agent Auth Token")]
        public string AgentToken { get; set; } = "abc123";
    }
}