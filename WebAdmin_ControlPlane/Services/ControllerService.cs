using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using Microsoft.EntityFrameworkCore;
using WebAdmin_ControlPlane.Data;
using WebAdmin_ControlPlane.Models;
using System.Text.Json.Nodes;
using WebAdmin_ControlPlane.Utils;

namespace WebAdmin_ControlPlane.Services
{
    public class ControllerService
    {
        private readonly IServiceScopeFactory _scopeFactory;

        public ControllerService(IServiceScopeFactory scopeFactory)
        {
            _scopeFactory = scopeFactory;
        }

        private async Task<string> SendTcpCommandAsync(object payload)
        {
            using var scope = _scopeFactory.CreateScope();
            var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();
            var config = await db.SystemConfigs.FirstOrDefaultAsync();

            if (config == null) throw new Exception("Chưa có cấu hình Controller. Vui lòng vào Cấu hình hệ thống.");

            //string json = JsonSerializer.Serialize(payload);
            //claude fix nhu sau: 
            var obj = JsonSerializer.SerializeToNode(payload)!.AsObject();
            obj.Remove("AuthToken");   // không gửi token thô nữa, đã có chữ ký
            obj["Timestamp"] = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
            string payloadString = obj.ToJsonString();
            string signature = HmacHelper.ComputeHmacSha256(payloadString, config.ControllerToken);
            string json = JsonSerializer.Serialize(new { Payload = payloadString, Signature = signature });
            //
            try
            {
                using var client = new TcpClient();
                using var cts = new CancellationTokenSource(TimeSpan.FromSeconds(5));
                await client.ConnectAsync(config.ControllerIP, config.ControllerPort, cts.Token);

                var stream = client.GetStream();

                // Gửi dữ liệu (kèm \n để Controller biết kết thúc lệnh)
                byte[] data = Encoding.UTF8.GetBytes(json + "\n");
                await stream.WriteAsync(data, 0, data.Length);
                await stream.FlushAsync();

                // Đọc phản hồi
                using var reader = new StreamReader(stream, Encoding.UTF8);
                string response = await reader.ReadLineAsync();

                if (string.IsNullOrEmpty(response))
                    throw new Exception("Controller không phản hồi.");

                return response;
            }
            catch (Exception ex)
            {
                throw new Exception("Lỗi kết nối Controller: " + ex.Message);
            }
        }

        public async Task<ControllerResponse> CreateJobAsync(ResourcePolicy policy)
        {
            using var scope = _scopeFactory.CreateScope();
            var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();
            var config = await db.SystemConfigs.FirstOrDefaultAsync();

            if (config == null) throw new Exception("Chưa có cấu hình hệ thống.");

            var payload = new
            {
                CommandType = "CreateJob",
                JobType = policy.TargetProcess, // Tên app
                TargetUser = "admin",           // Có thể sửa thành user thực tế
                MemoryMB = policy.MaxRamMB ?? 100,
                Params = new string[] { },      // Mảng tham số nếu cần
                AuthToken = config.ControllerToken
            };

            string jsonResponse = await SendTcpCommandAsync(payload);
            var options = new JsonSerializerOptions { PropertyNameCaseInsensitive = true };
            return JsonSerializer.Deserialize<ControllerResponse>(jsonResponse, options);
        }

        public async Task<bool> CancelJobAsync(string jobId)
        {
            using var scope = _scopeFactory.CreateScope();
            var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();
            var config = await db.SystemConfigs.FirstOrDefaultAsync();

            if (config == null) throw new Exception("Chưa có cấu hình hệ thống.");

            var payload = new
            {
                CommandType = "CancelJob",
                JobId = jobId,
                AuthToken = config.ControllerToken
            };

            string jsonResponse = await SendTcpCommandAsync(payload);
            var options = new JsonSerializerOptions { PropertyNameCaseInsensitive = true };
            var res = JsonSerializer.Deserialize<ControllerResponse>(jsonResponse, options);
            return res != null && res.Status == "ok";
        }

        public async Task<List<ControllerServerInfo>> GetServersAsync()
        {
            using var scope = _scopeFactory.CreateScope();
            var db = scope.ServiceProvider.GetRequiredService<AppDbContext>();
            var config = await db.SystemConfigs.FirstOrDefaultAsync();

            if (config == null) throw new Exception("Chưa có cấu hình hệ thống.");

            var payload = new
            {
                CommandType = "GetServers",
                AuthToken = config.ControllerToken
            };

            string jsonResponse = await SendTcpCommandAsync(payload);
            var options = new JsonSerializerOptions { PropertyNameCaseInsensitive = true };
            var res = JsonSerializer.Deserialize<ControllerResponse>(jsonResponse, options);

            return res?.Servers ?? new List<ControllerServerInfo>();
        }

        // Hoi Controller danh sach tien trinh cua 1 server (Controller da keo san tu Agent)
        public async Task<List<ProcessInfo>> GetProcessesAsync(string nodeId)
        {
            var payload = new
            {
                CommandType = "GetProcesses",
                NodeId = nodeId
            };

            string jsonResponse = await SendTcpCommandAsync(payload);
            var options = new JsonSerializerOptions { PropertyNameCaseInsensitive = true };
            var res = JsonSerializer.Deserialize<ProcessListResponse>(jsonResponse, options);

            if (res == null || res.Status != "ok")
                throw new Exception(res?.Message ?? "Controller không trả về danh sách tiến trình.");

            return res.Processes ?? new List<ProcessInfo>();
        }
    }
}