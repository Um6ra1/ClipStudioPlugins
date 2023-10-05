/**
 * @file
 * @brief	�f�o�b�O���O�o��
 * @author	Um6r41 (���u���Ƃł�������)
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
