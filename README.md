# Linux HTTP 静态文件服务器

基于 **C 语言** 与 **libevent** 的简易 HTTP/1.1 服务器，适用于在 **Linux** 下提供静态资源访问、目录索引与视频分段传输（Range）。使用 **GCC** 与 **make** 构建。

## 功能概览

- **GET** 静态文件：按扩展名返回 `Content-Type`，支持 **HTTP Range**（`206 Partial Content`），便于视频拖拽与分段加载。
- **目录列表**：访问目录时返回 HTML 索引页（类型、名称、大小、修改时间）；根路径 `/` 映射为资源根目录下的 `.`。
- **URL 解码**：路径支持百分号编码（如中文文件名）。
- **路径安全**：通过 `realpath` 将请求路径约束在资源根目录内，越界返回 `403`。
- **视频页**：资源中存在 `video/video_player.html` 时，目录列表中的 `.mp4` 会链到 `/video_player.html?video=…`；请求目标解析与路径校验在 `src/http/http_dispatch.c`。
- **连接与统计**：读超时、读缓冲区水位；原子计数器统计当前连接数、总请求数、发送字节数、错误数；可选按间隔写入统计日志。

## 依赖

- Linux（使用 `linux/limits.h` 等）
- GCC
- libevent（开发包，链接 `-levent`）

Debian/Ubuntu 示例：

```bash
sudo apt-get install build-essential libevent-dev
```

## 编译与运行

```bash
make          # 在 obj/<子目录>/ 生成 .o，在项目根目录生成 server.out
make clean    # 删除整个 obj/ 目录与 server.out
```

在**项目根目录**启动（程序会读取当前目录下的 `server.conf`）：

```bash
./server.out
```

启动前会将进程工作目录切换到配置中的 `root_dir`，因此静态文件均相对于该目录解析。

## 配置文件 `server.conf`

采用 `键=值` 形式（每行一项）。以 `#` 开头的行为注释。

| 配置项 | 说明 |
|--------|------|
| `root_dir` | 静态资源根目录的**绝对路径**。若目录不存在或无法进入，`chdir` 失败，进程退出（**不会**自动创建目录）。 |
| `port` | 监听端口。 |
| `max_connections` | 已写入配置结构体；**当前版本未在监听/接入逻辑中限流**，可视为预留项。 |
| `timeout_seconds` | 客户端 `bufferevent` 读超时（秒），超时后触发超时事件并断开。 |
| `enable_directory_listing` | 已解析；**当前版本未分支关闭目录列表**，访问目录时仍会生成索引页。 |
| `enable_stats_logging` | 非 0 时，按 `stats_logging_interval` 间隔向 `log_file` **追加**统计快照。 |
| `log_file` | 统计日志路径；若为相对路径，会基于启动时**项目根目录的当前工作目录**拼成绝对路径后再写入。 |
| `stats_logging_interval` | 统计写入间隔（秒）。 |
| `enable_connection_info` | 非 0 时在标准输出打印新连接、对端关闭、错误与超时等信息。 |

修改 `root_dir` 后请确保该路径在目标机器上存在且可读。

## 源码结构

`src/` 下按职责分子目录；头文件仍集中在 `include/`，编译通过 `-I include` 解析。

| 目录 | 文件 | 作用 |
|------|------|------|
| `src/app/` | `main.c` | 程序入口：信号、配置加载、`event_base`、定时统计、`evconnlistener` |
| `src/config/` | `server_config.c` | 解析 `server.conf` |
| `src/event/` | `event_callbacks.c` | 仅 libevent 胶水：监听、连接、`bufferevent` 回调；读事件转交 `http_dispatch_on_read` |
| `src/http/` | `http_dispatch.c` | 读请求行、URL→本地路径、视频参数、`realpath` 根目录校验、解析头后调用 `http_request` |
| `src/http/` | `http_handler.c` | GET 资源处理：响应头、分段发送、目录 HTML、错误页 |
| `src/http/` | `http_headers.c` | 解析请求头、`get_header_value` |
| `src/http/` | `http_range.c` | 解析 `Range: bytes=…` |
| `src/http/` | `http_line_reader.c` | 从 `bufferevent` 按行读取 |
| `src/util/` | `path_guard.c` | `realpath` + 文档根边界校验（供 dispatch 复用） |
| `src/util/` | `url_decode.c` | URL 百分号解码 |
| `src/util/` | `file_mime.c` | 按扩展名推断 `Content-Type`（`file_mime_lookup`） |
| `src/stats/` | `server_stats.c` | 原子统计与定时日志 |
| `include/` | `*.h` | 与上表对应的声明 |

## HTTP 行为摘要

- 仅实现 **GET**；其他方法返回 `405 Method Not Allowed`。
- 响应头中带 `Connection: close`，处理完即关闭连接。
- 视频类 `Content-Type` 会附加 `Accept-Ranges: bytes` 等头，便于播放器行为一致。

## 许可证与声明

若仓库未单独提供许可证文件，默认以仓库所有者声明为准。早期说明文字中不严谨之处已在本 README 中按当前代码行为做了对齐；后续若实现连接数上限或“关闭目录列表”开关，只需在文档中同步更新上表即可。
