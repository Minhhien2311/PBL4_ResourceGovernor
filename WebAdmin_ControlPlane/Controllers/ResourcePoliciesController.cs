using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.Rendering;
using Microsoft.EntityFrameworkCore;
using WebAdmin_ControlPlane.Data;
using WebAdmin_ControlPlane.Models;
using WebAdmin_ControlPlane.Services;

namespace WebAdmin_ControlPlane.Controllers
{
    [Authorize]
    public class ResourcePoliciesController : Controller
    {
        private readonly AppDbContext _context;
        private readonly AgentTcpService _agentService;
        private readonly TelemetryCacheService _cache;

        public ResourcePoliciesController(AppDbContext context, AgentTcpService agentService, TelemetryCacheService cache)
        {
            _context = context;
            _agentService = agentService;
            _cache = cache;
        }

        public async Task<IActionResult> Index() => View(await _context.ResourcePolicies.ToListAsync());

        public IActionResult Create()
        {
            ViewBag.Servers = new SelectList(_context.ServerNodes.ToList(), "NodeId", "NodeId");
            return View();
        }

        [HttpPost]
        [ValidateAntiForgeryToken]
        public async Task<IActionResult> Create([Bind("Id,NodeId,TargetProcess,TargetUser,MaxRamMB,OsAction")] ResourcePolicy policy)
        {
            if (ModelState.IsValid)
            {
                _context.Add(policy);
                await _context.SaveChangesAsync();

                var result = await _agentService.UpdatePolicyAsync(policy);

                // GHI NHẬT KÝ
                _context.EventLogs.Add(new EventLog
                {
                    NodeId = policy.NodeId,
                    Action = "Tạo mới",
                    TargetProcess = policy.TargetProcess,
                    Message = result.success ? "Thành công" : "Thất bại: " + result.message
                });
                await _context.SaveChangesAsync();

                TempData[result.success ? "Success" : "Error"] = result.success ? "Đã gửi Agent thành công" : result.message;

                return RedirectToAction(nameof(Index));
            }
            ViewBag.Servers = new SelectList(_context.ServerNodes.ToList(), "NodeId", "NodeId", policy.NodeId);
            return View(policy);
        }

        public async Task<IActionResult> Edit(int? id)
        {
            if (id == null) return NotFound();
            var policy = await _context.ResourcePolicies.FindAsync(id);
            if (policy == null) return NotFound();
            ViewBag.Servers = new SelectList(_context.ServerNodes.ToList(), "NodeId", "NodeId", policy.NodeId);
            return View(policy);
        }

        [HttpPost]
        [ValidateAntiForgeryToken]
        public async Task<IActionResult> Edit(int id, [Bind("Id,NodeId,TargetProcess,TargetUser,MaxRamMB,OsAction")] ResourcePolicy policy)
        {
            if (id != policy.Id) return NotFound();

            if (ModelState.IsValid)
            {
                try
                {
                    _context.Update(policy);
                    await _context.SaveChangesAsync();

                    var result = await _agentService.UpdatePolicyAsync(policy);

                    // GHI NHẬT KÝ
                    _context.EventLogs.Add(new EventLog
                    {
                        NodeId = policy.NodeId,
                        Action = "Cập nhật",
                        TargetProcess = policy.TargetProcess,
                        Message = result.success ? "Thành công" : "Thất bại: " + result.message
                    });
                    await _context.SaveChangesAsync();

                    TempData[result.success ? "Success" : "Error"] = result.success ? "Cập nhật và gửi Agent thành công" : result.message;
                }
                catch (DbUpdateConcurrencyException)
                {
                    if (!_context.ResourcePolicies.Any(e => e.Id == policy.Id)) return NotFound();
                    else throw;
                }
                return RedirectToAction(nameof(Index));
            }
            ViewBag.Servers = new SelectList(_context.ServerNodes.ToList(), "NodeId", "NodeId", policy.NodeId);
            return View(policy);
        }

        public async Task<IActionResult> Delete(int? id)
        {
            if (id == null) return NotFound();
            var policy = await _context.ResourcePolicies.FirstOrDefaultAsync(m => m.Id == id);
            if (policy == null) return NotFound();
            return View(policy);
        }

        [HttpPost, ActionName("Delete")]
        [ValidateAntiForgeryToken]
        public async Task<IActionResult> DeleteConfirmed(int id)
        {
            var policy = await _context.ResourcePolicies.FindAsync(id);
            if (policy != null)
            {
                var result = await _agentService.RemovePolicyAsync(policy.NodeId, policy.TargetProcess, policy.TargetUser);

                _context.ResourcePolicies.Remove(policy);
                await _context.SaveChangesAsync();

                // GHI NHẬT KÝ
                _context.EventLogs.Add(new EventLog
                {
                    NodeId = policy.NodeId,
                    Action = "Xóa",
                    TargetProcess = policy.TargetProcess,
                    Message = result.success ? "Thành công" : "Thất bại: " + result.message
                });
                await _context.SaveChangesAsync();

                TempData[result.success ? "Success" : "Error"] = result.success ? "Đã xóa và gỡ luật khỏi Agent" : result.message;
            }
            return RedirectToAction(nameof(Index));
        }

        [HttpGet]
        public IActionResult GetProcessList(string nodeId)
        {
            if (string.IsNullOrEmpty(nodeId)) return Json(new List<ProcessInfo>());
            if (_cache.NodeProcesses.TryGetValue(nodeId, out var processes))
                return Json(processes.OrderBy(p => p.Name).ToList());
            return Json(new List<ProcessInfo>());
        }
    }
}