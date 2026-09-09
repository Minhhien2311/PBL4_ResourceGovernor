public class HeartbeatPayload
{
    public string NodeType { get; set; }
    public string Status { get; set; }
    public double CPUUsagePercent { get; set; }
    public double TotalRAM { get; set; }
    public double UsedRAM { get; set; }
    public int ProcessCount { get; set; }
    public double BandwidthInUse { get; set; }
    public int ActiveConnections { get; set; }
}