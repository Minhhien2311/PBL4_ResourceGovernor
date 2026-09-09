public class NetworkPolicyCommand
{
    public string CommandType { get; set; }
    public int TargetPort { get; set; }
    public int MaxBandwidthKbps { get; set; }
    public string ApplyToIp { get; set; }
    public string AuthToken { get; set; }
}