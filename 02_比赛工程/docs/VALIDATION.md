# 比赛工程验证记录

日期：2026-10-03；范围为本目录独立构建与主机任务桥接测试。未进行烧录、遥控实机、测距/光流或飞行验收。

- `bash scripts/check_contest.sh`：C11 主机测试开启 AddressSanitizer 与 UndefinedBehaviorSanitizer；覆盖事件与心跳分离、STOP/LAND/START 优先级、事件过期、序号/时钟回绕、空闲校准限制、无线命令到控制器状态门控。PASS。
- `vofa.csv`：32 列，32 个初始值全部为 0。PASS。
- `python3 scripts/check_docs.py`：本项目内所有 Markdown 相对链接存在。PASS。
- `pio run -e contest_hover`：STM32F103RCT6 / ststm32 19.6.0 / stm32cube F1 1.8.6；Flash 32168 bytes、RAM 1904 bytes。PASS（最终重建结果见构建记录）。
- `bash scripts/build_remote.sh`：SDCC mcs51 编译并生成 `remote/tle100/tle100.hex`。PASS。

主机测试只使用限定数学输入，没有模拟设备替代真实无线验收。飞控链接产物不含电机 PWM 启动/占空比应用调用；实际输出启用通道恒 0。未装的新模块、未到的遥控器烧录器、安装坐标和光流补偿、电机旋向映射/供电、PID 与故障降落均标记待验收，不以编译成功代替真机证据。

最终完整命令输出见 [build_validation.log](build_validation.log)；固件校验见 [firmware_sha256.json](firmware_sha256.json)。

## 2026-10-07 回归

重新运行`bash scripts/check_contest.sh`：主机任务桥接测试、32列全零CSV、Markdown链接检查及`contest_hover`固件构建全部通过。验收脚本新增两项持续门禁：最终ELF不得包含`Board_MotorPwm`/`HAL_TIM_PWM_Start`，构建出的BIN必须与仓库中的`firmware/contest_hover.bin`逐字节一致，并通过`docs/firmware_sha256.json`校验。完整输出见 [2026-10-07回归日志](build_validation_2026-10-07.log)。当前结果仍只是代码/构建验证，没有烧录或实机传感器、无线、飞行验证。

使用SDCC 4.5.0重建比赛目录的TLE100发射端；其HEX与教学工程重建结果逐字节一致，且匹配本目录固件SHA256清单。构建记录见 [遥控固件重建日志](remote-build-2026-10-07.log)。
