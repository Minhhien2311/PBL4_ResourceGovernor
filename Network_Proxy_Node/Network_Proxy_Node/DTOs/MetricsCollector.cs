using System;
using System.Diagnostics;
using System.Threading;

public static class MetricsCollector
{
    private static readonly object _lock = new object();
    private static readonly Process _self = Process.GetCurrentProcess();
    private static readonly Stopwatch _clock = Stopwatch.StartNew();

    private static TimeSpan _lastCpu = _self.TotalProcessorTime;
    private static long _lastBytes = 0;
    private static double _lastCpuPercent = 0;
    private static double _lastKbps = 0;

    public static MetricsResponse Snapshot()
    {
        lock (_lock)
        {
            _self.Refresh();
            double elapsed = _clock.Elapsed.TotalSeconds;

            // Chỉ tính lại khi đã qua >= 1 giây
            if (elapsed >= 1.0)
            {
                TimeSpan cpuNow = _self.TotalProcessorTime;
                _lastCpuPercent = Math.Max(0,
                    (cpuNow - _lastCpu).TotalSeconds
                    / (elapsed * Environment.ProcessorCount) * 100.0);
                _lastCpu = cpuNow;

                long bytesNow = Interlocked.Read(ref Program.TotalBytesTransferred);
                _lastKbps = (bytesNow - _lastBytes) / 1024.0 / elapsed;
                _lastBytes = bytesNow;

                _clock.Restart();
            }

            return new MetricsResponse
            {
                status = "ok",
                NodeId = ProxyConfig.NodeId,
                CPUUsagePercent = Math.Round(_lastCpuPercent, 2),
                TotalRAM = GC.GetGCMemoryInfo().TotalAvailableMemoryBytes / 1024 / 1024,
                UsedRAM = _self.WorkingSet64 / 1024 / 1024,
                ProcessCount = GetProcessCount(),
                BandwidthInUse = Math.Round(_lastKbps, 2),
                ActiveConnections = Program.ActiveProxyConnections
            };
        }
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
