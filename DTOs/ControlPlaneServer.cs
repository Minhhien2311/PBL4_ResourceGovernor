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
        TcpListener listener = new TcpListener(IPAddress.Parse("127.0.0.1"), 9002);
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
            using (StreamReader reader = new StreamReader(stream, Encoding.UTF8))
            using (StreamWriter writer = new StreamWriter(stream, Encoding.UTF8) { AutoFlush = true })
            {
                string jsonString = await reader.ReadLineAsync();
                if (string.IsNullOrEmpty(jsonString)) return;

                var command = JsonSerializer.Deserialize<NetworkPolicyCommand>(jsonString);
                var ack = new AckResponse();

                if (command?.AuthToken != SecretToken)
                {
                    ack.status = "error";
                    ack.message = "Invalid token";
                }
                else if (command?.CommandType == "UpdateNetworkPolicy")
                {
                    lock (Program.PolicyLock)
                    {
                        Program.CurrentPolicy = command;
                    }
                    ack.status = "ok";
                    ack.message = "Network policy applied successfully";

                    _ = TelemetryClient.SendLogAsync("PolicyApplied", $"Da ap dung gioi han bang thong moi: {command.MaxBandwidthKbps} Kbps");
                }
                else
                {
                    ack.status = "error";
                    ack.message = "Unknown command";
                }

                string responseJson = JsonSerializer.Serialize(ack);
                await writer.WriteAsync(responseJson + "\n");
            }
        }
        catch
        {
        }
    }
}