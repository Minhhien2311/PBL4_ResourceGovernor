using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using WebAdmin_ControlPlane.Data;
using WebAdmin_ControlPlane.Models;

namespace WebAdmin_ControlPlane.Controllers
{
    [Authorize]
    public class SystemConfigController : Controller
    {
        private readonly AppDbContext _context;

        public SystemConfigController(AppDbContext context)
        {
            _context = context;
        }

        // GET: SystemConfig
        public async Task<IActionResult> Index()
        {
            var config = await _context.SystemConfigs.FirstOrDefaultAsync();
            if (config == null)
            {
                // Tạo cấu hình mặc định nếu chưa có
                config = new SystemConfig();
                _context.SystemConfigs.Add(config);
                await _context.SaveChangesAsync();
            }
            return View(config);
        }

        // POST: SystemConfig/Edit
        [HttpPost]
        [ValidateAntiForgeryToken]
        public async Task<IActionResult> Edit(SystemConfig model)
        {
            if (!ModelState.IsValid)
                return View("Index", model);

            var config = await _context.SystemConfigs.FirstOrDefaultAsync();
            if (config == null)
            {
                config = new SystemConfig();
                _context.SystemConfigs.Add(config);
            }

            config.AuthToken = model.AuthToken;
            config.OSAgentIP = model.OSAgentIP;
            config.OSAgentPort = model.OSAgentPort;
            config.ProxyIP = model.ProxyIP;
            config.ProxyPort = model.ProxyPort;
            config.HeartbeatIntervalSeconds = model.HeartbeatIntervalSeconds;

            await _context.SaveChangesAsync();
            TempData["Success"] = "Đã lưu cấu hình hệ thống.";
            return RedirectToAction(nameof(Index));
        }
    }
}