#include "Pch.hpp"

#include "Timer.hpp"

namespace helios
{
	void Timer::Tick()
	{
		m_CurrentFrameTime = m_Clock.now();
		m_DeltaTime = (m_CurrentFrameTime - m_PreviousFrameTime).count();
		m_TotalTime += m_DeltaTime;

		m_PreviousFrameTime = m_CurrentFrameTime;
	}
}