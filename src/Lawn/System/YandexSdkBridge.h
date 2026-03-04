#ifndef __YANDEXSDKBRIDGE_H__
#define __YANDEXSDKBRIDGE_H__

#include <string>
#include <vector>

enum class YandexSdkEventType
{
	YANDEX_SDK_EVENT_NONE = 0,
	YANDEX_SDK_EVENT_READY,
	YANDEX_SDK_EVENT_PAYMENT_SUCCESS,
	YANDEX_SDK_EVENT_PAYMENT_FAILED,
	YANDEX_SDK_EVENT_LEADERBOARD_SUBMIT_SUCCESS,
	YANDEX_SDK_EVENT_LEADERBOARD_SUBMIT_FAILED
};

struct YandexSdkEvent
{
	YandexSdkEventType				mType = YandexSdkEventType::YANDEX_SDK_EVENT_NONE;
	std::string						mPayload;
};

class YandexSdkBridge
{
private:
	bool							mInitialized = false;
	bool							mReady = false;
	std::vector<YandexSdkEvent>		mEvents;

private:
	void							PollEvents();

public:
	YandexSdkBridge();
	~YandexSdkBridge();

	void							Initialize();
	void							Update();

	bool							IsInitialized() const { return mInitialized; }
	bool							IsReady() const { return mReady; }

	bool							SubmitLeaderboardScore(const std::string& theLeaderboardName, int theScore);
	bool							RequestPurchase(const std::string& theSku, int theQuantity);

	const std::vector<YandexSdkEvent>& GetEvents() const { return mEvents; }
	void							ClearEvents() { mEvents.clear(); }
};

#endif
