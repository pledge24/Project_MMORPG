#pragma once
#include <string>
#include <Windows.h>

/**
 * UTF-8 string과 UTF-16 wstring 사이를 바꾼다. DB와 Windows API가 쓰는 wchar 문자열을
 * protobuf와 설정값의 UTF-8 문자열로 옮길 때 쓴다.
 * 상태가 없으므로 여러 스레드에서 불러도 된다.
 */
class EncodingConverter
{
public:
    /** wstr이 nullptr이거나 변환에 실패하면 빈 문자열을 돌려준다. */
    static string WCharToString(const wchar_t* wstr)
    {
        if (!wstr)
        {
            return "";
        }

        // 변환에 필요한 버퍼 크기 계산
        int bufferSize = WideCharToMultiByte(
            CP_UTF8,            // 대상 코드 페이지 (UTF-8)
            0,                  // 플래그
            wstr,               // 변환할 와이드 문자열
            -1,                 // 문자열의 길이 (-1은 널 종료 문자까지 계산)
            NULL,               // 변환 결과를 저장할 버퍼 (크기만 계산)
            0,                  // 버퍼 크기
            NULL, NULL          // 기본 문자, 기본 문자 사용 여부
        );

        if (bufferSize == 0)
        {
            return "";
        }

        // 버퍼 할당 및 변환 수행
        string result(bufferSize, 0);
        WideCharToMultiByte(
            CP_UTF8,
            0,
            wstr,
            -1,
            &result[0],
            bufferSize,
            NULL, NULL
        );

        // 마지막 널 문자를 제거 (std::string은 널 문자로 끝나지 않음)
        result.resize(bufferSize - 1);

        return result;
    }

    /** str은 UTF-8이어야 한다. 비어 있거나 변환에 실패하면 빈 문자열을 돌려준다. */
    static wstring StringToWString(const std::string& str)
    {
        if (str.empty())
        {
            return L"";
        }

        // 변환에 필요한 버퍼 크기 계산
        int bufferSize = MultiByteToWideChar(
            CP_UTF8,        // 대상 코드 페이지 (UTF-8)
            0,              // 플래그
            str.c_str(),    // 변환할 문자열
            -1,             // 문자열의 길이 (-1은 널 종료 문자까지 계산)
            NULL,           // 변환 결과를 저장할 버퍼 (크기만 계산)
            0               // 버퍼 크기
        );

        if (bufferSize == 0)
        {
            // 오류 처리. GetLastError()를 통해 자세한 오류 정보 확인 가능
            return L"";
        }

        // 버퍼 할당 및 변환 수행
        wstring result(bufferSize, 0);
        MultiByteToWideChar(
            CP_UTF8,
            0,
            str.c_str(),
            -1,
            &result[0],
            bufferSize
        );

        // 마지막 널 문자를 제거 (std::wstring은 널 문자로 끝나지 않음)
        result.resize(bufferSize - 1);

        return result;
    }
};