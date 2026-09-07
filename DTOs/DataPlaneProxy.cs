using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

public class DataPlaneProxy
{
    private readonly int _listenPort;

    public DataPlaneProxy(int listenPort)
    {
        _listenPort = listenPort;
    }

    public async Task StartAsync()
    {
        TcpListener listener = new TcpListener(IPAddress.Parse("127.0.0.1"), _listenPort);
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
                using TcpClient upstream = new TcpClient();
                try
                {
                    await upstream.ConnectAsync("127.0.0.1", 80);
                }
                catch
                {
                    return;
                }

                using NetworkStream clientStream = client.GetStream();
                using NetworkStream upstreamStream = upstream.GetStream();

                int kbps;
                lock (Program.PolicyLock)
                {
                    kbps = Program.CurrentPolicy.MaxBandwidthKbps;
                }

                double bytesPerSecond = (kbps * 1024.0) / 8.0;
                var tokenBucket = new TokenBucket((long)(bytesPerSecond * 2), bytesPerSecond);

                Task clientToUpstream = RelayDataAsync(clientStream, upstreamStream, null);
                Task upstreamToClient = RelayDataAsync(upstreamStream, clientStream, tokenBucket);

                await Task.WhenAny(clientToUpstream, upstreamToClient);
            }
        }
        catch
        {
        }
        finally
        {
            Interlocked.Decrement(ref Program.ActiveProxyConnections);
        }
    }

    private async Task RelayDataAsync(Stream source, Stream destination, TokenBucket tokenBucket)
    {
        byte[] buffer = new byte[4096];
        int bytesRead;
        try
        {
            while ((bytesRead = await source.ReadAsync(buffer, 0, buffer.Length)) > 0)
            {
                if (tokenBucket != null)
                {
                    await tokenBucket.ConsumeAsync(bytesRead);
                }

                await destination.WriteAsync(buffer, 0, bytesRead);
                Interlocked.Add(ref Program.TotalBytesTransferred, bytesRead);
            }
        }
        catch
        {
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

    public TokenBucket(long capacity, double fillRatePerSecond)
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
        double elapsedSeconds = (now - _lastRefill).TotalSeconds;
        if (elapsedSeconds > 0)
        {
            double tokensToAdd = elapsedSeconds * _fillRatePerSecond;
            _tokens = Math.Min(_capacity, _tokens + tokensToAdd);
            _lastRefill = now;
        }
    }
}