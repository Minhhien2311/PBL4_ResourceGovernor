using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace WebAdmin_ControlPlane.Migrations
{
    /// <inheritdoc />
    public partial class AddPausePenaltyFields : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.AddColumn<bool>(
                name: "AutoResume",
                table: "ResourcePolicies",
                type: "bit",
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
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropColumn(
                name: "AutoResume",
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
        }
    }
}
