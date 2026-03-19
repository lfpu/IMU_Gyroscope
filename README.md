# MPU6886 IMU → UDP → 3D 姿态显示

> **PlatformIO + ESP32 + MPU6886** 读取 IMU 数据，并通过 **自定义 TLV UDP 协议** 实时发送到 PC，在 PC 端进行 **3D 姿态可视化**。

📺 **PC 端 3D 显示项目**：https://github.com/lfpu/IMU_3DMonitor

---

## ✨ 项目简介

本项目演示了在 **PlatformIO** 环境下，使用 **ESP32** 读取 **MPU6886** 的：

- 加速度（Accelerometer）
- 陀螺仪（Gyroscope）
- 温度（Temperature）
- 设备 ID（WHO_AM_I）

并使用 **自定义 TLV（Type-Length-Value）UDP 协议**（`imu_protocol.h`），将 IMU 数据实时发送到 PC 端，用于 **3D 姿态模拟与显示**。

---

## 🧩 系统架构

```
MPU6886  -->  ESP32 (PlatformIO)
              |  I2C 读取
              |  TLV 打包 + CRC32
              v
           UDP 数据包
              v
        PC / IMU_3DMonitor
              v
         OpenGL 3D 姿态显示
```

---

## 🧰 硬件要求

- **ESP32**（ESP32-WROOM / M5StickC / ATOM 等）
- **MPU6886**（常见于 M5 系列开发板）
- Wi-Fi 网络

**I2C 连接（默认）**：

| 信号 | ESP32 GPIO |
|-----|------------|
| SDA | GPIO43 |
| SCL | GPIO44 |

> MPU6886 默认 I2C 地址：`0x68`  
> WHO_AM_I 常见返回值：`0x19`

---

## 📦 工程结构

```
.
├─ include/
│  └─ imu_protocol.h   # 自定义 TLV UDP 协议（与你的 PC 端一致）
├─ src/
│  └─ main.cpp         # IMU 采集 + UDP 发送
├─ platformio.ini      # PlatformIO 配置
└─ README.md
```

---

## 📡 自定义 TLV UDP 协议说明

- **Header**：固定 28 字节（magic / version / flags / device_id / seq / timestamp）
- **Payload**：若干 TLV 结构
- **Packet Tail**：CRC32（`esp_rom_crc32_le`，小端）

### 已使用 TLV 类型

| TLV | 内容 | 数据类型 |
|----|------|----------|
| `TLV_GYRO` | 陀螺仪 | `float[3]` |
| `TLV_ACCEL` | 加速度 | `float[3]` |
| `TLV_TEMP` | 温度 | `float` |

> 单位通过 Header 中的 `flags` 字段指示：
> - 加速度：`g` / `m/s²`
> - 角速度：`deg/s` / `rad/s`

---

## 🚀 快速使用

1. 安装 **VS Code + PlatformIO**
2. 修改 `platformio.ini` 中的参数：
   - `WIFI_SSID` / `WIFI_PASS`
   - `UDP_TARGET_IP` / `UDP_TARGET_PORT`
3. 编译并烧录到 ESP32
4. 运行 PC 端 **IMU_3DMonitor**
5. 摇动设备，即可看到 3D 姿态变化

---

## 🧪 调试建议

- 使用 **Wireshark** 抓取 UDP 数据，验证 TLV 与 CRC
- 串口监视器查看 `WHO_AM_I` 与 Wi-Fi 状态
- 确保 PC 与 ESP32 在同一网段，防火墙允许 UDP

---

## 📄 License

MIT License

---

## 🔗 相关项目

- **PC 端 3D 可视化**  
  https://github.com/lfpu/IMU_3DMonitor

---

如果你后续计划：
- 增加 **四元数（TLV_QUAT）**
- 加入 **传感器融合（Mahony / Madgwick）**
- 扩展为 **多 IMU 设备同时显示**

这个协议和工程结构都可以直接复用 👍
