# linux-monitor-c

A tiny Linux system monitor written in C.

## Features

- Shows CPU usage from `/proc/stat`
- Shows RAM usage from `/proc/meminfo`
- Shows network speed from `/proc/net/dev`
- Shows disk usage with `statvfs("/")`
- Shows listening TCP ports from `/proc/net/tcp`
- Supports custom refresh interval with `-d`

## Requiements

- Linux
- GCC or Clang
- Make, optional

## Build 

Compile directly:

```bash
gcc monitor.c -o monitor
```

Or use Makefile:

```bash
make
```

## Usage

Run with the default refresh interval:

```bash
./monitor
```

Set refresh interval to 2 seconds:

```bash
./monitor -d 2
```

## Example Output

```text
     cpu | 使用率: 4.46%
    cpu0 | 使用率: 4.61%
    cpu1 | 使用率: 4.30%
     RAM | 使用率: 48.16%
      lo | 接收速度: 0.00 B/s | 传输速度: 0.00 B/s
  enp0s5 | 接收速度: 0.00 B/s | 传输速度: 0.00 B/s
 docker0 | 接收速度: 0.00 B/s | 传输速度: 0.00 B/s
    DISK | 使用率: 22.22% | 已用/可用: 13.70GB/47.96GB
    PORT | TCP LISTEN: 22 53 631 30631 53
    PORT | TCP IPv6 LISTEN: 22 631
----------------------------------------
```

## Notes

This project is mainly designed for Linux because it reads data from the `/proc` filesystem.

CPU and network speed are calculated by comparing two samples, so the first output may show `information is collecting...`.

~~ License

MIT
