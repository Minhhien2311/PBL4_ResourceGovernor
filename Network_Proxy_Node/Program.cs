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

    // Dùng bởi TelemetryClient (vòng heartbeat đẩy HTTP cũ, hiện đã tắt)
    public static long LastBytesSnapshot = 0;

    static async Task Main(string[] args)
    {
        Console.WriteLine("Khoi dong Network QoS Proxy (Full Data & Control Plane)...");
        Console.WriteLine(
            $"[Config] NodeId={ProxyConfig.NodeId}, " +
            $"ControlPort={ProxyConfig.ControlPort}, DataPort={ProxyConfig.DataPort}");

        // Không còn Controller: Proxy tự đẩy heartbeat HTTP lên Web Admin (PROXY_HEARTBEAT_URL).
        new TelemetryClient().StartHeartbeat();

        var controlPlane = new ControlPlaneServer();
        var dataPlaneProxy = new PolicyListener(ProxyConfig.DataPort);

        // Cong dieu khien phu (mac dinh 9001): KHONG dua vao WhenAny, bind loi thi bo qua.
        foreach (int extra in ProxyConfig.ExtraControlPorts)
        {
            _ = controlPlane.StartListeningAsync(extra, optional: true);
        }

        Task controlTask = controlPlane.StartListeningAsync();
        Task dataTask = dataPlaneProxy.StartAsync();

        // Nếu 1 trong 2 listener chết (ví dụ port bị chiếm), await sẽ ném lại exception
        Task finished = await Task.WhenAny(controlTask, dataTask);
        await finished;
    }
}
