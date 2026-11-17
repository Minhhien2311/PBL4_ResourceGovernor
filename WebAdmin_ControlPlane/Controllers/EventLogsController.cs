using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using WebAdmin_ControlPlane.Data;

namespace WebAdmin_ControlPlane.Controllers
{
    [Authorize]
    public class EventLogsController : Controller
    {
        private readonly AppDbContext _context;

        public EventLogsController(AppDbContext context)
        {
            _context = context;
        }

        // GET: EventLogs
        public async Task<IActionResult> Index()
        {
            var logs = await _context.EventLogs
                .OrderByDescending(l => l.Timestamp)
                .Take(200) // Giới hạn 200 bản ghi gần nhất để tránh quá tải
                .ToListAsync();
            return View(logs);
        }
    }
}