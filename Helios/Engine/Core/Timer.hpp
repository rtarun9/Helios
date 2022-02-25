#pragma once

#include "Pch.hpp"

namespace helios
{
	class Timer
	{
	public:
		void Tick();

		double GetDeltaTime() const;
		double GetTotalTime() const;

	private:
		std::chrono::high_resolution_clock m_Clock;

		std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::duration<double>> m_CurrentFrameTime;
		std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::duration<double>> m_PreviousFrameTime;

		double m_DeltaTime{};
		double m_TotalTime{};
	};
}