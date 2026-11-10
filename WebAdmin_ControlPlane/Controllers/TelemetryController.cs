using Microsoft.AspNetCore.Mvc;
using WebAdmin_ControlPlane.Models;
using WebAdmin_ControlPlane.Services;

namespace WebAdmin_ControlPlane.Controllers
{
    [Route("api/[controller]")]
    [ApiController]
    public class TelemetryController : ControllerBase
    {
        private readonly TelemetryCacheService _cache;

        public TelemetryController(TelemetryCacheService cache)
        {
            _cache = cache;
        }

        [HttpPost("heartbeat")]
        public IActionResult ReceiveHeartbeat([FromBody] HeartbeatPayload payload)
        {
            if (payload == null || string.IsNullOrEmpty(payload.NodeId))
                return BadRequest();

            // Lưu trạng thái, process và thời gian
            _cache.LatestHeartbeats[payload.NodeId] = payload;
            _cache.NodeProcesses[payload.NodeId] = payload.Processes;
            _cache.LastHeartbeatTimes[payload.NodeId] = DateTime.Now; // Lưu thời điểm

            return Ok(new { status = "ok" });
        }
    }
}