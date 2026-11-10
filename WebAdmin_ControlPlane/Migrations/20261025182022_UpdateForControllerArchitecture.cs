using System;
using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace WebAdmin_ControlPlane.Migrations
{
    /// <inheritdoc />
    public partial class UpdateForControllerArchitecture : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "EventLogs");

            migrationBuilder.DropTable(
                name: "NodeStatuses");

            migrationBuilder.DropColumn(
                name: "AuthToken",
                table: "SystemConfigs");

            migrationBuilder.DropColumn(
                name: "HeartbeatIntervalSeconds",
                table: "SystemConfigs");

            migrationBuilder.DropColumn(
                name: "OSAgentPort",
                table: "SystemConfigs");

            migrationBuilder.DropColumn(
                name: "AutoResume",
                table: "ResourcePolicies");

            migrationBuilder.DropColumn(
                name: "MaxBandwidthKbps",
                table: "ResourcePolicies");

            migrationBuilder.DropColumn(
                name: "MaxViolationsBeforeKill",
                table: "ResourcePolicies");

            migrationBuilder.DropColumn(
                name: "PauseDurationSeconds",
                table: "ResourcePolicies");

            migrationBuilder.DropColumn(
                name: "PenaltyMultiplier",
                table: "ResourcePolicies");

            migrationBuilder.DropColumn(
                name: "TargetPort",
                table: "ResourcePolicies");

            migrationBuilder.RenameColumn(
                name: "ProxyPort",
                table: "SystemConfigs",
                newName: "ControllerPort");

            migrationBuilder.RenameColumn(
                name: "ProxyIP",
                table: "SystemConfigs",
                newName: "ControllerToken");

            migrationBuilder.RenameColumn(
                name: "OSAgentIP",
                table: "SystemConfigs",
                newName: "ControllerIP");

            migrationBuilder.RenameColumn(
                name: "ApplyToIp",
                table: "ResourcePolicies",
                newName: "State");

            migrationBuilder.AddColumn<string>(
                name: "JobId",
                table: "ResourcePolicies",
                type: "nvarchar(max)",
                nullable: true);
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropColumn(
                name: "JobId",
                table: "ResourcePolicies");

            migrationBuilder.RenameColumn(
                name: "ControllerToken",
                table: "SystemConfigs",
                newName: "ProxyIP");

            migrationBuilder.RenameColumn(
                name: "ControllerPort",
                table: "SystemConfigs",
                newName: "ProxyPort");

            migrationBuilder.RenameColumn(
                name: "ControllerIP",
                table: "SystemConfigs",
                newName: "OSAgentIP");

            migrationBuilder.RenameColumn(
                name: "State",
                table: "ResourcePolicies",
                newName: "ApplyToIp");

            migrationBuilder.AddColumn<string>(
                name: "AuthToken",
                table: "SystemConfigs",
                type: "nvarchar(max)",
                nullable: false,
                defaultValue: "");

            migrationBuilder.AddColumn<int>(
                name: "HeartbeatIntervalSeconds",
                table: "SystemConfigs",
                type: "int",
                nullable: false,
                defaultValue: 0);

            migrationBuilder.AddColumn<int>(
                name: "OSAgentPort",
                table: "SystemConfigs",
                type: "int",
                nullable: false,
                defaultValue: 0);

            migrationBuilder.AddColumn<bool>(
                name: "AutoResume",
                table: "ResourcePolicies",
                type: "bit",
                nullable: true);

            migrationBuilder.AddColumn<int>(
                name: "MaxBandwidthKbps",
                table: "ResourcePolicies",
                type: "int",
                nullable: true);

            migrationBuilder.AddColumn<int>(
                name: "MaxViolationsBeforeKill",
                table: "ResourcePolicies",
                type: "int",
                nullable: true);

            migrationBuilder.AddColumn<int>(
                name: "PauseDurationSeconds",
                table: "ResourcePolicies",
                type: "int",
                nullable: true);

            migrationBuilder.AddColumn<double>(
                name: "PenaltyMultiplier",
                table: "ResourcePolicies",
                type: "float",
                nullable: true);

            migrationBuilder.AddColumn<int>(
                name: "TargetPort",
                table: "ResourcePolicies",
                type: "int",
                nullable: true);

            migrationBuilder.CreateTable(
                name: "EventLogs",
                columns: table => new
                {
                    Id = table.Column<int>(type: "int", nullable: false)
                        .Annotation("SqlServer:Identity", "1, 1"),
                    EventType = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    Message = table.Column<string>(type: "nvarchar(max)", nullable: true),
                    NodeType = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    Timestamp = table.Column<DateTime>(type: "datetime2", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_EventLogs", x => x.Id);
                });

            migrationBuilder.CreateTable(
                name: "NodeStatuses",
                columns: table => new
                {
                    Id = table.Column<int>(type: "int", nullable: false)
                        .Annotation("SqlServer:Identity", "1, 1"),
                    ActiveConnections = table.Column<int>(type: "int", nullable: false),
                    BandwidthInUse = table.Column<double>(type: "float", nullable: false),
                    CPUUsagePercent = table.Column<double>(type: "float", nullable: false),
                    LastHeartbeat = table.Column<DateTime>(type: "datetime2", nullable: false),
                    NodeType = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    ProcessCount = table.Column<int>(type: "int", nullable: false),
                    Status = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    TotalRAM = table.Column<double>(type: "float", nullable: false),
                    UsedRAM = table.Column<double>(type: "float", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_NodeStatuses", x => x.Id);
                });
        }
    }
}
