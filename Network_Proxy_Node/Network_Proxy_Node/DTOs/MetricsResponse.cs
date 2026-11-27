public class MetricsResponse
{
    public string status { get; set; } = "ok";
    public string NodeId { get; set; } = string.Empty;
    public double CPUUsagePercent { get; set; }
    public long TotalRAM { get; set; }
    public long UsedRAM { get; set; }
    public int ProcessCount { get; set; }
    public double BandwidthInUse { get; set; }
    public int ActiveConnections { get; set; }
}
