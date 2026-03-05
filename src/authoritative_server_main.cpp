#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include "Lawn/System/AuthoritativeRuntime.h"
#include "Lawn/System/NetProtocol.h"

namespace
{
std::atomic<bool> gKeepRunning(true);

struct ServerCliConfig
{
	AuthoritativeServerConfig		mServerConfig;
	AuthoritativeMatchmakingMode	mMatchmakingMode = AuthoritativeMatchmakingMode::MATCHMAKING_RANDOM;
	uint32_t						mTickRate = 30;
	uint64_t						mDurationSeconds = 20;
	uint32_t						mSyntheticPlayers = 6;
	int								mBaseMmr = 1000;
	uint64_t						mStatsLogEveryTicks = 150;
	bool							mDisableTickSleep = false;
	bool							mConsoleCoreOnly = true;
	std::string						mServerLogPath = "server_log.txt";
};

void PrintUsage(const char* theExeName)
{
	std::cout
		<< "Usage: " << theExeName << " [options]\n"
		<< "\n"
		<< "Profiles:\n"
		<< "  --profile local|cloud      Predefined runtime profile (default: local)\n"
		<< "\n"
		<< "Runtime options:\n"
		<< "  --tick-rate N              Authoritative ticks per second (default from profile)\n"
		<< "  --duration-seconds N       Runtime duration in seconds, 0 = run forever\n"
		<< "  --players N                Synthetic players to enqueue at startup\n"
		<< "  --mmr-base N               Base MMR used for synthetic players\n"
		<< "  --mode random|mmr          Matchmaking mode for synthetic players\n"
		<< "  --players-per-lobby N      AuthoritativeServerConfig::mPlayersPerLobby override\n"
		<< "  --min-players-to-start N   AuthoritativeServerConfig::mMinPlayersToStart override\n"
		<< "  --bot-fill-after-ticks N   AuthoritativeServerConfig::mBotFillAfterTicks override\n"
		<< "  --pvp-max-zombies N        AuthoritativeServerConfig::mPvpMaxZombiesPerPhase override\n"
		<< "  --log-every-ticks N        Periodic server stats interval\n"
		<< "  --server-log PATH          Write full server log to file path\n"
		<< "  --console-all-events       Print all runtime events to console\n"
		<< "  --disable-sleep            Run as fast as possible (no per-tick sleep)\n"
		<< "  --help                     Show this help\n";
}

void HandleSignal(int)
{
	gKeepRunning = false;
}

void ApplyLocalProfile(ServerCliConfig& theConfig)
{
	theConfig.mTickRate = 30;
	theConfig.mDurationSeconds = 0;
	theConfig.mSyntheticPlayers = 6;
	theConfig.mBaseMmr = 1000;
	theConfig.mStatsLogEveryTicks = 150;
	theConfig.mServerConfig.mPlayersPerLobby = 10;
	theConfig.mServerConfig.mMinPlayersToStart = 2;
	theConfig.mServerConfig.mBotFillAfterTicks = 90;
	theConfig.mServerConfig.mMmrWindow = 200;
}

void ApplyCloudProfile(ServerCliConfig& theConfig)
{
	theConfig.mTickRate = 30;
	theConfig.mDurationSeconds = 0;
	theConfig.mSyntheticPlayers = 20;
	theConfig.mBaseMmr = 1200;
	theConfig.mStatsLogEveryTicks = 300;
	theConfig.mServerConfig.mPlayersPerLobby = 10;
	theConfig.mServerConfig.mMinPlayersToStart = 2;
	theConfig.mServerConfig.mBotFillAfterTicks = 600;
	theConfig.mServerConfig.mMmrWindow = 250;
}

bool ParseUInt64(const char* theText, uint64_t& theOut)
{
	if (theText == nullptr || *theText == '\0')
	{
		return false;
	}

	char* aEnd = nullptr;
	unsigned long long aValue = std::strtoull(theText, &aEnd, 10);
	if (aEnd == theText || *aEnd != '\0')
	{
		return false;
	}

	theOut = static_cast<uint64_t>(aValue);
	return true;
}

bool ParseInt(const char* theText, int& theOut)
{
	if (theText == nullptr || *theText == '\0')
	{
		return false;
	}

	char* aEnd = nullptr;
	long aValue = std::strtol(theText, &aEnd, 10);
	if (aEnd == theText || *aEnd != '\0')
	{
		return false;
	}

	theOut = static_cast<int>(aValue);
	return true;
}

bool ParseMode(const std::string& theText, AuthoritativeMatchmakingMode& theOut)
{
	if (theText == "random")
	{
		theOut = AuthoritativeMatchmakingMode::MATCHMAKING_RANDOM;
		return true;
	}
	if (theText == "mmr")
	{
		theOut = AuthoritativeMatchmakingMode::MATCHMAKING_MMR;
		return true;
	}
	return false;
}

bool ParseProfile(const std::string& theText, ServerCliConfig& theConfig)
{
	if (theText == "local")
	{
		ApplyLocalProfile(theConfig);
		return true;
	}
	if (theText == "cloud")
	{
		ApplyCloudProfile(theConfig);
		return true;
	}
	return false;
}

bool ParseArgs(int argc, char** argv, ServerCliConfig& theConfig)
{
	ApplyLocalProfile(theConfig);

	for (int i = 1; i < argc; ++i)
	{
		const std::string aArg(argv[i]);

		auto NeedsValue = [&](const char* theName) -> const char*
		{
			if (i + 1 >= argc)
			{
				std::cerr << "Missing value for " << theName << "\n";
				return nullptr;
			}
			return argv[++i];
		};

		if (aArg == "--profile")
		{
			const char* aValue = NeedsValue("--profile");
			if (aValue == nullptr || !ParseProfile(aValue, theConfig))
			{
				std::cerr << "Invalid --profile value\n";
				return false;
			}
		}
		else if (aArg == "--tick-rate")
		{
			uint64_t aValue = 0;
			const char* aText = NeedsValue("--tick-rate");
			if (aText == nullptr || !ParseUInt64(aText, aValue) || aValue == 0)
			{
				std::cerr << "Invalid --tick-rate value\n";
				return false;
			}
			theConfig.mTickRate = static_cast<uint32_t>(aValue);
		}
		else if (aArg == "--duration-seconds")
		{
			uint64_t aValue = 0;
			const char* aText = NeedsValue("--duration-seconds");
			if (aText == nullptr || !ParseUInt64(aText, aValue))
			{
				std::cerr << "Invalid --duration-seconds value\n";
				return false;
			}
			theConfig.mDurationSeconds = aValue;
		}
		else if (aArg == "--players")
		{
			uint64_t aValue = 0;
			const char* aText = NeedsValue("--players");
			if (aText == nullptr || !ParseUInt64(aText, aValue))
			{
				std::cerr << "Invalid --players value\n";
				return false;
			}
			theConfig.mSyntheticPlayers = static_cast<uint32_t>(aValue);
		}
		else if (aArg == "--mmr-base")
		{
			int aValue = 0;
			const char* aText = NeedsValue("--mmr-base");
			if (aText == nullptr || !ParseInt(aText, aValue))
			{
				std::cerr << "Invalid --mmr-base value\n";
				return false;
			}
			theConfig.mBaseMmr = aValue;
		}
		else if (aArg == "--mode")
		{
			const char* aValue = NeedsValue("--mode");
			if (aValue == nullptr || !ParseMode(aValue, theConfig.mMatchmakingMode))
			{
				std::cerr << "Invalid --mode value\n";
				return false;
			}
		}
		else if (aArg == "--players-per-lobby")
		{
			uint64_t aValue = 0;
			const char* aText = NeedsValue("--players-per-lobby");
			if (aText == nullptr || !ParseUInt64(aText, aValue) || aValue == 0)
			{
				std::cerr << "Invalid --players-per-lobby value\n";
				return false;
			}
			theConfig.mServerConfig.mPlayersPerLobby = static_cast<uint32_t>(aValue);
		}
		else if (aArg == "--min-players-to-start")
		{
			uint64_t aValue = 0;
			const char* aText = NeedsValue("--min-players-to-start");
			if (aText == nullptr || !ParseUInt64(aText, aValue) || aValue == 0)
			{
				std::cerr << "Invalid --min-players-to-start value\n";
				return false;
			}
			theConfig.mServerConfig.mMinPlayersToStart = static_cast<uint32_t>(aValue);
		}
		else if (aArg == "--bot-fill-after-ticks")
		{
			uint64_t aValue = 0;
			const char* aText = NeedsValue("--bot-fill-after-ticks");
			if (aText == nullptr || !ParseUInt64(aText, aValue))
			{
				std::cerr << "Invalid --bot-fill-after-ticks value\n";
				return false;
			}
			theConfig.mServerConfig.mBotFillAfterTicks = aValue;
		}
		else if (aArg == "--pvp-max-zombies")
		{
			int aValue = 0;
			const char* aText = NeedsValue("--pvp-max-zombies");
			if (aText == nullptr || !ParseInt(aText, aValue) || aValue <= 0)
			{
				std::cerr << "Invalid --pvp-max-zombies value\n";
				return false;
			}
			theConfig.mServerConfig.mPvpMaxZombiesPerPhase = aValue;
		}
		else if (aArg == "--log-every-ticks")
		{
			uint64_t aValue = 0;
			const char* aText = NeedsValue("--log-every-ticks");
			if (aText == nullptr || !ParseUInt64(aText, aValue) || aValue == 0)
			{
				std::cerr << "Invalid --log-every-ticks value\n";
				return false;
			}
			theConfig.mStatsLogEveryTicks = aValue;
		}
		else if (aArg == "--server-log")
		{
			const char* aValue = NeedsValue("--server-log");
			if (aValue == nullptr || *aValue == '\0')
			{
				std::cerr << "Invalid --server-log value\n";
				return false;
			}
			theConfig.mServerLogPath = aValue;
		}
		else if (aArg == "--console-all-events")
		{
			theConfig.mConsoleCoreOnly = false;
		}
		else if (aArg == "--disable-sleep")
		{
			theConfig.mDisableTickSleep = true;
		}
		else
		{
			std::cerr << "Unknown argument: " << aArg << "\n";
			return false;
		}
	}

	return true;
}

void PrintConfig(const ServerCliConfig& theConfig)
{
	std::cout
		<< "[server] tickRate=" << theConfig.mTickRate
		<< " durationSeconds=" << theConfig.mDurationSeconds
		<< " players=" << theConfig.mSyntheticPlayers
		<< " matchmakingMode=" << (theConfig.mMatchmakingMode == AuthoritativeMatchmakingMode::MATCHMAKING_MMR ? "mmr" : "random")
		<< " playersPerLobby=" << theConfig.mServerConfig.mPlayersPerLobby
		<< " minPlayersToStart=" << theConfig.mServerConfig.mMinPlayersToStart
		<< " botFillAfterTicks=" << theConfig.mServerConfig.mBotFillAfterTicks
		<< "\n";
}

bool ContainsToken(const std::string& theText, const char* theToken)
{
	return theText.find(theToken) != std::string::npos;
}

bool IsCoreRuntimeEvent(const AuthoritativeRuntimeEvent& theEvent)
{
	switch (theEvent.mEventType)
	{
	case NetEventType::NET_EVENT_MATCH_START:
	case NetEventType::NET_EVENT_MATCH_END:
	case NetEventType::NET_EVENT_PVP_PHASE_START:
	case NetEventType::NET_EVENT_PVP_PHASE_END:
		return true;

	case NetEventType::NET_EVENT_SNAPSHOT_APPLY:
		// Keep bot creation and critical replay state visible.
		return ContainsToken(theEvent.mDetails, "[BOT] assigned trackId") ||
			ContainsToken(theEvent.mDetails, "[BOT] replay completed") ||
			ContainsToken(theEvent.mDetails, "[BOT] missing trackId");

	case NetEventType::NET_EVENT_COMMAND_REJECTED:
		// Keep elimination / disconnect / anti-cheat surface visible.
		return ContainsToken(theEvent.mDetails, "eliminated") ||
			ContainsToken(theEvent.mDetails, "disconnected") ||
			ContainsToken(theEvent.mDetails, "anti-cheat");

	case NetEventType::NET_EVENT_COMMAND_ACCEPTED:
	case NetEventType::NET_EVENT_DIAMONDS_UPDATED:
	case NetEventType::NET_EVENT_INVALID:
	default:
		return false;
	}
}

void WriteLine(std::ofstream& theLog, const std::string& theLine)
{
	if (theLog.is_open())
	{
		theLog << theLine << '\n';
		theLog.flush();
	}
}
} // namespace

int main(int argc, char** argv)
{
	for (int i = 1; i < argc; ++i)
	{
		if (std::string(argv[i]) == "--help")
		{
			PrintUsage(argv[0]);
			return 0;
		}
	}

	ServerCliConfig aConfig;
	if (!ParseArgs(argc, argv, aConfig))
	{
		PrintUsage(argv[0]);
		return 1;
	}

	std::signal(SIGINT, HandleSignal);
	std::signal(SIGTERM, HandleSignal);

	PrintConfig(aConfig);

	std::filesystem::path aLogPath(aConfig.mServerLogPath);
	if (!aLogPath.parent_path().empty())
	{
		std::filesystem::create_directories(aLogPath.parent_path());
	}
	std::ofstream aServerLog(aLogPath, std::ios::out | std::ios::trunc);
	if (!aServerLog.is_open())
	{
		std::cerr << "[server] failed to open log file: " << aConfig.mServerLogPath << "\n";
		return 1;
	}
	std::cout << "[core] full server log file: " << aConfig.mServerLogPath << "\n";
	WriteLine(aServerLog, std::string("[server] log started path=") + aConfig.mServerLogPath);

	AuthoritativeServerRuntime aServer(aConfig.mServerConfig);
	if (aConfig.mSyntheticPlayers > 0)
	{
		std::string aLine = "[core] matchmaking started syntheticPlayers=" + std::to_string(aConfig.mSyntheticPlayers);
		std::cout << aLine << "\n";
		WriteLine(aServerLog, aLine);
	}
	for (uint32_t i = 0; i < aConfig.mSyntheticPlayers; ++i)
	{
		uint64_t aPlayerId = static_cast<uint64_t>(i + 1);
		int aPlayerMmr = aConfig.mBaseMmr + static_cast<int>(i) * 10;
		bool anEnqueued = aServer.EnqueueMatchmakingRequest(aPlayerId, aPlayerMmr, aConfig.mMatchmakingMode);
		std::string aLine = std::string("[player] created playerId=") + std::to_string(aPlayerId) +
			" mmr=" + std::to_string(aPlayerMmr) +
			" enqueueResult=" + (anEnqueued ? "ok" : "skip");
		std::cout << aLine << "\n";
		WriteLine(aServerLog, aLine);
	}

	const auto aTickDelay = std::chrono::milliseconds(
		aConfig.mTickRate == 0 ? 0 : static_cast<int>(1000 / aConfig.mTickRate));

	const auto aStart = std::chrono::steady_clock::now();
	while (gKeepRunning)
	{
		aServer.AdvanceOneTick();
		uint64_t aTick = aServer.GetServerTick();

		const std::vector<AuthoritativeRuntimeEvent>& aEvents = aServer.GetEvents();
		for (const AuthoritativeRuntimeEvent& aEvent : aEvents)
		{
			std::string aLine = std::string("[event] tick=") + std::to_string(aEvent.mTick) +
				" match=" + std::to_string(aEvent.mMatchId) +
				" player=" + std::to_string(aEvent.mPlayerId) +
				" type=" + NetEventTypeToString(aEvent.mEventType) +
				" error=" + NetProtocolErrorToString(aEvent.mError) +
				" details=\"" + aEvent.mDetails + "\"";
			WriteLine(aServerLog, aLine);
			if (!aConfig.mConsoleCoreOnly || IsCoreRuntimeEvent(aEvent))
			{
				std::cout << aLine << "\n";
			}
		}
		aServer.ClearEvents();

		if (aConfig.mStatsLogEveryTicks > 0 && (aTick % aConfig.mStatsLogEveryTicks) == 0)
		{
			std::string aStatsLine = std::string("[stats] tick=") + std::to_string(aTick) +
				" queuedRandom=" + std::to_string(aServer.GetQueuedPlayers(AuthoritativeMatchmakingMode::MATCHMAKING_RANDOM)) +
				" queuedMmr=" + std::to_string(aServer.GetQueuedPlayers(AuthoritativeMatchmakingMode::MATCHMAKING_MMR)) +
				" activeMatches=" + std::to_string(aServer.GetActiveMatchCount());
			WriteLine(aServerLog, aStatsLine);
			if (!aConfig.mConsoleCoreOnly)
			{
				std::cout << aStatsLine << "\n";
			}
		}

		if (aConfig.mDurationSeconds > 0)
		{
			auto aElapsed = std::chrono::steady_clock::now() - aStart;
			uint64_t aElapsedSeconds = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(aElapsed).count());
			if (aElapsedSeconds >= aConfig.mDurationSeconds)
			{
				std::string aLine = "[core] duration reached, shutting down";
				std::cout << aLine << "\n";
				WriteLine(aServerLog, aLine);
				break;
			}
		}

		if (!aConfig.mDisableTickSleep && aTickDelay.count() > 0)
		{
			std::this_thread::sleep_for(aTickDelay);
		}
	}

	std::string aFinalLine = std::string("[core] stopped at tick=") + std::to_string(aServer.GetServerTick());
	std::cout << aFinalLine << "\n";
	WriteLine(aServerLog, aFinalLine);
	return 0;
}
