using System;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

public class TelemetryClient
{
    private static readonly HttpClient _httpClient = new HttpClient();
    private const string HeartbeatUrl = "http://localhost:5247/api/telemetry/heartbeat"; //
    private const string LogUrl = "http://localhost:5247/api/telemetry/log";         // gửi heartbeat, gửi log/event từ proxy về web admin

    public void StartHeartbeat()
    {
        _ = Task.Run(async () =>
        {
            while (true)
            {
                try
                {
                    var payload = new HeartbeatPayload
                    {
                        NodeType = "NetworkProxy",
                        Status = "Running",
                        CPUUsagePercent = 12.5,
                        TotalRAM = 8192,
                        UsedRAM = 2450,
                        ProcessCount = 48,
                        BandwidthInUse = Program.TotalBytesTransferred / 1024.0,
                        ActiveConnections = Program.ActiveProxyConnections
                    };

                    string json = JsonSerializer.Serialize(payload);
                    var content = new StringContent(json, Encoding.UTF8, "application/json");

                    await _httpClient.PostAsync(HeartbeatUrl, content);
                }
                catch
                {
                }

                await Task.Delay(5000);
            }
        });
    }

    public static async Task SendLogAsync(string eventType, string message)
    {
        try
        {
            var payload = new
            {
                NodeType = "NetworkProxy",
                EventType = eventType,
                Message = message,
                Timestamp = DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ")
            };

            string json = JsonSerializer.Serialize(payload);
            var content = new StringContent(json, Encoding.UTF8, "application/json");
            await _httpClient.PostAsync(LogUrl, content);
        }
        catch
        {
        }
    }
}