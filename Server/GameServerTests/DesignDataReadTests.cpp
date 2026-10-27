#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Data/Generated/DesignData.h"

/*--------------------------------------------------------------
    생성된 역직렬화 테스트

    DataBuilder가 생성한 헤더의 읽기 함수가 형식(필드의 유무와 타입, 열거형 값 이름)을
    확인하는지 본다. 지금 생성된 표가 쓰지 않는 타입(목록, 선택 값, 열거형 목록)도 여기서
    인스턴스화해서, 그 타입을 처음 쓰는 표가 생길 때 빌드가 깨지지 않게 한다.

    픽스처 결합도: 없음.
---------------------------------------------------------------*/

namespace
{
    template<typename T>
    T ReadField(const Json& value)
    {
        return DesignData::Detail::Read<T>(Json{{"Field", value}}, "Field");
    }

    template<typename T>
    string ReadError(const Json& value)
    {
        try
        {
            ReadField<T>(value);
        }
        catch (const DesignData::Detail::FieldError& error)
        {
            return error.cause;
        }

        return "";
    }
}

TEST(DesignDataReadTest, ScalarsAreReadByType)
{
    EXPECT_EQ(ReadField<int32>(7), 7);
    EXPECT_EQ(ReadField<int64>(int64(1) << 40), int64(1) << 40);
    EXPECT_FLOAT_EQ(ReadField<float>(1.5), 1.5f);
    EXPECT_FLOAT_EQ(ReadField<float>(2), 2.f) << "정수로 적힌 실수도 읽는다";
    EXPECT_EQ(ReadField<string>("골드"), "골드");
    EXPECT_TRUE(ReadField<bool>(true));
}

TEST(DesignDataReadTest, WrongTypeIsRejected)
{
    EXPECT_NE(ReadError<int32>("7").find("정수가 아니다"), string::npos);
    EXPECT_NE(ReadError<int32>(1.5).find("정수가 아니다"), string::npos);
    EXPECT_NE(ReadError<int32>(int64(1) << 40).find("범위"), string::npos);
    EXPECT_NE(ReadError<string>(1).find("문자열이 아니다"), string::npos);
    EXPECT_NE(ReadError<bool>(1).find("불이 아니다"), string::npos);
}

TEST(DesignDataReadTest, MissingFieldIsRejected)
{
    try
    {
        DesignData::Detail::Read<int32>(Json::object(), "Id");
        FAIL() << "없는 필드를 읽었다";
    }
    catch (const DesignData::Detail::FieldError& error)
    {
        EXPECT_NE(error.cause.find("'Id'가 없다"), string::npos) << error.cause;
    }
}

TEST(DesignDataReadTest, EnumIsReadByValueName)
{
    EXPECT_EQ(ReadField<DesignData::CurrencyType>("GOLD"), DesignData::CurrencyType::GOLD);
    EXPECT_NE(ReadError<DesignData::CurrencyType>("RUBY").find("열거형 값이 아니다"), string::npos);
    EXPECT_NE(ReadError<DesignData::CurrencyType>(1).find("열거형 값이 아니다"), string::npos) << "열거형은 번호가 아니라 이름으로 싣는다";
}

TEST(DesignDataReadTest, ListsAndOptionalValues)
{
    EXPECT_EQ(ReadField<vector<int32>>(Json::array({1, 2})), vector<int32>({1, 2}));
    EXPECT_TRUE(ReadField<vector<int32>>(Json::array()).empty());
    EXPECT_EQ(ReadField<vector<DesignData::CurrencyType>>(Json::array({"GOLD"})), vector<DesignData::CurrencyType>({DesignData::CurrencyType::GOLD}));
    EXPECT_NE(ReadError<vector<int32>>(1).find("배열이 아니다"), string::npos);
    EXPECT_NE(ReadError<vector<int32>>(Json::array({1, "2"})).find("정수가 아니다"), string::npos);

    EXPECT_EQ(ReadField<optional<int32>>(nullptr), nullopt);
    EXPECT_EQ(ReadField<optional<int32>>(3), optional<int32>(3));
}

TEST(DesignDataReadTest, TableErrorNamesFileAndRow)
{
    DesignData::CurrencyTable table;
    const Json document = Json::array({
        Json{{"Id", 1}, {"Name", "골드"}, {"Type", "GOLD"}},
        Json{{"Id", 2}, {"Name", "명예"}},
    });

    const optional<string> error = DesignData::ParseCurrency(document, OUT table);

    ASSERT_TRUE(error.has_value());
    EXPECT_NE(error->find("Currency.json 2번째 행"), string::npos) << *error;
    EXPECT_NE(error->find("'Type'가 없다"), string::npos) << *error;
}

TEST(DesignDataReadTest, DataVersionIsRead)
{
    string version;
    EXPECT_FALSE(DesignData::ParseDataVersion(Json{{"Version", "342624c01048663c"}}, OUT version).has_value());
    EXPECT_EQ(version, "342624c01048663c");

    EXPECT_TRUE(DesignData::ParseDataVersion(Json::array(), OUT version).has_value());
}
