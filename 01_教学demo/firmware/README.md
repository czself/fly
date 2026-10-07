# 教学固件快照

这些BIN来自本教学项目各独立环境的已验证构建，校验见 [manifest.json](manifest.json)。修改源码后应重新编译，再运行`scripts/package_teaching.py`更新包。

`demo03_motor_bench.bin`会在规定按键门控后输出单电机PWM，仅供拆桨台架；`demo_selftest.bin`只读 EEPROM 且不包含电机 PWM。其他数值教学环境不启动电机PWM。每次下载会替换板上程序，按对应课教程选固件。遥控器不能用这些BIN，它需要`remote/tle100/tle100.hex`和USBASP。

建议直接在本项目使用`pio run -e <环境> -t upload`，它使用已验证的板载ISP序列。没有自动下载动作。教学软件模型和控制器预览不是真机参赛固件。
