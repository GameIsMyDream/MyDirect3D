#pragma once

/**
 * Direct3D 公用程序类。
 */
class Direct3DUtility
{
public:
	// 将ANSI字符串转换为宽字符串
	static inline std::wstring StringToWString(const std::string String)
	{
		WCHAR Buffer[512] = { 0 };

		MultiByteToWideChar(CP_ACP, 0, String.c_str(), -1, Buffer, 512);

		return std::wstring(Buffer);
	}
};

/**
 * Direct3D 异常类。
 */
class DxException
{
public:
	DxException() = default;
	DxException(HRESULT InitErrorCode, const std::wstring InitFunctionName, const std::wstring InitFileName, unsigned int InitLineNumber);

public:
	std::wstring ToString() const; // 获取异常的字符串描述。

private:
	HRESULT ErrorCode;			// 错误码
	std::wstring FunctionName;	// 函数名
	std::wstring FileName;		// 文件名
	unsigned int LineNumber;	// 行号
};

#ifndef THROW_IF_FAILED
#define THROW_IF_FAILED(x)																				\
{																										\
	HRESULT HardwareResult = (x);																		\
	if (FAILED(HardwareResult))																			\
	{																									\
		throw DxException(HardwareResult, L#x, Direct3DUtility::StringToWString(__FILE__), __LINE__);	\
	}																									\
}
#endif

#ifndef RELEASE_COM
#define RELEASE_COM(x)	\
{						\
	if(x)				\
	{					\
		x->Release();	\
		x = 0;			\
	}					\
}
#endif
