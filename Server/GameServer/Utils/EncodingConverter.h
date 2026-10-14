#pragma once
#include <string>
#include <Windows.h>

/**
 * UTF-8 string과 UTF-16 wstring 사이의 변환기.
 *
 * Windows 쪽이 쓰는 UTF-16 문자열과 나머지 코드가 쓰는 UTF-8 문자열 사이를 옮길 때 쓴다.
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

        // 버퍼 없이 부르면 필요한 크기만 돌려준다. 길이를 -1로 넘겼으므로 크기에 널 문자가 들어 있다.
        int bufferSize = WideCharToMultiByte(
            CP_UTF8,
            0,
            wstr,
            -1,
            NULL,
            0,
            NULL, NULL
        );

        if (bufferSize == 0)
        {
            return "";
        }

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

        // 널 문자를 잘라 낸다. 남기면 문자열 끝에 '\0'이 붙어 비교와 길이가 어긋난다.
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

        // 버퍼 없이 부르면 필요한 크기만 돌려준다. 길이를 -1로 넘겼으므로 크기에 널 문자가 들어 있다.
        int bufferSize = MultiByteToWideChar(
            CP_UTF8,
            0,
            str.c_str(),
            -1,
            NULL,
            0
        );

        if (bufferSize == 0)
        {
            return L"";
        }

        wstring result(bufferSize, 0);
        MultiByteToWideChar(
            CP_UTF8,
            0,
            str.c_str(),
            -1,
            &result[0],
            bufferSize
        );

        // 널 문자를 잘라 낸다. 남기면 문자열 끝에 L'\0'이 붙어 비교와 길이가 어긋난다.
        result.resize(bufferSize - 1);

        return result;
    }
};