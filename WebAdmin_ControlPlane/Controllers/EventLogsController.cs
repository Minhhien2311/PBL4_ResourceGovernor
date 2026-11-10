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

        public async Task<IActionResult> Index()
        {
            // Lấy 200 bản ghi mới nhất
            var logs = await _context.EventLogs
                .OrderByDescending(l => l.Timestamp)
                .Take(200)
                .ToListAsync();
            return View(logs);
        }
    }
}