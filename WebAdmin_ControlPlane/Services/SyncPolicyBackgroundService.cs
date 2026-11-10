using Microsoft.EntityFrameworkCore;
using WebAdmin_ControlPlane.Data;

namespace WebAdmin_ControlPlane.Services
{
    public class SyncPolicyBackgroundService : BackgroundService
    {
        private readonly IServiceProvider _serviceProvider;
        private readonly ILogger<SyncPolicyBackgroundService> _logger;

        public SyncPolicyBackgroundService(IServiceProvider serviceProvider, ILogger<SyncPolicyBackgroundService> logger)
        {
            _serviceProvider = serviceProvider;
            _logger = logger;
        }

        protected override async Task ExecuteAsync(CancellationToken stoppingToken)
        {
            while (!stoppingToken.IsCancellationRequested)
            {
                try
                {
                    using var scope = _serviceProvider.CreateScope();
                    var db = await scope.ServiceProvider.GetRequiredService<AppDbContext>().ResourcePolicies.ToListAsync();
                    var agentService = scope.ServiceProvider.GetRequiredService<AgentTcpService>();

                    // Thử gửi lại toàn bộ luật (nếu Agent offline sẽ throw lỗi, ta catch lại)
                    foreach (var policy in db)
                    {
                        await agentService.UpdatePolicyAsync(policy);
                    }
                    _logger.LogInformation("Đã đồng bộ toàn bộ Policy xuống Agent.");
                }
                catch (Exception)
                {
                    // Agent đang offline, bỏ qua và thử lại sau 10 giây
                }
                await Task.Delay(TimeSpan.FromSeconds(10), stoppingToken);
            }
        }
    }
}