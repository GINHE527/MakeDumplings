#Copyright GINHE, Inc. All Rights Reserved.

import sys
import subprocess
import importlib

def check_and_install_packages():
    # 需要检测的第三方库列表（包名: 导入名）
    required_packages = {
        'tqdm': 'tqdm',
        'Pillow': 'PIL'  # 安装包名为Pillow，导入名为PIL
    }

    for pkg, import_name in required_packages.items():
        try:
            importlib.import_module(import_name)
            print(f"[+] 库 '{import_name}' 已存在")
        except ImportError:
            print(f"[-] 库 '{import_name}'(包名: {pkg}) 未安装，正在通过清华源安装...")
            try:
                subprocess.check_call([
                    sys.executable,
                    '-m',
                    'pip',
                    'install',
                    '--user',
                    pkg,
                    '-i', 'https://mirrors.tuna.tsinghua.edu.cn/pypi/web/simple'
                ])
                print(f"[✓] 库 '{pkg}' 安装成功")
            except subprocess.CalledProcessError:
                print("Failed")
                sys.exit(1)

if __name__ == "__main__":
    check_and_install_packages()
    print("Python Allright")