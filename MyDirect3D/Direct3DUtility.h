#pragma once

/**
 * Direct3D ���ó����ࡣ
 */
class Direct3DUtility
{
public:
	// ��ANSI�ַ���ת��Ϊ���ַ���
	static inline std::wstring StringToWString(const std::string String)
	{
		WCHAR Buffer[512] = { 0 };

		MultiByteToWideChar(CP_ACP, 0, String.c_str(), -1, Buffer, 512);

		return std::wstring(Buffer);
	}
};

/**
 * Direct3D �쳣�ࡣ
 */
class DxException
{
public:
	DxException() = default;
	DxException(HRESULT InitErrorCode, const std::wstring InitFunctionName, const std::wstring InitFileName, unsigned int InitLineNumber);

public:
	std::wstring ToString() const; // ��ȡ�쳣���ַ���������

private:
	HRESULT ErrorCode;			// ������
	std::wstring FunctionName;	// ������
	std::wstring FileName;		// �ļ���
	unsigned int LineNumber;	// �к�
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
