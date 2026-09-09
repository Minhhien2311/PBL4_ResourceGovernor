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
        TcpListener listener = new TcpListener(
            IPAddress.Parse("127.0.0.1"),
            _listenPort
        );

        listener.Start();

        Console.WriteLine(
            $"[DataPlane] Lang nghe tai cong {_listenPort}"
        );

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
                int targetPort;
                int kbps;

                // Đọc policy hiện tại một cách thread-safe
                lock (Program.PolicyLock)
                {
                    targetPort = Program.CurrentPolicy.TargetPort;
                    kbps = Program.CurrentPolicy.MaxBandwidthKbps;
                }

                Console.WriteLine(
                    $"[DataPlane] Ket noi moi -> 127.0.0.1:{targetPort}"
                );

                using TcpClient upstream = new TcpClient();

                try
                {
                    // Kết nối tới port được cấu hình trong Network Policy
                    await upstream.ConnectAsync(
                        "127.0.0.1",
                        targetPort
                    );
                }
                catch (Exception ex)
                {
                    Console.WriteLine(
                        $"[DataPlane] Khong the ket noi upstream " +
                        $"127.0.0.1:{targetPort}: {ex.Message}"
                    );

                    return;
                }

                using NetworkStream clientStream = client.GetStream();
                using NetworkStream upstreamStream = upstream.GetStream();

                // Kiểm tra bandwidth hợp lệ
                if (kbps <= 0)
                {
                    Console.WriteLine(
                        $"[DataPlane] MaxBandwidthKbps khong hop le: {kbps}"
                    );

                    return;
                }

                // Kbps -> Bytes/second
                double bytesPerSecond =
                    (kbps * 1024.0) / 8.0;

                // Bucket có khả năng chứa tối đa 2 giây dữ liệu
                long bucketCapacity =
                    (long)(bytesPerSecond * 2);

                var tokenBucket = new TokenBucket(
                    bucketCapacity,
                    bytesPerSecond
                );

                // Client -> Upstream: không giới hạn
                Task clientToUpstream =
                    RelayDataAsync(
                        clientStream,
                        upstreamStream,
                        null
                    );

                // Upstream -> Client: giới hạn bandwidth
                Task upstreamToClient =
                    RelayDataAsync(
                        upstreamStream,
                        clientStream,
                        tokenBucket
                    );

                // Chờ cả hai chiều hoàn thành
                await Task.WhenAll(
                    clientToUpstream,
                    upstreamToClient
                );
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine(
                $"[Loi Proxy Bi Crash]: {ex.Message}"
            );
        }
        finally
        {
            Interlocked.Decrement(
                ref Program.ActiveProxyConnections
            );
        }
    }

    private async Task RelayDataAsync(
        Stream source,
        Stream destination,
        TokenBucket tokenBucket)
    {
        byte[] buffer = new byte[4096];

        try
        {
            int bytesRead;

            while (
                (bytesRead = await source.ReadAsync(
                    buffer,
                    0,
                    buffer.Length
                )) > 0)
            {
                // Chỉ throttle chiều upstream -> client
                if (tokenBucket != null)
                {
                    await tokenBucket.ConsumeAsync(bytesRead);
                }

                await destination.WriteAsync(
                    buffer,
                    0,
                    bytesRead
                );

                Interlocked.Add(
                    ref Program.TotalBytesTransferred,
                    bytesRead
                );
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine(
                $"[DataPlane] Loi relay du lieu: {ex.Message}"
            );
        }
    }
}

public class TokenBucket
{
    private readonly long _capacity;
    private double _tokens;
    private readonly double _fillRatePerSecond;
    private DateTime _lastRefill;
    private readonly object _lock = new object();

    public TokenBucket(
        long capacity,
        double fillRatePerSecond)
    {
        _capacity = capacity;
        _tokens = capacity;
        _fillRatePerSecond = fillRatePerSecond;
        _lastRefill = DateTime.UtcNow;
    }

    public async Task ConsumeAsync(int tokensToConsume)
    {
        while (true)
        {
            lock (_lock)
            {
                Refill();

                if (_tokens >= tokensToConsume)
                {
                    _tokens -= tokensToConsume;
                    return;
                }
            }

            await Task.Delay(5);
        }
    }

    private void Refill()
    {
        DateTime now = DateTime.UtcNow;

        double elapsedSeconds =
            (now - _lastRefill).TotalSeconds;

        if (elapsedSeconds > 0)
        {
            double tokensToAdd =
                elapsedSeconds * _fillRatePerSecond;

            _tokens = Math.Min(
                _capacity,
                _tokens + tokensToAdd
            );

            _lastRefill = now;
        }
    }
}