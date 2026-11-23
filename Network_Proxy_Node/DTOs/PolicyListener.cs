using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

public class PolicyListener
{
    private readonly int _listenPort;

    public PolicyListener(int listenPort)
    {
        _listenPort = listenPort;
    }

    public async Task StartAsync()
    {
        TcpListener listener = new TcpListener(IPAddress.Any, _listenPort);
        listener.Start();

        Console.WriteLine($"[DataPlane] Lang nghe tai cong {_listenPort}");

        while (true)
        {
            TcpClient client = await listener.AcceptTcpClientAsync();
            _ = Task.Run(() => HandleClientAsync(client));
        }
    }

    private async Task HandleClientAsync(TcpClient client)
    {
        Interlocked.Increment(ref Program.ActiveProxyConnections);

        try
        {
            using (client)
            {
                string remoteIp = GetRemoteIp(client);

                int targetPort;
                lock (Program.PolicyLock)
                {
                    targetPort = Program.CurrentPolicy.TargetPort;
                }

                Console.WriteLine(
                    $"[DataPlane] Ket noi moi tu {remoteIp} -> {ProxyConfig.UpstreamHost}:{targetPort}");

                using TcpClient upstream = new TcpClient();

                try
                {
                    await upstream.ConnectAsync(ProxyConfig.UpstreamHost, targetPort);
                }
                catch (Exception ex)
                {
                    Console.WriteLine(
                        $"[DataPlane] Khong the ket noi upstream " +
                        $"{ProxyConfig.UpstreamHost}:{targetPort}: {ex.Message}");
                    return;
                }

                using NetworkStream clientStream = client.GetStream();
                using NetworkStream upstreamStream = upstream.GetStream();

                // Bucket khởi tạo với tốc độ tạm, mỗi vòng relay cập nhật lại theo policy hiện tại
                var tokenBucket = new TokenBucket(KbpsToBytesPerSecond(1024));

                // Client -> Upstream: không giới hạn
                Task clientToUpstream = RelayDataAsync(
                    clientStream, upstreamStream, null, remoteIp);

                // Upstream -> Client: giới hạn băng thông
                Task upstreamToClient = RelayDataAsync(
                    upstreamStream, clientStream, tokenBucket, remoteIp);

                await Task.WhenAll(clientToUpstream, upstreamToClient);
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[Loi Proxy Bi Crash]: {ex.Message}");
        }
        finally
        {
            Interlocked.Decrement(ref Program.ActiveProxyConnections);
        }
    }

    private async Task RelayDataAsync(
        Stream source,
        Stream destination,
        TokenBucket? tokenBucket,
        string remoteIp)
    {
        byte[] buffer = new byte[4096];
        int lastKbps = 0;

        try
        {
            while (true)
            {
                // Chỉ throttle chiều upstream -> client (tokenBucket != null)
                int kbps = tokenBucket != null ? GetEffectiveKbps(remoteIp) : 0;
                int toRead = buffer.Length;

                if (tokenBucket != null && kbps > 0)
                {
                    // Policy mới có hiệu lực ngay cả với kết nối đang chạy
                    if (kbps != lastKbps)
                    {
                        tokenBucket.SetRate(KbpsToBytesPerSecond(kbps));
                        lastKbps = kbps;
                    }

                    // Không đọc nhiều hơn dung lượng bucket, tránh treo vô hạn
                    toRead = Math.Min(buffer.Length, tokenBucket.MaxChunk);
                }

                int bytesRead = await source.ReadAsync(buffer, 0, toRead);
                if (bytesRead <= 0)
                {
                    break;
                }

                if (tokenBucket != null && kbps > 0)
                {
                    await tokenBucket.ConsumeAsync(bytesRead);
                }

                await destination.WriteAsync(buffer, 0, bytesRead);

                Interlocked.Add(ref Program.TotalBytesTransferred, bytesRead);
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[DataPlane] Loi relay du lieu: {ex.Message}");
        }
    }

    // Trả về kbps áp dụng cho IP này, hoặc 0 nếu không bị giới hạn
    private static int GetEffectiveKbps(string remoteIp)
    {
        lock (Program.PolicyLock)
        {
            NetworkPolicyCommand p = Program.CurrentPolicy;

            if (p.MaxBandwidthKbps <= 0)
            {
                return 0;
            }

            string target = (p.ApplyToIp ?? "All").Trim();

            if (target.Length == 0
                || string.Equals(target, "All", StringComparison.OrdinalIgnoreCase)
                || string.Equals(target, remoteIp, StringComparison.OrdinalIgnoreCase))
            {
                return p.MaxBandwidthKbps;
            }

            return 0;
        }
    }

    private static double KbpsToBytesPerSecond(int kbps)
    {
        return (kbps * 1024.0) / 8.0;
    }

    private static string GetRemoteIp(TcpClient client)
    {
        if (client.Client.RemoteEndPoint is IPEndPoint ep)
        {
            return ep.Address.MapToIPv4().ToString();
        }
        return string.Empty;
    }
}

public class TokenBucket
{
    private long _capacity;
    private double _tokens;
    private double _fillRatePerSecond;
    private DateTime _lastRefill;
    private readonly object _lock = new object();

    public TokenBucket(double fillRatePerSecond)
    {
        _fillRatePerSecond = fillRatePerSecond;
        _capacity = Math.Max(1, (long)(fillRatePerSecond * 2));
        _tokens = _capacity;
        _lastRefill = DateTime.UtcNow;
    }

    // Số byte tối đa nên đọc/ghi trong 1 lần, luôn <= capacity
    public int MaxChunk
    {
        get
        {
            lock (_lock)
            {
                return (int)Math.Max(1, Math.Min(4096, _capacity));
            }
        }
    }

    // Đổi tốc độ khi policy thay đổi
    public void SetRate(double fillRatePerSecond)
    {
        lock (_lock)
        {
            Refill();

            _fillRatePerSecond = fillRatePerSecond;
            _capacity = Math.Max(1, (long)(fillRatePerSecond * 2));
            _tokens = Math.Min(_tokens, _capacity);
        }
    }

    public async Task ConsumeAsync(int tokensToConsume)
    {
        while (true)
        {
            lock (_lock)
            {
                Refill();

                // Chặn trường hợp yêu cầu lớn hơn capacity (sẽ không bao giờ đủ token)
                int need = (int)Math.Min(tokensToConsume, _capacity);

                if (_tokens >= need)
                {
                    _tokens -= need;
                    return;
                }
            }

            await Task.Delay(5);
        }
    }

    private void Refill()
    {
        DateTime now = DateTime.UtcNow;
        double elapsedSeconds = (now - _lastRefill).TotalSeconds;

        if (elapsedSeconds > 0)
        {
            _tokens = Math.Min(_capacity, _tokens + elapsedSeconds * _fillRatePerSecond);
            _lastRefill = now;
        }
    }
}
