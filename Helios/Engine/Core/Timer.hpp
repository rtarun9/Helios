#pragma once

#include "Pch.hpp"

namespace helios
{
	class Timer
	{
	public:
		void Start();
		void Stop();

		double GetDeltaTime() const;
		double GetTotalTime() const;

	private:
		std::chrono::system_clock m_Clock;

		std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<double>> m_CurrentFrameTime;
		std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<double>> m_PreviousFrameTime;

		double m_DeltaTime{};
		double m_TotalTime{};
	};
}