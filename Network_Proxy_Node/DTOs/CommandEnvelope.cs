using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PBL4_ResourceGovernor.Network_Proxy_Node.DTOs
{
    public class CommandEnvelope
    {
        public string Payload { get; set; } = string.Empty;
        public string Signature { get; set; } = string.Empty;
    }
}
