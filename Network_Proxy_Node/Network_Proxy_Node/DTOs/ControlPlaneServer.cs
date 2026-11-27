using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

public class ControlPlaneServer
{
    private static readonly JsonSerializerOptions JsonOptions = new JsonSerializerOptions
    {
        PropertyNameCaseInsensitive = true
    };

    // port = 0: dung cong chinh (ProxyConfig.ControlPort).
    // optional = true: cong phu, neu bi chiem (vi du Agent da dung 9001) thi chi canh bao va bo qua.
    public async Task StartListeningAsync(int port = 0, bool optional = false)
    {
        if (port == 0)
        {
            port = ProxyConfig.ControlPort;
        }

        TcpListener listener = new TcpListener(IPAddress.Any, port);
        try
        {
            listener.Start();
        }
        catch (SocketException ex) when (optional)
        {
            Console.WriteLine($"[Control] Khong mo duoc cong phu {port}: {ex.Message} (bo qua)");
            return;
        }

        Console.WriteLine($"[Control] Dang lang nghe tren 0.0.0.0:{port}");

        while (true)
        {
            TcpClient client = await listener.AcceptTcpClientAsync();
            _ = Task.Run(() => HandleAdminCommandAsync(client));
        }
    }

    private async Task HandleAdminCommandAsync(TcpClient client)
    {
        try
        {
            using (client)
            using (NetworkStream stream = client.GetStream())
            using (StreamReader reader = new StreamReader(stream, Encoding.UTF8, false, 1024, true))
            using (StreamWriter writer = new StreamWriter(stream, new UTF8Encoding(false), 1024, true) { AutoFlush = true })
            {
                string? line;

                // Chờ tối đa 5 giây cho 1 dòng JSON kết thúc bằng \n
                try
                {
                    using var cts = new CancellationTokenSource(TimeSpan.FromSeconds(5));
                    line = await reader.ReadLineAsync(cts.Token);
                }
                catch (OperationCanceledException)
                {
                    Console.WriteLine("[Control] Timeout khi doc command");
                    return;
                }

                if (string.IsNullOrWhiteSpace(line))
                {
                    return;
                }

                object response = ProcessMessage(line);

                // Kiểu object: System.Text.Json tự dùng kiểu thật (AckResponse hoặc MetricsResponse)
                await writer.WriteAsync(JsonSerializer.Serialize(response) + "\n");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[Loi Proxy Bi Crash]: {ex.Message}");
        }
    }

    private object ProcessMessage(string line)
    {
        // Lớp 1: bóc vỏ { Payload, Signature }
        CommandEnvelope? envelope;
        try
        {
            envelope = JsonSerializer.Deserialize<CommandEnvelope>(line, JsonOptions);
        }
        catch (JsonException ex)
        {
            Console.WriteLine($"[Control] JSON khong hop le: {ex.Message}");
            return Error("Invalid JSON");
        }

        if (envelope == null
            || string.IsNullOrEmpty(envelope.Payload)
            || string.IsNullOrEmpty(envelope.Signature))
        {
            return Error("Invalid JSON");
        }

        // Xác thực HMAC trên chuỗi Payload NGUYÊN BẢN
        string expected = HmacHelper.ComputeHmacSha256Hex(
            envelope.Payload,
            ProxyConfig.SecretToken);

        if (!HmacHelper.SignatureEquals(envelope.Signature, expected))
        {
            Console.WriteLine("[Control] Tu choi command: Invalid Signature");
            return Error("Invalid Signature");
        }

        // Lớp 2: chỉ parse Payload SAU KHI chữ ký đúng
        NetworkPolicyCommand? command;
        try
        {
            command = JsonSerializer.Deserialize<NetworkPolicyCommand>(
                envelope.Payload,
                JsonOptions);
        }
        catch (JsonException ex)
        {
            Console.WriteLine($"[Control] Payload khong hop le: {ex.Message}");
            return Error("Invalid JSON");
        }

        if (command == null)
        {
            return Error("Invalid command");
        }

        // Kiểm tra Timestamp (Unix seconds, lệch tối đa 5 giây)
        if (!HmacHelper.IsTimestampValid(
                command.Timestamp,
                ProxyConfig.MaxTimestampSkewSeconds))
        {
            Console.WriteLine("[Control] Tu choi command: Timestamp expired");
            return Error("Timestamp expired");
        }

        // Controller hỏi số liệu định kỳ (poller.cpp, 5 giây/lần)
        if (command.CommandType == "GetMetrics")
        {
            return MetricsCollector.Snapshot();
        }

        // Xử lý lệnh giới hạn băng thông.
        // Web Admin gọi thẳng Proxy nên chấp nhận cả tên lệnh "SetNetworkPolicy".
        if (command.CommandType == "UpdateNetworkPolicy"
            || command.CommandType == "SetNetworkPolicy")
        {
            if (command.TargetPort < 1 || command.TargetPort > 65535)
            {
                return Error("Invalid TargetPort");
            }

            if (command.MaxBandwidthKbps <= 0)
            {
                return Error("Invalid MaxBandwidthKbps");
            }

            lock (Program.PolicyLock)
            {
                Program.CurrentPolicy = command;
            }

            Console.WriteLine(
                $"[Control] Policy applied: Port={command.TargetPort}, " +
                $"Bandwidth={command.MaxBandwidthKbps} Kbps, IP={command.ApplyToIp}");

            return new AckResponse
            {
                status = "ok",
                message = "Network policy applied"
            };
        }

        Console.WriteLine($"[Control] Unknown command: {command.CommandType}");
        return Error("Unknown command");
    }

    private static AckResponse Error(string message)
    {
        return new AckResponse { status = "error", message = message };
    }
}
