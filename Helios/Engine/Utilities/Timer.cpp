#include "Pch.hpp"

#include "Timer.hpp"

namespace helios
{
	void Timer::Start()
	{
		m_CurrentFrameTime = m_Clock.now();
	}

	void Timer::Stop()
	{
		m_DeltaTime = (m_CurrentFrameTime - m_PreviousFrameTime).count() * 1e-9;
		m_TotalTime += m_DeltaTime;

		m_PreviousFrameTime = m_CurrentFrameTime;
	}

	double Timer::GetDeltaTime() const
	{
		return m_DeltaTime;
	}

	double Timer::GetTotalTime() const
	{
		return m_TotalTime;
	}
}