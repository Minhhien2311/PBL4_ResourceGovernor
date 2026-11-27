using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace WebAdmin_ControlPlane.Migrations
{
    /// <inheritdoc />
    public partial class FinalDatabaseUpdate : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "ServerNodes");

            migrationBuilder.DropColumn(
                name: "AgentIP",
                table: "SystemConfigs");

            migrationBuilder.DropColumn(
                name: "AgentPort",
                table: "SystemConfigs");

            migrationBuilder.DropColumn(
                name: "AgentToken",
                table: "SystemConfigs");
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
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
                name: "ServerNodes",
                columns: table => new
                {
                    NodeId = table.Column<string>(type: "nvarchar(450)", nullable: false),
                    AuthToken = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    IpAddress = table.Column<string>(type: "nvarchar(max)", nullable: false),
                    Port = table.Column<int>(type: "int", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_ServerNodes", x => x.NodeId);
                });
        }
    }
}
