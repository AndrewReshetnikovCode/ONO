#ifdef __EMSCRIPTEN__

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>
#include <emscripten/emscripten.h>
#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/System/AuthoritativeProgressionStore.h"
#include "Lawn/System/BotActionTrack.h"
#include "Lawn/System/ClientSessionRuntime.h"
#include "Lawn/System/NetProtocol.h"
#include "Lawn/System/RuntimeTelemetry.h"
#include "Lawn/System/YandexSdkBridge.h"
#include "Lawn/System/PlayerInfo.h"
#include "Lawn/Widget/GameSelector.h"

static std::string gPvzDebugReturn;
static bool gPvzAutoRuntimeStartRequested = false;
static bool gPvzAutoRuntimeStartConsumed = false;

static const char* PvzDebugReturn(const std::string& theValue)
{
	gPvzDebugReturn = theValue;
	return gPvzDebugReturn.c_str();
}

static std::string PvzEscapeJson(const std::string& theValue)
{
	std::string aOut;
	aOut.reserve(theValue.size() + 8);
	for (char aChar : theValue)
	{
		switch (aChar)
		{
		case '\"': aOut += "\\\""; break;
		case '\\': aOut += "\\\\"; break;
		case '\n': aOut += "\\n"; break;
		case '\r': aOut += "\\r"; break;
		case '\t': aOut += "\\t"; break;
		default: aOut += aChar; break;
		}
	}
	return aOut;
}

static LawnApp* PvzGetApp()
{
	return gLawnApp;
}

extern "C" {

EMSCRIPTEN_KEEPALIVE const char* PvzDebug_TestFunction(const char* theA, int theB)
{
	std::string aInput = theA ? theA : "";
	std::ostringstream aOut;
	aOut << "{"
		<< "\"input\":\"" << PvzEscapeJson(aInput) << "\","
		<< "\"value\":" << theB << ","
		<< "\"result\":\"" << PvzEscapeJson(aInput + ":" + std::to_string(theB)) << "\""
		<< "}";
	return PvzDebugReturn(aOut.str());
}

EMSCRIPTEN_KEEPALIVE int PvzDebug_HasLawnApp()
{
	return PvzGetApp() != nullptr ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE const char* PvzDebug_GetClientSessionState()
{
	LawnApp* aApp = PvzGetApp();
	if (aApp == nullptr)
	{
		return "NO_APP";
	}
	return aApp->GetClientSessionStateString();
}

EMSCRIPTEN_KEEPALIVE int PvzDebug_StartAdventureFromSelector()
{
	LawnApp* aApp = PvzGetApp();
	if (aApp == nullptr || aApp->mGameSelector == nullptr)
	{
		return 0;
	}

	aApp->mGameSelector->ClickedAdventure();
	return 1;
}

EMSCRIPTEN_KEEPALIVE int PvzDebug_EnableAuthoritativeRuntime()
{
	LawnApp* aApp = PvzGetApp();
	if (aApp == nullptr)
	{
		return 0;
	}

	aApp->mEnableAuthoritativeClientSession = true;
	if (aApp->mClientSessionRuntime == nullptr)
	{
		aApp->InitClientSessionRuntime();
	}
	return aApp->mClientSessionRuntime != nullptr ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE int PvzDebug_StartStoryModeWithOpponentSearch(int theLookForSavedGame)
{
	LawnApp* aApp = PvzGetApp();
	if (aApp == nullptr)
	{
		return 0;
	}

	aApp->StartStoryModeWithOpponentSearch(theLookForSavedGame != 0);
	return 1;
}

EMSCRIPTEN_KEEPALIVE int PvzDebug_StartSurvivalModeWithOpponentSearch(int theLookForSavedGame)
{
	return PvzDebug_StartStoryModeWithOpponentSearch(theLookForSavedGame);
}

EMSCRIPTEN_KEEPALIVE int PvzDebug_IsSelectorReady()
{
	LawnApp* aApp = PvzGetApp();
	if (aApp == nullptr)
	{
		return 0;
	}
	return aApp->mGameSelector != nullptr ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void PvzDebug_RequestAutoRuntimeStart()
{
	gPvzAutoRuntimeStartRequested = true;
	gPvzAutoRuntimeStartConsumed = false;
}

EMSCRIPTEN_KEEPALIVE int PvzDebug_SubmitPlantCommand(int theGridX, int theGridY, int theSeedType, int theImitaterSeedType)
{
	LawnApp* aApp = PvzGetApp();
	if (aApp == nullptr)
	{
		return 0;
	}

	return aApp->SubmitAuthoritativePlantCommand(
		theGridX,
		theGridY,
		static_cast<SeedType>(theSeedType),
		static_cast<SeedType>(theImitaterSeedType)
	) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE int PvzDebug_SubmitRemovePlantCommand(int theGridX, int theGridY)
{
	LawnApp* aApp = PvzGetApp();
	if (aApp == nullptr)
	{
		return 0;
	}
	return aApp->SubmitAuthoritativeRemovePlantCommand(theGridX, theGridY) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE int PvzDebug_GetBoardSun()
{
	LawnApp* aApp = PvzGetApp();
	if (aApp == nullptr || aApp->mBoard == nullptr)
	{
		return -1;
	}
	return aApp->mBoard->mSunMoney;
}

EMSCRIPTEN_KEEPALIVE const char* PvzDebug_GetGameModeJson()
{
	LawnApp* aApp = PvzGetApp();
	std::ostringstream aOut;
	if (aApp == nullptr)
	{
		aOut << "{\"hasApp\":false}";
		return PvzDebugReturn(aOut.str());
	}

	aOut << "{"
		<< "\"hasApp\":true,"
		<< "\"gameMode\":" << static_cast<int>(aApp->mGameMode) << ","
		<< "\"isSurvivalMode\":" << (aApp->IsSurvivalMode() ? "true" : "false") << ","
		<< "\"isAdventureMode\":" << (aApp->IsAdventureMode() ? "true" : "false")
		<< "}";
	return PvzDebugReturn(aOut.str());
}

EMSCRIPTEN_KEEPALIVE const char* PvzDebug_GetGameRenderMetricsJson()
{
	LawnApp* aApp = PvzGetApp();
	std::ostringstream aOut;
	if (aApp == nullptr)
	{
		aOut << "{\"hasApp\":false}";
		return PvzDebugReturn(aOut.str());
	}

	const int aWidth = aApp->mWidth;
	const int aHeight = aApp->mHeight;
	const double aAspect43 = 4.0 / 3.0;
	const double aAspect = (aHeight > 0) ? (static_cast<double>(aWidth) / static_cast<double>(aHeight)) : 0.0;
	const double aAspectError = std::fabs(aAspect - aAspect43);
	const bool aAspectIs43 = (aHeight > 0) && (aAspectError <= 0.0025);

	aOut << "{"
		<< "\"hasApp\":true,"
		<< "\"appWidth\":" << aWidth << ","
		<< "\"appHeight\":" << aHeight << ","
		<< "\"appAspect\":" << aAspect << ","
		<< "\"appAspectErrorTo43\":" << aAspectError << ","
		<< "\"appAspectIs43\":" << (aAspectIs43 ? "true" : "false") << ","
		<< "\"isWindowed\":" << (aApp->mIsWindowed ? "true" : "false") << ","
		<< "\"fullscreenBits\":" << aApp->mFullscreenBits
		<< "}";
	return PvzDebugReturn(aOut.str());
}

EMSCRIPTEN_KEEPALIVE const char* PvzDebug_GetLatestSnapshotJson()
{
	LawnApp* aApp = PvzGetApp();
	std::ostringstream aOut;
	if (aApp == nullptr || aApp->mClientSessionRuntime == nullptr)
	{
		aOut << "{\"hasRuntime\":false}";
		return PvzDebugReturn(aOut.str());
	}

	const ClientAuthoritativeSnapshot& aSnapshot = aApp->mClientSessionRuntime->GetLatestSnapshot();
	aOut << "{"
		<< "\"hasRuntime\":true,"
		<< "\"hasSnapshot\":" << (aSnapshot.mHasSnapshot ? "true" : "false") << ","
		<< "\"matchId\":" << aSnapshot.mMatchId << ","
		<< "\"tick\":" << aSnapshot.mTick << ","
		<< "\"sun\":" << aSnapshot.mAuthoritativeSun << ","
		<< "\"damage\":" << aSnapshot.mAuthoritativeDamage << ","
		<< "\"eliminated\":" << (aSnapshot.mEliminated ? "true" : "false") << ","
		<< "\"pvpEnemyBoardDisplayed\":" << (aSnapshot.mPvpEnemyBoardDisplayed ? "true" : "false") << ","
		<< "\"focusedEnemyPlayerId\":" << aSnapshot.mFocusedEnemyPlayerId << ","
		<< "\"focusedEnemyName\":\"" << PvzEscapeJson(aSnapshot.mFocusedEnemyName) << "\""
		<< "}";
	return PvzDebugReturn(aOut.str());
}

EMSCRIPTEN_KEEPALIVE const char* PvzDebug_GetSessionEventsJson(int theLimit)
{
	LawnApp* aApp = PvzGetApp();
	std::ostringstream aOut;
	if (aApp == nullptr || aApp->mClientSessionRuntime == nullptr)
	{
		aOut << "[]";
		return PvzDebugReturn(aOut.str());
	}

	const std::vector<AuthoritativeRuntimeEvent>& aEvents = aApp->mClientSessionRuntime->GetEvents();
	size_t aLimit = theLimit > 0 ? static_cast<size_t>(theLimit) : 0;
	size_t aStart = 0;
	if (aLimit > 0 && aEvents.size() > aLimit)
	{
		aStart = aEvents.size() - aLimit;
	}

	aOut << "[";
	for (size_t i = aStart; i < aEvents.size(); ++i)
	{
		const AuthoritativeRuntimeEvent& aEvent = aEvents[i];
		if (i > aStart)
		{
			aOut << ",";
		}
		aOut << "{"
			<< "\"eventType\":\"" << PvzEscapeJson(NetEventTypeToString(aEvent.mEventType)) << "\","
			<< "\"matchId\":" << aEvent.mMatchId << ","
			<< "\"tick\":" << aEvent.mTick << ","
			<< "\"playerId\":" << aEvent.mPlayerId << ","
			<< "\"error\":\"" << PvzEscapeJson(NetProtocolErrorToString(aEvent.mError)) << "\","
			<< "\"details\":\"" << PvzEscapeJson(aEvent.mDetails) << "\""
			<< "}";
	}
	aOut << "]";
	return PvzDebugReturn(aOut.str());
}

EMSCRIPTEN_KEEPALIVE void PvzDebug_ClearSessionEvents()
{
	LawnApp* aApp = PvzGetApp();
	if (aApp != nullptr && aApp->mClientSessionRuntime != nullptr)
	{
		aApp->mClientSessionRuntime->ClearEvents();
	}
}

EMSCRIPTEN_KEEPALIVE const char* PvzDebug_GetAuthoritativeProfileJson()
{
	LawnApp* aApp = PvzGetApp();
	std::ostringstream aOut;
	if (aApp == nullptr || aApp->mAuthoritativeProgressionStore == nullptr || aApp->mPlayerInfo == nullptr)
	{
		aOut << "{\"hasProfile\":false}";
		return PvzDebugReturn(aOut.str());
	}

	uint64_t aPlayerId = aApp->mPlayerInfo->mId;
	const AuthoritativePlayerProfile* aProfile = aApp->mAuthoritativeProgressionStore->FindProfile(aPlayerId);
	if (aProfile == nullptr)
	{
		aOut << "{\"hasProfile\":false,\"playerId\":" << aPlayerId << "}";
		return PvzDebugReturn(aOut.str());
	}

	std::vector<std::string> aCosmetics = aApp->mAuthoritativeProgressionStore->GetCosmeticsOwned(aPlayerId);
	aOut << "{"
		<< "\"hasProfile\":true,"
		<< "\"playerId\":" << aProfile->mPlayerId << ","
		<< "\"diamonds\":" << aProfile->mDiamonds << ","
		<< "\"mmr\":" << aProfile->mMmr << ","
		<< "\"weeklyScore\":" << aProfile->mWeeklyScore << ","
		<< "\"importedFromLocal\":" << (aProfile->mImportedFromLocal ? "true" : "false") << ","
		<< "\"schemaVersion\":" << aProfile->mSchemaVersion << ","
		<< "\"cosmetics\":[";
	for (size_t i = 0; i < aCosmetics.size(); ++i)
	{
		if (i > 0)
		{
			aOut << ",";
		}
		aOut << "\"" << PvzEscapeJson(aCosmetics[i]) << "\"";
	}
	aOut << "]}";
	return PvzDebugReturn(aOut.str());
}

EMSCRIPTEN_KEEPALIVE const char* PvzDebug_GetYandexStatusJson()
{
	LawnApp* aApp = PvzGetApp();
	std::ostringstream aOut;
	if (aApp == nullptr || aApp->mYandexSdkBridge == nullptr)
	{
		aOut << "{\"hasBridge\":false}";
		return PvzDebugReturn(aOut.str());
	}

	aOut << "{"
		<< "\"hasBridge\":true,"
		<< "\"initialized\":" << (aApp->mYandexSdkBridge->IsInitialized() ? "true" : "false") << ","
		<< "\"ready\":" << (aApp->mYandexSdkBridge->IsReady() ? "true" : "false") << ","
		<< "\"eventCount\":" << aApp->mYandexSdkBridge->GetEvents().size()
		<< "}";
	return PvzDebugReturn(aOut.str());
}

EMSCRIPTEN_KEEPALIVE const char* PvzDebug_ValidatePlaceCommandJson(
	int theMatchId,
	int thePlayerId,
	int theCommandId,
	int theClientTick,
	int theGridX,
	int theGridY,
	int theSeedType,
	int theImitaterSeedType)
{
	NetClientCommand aCommand;
	aCommand.mEnvelope.mVersion = NET_PROTOCOL_VERSION_V1;
	aCommand.mEnvelope.mMatchId = static_cast<uint64_t>(std::max(0, theMatchId));
	aCommand.mEnvelope.mPlayerId = static_cast<uint64_t>(std::max(0, thePlayerId));
	aCommand.mEnvelope.mCommandId = static_cast<uint64_t>(std::max(0, theCommandId));
	aCommand.mEnvelope.mClientTick = static_cast<uint32_t>(std::max(0, theClientTick));
	aCommand.mEnvelope.mCommandType = NetCommandType::NET_COMMAND_PLACE_PLANT;

	NetPlacePlantPayload aPayload;
	aPayload.mGridX = theGridX;
	aPayload.mGridY = theGridY;
	aPayload.mSeedType = theSeedType;
	aPayload.mImitaterSeedType = theImitaterSeedType;
	aCommand.mPayload = aPayload;

	NetCommandValidationResult aResult = NetValidateClientCommand(aCommand);
	std::ostringstream aOut;
	aOut << "{"
		<< "\"ok\":" << (aResult.IsOk() ? "true" : "false") << ","
		<< "\"error\":\"" << PvzEscapeJson(NetProtocolErrorToString(aResult.mError)) << "\","
		<< "\"reason\":\"" << PvzEscapeJson(aResult.mReason) << "\""
		<< "}";
	return PvzDebugReturn(aOut.str());
}

EMSCRIPTEN_KEEPALIVE const char* PvzDebug_AssignBotTracksJson(int theLobbyId, int theMmrHint, int theBotCount)
{
	size_t aBotCount = theBotCount > 0 ? static_cast<size_t>(theBotCount) : 0;
	std::vector<int> aTracks = BotActionTrackPool::AssignTracksUniqueFirst(
		static_cast<uint64_t>(std::max(0, theLobbyId)),
		theMmrHint,
		aBotCount
	);

	std::ostringstream aOut;
	aOut << "[";
	for (size_t i = 0; i < aTracks.size(); ++i)
	{
		if (i > 0)
		{
			aOut << ",";
		}
		aOut << aTracks[i];
	}
	aOut << "]";
	return PvzDebugReturn(aOut.str());
}

EMSCRIPTEN_KEEPALIVE const char* PvzDebug_GetBotTrackJson(int theTrackId)
{
	const BotActionTrack* aTrack = BotActionTrackPool::FindTrackById(theTrackId);
	std::ostringstream aOut;
	if (aTrack == nullptr)
	{
		aOut << "{\"found\":false,\"trackId\":" << theTrackId << "}";
		return PvzDebugReturn(aOut.str());
	}

	aOut << "{"
		<< "\"found\":true,"
		<< "\"trackId\":" << aTrack->mTrackId << ","
		<< "\"actions\":[";
	for (size_t i = 0; i < aTrack->mActions.size(); ++i)
	{
		const BotRecordedAction& aAction = aTrack->mActions[i];
		if (i > 0)
		{
			aOut << ",";
		}

		aOut << "{"
			<< "\"tickOffset\":" << aAction.mTickOffset << ","
			<< "\"actionType\":" << static_cast<int>(aAction.mActionType) << ","
			<< "\"gridX\":" << aAction.mGridX << ","
			<< "\"gridY\":" << aAction.mGridY << ","
			<< "\"seedType\":" << aAction.mSeedType << ","
			<< "\"imitaterSeedType\":" << aAction.mImitaterSeedType
			<< "}";
	}
	aOut << "]}";
	return PvzDebugReturn(aOut.str());
}

EMSCRIPTEN_KEEPALIVE const char* PvzDebug_ListBotTracksJson(int theLimit)
{
	const std::vector<BotActionTrack>& aTracks = BotActionTrackPool::GetTracks();
	size_t aLimit = theLimit > 0 ? static_cast<size_t>(theLimit) : aTracks.size();
	size_t aCount = std::min(aTracks.size(), aLimit);

	std::ostringstream aOut;
	aOut << "[";
	for (size_t i = 0; i < aCount; ++i)
	{
		const BotActionTrack& aTrack = aTracks[i];
		if (i > 0)
		{
			aOut << ",";
		}
		aOut << "{"
			<< "\"trackId\":" << aTrack.mTrackId << ","
			<< "\"actionCount\":" << aTrack.mActions.size()
			<< "}";
	}
	aOut << "]";
	return PvzDebugReturn(aOut.str());
}

EMSCRIPTEN_KEEPALIVE const char* PvzDebug_GetRuntimeTelemetryJson()
{
	LawnApp* aApp = PvzGetApp();
	std::ostringstream aOut;
	if (aApp == nullptr || aApp->mRuntimeTelemetry == nullptr)
	{
		aOut << "{\"hasTelemetry\":false}";
		return PvzDebugReturn(aOut.str());
	}

	const RuntimeTelemetrySnapshot& aSnapshot = aApp->mRuntimeTelemetry->GetSnapshot();
	aOut << "{"
		<< "\"hasTelemetry\":true,"
		<< "\"frameCount\":" << aSnapshot.mFrameCount << ","
		<< "\"framesAtOrBelow16Ms\":" << aSnapshot.mFramesAtOrBelow16Ms << ","
		<< "\"framesAtOrBelow33Ms\":" << aSnapshot.mFramesAtOrBelow33Ms << ","
		<< "\"framesAbove33Ms\":" << aSnapshot.mFramesAbove33Ms << ","
		<< "\"averageFrameMs\":" << aSnapshot.mAverageFrameMs << ","
		<< "\"maxFrameMs\":" << aSnapshot.mMaxFrameMs
		<< "}";
	return PvzDebugReturn(aOut.str());
}

} // extern "C"

EM_JS(int, PvzInstallDebugConsoleBridgeJs, (), {
	if (typeof window === 'undefined') {
		return 0;
	}
	if (window.PvzDebug && window.PvzDebug.__installed) {
		return 1;
	}

	function parseJsonOrRaw(v) {
		if (typeof v !== 'string') return v;
		try { return JSON.parse(v); } catch (e) { return v; }
	}

	function ccallSafe(name, returnType, argTypes, args) {
		if (typeof Module === 'undefined' || !Module.ccall) {
			throw new Error('[PvzDebug] Module.ccall is not available yet');
		}
		return Module.ccall(name, returnType, argTypes, args);
	}

	function getCanvasMetrics() {
		if (typeof Module === 'undefined' || !Module['canvas']) {
			return { hasCanvas: false };
		}

		const canvas = Module['canvas'];
		const rect = canvas.getBoundingClientRect ? canvas.getBoundingClientRect() : { width: canvas.clientWidth || 0, height: canvas.clientHeight || 0, left: 0, top: 0 };
		const viewportWidth = Math.max(0, window.innerWidth || document.documentElement.clientWidth || 0);
		const viewportHeight = Math.max(0, window.innerHeight || document.documentElement.clientHeight || 0);
		const cssWidth = Math.max(0, Math.round(rect.width || 0));
		const cssHeight = Math.max(0, Math.round(rect.height || 0));
		const backingWidth = canvas.width || 0;
		const backingHeight = canvas.height || 0;
		const aspect43 = 4 / 3;
		const strictTargetWidth = 800;
		const strictTargetHeight = 600;
		const cssAspect = cssHeight > 0 ? (cssWidth / cssHeight) : 0;
		const backingAspect = backingHeight > 0 ? (backingWidth / backingHeight) : 0;
		const cssError = Math.abs(cssAspect - aspect43);
		const backingError = Math.abs(backingAspect - aspect43);

		return {
			hasCanvas: true,
			viewportWidth,
			viewportHeight,
			screenWidth: window.screen ? window.screen.width : 0,
			screenHeight: window.screen ? window.screen.height : 0,
			devicePixelRatio: window.devicePixelRatio || 1,
			isFullscreenActive: !!document.fullscreenElement,
			canvasCssLeft: Math.round(rect.left || 0),
			canvasCssTop: Math.round(rect.top || 0),
			canvasCssWidth: cssWidth,
			canvasCssHeight: cssHeight,
			canvasCssAspect: cssAspect,
			canvasCssAspectErrorTo43: cssError,
			canvasCssAspectIs43: cssHeight > 0 && cssError <= 0.0025,
			canvasBackingWidth: backingWidth,
			canvasBackingHeight: backingHeight,
			canvasBackingAspect: backingAspect,
			canvasBackingAspectErrorTo43: backingError,
			canvasBackingAspectIs43: backingHeight > 0 && backingError <= 0.0025,
			strictTargetWidth,
			strictTargetHeight,
			canvasCssMatchesStrict800x600: cssWidth === strictTargetWidth && cssHeight === strictTargetHeight,
			canvasBackingMatchesStrict800x600: backingWidth === strictTargetWidth && backingHeight === strictTargetHeight
		};
	}

	function getRenderMetricsSafe() {
		try {
			return parseJsonOrRaw(ccallSafe('PvzDebug_GetGameRenderMetricsJson', 'string', [], []));
		} catch (e) {
			return {
				hasApp: false,
				error: String((e && e.message) ? e.message : e)
			};
		}
	}

	function getAspectReport() {
		return {
			render: getRenderMetricsSafe(),
			canvas: getCanvasMetrics(),
			timestampMs: Date.now()
		};
	}

	function getAspectReportJson() {
		return JSON.stringify(getAspectReport(), null, 2);
	}

	function updateAspectMetricsHud() {
		if (!window.__pvzAspectMetricsEnabled) {
			return;
		}
		let hud = document.getElementById('pvz-aspect-metrics-hud');
		if (!hud) {
			hud = document.createElement('pre');
			hud.id = 'pvz-aspect-metrics-hud';
			hud.style.position = 'fixed';
			hud.style.left = '8px';
			hud.style.top = '8px';
			hud.style.zIndex = '2147483647';
			hud.style.margin = '0';
			hud.style.padding = '8px 10px';
			hud.style.background = 'rgba(0,0,0,0.72)';
			hud.style.color = '#8dff8d';
			hud.style.border = '1px solid rgba(141,255,141,0.6)';
			hud.style.font = '12px/1.3 monospace';
			hud.style.whiteSpace = 'pre';
			hud.style.pointerEvents = 'none';
			document.body.appendChild(hud);
		}

		const report = getAspectReport();
		const render = report.render || {};
		const canvas = report.canvas || {};
		const renderSize = `${render.appWidth || 0}x${render.appHeight || 0}`;
		const cssSize = `${canvas.canvasCssWidth || 0}x${canvas.canvasCssHeight || 0}`;
		const backingSize = `${canvas.canvasBackingWidth || 0}x${canvas.canvasBackingHeight || 0}`;

		hud.textContent =
			`aspect debug (strict 800x600)\n` +
			`render: ${renderSize}  aspect=${(render.appAspect || 0).toFixed(6)}  is43=${!!render.appAspectIs43}\n` +
			`canvas css: ${cssSize}  is800x600=${!!canvas.canvasCssMatchesStrict800x600}  is43=${!!canvas.canvasCssAspectIs43}\n` +
			`canvas backing: ${backingSize}  is800x600=${!!canvas.canvasBackingMatchesStrict800x600}  is43=${!!canvas.canvasBackingAspectIs43}\n` +
			`viewport: ${(canvas.viewportWidth || 0)}x${(canvas.viewportHeight || 0)} fullscreen=${!!canvas.isFullscreenActive}`;
	}

	const api = {
		__installed: true,
		TestFunction: (a, b) => parseJsonOrRaw(ccallSafe('PvzDebug_TestFunction', 'string', ['string', 'number'], [String(a), Number(b) || 0])),
		testFunction: (a, b) => parseJsonOrRaw(ccallSafe('PvzDebug_TestFunction', 'string', ['string', 'number'], [String(a), Number(b) || 0])),
		hasLawnApp: () => !!ccallSafe('PvzDebug_HasLawnApp', 'number', [], []),
		GetClientSessionState: () => ccallSafe('PvzDebug_GetClientSessionState', 'string', [], []),
		getClientSessionState: () => ccallSafe('PvzDebug_GetClientSessionState', 'string', [], []),
		EnableAuthoritativeRuntime: () => !!ccallSafe('PvzDebug_EnableAuthoritativeRuntime', 'number', [], []),
		enableAuthoritativeRuntime: () => !!ccallSafe('PvzDebug_EnableAuthoritativeRuntime', 'number', [], []),
		StartStoryModeWithOpponentSearch: (lookForSavedGame = 0) => !!ccallSafe('PvzDebug_StartStoryModeWithOpponentSearch', 'number', ['number'], [lookForSavedGame | 0]),
		startStoryModeWithOpponentSearch: (lookForSavedGame = 0) => !!ccallSafe('PvzDebug_StartStoryModeWithOpponentSearch', 'number', ['number'], [lookForSavedGame | 0]),
		StartSurvivalModeWithOpponentSearch: (lookForSavedGame = 0) => !!ccallSafe('PvzDebug_StartSurvivalModeWithOpponentSearch', 'number', ['number'], [lookForSavedGame | 0]),
		startSurvivalModeWithOpponentSearch: (lookForSavedGame = 0) => !!ccallSafe('PvzDebug_StartSurvivalModeWithOpponentSearch', 'number', ['number'], [lookForSavedGame | 0]),
		isSelectorReady: () => !!ccallSafe('PvzDebug_IsSelectorReady', 'number', [], []),
		StartAdventureFromSelector: () => !!ccallSafe('PvzDebug_StartAdventureFromSelector', 'number', [], []),
		startAdventureFromSelector: () => !!ccallSafe('PvzDebug_StartAdventureFromSelector', 'number', [], []),
		SubmitPlantCommand: (x, y, seedType, imitaterSeedType = -1) => !!ccallSafe('PvzDebug_SubmitPlantCommand', 'number', ['number', 'number', 'number', 'number'], [x | 0, y | 0, seedType | 0, imitaterSeedType | 0]),
		submitPlantCommand: (x, y, seedType, imitaterSeedType = -1) => !!ccallSafe('PvzDebug_SubmitPlantCommand', 'number', ['number', 'number', 'number', 'number'], [x | 0, y | 0, seedType | 0, imitaterSeedType | 0]),
		SubmitRemovePlantCommand: (x, y) => !!ccallSafe('PvzDebug_SubmitRemovePlantCommand', 'number', ['number', 'number'], [x | 0, y | 0]),
		submitRemovePlantCommand: (x, y) => !!ccallSafe('PvzDebug_SubmitRemovePlantCommand', 'number', ['number', 'number'], [x | 0, y | 0]),
		getBoardSun: () => ccallSafe('PvzDebug_GetBoardSun', 'number', [], []),
		getGameMode: () => parseJsonOrRaw(ccallSafe('PvzDebug_GetGameModeJson', 'string', [], [])),
		getGameRenderMetrics: () => getRenderMetricsSafe(),
		getCanvasMetrics: () => getCanvasMetrics(),
		getAspectReport: () => getAspectReport(),
		getAspectReportJson: () => getAspectReportJson(),
		printAspectReport: () => {
			const text = getAspectReportJson();
			console.info('[PvzDebug][AspectReport]\n' + text);
			return text;
		},
		getLatestSnapshot: () => parseJsonOrRaw(ccallSafe('PvzDebug_GetLatestSnapshotJson', 'string', [], [])),
		getSessionEvents: (limit = 32) => parseJsonOrRaw(ccallSafe('PvzDebug_GetSessionEventsJson', 'string', ['number'], [limit | 0])),
		clearSessionEvents: () => ccallSafe('PvzDebug_ClearSessionEvents', 'void', [], []),
		getAuthoritativeProfile: () => parseJsonOrRaw(ccallSafe('PvzDebug_GetAuthoritativeProfileJson', 'string', [], [])),
		getYandexStatus: () => parseJsonOrRaw(ccallSafe('PvzDebug_GetYandexStatusJson', 'string', [], [])),
		validatePlaceCommand: (matchId, playerId, commandId, clientTick, gridX, gridY, seedType, imitaterSeedType = -1) =>
			parseJsonOrRaw(ccallSafe('PvzDebug_ValidatePlaceCommandJson', 'string',
				['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number'],
				[matchId | 0, playerId | 0, commandId | 0, clientTick | 0, gridX | 0, gridY | 0, seedType | 0, imitaterSeedType | 0])),
		assignBotTracks: (lobbyId, mmrHint, botCount) =>
			parseJsonOrRaw(ccallSafe('PvzDebug_AssignBotTracksJson', 'string', ['number', 'number', 'number'], [lobbyId | 0, mmrHint | 0, botCount | 0])),
		getBotTrack: (trackId) => parseJsonOrRaw(ccallSafe('PvzDebug_GetBotTrackJson', 'string', ['number'], [trackId | 0])),
		listBotTracks: (limit = 20) => parseJsonOrRaw(ccallSafe('PvzDebug_ListBotTracksJson', 'string', ['number'], [limit | 0])),
		getRuntimeTelemetry: () => parseJsonOrRaw(ccallSafe('PvzDebug_GetRuntimeTelemetryJson', 'string', [], []))
	};

	window.PvzDebug = api;

	function applyCanvasAspect43() {
		if (typeof Module === 'undefined' || !Module['canvas']) {
			return false;
		}

		const canvas = Module['canvas'];
		const targetW = 800;
		const targetH = 600;
		const viewW = Math.max(1, window.innerWidth || document.documentElement.clientWidth || targetW);
		const viewH = Math.max(1, window.innerHeight || document.documentElement.clientHeight || targetH);
		const left = Math.floor((viewW - targetW) / 2);
		const top = Math.floor((viewH - targetH) / 2);

		if (document.documentElement) {
			document.documentElement.style.margin = '0';
			document.documentElement.style.padding = '0';
			document.documentElement.style.background = '#000';
			document.documentElement.style.overflow = 'hidden';
		}
		if (document.body) {
			document.body.style.margin = '0';
			document.body.style.padding = '0';
			document.body.style.background = '#000';
			document.body.style.overflow = 'hidden';
		}

		// Enforce strict fixed 800x600 presentation in all modes (windowed/fullscreen).
		canvas.style.setProperty('position', 'fixed', 'important');
		canvas.style.setProperty('left', left + 'px', 'important');
		canvas.style.setProperty('top', top + 'px', 'important');
		canvas.style.setProperty('width', targetW + 'px', 'important');
		canvas.style.setProperty('height', targetH + 'px', 'important');
		canvas.style.setProperty('max-width', 'none', 'important');
		canvas.style.setProperty('max-height', 'none', 'important');
		canvas.style.setProperty('display', 'block', 'important');
		canvas.style.setProperty('margin', '0', 'important');
		canvas.style.setProperty('transform', 'none', 'important');

		// Force fixed render resolution.
		if (typeof Module.setCanvasSize === 'function') {
			if (canvas.width !== targetW || canvas.height !== targetH) {
				Module.setCanvasSize(targetW, targetH, false);
			}
		}
		return true;
	}

	function scheduleAspect43Apply() {
		applyCanvasAspect43();
		updateAspectMetricsHud();
		// Fullscreen transitions can re-apply browser styles; enforce again on next frames.
		window.requestAnimationFrame(applyCanvasAspect43);
		window.setTimeout(applyCanvasAspect43, 60);
		window.setTimeout(applyCanvasAspect43, 250);
	}

	try {
		const params = new URLSearchParams(window.location.search || "");
		window.__pvzAspectMetricsEnabled = params.get('pvz_aspect_metrics') === '1';
	} catch (e) {
		window.__pvzAspectMetricsEnabled = false;
	}

	scheduleAspect43Apply();
	if (!window.__pvzAspect43Bound) {
		window.__pvzAspect43Bound = true;
		window.addEventListener('resize', scheduleAspect43Apply);
		window.addEventListener('orientationchange', scheduleAspect43Apply);
		document.addEventListener('fullscreenchange', scheduleAspect43Apply);
		document.addEventListener('webkitfullscreenchange', scheduleAspect43Apply);
		document.addEventListener('mozfullscreenchange', scheduleAspect43Apply);
		document.addEventListener('MSFullscreenChange', scheduleAspect43Apply);
		document.addEventListener('visibilitychange', () => {
			if (!document.hidden) {
				scheduleAspect43Apply();
			}
		});

		// Keep aspect locked even if external scripts mutate canvas style.
		window.__pvzAspect43Timer = window.setInterval(() => {
			applyCanvasAspect43();
			updateAspectMetricsHud();
		}, 500);
	}

	try {
		const params = new URLSearchParams(window.location.search || "");
		if (params.get('pvz_auto_runtime') === '1') {
			let attempts = 0;
			const timer = setInterval(() => {
				attempts += 1;
				try {
					ccallSafe('PvzDebug_RequestAutoRuntimeStart', 'void', [], []);
					clearInterval(timer);
				} catch (e) {
					if (attempts > 80) clearInterval(timer);
				}
			}, 250);
		}
	} catch (e) {}

	if (window.__pvzAspectMetricsEnabled && !window.__pvzAspectMetricsHudTimer) {
		updateAspectMetricsHud();
		window.__pvzAspectMetricsHudTimer = window.setInterval(updateAspectMetricsHud, 500);
	}

	console.info('[PvzDebug] bridge installed. Use window.PvzDebug.TestFunction(\"abc\", 1)');
	return 1;
});

void PvzInstallDebugConsoleBridge()
{
	static bool sInstalled = false;
	if (sInstalled)
	{
		return;
	}
	sInstalled = PvzInstallDebugConsoleBridgeJs() != 0;
}

bool PvzHasPendingAutoRuntimeStart()
{
	return gPvzAutoRuntimeStartRequested && !gPvzAutoRuntimeStartConsumed;
}

void PvzConsumeAutoRuntimeStart()
{
	gPvzAutoRuntimeStartConsumed = true;
}

#endif
