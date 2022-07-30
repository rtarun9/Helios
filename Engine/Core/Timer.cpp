#include "Pch.hpp"

#include "Timer.hpp"

namespace helios::core
{
	void Timer::Tick()
	{
		mCurrentFrameTime = mClock.now();
		mDeltaTime = (mCurrentFrameTime - mPreviousFrameTime).count();
		mTotalTime += mDeltaTime;

		mPreviousFrameTime = mCurrentFrameTime;
	}
}