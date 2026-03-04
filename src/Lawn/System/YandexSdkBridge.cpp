#include "YandexSdkBridge.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

YandexSdkBridge::YandexSdkBridge()
{
}

YandexSdkBridge::~YandexSdkBridge()
{
}

#ifdef __EMSCRIPTEN__
EM_JS(void, yaSdkEnsureInit, (), {
	if (!Module._yaBridge) {
		Module._yaBridge = {
			initStarted: false,
			ready: false,
			eventQueue: []
		};
	}

	if (Module._yaBridge.initStarted) return;
	Module._yaBridge.initStarted = true;

	if (!window.YaGames || !window.YaGames.init) {
		Module._yaBridge.eventQueue.push("LEADERBOARD_FAILED:no_sdk");
		return;
	}

	window.YaGames.init()
		.then(function(ysdk) {
			Module._yaBridge.sdk = ysdk;
			Module._yaBridge.ready = true;
			Module._yaBridge.eventQueue.push("READY");
		})
		.catch(function() {
			Module._yaBridge.eventQueue.push("LEADERBOARD_FAILED:init_failed");
		});
});

EM_JS(int, yaSdkIsReady, (), {
	return (Module._yaBridge && Module._yaBridge.ready) ? 1 : 0;
});

EM_JS(int, yaSdkSubmitLeaderboardScore, (const char* ptrName, int score), {
	if (!Module._yaBridge || !Module._yaBridge.ready || !Module._yaBridge.sdk || !Module._yaBridge.sdk.leaderboards) return 0;
	var leaderboard = UTF8ToString(ptrName);
	Module._yaBridge.sdk.leaderboards.setLeaderboardScore(leaderboard, score)
		.then(function() { Module._yaBridge.eventQueue.push("LEADERBOARD_OK:" + leaderboard + ":" + score); })
		.catch(function() { Module._yaBridge.eventQueue.push("LEADERBOARD_FAILED:" + leaderboard); });
	return 1;
});

EM_JS(int, yaSdkRequestPurchase, (const char* ptrSku), {
	if (!Module._yaBridge || !Module._yaBridge.ready || !Module._yaBridge.sdk || !Module._yaBridge.sdk.payments) return 0;
	var sku = UTF8ToString(ptrSku);
	Module._yaBridge.sdk.payments.purchase({ id: sku })
		.then(function() { Module._yaBridge.eventQueue.push("PAYMENT_OK:" + sku); })
		.catch(function() { Module._yaBridge.eventQueue.push("PAYMENT_FAILED:" + sku); });
	return 1;
});

EM_JS(int, yaSdkPopEvent, (char* outBuf, int outLen), {
	if (!Module._yaBridge || !Module._yaBridge.eventQueue || Module._yaBridge.eventQueue.length === 0) return 0;
	var event = Module._yaBridge.eventQueue.shift();
	stringToUTF8(event, outBuf, outLen);
	return 1;
});
#endif

void YandexSdkBridge::Initialize()
{
	if (mInitialized)
	{
		return;
	}

	mInitialized = true;

#ifdef __EMSCRIPTEN__
	yaSdkEnsureInit();
#else
	mReady = false;
#endif
}

void YandexSdkBridge::PollEvents()
{
#ifdef __EMSCRIPTEN__
	char aBuffer[256] = {0};
	while (yaSdkPopEvent(aBuffer, static_cast<int>(sizeof(aBuffer))) != 0)
	{
		std::string aEventText(aBuffer);
		YandexSdkEvent aEvent;
		aEvent.mPayload = aEventText;

		if (aEventText == "READY")
		{
			aEvent.mType = YandexSdkEventType::YANDEX_SDK_EVENT_READY;
			mReady = true;
		}
		else if (aEventText.rfind("PAYMENT_OK:", 0) == 0)
		{
			aEvent.mType = YandexSdkEventType::YANDEX_SDK_EVENT_PAYMENT_SUCCESS;
		}
		else if (aEventText.rfind("PAYMENT_FAILED:", 0) == 0)
		{
			aEvent.mType = YandexSdkEventType::YANDEX_SDK_EVENT_PAYMENT_FAILED;
		}
		else if (aEventText.rfind("LEADERBOARD_OK:", 0) == 0)
		{
			aEvent.mType = YandexSdkEventType::YANDEX_SDK_EVENT_LEADERBOARD_SUBMIT_SUCCESS;
		}
		else
		{
			aEvent.mType = YandexSdkEventType::YANDEX_SDK_EVENT_LEADERBOARD_SUBMIT_FAILED;
		}

		mEvents.push_back(aEvent);
	}
#endif
}

void YandexSdkBridge::Update()
{
	if (!mInitialized)
	{
		return;
	}

#ifdef __EMSCRIPTEN__
	if (!mReady)
	{
		mReady = yaSdkIsReady() != 0;
	}
#endif

	PollEvents();
}

bool YandexSdkBridge::SubmitLeaderboardScore(const std::string& theLeaderboardName, int theScore)
{
	if (!mInitialized || theLeaderboardName.empty())
	{
		return false;
	}

#ifdef __EMSCRIPTEN__
	if (!mReady)
	{
		return false;
	}
	return yaSdkSubmitLeaderboardScore(theLeaderboardName.c_str(), theScore) != 0;
#else
	(void)theScore;
	return false;
#endif
}

bool YandexSdkBridge::RequestPurchase(const std::string& theSku, int theQuantity)
{
	if (!mInitialized || theSku.empty() || theQuantity <= 0)
	{
		return false;
	}

#ifdef __EMSCRIPTEN__
	if (!mReady)
	{
		return false;
	}
	return yaSdkRequestPurchase(theSku.c_str()) != 0;
#else
	return false;
#endif
}
