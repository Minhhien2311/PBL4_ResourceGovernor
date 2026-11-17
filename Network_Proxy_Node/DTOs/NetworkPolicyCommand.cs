public class NetworkPolicyCommand
{
    public string CommandType { get; set; } = string.Empty;
    public int TargetPort { get; set; }
    public int MaxBandwidthKbps { get; set; }
    public string ApplyToIp { get; set; } = "All";
    public long Timestamp { get; set; }
}