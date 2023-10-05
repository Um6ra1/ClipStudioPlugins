#pragma once

/**
 * @file
 * @brief	デバッグログ出力
 * @author	Um6r41 (同志たつとでっかいの)
 * @date 2023.10.05
 */

#define LOG(...) { _LogDbgView(__VA_ARGS__); }

void _LogDbgView(const char* fmt, ...);
