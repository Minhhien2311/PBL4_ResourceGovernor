using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using WebAdmin_ControlPlane.Data;
using WebAdmin_ControlPlane.Models;
using WebAdmin_ControlPlane.Services;

namespace WebAdmin_ControlPlane.Controllers
{
    [Authorize]
    public class SystemConfigController : Controller
    {
        private readonly AppDbContext _context;
        private readonly ControllerService _controllerService;

        public SystemConfigController(AppDbContext context, ControllerService controllerService)
        {
            _context = context;
            _controllerService = controllerService;
        }

        // Trang hiển thị: Vừa cấu hình Controller, vừa xem danh sách Server
        public async Task<IActionResult> Index()
        {
            // 1. Lấy cấu hình Controller
            var config = await _context.SystemConfigs.FirstOrDefaultAsync();
            if (config == null)
            {
                config = new SystemConfig();
                _context.SystemConfigs.Add(config);
                await _context.SaveChangesAsync();
            }

            // 2. Gọi Controller lấy danh sách Server đang online (Read-only)
            List<ControllerServerInfo> servers = new List<ControllerServerInfo>();
            try
            {
                servers = await _controllerService.GetServersAsync();
            }
            catch (Exception ex)
            {
                TempData["Error"] = "Không thể lấy danh sách Server từ Controller: " + ex.Message;
            }

            ViewBag.Servers = servers;
            return View(config);
        }

        // Cập nhật cấu hình Controller
        [HttpPost]
        [ValidateAntiForgeryToken]
        public async Task<IActionResult> SaveConfig(SystemConfig model)
        {
            if (!ModelState.IsValid)
            {
                TempData["Error"] = "Dữ liệu không hợp lệ.";
                return RedirectToAction(nameof(Index));
            }

            var config = await _context.SystemConfigs.FirstOrDefaultAsync();
            if (config == null)
            {
                config = new SystemConfig();
                _context.SystemConfigs.Add(config);
            }

            config.ControllerIP = model.ControllerIP;
            config.ControllerPort = model.ControllerPort;
            config.ControllerToken = model.ControllerToken;

            await _context.SaveChangesAsync();
            TempData["Success"] = "Đã lưu cấu hình kết nối Controller!";
            return RedirectToAction(nameof(Index));
        }
    }
}