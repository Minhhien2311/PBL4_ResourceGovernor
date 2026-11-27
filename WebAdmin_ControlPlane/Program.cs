//using Microsoft.EntityFrameworkCore;
//using WebAdmin_ControlPlane.Data;
//using WebAdmin_ControlPlane.Services;
//using Microsoft.AspNetCore.Authentication.Cookies;

//var builder = WebApplication.CreateBuilder(args);

//builder.Services.AddDbContext<AppDbContext>(options =>
//    options.UseSqlServer(builder.Configuration.GetConnectionString("DefaultConnection")));

//builder.Services.AddControllersWithViews();

//// --- ĐĂNG KÝ CÁC SERVICE ---
//// 1. Service giao tiếp với Controller (nếu vẫn dùng cho Dashboard)
//builder.Services.AddSingleton<ControllerService>();

//// 2. Service giao tiếp trực tiếp với Agent (Port 9001)
//builder.Services.AddSingleton<AgentTcpService>();

//// 3. Background Service chạy ngầm tự động đồng bộ luật xuống Agent
//builder.Services.AddHostedService<SyncPolicyBackgroundService>();

//builder.Services.AddSingleton<TelemetryCacheService>();
//builder.Services.AddHostedService<AgentPollingService>();

//// --- CẤU HÌNH ĐĂNG NHẬP ---
//builder.Services.AddAuthentication(CookieAuthenticationDefaults.AuthenticationScheme)
//    .AddCookie(options =>
//    {
//        options.LoginPath = "/Account/Login";
//        options.LogoutPath = "/Account/Logout";
//        options.AccessDeniedPath = "/Account/AccessDenied";
//    });
//builder.Services.AddAuthorization();

//var app = builder.Build();

//// Configure the HTTP request pipeline.
//if (!app.Environment.IsDevelopment())
//{
//    app.UseExceptionHandler("/Home/Error");
//    app.UseHsts();
//}

//app.UseHttpsRedirection();
//app.UseRouting();

//app.UseAuthentication();
//app.UseAuthorization();

//app.MapStaticAssets();
//app.MapControllerRoute(
//    name: "default",
//    pattern: "{controller=Home}/{action=Index}/{id?}")
//    .WithStaticAssets();

//app.Run();

using Microsoft.EntityFrameworkCore;
using WebAdmin_ControlPlane.Data;
using WebAdmin_ControlPlane.Services;
using Microsoft.AspNetCore.Authentication.Cookies;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddDbContext<AppDbContext>(options =>
    options.UseSqlServer(builder.Configuration.GetConnectionString("DefaultConnection")));

builder.Services.AddControllersWithViews();

// --- ĐĂNG KÝ CÁC SERVICE ---
// 1. Service giao tiếp với Controller (nếu vẫn dùng cho Dashboard)
builder.Services.AddSingleton<ControllerService>();

// 2. Service giao tiếp trực tiếp với Agent (Port 9001)
builder.Services.AddSingleton<AgentTcpService>();

// --- CẤU HÌNH ĐĂNG NHẬP ---
builder.Services.AddAuthentication(CookieAuthenticationDefaults.AuthenticationScheme)
    .AddCookie(options =>
    {
        options.LoginPath = "/Account/Login";
        options.LogoutPath = "/Account/Logout";
        options.AccessDeniedPath = "/Account/AccessDenied";
    });
builder.Services.AddAuthorization();

var app = builder.Build();

// Configure the HTTP request pipeline.
if (!app.Environment.IsDevelopment())
{
    app.UseExceptionHandler("/Home/Error");
    app.UseHsts();
}

app.UseHttpsRedirection();
app.UseRouting();

app.UseAuthentication();
app.UseAuthorization();

app.MapStaticAssets();
app.MapControllerRoute(
    name: "default",
    pattern: "{controller=Home}/{action=Index}/{id?}")
    .WithStaticAssets();

app.Run();