# F22 悬停比赛工程

这是之后集中开发和验收参赛固件的**独立工程目录**。教学例程已经分开；本目录自带飞控源码、遥控器源码与 HEX、CSV、接线表、比赛规则和厂家手册，复制整个目录即可构建。

**当前可用版本：真实传感器与无线任务联调，实际电机 PWM 恒为 0。尚不能直接用于比赛飞行。**主程序没有启动 PWM，也没有可打开的飞行输出开关；板载电机 HAL 不参与本环境构建。光流/ToF、遥控烧录器尚未到货，安装坐标、电机位置/旋向、供电与 PID 均待实机验收。

## 文件与构建

| 文件 | 作用 |
|---|---|
| [src/main.c](src/main.c) | 唯一飞控入口：真实 IMU、ToF、光流，无线任务、UART4 遥测 |
| [src/config.h](src/config.h) | IMU 安装矩阵、光流比例/轴映射/旋转补偿、验证标志 |
| src/flight | 六轴姿态、高度/速度观测、任务桥接、PID、悬停状态机与虚拟四轴混控 |
| src/sensors | PMW3901 SPI2、VL53L1X I²C2 驱动及来源许可证 |
| [vofa.csv](vofa.csv) | 32 通道全零模板，供本版本导入 VOFA |
| [firmware/contest_hover.bin](firmware/contest_hover.bin) | 本版本联调固件，不输出电机；不是已验收的实飞固件 |
| [remote/tle100](remote/tle100/README.md) | TLE100 AT89S52 发射端源码和 tle100.hex |
| [docs/MODULE_WIRING.md](docs/MODULE_WIRING.md) | 已买 ATK-PMW3901 4m 模块转接线 |
| [docs/COMPETITION.md](docs/COMPETITION.md) | 比赛规则映射；原 PDF 在 docs/references |
| [docs/RADIO_PROTOCOL.md](docs/RADIO_PROTOCOL.md) | 无线帧格式、心跳、按键与超时 |

```bash
cd /data/Downloads/f22_hal_demos/02_比赛工程
pio run -e contest_hover
bash scripts/check_contest.sh
bash scripts/build_remote.sh
```

这些命令仅测试和编译，不烧录。完成相应接线验收后，飞控烧录命令为 `pio run -e contest_hover -t upload`，保留 F22 已验证的 CH340 自动 ISP 复位顺序；运行联调时 USB 只供电，不在板载 CH340 发任何串口字节。遥控器用 USBASP/ProgISP 下载 `remote/tle100/tle100.hex`，不能用上述 F22 命令。

## 无线与串口

飞控 E49：M0 PA6、M1 PA7 均为低，USART1 PA9/PA10 为 9600 8N1；主程序只在此口接收协议帧，不发送日志。遥控器两端频率、地址、信道与 UART 配置须一致。原厂 F/B/L/R 单字符固件与此版本不兼容，必须使用本目录发射端 HEX。

F2 起飞、F3 降落、F4 急停、F1 空闲校准；板载 KEY1/KEY2 都只急停，起飞必须用无线命令。START 仅在 IDLE 且输入条件合格时起效；达到 0.5m 后 HOLD，稳定超过 5 秒只显示资格标记，继续等待 LAND。LAND 在起飞/保持期间允许提前请求；STOP 清空虚拟输出。连续心跳间隔约 26ms，300ms 没有新有效包则链路无效。队列保存字节到达时刻，后续心跳不会清掉任务事件，也不会让过期的 START 重新有效；同批任务以 STOP、LAND、START 的优先级处理。

校准只在空闲执行，要求约 0.5s 加速度模长 0.9～1.1g 且各陀螺绝对值低于 2°/s。校准或传感器重连期间丢弃积压无线请求，操作完成后重新按键。无传感器/未验证坐标时 START 进入 FAULT，用 STOP 回到 IDLE。

VOFA 使用**独立 3.3V USB-TTL**：JP14 PC10/TX → 转接器 RX，GND → GND；115200 / JustFloat / 无流控。只连 TX 和 GND，不从该转接器给板子供电。板载 CH340 和 E49 共用 USART1，不能在 USART1 的 9600 遥控口同时看这份 115200 遥测。USB-TTL 的电压选择须与 3.3V IO 对应。

导入 [vofa.csv](vofa.csv)：CH0 状态；1～3 roll/pitch/相对 yaw；4～5 高度和向上速度；6～9 本地 XY 位置/速度；10～12 光流计数/质量；13～17 通信/测量/安装/标定有效位；18～19 目标高度/示例推力；20～23 虚拟 FL/FR/RR/RL；24 实际 PWM=0；25～26 稳定秒数/超过 5s；27 无线链路；28 最后协议命令；29 有效包数；30 UART 错误数；31 输出使能=0。无有效测量的观测量为 NaN；CSV 初值为 0 不代表测量有效。

## 实飞前仍需完成

先按 [手册现状](docs/HARDWARE_OVERVIEW.md) 和 [模块接线](docs/MODULE_WIRING.md) 验证通信，再验证 `src/config.h` 的 IMU 正交旋转矩阵与光流平移/旋转补偿。两个标志默认 0；未经实验不得直接改为 1。当前 XY 积分参考为姿态初始化时的本地航向，起飞捕获当前位置作为目标；六轴 yaw 仍会漂移，光流不识别地面红圈。

当前模型 START 要求高度≤10cm，而光流要求高度≥8cm，未实现实机地面零点、传感器安装高度偏移和离地初段的观测切换。50Hz 循环仅用于联调，读取和重连均有延迟；需另行设计稳定的高速角速度内环、观测时序和落地识别。示例混控仅虚拟四轴 FL/FR/RR/RL，不知道实际 M 口对应关系，也不能用于未确认的六轴布局。

模型的数据失效/无线失联会进入 FAULT 并清空虚拟需求；**这不是已经验证的空中失联降落策略**。添加真实动力控制前必须完成机架/旋向与 PWM 极性、悬停推力/PID、姿态闭环、地面起飞、失效处置和动力供电测试，再在符合比赛牵引绳要求的场地验收。目标 20～100cm、连续悬停超过 5s、无线一键起降及落点评分以原方案为准。

本目录本批未烧录、未输出电机、未做真机悬停验收。构建和主机测试只证明代码可编译以及限定输入下的逻辑，不证明飞行能力；结果记录见 [验证记录](docs/VALIDATION.md)。
