#ifndef __RUNTIMETELEMETRY_H__
#define __RUNTIMETELEMETRY_H__

#include <cstdint>

struct RuntimeTelemetrySnapshot
{
	uint64_t						mFrameCount = 0;
	uint64_t						mFramesAtOrBelow16Ms = 0;
	uint64_t						mFramesAtOrBelow33Ms = 0;
	uint64_t						mFramesAbove33Ms = 0;
	double							mAverageFrameMs = 0.0;
	uint32_t						mMaxFrameMs = 0;
};

class RuntimeTelemetry
{
private:
	RuntimeTelemetrySnapshot		mSnapshot;
	uint64_t						mTotalFrameMs = 0;

public:
	void							Reset();
	void							RecordFrameDurationMs(uint32_t theFrameMs);
	const RuntimeTelemetrySnapshot& GetSnapshot() const { return mSnapshot; }
};

#endif
