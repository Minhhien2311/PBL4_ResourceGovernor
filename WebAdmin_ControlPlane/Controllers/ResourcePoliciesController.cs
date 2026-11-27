//using Microsoft.AspNetCore.Authorization;
//using Microsoft.AspNetCore.Mvc;
//using Microsoft.AspNetCore.Mvc.Rendering;
//using Microsoft.EntityFrameworkCore;
//using WebAdmin_ControlPlane.Data;
//using WebAdmin_ControlPlane.Models;
//using WebAdmin_ControlPlane.Services;

//namespace WebAdmin_ControlPlane.Controllers
//{
//    [Authorize]
//    public class ResourcePoliciesController : Controller
//    {
//        private readonly AppDbContext _context;
//        private readonly AgentTcpService _agentService;
//        private readonly ControllerService _controllerService;

//        public ResourcePoliciesController(AppDbContext context, AgentTcpService agentService, ControllerService controllerService)
//        {
//            _context = context;
//            _agentService = agentService;
//            _controllerService = controllerService;
//        }

//        // Danh sach server cho dropdown: lay tu Controller (servers.conf) de NodeId luon khop voi Controller
//        private async Task<SelectList> LoadServerListAsync(string? selectedNodeId = null)
//        {
//            try
//            {
//                var servers = await _controllerService.GetServersAsync();
//                var nodeIds = servers
//                    .Where(s => !string.IsNullOrEmpty(s.NodeId))
//                    .Select(s => s.NodeId!)
//                    .Distinct()
//                    .ToList();
//                return new SelectList(nodeIds, selectedNodeId);
//            }
//            catch (Exception ex)
//            {
//                ViewBag.ServerError = "Không lấy được danh sách Server từ Controller: " + ex.Message;
//                return new SelectList(new List<string>(), selectedNodeId);
//            }
//        }

//        public async Task<IActionResult> Index() => View(await _context.ResourcePolicies.ToListAsync());

//        public async Task<IActionResult> Create()
//        {
//            ViewBag.Servers = await LoadServerListAsync();
//            return View();
//        }

//        [HttpPost]
//        [ValidateAntiForgeryToken]
//        public async Task<IActionResult> Create([Bind("Id,NodeId,PolicyType,TargetProcess,TargetUser,MaxRamMB,MaxCpuPercent,ReclaimRam,OsAction,TargetPort,MaxBandwidthKbps,ApplyToIp")] ResourcePolicy policy)
//        {
//            if (ModelState.IsValid)
//            {
//                _context.Add(policy);
//                await _context.SaveChangesAsync();

//                // PHÂN NHÁNH GỬI LỆNH THEO LOẠI CHÍNH SÁCH
//                (bool success, string message) result;
//                if (policy.PolicyType == "NETWORK")
//                {
//                    result = await _agentService.UpdateNetworkPolicyAsync(policy);
//                }
//                else
//                {
//                    result = await _agentService.UpdatePolicyAsync(policy);
//                }

//                // Ghi log
//                _context.EventLogs.Add(new EventLog
//                {
//                    NodeId = policy.NodeId,
//                    Action = "Tạo mới " + policy.PolicyType,
//                    TargetProcess = policy.PolicyType == "NETWORK" ? $"Port {policy.TargetPort}" : policy.TargetProcess,
//                    Message = result.success ? "Thành công" : "Thất bại: " + result.message
//                });
//                await _context.SaveChangesAsync();

//                TempData[result.success ? "Success" : "Error"] = result.success ? "Đã gửi Agent thành công" : result.message;
//                return RedirectToAction(nameof(Index));
//            }
//            ViewBag.Servers = await LoadServerListAsync(policy.NodeId);
//            return View(policy);
//        }

//        public async Task<IActionResult> Edit(int? id)
//        {
//            if (id == null) return NotFound();
//            var policy = await _context.ResourcePolicies.FindAsync(id);
//            if (policy == null) return NotFound();
//            ViewBag.Servers = await LoadServerListAsync(policy.NodeId);
//            return View(policy);
//        }

//        [HttpPost]
//        [ValidateAntiForgeryToken]
//        public async Task<IActionResult> Edit(int id, [Bind("Id,NodeId,PolicyType,TargetProcess,TargetUser,MaxRamMB,MaxCpuPercent,ReclaimRam,OsAction,TargetPort,MaxBandwidthKbps,ApplyToIp")] ResourcePolicy policy)
//        {
//            if (id != policy.Id) return NotFound();

//            if (ModelState.IsValid)
//            {
//                try
//                {
//                    _context.Update(policy);
//                    await _context.SaveChangesAsync();

//                    // PHÂN NHÁNH GỬI LỆNH THEO LOẠI CHÍNH SÁCH
//                    (bool success, string message) result;
//                    if (policy.PolicyType == "NETWORK")
//                    {
//                        result = await _agentService.UpdateNetworkPolicyAsync(policy);
//                    }
//                    else
//                    {
//                        result = await _agentService.UpdatePolicyAsync(policy);
//                    }

//                    _context.EventLogs.Add(new EventLog
//                    {
//                        NodeId = policy.NodeId,
//                        Action = "Cập nhật " + policy.PolicyType,
//                        TargetProcess = policy.PolicyType == "NETWORK" ? $"Port {policy.TargetPort}" : policy.TargetProcess,
//                        Message = result.success ? "Thành công" : "Thất bại: " + result.message
//                    });
//                    await _context.SaveChangesAsync();

//                    TempData[result.success ? "Success" : "Error"] = result.success ? "Cập nhật và gửi Agent thành công" : result.message;
//                }
//                catch (DbUpdateConcurrencyException)
//                {
//                    if (!_context.ResourcePolicies.Any(e => e.Id == policy.Id)) return NotFound();
//                    else throw;
//                }
//                return RedirectToAction(nameof(Index));
//            }
//            ViewBag.Servers = await LoadServerListAsync(policy.NodeId);
//            return View(policy);
//        }

//        public async Task<IActionResult> Delete(int? id)
//        {
//            if (id == null) return NotFound();
//            var policy = await _context.ResourcePolicies.FirstOrDefaultAsync(m => m.Id == id);
//            if (policy == null) return NotFound();
//            return View(policy);
//        }

//        [HttpPost, ActionName("Delete")]
//        [ValidateAntiForgeryToken]
//        public async Task<IActionResult> DeleteConfirmed(int id)
//        {
//            var policy = await _context.ResourcePolicies.FindAsync(id);
//            if (policy != null)
//            {
//                var result = await _agentService.RemovePolicyAsync(policy.NodeId, policy.TargetProcess, policy.TargetUser);

//                _context.ResourcePolicies.Remove(policy);
//                await _context.SaveChangesAsync();

//                _context.EventLogs.Add(new EventLog
//                {
//                    NodeId = policy.NodeId,
//                    Action = "Xóa " + policy.PolicyType,
//                    TargetProcess = policy.PolicyType == "NETWORK" ? $"Port {policy.TargetPort}" : policy.TargetProcess,
//                    Message = result.success ? "Thành công" : "Thất bại: " + result.message
//                });
//                await _context.SaveChangesAsync();

//                TempData[result.success ? "Success" : "Error"] = result.success ? "Đã xóa và gỡ luật khỏi Agent" : result.message;
//            }
//            return RedirectToAction(nameof(Index));
//        }

//        // Luong: Agent --GetMetrics--> Controller (poller luu lai) --GetProcesses--> WebAdmin (ham nay)
//        [HttpGet]
//        public async Task<IActionResult> GetProcessList(string nodeId)
//        {
//            if (string.IsNullOrEmpty(nodeId)) return Json(new List<ProcessInfo>());

//            try
//            {
//                var processes = await _controllerService.GetProcessesAsync(nodeId);
//                return Json(processes.OrderBy(p => p.Name).ToList());
//            }
//            catch (Exception ex)
//            {
//                // Body la thong bao loi de view hien thi cho nguoi dung
//                return StatusCode(502, ex.Message);
//            }
//        }

//        [HttpGet]
//        public async Task<IActionResult> DownloadReport(string jobId, string filename = "result.json")
//        {
//            if (string.IsNullOrEmpty(jobId))
//            {
//                TempData["Error"] = "Vui lòng nhập JobId để tải báo cáo.";
//                return RedirectToAction(nameof(Index));
//            }

//            try
//            {
//                string base64Data = await _agentService.GetJobOutputAsync(jobId, filename);
//                byte[] fileBytes = Convert.FromBase64String(base64Data);

//                string contentType = filename.EndsWith(".csv") ? "text/csv" : "application/json";
//                return File(fileBytes, contentType, $"{jobId}_{filename}");
//            }
//            catch (Exception ex)
//            {
//                TempData["Error"] = "Không thể tải báo cáo: " + ex.Message;
//                return RedirectToAction(nameof(Index));
//            }
//        }
//    }
//}

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
        private readonly ControllerService _controllerService;

        public ResourcePoliciesController(AppDbContext context, AgentTcpService agentService, ControllerService controllerService)
        {
            _context = context;
            _agentService = agentService;
            _controllerService = controllerService;
        }

        // Danh sach server cho dropdown: lay tu Controller (servers.conf) de NodeId luon khop voi Controller
        private async Task<SelectList> LoadServerListAsync(string? selectedNodeId = null)
        {
            try
            {
                var servers = await _controllerService.GetServersAsync();
                var nodeIds = servers
                    .Where(s => !string.IsNullOrEmpty(s.NodeId))
                    .Select(s => s.NodeId!)
                    .Distinct()
                    .ToList();
                return new SelectList(nodeIds, selectedNodeId);
            }
            catch (Exception ex)
            {
                ViewBag.ServerError = "Không lấy được danh sách Server từ Controller: " + ex.Message;
                return new SelectList(new List<string>(), selectedNodeId);
            }
        }

        public async Task<IActionResult> Index() => View(await _context.ResourcePolicies.ToListAsync());

        // Hỗ trợ nhận sẵn nodeId, targetProcess, targetUser khi bấm nút "Giới hạn" từ Dashboard
        public async Task<IActionResult> Create(string? nodeId = null, string? targetProcess = null, string? targetUser = null)
        {
            ViewBag.Servers = await LoadServerListAsync(nodeId);
            var policy = new ResourcePolicy
            {
                NodeId = nodeId ?? string.Empty,
                PolicyType = "OS",
                TargetProcess = targetProcess ?? string.Empty,
                TargetUser = targetUser ?? string.Empty
            };
            return View(policy);
        }

        [HttpPost]
        [ValidateAntiForgeryToken]
        public async Task<IActionResult> Create([Bind("Id,NodeId,PolicyType,TargetProcess,TargetUser,MaxRamMB,MaxCpuPercent,ReclaimRam,OsAction,TargetPort,MaxBandwidthKbps,ApplyToIp")] ResourcePolicy policy)
        {
            if (ModelState.IsValid)
            {
                _context.Add(policy);
                await _context.SaveChangesAsync();

                // PHÂN NHÁNH GỬI LỆNH THEO LOẠI CHÍNH SÁCH
                (bool success, string message) result;
                if (policy.PolicyType == "NETWORK")
                {
                    result = await _agentService.UpdateNetworkPolicyAsync(policy);
                }
                else
                {
                    result = await _agentService.UpdatePolicyAsync(policy);
                }

                // Ghi log
                _context.EventLogs.Add(new EventLog
                {
                    NodeId = policy.NodeId,
                    Action = "Tạo mới " + policy.PolicyType,
                    TargetProcess = policy.PolicyType == "NETWORK" ? $"Port {policy.TargetPort}" : policy.TargetProcess,
                    Message = result.success ? "Thành công" : "Thất bại: " + result.message
                });
                await _context.SaveChangesAsync();

                TempData[result.success ? "Success" : "Error"] = result.success ? "Đã gửi Agent thành công" : result.message;
                return RedirectToAction(nameof(Index));
            }
            ViewBag.Servers = await LoadServerListAsync(policy.NodeId);
            return View(policy);
        }

        public async Task<IActionResult> Edit(int? id)
        {
            if (id == null) return NotFound();
            var policy = await _context.ResourcePolicies.FindAsync(id);
            if (policy == null) return NotFound();
            ViewBag.Servers = await LoadServerListAsync(policy.NodeId);
            return View(policy);
        }

        [HttpPost]
        [ValidateAntiForgeryToken]
        public async Task<IActionResult> Edit(int id, [Bind("Id,NodeId,PolicyType,TargetProcess,TargetUser,MaxRamMB,MaxCpuPercent,ReclaimRam,OsAction,TargetPort,MaxBandwidthKbps,ApplyToIp")] ResourcePolicy policy)
        {
            if (id != policy.Id) return NotFound();

            if (ModelState.IsValid)
            {
                try
                {
                    _context.Update(policy);
                    await _context.SaveChangesAsync();

                    // PHÂN NHÁNH GỬI LỆNH THEO LOẠI CHÍNH SÁCH
                    (bool success, string message) result;
                    if (policy.PolicyType == "NETWORK")
                    {
                        result = await _agentService.UpdateNetworkPolicyAsync(policy);
                    }
                    else
                    {
                        result = await _agentService.UpdatePolicyAsync(policy);
                    }

                    _context.EventLogs.Add(new EventLog
                    {
                        NodeId = policy.NodeId,
                        Action = "Cập nhật " + policy.PolicyType,
                        TargetProcess = policy.PolicyType == "NETWORK" ? $"Port {policy.TargetPort}" : policy.TargetProcess,
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
            ViewBag.Servers = await LoadServerListAsync(policy.NodeId);
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

                _context.EventLogs.Add(new EventLog
                {
                    NodeId = policy.NodeId,
                    Action = "Xóa " + policy.PolicyType,
                    TargetProcess = policy.PolicyType == "NETWORK" ? $"Port {policy.TargetPort}" : policy.TargetProcess,
                    Message = result.success ? "Thành công" : "Thất bại: " + result.message
                });
                await _context.SaveChangesAsync();

                TempData[result.success ? "Success" : "Error"] = result.success ? "Đã xóa và gỡ luật khỏi Agent" : result.message;
            }
            return RedirectToAction(nameof(Index));
        }

        // Luong: Agent --GetMetrics--> Controller (poller luu lai) --GetProcesses--> WebAdmin (ham nay)
        [HttpGet]
        public async Task<IActionResult> GetProcessList(string nodeId)
        {
            if (string.IsNullOrEmpty(nodeId)) return Json(new List<ProcessInfo>());

            try
            {
                var processes = await _controllerService.GetProcessesAsync(nodeId);
                return Json(processes.OrderBy(p => p.Name).ToList());
            }
            catch (Exception ex)
            {
                // Body la thong bao loi de view hien thi cho nguoi dung
                return StatusCode(502, ex.Message);
            }
        }

        [HttpGet]
        public async Task<IActionResult> DownloadReport(string jobId, string filename = "result.json")
        {
            if (string.IsNullOrEmpty(jobId))
            {
                TempData["Error"] = "Vui lòng nhập JobId để tải báo cáo.";
                return RedirectToAction(nameof(Index));
            }

            try
            {
                string base64Data = await _agentService.GetJobOutputAsync(jobId, filename);
                byte[] fileBytes = Convert.FromBase64String(base64Data);

                string contentType = filename.EndsWith(".csv") ? "text/csv" : "application/json";
                return File(fileBytes, contentType, $"{jobId}_{filename}");
            }
            catch (Exception ex)
            {
                TempData["Error"] = "Không thể tải báo cáo: " + ex.Message;
                return RedirectToAction(nameof(Index));
            }
        }
    }
}