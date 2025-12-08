#pragma once
#include <random>
#include <cmath>
#include <numbers>

struct vector2D
{
    vector2D() {};
    vector2D(float x_, float y_) : x(x_), y(y_) {};

    static vector2D GetZeroVector() { return vector2D(0.f, 0.f); }
    float GetMagnitude() const { return sqrt(x * x + y * y); }
    vector2D GetNormalize() const {
        float magnitude = GetMagnitude();

        if (magnitude == 0.0)
        {
            return GetZeroVector();
        }

        // 각 성분을 크기로 나눕니다.
        return vector2D(x / magnitude, y / magnitude);
    }

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

    friend vector2D operator-=(const vector2D& lhs, const vector2D& rhs)
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

struct vector3D
{
    vector3D() {};
    vector3D(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {};

    static vector3D GetZeroVector() { return vector3D(0.f, 0.f, 0.f); }
    float GetMagnitude() { return sqrt(x * x + y * y + z * z); }
    vector3D GetNormalize()
    {
        float magnitude = GetMagnitude();

        if (magnitude == 0.0)
        {
            return GetZeroVector();
        }

        // 각 성분을 크기로 나눕니다.
        return vector3D(x / magnitude, y / magnitude, z /magnitude);
    }

    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

class Utils
{
public:
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
			std::uniform_int_distribution<T> distribution(min, max-1);
			return distribution(generator);
		}
		else
		{
			std::uniform_real_distribution<T> distribution(min, max);
			return distribution(generator);
		}
	}
};

class MathUtil
{
public:
    static vector2D GetUnitVector(float yaw)
    {
        constexpr float PI = std::numbers::pi_v<float>;
        float yaw_radians = yaw * (PI / 180.0f);

        float dirX = std::cosf(yaw_radians);
        float dirY = std::sinf(yaw_radians);

        return vector2D{ dirX, dirY };
    }

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

    static float Distance(const vector2D& src, const vector2D& dst, bool noSqrt = false)
    {
        float dx = dst.x - src.x;
        float dy = dst.y - src.y;
        float squareDist = dx * dx + dy * dy;

        return noSqrt ? squareDist : sqrt(squareDist);
    }

    static float Distance(Protocol::PosInfo* src, Protocol::PosInfo* dst, bool noSqrt = false)
    {
        float dx = dst->pos().x() - src->pos().x();
        float dy = dst->pos().y() - src->pos().y();
        float squareDist = dx * dx + dy * dy;

        return noSqrt ? squareDist : sqrt(squareDist);
    }

    static vector2D PosInfoToVector2D(Protocol::PosInfo* posInfo)
    {
        return { posInfo->pos().x(), posInfo->pos().y() };
    }

    static bool InRange(Protocol::PosInfo* curPos, Protocol::PosInfo* target, float range)
    {
        float squareDist = MathUtil::Distance(curPos, target, true);
        
        return squareDist <= (range * range);
    }

    static bool IsZeroVector(Protocol::Vector* vector)
    {
        bool zeroX = vector->x() == 0.f;
        bool zeroY = vector->y() == 0.f;
        bool zeroZ = vector->z() == 0.f;

        return (zeroX && zeroY && zeroZ);
    }
};