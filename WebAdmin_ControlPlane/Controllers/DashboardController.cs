using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using System.Text.Json;
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
            var nodes = await GetNodesWithUpdatedStatusAsync();
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
            return Json(nodes, new JsonSerializerOptions { PropertyNamingPolicy = JsonNamingPolicy.CamelCase });
        }

        [HttpGet]
        public async Task<IActionResult> GetDashboardStats()
        {
            var nodes = await GetNodesWithUpdatedStatusAsync();

            var stats = new
            {
                onlineNodes = nodes.Count(n => n.Status == "Running"),
                totalPolicies = await _context.ResourcePolicies.CountAsync(),
                todayEvents = await _context.EventLogs
                    .Where(l => l.Timestamp.Date == DateTime.Today)
                    .CountAsync(),
                offlineNodes = nodes.Count(n => n.Status != "Running")
            };

            return Json(stats, new JsonSerializerOptions { PropertyNamingPolicy = JsonNamingPolicy.CamelCase });
        }

        private async Task<List<NodeStatus>> GetNodesWithUpdatedStatusAsync()
        {
            var threshold = TimeSpan.FromSeconds(15);
            var now = DateTime.Now;
            var nodes = await _context.NodeStatuses.ToListAsync();

            foreach (var node in nodes)
            {
                node.Status = (now - node.LastHeartbeat > threshold) ? "Offline" : "Running";
            }

            return nodes;
        }
    }
}