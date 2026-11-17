using System.ComponentModel.DataAnnotations;

namespace WebAdmin_ControlPlane.Models
{
    public class SystemConfig
    {
        [Key]
        public int Id { get; set; }

        [Required]
        [Display(Name = "Auth Token")]
        public string AuthToken { get; set; } = "abc123";

        [Display(Name = "OS Agent IP")]
        public string OSAgentIP { get; set; } = "127.0.0.1";

        [Display(Name = "OS Agent Port")]
        public int OSAgentPort { get; set; } = 9001;

        [Display(Name = "Network Proxy IP")]
        public string ProxyIP { get; set; } = "127.0.0.1";

        [Display(Name = "Network Proxy Port")]
        public int ProxyPort { get; set; } = 9002;

        [Display(Name = "Heartbeat Interval (seconds)")]
        public int HeartbeatIntervalSeconds { get; set; } = 5;
    }
}