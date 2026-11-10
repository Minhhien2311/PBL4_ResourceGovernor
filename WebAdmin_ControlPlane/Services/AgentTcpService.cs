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

        // Hàm dùng chung để gửi TCP
        private async Task<string> SendTcpPacketAsync(string nodeId, object finalPacket)
        {
            using var scope = _scopeFactory.CreateScope();
            var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();
            var node = await db.ServerNodes.FirstOrDefaultAsync(n => n.NodeId == nodeId);
            if (node == null) throw new Exception("Không tìm thấy cấu hình Server trong Database.");

            using var client = new TcpClient();
            using var cts = new CancellationTokenSource(TimeSpan.FromSeconds(5));

            try
            {
                await client.ConnectAsync(node.IpAddress, node.Port, cts.Token);
                var stream = client.GetStream();

                // Serialize gói tin chứa Payload và Signature
                string jsonSend = JsonSerializer.Serialize(finalPacket, new JsonSerializerOptions { WriteIndented = false });
                byte[] data = Encoding.UTF8.GetBytes(jsonSend + "\n");

                await stream.WriteAsync(data, 0, data.Length);
                await stream.FlushAsync();

                using var reader = new StreamReader(stream, Encoding.UTF8);
                return await reader.ReadLineAsync() ?? "";
            }
            catch (Exception ex)
            {
                throw new Exception("Lỗi kết nối TCP: " + ex.Message);
            }
        }

        public async Task<(bool success, string message)> UpdatePolicyAsync(ResourcePolicy policy)
        {
            if (string.IsNullOrEmpty(policy.NodeId)) return (false, "Chưa chọn Server.");

            using var scope = _scopeFactory.CreateScope();
            var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();
            var node = await db.ServerNodes.FirstOrDefaultAsync(n => n.NodeId == policy.NodeId);
            if (node == null) return (false, "Không tìm thấy Server.");

            try
            {
                // 1. Tạo Payload Object
                long timestamp = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
                var payloadObj = new
                {
                    CommandType = "UpdateOsPolicy",
                    TargetUser = policy.TargetUser ?? "",
                    TargetProcess = policy.TargetProcess ?? "",
                    MaxRamMB = policy.MaxRamMB ?? 100,
                    Action = policy.OsAction ?? "Pause",
                    Timestamp = timestamp
                };

                // 2. Chuyển thành chuỗi String (Không có khoảng trắng thừa)
                string payloadString = JsonSerializer.Serialize(payloadObj, new JsonSerializerOptions { WriteIndented = false });

                // 3. Băm chuỗi bằng AuthToken
                string signature = HmacHelper.ComputeHmacSha256(payloadString, node.AuthToken);

                // 4. Đóng gói
                var finalPacket = new { Payload = payloadString, Signature = signature };

                // 5. Gửi
                string res = await SendTcpPacketAsync(policy.NodeId, finalPacket);
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
            var node = await db.ServerNodes.FirstOrDefaultAsync(n => n.NodeId == nodeId);
            if (node == null) return (false, "Không tìm thấy Server.");

            try
            {
                long timestamp = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
                var payloadObj = new
                {
                    CommandType = "RemoveOsPolicy",
                    TargetProcess = targetProcess ?? "",
                    TargetUser = targetUser ?? "",
                    Timestamp = timestamp
                };

                string payloadString = JsonSerializer.Serialize(payloadObj, new JsonSerializerOptions { WriteIndented = false });
                string signature = HmacHelper.ComputeHmacSha256(payloadString, node.AuthToken);

                var finalPacket = new { Payload = payloadString, Signature = signature };

                string res = await SendTcpPacketAsync(nodeId, finalPacket);
                return (res.Contains("\"ok\""), res);
            }
            catch (Exception ex)
            {
                return (false, ex.Message);
            }
        }
    }
}