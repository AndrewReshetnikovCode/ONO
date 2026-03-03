#ifndef __NETPROTOCOL_H__
#define __NETPROTOCOL_H__

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

struct NetProtocolVersion
{
	uint16_t					mMajor;
	uint16_t					mMinor;
};

extern const NetProtocolVersion	NET_PROTOCOL_VERSION_V1;

enum class NetProtocolCompatibility
{
	COMPATIBLE = 0,
	CLIENT_UPGRADE_REQUIRED,
	SERVER_UPGRADE_REQUIRED
};

enum class NetProtocolError
{
	NET_PROTOCOL_OK = 0,
	NET_PROTOCOL_UNSUPPORTED_VERSION,
	NET_PROTOCOL_INVALID_ENVELOPE,
	NET_PROTOCOL_INVALID_PAYLOAD,
	NET_PROTOCOL_INVALID_IDEMPOTENCY_KEY,
	NET_PROTOCOL_UNSUPPORTED_COMMAND,
	NET_PROTOCOL_COMMAND_TYPE_PAYLOAD_MISMATCH
};

enum class NetCommandType
{
	NET_COMMAND_INVALID = 0,
	NET_COMMAND_PLACE_PLANT,
	NET_COMMAND_REMOVE_PLANT,
	NET_COMMAND_SEND_PVP_ZOMBIES,
	NET_COMMAND_STORE_PURCHASE_INTENT,
	NET_COMMAND_HEARTBEAT
};

enum class NetEventType
{
	NET_EVENT_INVALID = 0,
	NET_EVENT_COMMAND_ACCEPTED,
	NET_EVENT_COMMAND_REJECTED,
	NET_EVENT_SNAPSHOT_APPLY,
	NET_EVENT_PVP_PHASE_START,
	NET_EVENT_PVP_PHASE_END,
	NET_EVENT_MATCH_START,
	NET_EVENT_MATCH_END,
	NET_EVENT_DIAMONDS_UPDATED
};

enum class NetAuthorityDomain
{
	NET_AUTH_WAVE_PROGRESSION = 0,
	NET_AUTH_SUN_GENERATION,
	NET_AUTH_DAMAGE_RESOLUTION,
	NET_AUTH_PVP_SPAWN_LIMITS,
	NET_AUTH_REWARD_CALCULATION,
	NET_AUTH_MMR_CALCULATION,
	NET_AUTH_DIAMONDS_STORAGE,
	NET_AUTH_COSMETICS_OWNERSHIP,
	NET_AUTH_WEEKLY_SCORE,
	NET_AUTH_PLAYER_INPUT_CAPTURE,
	NET_AUTH_RENDERING,
	NET_AUTH_LOCAL_VISUAL_PREDICTION
};

enum class NetAuthorityOwner
{
	NET_OWNER_SERVER = 0,
	NET_OWNER_CLIENT
};

struct NetCommandEnvelope
{
	NetProtocolVersion			mVersion;
	uint64_t					mMatchId;
	uint64_t					mPlayerId;
	uint64_t					mCommandId;
	uint32_t					mClientTick;
	NetCommandType				mCommandType;
};

struct NetPlacePlantPayload
{
	int							mGridX;
	int							mGridY;
	int							mSeedType;
	int							mImitaterSeedType;
};

struct NetRemovePlantPayload
{
	int							mGridX;
	int							mGridY;
};

struct NetSendPvpZombiesPayload
{
	uint64_t					mTargetPlayerId;
	int							mZombieType;
	int							mZombieCount;
};

struct NetStorePurchaseIntentPayload
{
	std::string					mSku;
	int							mQuantity;
};

struct NetHeartbeatPayload
{
	uint32_t					mClientFrame;
};

typedef std::variant<
	NetPlacePlantPayload,
	NetRemovePlantPayload,
	NetSendPvpZombiesPayload,
	NetStorePurchaseIntentPayload,
	NetHeartbeatPayload
> NetClientCommandPayload;

struct NetClientCommand
{
	NetCommandEnvelope			mEnvelope;
	NetClientCommandPayload		mPayload;
};

struct NetCommandValidationResult
{
	NetProtocolError			mError;
	std::string					mReason;

	inline bool					IsOk() const { return mError == NetProtocolError::NET_PROTOCOL_OK; }
};

struct NetCommandIdempotencyKey
{
	uint64_t					mMatchId;
	uint64_t					mPlayerId;
	uint64_t					mCommandId;
	uint32_t					mClientTick;
};

NetProtocolCompatibility		NetCheckProtocolCompatibility(const NetProtocolVersion& theClientVersion, const NetProtocolVersion& theServerVersion = NET_PROTOCOL_VERSION_V1);
bool							NetIsVersionCompatible(const NetProtocolVersion& theClientVersion, const NetProtocolVersion& theServerVersion = NET_PROTOCOL_VERSION_V1);
std::string						NetProtocolVersionToString(const NetProtocolVersion& theVersion);

const char*						NetCommandTypeToString(NetCommandType theCommandType);
const char*						NetEventTypeToString(NetEventType theEventType);
const char*						NetProtocolErrorToString(NetProtocolError theError);
const char*						NetAuthorityDomainToString(NetAuthorityDomain theDomain);
const char*						NetAuthorityOwnerToString(NetAuthorityOwner theOwner);

NetCommandValidationResult		NetValidateCommandEnvelope(const NetCommandEnvelope& theEnvelope);
NetCommandValidationResult		NetValidateClientCommand(const NetClientCommand& theCommand);

NetAuthorityOwner				NetGetAuthorityOwner(NetAuthorityDomain theDomain);
bool							NetIsServerAuthoritative(NetAuthorityDomain theDomain);
std::vector<NetAuthorityDomain>	NetGetServerAuthoritativeDomains();

NetCommandIdempotencyKey		NetMakeCommandIdempotencyKey(const NetCommandEnvelope& theEnvelope);
bool							NetIsValidCommandIdempotencyKey(const NetCommandIdempotencyKey& theKey);
std::string						NetIdempotencyKeyToString(const NetCommandIdempotencyKey& theKey);

#endif
