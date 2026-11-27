using System;
using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace WebAdmin_ControlPlane.Migrations
{
    /// <inheritdoc />
    public partial class InitDatabase : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.CreateTable(
                name: "EventLogs",
                columns: table => new
                {
                    Id = table.Column<int>(type: "int", nullable: false)
                        .Annotation("SqlServer:Identity", "1, 1"),
                    NodeId = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    Action = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    TargetProcess = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    Message = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    Timestamp = table.Column<DateTime>(type: "datetime2", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_EventLogs", x => x.Id);
                });

            migrationBuilder.CreateTable(
                name: "ResourcePolicies",
                columns: table => new
                {
                    Id = table.Column<int>(type: "int", nullable: false)
                        .Annotation("SqlServer:Identity", "1, 1"),
                    NodeId = table.Column<string>(type: "nvarchar(max)", nullable: true),
                    PolicyType = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    TargetProcess = table.Column<string>(type: "nvarchar(max)", nullable: true),
                    TargetUser = table.Column<string>(type: "nvarchar(max)", nullable: true),
                    MaxRamMB = table.Column<int>(type: "int", nullable: true),
                    MaxCpuPercent = table.Column<int>(type: "int", nullable: true),
                    ReclaimRam = table.Column<bool>(type: "bit", nullable: false),
                    TargetPort = table.Column<int>(type: "int", nullable: true),
                    MaxBandwidthKbps = table.Column<int>(type: "int", nullable: true),
                    ApplyToIp = table.Column<string>(type: "nvarchar(max)", nullable: true)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_ResourcePolicies", x => x.Id);
                });

            migrationBuilder.CreateTable(
                name: "ServerNodes",
                columns: table => new
                {
                    NodeId = table.Column<string>(type: "nvarchar(450)", nullable: false),
                    IpAddress = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    Port = table.Column<int>(type: "int", nullable: false),
                    AuthToken = table.Column<string>(type: "nvarchar(max)", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_ServerNodes", x => x.NodeId);
                });

            migrationBuilder.CreateTable(
                name: "SystemConfigs",
                columns: table => new
                {
                    Id = table.Column<int>(type: "int", nullable: false)
                        .Annotation("SqlServer:Identity", "1, 1"),
                    ControllerIP = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    ControllerPort = table.Column<int>(type: "int", nullable: false),
                    ControllerToken = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    AgentIP = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    AgentPort = table.Column<int>(type: "int", nullable: false),
                    AgentToken = table.Column<string>(type: "nvarchar(max)", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_SystemConfigs", x => x.Id);
                });
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "EventLogs");

            migrationBuilder.DropTable(
                name: "ResourcePolicies");

            migrationBuilder.DropTable(
                name: "ServerNodes");

            migrationBuilder.DropTable(
                name: "SystemConfigs");
        }
    }
}
