#pragma once

/**
 * @file
 * @brief	�f�o�b�O���O�o��
 * @author	Um6r41 (���u���Ƃł�������)
 * @date 2023.10.05
 */

#define LOG(...) { _LogDbgView(__VA_ARGS__); }

void _LogDbgView(const char* fmt, ...);
