using System;
using System.Threading.Tasks;

class Program
{
    public static NetworkPolicyCommand CurrentPolicy = new NetworkPolicyCommand
    {
        TargetPort = 80,
        MaxBandwidthKbps = 1024,
        ApplyToIp = "All"
    };
    public static readonly object PolicyLock = new object();

    public static long TotalBytesTransferred = 0;
    public static int ActiveProxyConnections = 0;

    static async Task Main(string[] args)
    {
        Console.WriteLine("Khoi dong Network QoS Proxy (Full Data & Control Plane)...");

        var telemetry = new TelemetryClient();
        telemetry.StartHeartbeat();

        var controlPlane = new ControlPlaneServer();
        _ = controlPlane.StartListeningAsync();

        var dataPlaneProxy = new DataPlaneProxy(8080);
        await dataPlaneProxy.StartAsync();
    }
}