#pragma once

namespace helios::core
{
	class Timer
	{
	public:
		void Tick();

		inline double GetDeltaTime() const { return mDeltaTime; };
		inline double GetTotalTime() const { return mTotalTime; };

	private:
		std::chrono::high_resolution_clock mClock{};

		std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::duration<double>> mCurrentFrameTime{};
		std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::duration<double>> mPreviousFrameTime{};

		double mDeltaTime{};
		double mTotalTime{};
	};
}