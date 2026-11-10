using System;
using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace WebAdmin_ControlPlane.Migrations
{
    /// <inheritdoc />
    public partial class AddTargetUserToPolicy : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropColumn(
                name: "IsActive",
                table: "ResourcePolicies");

            migrationBuilder.DropColumn(
                name: "JobId",
                table: "ResourcePolicies");

            migrationBuilder.DropColumn(
                name: "Type",
                table: "ResourcePolicies");

            migrationBuilder.RenameColumn(
                name: "State",
                table: "ResourcePolicies",
                newName: "TargetUser");

            migrationBuilder.AddColumn<string>(
                name: "AgentIP",
                table: "SystemConfigs",
                type: "nvarchar(max)",
                nullable: false,
                defaultValue: "");

            migrationBuilder.AddColumn<int>(
                name: "AgentPort",
                table: "SystemConfigs",
                type: "int",
                nullable: false,
                defaultValue: 0);

            migrationBuilder.AddColumn<string>(
                name: "AgentToken",
                table: "SystemConfigs",
                type: "nvarchar(max)",
                nullable: false,
                defaultValue: "");

            migrationBuilder.CreateTable(
                name: "NodeStatuses",
                columns: table => new
                {
                    NodeId = table.Column<string>(type: "nvarchar(450)", nullable: false),
                    Status = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    CPUUsagePercent = table.Column<double>(type: "float", nullable: false),
                    TotalRAM = table.Column<long>(type: "bigint", nullable: false),
                    UsedRAM = table.Column<long>(type: "bigint", nullable: false),
                    LastHeartbeat = table.Column<DateTime>(type: "datetime2", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_NodeStatuses", x => x.NodeId);
                });
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "NodeStatuses");

            migrationBuilder.DropColumn(
                name: "AgentIP",
                table: "SystemConfigs");

            migrationBuilder.DropColumn(
                name: "AgentPort",
                table: "SystemConfigs");

            migrationBuilder.DropColumn(
                name: "AgentToken",
                table: "SystemConfigs");

            migrationBuilder.RenameColumn(
                name: "TargetUser",
                table: "ResourcePolicies",
                newName: "State");

            migrationBuilder.AddColumn<bool>(
                name: "IsActive",
                table: "ResourcePolicies",
                type: "bit",
                nullable: false,
                defaultValue: false);

            migrationBuilder.AddColumn<string>(
                name: "JobId",
                table: "ResourcePolicies",
                type: "nvarchar(max)",
                nullable: true);

            migrationBuilder.AddColumn<int>(
                name: "Type",
                table: "ResourcePolicies",
                type: "int",
                nullable: false,
                defaultValue: 0);
        }
    }
}
