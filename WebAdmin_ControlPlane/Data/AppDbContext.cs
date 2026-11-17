using Microsoft.EntityFrameworkCore;
using WebAdmin_ControlPlane.Models;

namespace WebAdmin_ControlPlane.Data
{
    public class AppDbContext : DbContext
    {
        public AppDbContext(DbContextOptions<AppDbContext> options) : base(options) { }

        public DbSet<ResourcePolicy> ResourcePolicies { get; set; }
        public DbSet<NodeStatus> NodeStatuses { get; set; }
        public DbSet<EventLog> EventLogs { get; set; }
        public DbSet<SystemConfig> SystemConfigs { get; set; }

    }
}