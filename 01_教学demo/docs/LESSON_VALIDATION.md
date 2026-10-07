# 教学交付验证记录 · 2026-10-03

结论：教学代码、CSV和文档已交付；主机测试、9个F22教学环境及TLE100发射端构建通过。硬件尚未齐备，本次没有烧录/无线/光流/测距/飞行实测，不能称为悬停比赛飞控验收完成。

| 检查 | 结果 | 证据 |
|---|---|---|
| 遥控与心跳协议 | PASS，字符过期、CRC/噪声/半帧/重复与旧包、300ms链路超时、序号/计时回绕 | `tests/test_flight.c` |
| 单电机门控 | PASS，上电按住、释放、2s长按、3s超时、KEY2、重复触发与计时回绕 | 同上；只验证纯逻辑 |
| PID/控制器 | PASS，抗积分饱和、测量微分、NaN/Inf/dt、缺传感器、超高/倾斜、故障锁存、混控限幅 | 同上 |
| 软件悬停过程 | PASS，起飞→保持超过5s继续等待→显式降落→空闲；理想模型最大0.506m | 模型测试输出；不是真机高度 |
| ToF/光流传输 | PASS，型号、默认配置、字节序/负增量、质量、NACK/读写故障、旧测量、超时和回绕 | `tests/test_sensors.c`，假设备寄存器，不是实机 |
| 姿态/高度/光流数学 | PASS，FRD重力参考、yaw积分、倾角收敛、非法dt、斜距投影、向上高度差分、方向旋转/陀螺补偿与未标定门控 | 同上 |
| F22构建 | PASS，7课+遥控新协议与电机台架共9环境 | [构建日志](validation/host-and-build.log) |
| TLE100构建 | PASS，SDCC4.5.0，AT89S52 mcs51，代码583B，无外部RAM；Intel HEX可解析且校验正确 | `remote/tle100/tle100.hex`、`build/tle100.mem` |
| CSV与教程 | PASS，6份全零模板、列数/顺序、代码发帧数量、文档链接 | `scripts/check_teaching_assets.py` |
| 本次实机验证 | 待验证，无可用`/dev/ttyUSB*`，新硬件与烧录器未到 | 没有upload或电机运行操作 |

重复验证：

```bash
bash scripts/check_demos.sh
bash scripts/build_remote.sh
python3 scripts/check_teaching_assets.py
```

主机编译采用`-Wall -Wextra -Werror -pedantic`与AddressSanitizer/UndefinedBehaviorSanitizer；两组测试退出0。F22用现有PlatformIO ST STM32平台19.6.0、STM32CubeF1 1.8.6、ARM GCC7.2.1构建，各环境SUCCESS。当前无CH340端口提示只影响下载，不影响构建；没有编译器warning/error。

F22产物位于`.pio/build/<环境>/firmware.bin`；遥控HEX位于`remote/tle100/tle100.hex`。大小与校验见 [产物SHA256](validation/firmware_sha256.csv)。用户原采集`data/vofa+.csv`内容未改，原根目录保留兼容链接；模板各自放在demo目录。

已有实机传感器历史见 [VOFA历史记录](VOFA_VALIDATION.md) / [原自检记录](HANDOFF.md)，其不同固件阶段的结果不代替本次新demo验收。当前第1课为20通道；最新低通气压高度的软件构建并不意味着低空高度准确。

## 尚需到货/台架验证

新遥控协议两端下载、空闲心跳和断链，模块排母脚序/供电/使能，ToF尺测与状态/时效，光流质量/尺度/旋转补偿，IMU机体轴与采样周期，单电机停机电平/频率/看门狗、位置旋向和动力供电，以及真实控制闭环。第7课物理PWM恒0且安装/光流标志默认0，尚未接入无线任务控制。四/六轴实物布局未确认，虚拟四轴混控无M口映射。

试飞前需要设计起飞初段缺少光流时的地面参考、丢测/失联降落策略与真实调参；教学模型的故障清零只是软件门控，不是已验证的空中保护策略。

## 目录集中整理

教学工程整体移动到`01_教学demo`，重新运行9环境构建、两组主机测试、CSV/链接检查与遥控器重编译均通过。源码、算法行为和采集CSV内容未因目录调整改变。新增本地手册、总讲义与ZIP包；详细校验见 [整理记录](validation/整理记录.json) 和 [整理后的验证日志](validation/目录整理验证.log)。比赛工程是旁边独立项目，不从教学目录读取源文件。

## 2026-10-07：板载自检安全构建

用户要求重新尝试早期的开发板自检固件。源码仍位于`src/demo/demo_selftest/main.c`，环境为`demo_selftest`。本次将 EEPROM 写入和电机 PWM 测试设为关闭，并从该环境排除电机定时器驱动；构建后的 ELF 用`arm-none-eabi-nm`检查，不含`Board_MotorPwm`或`HAL_TIM_PWM_Start`符号。EEPROM仍做0x50–0x57地址探测，不修改内容。邻近I2C扫描示例也改为同一地址范围扫描，兼容板上A0/A1/A2焊盘配置。

主机逻辑/传感器替身测试和16个有效PlatformIO环境全部通过，教学资源检查通过。完整输出见 [本次构建日志](validation/selftest-safe-build-2026-10-07.log)。系统未发现`/dev/ttyUSB*`、`/dev/ttyACM*`或CH340设备，所以这次没有烧录，也没有读取当前实物板的传感器结果；板上诊断仍待串口重新连接后进行。
