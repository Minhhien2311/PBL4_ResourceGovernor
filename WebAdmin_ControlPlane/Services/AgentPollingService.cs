using Microsoft.EntityFrameworkCore;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using WebAdmin_ControlPlane.Data;
using WebAdmin_ControlPlane.Models;

namespace WebAdmin_ControlPlane.Services
{
    public class AgentPollingService : BackgroundService
    {
        private readonly IServiceProvider _serviceProvider;
        private readonly TelemetryCacheService _cache;

        public AgentPollingService(IServiceProvider serviceProvider, TelemetryCacheService cache)
        {
            _serviceProvider = serviceProvider;
            _cache = cache;
        }

        protected override async Task ExecuteAsync(CancellationToken stoppingToken)
        {
            while (!stoppingToken.IsCancellationRequested)
            {
                using (var scope = _serviceProvider.CreateScope())
                {
                    var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();
                    var servers = await db.ServerNodes.ToListAsync();

                    foreach (var server in servers)
                    {
                        await PollServerAsync(server);
                    }
                }
                // Chờ 5 giây trước khi quét lại
                await Task.Delay(5000, stoppingToken);
            }
        }

        private async Task PollServerAsync(ServerNode server)
        {
            try
            {
                using var client = new TcpClient();
                using var cts = new CancellationTokenSource(TimeSpan.FromSeconds(3)); // Timeout 3s
                await client.ConnectAsync(server.IpAddress, server.Port, cts.Token);
                var stream = client.GetStream();

                // 1. Gửi lệnh GetMetrics
                var payload = new { CommandType = "GetMetrics", AuthToken = server.AuthToken };
                byte[] data = Encoding.UTF8.GetBytes(JsonSerializer.Serialize(payload) + "\n");
                await stream.WriteAsync(data);
                await stream.FlushAsync();

                using var reader = new StreamReader(stream, Encoding.UTF8);
                string response = await reader.ReadLineAsync() ?? "";

                if (response.Contains("\"ok\""))
                {
                    // Parse dữ liệu trả về (Bạn cần đảm bảo bên Agent trả về đúng các trường này)
                    var options = new JsonSerializerOptions { PropertyNameCaseInsensitive = true };
                    var metrics = JsonSerializer.Deserialize<HeartbeatPayload>(response, options);

                    if (metrics != null)
                    {
                        metrics.NodeId = server.NodeId; // Gán lại NodeId từ DB cho chắc chắn
                        _cache.LatestHeartbeats[server.NodeId] = metrics;
                        _cache.LastHeartbeatTimes[server.NodeId] = DateTime.Now;

                        // Nếu Agent trả về kèm danh sách Processes trong GetMetrics
                        if (metrics.Processes != null && metrics.Processes.Any())
                        {
                            _cache.NodeProcesses[server.NodeId] = metrics.Processes;
                        }
                    }
                }
            }
            catch (Exception)
            {
                // Nếu lỗi (Agent offline), bỏ qua và vòng lặp sau thử lại
                // Không cập nhật LastHeartbeatTimes -> Dashboard sẽ tự hiện Offline
            }
        }
    }
}