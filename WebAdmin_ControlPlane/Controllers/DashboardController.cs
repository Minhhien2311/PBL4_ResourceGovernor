using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using WebAdmin_ControlPlane.Data;
using WebAdmin_ControlPlane.Services;

namespace WebAdmin_ControlPlane.Controllers
{
    [Authorize]
    public class DashboardController : Controller
    {
        private readonly AppDbContext _context;
        private readonly TelemetryCacheService _cache;

        public DashboardController(AppDbContext context, TelemetryCacheService cache)
        {
            _context = context;
            _cache = cache;
        }

        public async Task<IActionResult> Index()
        {
            // Lấy danh sách server đã cấu hình trong DB
            var nodes = await _context.ServerNodes.ToListAsync();

            // Truyền Cache sang View để kiểm tra trạng thái
            ViewBag.Cache = _cache;
            return View(nodes);
        }
    }
}