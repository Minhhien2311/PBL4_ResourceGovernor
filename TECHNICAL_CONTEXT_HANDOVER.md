# Technical Context Handover

> Tài liệu này mô tả code hiện có tại thời điểm khảo sát. Tên hàm, tên biến, kiểu dữ liệu và JSON key được giữ theo source.  
> Phạm vi: C++ OS Agent ở thư mục gốc và ASP.NET Core Web Admin/Controller trong `WebAdmin_ControlPlane`.

## 0. Kiến trúc tổng thể

Hệ thống hiện có hai tiến trình logic chính:

1. **OS Agent (C++)**
   - Chạy trên Linux.
   - Đọc `/proc`, đo CPU/RAM/process.
   - Mở TCP server tại port `9001` để nhận lệnh điều phối từ Web Admin.
   - Gửi heartbeat metric bằng HTTP `POST` tới Controller.
   - Áp dụng giới hạn RAM bằng `SIGSTOP`/`SIGCONT`, giới hạn CPU bằng cgroup v2, và tùy chọn `process_madvise(MADV_PAGEOUT)`.
   - Có thêm cơ chế submit/cancel job chạy dưới user không phải root.

2. **WebAdmin_ControlPlane (ASP.NET Core MVC/API)**
   - Giao diện quản trị có cookie authentication.
   - Lưu policy, node status, event log, system config bằng Entity Framework Core/SQL Server.
   - Nhận heartbeat HTTP từ Agent.
   - Kết nối TCP tới Agent để gửi JSON command và đọc ACK.

Luồng dữ liệu đang chạy:

```text
Agent --HTTP POST /api/telemetry/heartbeat--> WebAdmin Controller --SQL Server--> DB
Web Admin --TCP 127.0.0.1:9001, JSON newline--> Agent
Agent --JSON response newline--> Web Admin
```

Không có gRPC, WebSocket, MQTT hoặc UDP trong code hiện tại.

---

## 1. Cấu trúc thư mục

Các thư mục/generated assets lớn như `.git`, `bin`, `obj`, `node_modules`, `venv`, `build` được loại khỏi cây dưới đây.

```text
MAIN/
├── README.md
├── .gitignore
├── agent.conf.example
├── jobs.conf
├── config.cpp
├── config.h
├── heartbeat.cpp
├── heartbeat.h
├── job.cpp
├── job.h
├── logger.cpp
├── logger.h
├── main.cpp
├── metrics.cpp
├── metrics.h
├── monitor.cpp
├── monitor.h
├── qoscg.cpp
├── qoscg.h
├── reclaim.cpp
├── reclaim.h
├── shared.cpp
├── shared.h
├── tcp_server.cpp
├── tcp_server.h
├── cpu_job.cpp
├── report_job.cpp
├── WebAdmin_ControlPlane/
│   ├── Program.cs
│   ├── WebAdmin_ControlPlane.csproj
│   ├── WebAdmin_ControlPlane.slnx
│   ├── appsettings.json
│   ├── Controllers/
│   │   ├── AccountController.cs
│   │   ├── DashboardController.cs
│   │   ├── EventLogsController.cs
│   │   ├── HomeController.cs
│   │   ├── ResourcePoliciesController.cs
│   │   ├── SystemConfigController.cs
│   │   └── TelemetryController.cs
│   ├── Data/
│   │   └── AppDbContext.cs
│   ├── Migrations/
│   │   ├── 20260902173813_InitialCreate.cs
│   │   ├── 20260902173813_InitialCreate.Designer.cs
│   │   ├── 20260902185714_UpdatePolicyModel.cs
│   │   ├── 20260902185714_UpdatePolicyModel.Designer.cs
│   │   ├── 20260904094001_AddSystemConfig.cs
│   │   ├── 20260904094001_AddSystemConfig.Designer.cs
│   │   └── AppDbContextModelSnapshot.cs
│   ├── Models/
│   │   ├── ErrorViewModel.cs
│   │   ├── EventLog.cs
│   │   ├── NodeStatus.cs
│   │   ├── ResourcePolicy.cs
│   │   └── SystemConfig.cs
│   ├── Properties/
│   │   └── launchSettings.json
│   ├── Services/
│   │   └── TcpCommandService.cs
│   ├── Views/
│   │   ├── _ViewImports.cshtml
│   │   ├── _ViewStart.cshtml
│   │   ├── Account/Login.cshtml
│   │   ├── Dashboard/Index.cshtml
│   │   ├── Dashboard/_NodeStatusList.cshtml
│   │   ├── EventLogs/Index.cshtml
│   │   ├── Home/Index.cshtml
│   │   ├── Home/Privacy.cshtml
│   │   ├── ResourcePolicies/Create.cshtml
│   │   ├── ResourcePolicies/Delete.cshtml
│   │   ├── ResourcePolicies/Details.cshtml
│   │   ├── ResourcePolicies/Edit.cshtml
│   │   ├── ResourcePolicies/Index.cshtml
│   │   ├── Shared/Error.cshtml
│   │   ├── Shared/_Layout.cshtml
│   │   ├── Shared/_Layout.cshtml.css
│   │   └── Shared/_ValidationScriptsPartial.cshtml
│   └── wwwroot/
│       ├── assets/img/
│       ├── css/
│       ├── img/
│       ├── js/
│       └── lib/
├── resource-governor       # binary runtime, nếu được build
├── resource_governor       # binary/runtime variant, nếu được build
├── cpu_job                 # binary runtime, nếu được build
└── report_job              # binary runtime, nếu được build
```

### 1.1. C++ Agent

| File | Nhiệm vụ |
|---|---|
| `main.cpp` | Tạo và `join()` ba thread: `chayTcpServer`, `chayMonitorLoop`, `chayHeartbeatLoop`. |
| `tcp_server.cpp/.h` | TCP server, framing JSON, xác thực HMAC/AuthToken và dispatch command. |
| `heartbeat.cpp/.h` | Vòng lặp HTTP heartbeat bằng libcurl, gửi metric/process list lên Controller. |
| `monitor.cpp/.h` | Vòng lặp quét `/proc`, chọn policy, pause/resume RAM, áp CPU cgroup và reclaim RAM. |
| `metrics.cpp/.h` | Đọc `/proc/stat`, `/proc/meminfo`, `/proc/[pid]/status`; tạo `SoLieu`, `TienTrinhInfo`. |
| `job.cpp/.h` | Submit job bằng `fork`/`execv`, giới hạn số job, status, cancel, reap child. |
| `shared.cpp/.h` | `LuatRieng`, mutex `khoa`, vector policy dùng chung giữa TCP và monitor. |
| `qoscg.cpp/.h` | Kiểm tra cgroup v2, tạo cgroup per-PID và ghi `cpu.max`. |
| `reclaim.cpp/.h` | Gọi `pidfd_open` và `process_madvise(MADV_PAGEOUT)` để đẩy page ra swap. |
| `config.cpp/.h` | Đọc key/value từ `agent.conf`; đọc lại mỗi lần gọi. |
| `logger.cpp/.h` | Ghi append vào `agent.log`, có mutex nội bộ. |
| `cpu_job.cpp` | Job mẫu bận CPU theo số giây, mặc định 30, hợp lệ 1–600. |
| `report_job.cpp` | Job mẫu cấp phát/touch RAM và chạy theo thời gian; hỗ trợ `SIGTERM`/`SIGINT`. |
| `jobs.conf` | Map `JobType=absolute_executable_path` cho `SubmitJob`. |

`README.md` hiện chỉ có tiêu đề, chưa phải tài liệu build/vận hành.

### 1.2. ASP.NET Controller/Admin

| File | Nhiệm vụ |
|---|---|
| `Program.cs` | DI, EF Core SQL Server, MVC, cookie auth, đăng ký `TcpCommandService` singleton + hosted service. |
| `Data/AppDbContext.cs` | Bốn `DbSet`: `ResourcePolicies`, `NodeStatuses`, `EventLogs`, `SystemConfigs`. |
| `Services/TcpCommandService.cs` | Gửi command TCP đến node và đọc JSON ACK; lấy process list. |
| `Controllers/TelemetryController.cs` | HTTP API nhận heartbeat/log từ node. |
| `Controllers/ResourcePoliciesController.cs` | CRUD policy và endpoint lấy process list. |
| `Controllers/DashboardController.cs` | Dashboard node online/offline, thống kê, JSON polling. |
| `Controllers/SystemConfigController.cs` | Đọc/tạo/cập nhật cấu hình kết nối. |
| `Controllers/EventLogsController.cs` | Hiển thị 200 event log mới nhất. |
| `Controllers/AccountController.cs` | Login/logout demo bằng cookie. |
| `Controllers/HomeController.cs` | Home, Privacy, Error. |
| `Models/*.cs` | Entity/model binding cho DB và UI. |
| `Migrations/*.cs` | Schema EF Core cho các entity. |
| `Views/**/*.cshtml` | Giao diện MVC; Dashboard polling 5 giây. |

---

## 2. Communication Contract

## 2.1. Agent heartbeat: HTTP JSON

Agent đọc:

```text
HeartbeatUrl = docCauHinh("HeartbeatUrl", "http://localhost:5247/api/telemetry/heartbeat")
```

Trong `agent.conf` hiện tại URL là `http://127.0.0.1:5247/x`, vì vậy nếu giữ nguyên file cấu hình hiện tại heartbeat **không đi vào** endpoint ASP.NET `/api/telemetry/heartbeat`.

Mỗi vòng lặp:

1. `chayHeartbeatLoop()`
2. `laySoLieu()`
3. `layTienTrinh(30)`
4. Serialize JSON
5. HTTP `POST`, header `Content-Type: application/json`, timeout 5 giây
6. Sleep 5 giây

Payload thực tế:

```json
{
  "Processes": [
    {
      "name": "process-name",
      "pid": 1234,
      "user": "linux-user",
      "ramMB": 12.3
    }
  ],
  "NodeType": "OSAgent",
  "NodeId": "server-a",
  "Status": "Running",
  "CPUUsagePercent": 13.5,
  "TotalRAM": 15984,
  "UsedRAM": 4096,
  "ProcessCount": 120
}
```

`NodeStatus` của Controller không có property `Processes` và `NodeId`. Với ASP.NET JSON model binding mặc định, các field này không được lưu vào `NodeStatus`; process list trong heartbeat hiện bị bỏ qua.

### Controller HTTP endpoints

| Method | Route | Request model | Response/side effect |
|---|---|---|---|
| `POST` | `/api/telemetry/heartbeat` | `[FromBody] NodeStatus status` | Nếu `status == null`: `400`; upsert theo `NodeType`; cập nhật `LastHeartbeat = DateTime.Now`; trả `{ "status": "ok" }`. |
| `POST` | `/api/telemetry/log` | `[FromBody] EventLog log` | Nếu null: `400`; set `Timestamp = DateTime.Now`, insert DB; trả `{ "status": "ok" }`. |

Controller tìm node theo `NodeType`, không theo `NodeId`. Nhiều node cùng `NodeType = "OSAgent"` sẽ ghi đè cùng một row.

## 2.2. Agent command server: TCP + newline-delimited JSON

- Bind `AF_INET`, `INADDR_ANY`, port `9001`.
- `listen(serverfd, 5)`.
- Mỗi connection đọc tối đa `MAX_LENH = 4096` bytes.
- Đọc tới newline, client đóng connection, hoặc `json::accept(duLieu)` trả true.
- Receive timeout: 5 giây.
- Response là một JSON object + `\n`, sau đó đóng connection.
- Request phải là JSON object.

### Authentication envelope A: HMAC-SHA256

```json
{
  "Payload": "{\"CommandType\":\"GetMetrics\",\"Timestamp\":1730000000}",
  "Signature": "<64-char lowercase hex HMAC-SHA256>"
}
```

- HMAC input chính xác là chuỗi `Payload`, không phải object đã parse.
- Secret: `docCauHinh("Token", "abc123")`.
- So sánh signature bằng `CRYPTO_memcmp`.
- `Payload` phải parse thành object.
- `Timestamp` phải là integer dương.
- Chênh lệch `abs(serverTime - clientTime) <= 5` giây.
- Sai sẽ trả `status = "error"` với một trong các message: `HMAC calculation failed`, `Invalid HMAC signature`, `Packet expired`, `Missing or invalid Timestamp`, `Malformed Payload JSON`.

### Authentication envelope B: AuthToken trực tiếp

```json
{
  "AuthToken": "abc123",
  "CommandType": "GetMetrics"
}
```

- Chỉ được nhận khi `docCauHinh("RequireHmac", "0") != "1"`.
- So sánh token constant-time với `SECRET_TOKEN`.
- `j = outer`, tức các command field nằm ở top-level.
- Khi `RequireHmac=1`, request trực tiếp bị từ chối với `HMAC signature required`.

### Command payloads và responses

#### `GetMetrics`

Request payload tối thiểu:

```json
{ "CommandType": "GetMetrics" }
```

Response thành công:

```json
{
  "NodeId": "server-a",
  "status": "ok",
  "CPUUsagePercent": 12.5,
  "TotalRAM": 15984,
  "UsedRAM": 4096,
  "ProcessCount": 120,
  "Processes": [
    { "name": "report_job", "pid": 1234, "user": "user", "ramMB": 100.0 }
  ]
}
```

`Processes` tối đa 30 process, sort giảm dần theo `ramMb`.

#### `UpdateOsPolicy`

Request:

```json
{
  "CommandType": "UpdateOsPolicy",
  "TargetProcess": "report_job",
  "TargetUser": "alice",
  "MaxRamMB": 100,
  "MaxCpuPercent": 50,
  "ReclaimRam": true
}
```

`TargetUser` có thể rỗng để áp dụng chung (`uid = -1`). Validation:

- `TargetProcess` không rỗng.
- `MaxRamMB` trong `[0, 1048576]`.
- `MaxCpuPercent` trong `[0, 6400]`.
- Ít nhất một trong `MaxRamMB > 0` hoặc `MaxCpuPercent > 0`.
- Nếu `TargetUser` không rỗng, `getpwnam()` phải tìm thấy user.
- Tối đa 100 policy trong `danhSachLuat`.

Response thành công: `{ "status": "ok", "message": "Policy applied" }`.

#### `GetOsPolicies`

Request:

```json
{ "CommandType": "GetOsPolicies" }
```

Response:

```json
{
  "status": "ok",
  "Policies": [
    {
      "TargetUser": "",
      "TargetProcess": "report_job",
      "MaxRamMB": 100,
      "MaxCpuPercent": 50,
      "ReclaimRam": true
    }
  ]
}
```

`TargetUser = ""` biểu thị policy chung cho mọi user.

#### `RemoveOsPolicy`

Request:

```json
{
  "CommandType": "RemoveOsPolicy",
  "TargetProcess": "report_job",
  "TargetUser": "alice"
}
```

Xóa các policy khớp `uid` và `tenTienTrinh`. Response:

- Xóa được: `{ "status": "ok", "message": "Policy removed" }`
- Không tìm thấy: `{ "status": "error", "message": "Policy not found" }`

#### `SubmitJob`

Request:

```json
{
  "CommandType": "SubmitJob",
  "JobId": "job-1",
  "JobType": "report_job",
  "TargetUser": "alice",
  "Params": ["20", "100"]
}
```

- `JobId`, `JobType`: chỉ `[A-Za-z0-9_-]`, tối đa 64 bytes.
- `Params`: array tối đa 8 phần tử; mỗi phần tử string printable ASCII, tối đa 64 bytes.
- `JobType` phải map tới executable tuyệt đối và executable (`jobs.conf`).
- `TargetUser` bắt buộc là user tồn tại, `pw_uid >= 1000`.
- Tối đa 3 job `RUNNING`, bảng tối đa 200 entry; entry không chạy được dọn khi cần.
- Child chạy `setsid`, tắt core dump, đóng FD từ 3 đến 1023, redirect stdin/stdout/stderr tới `/dev/null`, set groups/gid/uid, kiểm tra không thể `setuid(0)`, `chdir("/tmp")`, rồi `execv`.

Response submit mới:

```json
{
  "status": "ok",
  "JobId": "job-1",
  "State": "RUNNING",
  "Pid": 1234
}
```

Submit lại cùng `JobId` là idempotent và trả `Already submitted` cùng state hiện tại.

#### `GetJobStatus`

Request:

```json
{ "CommandType": "GetJobStatus", "JobId": "job-1" }
```

Response:

```json
{
  "status": "ok",
  "JobId": "job-1",
  "JobType": "report_job",
  "TargetUser": "alice",
  "State": "RUNNING",
  "Pid": 1234,
  "ElapsedSeconds": 7
}
```

State nội bộ: `RUNNING`, `DONE`, `FAILED`, `CANCELLED`; nếu process đang bị stop thì response có thể hiển thị `SUSPENDED`. `ExitCode` chỉ xuất hiện khi state không còn `RUNNING`.

#### `CancelJob`

Request:

```json
{ "CommandType": "CancelJob", "JobId": "job-1" }
```

Agent gửi `SIGTERM`, sau đó `SIGCONT` để process đang bị stop vẫn nhận signal; đặt `yeuCauHuy = time(nullptr)`. Nếu sau 5 giây chưa kết thúc, `thuHoiJob()` gửi `SIGKILL`.

Response khi đang chạy:

```json
{
  "status": "ok",
  "JobId": "job-1",
  "message": "Cancel requested",
  "State": "RUNNING"
}
```

### Command chưa được Agent active xử lý

`UpdateNetworkPolicy` không có nhánh xử lý trong active `tcp_server.cpp`; command này sẽ trả:

```json
{ "status": "error", "message": "Unknown command" }
```

---

## 3. Data models / structs / classes

### 3.1. C++ Agent

```cpp
struct LuatRieng {
    long uid;                  // -1 = ap dung cho moi user
    std::string tenTienTrinh;
    long maxKb;                // 0 = khong gioi han RAM
    int maxCpu = 0;            // % cua MOT loi (100 = 1 loi), 0 = khong gioi han CPU
    bool thuHoiRam = false;    // true: tam dung xong thi ep RAM ra swap
};
```

Global shared state:

```cpp
extern std::mutex khoa;
extern std::vector<LuatRieng> danhSachLuat;
```

Định nghĩa hiện tại:

```cpp
std::mutex khoa;
std::vector<LuatRieng> danhSachLuat = { {-1, "eat_ram", 100000} };
```

`JobAgent` là struct private trong `job.cpp`:

```cpp
struct JobAgent {
    pid_t pid = -1;
    std::string loai, user, state;   // RUNNING, DONE, FAILED, CANCELLED
    time_t batDau = 0;
    time_t yeuCauHuy = 0;
    int maThoat = 0;
};
```

Job table:

```cpp
static std::mutex m;
static std::map<std::string, JobAgent> bang;
```

Metric structs:

```cpp
struct SoLieu {
    double cpuPhanTram;
    long tongRamMb;
    long daDungRamMb;
    int soTienTrinh;
};

struct TienTrinhInfo {
    int pid;
    std::string ten;
    std::string user;
    long uid;
    double ramMb;
};
```

### 3.2. C# models

`NodeStatus`:

```csharp
public class NodeStatus
{
    [Key] public int Id { get; set; }
    [Required] public string NodeType { get; set; } = "";
    [Required] public string Status { get; set; } = "";
    public double CPUUsagePercent { get; set; }
    public double TotalRAM { get; set; }
    public double UsedRAM { get; set; }
    public int ProcessCount { get; set; }
    public double BandwidthInUse { get; set; }
    public int ActiveConnections { get; set; }
    public DateTime LastHeartbeat { get; set; }
}
```

`ResourcePolicy`:

```csharp
public enum PolicyType
{
    OsResource = 1,
    NetworkQoS = 2
}

public class ResourcePolicy
{
    [Key] public int Id { get; set; }
    [Required] public PolicyType Type { get; set; }
    public bool IsActive { get; set; } = true;
    public string? TargetProcess { get; set; }
    public int? MaxRamMB { get; set; }
    public string? OsAction { get; set; } = "Kill";
    public int? TargetPort { get; set; }
    public int? MaxBandwidthKbps { get; set; }
    public string? ApplyToIp { get; set; } = "All";
}
```

`SystemConfig`:

```csharp
public class SystemConfig
{
    [Key] public int Id { get; set; }
    [Required] public string AuthToken { get; set; } = "abc123";
    public string OSAgentIP { get; set; } = "127.0.0.1";
    public int OSAgentPort { get; set; } = 9001;
    public string ProxyIP { get; set; } = "127.0.0.1";
    public int ProxyPort { get; set; } = 9002;
    public int HeartbeatIntervalSeconds { get; set; } = 5;
}
```

`EventLog`:

```csharp
public class EventLog
{
    [Key] public int Id { get; set; }
    [Required] public string NodeType { get; set; } = "";
    [Required] public string EventType { get; set; } = "";
    public string? Message { get; set; }
    public DateTime Timestamp { get; set; }
}
```

`ProcessListResponse` và `ProcessInfo` chỉ tồn tại trong `TcpCommandService.cs`:

```csharp
public class ProcessListResponse
{
    public string Status { get; set; }
    public List<ProcessInfo> Processes { get; set; }
}

public class ProcessInfo
{
    public string Name { get; set; }
    public int Pid { get; set; }
    public double RamMB { get; set; }
}
```

`AppDbContext` khai báo:

```csharp
public DbSet<ResourcePolicy> ResourcePolicies { get; set; }
public DbSet<NodeStatus> NodeStatuses { get; set; }
public DbSet<EventLog> EventLogs { get; set; }
public DbSet<SystemConfig> SystemConfigs { get; set; }
```

---

## 4. Core workflows

## 4.1. Agent startup

`main()` tạo:

```cpp
std::thread luongTcp(chayTcpServer);
std::thread luongMonitor(chayMonitorLoop);
std::thread heartbeatThread(chayHeartbeatLoop);
```

Ba thread chạy vô hạn và được `join()`. Không có graceful shutdown signal/stop token.

`chayMonitorLoop()` gọi `cgDonDepKhiKhoiDong()` trước vòng lặp chính để đưa process còn trong cgroup `qos/p<PID>` về root cgroup và xóa cgroup cũ.

## 4.2. Registration/handshake

Không có endpoint registration riêng. Agent tự “đăng ký” lần đầu bằng heartbeat:

1. `chayHeartbeatLoop()` đọc `HeartbeatUrl`, `NodeId`.
2. Thu thập `SoLieu`/process list.
3. POST JSON tới `/api/telemetry/heartbeat`.
4. `TelemetryController.ReceiveHeartbeat(NodeStatus status)` tìm row theo `NodeType`.
5. Nếu có: update fields và `LastHeartbeat`; nếu chưa: insert.
6. Trả `{ "status": "ok" }`.

`NodeId` hiện được gửi nhưng không có trong `NodeStatus`, do đó không tham gia identity/upsert.

## 4.3. Metric collection/reporting

`laySoLieu()`:

1. Đọc dòng đầu `/proc/stat`.
2. Đợi `usleep(500000)`.
3. Đọc lần hai và tính CPU:
   `100.0 * ((delta_total) - (delta_idle + delta_iowait)) / delta_total`.
4. Đọc `MemTotal` và `MemAvailable` từ `/proc/meminfo`; đơn vị kB đổi sang MB bằng `/1024`.
5. Đếm directory tên toàn số trong `/proc`.

`layTienTrinh(size_t toiDa)`:

1. Quét `/proc/<pid>/status`.
2. Đọc `Name`, `Uid`, `VmRSS`.
3. Bỏ kernel thread không có `VmRSS`.
4. `ramMb = round(vmrss / 1024.0 * 10) / 10`.
5. Sort giảm dần `ramMb`, resize tối đa `toiDa`, rồi resolve username bằng `getpwuid_r`.

Heartbeat gửi process list 30 phần tử mỗi 5 giây. TCP `GetMetrics` cũng thu thập đồng bộ; do `laySoLieu()` sleep 0.5 giây, request metrics có latency tối thiểu khoảng 0.5 giây.

## 4.4. Controller gửi policy xuống Agent

Workflow từ UI:

1. `ResourcePoliciesController.Create(...)` hoặc `Edit(...)` validate `ModelState`.
2. Lưu `ResourcePolicy` vào SQL Server.
3. Gọi `_tcpCommandService.SendPolicyToNodeAsync(resourcePolicy)`.
4. `TcpCommandService.GetSystemConfigAsync()` lấy `SystemConfig`.
5. Với `PolicyType.OsResource`, tạo payload `CommandType = "UpdateOsPolicy"`.
6. Mở `TcpClient` đến `OSAgentIP:OSAgentPort`, ghi JSON + newline.
7. Đọc response, tìm key `status`.
8. Hiển thị TempData success/error.

**Lệch contract hiện tại:** C# không gửi `MaxCpuPercent`, `ReclaimRam`, `TargetUser`; Agent có hỗ trợ các field này nhưng sẽ nhận giá trị mặc định (`0`, `false`, `""`). C# gửi `OsAction`, Agent không đọc field này.

`ResourcePoliciesController.DeleteConfirmed` chỉ xóa policy trong DB, không gửi `RemoveOsPolicy` xuống Agent. Vì vậy policy runtime trong Agent có thể còn tồn tại sau khi xóa trên UI.

## 4.5. Agent enforcement

Mỗi khoảng 3 giây trong `chayMonitorLoop()`:

1. Copy `danhSachLuat` dưới mutex `khoa`.
2. Đọc phần trăm RAM trống.
3. Quét mọi `/proc/<pid>/status`.
4. Bỏ PID 1, chính Agent, UID 0 và process không có RSS.
5. Chọn policy riêng theo UID trước policy chung `uid = -1`.
6. CPU:
   - Nếu `maxCpu > 0`, gọi `cgGioiHanCpu(pid, maxCpu)`.
   - `cpu.max = quota + " 100000"`, quota `maxCpu * 1000`.
   - Nếu policy bỏ giới hạn CPU, gọi `cgGo(pid)`.
7. RAM:
   - Nếu `vmrss >= gioiHan`, `kill(pid, SIGSTOP)`.
   - Lưu thời điểm stop và ngưỡng lúc stop.
   - Nếu `thuHoiRam`, gọi `thuHoiRamTienTrinh(pid)`.
   - Resume khi policy bị gỡ, ngưỡng được nâng và RSS dưới ngưỡng mới, hoặc bị stop ít nhất `TTL_GIAY = 60` giây và RAM trống `>= MOC_THA_PHAN_TRAM = 32`.
8. Dọn PID đã chết khỏi các map tracking.
9. Gọi `thuHoiJob()`, sleep 3 giây.

---

## 5. Key function signatures

### 5.1. C++ Agent

| File | Signature | Return | Mô tả |
|---|---|---|---|
| `main.cpp` | `int main()` | `int` | Khởi chạy ba thread Agent. |
| `config.cpp` | `std::string docCauHinh(const std::string& khoa, const std::string& macDinh)` | `std::string` | Đọc key từ `agent.conf`, fallback default. |
| `heartbeat.cpp` | `void chayHeartbeatLoop()` | `void` | HTTP heartbeat vô hạn mỗi 5 giây. |
| `metrics.cpp` | `SoLieu laySoLieu()` | `SoLieu` | Đo CPU/RAM/process count. |
| `metrics.cpp` | `std::vector<TienTrinhInfo> layTienTrinh(size_t toiDa)` | `std::vector<TienTrinhInfo>` | Lấy top process theo RSS. |
| `monitor.cpp` | `void chayMonitorLoop()` | `void` | Enforcement loop RAM/CPU/reclaim. |
| `tcp_server.cpp` | `void chayTcpServer()` | `void` | TCP accept/read/auth/dispatch/write loop. |
| `tcp_server.cpp` | `static bool tinhHmacSha256(const std::string& payload, const std::string& secret, std::string& chuKyHex)` | `bool` | Tính HMAC-SHA256 hex. |
| `tcp_server.cpp` | `static bool chuKyBangNhauHangDinh(const std::string& a, const std::string& b)` | `bool` | So sánh constant-time. |
| `job.cpp` | `void thuHoiJob()` | `void` | `waitpid(WNOHANG)`, cập nhật state và timeout kill. |
| `job.cpp` | `nlohmann::json xuLySubmitJob(const nlohmann::json& j, const std::string& nguon)` | `nlohmann::json` | Validate và fork/exec job. |
| `job.cpp` | `nlohmann::json xuLyGetJobStatus(const nlohmann::json& j)` | `nlohmann::json` | Trả status job. |
| `job.cpp` | `nlohmann::json xuLyCancelJob(const nlohmann::json& j, const std::string& nguon)` | `nlohmann::json` | Gửi SIGTERM/SIGCONT cho job. |
| `qoscg.cpp` | `bool cgSanSang()` | `bool` | Kiểm tra cgroup v2 và CPU controller. |
| `qoscg.cpp` | `bool cgGioiHanCpu(int pid, int phanTram)` | `bool` | Tạo cgroup per-PID và áp CPU quota. |
| `qoscg.cpp` | `void cgGo(int pid)` | `void` | Đưa PID về root cgroup và xóa cgroup. |
| `qoscg.cpp` | `void cgDonDepKhiKhoiDong()` | `void` | Dọn cgroup từ lần chạy trước. |
| `reclaim.cpp` | `long thuHoiRamTienTrinh(int pid)` | `long` | Page out memory; trả bytes hoặc `-1`. |
| `logger.cpp` | `void ghiNhatKy(const std::string& noiDung)` | `void` | Append log vào `agent.log`. |

### 5.2. ASP.NET Core

| File | Signature | Return | Mô tả |
|---|---|---|---|
| `TelemetryController.cs` | `Task<IActionResult> ReceiveHeartbeat([FromBody] NodeStatus status)` | `Task<IActionResult>` | Upsert telemetry theo `NodeType`. |
| `TelemetryController.cs` | `Task<IActionResult> ReceiveLog([FromBody] EventLog log)` | `Task<IActionResult>` | Insert event log. |
| `TcpCommandService.cs` | `protected override async Task ExecuteAsync(CancellationToken stoppingToken)` | `Task` | Hosted loop hiện chỉ delay 30 giây. |
| `TcpCommandService.cs` | `private async Task<SystemConfig> GetSystemConfigAsync()` | `Task<SystemConfig>` | Lấy config đầu tiên từ DB. |
| `TcpCommandService.cs` | `public async Task<(bool success, string message)> SendPolicyToNodeAsync(ResourcePolicy policy)` | `Task<(bool success, string message)>` | Gửi policy qua TCP. |
| `TcpCommandService.cs` | `public async Task<List<ProcessInfo>> GetProcessListFromOsAgentAsync()` | `Task<List<ProcessInfo>>` | Gửi `GetProcessList`, parse process response. |
| `ResourcePoliciesController.cs` | `Task<IActionResult> Index()` | `Task<IActionResult>` | Liệt kê policy. |
| `ResourcePoliciesController.cs` | `Task<IActionResult> Details(int? id)` | `Task<IActionResult>` | Chi tiết policy. |
| `ResourcePoliciesController.cs` | `IActionResult Create()` | `IActionResult` | Render form create. |
| `ResourcePoliciesController.cs` | `Task<IActionResult> Create(ResourcePolicy resourcePolicy)` | `Task<IActionResult>` | Lưu và gửi policy. |
| `ResourcePoliciesController.cs` | `Task<IActionResult> Edit(int? id)` | `Task<IActionResult>` | Render form edit. |
| `ResourcePoliciesController.cs` | `Task<IActionResult> Edit(int id, ResourcePolicy resourcePolicy)` | `Task<IActionResult>` | Cập nhật và gửi policy. |
| `ResourcePoliciesController.cs` | `Task<IActionResult> Delete(int? id)` | `Task<IActionResult>` | Render confirm delete. |
| `ResourcePoliciesController.cs` | `Task<IActionResult> DeleteConfirmed(int id)` | `Task<IActionResult>` | Xóa DB, không gửi command Agent. |
| `ResourcePoliciesController.cs` | `Task<JsonResult> GetProcessList()` | `Task<JsonResult>` | Proxy process list từ Agent. |
| `DashboardController.cs` | `Task<IActionResult> Index()` | `Task<IActionResult>` | Dashboard. |
| `DashboardController.cs` | `Task<IActionResult> GetNodeStatus()` | `Task<IActionResult>` | JSON node status. |
| `DashboardController.cs` | `Task<IActionResult> GetDashboardStats()` | `Task<IActionResult>` | JSON aggregate stats. |
| `DashboardController.cs` | `private async Task<List<NodeStatus>> GetNodesWithUpdatedStatusAsync()` | `Task<List<NodeStatus>>` | Đánh dấu Offline nếu heartbeat quá 15 giây. |
| `SystemConfigController.cs` | `Task<IActionResult> Index()` | `Task<IActionResult>` | Lấy/tạo config mặc định. |
| `SystemConfigController.cs` | `Task<IActionResult> Edit(SystemConfig model)` | `Task<IActionResult>` | Cập nhật system config. |
| `EventLogsController.cs` | `Task<IActionResult> Index()` | `Task<IActionResult>` | Lấy 200 log mới nhất. |
| `AccountController.cs` | `IActionResult Login()` | `IActionResult` | Render login. |
| `AccountController.cs` | `Task<IActionResult> Login(string username, string password)` | `Task<IActionResult>` | Login demo cố định. |
| `AccountController.cs` | `Task<IActionResult> Logout()` | `Task<IActionResult>` | Sign out cookie. |

---

## 6. Trạng thái hiện tại, stub và WIP

### Đã có implementation hoạt động

- TCP Agent server, newline framing, giới hạn message 4096 byte.
- HMAC-SHA256 + timestamp replay window 5 giây.
- AuthToken direct mode và `RequireHmac`.
- Metric collection CPU/RAM/process.
- Heartbeat HTTP bằng libcurl.
- Runtime OS policy trong memory.
- RAM pause/resume với TTL.
- CPU cgroup v2 per process.
- Optional RAM reclaim.
- Job submit/status/cancel với child process isolation cơ bản.
- ASP.NET MVC CRUD policy/config/log/dashboard.
- EF Core SQL Server models/migrations.
- Cookie authentication pipeline.
- Heartbeat/log API.

### Stub/mock hoặc behavior chưa hoàn thiện

1. **Network QoS chưa triển khai ở Agent**
   - Model và UI có `PolicyType.NetworkQoS`, `TargetPort`, `MaxBandwidthKbps`, `ApplyToIp`.
   - `TcpCommandService` gửi `UpdateNetworkPolicy`.
   - Active Agent không có nhánh `UpdateNetworkPolicy`; kết quả là `Unknown command`.

2. **Process list contract bị lệch**
   - Controller gọi `CommandType = "GetProcessList"`.
   - Active Agent chỉ xử lý `GetMetrics`.
   - `GetProcessListFromOsAgentAsync()` thường nhận `status=error` rồi catch/return `new List<ProcessInfo>()`.
   - Agent `GetMetrics` trả process object có `user`, nhưng C# `ProcessInfo` không có property `User`.

3. **Policy fields bị mất khi gửi từ UI**
   - UI model không có `TargetUser`, `MaxCpuPercent`, `ReclaimRam`.
   - Agent hỗ trợ ba field này.
   - UI chỉ truyền `TargetProcess`, `MaxRamMB`, `OsAction`, `AuthToken`.

4. **Delete policy không đồng bộ Agent**
   - Delete chỉ xóa DB.
   - Không gọi `RemoveOsPolicy`; runtime policy vẫn còn trong `danhSachLuat` cho tới restart hoặc command remove riêng.

5. **Heartbeat URL cấu hình hiện tại không khớp route**
   - `agent.conf.example` và `agent.conf` đang dùng `HeartbeatUrl=http://127.0.0.1:5247/x`.
   - Controller route thật là `/api/telemetry/heartbeat`.

6. **Node identity chưa hoàn chỉnh**
   - Agent gửi `NodeId`.
   - `NodeStatus` không có `NodeId`.
   - Controller upsert bằng `NodeType`, không hỗ trợ nhiều OS Agent độc lập.

7. **Cấu hình `HeartbeatIntervalSeconds` chưa được sử dụng**
   - UI/DB lưu giá trị.
   - Agent luôn `sleep(5)`.

8. **`TcpCommandService.ExecuteAsync` là hosted stub**
   - Service được đăng ký background service nhưng loop chỉ delay 30 giây, không thực hiện polling/retry/reconciliation.

9. **Default policy hard-coded**
   - `danhSachLuat` khởi tạo với `{ -1, "eat_ram", 100000 }`.
   - Policy runtime không được persist từ Agent sang DB/file.

10. **Tài liệu/build chưa đầy đủ**
    - `README.md` chỉ có tiêu đề.
    - Không thấy project/build manifest C++ trong source tree; binary root là artifact/runtime được ignore.
    - `agent.conf` bị ignore nhưng có thể tồn tại local và chứa URL/token thực tế.

### Code cũ bị comment, không phải implementation active

- `tcp_server.cpp` chứa các phiên bản TCP server cũ trong `//` và `/* ... */`, gồm bản token đơn giản và bản dispatch trước khi bổ sung `RequireHmac`.
- `monitor.cpp` chứa nhiều phiên bản monitor cũ trong block comment, gồm logic `tenDangQuanLy`, `soKbMaxDungChung`, `hanhDong` và các action `Kill/Pause/Notify`. Các symbol này không còn trong `shared.h` active.
- Khi sửa tính năng, chỉ dùng implementation nằm ngoài các block comment cuối file; không coi code comment là contract hiện tại.

### Các giới hạn vận hành cần giữ khi mở rộng

- Không để monitor tác động PID 1, chính Agent hoặc UID 0 theo logic hiện tại.
- Giữ lock `khoa` khi đọc/ghi `danhSachLuat`.
- Giữ `m` khi truy cập `bang`.
- Giữ newline cuối TCP response để client hiện tại parse ổn định.
- Giữ `MAX_LENH = 4096`, timeout đọc 5 giây, và không đọc request vô hạn.
- Nếu thêm command mới, phải cập nhật đồng thời dispatch Agent, `TcpCommandService`, model/UI nếu có, và bảng contract này.
- Nếu thay đổi heartbeat payload, phải cập nhật `NodeStatus`/migration hoặc chấp nhận field bị ignore.
