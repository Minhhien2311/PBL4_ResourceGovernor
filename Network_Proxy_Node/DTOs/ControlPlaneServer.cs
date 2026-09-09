using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

public class ControlPlaneServer
{
    private const string SecretToken = "abc123";

    public async Task StartListeningAsync()
    {
        TcpListener listener = new TcpListener(
            IPAddress.Parse("127.0.0.1"),
            9002
        );

        listener.Start();

        Console.WriteLine("[Control] Dang lang nghe tren 127.0.0.1:9002");

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
            using (StreamReader reader = new StreamReader(
                stream,
                Encoding.UTF8,
                false,
                1024,
                true))
            using (StreamWriter writer = new StreamWriter(
                stream,
                new UTF8Encoding(false),
                1024,
                true)
            {
                AutoFlush = true
            })
            {
                // Đọc đúng 1 command JSON kết thúc bằng newline
                string jsonString = await reader.ReadLineAsync();

                if (string.IsNullOrWhiteSpace(jsonString))
                {
                    return;
                }

                // Cho phép JSON không phân biệt hoa/thường
                var options = new JsonSerializerOptions
                {
                    PropertyNameCaseInsensitive = true
                };

                NetworkPolicyCommand? command;

                try
                {
                    command = JsonSerializer.Deserialize<NetworkPolicyCommand>(
                        jsonString,
                        options
                    );
                }
                catch (JsonException ex)
                {
                    Console.WriteLine(
                        $"[Control] JSON khong hop le: {ex.Message}"
                    );

                    var errorAck = new AckResponse
                    {
                        status = "error",
                        message = "Invalid JSON"
                    };

                    string errorJson = JsonSerializer.Serialize(errorAck);
                    await writer.WriteAsync(errorJson + "\n");

                    return;
                }

                var ack = new AckResponse();

                // Kiểm tra command có tồn tại không
                if (command == null)
                {
                    ack.status = "error";
                    ack.message = "Invalid command";
                }
                // Kiểm tra AuthToken
                else if (command.AuthToken != SecretToken)
                {
                    ack.status = "error";
                    ack.message = "Invalid token";

                    Console.WriteLine(
                        "[Control] Tu choi command: Invalid token"
                    );
                }
                // Xử lý UpdateNetworkPolicy
                else if (command.CommandType == "UpdateNetworkPolicy")
                {
                    lock (Program.PolicyLock)
                    {
                        Program.CurrentPolicy = command;
                    }

                    ack.status = "ok";
                    ack.message = "Network policy applied successfully";

                    Console.WriteLine(
                        $"[Control] Policy applied: " +
                        $"Port={command.TargetPort}, " +
                        $"Bandwidth={command.MaxBandwidthKbps} Kbps, " +
                        $"IP={command.ApplyToIp}"
                    );

                    _ = TelemetryClient.SendLogAsync(
                        "PolicyApplied",
                        $"Da ap dung gioi han bang thong moi: " +
                        $"{command.MaxBandwidthKbps} Kbps"
                    );
                }
                else
                {
                    ack.status = "error";
                    ack.message = "Unknown command";

                    Console.WriteLine(
                        $"[Control] Unknown command: {command.CommandType}"
                    );
                }

                // Gửi ACK kết thúc bằng newline
                string responseJson = JsonSerializer.Serialize(ack);

                await writer.WriteAsync(responseJson + "\n");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine(
                $"[Loi Proxy Bi Crash]: {ex.Message}"
            );
        }
    }
}