#include "MyPch.h"
#include "Direct3DUtility.h"
#include <comdef.h>

using namespace std;

DxException::DxException(HRESULT InitErrorCode, const std::wstring InitFunctionName, const std::wstring InitFileName, unsigned int InitLineNumber) :
	ErrorCode(InitErrorCode),
	FunctionName(InitFunctionName),
	FileName(InitFileName),
	LineNumber(InitLineNumber)
{

}

std::wstring DxException::ToString() const
{
	_com_error Error(ErrorCode);

	return FunctionName + L" failed in " + FileName + L", line " + to_wstring(LineNumber) + L", error: " + Error.ErrorMessage();
}
