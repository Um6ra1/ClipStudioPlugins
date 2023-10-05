/**
 * @file
 * @brief	デバッグログ出力
 * @author	Um6r41 (同志たつとでっかいの)
 * @date 2023.10.05
 */

#include <Windows.h>
#include <stdarg.h>
#include <string>

void _LogDbgView(const char* fmt, ...) {
	va_list arg;
	va_start(arg, fmt);

	char data[2048];
	vsprintf_s(data, fmt, arg);
	va_end(arg);

	OutputDebugStringA(data);
}
