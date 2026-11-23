using System;
using System.Diagnostics;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

public class TelemetryClient
{
    private static readonly HttpClient _httpClient = new HttpClient
    {
        Timeout = TimeSpan.FromSeconds(3)
    };

    public void StartHeartbeat()
    {
        _ = Task.Run(async () =>
        {
            using Process self = Process.GetCurrentProcess();
            TimeSpan lastCpu = self.TotalProcessorTime;
            Stopwatch clock = Stopwatch.StartNew();

            while (true)
            {
                await Task.Delay(5000);

                try
                {
                    double elapsedSec = clock.Elapsed.TotalSeconds;
                    clock.Restart();

                    // CPU của riêng tiến trình proxy, tính theo delta
                    self.Refresh();
                    TimeSpan cpuNow = self.TotalProcessorTime;
                    double cpuPercent = (cpuNow - lastCpu).TotalSeconds
                                        / (elapsedSec * Environment.ProcessorCount) * 100.0;
                    lastCpu = cpuNow;

                    // Băng thông (KB/s) tính theo delta giữa 2 lần gửi
                    long totalNow = Interlocked.Read(ref Program.TotalBytesTransferred);
                    long deltaBytes = totalNow - Program.LastBytesSnapshot;
                    Program.LastBytesSnapshot = totalNow;
                    double kbPerSec = deltaBytes / 1024.0 / elapsedSec;

                    var payload = new HeartbeatPayload
                    {
                        NodeType = "NetworkProxy",
                        Status = "Running",
                        CPUUsagePercent = Math.Round(Math.Max(0, cpuPercent), 2),
                        TotalRAM = GetTotalRamMb(),
                        UsedRAM = Math.Round(self.WorkingSet64 / 1024.0 / 1024.0, 1),
                        ProcessCount = GetProcessCount(),
                        BandwidthInUse = Math.Round(kbPerSec, 2),
                        ActiveConnections = Program.ActiveProxyConnections
                    };

                    string json = JsonSerializer.Serialize(payload);
                    var content = new StringContent(json, Encoding.UTF8, "application/json");

                    await _httpClient.PostAsync(ProxyConfig.HeartbeatUrl, content);
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"[Telemetry] Gui heartbeat that bai: {ex.Message}");
                }
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
            await _httpClient.PostAsync(ProxyConfig.LogUrl, content);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[Telemetry] Gui log that bai: {ex.Message}");
        }
    }

    private static double GetTotalRamMb()
    {
        long bytes = GC.GetGCMemoryInfo().TotalAvailableMemoryBytes;
        return Math.Round(bytes / 1024.0 / 1024.0, 0);
    }

    private static int GetProcessCount()
    {
        try
        {
            Process[] all = Process.GetProcesses();
            int count = all.Length;
            foreach (Process p in all) p.Dispose();
            return count;
        }
        catch
        {
            return 0;
        }
    }
}
