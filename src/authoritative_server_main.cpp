#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
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
	theConfig.mDurationSeconds = 20;
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

	AuthoritativeServerRuntime aServer(aConfig.mServerConfig);
	for (uint32_t i = 0; i < aConfig.mSyntheticPlayers; ++i)
	{
		uint64_t aPlayerId = static_cast<uint64_t>(i + 1);
		int aPlayerMmr = aConfig.mBaseMmr + static_cast<int>(i) * 10;
		bool anEnqueued = aServer.EnqueueMatchmakingRequest(aPlayerId, aPlayerMmr, aConfig.mMatchmakingMode);
		std::cout << "[server] enqueue playerId=" << aPlayerId << " mmr=" << aPlayerMmr << " result=" << (anEnqueued ? "ok" : "skip") << "\n";
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
			std::cout
				<< "[event] tick=" << aEvent.mTick
				<< " match=" << aEvent.mMatchId
				<< " player=" << aEvent.mPlayerId
				<< " type=" << NetEventTypeToString(aEvent.mEventType)
				<< " error=" << NetProtocolErrorToString(aEvent.mError)
				<< " details=\"" << aEvent.mDetails << "\"\n";
		}
		aServer.ClearEvents();

		if (aConfig.mStatsLogEveryTicks > 0 && (aTick % aConfig.mStatsLogEveryTicks) == 0)
		{
			std::cout
				<< "[stats] tick=" << aTick
				<< " queuedRandom=" << aServer.GetQueuedPlayers(AuthoritativeMatchmakingMode::MATCHMAKING_RANDOM)
				<< " queuedMmr=" << aServer.GetQueuedPlayers(AuthoritativeMatchmakingMode::MATCHMAKING_MMR)
				<< " activeMatches=" << aServer.GetActiveMatchCount()
				<< "\n";
		}

		if (aConfig.mDurationSeconds > 0)
		{
			auto aElapsed = std::chrono::steady_clock::now() - aStart;
			uint64_t aElapsedSeconds = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(aElapsed).count());
			if (aElapsedSeconds >= aConfig.mDurationSeconds)
			{
				std::cout << "[server] duration reached, shutting down\n";
				break;
			}
		}

		if (!aConfig.mDisableTickSleep && aTickDelay.count() > 0)
		{
			std::this_thread::sleep_for(aTickDelay);
		}
	}

	std::cout << "[server] stopped at tick=" << aServer.GetServerTick() << "\n";
	return 0;
}
