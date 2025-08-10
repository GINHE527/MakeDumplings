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
import os
import logging
import ctypes
from pathlib import Path
import webbrowser

# 初始化日志系统
logging.basicConfig(
    filename='app.log',
    level=logging.DEBUG,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger()

def show_error_message(message):
    """显示错误消息框"""
    ctypes.windll.user32.MessageBoxW(0, message, "错误", 0x10)

def resource_path(relative_path):
    """获取资源的绝对路径（适配打包和开发环境）"""
    try:
        if getattr(sys, 'frozen', False):
            base_path = getattr(sys, '_MEIPASS', os.path.dirname(sys.executable))
        else:
            base_path = os.path.dirname(os.path.abspath(__file__))
        return os.path.join(base_path, relative_path)
    except Exception as e:
        logger.error(f"资源路径解析失败: {str(e)}")
        raise

def read_custom_ini(file_path):
    """读取自定义INI配置文件"""
    try:
        config = configparser.ConfigParser()
        with open(file_path, 'r', encoding='utf-8') as f:
            ini_content = '[DEFAULT]\n' + f.read()
        config.read_string(ini_content)
        return config
    except Exception as e:
        logger.error(f"INI文件读取失败: {str(e)}")
        raise

def set_hidden_attribute(path):
    """设置文件/文件夹隐藏属性"""
    try:
        if os.path.exists(path):
            subprocess.run(
                f'attrib +h "{path}"',
                shell=True,
                check=True,
                creationflags=subprocess.CREATE_NO_WINDOW
            )
            logger.debug(f"已设置隐藏属性: {path}")
    except Exception as e:
        logger.warning(f"隐藏属性设置失败: {path} - {str(e)}")

def get_root_directory(install_path):
    """获取需要清理的根目录"""
    try:
        path = Path(install_path).resolve()
        parts = path.parts
        
        if len(parts) < 3 or not parts[1].startswith('.'):
            raise ValueError(f"路径格式必须为: 盘符\\.随机数目录\\...\n当前路径: {install_path}")
        
        root_dir = Path(parts[0]) / parts[1]
        return root_dir
    except Exception as e:
        logger.error(f"根目录解析失败: {str(e)}")
        raise

def validate_paths(zip_dir, zip_file):
    """验证ZIP目录和文件是否存在"""
    try:
        if not os.path.isdir(zip_dir):
            raise FileNotFoundError(f"ZIP目录不存在: {zip_dir}")
        if not os.path.isfile(zip_file):
            available_zips = [f for f in os.listdir(zip_dir) if f.endswith('.zip')]
            hint = f"可用ZIP文件: {available_zips}" if available_zips else "目录中没有ZIP文件"
            raise FileNotFoundError(f"ZIP文件不存在: {os.path.basename(zip_file)}\n{hint}")
    except Exception as e:
        logger.error(f"路径验证失败: {str(e)}")
        raise

def extract_with_progress(zip_file, install_path):
    """带进度条的解压函数"""
    try:
        with zipfile.ZipFile(zip_file, 'r') as zip_ref:
            file_list = zip_ref.infolist()
            for file in file_list:
                try:
                    zip_ref.extract(file, install_path)
                except Exception as e:
                    logger.warning(f"解压文件失败: {file.filename} - {str(e)}")
                    continue
    except Exception as e:
        logger.error(f"解压过程失败: {str(e)}")
        raise

def find_single_file(directory, pattern):
    """在目录中查找唯一匹配的文件"""
    files = list(Path(directory).rglob(pattern))
    if len(files) != 1:
        raise ValueError(f"必须包含且仅包含一个{pattern}文件，但找到 {len(files)} 个")
    return files[0]

def run_application(exe_path, install_path):
    """运行目标应用程序"""
    try:
        proc = subprocess.Popen(
            str(exe_path),
            cwd=str(install_path),
            shell=True,
            creationflags=subprocess.CREATE_NO_WINDOW
        )
        time.sleep(1)  # 等待程序启动
        return proc
    except Exception as e:
        logger.error(f"启动应用程序失败: {str(e)}")
        raise

def main():
    try:
        # 确保工作目录正确
        if getattr(sys, 'frozen', False):
            os.chdir(os.path.dirname(sys.executable))

        logger.info("程序启动")

        # 读取配置文件
        ini_path = resource_path('Name.ini')
        config = read_custom_ini(ini_path)
        name = config.get('DEFAULT', 'Name')
        install_path = config.get('DEFAULT', f"{name}_path")
        
        # 计算需要清理的根目录
        root_dir_to_clean = get_root_directory(install_path)
        logger.info(f"待清理根目录: {root_dir_to_clean}")

        # 构建资源路径
        zip_dir = resource_path('zip')
        zip_file = os.path.join(zip_dir, f"{name}.zip")
        exe_path = os.path.join(install_path, f"{name}.exe")

        # 验证路径
        validate_paths(zip_dir, zip_file)

        # 创建安装目录
        install_path_obj = Path(install_path)
        if install_path_obj.exists():
            logger.info(f"清理旧安装目录: {install_path}")
            shutil.rmtree(install_path_obj, ignore_errors=True)
        install_path_obj.mkdir(parents=True, exist_ok=True)

        # 设置根目录隐藏属性
        set_hidden_attribute(root_dir_to_clean)

        # 解压文件
        logger.info(f"从 [{zip_file}] 解压到 [{install_path}]")
        extract_with_progress(zip_file, install_path)

        # 定位唯一的HTML文件
        html_file = find_single_file(install_path, "*.html")
        html_relative = html_file.relative_to(install_path)
        htmlpath = str(html_relative).replace(os.path.sep, "/")

        # 启动程序
        exe_path_obj = Path(exe_path)
        if not exe_path_obj.exists():
            found_exes = list(Path(install_path).rglob("*.exe"))
            if found_exes:
                exe_path_obj = found_exes[0]
                logger.info(f"自动定位到可执行文件: {exe_path_obj}")

        logger.info(f"启动程序: {exe_path_obj}")
        proc = run_application(exe_path_obj, install_path_obj)
        
        # 打开浏览器
        webbrowser.open(f"http://localhost:8000/{htmlpath}")

        # 等待程序退出
        try:
            proc.wait()
        except KeyboardInterrupt:
            proc.terminate()
            logger.info("用户中断，已终止进程")

        # 清理根目录
        logger.info("程序退出，3秒后清理根目录...")
        time.sleep(3)
        if root_dir_to_clean.exists():
            logger.info(f"正在清理根目录: {root_dir_to_clean}")
            shutil.rmtree(root_dir_to_clean, ignore_errors=True)
            logger.info("清理完成")

    except Exception as e:
        logger.exception("程序发生致命错误")
        error_msg = f"程序运行出错: {str(e)}\n详细信息请查看日志文件: {os.path.abspath('app.log')}"
        show_error_message(error_msg)
        if not getattr(sys, 'frozen', False):
            input("按回车退出...")
        sys.exit(1)

if __name__ == "__main__":
    main()
'''



def create_beginpak():

    base_dir = Path(__file__).parent.absolute()
    file_path = base_dir / "BeginPak.py"
    
    with open(file_path, "w", encoding="utf-8") as f:
        f.write(BEGINPAK_CODE)

def prepare_resources():
    """验证并准备所有必需资源"""
    base_dir = Path(__file__).parent.absolute() 
    required = [
        'Name.ini',
        'zip/',  # 确保是目录
        'logo.png'
    ]
    
    missing = []
    for item in required:
        path =  base_dir / item
        if item.endswith('/'):
            if not path.is_dir():
                missing.append(str(path))  # 显示完整路径更易调试
        else:
            if not path.is_file():
                missing.append(str(path))
    
    if missing:
        raise FileNotFoundError(f"缺少必要资源: {', '.join(missing)}")

def embed_pyinstaller():
    """将PyInstaller嵌入到项目目录"""
    base_dir = Path(__file__).parent.absolute() 
    internal_dir = base_dir / "internal" 
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
    # 获取脚本所在目录的绝对路径
    script_dir = Path(__file__).parent.absolute()
    
    # 生成临时spec文件
    spec_content = f'''
# -*- mode: python ; coding: utf-8 -*-
block_cipher = None

a = Analysis(
    [r'{script_dir / "BeginPak.py"}'],
    pathex={[str(script_dir)]},
    binaries=[],
    datas=[
        (r'{script_dir / "Name.ini"}', '.'),
        (r'{script_dir / "zip"}/*', 'zip'),
        (r'{script_dir / "internal"}/*', 'internal'),
        (r'{script_dir / "logo.png"}', '.')
    ],
    hiddenimports=['PIL'],
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
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    icon=r'{script_dir / "logo.png"}'
)
'''
    spec_path = script_dir / "final.spec"
    with open(spec_path, "w", encoding="utf-8") as f:
        f.write(spec_content)

    # 配置环境变量
    env = os.environ.copy()
    env["PYTHONPATH"] = f"{script_dir / 'internal'}{os.pathsep}{env.get('PYTHONPATH', '')}"

    # 执行打包命令
    cmd = [
        sys.executable, "-m", "PyInstaller",
        str(spec_path),
        "--clean",
        "--noconfirm",
        f"--distpath={script_dir}",
        f"--workpath={script_dir / 'build'}"
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
        print(f"打包成功！生成文件: {script_dir / 'FinalApp.exe'}")
    except subprocess.CalledProcessError as e:
        print(f"打包失败，错误码: {e.returncode}")
        print("错误输出:", e.stdout)
        sys.exit(1)
    finally:
        # 清理临时文件（调试时可注释掉）
        clean_files = [spec_path, script_dir / "BeginPak.py"]
        for f in clean_files:
            if f.exists():
                f.unlink()
        build_dir = script_dir / "build"
        if build_dir.exists():
            shutil.rmtree(build_dir)
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
