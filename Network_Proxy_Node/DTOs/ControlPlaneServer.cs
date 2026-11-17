using PBL4_ResourceGovernor.Network_Proxy_Node.DTOs;
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

    public async Task StartListeningAsync()
    {
        TcpListener listener = new TcpListener(IPAddress.Any, ProxyConfig.ControlPort);
        listener.Start();

        Console.WriteLine($"[Control] Dang lang nghe tren 0.0.0.0:{ProxyConfig.ControlPort}");

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

                AckResponse ack = ProcessMessage(line);

                await writer.WriteAsync(JsonSerializer.Serialize(ack) + "\n");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[Loi Proxy Bi Crash]: {ex.Message}");
        }
    }

    private AckResponse ProcessMessage(string line)
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

        // Xử lý lệnh
        if (command.CommandType == "UpdateNetworkPolicy")
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

            _ = TelemetryClient.SendLogAsync(
                "PolicyApplied",
                $"Da ap dung gioi han bang thong moi: {command.MaxBandwidthKbps} Kbps");

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