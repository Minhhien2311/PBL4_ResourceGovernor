using System;
using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace WebAdmin_ControlPlane.Migrations
{
    /// <inheritdoc />
    public partial class UpdatePolicyModel : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.AlterColumn<string>(
                name: "TargetProcess",
                table: "ResourcePolicies",
                type: "nvarchar(max)",
                nullable: true,
                oldClrType: typeof(string),
                oldType: "nvarchar(max)");

            migrationBuilder.AlterColumn<int>(
                name: "MaxRamMB",
                table: "ResourcePolicies",
                type: "int",
                nullable: true,
                oldClrType: typeof(int),
                oldType: "int");

            migrationBuilder.AlterColumn<int>(
                name: "MaxBandwidthKbps",
                table: "ResourcePolicies",
                type: "int",
                nullable: true,
                oldClrType: typeof(int),
                oldType: "int");

            migrationBuilder.AddColumn<string>(
                name: "ApplyToIp",
                table: "ResourcePolicies",
                type: "nvarchar(max)",
                nullable: true);

            migrationBuilder.AddColumn<string>(
                name: "OsAction",
                table: "ResourcePolicies",
                type: "nvarchar(max)",
                nullable: true);

            migrationBuilder.AddColumn<int>(
                name: "TargetPort",
                table: "ResourcePolicies",
                type: "int",
                nullable: true);

            migrationBuilder.AddColumn<int>(
                name: "Type",
                table: "ResourcePolicies",
                type: "int",
                nullable: false,
                defaultValue: 0);

            migrationBuilder.CreateTable(
                name: "EventLogs",
                columns: table => new
                {
                    Id = table.Column<int>(type: "int", nullable: false)
                        .Annotation("SqlServer:Identity", "1, 1"),
                    NodeType = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    EventType = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    Message = table.Column<string>(type: "nvarchar(max)", nullable: true),
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
                    NodeType = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    Status = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    CPUUsagePercent = table.Column<double>(type: "float", nullable: false),
                    TotalRAM = table.Column<double>(type: "float", nullable: false),
                    UsedRAM = table.Column<double>(type: "float", nullable: false),
                    ProcessCount = table.Column<int>(type: "int", nullable: false),
                    BandwidthInUse = table.Column<double>(type: "float", nullable: false),
                    ActiveConnections = table.Column<int>(type: "int", nullable: false),
                    LastHeartbeat = table.Column<DateTime>(type: "datetime2", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_NodeStatuses", x => x.Id);
                });
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "EventLogs");

            migrationBuilder.DropTable(
                name: "NodeStatuses");

            migrationBuilder.DropColumn(
                name: "ApplyToIp",
                table: "ResourcePolicies");

            migrationBuilder.DropColumn(
                name: "OsAction",
                table: "ResourcePolicies");

            migrationBuilder.DropColumn(
                name: "TargetPort",
                table: "ResourcePolicies");

            migrationBuilder.DropColumn(
                name: "Type",
                table: "ResourcePolicies");

            migrationBuilder.AlterColumn<string>(
                name: "TargetProcess",
                table: "ResourcePolicies",
                type: "nvarchar(max)",
                nullable: false,
                defaultValue: "",
                oldClrType: typeof(string),
                oldType: "nvarchar(max)",
                oldNullable: true);

            migrationBuilder.AlterColumn<int>(
                name: "MaxRamMB",
                table: "ResourcePolicies",
                type: "int",
                nullable: false,
                defaultValue: 0,
                oldClrType: typeof(int),
                oldType: "int",
                oldNullable: true);

            migrationBuilder.AlterColumn<int>(
                name: "MaxBandwidthKbps",
                table: "ResourcePolicies",
                type: "int",
                nullable: false,
                defaultValue: 0,
                oldClrType: typeof(int),
                oldType: "int",
                oldNullable: true);
        }
    }
}
