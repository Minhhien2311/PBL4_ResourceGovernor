public class HeartbeatPayload
{
    public string NodeType { get; set; } // loại node đang gửi heartbeat
    public string Status { get; set; } // trạng thái node
    public double CPUUsagePercent { get; set; }
    public double TotalRAM { get; set; }
    public double UsedRAM { get; set; }
    public int ProcessCount { get; set; }
    public double BandwidthInUse { get; set; }
    public int ActiveConnections { get; set; }
}