using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using WebAdmin_ControlPlane.Services;
using WebAdmin_ControlPlane.Models;

namespace WebAdmin_ControlPlane.Controllers
{
    [Authorize]
    public class DashboardController : Controller
    {
        private readonly ControllerService _controllerService;

        public DashboardController(ControllerService controllerService)
        {
            _controllerService = controllerService;
        }

        public async Task<IActionResult> Index()
        {
            List<ControllerServerInfo> servers = new List<ControllerServerInfo>();
            ViewBag.Error = null;

            try
            {
                // Lấy danh sách server trực tiếp từ Controller
                servers = await _controllerService.GetServersAsync();
            }
            catch (Exception ex)
            {
                ViewBag.Error = "Không thể lấy dữ liệu từ Controller: " + ex.Message;
            }

            return View(servers);
        }
    }
}