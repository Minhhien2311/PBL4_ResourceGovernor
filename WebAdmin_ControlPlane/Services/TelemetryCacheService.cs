using System.Collections.Concurrent;
using WebAdmin_ControlPlane.Models;

namespace WebAdmin_ControlPlane.Services
{
    public class TelemetryCacheService
    {
        // Lưu danh sách process theo từng NodeId
        public ConcurrentDictionary<string, List<ProcessInfo>> NodeProcesses { get; } = new();

        // Lưu trạng thái mới nhất của từng Node
        public ConcurrentDictionary<string, HeartbeatPayload> LatestHeartbeats { get; } = new();

        // Lưu thời điểm cuối cùng nhận được Heartbeat
        public ConcurrentDictionary<string, DateTime> LastHeartbeatTimes { get; } = new();
    }
}