using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.Rendering;
using Microsoft.EntityFrameworkCore;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using WebAdmin_ControlPlane.Data;
using WebAdmin_ControlPlane.Models;
using WebAdmin_ControlPlane.Services;

namespace WebAdmin_ControlPlane.Controllers
{
    [Authorize]
    public class ResourcePoliciesController : Controller
    {
        private readonly AppDbContext _context;
        private readonly TcpCommandService _tcpCommandService;

        public ResourcePoliciesController(AppDbContext context, TcpCommandService tcpCommandService)
        {
            _context = context;
            _tcpCommandService = tcpCommandService;
        }

        // GET: ResourcePolicies
        public async Task<IActionResult> Index()
        {
            return View(await _context.ResourcePolicies.ToListAsync());
        }

        // GET: ResourcePolicies/Details/5
        public async Task<IActionResult> Details(int? id)
        {
            if (id == null)
            {
                return NotFound();
            }

            var resourcePolicy = await _context.ResourcePolicies
                .FirstOrDefaultAsync(m => m.Id == id);
            if (resourcePolicy == null)
            {
                return NotFound();
            }

            return View(resourcePolicy);
        }

        // GET: ResourcePolicies/Create
        public IActionResult Create()
        {
            return View();
        }

        // POST: ResourcePolicies/Create
        [HttpPost]
        [ValidateAntiForgeryToken]
        public async Task<IActionResult> Create([Bind("Id,Type,IsActive,TargetProcess,MaxRamMB,OsAction,TargetPort,MaxBandwidthKbps,ApplyToIp")] ResourcePolicy resourcePolicy)
        {
            if (ModelState.IsValid)
            {
                _context.Add(resourcePolicy);
                await _context.SaveChangesAsync();

                var (success, message) = await _tcpCommandService.SendPolicyToNodeAsync(resourcePolicy);
                if (success)
                    TempData["Success"] = "Lưu và áp dụng thành công: " + message;
                else
                    TempData["Error"] = "Lưu thành công nhưng gửi lệnh thất bại: " + message;

                return RedirectToAction(nameof(Index));
            }
            return View(resourcePolicy);
        }

        // GET: ResourcePolicies/Edit/5
        public async Task<IActionResult> Edit(int? id)
        {
            if (id == null)
            {
                return NotFound();
            }

            var resourcePolicy = await _context.ResourcePolicies.FindAsync(id);
            if (resourcePolicy == null)
            {
                return NotFound();
            }
            return View(resourcePolicy);
        }

        // POST: ResourcePolicies/Edit/5
        [HttpPost]
        [ValidateAntiForgeryToken]
        public async Task<IActionResult> Edit(int id, [Bind("Id,Type,IsActive,TargetProcess,MaxRamMB,OsAction,TargetPort,MaxBandwidthKbps,ApplyToIp")] ResourcePolicy resourcePolicy)
        {
            if (id != resourcePolicy.Id)
            {
                return NotFound();
            }

            if (ModelState.IsValid)
            {
                try
                {
                    _context.Update(resourcePolicy);
                    await _context.SaveChangesAsync();

                    var (success, message) = await _tcpCommandService.SendPolicyToNodeAsync(resourcePolicy);
                    if (success)
                        TempData["Success"] = "Cập nhật và áp dụng thành công: " + message;
                    else
                        TempData["Error"] = "Cập nhật thành công nhưng gửi lệnh thất bại: " + message;
                }
                catch (DbUpdateConcurrencyException)
                {
                    if (!ResourcePolicyExists(resourcePolicy.Id))
                    {
                        return NotFound();
                    }
                    else
                    {
                        throw;
                    }
                }
                return RedirectToAction(nameof(Index));
            }
            return View(resourcePolicy);
        }

        // GET: ResourcePolicies/Delete/5
        public async Task<IActionResult> Delete(int? id)
        {
            if (id == null)
            {
                return NotFound();
            }

            var resourcePolicy = await _context.ResourcePolicies
                .FirstOrDefaultAsync(m => m.Id == id);
            if (resourcePolicy == null)
            {
                return NotFound();
            }

            return View(resourcePolicy);
        }

        // POST: ResourcePolicies/Delete/5
        [HttpPost, ActionName("Delete")]
        [ValidateAntiForgeryToken]
        public async Task<IActionResult> DeleteConfirmed(int id)
        {
            var resourcePolicy = await _context.ResourcePolicies.FindAsync(id);
            if (resourcePolicy != null)
            {
                _context.ResourcePolicies.Remove(resourcePolicy);
            }

            await _context.SaveChangesAsync();
            return RedirectToAction(nameof(Index));
        }

        // Action trả về danh sách tiến trình
        [HttpGet]
        public async Task<JsonResult> GetProcessList()
        {
            var processes = await _tcpCommandService.GetProcessListFromOsAgentAsync();
            return Json(processes);
        }

        private bool ResourcePolicyExists(int id)
        {
            return _context.ResourcePolicies.Any(e => e.Id == id);
        }
    }
}