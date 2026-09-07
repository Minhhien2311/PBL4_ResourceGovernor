using Microsoft.AspNetCore.Mvc;
using WebAdmin_ControlPlane.Data;
using WebAdmin_ControlPlane.Models;

namespace WebAdmin_ControlPlane.Controllers
{
    [Route("api/[controller]")]
    [ApiController]
    public class TelemetryController : ControllerBase
    {
        private readonly AppDbContext _context;

        public TelemetryController(AppDbContext context)
        {
            _context = context;
        }

        [HttpPost("heartbeat")]
        public async Task<IActionResult> ReceiveHeartbeat([FromBody] NodeStatus status)
        {
            if (status == null)
                return BadRequest();

            // Tìm node theo NodeType (nếu có) hoặc thêm mới
            var existing = _context.NodeStatuses
                .FirstOrDefault(n => n.NodeType == status.NodeType);

            if (existing != null)
            {
                existing.Status = status.Status;
                existing.CPUUsagePercent = status.CPUUsagePercent;
                existing.TotalRAM = status.TotalRAM;
                existing.UsedRAM = status.UsedRAM;
                existing.ProcessCount = status.ProcessCount;
                existing.BandwidthInUse = status.BandwidthInUse;
                existing.ActiveConnections = status.ActiveConnections;
                existing.LastHeartbeat = DateTime.Now;
            }
            else
            {
                status.LastHeartbeat = DateTime.Now;
                _context.NodeStatuses.Add(status);
            }

            await _context.SaveChangesAsync();
            return Ok(new { status = "ok" });
        }

        [HttpPost("log")]
        public async Task<IActionResult> ReceiveLog([FromBody] EventLog log)
        {
            if (log == null)
                return BadRequest();

            log.Timestamp = DateTime.Now;
            _context.EventLogs.Add(log);
            await _context.SaveChangesAsync();

            return Ok(new { status = "ok" });
        }
    }
}