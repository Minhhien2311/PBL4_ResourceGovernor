using System;

public static class ProxyConfig
{
    // Token này phải trùng với token của proxy trong servers.conf (TV1)
    public static readonly string SecretToken =
        Environment.GetEnvironmentVariable("PROXY_TOKEN") ?? "abc123";

    public static readonly int ControlPort =
        int.Parse(Environment.GetEnvironmentVariable("PROXY_CONTROL_PORT") ?? "9002");

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