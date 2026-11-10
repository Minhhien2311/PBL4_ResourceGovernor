using System.ComponentModel.DataAnnotations;

namespace WebAdmin_ControlPlane.Models
{
    public class ServerNode
    {
        [Key]
        public string NodeId { get; set; } = string.Empty;

        public string IpAddress { get; set; } = string.Empty;

        public int Port { get; set; } = 9001;

        public string AuthToken { get; set; } = "abc123";
    }
}