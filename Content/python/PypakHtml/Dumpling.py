#Copyright GINHE, Inc. All Rights Reserved.
import sys
import os
import subprocess
import shutil
import configparser
import zipfile
import time
from tqdm import tqdm
import webbrowser
from pathlib import Path


BEGINPAK_CODE = r'''
#Copyright GINHE, Inc. All Rights Reserved.
#Auto generated ,don't change it.
import sys
import configparser
import zipfile
import shutil
import subprocess
import time
from tqdm import tqdm
from pathlib import Path
import webbrowser

def resource_path(relative_path):
    if getattr(sys, 'frozen', False) and hasattr(sys, '_MEIPASS'):
        base_path = sys._MEIPASS
    else:
        base_path = os.path.abspath(".")
    return os.path.join(base_path, relative_path)

def read_custom_ini(file_path):
    config = configparser.ConfigParser()
    with open(file_path, 'r', encoding='utf-8') as f:
        ini_content = '[DEFAULT]\n' + f.read()
    config.read_string(ini_content)
    return config

def set_hidden_attribute(path):
    try:
        subprocess.run(f'attrib +h "{path}"', shell=True, check=True)
        #print(f"已设置隐藏属性: {path}")
    except subprocess.CalledProcessError as e:
        print(f"警告: 隐藏属性设置失败 ({path}), 错误: {str(e)}")
    except Exception as e:
        print(f"未知错误: {str(e)}")

def get_root_directory(install_path):
    path = Path(install_path).resolve()
    parts = path.parts
    
    if len(parts) < 3 or not parts[1].startswith('.'):
        raise ValueError(f"路径格式必须为: 盘符\\.随机数目录\\...\n当前路径: {install_path}")
    
    root_dir = Path(parts[0]) / parts[1]
    return root_dir

def validate_paths(zip_dir, zip_file):
    if not os.path.isdir(zip_dir):
        raise FileNotFoundError(f"ZIP目录不存在: {zip_dir}")
    if not os.path.isfile(zip_file):
        available_zips = [f for f in os.listdir(zip_dir) if f.endswith('.zip')]
        hint = f"可用ZIP文件: {available_zips}" if available_zips else "目录中没有ZIP文件"
        raise FileNotFoundError(f"ZIP文件不存在: {os.path.basename(zip_file)}\n{hint}")

def main():
    try:
        ini_path = resource_path('Name.ini')
        config = read_custom_ini(ini_path)
        name = config.get('DEFAULT', 'Name')
        install_path = config.get('DEFAULT', f"{name}_path")
        
        # 计算需要清理的根目录
        root_dir_to_clean = get_root_directory(install_path)
        #print(f"待清理根目录: {root_dir_to_clean}")
        # 构建资源路径（适配打包环境）
        zip_dir = resource_path('zip')
        zip_file = os.path.join(zip_dir, f"{name}.zip")
        exe_path = os.path.join(install_path, f"{name}.exe")
        # 验证路径
        validate_paths(zip_dir, zip_file)
        # 创建安装目录
        install_path_obj = Path(install_path)
        if install_path_obj.exists():
            #print(f"清理旧安装目录: {install_path}")
            shutil.rmtree(install_path_obj)
        install_path_obj.mkdir(parents=True, exist_ok=True)

        # 设置根目录隐藏属性
        if root_dir_to_clean.exists():
            set_hidden_attribute(root_dir_to_clean)
        else:
            print(f"根目录不存在，无法设置隐藏: {root_dir_to_clean}")

        # 解压文件
        #print(f"从 [{zip_file}] 解压到 [{install_path}]")
        with zipfile.ZipFile(zip_file, 'r') as zip_ref:
            file_list = zip_ref.infolist()
            with tqdm(total=len(file_list), desc="解压进度", unit="文件") as pbar:
                for file in file_list:
                    zip_ref.extract(file, install_path)
                    pbar.update(1)

         # 定位唯一的HTML文件
        html_files = list(Path(install_path).rglob("*.html"))
        if len(html_files) != 1:
            raise ValueError(f"必须包含且仅包含一个HTML文件，但找到 {len(html_files)} 个")
        html_relative = html_files[0].relative_to(install_path)
        htmlpath = str(html_relative).replace(os.path.sep, "/")

        # 启动程序
        exe_path_obj = Path(exe_path)
        if not exe_path_obj.exists():
            found_exes = list(Path(install_path).rglob("*.exe"))
            if found_exes:
                exe_path_obj = found_exes[0]
                #print(f"自动定位到可执行文件: {exe_path_obj}")
        
        #print(f"启动程序: {exe_path_obj}")
        proc = subprocess.Popen(str(exe_path_obj), cwd=str(install_path_obj), shell=True)
        time.sleep(0.5)
        #htmlpath 位于解压的压缩包内的html文件，保证html文件有且只有一个
        webbrowser.open(f"http://localhost:8000/{htmlpath}")

        try:
            proc.wait()
        except KeyboardInterrupt:
            proc.terminate()
            #print("已终止进程")

        # 清理根目录
        #print("程序退出，3秒后清理根目录...")
        time.sleep(3)
        if root_dir_to_clean.exists():
            #print(f"正在清理根目录: {root_dir_to_clean}")
            shutil.rmtree(root_dir_to_clean, ignore_errors=True)
            #print("清理完成")
        else:
            print(f"根目录已不存在: {root_dir_to_clean}")

    except Exception as e:
        #print(f"操作失败: {str(e)}")
        if not getattr(sys, 'frozen', False):
            input("按回车退出...")
        else:
            time.sleep(5)

if __name__ == "__main__":
    print("Copyright GINHE, Inc. All Rights Reserved.")
    print("The Server begin to open")
    main()
'''



def create_beginpak():
    # 强制覆盖写入BeginPak.py
    with open("BeginPak.py", "w", encoding="utf-8") as f:
        f.write(BEGINPAK_CODE)

def prepare_resources():
    """验证并准备所有必需资源"""
    required = [
        'Name.ini',
        'zip/',  # 确保是目录
        'logo.png'
    ]
    
    missing = []
    for item in required:
        path = Path(item)
        if not os.path.exists(item):
            # 特殊处理目录检查
            if item.endswith('/') and not path.is_dir():
                missing.append(item)
            elif not item.endswith('/') and not path.is_file():
                missing.append(item)
    
    if missing:
        raise FileNotFoundError(f"缺少必要资源: {', '.join(missing)}")

def embed_pyinstaller():
    """将PyInstaller嵌入到项目目录"""
    internal_dir = Path("internal")
    pyinstaller_path = internal_dir / "PyInstaller"
    if not pyinstaller_path.exists():
        print("正在安装PyInstaller到本地目录...")
        # 创建目标目录
        internal_dir.mkdir(exist_ok=True)
        
        # 使用pip安装到指定目录
        result = subprocess.run(
            [sys.executable, "-m", "pip", "install",
             "--target", str(internal_dir),
             "--no-compile", "pyinstaller"],
            capture_output=True,
            text=True
        )
        if result.returncode != 0:
            print("PyInstaller安装失败！")
            print("标准输出:", result.stdout)
            print("错误输出:", result.stderr)
            sys.exit(1)

def build_final_exe():
    """构建最终的可执行文件"""
    # 生成临时spec文件
    spec_content = f'''
# -*- mode: python ; coding: utf-8 -*-
block_cipher = None

a = Analysis(
    ['BeginPak.py'],
    pathex={[os.getcwd()]},
    binaries=[],
    datas=[
        ('Name.ini', '.'),
        ('zip/*', 'zip'),
        ('internal/*', 'internal')
    ],
    hiddenimports=[],
    hookspath=[],
    hooksconfig={{}},
    runtime_hooks=[],
    excludes=[],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
    noarchive=False,
)
pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.zipfiles,
    a.datas,
    [],
    name='FinalApp',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=True,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)
'''
    with open("final.spec", "w", encoding="utf-8") as f:
        f.write(spec_content)

    # 配置环境变量
    env = os.environ.copy()
    env["PYTHONPATH"] = f"internal{os.pathsep}{env.get('PYTHONPATH', '')}"

    # 执行打包命令
    cmd = [
        sys.executable, "-m", "PyInstaller",
        "final.spec",
        "--clean",
        "--noconfirm",
        "--distpath=.",
        "--workpath=build"
    ]
    
    try:
        print("开始打包，详细日志如下：")
        result = subprocess.run(
            cmd,
            check=True,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )
        print(result.stdout)
        print("打包成功！生成文件: FinalApp.exe")
    except subprocess.CalledProcessError as e:
        print(f"打包失败，错误码: {e.returncode}")
        print("错误输出:", e.stdout)
        sys.exit(1)
    finally:
        # 清理临时文件（调试时可注释掉）
        clean_files = ["final.spec", "BeginPak.py"]
        for f in clean_files:
            if Path(f).exists():
                Path(f).unlink()
        if Path("build").exists():
            shutil.rmtree("build")

def main():
    # 在 main() 开头添加
    
    # 准备阶段
    try:
        create_beginpak()
        prepare_resources()
        embed_pyinstaller()
        
        # 构建最终程序
        build_final_exe()
    except Exception as e:
        print(f"发生未预期的错误: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    main()