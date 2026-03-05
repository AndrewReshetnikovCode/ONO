#ifdef __EMSCRIPTEN__

#include <algorithm>
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

EMSCRIPTEN_KEEPALIVE int PvzDebug_IsSelectorReady()
{
	LawnApp* aApp = PvzGetApp();
	if (aApp == nullptr)
	{
		return 0;
	}
	return aApp->mGameSelector != nullptr ? 1 : 0;
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
		<< "\"eliminated\":" << (aSnapshot.mEliminated ? "true" : "false")
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
		isSelectorReady: () => !!ccallSafe('PvzDebug_IsSelectorReady', 'number', [], []),
		StartAdventureFromSelector: () => !!ccallSafe('PvzDebug_StartAdventureFromSelector', 'number', [], []),
		startAdventureFromSelector: () => !!ccallSafe('PvzDebug_StartAdventureFromSelector', 'number', [], []),
		SubmitPlantCommand: (x, y, seedType, imitaterSeedType = -1) => !!ccallSafe('PvzDebug_SubmitPlantCommand', 'number', ['number', 'number', 'number', 'number'], [x | 0, y | 0, seedType | 0, imitaterSeedType | 0]),
		submitPlantCommand: (x, y, seedType, imitaterSeedType = -1) => !!ccallSafe('PvzDebug_SubmitPlantCommand', 'number', ['number', 'number', 'number', 'number'], [x | 0, y | 0, seedType | 0, imitaterSeedType | 0]),
		SubmitRemovePlantCommand: (x, y) => !!ccallSafe('PvzDebug_SubmitRemovePlantCommand', 'number', ['number', 'number'], [x | 0, y | 0]),
		submitRemovePlantCommand: (x, y) => !!ccallSafe('PvzDebug_SubmitRemovePlantCommand', 'number', ['number', 'number'], [x | 0, y | 0]),
		getBoardSun: () => ccallSafe('PvzDebug_GetBoardSun', 'number', [], []),
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

#endif
