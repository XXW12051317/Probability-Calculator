import tkinter as tk  # 导入 tkinter 模块用于创建图形界面
import os  # 导入 os 模块用于文件路径操作
import subprocess  # 导入 subprocess 模块用于调用外部程序
import sys
import pandas  # 导入 pandas 模块用于数据处理
import re  # 导入 re 模块用于正则表达式处理
from unittest import result  # 导入 sys 模块用于系统相关操作
from logging import root
from tkinter import (
    messagebox,
    filedialog,
    scrolledtext,
    simpledialog,
    ttk
)  # 导入 messagebox 模块用于显示消息框,filedialog 模块用于文件对话框,scrolledtext 模块用于带滚动条的文本框
import matplotlib.pyplot as plt  # 导入 matplotlib.pyplot 模块用于绘图
import numpy as np  # 导入 numpy 模块用于数值计算

# 定义一个函数获取当前目录，优先获取可执行文件所在目录
def get_exe_path():
    if getattr(sys, "frozen", False):  # 如果程序被打包成可执行文件
        return os.path.dirname(sys.executable)  # 返回当前可执行文件所在目录
    try:  # 获取当前脚本所在目录
        return os.path.dirname(os.path.abspath(__file__))  # 返回当前脚本所在目录
    except NameError:
        return (
            os.getcwd()
        )  # 返回当前工作目录，覆盖之前的脚本目录，确保在某些环境中也能找到可执行文件

def get_data_dir():
    data_dir = os.path.join(get_exe_path(), "数据")
    os.makedirs(data_dir, exist_ok=True)
    return data_dir

def clean_full_dataframe(df):
    def convert_units(val):
        res = str(val).upper().replace('￥','').replace("$","").replace(",","").strip()
        units = {"K":1e3,"M":1e6,"B":1e9,"T":1e12}
        for unit,multiplier in units.items():
            if res.endswith(unit):
                try:
                    number_part = res[:-1].strip()
                    return str(float(number_part)*multiplier)
                except:
                    return res
        try:
                return float(res)
        except:
                return res

    for col in df.columns:
        df[col] = df[col].apply(convert_units)
    df = df.replace('nan',np.nan).dropna(how='all')
    return df
    
def choose_column(df):
    """弹出窗口让用户选择分析列"""

    win = tk.Toplevel(root)
    win.title("选择分析列")
    win.geometry("320x150")
    win.grab_set()      # 模态窗口

    tk.Label(
        win,
        text="请选择需要分析的列：",
        font=("微软雅黑", 11)
    ).pack(pady=10)

    columns = list(df.columns)

    combo = ttk.Combobox(
        win,
        values=columns,
        state="readonly",
        width=25
    )

    combo.current(0)
    combo.pack()

    result = {"column": None}

    def ok():
        result["column"] = combo.get()
        win.destroy()

    tk.Button(
        win,
        text="确定",
        command=ok,
        width=10
    ).pack(pady=15)

    win.wait_window()

    return result["column"]

# 定义点击文件按钮的回调函数
def click_file_button():
    # 打开文件对话框让用户选择文件，设置标题和文件类型过滤器
    file_path = filedialog.askopenfilename(
        title="选择输入文件", filetypes=[("文件", "*.txt *.xlsx *.xls *.csv")]
    )  # 打开文件对话框让用户选择输入文件，设置标题和文件类型过滤器，允许选择文本文件和 Excel 文件
    if not file_path:  # 如果用户选择了文件
        messagebox.showwarning("警告", "未选择文件，请重新选择！")  # 显示警告信息
        return
    input_str = ""  # 初始化输入字符串
    if os.path.exists(file_path):  # 如果文件存在
        try:
            if file_path.lower().endswith((".xlsx", ".xls")):  # 如果文件是 Excel 文件
                # 以 header=None 读取，后面智能判断首行是否为表头
                df = pandas.read_excel(file_path)
                

            elif file_path.lower().endswith(".csv"):  # 如果文件是 CSV 文件
                # 以 header=None 读取，然后智能判断首行是否为表头
                df = pandas.read_csv(
                    file_path, header=None
                )  # 使用 pandas 读取 CSV 文件
               
            elif file_path.lower().endswith(".txt"):  # 如果文件是文本文件
                with open(
                    file_path, "r", encoding="utf-8"
                ) as f:  # 使用 with 语句安全地打开文件，确保文件在使用后正确关闭,file_path 是用户选择的文件路径，'r' 表示以只读模式打开文件，encoding='utf-8' 指定文件编码为 UTF-8
                    lines = [
                        line.strip() for line in f.readlines() if line.strip()
                    ]  # 使用 readlines() 方法将文件内容按行读取为列表，并去除空行和两端空白
                    count = len(lines)  # 计算表达式数量
                    input_str = f"{count}\n{file_path}\n"  # 构建输入字符串，格式为：表达式数量\n文件路径\n
            if file_path.lower().endswith((".xlsx", ".xls", ".csv")):

                df_cleaned = clean_full_dataframe(df)
                column = choose_column(df_cleaned)

                if column is None:
                    return

                data = pandas.to_numeric(
                df_cleaned[column],
                errors="coerce"
                ).dropna()

                data_dir = get_data_dir()
                result_path = os.path.join(data_dir, "results.csv")

                os.makedirs(data_dir, exist_ok=True)

                temp_txt = os.path.join(
                        data_dir,
                        "temp_input.txt"
                    )  # 构建临时文本文件路径

                np.savetxt(
                 temp_txt,
                 data.to_numpy(),
                  fmt="%.15g"
                )  # 将数据保存为临时文本文件，格式为浮点数，保留15位有效数字

                input_str = f"{len(data)}\n{temp_txt}\n"
               
                
            if not input_str:  # 如果输入字符串为空
                messagebox.showwarning(
                    "警告", "文件内容为空，请检查文件是否正确！"
                )  # 显示警告信息
        except Exception as e:
            messagebox.showerror(
                "错误", f"处理文件时发生错误：{e}"
            )  # 显示文件处理错误信息

        data_dir = get_data_dir()
        exe_path = os.path.join(
           get_exe_path(), "样本方差等概率论计算器.exe"
        )  # 构建可执行文件路径
        print("调用的 exe：", exe_path)
        if not os.path.exists(exe_path):  # 如果可执行文件不存在
            messagebox.showerror(
                "错误",
                f"找不到可执行文件，请检查路径是否正确！有以下解决方法：\n\t1.关闭窗口，把可执行文件（exe）文件放在：{get_exe_path()}（推荐,一劳永逸）\n\t2.在弹窗选择文件窗口中选择可执行文件",
            )  # 显示错误信息
            exe_path = filedialog.askopenfilename(
                title="选择可执行文件", filetypes=[("可执行文件", "*.exe")]
            )  # 打开文件对话框让用户选择可执行文件
        if not exe_path:  # 如果用户没有选择可执行文件
            messagebox.showwarning(
                "警告", "没有可执行文件怎么计算喵~^v^"
            )  # 显示警告信息
            return
        # 调用可执行文件并传递输入字符串，捕获输出和错误信息
        btn1.config(state=tk.DISABLED)  # 开始前禁用按钮
        try:
            root.config(cursor="watch")  # 设置鼠标指针为等待状态，表示正在计算
            root.update()  # 更新主窗口，确保界面响应
            process = subprocess.run(
                [exe_path],  # 调用可执行文件
                input=input_str,  # 将准备好的输入字符串传递给 C++ 程序的标准输入
                capture_output=True,  # 捕获标准输出和标准错误，以便后续处理和调试
                text=True,  # 指定输入输出为文本模式（字符串），而不是字节
                encoding="gbk",  # 指定编码为 gbk，以正确处理 C++ 程序输出的中文字符
            )
            result_text.delete(1.0, tk.END)  # 清空旧内容
            if process.stdout:
                result_text.insert(tk.END, process.stdout)
            if process.stderr:
                # 错误信息用弹窗显示（更醒目）
                messagebox.showerror("计算错误", process.stderr)
            if process.returncode != 0 and not process.stderr:
                # 程序异常退出但没有错误输出
                result_text.insert(
                    tk.END, f"程序异常退出，返回码：{process.returncode}"
                )
        except Exception as e:
            messagebox.showerror(
                "错误", f"调用计算引擎时发生错误：{e}"
            )  # 显示调用错误信息

        finally:
            get_result()
            root.config(cursor="")  # 恢复默认光标
            btn1.config(state=tk.NORMAL)  # 计算结束后重新启用按钮
            root.update()
            

    else:  # 如果文件不存在
        messagebox.showerror("错误", "文件不存在，请检查路径是否正确！")  # 显示错误信息


# 交互输入
def click_manual_button():
    data_dir = get_data_dir()
    exe_path = os.path.join(
        get_exe_path(), "样本方差等概率论计算器.exe"
    )  # 构建可执行文件路径
    # 优先在新控制台中启动可执行文件，确保交互式 stdin/stdout 可用（适用于 Windows）
    try:
        if os.path.exists(exe_path):  # 如果可执行文件存在
            if os.name == "nt":
                subprocess.Popen(
                    [exe_path], creationflags=subprocess.CREATE_NEW_CONSOLE
                )
            else:
                subprocess.Popen([exe_path])
            return
    except Exception as e:
        messagebox.showerror("错误", f"启动可执行文件时发生错误：{e}")

    # 如果默认位置没有找到可执行文件，提示并让用户选择
    messagebox.showerror(
        "错误",
        f"找不到可执行文件，请检查路径是否正确！有以下解决方法：\n\t1.关闭窗口，把可执行文件（exe）文件放在：{data_dir}（推荐,一劳永逸）\n\t2.在弹窗选择文件窗口中选择可执行文件",
    )
    exe_path = filedialog.askopenfilename(
        title="选择可执行文件", filetypes=[("可执行文件", "*.exe")]
    )  # 打开文件对话框让用户选择可执行文件
    if not exe_path:  # 如果用户没有选择可执行文件
        messagebox.showwarning("警告", "没有可执行文件怎么计算喵~^v^")  # 显示警告信息
        return
    btn2.config(state=tk.DISABLED)  # 开始前禁用按钮
    try:
        if os.name == "nt":
            subprocess.Popen([exe_path], creationflags=subprocess.CREATE_NEW_CONSOLE)
        else:
            subprocess.Popen([exe_path])
    except Exception as e:
        messagebox.showerror("错误", f"启动可执行文件时发生错误：{e}")
    finally:
        get_result()
        btn2.config(state=tk.NORMAL)  # 计算结束后重新启用按钮
        

        # 界面部分


def get_result():
    data_dir = get_data_dir()

    result_path = os.path.join(
        data_dir,
        "results.csv"
    )

    print("读取：", result_path)

    if not os.path.exists(result_path):
        messagebox.showerror("错误", f"找不到结果文件：\n{result_path}")
        return

    stats_df = pandas.read_csv(result_path, nrows=1, header=None)

    print(result_path)
    stats_df = pandas.read_csv(result_path, nrows=1,header=None)
    mean = stats_df.iloc[0, 0]  # 获取均值
    var = stats_df.iloc[0, 1]  # 获取方差
    med = stats_df.iloc[0, 2]  # 获取中位数
    p10 = stats_df.iloc[0, 3]  # 获取10百分位数
    p25 = stats_df.iloc[0, 4]  # 获取25百分位数
    p75 = stats_df.iloc[0, 5]  # 获取75百分位
    p90 = stats_df.iloc[0, 6]  # 获取90百分位数

    data_df = pandas.read_csv(result_path, skiprows=1, header=None)
    raw_data = data_df.iloc[0, : ].dropna().tolist()  # 获取原始数据，去除空值

    std_dev = np.sqrt(var) if var is not None else None# 计算标准差，如果方差不为 None 则取平方根，否则为 None
    if mean is not None and var is not None:
        plt.figure(figsize=(11, 6))#创建一个新的图形，设置大小为 8x5 英寸
        plt.hist(raw_data, bins=1000, color='skyblue', edgecolor='black',label='原始数据')#绘制原始数据的直方图，设置柱子数量为 15，颜色为天蓝色，边框颜色为黑色
        plt.axvline(mean, color='red', linestyle='solid', linewidth=2, label=f'均值: {mean:.2f}')#在图中添加一条垂直线表示均值，颜色为红色，线型实线，宽度为 2，并添加标签显示均值数值
        plt.axvline(mean + std_dev, color='green', linestyle='dashed', linewidth=2, label=f'均值 + 1σ: {mean + std_dev:.2f}')#在图中添加一条垂直线表示均值加一个标准差，颜色为绿色，线型为虚线，宽度为 2，并添加标签显示数值
        plt.axvline(mean - std_dev, color='orange', linestyle='dashed', linewidth=2, label=f'均值 - 1σ: {mean - std_dev:.2f}')#在图中添加一条垂直线表示均值减一个标准差，颜色为橙色，线型为虚线，宽度为 2，并添加标签显示数值
        plt.axvline(mean + 2*std_dev, color='green', linestyle='dotted', linewidth=2, label=f'均值 + 2σ: {mean + 2*std_dev:.2f}')#在图中添加一条垂直线表示均值加两个标准差，颜色为绿色，线型为点线，宽度为 2，并添加标签显示数值
        plt.axvline(mean - 2*std_dev, color='orange', linestyle='dotted', linewidth=2, label=f'均值 - 2σ: {mean - 2*std_dev:.2f}')#在图中添加一条垂直线表示均值减两个标准差，颜色为橙色，线型为点线，宽度为 2，并添加标签显示数值
        plt.axvline(mean + 3*std_dev, color='green', linestyle='dashdot', linewidth=2, label=f'均值 + 3σ: {mean + 3*std_dev:.2f}')#在图中添加一条垂直线表示均值加三个标准差，颜色为绿色，线型为点划线，宽度为 2，并添加标签显示数值
        plt.axvline(mean - 3*std_dev, color='orange', linestyle='dashdot', linewidth=2, label=f'均值 - 3σ: {mean - 3*std_dev:.2f}')#在图中添加一条垂直线表示均值减三个标准差，颜色为橙色，线型为点划线，宽度为 2，并添加标签显示数值
        plt.axvline(p10, color='blue', linestyle='solid', linewidth=2, label=f'10百分位数: {p10:.2f}')#在图中添加一条垂直线表示10百分位数，颜色为蓝色，线型实线，宽度为 2，并添加标签显示数值
        plt.axvline(p25, color='blue', linestyle='dashed', linewidth=2, label=f'25百分位数: {p25:.2f}')#在图中添加一条垂直线表示25百分位数，颜色为蓝色，线型为虚线，宽度为 2，并添加标签显示数值
        plt.axvline(med, color='purple', linestyle='solid', linewidth=2, label=f'中位数: {med:.2f}')#在图中添加一条垂直线表示中位数，颜色为紫色，线型实线，宽度为 2，并添加标签显示数值
        plt.axvline(p75, color='blue', linestyle='dotted', linewidth=2, label=f'75百分位数: {p75:.2f}')#在图中添加一条垂直线表示75百分位数，颜色为蓝色，线型为点线，宽度为 2，并添加标签显示数值
        plt.axvline(p90, color='blue', linestyle='dashdot', linewidth=2, label=f'90百分位数: {p90:.2f}')#在图中添加一条垂直线表示90百分位数，颜色为蓝色，线型为点划线，宽度为 2，并添加标签显示数值
        plt.axvspan(mean - std_dev, mean + std_dev, color='yellow', alpha=0.5, label='±1σ 区域')#在图中添加一个矩形区域表示均值加减一个标准差的范围，颜色为黄色，透明度为 0.3，并添加标签显示该区域
        plt.axvspan(mean - 2*std_dev, mean + 2*std_dev, color='yellow', alpha=0.3, label='±2σ 区域')#在图中添加一个矩形区域表示均值加减两个标准差的范围，颜色为黄色，透明度为 0.1，并添加标签显示该区域
        plt.axvspan(mean - 3*std_dev, mean + 3*std_dev, color='yellow', alpha=0.1, label='±3σ 区域')#在图中添加一个矩形区域表示均值加减三个标准差的范围，颜色为黄色，透明度为 0.1，并添加标签显示该区域
        plt.title('原始数据分布',fontproperties='SimHei',fontsize=14)#设置图表标题
        plt.xlabel('数值(X)',fontproperties='SimHei',fontsize=12)#设置 x 轴标签
        plt.ylabel('频数(Frequency)',fontproperties='SimHei',fontsize=12)#设置 y 轴标签
        plt.legend(bbox_to_anchor=(0.999, 1), loc='upper left', prop={'family': 'SimHei', 'size': 10},borderpad = 0.8,frameon=True,shadow=True)#显示图例，设置位置为右上角，使用中文字体，添加边框和阴影
        plt.subplots_adjust(right=0.85)  # 调整子图参数，增加右侧空白以防止图例遮挡内容
        plt.show()#显示图形
    else:
        messagebox.showwarning("警告", "无法加载结果数据，请检查文件路径和内容！")#显示警告信息

       


root = tk.Tk()  # 创建主窗口
root.title("样本方差等概率论计算器V4.0")  # 设置窗口标题
root.geometry("700x500")  # 设置窗口大小

# 创建标签并设置字体
label = tk.Label(
    root, text="兄弟，请选择你的输入方式：", font=("微软雅黑", 12)
)  # 创建标签
label.pack(
    pady=20
)  # 将标签添加到窗口并设置垂直间距,pack方法是tkinter中用于布局的常用方法之一，使用它可以将组件添加到窗口中，并且可以设置组件之间的间距和对齐方式。

# 创建按钮并设置字体和点击事件
btn1 = tk.Button(
    root,
    text="从文件读取",
    font=("微软雅黑", 12),
    command=click_file_button,
    width=20,
    height=2,
)  # 创建按钮1，设置文本、字体和点击事件
btn1.pack(pady=10)  # 将按钮1添加到窗口并设置垂直间距

# 创建按钮并设置字体和点击事件
btn2 = tk.Button(
    root,
    text="交互性输入",
    font=("微软雅黑", 12),
    command=click_manual_button,
    width=20,
    height=2,
)  # 创建按钮2，设置文本、字体和点击事件
btn2.pack(pady=10)  # 将按钮2添加到窗口并设置垂直间距

result_frame = tk.Frame(root)  # 创建一个新的框架用于显示结果
result_frame.pack(
    pady=10, fill=tk.BOTH, expand=True
)  # 将结果框架添加到窗口并设置垂直间距，填充方式为 BOTH，允许扩展

result_label = tk.Label(
    result_frame, text="计算结果：", font=("微软雅黑", 10)
)  # 创建结果标签，设置文本和字体
result_label.pack(anchor=tk.W)  # 将结果标签添加到结果框架，并设置对齐方式为左对齐

result_text = scrolledtext.ScrolledText(
    result_frame, wrap=tk.WORD, width=80, height=15, font=("微软雅黑", 10)
)  # 创建带滚动条的文本框，设置换行方式和字体
result_text.pack(
    fill=tk.BOTH, expand=True
)  # 将文本框添加到结果框架，并设置填充方式为 BOTH，允许扩展


root.mainloop()  # 进入主循环，等待用户操作

