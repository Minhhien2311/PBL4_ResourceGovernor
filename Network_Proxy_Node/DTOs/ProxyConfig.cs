using System;
using System.Collections.Generic;

public static class ProxyConfig
{
    // Token này phải trùng với token của proxy trong servers.conf (TV1)
    public static readonly string SecretToken =
        Environment.GetEnvironmentVariable("PROXY_TOKEN") ?? "abc123";

    // Phải TRÙNG đúng tên proxy trong servers.conf của Controller
    public static readonly string NodeId =
        Environment.GetEnvironmentVariable("PROXY_NODE_ID") ?? "proxy-a";

    public static readonly int ControlPort =
        int.Parse(Environment.GetEnvironmentVariable("PROXY_CONTROL_PORT") ?? "9002");

    // Cong dieu khien PHU (mac dinh 9001). Controller hoi GetMetrics o cot port_os (9001),
    // neu Proxy chi nghe 9002 thi bi "Connection refused". Doi bang PROXY_EXTRA_PORTS=9001,9003
    // hoac PROXY_EXTRA_PORTS=none de tat.
    public static readonly int[] ExtraControlPorts = ParseExtraPorts();

    private static int[] ParseExtraPorts()
    {
        string raw = Environment.GetEnvironmentVariable("PROXY_EXTRA_PORTS") ?? "9001";
        var ports = new List<int>();
        foreach (string part in raw.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
        {
            if (int.TryParse(part, out int p) && p >= 1 && p <= 65535
                && p != ControlPort && !ports.Contains(p))
            {
                ports.Add(p);
            }
        }
        return ports.ToArray();
    }

    public static readonly int DataPort =
        int.Parse(Environment.GetEnvironmentVariable("PROXY_DATA_PORT") ?? "8080");

    public static readonly string HeartbeatUrl =
        Environment.GetEnvironmentVariable("PROXY_HEARTBEAT_URL")
        ?? "http://172.20.10.4:5247/api/telemetry/heartbeat";

    public static readonly string LogUrl =
        Environment.GetEnvironmentVariable("PROXY_LOG_URL")
        ?? "http://172.20.10.4:5247/api/telemetry/log";

    public static readonly string UpstreamHost =
        Environment.GetEnvironmentVariable("PROXY_UPSTREAM_HOST") ?? "127.0.0.1";

    public const int MaxTimestampSkewSeconds = 5;
}
