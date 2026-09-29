using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using WebAdmin_ControlPlane.Data;
using WebAdmin_ControlPlane.Models;

namespace WebAdmin_ControlPlane.Controllers
{
    [Authorize]
    public class DashboardController : Controller
    {
        private readonly AppDbContext _context;

        public DashboardController(AppDbContext context)
        {
            _context = context;
        }

        public async Task<IActionResult> Index()
        {
            // Lấy trạng thái node và cập nhật offline nếu heartbeat quá hạn
            var nodes = await GetNodesWithUpdatedStatusAsync();

            // Thống kê
            ViewBag.TotalPolicies = await _context.ResourcePolicies.CountAsync();
            ViewBag.OnlineNodes = nodes.Count(n => n.Status == "Running");
            ViewBag.TodayEvents = await _context.EventLogs
                .Where(l => l.Timestamp.Date == DateTime.Today)
                .CountAsync();

            return View(nodes);
        }

        [HttpGet]
        public async Task<IActionResult> GetNodeStatus()
        {
            var nodes = await GetNodesWithUpdatedStatusAsync();
            return Json(nodes);
        }

        private async Task<List<NodeStatus>> GetNodesWithUpdatedStatusAsync()
        {
            var threshold = TimeSpan.FromSeconds(15); // ngưỡng coi là offline
            var now = DateTime.Now;
            var nodes = await _context.NodeStatuses.ToListAsync();

            foreach (var node in nodes)
            {
                if (now - node.LastHeartbeat > threshold)
                {
                    node.Status = "Offline"; // cập nhật trong memory, không lưu DB ngay
                }
                else
                {
                    node.Status = "Running";
                }
            }

            return nodes;
        }

        [HttpGet]
        public async Task<IActionResult> GetDashboardStats()
        {
            // Cập nhật trạng thái offline cho node trước khi thống kê
            var nodes = await GetNodesWithUpdatedStatusAsync(); // Dùng lại hàm đã có

            var stats = new
            {
                onlineNodes = nodes.Count(n => n.Status == "Running"),
                totalPolicies = await _context.ResourcePolicies.CountAsync(),
                todayEvents = await _context.EventLogs
                    .Where(l => l.Timestamp.Date == DateTime.Today)
                    .CountAsync(),
                offlineNodes = nodes.Count(n => n.Status != "Running")
            };

            return Json(stats);
        }
    }
}