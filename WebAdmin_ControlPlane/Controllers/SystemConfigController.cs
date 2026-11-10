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

        // 1. HIỂN THỊ DANH SÁCH
        public async Task<IActionResult> Index()
        {
            return View(await _context.ServerNodes.ToListAsync());
        }

        // 2. THÊM SERVER MỚI (POST)
        [HttpPost]
        [ValidateAntiForgeryToken]
        public async Task<IActionResult> AddServer(ServerNode node)
        {
            // Kiểm tra trùng NodeId (Khóa chính)
            if (_context.ServerNodes.Any(n => n.NodeId == node.NodeId))
            {
                ModelState.AddModelError("NodeId", "Tên Server (Node ID) này đã tồn tại. Vui lòng chọn tên khác!");
            }

            if (ModelState.IsValid)
            {
                _context.ServerNodes.Add(node);
                await _context.SaveChangesAsync();
                TempData["Success"] = "Đã thêm Server " + node.NodeId + " thành công!";
                return RedirectToAction(nameof(Index));
            }

            // Nếu có lỗi (trùng tên hoặc nhập thiếu), load lại trang Index kèm dữ liệu
            return View("Index", await _context.ServerNodes.ToListAsync());
        }

        // 3. SỬA SERVER (GET)
        public async Task<IActionResult> EditServer(string id)
        {
            if (id == null) return NotFound();

            var node = await _context.ServerNodes.FindAsync(id);
            if (node == null) return NotFound();

            return View(node);
        }

        // 4. SỬA SERVER (POST)
        [HttpPost]
        [ValidateAntiForgeryToken]
        public async Task<IActionResult> EditServer(string id, ServerNode model)
        {
            if (id != model.NodeId) return NotFound();

            if (ModelState.IsValid)
            {
                try
                {
                    // Chỉ cập nhật IP, Port, Token. Không cập nhật NodeId (Khóa chính).
                    var node = await _context.ServerNodes.FindAsync(id);
                    if (node == null) return NotFound();

                    node.IpAddress = model.IpAddress;
                    node.Port = model.Port;
                    node.AuthToken = model.AuthToken;

                    _context.Update(node);
                    await _context.SaveChangesAsync();

                    TempData["Success"] = "Đã cập nhật thông tin Server " + id + " thành công!";
                }
                catch (DbUpdateConcurrencyException)
                {
                    if (!_context.ServerNodes.Any(e => e.NodeId == model.NodeId)) return NotFound();
                    else throw;
                }
                return RedirectToAction(nameof(Index));
            }
            return View(model);
        }
    }
}