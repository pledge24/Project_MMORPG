#pragma once
#include <random>
#include <cmath>
#include <numbers>

struct vector2D
{
    vector2D() {};
    vector2D(float x_, float y_) : x(x_), y(y_) {};

    static vector2D GetZeroVector() { return vector2D(0.f, 0.f); }
    float GetMagnitude() { return sqrt(x * x + y * y); }
    vector2D GetNormalize() {
        float magnitude = GetMagnitude();

        if (magnitude == 0.0)
        {
            return GetZeroVector();
        }

        // 각 성분을 크기로 나눕니다.
        return vector2D(x / magnitude, y / magnitude);
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
        const float PI = std::numbers::pi_v<float>;
        float yaw_radians = yaw * (PI / 180.0f);

        float dirX = std::cosf(yaw_radians);
        float dirY = std::sinf(yaw_radians);

        return vector2D{ dirX, dirY };
    }

    static float VectorToYaw(const vector2D& vec)
    {
        float vy = vec.y;
        float vx = vec.x;
        float yaw_radians = std::atan2f(vy, vx);

        const float PI_F = std::numbers::pi_v<float>;
        float yaw_degrees = yaw_radians * (180.0f / PI_F);

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
        float dx = dst->x() - src->x();
        float dy = dst->y() - src->y();
        float squareDist = dx * dx + dy * dy;

        return noSqrt ? squareDist : sqrt(squareDist);
    }
};



