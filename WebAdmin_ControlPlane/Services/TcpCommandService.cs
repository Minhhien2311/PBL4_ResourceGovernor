using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Net.Http.Json;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using WebAdmin_ControlPlane.Data;
using WebAdmin_ControlPlane.Models;
using Microsoft.EntityFrameworkCore;

namespace WebAdmin_ControlPlane.Services
{
    public class TcpCommandService : BackgroundService
    {
        private readonly IServiceScopeFactory _scopeFactory;
        private readonly ILogger<TcpCommandService> _logger;

        public TcpCommandService(IServiceScopeFactory scopeFactory, ILogger<TcpCommandService> logger)
        {
            _scopeFactory = scopeFactory;
            _logger = logger;
        }

        protected override async Task ExecuteAsync(CancellationToken stoppingToken)
        {
            while (!stoppingToken.IsCancellationRequested)
            {
                await Task.Delay(TimeSpan.FromSeconds(30), stoppingToken);
            }
        }

        // Lấy cấu hình hệ thống từ DB
        private async Task<SystemConfig> GetSystemConfigAsync()
        {
            using var scope = _scopeFactory.CreateScope();
            var dbContext = scope.ServiceProvider.GetRequiredService<AppDbContext>();
            return await dbContext.SystemConfigs.FirstOrDefaultAsync();
        }

        public async Task<(bool success, string message)> SendPolicyToNodeAsync(ResourcePolicy policy)
        {
            var config = await GetSystemConfigAsync();
            if (config == null)
            {
                return (false, "Chưa có cấu hình hệ thống. Vui lòng tạo trong trang Cấu hình.");
            }

            string authToken = config.AuthToken;
            string nodeType = policy.Type == PolicyType.OsResource ? "OSAgent" : "NetworkProxy";
            int port = policy.Type == PolicyType.OsResource ? config.OSAgentPort : config.ProxyPort;
            string ip = policy.Type == PolicyType.OsResource ? config.OSAgentIP : config.ProxyIP;

            object payload = null;
            if (policy.Type == PolicyType.OsResource)
            {
                payload = new
                {
                    CommandType = "UpdateOsPolicy",
                    TargetProcess = policy.TargetProcess,
                    MaxRamMB = policy.MaxRamMB,
                    Action = policy.OsAction,
                    AuthToken = authToken
                };
            }
            else
            {
                payload = new
                {
                    CommandType = "UpdateNetworkPolicy",
                    TargetPort = policy.TargetPort,
                    MaxBandwidthKbps = policy.MaxBandwidthKbps,
                    ApplyToIp = policy.ApplyToIp,
                    AuthToken = authToken
                };
            }

            string json = JsonSerializer.Serialize(payload);

            try
            {
                using var client = new TcpClient();
                var connectTask = client.ConnectAsync(ip, port);
                if (await Task.WhenAny(connectTask, Task.Delay(3000)) != connectTask)
                {
                    return (false, $"Không thể kết nối tới {nodeType} (timeout)");
                }

                var stream = client.GetStream();
                byte[] data = Encoding.UTF8.GetBytes(json + "\n");
                await stream.WriteAsync(data, 0, data.Length);
                await stream.FlushAsync();

                var buffer = new byte[2048];
                var readTask = stream.ReadAsync(buffer, 0, buffer.Length);
                if (await Task.WhenAny(readTask, Task.Delay(5000)) != readTask)
                {
                    return (false, "Không nhận được phản hồi (ACK) từ node");
                }

                int bytesRead = await readTask;
                string ackJson = Encoding.UTF8.GetString(buffer, 0, bytesRead).Trim();
                var ack = JsonSerializer.Deserialize<Dictionary<string, JsonElement>>(ackJson);
                if (ack != null && ack.TryGetValue("status", out JsonElement statusElem))
                {
                    string status = statusElem.GetString();
                    if (status == "ok")
                        return (true, "Áp dụng thành công");
                    else
                        return (false, "Node báo lỗi: " + ackJson);
                }
                return (false, "Phản hồi không hợp lệ: " + ackJson);
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Lỗi gửi lệnh tới node {NodeType}", nodeType);
                return (false, "Lỗi: " + ex.Message);
            }
        }

        public async Task<List<ProcessInfo>> GetProcessListFromOsAgentAsync()
        {
            var config = await GetSystemConfigAsync();
            if (config == null)
                return new List<ProcessInfo>();

            var payload = new
            {
                CommandType = "GetProcessList",
                AuthToken = config.AuthToken
            };
            string json = JsonSerializer.Serialize(payload);

            try
            {
                using var client = new TcpClient();
                await client.ConnectAsync(config.OSAgentIP, config.OSAgentPort);
                var stream = client.GetStream();
                byte[] data = Encoding.UTF8.GetBytes(json + "\n");
                await stream.WriteAsync(data);
                await stream.FlushAsync();

                var buffer = new byte[8192];
                int bytesRead = await stream.ReadAsync(buffer, 0, buffer.Length);
                string response = Encoding.UTF8.GetString(buffer, 0, bytesRead).Trim();

                var result = JsonSerializer.Deserialize<ProcessListResponse>(response, new JsonSerializerOptions { PropertyNameCaseInsensitive = true });
                return result?.Processes ?? new List<ProcessInfo>();
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Lỗi lấy danh sách tiến trình từ OS Agent");
                return new List<ProcessInfo>();
            }
        }
    }

    public class ProcessListResponse
    {
        public string Status { get; set; }
        public List<ProcessInfo> Processes { get; set; }
    }

    public class ProcessInfo
    {
        public string Name { get; set; }
        public int Pid { get; set; }
        public double RamMB { get; set; }
    }
}