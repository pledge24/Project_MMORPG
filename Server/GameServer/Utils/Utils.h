#pragma once
#include <random>
#include <cmath>
#include <numbers>

/** 평면(x, y) 벡터. 좌표는 언리얼 단위(cm)를 따른다. 셀 행렬과 몬스터 이동이 높이를 빼고 계산할 때 쓴다. */
struct vector2D
{
    vector2D() {};
    vector2D(float x_, float y_) : x(x_), y(y_) {};

    static vector2D GetZeroVector() { return vector2D(0.f, 0.f); }
    float GetMagnitude() const { return sqrt(x * x + y * y); }
    /** 길이가 0이면 영벡터를 돌려준다. */
    vector2D GetNormalize() const {
        float magnitude = GetMagnitude();

        if (magnitude == 0.0)
        {
            return GetZeroVector();
        }

        // 각 성분을 크기로 나눈다.
        return vector2D(x / magnitude, y / magnitude);
    }

    /** x, y만 쓴다. vector의 z는 그대로 둔다. */
    void CopyTo(Protocol::Vector* vector)
    {
        vector->set_x(x);
        vector->set_y(y);
    }

    friend bool operator==(const vector2D& lhs, const vector2D& rhs)
    {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }

    friend vector2D operator+(const vector2D& lhs, const vector2D& rhs)
    {
        return { lhs.x + rhs.x, lhs.y + rhs.y };
    }

    friend vector2D operator-(const vector2D& lhs, const vector2D& rhs)
    {
        return { lhs.x - rhs.x, lhs.y - rhs.y };
    }

    friend vector2D operator*(const vector2D& lhs, float scale)
    {
        return { lhs.x * scale, lhs.y * scale };
    }

    float x = 0.f;
    float y = 0.f;
};

/** 3차원 벡터. 좌표는 언리얼 단위(cm)를 따른다. */
struct vector3D
{
    vector3D() {};
    vector3D(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {};

    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

/** 상태 없는 범용 함수 모음. */
class Utils
{
public:
	/**
	 * [min, max]에서 뽑는다. 정수도 max를 포함한다. 배열 인덱스를 뽑을 때는 size - 1을 넘긴다.
	 * 호출마다 엔진을 새로 만들므로 여러 스레드에서 불러도 된다. min이 max보다 크면 동작이 정의되지 않는다.
	 */
	template<typename T>
	static T GetRandom(T min, T max)
	{
		// 시드값을 얻기 위한 random_device 생성.
		std::random_device randomDevice;
		// random_device 를 통해 난수 생성 엔진을 초기화 한다.
		std::mt19937 generator(randomDevice());

		// 균등하게 나타나는 난수열을 생성하기 위해 균등 분포 정의.
		if constexpr (std::is_integral_v<T>)
		{
			std::uniform_int_distribution<T> distribution(min, max);
			return distribution(generator);
		}
		else
		{
			std::uniform_real_distribution<T> distribution(min, max);
			return distribution(generator);
		}
	}
};

/** 평면 거리와 방향 계산. yaw는 언리얼과 같이 도(degree) 단위이고 +x 축이 0도다. */
class MathUtil
{
public:
    /** -180도에서 180도 사이로 돌려준다. */
    static float VectorToYaw(const vector2D& vec)
    {
        float yaw_radians = std::atan2f(vec.y, vec.x);

        float yaw_degrees = yaw_radians * (180.0f / std::numbers::pi_v<float>);

        // 언리얼 방식에 맞게 -180~180 범위로 정규화
        if (yaw_degrees > 180.0f)
            yaw_degrees -= 360.0f;
        else if (yaw_degrees < -180.0f)
            yaw_degrees += 360.0f;

        return yaw_degrees;
    }

    /** 평면 거리다. noSqrt면 제곱 거리를 돌려준다. */
    static float Distance(const vector2D& src, const vector2D& dst, bool noSqrt = false)
    {
        float dx = dst.x - src.x;
        float dy = dst.y - src.y;
        float squareDist = dx * dx + dy * dy;

        return noSqrt ? squareDist : sqrt(squareDist);
    }

    /** z를 빼고 잰 평면 거리다. noSqrt면 제곱 거리를 돌려준다. */
    static float Distance(const Protocol::PosInfo* src, const Protocol::PosInfo* dst, bool noSqrt = false)
    {
        float dx = dst->pos().x() - src->pos().x();
        float dy = dst->pos().y() - src->pos().y();
        float squareDist = dx * dx + dy * dy;

        return noSqrt ? squareDist : sqrt(squareDist);
    }

    static vector2D PosInfoToVector2D(const Protocol::PosInfo* posInfo)
    {
        return { posInfo->pos().x(), posInfo->pos().y() };
    }

    /** 평면 거리가 range 이하면 true. */
    static bool InRange(const Protocol::PosInfo* curPos, const Protocol::PosInfo* target, float range)
    {
        float squareDist = MathUtil::Distance(curPos, target, true);
        
        return squareDist <= (range * range);
    }
};

using google::protobuf::RepeatedPtrField;

/** protobuf 메시지를 채우는 보조 함수 모음. */
class ProtoUtil
{
public:
    static void AddStat(RepeatedPtrField<Protocol::Stat>* statList, Protocol::StatType type, int64 Value)
    {
        Protocol::Stat* stat = statList->Add();
        {
            stat->set_type(type);
            stat->set_value(Value);
        }
    }
};