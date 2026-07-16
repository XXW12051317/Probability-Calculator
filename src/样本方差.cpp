#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <cctype>
#include <limits>
#include <io.h>
#include <fstream>
#include <filesystem>
#include <windows.h>
#undef min
#undef max



class ExpressionParser
{
private:
	std::string expr; // 输入的表达式字符串
	size_t pos;// 当前解析位置

	// 添加一个错误上下文，帮助定位错误位置
	std::string getErrorContext() const {
		size_t context_start = (pos > 10) ? pos - 10 : 0;// 上下文起始位置
		size_t context_len = std::min((size_t)20, expr.length() - context_start);// 上下文长度
		std::string context = expr.substr(context_start, context_len);// 提取上下文

		// 标记错误位置
		size_t error_pos_in_context = (pos > 10) ? 10 : pos;// 错误位置在上下文中的位置
		std::string marker = std::string(error_pos_in_context, ' ') + "^";// 错误标记

		return "\n"+context + "\n" + marker + " 这里";// 返回带有错误标记的上下文信息
	}

	// 检查是否到达字符串末尾
	char peek() const{
		if (pos >= expr.size()) return '\0';// 返回空字符表示结束
		return expr[pos];// 返回当前字符
	}
	// 获取下一个字符并移动位置
	char get() {
		if (pos >= expr.size()) return '\0';// 返回空字符表示结束
		return expr[pos++];// 返回当前字符并移动位置
	}

	// 跳过空格
	void skipSpaces() {
		// 跳过所有空格字符
		while (pos < expr.size() && std::isspace(expr[pos])) {
			pos++;
		}
	}

	// 检查是否为数字字符（包括小数点和正负号）
	bool isNumberChar(char c) const {
		return std::isdigit(c) || c == '.' || c == '+' || c == '-';
	}

	// 解析数字
	double parseNumber() {
		skipSpaces();// 跳过空格
		size_t start = pos;// 记录数字起始位置

		// 处理可选的正负号
		if (peek() == '+' || peek() == '-') {
			// 检查这是否真的是数字的一部分（不是运算符）
			// 如果后面是数字或小数点，那么它是数字的一部分
			char next = (pos + 1 < expr.size()) ? expr[pos + 1] : '\0';// 查看下一个字符
			// 只有在下一个字符是数字或小数点时，才将正负号视为数字的一部分
			if (std::isdigit(next) || next == '.') {
				pos++;// 移动位置，包含正负号
			}
		}

		// 读取整数部分
		while (pos < expr.size() && std::isdigit(expr[pos])) {
			pos++;// 移动位置，包含数字字符
		}

		// 读取小数部分
		if (pos < expr.size() && expr[pos] == '.') {
			pos++;//	移动位置，包含小数点
			while (pos < expr.size() && std::isdigit(expr[pos])) {
				pos++;// 移动位置，包含小数部分数字
			}
		}
		// 检查是否真的读取到了数字
		if (pos == start) {
			// 没有读取到任何数字
			if (peek() == '\0') {
				throw std::runtime_error("期望数字但表达式已结束");// 特殊情况：表达式结束
			}
			else {
				throw std::runtime_error("期望数字但找到 '" + std::string(1, peek()) + "'" + getErrorContext());// 报错，指出找到的非数字字符
			}
		}

		std::string numStr = expr.substr(start, pos - start);// 提取数字字符串

		// 检查数字格式是否正确
		// 不能只有正负号
		if (numStr == "+" || numStr == "-") {
			throw std::runtime_error("数字格式错误：只有正负号没有数字");
		}
		// 不能只有小数点
		if (numStr == "." || numStr == "+." || numStr == "-.") {
			throw std::runtime_error("数字格式错误：小数点前没有数字");
		}
		// 不能以多个小数点结尾
		if (numStr.find('.') != std::string::npos) // 如果包含小数点
		{
			size_t last_dot = numStr.find_last_of('.');// 找到最后一个小数点位置
			if (last_dot != numStr.length() - 1 &&
				numStr.find('.', last_dot + 1) != std::string::npos) // 检查是否有多个小数点
			{
				throw std::runtime_error("数字格式错误：多个小数点");
			}
		}

		try {
			double value = std::stod(numStr);// 转换为双精度浮点数

			// 检查是否溢出
			if (std::isinf(value)) {
				throw std::runtime_error("数字超出表示范围（无穷大）");
			}
			// 检查是否为NaN
			if (std::isnan(value)) {
				throw std::runtime_error("计算结果不是数字");
			}
			// 返回解析的数字值
			return value;
		}
		catch (const std::invalid_argument&) {
			throw std::runtime_error("无效的数字格式: '" + numStr + "'");// 捕获无效格式错误
		}
		catch (const std::out_of_range&) {
			throw std::runtime_error("数字超出表示范围");// 捕获范围溢出错误
		}
	}

	// 解析括号或数字
	double parsePrimary() {
		skipSpaces();

		if (peek() == '(') {
			size_t left_paren_pos = pos;      //  记录左括号位置
			get();  // 跳过 '('
			double result = parseExpression();// 解析括号内的表达式
			skipSpaces();// 跳过空格
			if (peek() == ')') {
				get();  // 跳过 ')'
			}
			else {
				// 将 pos 回退到左括号位置，让错误标记指向 '('
				pos = left_paren_pos;
				throw std::runtime_error("缺少右括号");
			}
			return result;// 返回括号内的结果
		}
		// 检查是否为空表达式
		if (peek() == '\0') {
			throw std::runtime_error("表达式不完整：期望数字或括号但表达式已结束");
		}

		return parseNumber();// 解析数字
	}

	// 解析乘除运算
	double parseTerm() {
		double result = parsePrimary();// 解析第一个操作数(括号)

		while (true) // 循环处理乘除运算
		{
			skipSpaces();
			char op = peek();

			if (op == '*' || op == '/') {
				get();
				// 检查乘除号后面是否还有操作符
				skipSpaces();
				// 如果下一个字符是操作符，抛出错误
				if (peek() == '*' || peek() == '/' || peek() == '+' || peek() == '-') {
					throw std::runtime_error("运算符 '" + std::string(1, op) +
						"' 后面缺少操作数" + getErrorContext());
				}

				try {
					double right = parsePrimary();// 解析右操作数

					if (op == '*') {
						// 检查乘法溢出
						if (result != 0 &&
							std::abs(right) > std::numeric_limits<double>::max() / std::abs(result)) {
							throw std::runtime_error("乘法结果超出范围");
						}
						result *= right;
					}
					else {
						if (std::abs(right) < 1e-10) {
							throw std::runtime_error("除数不能为0");
						}
						result /= right;
					}
				}
				catch (const std::runtime_error& e) {
					// 重新抛出，但添加上下文
					throw std::runtime_error(std::string(e.what()) + " " + getErrorContext());
				}
			}
			else {
				break;
			}
		}

		return result;
	}

	// 解析加减运算
	double parseExpression() {
		double result = parseTerm();

		while (true) {
			skipSpaces();
			char op = peek();
			if (op == '+' || op == '-') {
				get();

				// 检查加减号后面是否还有操作符（除了正负号）
				skipSpaces();
				char next = peek();
				if (next == '*' || next == '/' || next == '\0') {
					throw std::runtime_error("运算符 '" + std::string(1, op) +
						"' 后面缺少操作数" + getErrorContext());
				}

				try {
					double right = parseTerm();// 解析右操作数

					if (op == '+') {
						// 检查加法溢出
						if (result > 0 && right > std::numeric_limits<double>::max() - result) {
							throw std::runtime_error("加法结果超出范围");
						}
						// 检查加法溢出（负数情况）
						if (result < 0 && right < std::numeric_limits<double>::lowest() - result) {
							throw std::runtime_error("加法结果超出范围");
						}
						result += right;
					}
					else {
						// 检查减法溢出
						if (result < 0 && right > std::numeric_limits<double>::max() + result) {
							throw std::runtime_error("减法结果超出范围");
						}
						// 检查减法溢出（正数情况）
						if (result > 0 && right < result - std::numeric_limits<double>::max()) {
							throw std::runtime_error("减法结果超出范围");
						}
						result -= right;
					}
				}
				catch (const std::runtime_error& e) {
					// 重新抛出，但添加上下文
					throw std::runtime_error(std::string(e.what()) + " " + getErrorContext());
				}
			}
			else {
				break;
			}
		}

		return result;
	}

public:
	// 构造函数
	ExpressionParser(const std::string& expr) : expr(expr), pos(0) {}
	// 评估表达式
	double evaluate() {
		double result = parseExpression();

		// 检查是否整个字符串都解析完了
		skipSpaces();
		if (pos < expr.size()) {
			throw std::runtime_error("表达式包含无法识别的字符: '" +
				expr.substr(pos) + "' " + getErrorContext());
		}

		return result;
	}
};

// 用户友好的错误信息转换 —— 上下文优先，解释独立，技术细节可选
std::string getFriendlyErrorMessage(const std::exception& e) {
	std::string error = e.what();
	std::string friendly;

	if (error.find("期望数字但找到") != std::string::npos) {
		size_t quote_pos = error.find("'");
		if (quote_pos != std::string::npos) {
			std::string found_char = error.substr(quote_pos);
			if (found_char.find("'*'") != std::string::npos)
				friendly = "连续的操作符错误：乘号后面不能直接跟其他操作符";
			else if (found_char.find("'/'") != std::string::npos)
				friendly = "连续的操作符错误：除号后面不能直接跟其他操作符";
			else if (found_char.find("'+'") != std::string::npos)
				friendly = "连续的操作符错误：加号后面不能直接跟其他操作符";
			else if (found_char.find("'-'") != std::string::npos)
				friendly = "连续的操作符错误：减号后面不能直接跟其他操作符";
			else
				friendly = "表达式错误：在需要数字的位置找到了操作符";
		}
		else {
			friendly = "表达式错误：在需要数字的位置找到了操作符";
		}
	}
	else if (error.find("表达式不完整") != std::string::npos) {
		friendly = "表达式不完整：可能缺少数字或右括号";
	}
	else if (error.find("缺少右括号") != std::string::npos) {
		friendly = "括号不匹配：缺少右括号";
	}
	else if (error.find("运算符后面缺少操作数") != std::string::npos) {
		if (error.find("'*'") != std::string::npos)
			friendly = "表达式错误：乘号(*)后面缺少数字";
		else if (error.find("'/'") != std::string::npos)
			friendly = "表达式错误：除号(/)后面缺少数字";
		else if (error.find("'+'") != std::string::npos)
			friendly = "表达式错误：加号(+)后面缺少数字";
		else if (error.find("'-'") != std::string::npos)
			friendly = "表达式错误：减号(-)后面缺少数字";
		else
			friendly = "表达式错误：操作符后面缺少数字";
	}
	else if (error.find("除数不能为0") != std::string::npos) {
		friendly = "数学错误：除数不能为零";
	}
	else if (error.find("数字超出表示范围") != std::string::npos) {
		friendly = "数字太大或太小，超出计算范围";
	}
	else if (error.find("无穷大") != std::string::npos) {
		friendly = "计算结果为无穷大";
	}
	else if (error.find("不是数字") != std::string::npos) {
		friendly = "计算结果不是有效数字";
	}
	else if (error.find("无效的数字格式") != std::string::npos) {
		friendly = "数字格式错误：请检查小数点和数字位置";
	}
	else if (error.find("表达式包含无法识别的字符") != std::string::npos) {
		friendly = "表达式包含不支持的操作符或字符";
	}
	else if (error.find("多个小数点") != std::string::npos) {
		friendly = "数字格式错误：一个数字中不能有多个小数点";
	}
	else if (error.find("数字格式错误") != std::string::npos) {
		// 这些错误本身已经足够友好，直接返回（不含上下文）
		return error;
	}
	else {
		friendly = "表达式错误";
	}

	
	if (error.find("^")!=std::string::npos) {

		return error + "\n错误提示：" + friendly;
	}
	else {
		return friendly+"(" +error + ")";
	}
}
// 检查表达式是否只包含可打印 ASCII 字符
bool isAsciiPrintable(const std::string & s) {
	for (unsigned char c : s) {
		if (c < 32 || c > 126)  // 控制字符或非 ASCII
			return false;
	}
	return true;
}

// 解析并计算输入的数字表达式
double parseNumber(const std::string& input)
{
	//if (!isAsciiPrintable(input)) {
	//	throw std::invalid_argument(
	//		"表达式只能使用英文符号、数字和运算符，不能包含中文或其他特殊符号"
	//	);
	//}

	try {
		ExpressionParser parser(input);
		return parser.evaluate();
	}
	catch (const std::exception& e) {
		throw std::invalid_argument(getFriendlyErrorMessage(e));
	}
}




/**
 * @brief 计算样本均值
 *
 * @param num1 所有数据的总和
 * @param i 样本数量
 * @return double 样本均值
 */

double AVG(double& num1, int& i) 
{
	
	if (i <= 0)//检查样本数量是否大于零
	{
		//throw std::invalid_argument("样本数量必须大于零");
		return 0;
	}
	return num1 / static_cast<double>(i);//返回样本均值
}

/**
 * @brief 计算样本方差
 *
 * 公式：S² = Σ(xi - μ)² / (n-1)
 *
 * @param nums 样本数据数组
 * @param avg 样本均值
 * @param i 样本数量
 * @return double 样本方差
 */
double S(std::vector<double>& nums, double& avg, int i)
{
	if(i<2)//检查样本数量是否大于等于2
	{
		return 0;
		//throw std::invalid_argument("样本数量必须大于等于2");
	}
	int n = 0;
	double sum = 0;
	for (n; n < i; ++n)
	{
		sum += pow(nums[n] - avg, 2);//计算每个样本与均值的差的平方并累加
	}
	return  sum/ static_cast<double>(i - 1);
}

double median(std::vector<double> nums, int i)
{
	std::sort(nums.begin(), nums.end());//对样本数据进行排序
	if (i % 2 == 1) //如果样本数量为奇数
	{
		return nums[i / 2];//返回中位数
	}
	else //如果样本数量为偶数
	{
		return (nums[i / 2 - 1] + nums[i / 2]) / 2.0;//返回中位数（两个中间值的平均）
	}
}

// 计算百分位数
double cP(std::vector<double> nums,double p)
{
	if(nums.empty())
	{
		return 0;
	}
	std::sort(nums.begin(), nums.end());//对样本数据进行排序
	double rank = p * nums.size();//计算百分位数的排名
	size_t index = static_cast<size_t>(rank);//将排名转换为索引
	if (index >= nums.size())
	{
		index = nums.size() - 1;//如果索引超出范围，返回最后一个元素
	}
	return nums[index];//返回对应百分位数的值

}

int readPositiveInteger(const std::string& prompt) {
	std::cout << prompt;
	int count = 0;
	while (true) {
		std::string line;
		std::getline(std::cin, line);

		size_t start = line.find_first_not_of(" \t\r\n");
		size_t end = line.find_last_not_of(" \t\r\n");
		if (start == std::string::npos) {
			std::cerr << "输入不能为空，请重新输入：";
			continue;
		}
		std::string trimmed = line.substr(start, end - start + 1);

		bool all_digit = true;
		for (char c : trimmed) {
			if (!std::isdigit(c)) {
				all_digit = false;
				break;
			}
		}
		if (!all_digit) {
			std::cerr << "样本数量必须为正整数，请重新输入：";
			continue;
		}

		count = std::stoi(trimmed);
		if (count <= 0) {
			std::cerr << "样本数量必须大于零，请重新输入：";
			continue;
		}
		break;
	}
	return count;   
}

std::string getExeDir()
{
	char path[MAX_PATH];

	GetModuleFileNameA(NULL, path, MAX_PATH);

	std::filesystem::path exe(path);

	return exe.parent_path().string();
}

int main()
{
	// ----- 检测是否在控制台环境下运行 -----
	bool is_console = _isatty(_fileno(stdin));
	// ----- 可选：根据环境调整输出行为（比如在非控制台环境下减少输出） -----
	//bool force_quiet = (argc > 1 && std::string(argv[1]) == "--quiet");

	bool quiet_mode = !is_console; // 如果不是控制台环境，默认启用安静模式

	int count = 0;
	// ----- 交互模式下提示用户输入样本数量 -----
	if (!quiet_mode)
	{
		std::cout << "交互模式：请输入要计算的数的个数，按回车确认。程序将逐个提示输入每个数的表达式（支持整数、小数、分数、负数，支持加减乘除，括号请用英文）。\n";
		count = readPositiveInteger("输入要计算的数的个数：\n");
		if (count <= 0)
		{
			std::cerr << "输入流错误，无法读取样本数量\n";
			return 1;
		}

	}
	// ----- 非交互模式下直接读取样本数量 -----
	else {
		if (!(std::cin >> count))
		{
			std::cerr << "输入流错误，无法读取样本数量\n";
			return 1;
		}
	}

	// ----- 创建数组（直接指定大小）-----
	std::vector<double> nums;
	nums.reserve(count);
	double sum = 0.0;
	// ----- 循环输入数据（重试版）-----
	if (quiet_mode)
	{
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		std::string file_path;
		std::getline(std::cin, file_path);
		

		std::ifstream infile(file_path);

		if (!infile.is_open())
		{
			std::cerr << "无法打开文件：" << file_path << std::endl;
			return 1;
		}

		double val;

		while (infile >> val)
		{
			nums.push_back(val);
			sum += val;
		}

		infile.close();

		count = static_cast<int>(nums.size());

		if (count == 0)
		{
			std::cerr << "没有读取到任何数据！" << std::endl;
			return 1;
		}
	}
	else
	{
		
		for (int i = 0; i < count; )   // 注意：i 在这里不自动递增
		{
			if (!quiet_mode)
			{
				std::cout << "请输入第 " << (i + 1) << " 个数的表达式：\n";
				std::string input;
				if (!(std::getline(std::cin, input)))
				{
					// 输入流错误（比如用户输入了EOF），必须恢复状态
					std::cin.clear();
					//std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
					std::cerr << "输入流错误，请重新输入第 " << i + 1 << " 个数\n";
					continue;   // 重试当前 i
				}
				try
				{
					double val = parseNumber(input);
					nums.push_back(val);
					sum += val;
					std::cout << "  -> " << input << " = " << val << std::endl;
					++i;   // 只有成功时才移动到下一个位置

				}
				catch (const std::exception& ex)
				{
					std::cerr << "[数据异常]" << "第" << (i + 1) << "个输入位置：";
					std::cerr << ex.what() << std::endl;
					// 恢复输入流状态，准备下一次输入
					if (std::cin.fail()) {
						std::cin.clear();
						std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
					}
					// i 不变，下一次循环会覆盖当前位置，实现“重新输入该数”
					continue;   // 重试当前 i

				}
			}
		
		}
	}
		// ----- 计算并输出结果 -----
		double avg = AVG(sum, count);
		std::cout << "样本均值: " << avg << std::endl;

		double s = S(nums, avg, count);
		std::cout << "样本方差: " << s << std::endl;

		double med = median(nums, count);
		std::cout << "样本中位数: " << med << std::endl;

		double p10 = cP(nums, 0.10);
		std::cout << "样本10百分位数: " << p10 << std::endl;
		double p25 = cP(nums, 0.25);
		std::cout << "样本25百分位数: " << p25 << std::endl;
		double p75 = cP(nums, 0.75);
		std::cout << "样本75百分位数: " << p75 << std::endl;
		double p90 = cP(nums, 0.90);
		std::cout << "样本90百分位数: " << p90 << std::endl;

		// ----- 程序结束前的暂停（仅在交互模式下）-----
		if (!quiet_mode)
		{
			// ----- 窗口暂停（Windows）-----
			std::cout << "请按任意键继续. . .";
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			std::cin.get();
		}

		std::cout << "===== 即将写入 results.csv =====" << std::endl;

		std::filesystem::path result_path =
			std::filesystem::path(getExeDir()) /
			"数据" /
			"results.csv";

		std::filesystem::create_directories(
			result_path.parent_path()
		);



		std::ofstream outfile(result_path);

		if (!outfile.is_open())
		{
			std::cerr << "无法创建文件：" << result_path << std::endl;
			return 1;
		}

		outfile << avg << "," << s << "," << med << ","
			<< p10 << "," << p25 << "," << p75 << "," << p90 << "\n";

		for (size_t i = 0; i < nums.size(); ++i)
		{
			outfile << nums[i];
			if (i != nums.size() - 1)
				outfile << ",";
		}

		outfile.close();

		std::cout << "保存成功：" << result_path << std::endl;
	}
