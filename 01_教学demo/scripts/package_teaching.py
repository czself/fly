#!/usr/bin/env python3
"""Bundle the teaching project, manuals, CSVs and current validated firmware."""
from pathlib import Path
import hashlib,json,shutil,zipfile
root=Path(__file__).resolve().parents[1]
firmware=root/'firmware';firmware.mkdir(exist_ok=True)
envs=['demo01_vofa','demo02_remote','demo02_remote_framed','demo03_motor','demo03_motor_bench','demo04_hover_logic','demo05_tof','demo06_flow','demo07_integration']
records=[]
for name in envs:
    source=root/'.pio/build'/name/'firmware.bin'
    if not source.is_file():raise SystemExit('请先运行check_demos.sh，缺少产物：'+name)
    dest=firmware/(name+'.bin');shutil.copyfile(source,dest)
    records.append({'environment':name,'file':dest.name,'bytes':dest.stat().st_size,
                    'sha256':hashlib.sha256(dest.read_bytes()).hexdigest()})
(firmware/'manifest.json').write_text(json.dumps(records,ensure_ascii=False,indent=2)+'\n')
(firmware/'README.md').write_text('''# 教学固件快照

这些BIN来自本教学项目各独立环境的已验证构建，校验见 [manifest.json](manifest.json)。修改源码后应重新编译，再运行`scripts/package_teaching.py`更新包。

`demo03_motor_bench.bin`会在规定按键门控后输出单电机PWM，仅供拆桨台架；其他数值教学环境不启动电机PWM。每次下载会替换板上程序，按对应课教程选固件。遥控器不能用这些BIN，它需要`remote/tle100/tle100.hex`和USBASP。

建议直接在本项目使用`pio run -e <环境> -t upload`，它使用已验证的板载ISP序列。没有自动下载动作。教学软件模型和控制器预览不是真机参赛固件。
''')
archive=root.parent/'教学资料.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
    for p in sorted(root.rglob('*')):
        rel=p.relative_to(root)
        if '.pio' in rel.parts or '__pycache__' in rel.parts:continue
        if rel.parts[:3]==('remote','tle100','build'):continue
        if not p.is_file():continue
        z.write(p,Path(root.name)/rel)
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    for name in envs:assert f'{root.name}/firmware/{name}.bin' in z.namelist()
    assert f'{root.name}/教学总讲义.md' in z.namelist()
print('已生成：',archive,'大小',archive.stat().st_size,'字节；含9份BIN及完整教学资料')
