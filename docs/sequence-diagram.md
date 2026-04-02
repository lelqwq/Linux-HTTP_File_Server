# 项目时序图

本文档用 [Mermaid](https://mermaid.js.org/) 描述 **服务启动** 与 **单次 HTTP 请求** 的主要调用顺序。在支持 Mermaid 的编辑器（如 VS Code、GitHub、GitLab）中可预览图形。

---

## 1. 服务启动

```mermaid
sequenceDiagram
    autonumber
    participant Main as main.c
    participant Stats as server_stats
    participant Conf as server_config
    participant EV as libevent<br/>(event_base)
    participant Timer as 统计定时器
    participant L as evconnlistener

    Main->>Stats: init_server_stats()
    Main->>Conf: load_config("server.conf")
    Main->>Main: 拼接日志路径、chdir(root_dir)
    Main->>EV: event_base_new()
    Main->>EV: event_new / event_add<br/>(stats_timer_cb → print_server_stats)
    Main->>L: evconnlistener_new_bind<br/>(cb_listener, port)
    Main->>EV: event_base_dispatch() 阻塞
    Note over EV: 此后由事件循环驱动<br/>定时写统计日志、接受新连接
```

---

## 2. 新连接建立

```mermaid
sequenceDiagram
    autonumber
    participant Client as 客户端 TCP
    participant L as evconnlistener
    participant EV as event_base
    participant CB as event_callbacks<br/>(cb_listener)
    participant BE as bufferevent

    Client->>L: 三次握手完成
    L->>CB: cb_listener(fd, sa, …)
    CB->>BE: bufferevent_socket_new<br/>bufferevent_setcb(cb_read_browser, …, cb_client_close)
    CB->>BE: bufferevent_set_timeouts / setwatermark / enable(EV_READ)
    Note over BE: 读就绪时触发 cb_read_browser
```

---

## 3. 单次请求处理（以 GET 静态文件为例）

```mermaid
sequenceDiagram
    autonumber
    participant BE as bufferevent
    participant CB as cb_read_browser
    participant HD as http_dispatch
    participant Line as http_line_reader
    participant URL as url_decode
    participant PG as path_guard
    participant HH as http_headers
    participant HR as http_handler
    participant FS as 文件系统

    BE->>CB: 可读事件
    CB->>HD: http_dispatch_on_read(bev)
    HD->>Line: bufferevent_read_crlf_line() 读请求行
    HD->>HD: sscanf 解析 method / path / protocol
    opt 含查询串 video=
        HD->>URL: url_decode(视频路径)
    end
    HD->>PG: path_guard_resolve_under_root<br/>(文档路径、根目录)
    PG->>FS: realpath 等
    FS-->>PG: 规范化路径
    PG-->>HD: 通过 / 403 / 404
    HD->>HH: parse_http_headers(bev)
    HH->>Line: bufferevent_read_crlf_line() 逐行读头
    HD->>HR: http_request(GET, …)
    HR->>FS: stat(fs_path)
    FS-->>HR: 普通文件
    HR->>HR: get_header_value(Range)、http_parse_range_header
    HR->>HR: send_head()、send_file_range()
    HR->>BE: evbuffer 写出响应
    Note over BE: Connection: close，随后连接关闭走 cb_client_close
```

---

## 4. 连接关闭（示意）

```mermaid
sequenceDiagram
    participant BE as bufferevent
    participant CC as cb_client_close
    participant Stats as server_stats

    BE->>CC: BEV_EVENT_EOF / ERROR / TIMEOUT
    CC->>CC: free(client_addr)、bufferevent_free
    CC->>Stats: current_connections--
    opt BEV_EVENT_ERROR
        CC->>Stats: total_errors++
    end
```

---

## 说明

- **统计定时器**与请求处理在 **同一 `event_base`** 上异步交错执行，未单独画成交互，避免图形过于拥挤。
- **目录列表**、**仅请求行失败**、**非 GET** 等分支与上图主路径类似，仅在 `http_dispatch` / `http_request` 内提前返回或走 `send_error`、`send_dir`。
