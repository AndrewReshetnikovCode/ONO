#include "RuntimeTelemetry.h"

void RuntimeTelemetry::Reset()
{
	mSnapshot = RuntimeTelemetrySnapshot();
	mTotalFrameMs = 0;
}

void RuntimeTelemetry::RecordFrameDurationMs(uint32_t theFrameMs)
{
	mSnapshot.mFrameCount++;
	mTotalFrameMs += theFrameMs;
	if (theFrameMs <= 16)
	{
		mSnapshot.mFramesAtOrBelow16Ms++;
	}
	else if (theFrameMs <= 33)
	{
		mSnapshot.mFramesAtOrBelow33Ms++;
	}
	else
	{
		mSnapshot.mFramesAbove33Ms++;
	}

	if (theFrameMs > mSnapshot.mMaxFrameMs)
	{
		mSnapshot.mMaxFrameMs = theFrameMs;
	}

	mSnapshot.mAverageFrameMs = mSnapshot.mFrameCount == 0 ? 0.0 : static_cast<double>(mTotalFrameMs) / static_cast<double>(mSnapshot.mFrameCount);
}
