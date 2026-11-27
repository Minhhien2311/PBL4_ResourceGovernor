using System;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.DependencyInjection;
using WebAdmin_ControlPlane.Data;
using WebAdmin_ControlPlane.Models;
using WebAdmin_ControlPlane.Utils;

namespace WebAdmin_ControlPlane.Services
{
    public class AgentTcpService
    {
        private readonly IServiceScopeFactory _scopeFactory;

        public AgentTcpService(IServiceScopeFactory scopeFactory)
        {
            _scopeFactory = scopeFactory;
        }

        private async Task<string> SendTcpToControllerAsync(object finalPacket)
        {
            using var scope = _scopeFactory.CreateScope();
            var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();

            var config = await db.SystemConfigs.FirstOrDefaultAsync();
            if (config == null) throw new Exception("Chưa cấu hình Controller trong hệ thống.");

            using var client = new TcpClient();
            using var cts = new CancellationTokenSource(TimeSpan.FromSeconds(10)); // Tăng lên 10s vì phải chờ Agent trả file

            try
            {
                await client.ConnectAsync(config.ControllerIP, config.ControllerPort, cts.Token);
                var stream = client.GetStream();

                string jsonSend = JsonSerializer.Serialize(finalPacket, new JsonSerializerOptions { WriteIndented = false });
                byte[] data = Encoding.UTF8.GetBytes(jsonSend + "\n");

                await stream.WriteAsync(data, 0, data.Length);
                await stream.FlushAsync();

                using var reader = new StreamReader(stream, Encoding.UTF8);
                return await reader.ReadLineAsync() ?? "";
            }
            catch (Exception ex)
            {
                throw new Exception("Lỗi kết nối Controller: " + ex.Message);
            }
        }

        public async Task<(bool success, string message)> UpdatePolicyAsync(ResourcePolicy policy)
        {
            if (string.IsNullOrEmpty(policy.NodeId)) return (false, "Chưa chọn Server.");

            using var scope = _scopeFactory.CreateScope();
            var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();
            var config = await db.SystemConfigs.FirstOrDefaultAsync();
            if (config == null) return (false, "Không tìm thấy cấu hình Controller.");

            try
            {
                long timestamp = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
                var payloadObj = new
                {
                    CommandType = "SetPolicy", // Lệnh gửi cho Controller
                    NodeId = policy.NodeId,
                    TargetUser = policy.TargetUser ?? "",
                    TargetProcess = policy.TargetProcess ?? "",
                    MaxRamMB = policy.MaxRamMB ?? 100,
                    MaxCpuPercent = policy.MaxCpuPercent ?? 0,
                    ReclaimRam = policy.ReclaimRam,
                    Timestamp = timestamp
                };

                string payloadString = JsonSerializer.Serialize(payloadObj, new JsonSerializerOptions { WriteIndented = false });
                string signature = HmacHelper.ComputeHmacSha256(payloadString, config.ControllerToken);
                var finalPacket = new { Payload = payloadString, Signature = signature };

                string res = await SendTcpToControllerAsync(finalPacket);
                return (res.Contains("\"ok\""), res);
            }
            catch (Exception ex)
            {
                return (false, ex.Message);
            }
        }

        public async Task<(bool success, string message)> RemovePolicyAsync(string nodeId, string targetProcess, string targetUser)
        {
            if (string.IsNullOrEmpty(nodeId)) return (false, "Chưa chọn Server.");

            using var scope = _scopeFactory.CreateScope();
            var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();
            var config = await db.SystemConfigs.FirstOrDefaultAsync();
            if (config == null) return (false, "Không tìm thấy cấu hình Controller.");

            try
            {
                long timestamp = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
                var payloadObj = new
                {
                    CommandType = "RemoveOsPolicy",
                    NodeId = nodeId,
                    TargetProcess = targetProcess ?? "",
                    TargetUser = targetUser ?? "",
                    Timestamp = timestamp
                };

                string payloadString = JsonSerializer.Serialize(payloadObj, new JsonSerializerOptions { WriteIndented = false });
                string signature = HmacHelper.ComputeHmacSha256(payloadString, config.ControllerToken);
                var finalPacket = new { Payload = payloadString, Signature = signature };

                string res = await SendTcpToControllerAsync(finalPacket);
                return (res.Contains("\"ok\""), res);
            }
            catch (Exception ex)
            {
                return (false, ex.Message);
            }
        }

        public async Task<string> GetJobOutputAsync(string jobId, string filename = "result.json")
        {
            using var scope = _scopeFactory.CreateScope();
            var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();
            var config = await db.SystemConfigs.FirstOrDefaultAsync();
            if (config == null) throw new Exception("Không tìm thấy cấu hình Controller.");

            long timestamp = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
            var payloadObj = new
            {
                CommandType = "GetJobOutput",
                JobId = jobId,
                Filename = filename,
                Timestamp = timestamp
            };

            string payloadString = JsonSerializer.Serialize(payloadObj, new JsonSerializerOptions { WriteIndented = false });
            string signature = HmacHelper.ComputeHmacSha256(payloadString, config.ControllerToken);
            var finalPacket = new { Payload = payloadString, Signature = signature };

            string res = await SendTcpToControllerAsync(finalPacket);

            // Parse JSON trả về
            using var doc = JsonDocument.Parse(res);
            if (doc.RootElement.TryGetProperty("status", out var status) && status.GetString() == "ok")
            {
                if (doc.RootElement.TryGetProperty("ContentBase64", out var data))
                {
                    return data.GetString(); // Trả về chuỗi Base64
                }
            }
            throw new Exception("Lỗi từ Controller: " + res);
        }

        public async Task<(bool success, string message)> UpdateNetworkPolicyAsync(ResourcePolicy policy)
        {
            if (string.IsNullOrEmpty(policy.NodeId)) return (false, "Chưa chọn Server.");

            using var scope = _scopeFactory.CreateScope();
            var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();
            var config = await db.SystemConfigs.FirstOrDefaultAsync();
            if (config == null) return (false, "Không tìm thấy cấu hình Controller.");

            try
            {
                long timestamp = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
                var payloadObj = new
                {
                    CommandType = "SetNetworkPolicy", // Lệnh gửi cho Controller
                    NodeId = policy.NodeId,
                    TargetPort = policy.TargetPort ?? 80,
                    MaxBandwidthKbps = policy.MaxBandwidthKbps ?? 1024,
                    ApplyToIp = policy.ApplyToIp ?? "All",
                    Timestamp = timestamp
                };

                string payloadString = JsonSerializer.Serialize(payloadObj, new JsonSerializerOptions { WriteIndented = false });
                string signature = HmacHelper.ComputeHmacSha256(payloadString, config.ControllerToken);
                var finalPacket = new { Payload = payloadString, Signature = signature };

                string res = await SendTcpToControllerAsync(finalPacket); // Hàm gửi TCP đến 9100
                return (res.Contains("\"ok\""), res);
            }
            catch (Exception ex)
            {
                return (false, ex.Message);
            }
        }
    }
}