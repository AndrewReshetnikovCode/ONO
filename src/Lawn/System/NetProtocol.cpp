#include "NetProtocol.h"

#include <sstream>

const NetProtocolVersion NET_PROTOCOL_VERSION_V1 = {1, 0};

static NetCommandValidationResult MakeValidationResult(NetProtocolError theError, const std::string& theReason = "")
{
	NetCommandValidationResult aResult;
	aResult.mError = theError;
	aResult.mReason = theReason;
	return aResult;
}

NetProtocolCompatibility NetCheckProtocolCompatibility(const NetProtocolVersion& theClientVersion, const NetProtocolVersion& theServerVersion)
{
	if (theClientVersion.mMajor == theServerVersion.mMajor)
	{
		if (theClientVersion.mMinor > theServerVersion.mMinor)
		{
			return NetProtocolCompatibility::SERVER_UPGRADE_REQUIRED;
		}
		return NetProtocolCompatibility::COMPATIBLE;
	}

	if (theClientVersion.mMajor < theServerVersion.mMajor)
	{
		return NetProtocolCompatibility::CLIENT_UPGRADE_REQUIRED;
	}

	return NetProtocolCompatibility::SERVER_UPGRADE_REQUIRED;
}

bool NetIsVersionCompatible(const NetProtocolVersion& theClientVersion, const NetProtocolVersion& theServerVersion)
{
	return NetCheckProtocolCompatibility(theClientVersion, theServerVersion) == NetProtocolCompatibility::COMPATIBLE;
}

std::string NetProtocolVersionToString(const NetProtocolVersion& theVersion)
{
	return std::to_string(theVersion.mMajor) + "." + std::to_string(theVersion.mMinor);
}

const char* NetCommandTypeToString(NetCommandType theCommandType)
{
	switch (theCommandType)
	{
	case NetCommandType::NET_COMMAND_INVALID: return "NET_COMMAND_INVALID";
	case NetCommandType::NET_COMMAND_PLACE_PLANT: return "NET_COMMAND_PLACE_PLANT";
	case NetCommandType::NET_COMMAND_REMOVE_PLANT: return "NET_COMMAND_REMOVE_PLANT";
	case NetCommandType::NET_COMMAND_SEND_PVP_ZOMBIES: return "NET_COMMAND_SEND_PVP_ZOMBIES";
	case NetCommandType::NET_COMMAND_STORE_PURCHASE_INTENT: return "NET_COMMAND_STORE_PURCHASE_INTENT";
	case NetCommandType::NET_COMMAND_HEARTBEAT: return "NET_COMMAND_HEARTBEAT";
	}

	return "NET_COMMAND_UNKNOWN";
}

const char* NetEventTypeToString(NetEventType theEventType)
{
	switch (theEventType)
	{
	case NetEventType::NET_EVENT_INVALID: return "NET_EVENT_INVALID";
	case NetEventType::NET_EVENT_COMMAND_ACCEPTED: return "NET_EVENT_COMMAND_ACCEPTED";
	case NetEventType::NET_EVENT_COMMAND_REJECTED: return "NET_EVENT_COMMAND_REJECTED";
	case NetEventType::NET_EVENT_SNAPSHOT_APPLY: return "NET_EVENT_SNAPSHOT_APPLY";
	case NetEventType::NET_EVENT_PVP_PHASE_START: return "NET_EVENT_PVP_PHASE_START";
	case NetEventType::NET_EVENT_PVP_PHASE_END: return "NET_EVENT_PVP_PHASE_END";
	case NetEventType::NET_EVENT_MATCH_START: return "NET_EVENT_MATCH_START";
	case NetEventType::NET_EVENT_MATCH_END: return "NET_EVENT_MATCH_END";
	case NetEventType::NET_EVENT_DIAMONDS_UPDATED: return "NET_EVENT_DIAMONDS_UPDATED";
	}

	return "NET_EVENT_UNKNOWN";
}

const char* NetProtocolErrorToString(NetProtocolError theError)
{
	switch (theError)
	{
	case NetProtocolError::NET_PROTOCOL_OK: return "NET_PROTOCOL_OK";
	case NetProtocolError::NET_PROTOCOL_UNSUPPORTED_VERSION: return "NET_PROTOCOL_UNSUPPORTED_VERSION";
	case NetProtocolError::NET_PROTOCOL_INVALID_ENVELOPE: return "NET_PROTOCOL_INVALID_ENVELOPE";
	case NetProtocolError::NET_PROTOCOL_INVALID_PAYLOAD: return "NET_PROTOCOL_INVALID_PAYLOAD";
	case NetProtocolError::NET_PROTOCOL_INVALID_IDEMPOTENCY_KEY: return "NET_PROTOCOL_INVALID_IDEMPOTENCY_KEY";
	case NetProtocolError::NET_PROTOCOL_UNSUPPORTED_COMMAND: return "NET_PROTOCOL_UNSUPPORTED_COMMAND";
	case NetProtocolError::NET_PROTOCOL_COMMAND_TYPE_PAYLOAD_MISMATCH: return "NET_PROTOCOL_COMMAND_TYPE_PAYLOAD_MISMATCH";
	}

	return "NET_PROTOCOL_UNKNOWN_ERROR";
}

const char* NetAuthorityDomainToString(NetAuthorityDomain theDomain)
{
	switch (theDomain)
	{
	case NetAuthorityDomain::NET_AUTH_WAVE_PROGRESSION: return "NET_AUTH_WAVE_PROGRESSION";
	case NetAuthorityDomain::NET_AUTH_SUN_GENERATION: return "NET_AUTH_SUN_GENERATION";
	case NetAuthorityDomain::NET_AUTH_DAMAGE_RESOLUTION: return "NET_AUTH_DAMAGE_RESOLUTION";
	case NetAuthorityDomain::NET_AUTH_PVP_SPAWN_LIMITS: return "NET_AUTH_PVP_SPAWN_LIMITS";
	case NetAuthorityDomain::NET_AUTH_REWARD_CALCULATION: return "NET_AUTH_REWARD_CALCULATION";
	case NetAuthorityDomain::NET_AUTH_MMR_CALCULATION: return "NET_AUTH_MMR_CALCULATION";
	case NetAuthorityDomain::NET_AUTH_DIAMONDS_STORAGE: return "NET_AUTH_DIAMONDS_STORAGE";
	case NetAuthorityDomain::NET_AUTH_COSMETICS_OWNERSHIP: return "NET_AUTH_COSMETICS_OWNERSHIP";
	case NetAuthorityDomain::NET_AUTH_WEEKLY_SCORE: return "NET_AUTH_WEEKLY_SCORE";
	case NetAuthorityDomain::NET_AUTH_PLAYER_INPUT_CAPTURE: return "NET_AUTH_PLAYER_INPUT_CAPTURE";
	case NetAuthorityDomain::NET_AUTH_RENDERING: return "NET_AUTH_RENDERING";
	case NetAuthorityDomain::NET_AUTH_LOCAL_VISUAL_PREDICTION: return "NET_AUTH_LOCAL_VISUAL_PREDICTION";
	}

	return "NET_AUTH_UNKNOWN";
}

const char* NetAuthorityOwnerToString(NetAuthorityOwner theOwner)
{
	switch (theOwner)
	{
	case NetAuthorityOwner::NET_OWNER_SERVER: return "NET_OWNER_SERVER";
	case NetAuthorityOwner::NET_OWNER_CLIENT: return "NET_OWNER_CLIENT";
	}

	return "NET_OWNER_UNKNOWN";
}

NetCommandValidationResult NetValidateCommandEnvelope(const NetCommandEnvelope& theEnvelope)
{
	if (!NetIsVersionCompatible(theEnvelope.mVersion, NET_PROTOCOL_VERSION_V1))
	{
		std::string aReason = "unsupported protocol version: client=" + NetProtocolVersionToString(theEnvelope.mVersion) +
			", server=" + NetProtocolVersionToString(NET_PROTOCOL_VERSION_V1);
		return MakeValidationResult(NetProtocolError::NET_PROTOCOL_UNSUPPORTED_VERSION, aReason);
	}

	if (theEnvelope.mMatchId == 0 || theEnvelope.mPlayerId == 0 || theEnvelope.mCommandId == 0)
	{
		return MakeValidationResult(NetProtocolError::NET_PROTOCOL_INVALID_ENVELOPE, "matchId/playerId/commandId must be non-zero");
	}

	if (theEnvelope.mCommandType == NetCommandType::NET_COMMAND_INVALID)
	{
		return MakeValidationResult(NetProtocolError::NET_PROTOCOL_UNSUPPORTED_COMMAND, "command type is NET_COMMAND_INVALID");
	}

	NetCommandIdempotencyKey aKey = NetMakeCommandIdempotencyKey(theEnvelope);
	if (!NetIsValidCommandIdempotencyKey(aKey))
	{
		return MakeValidationResult(NetProtocolError::NET_PROTOCOL_INVALID_IDEMPOTENCY_KEY, "invalid idempotency key");
	}

	return MakeValidationResult(NetProtocolError::NET_PROTOCOL_OK);
}

NetCommandValidationResult NetValidateClientCommand(const NetClientCommand& theCommand)
{
	NetCommandValidationResult aEnvelopeResult = NetValidateCommandEnvelope(theCommand.mEnvelope);
	if (!aEnvelopeResult.IsOk())
	{
		return aEnvelopeResult;
	}

	switch (theCommand.mEnvelope.mCommandType)
	{
	case NetCommandType::NET_COMMAND_PLACE_PLANT:
	{
		if (!std::holds_alternative<NetPlacePlantPayload>(theCommand.mPayload))
		{
			return MakeValidationResult(NetProtocolError::NET_PROTOCOL_COMMAND_TYPE_PAYLOAD_MISMATCH, "place plant command payload mismatch");
		}

		const NetPlacePlantPayload& aPayload = std::get<NetPlacePlantPayload>(theCommand.mPayload);
		if (aPayload.mGridX < 0 || aPayload.mGridY < 0)
		{
			return MakeValidationResult(NetProtocolError::NET_PROTOCOL_INVALID_PAYLOAD, "grid coordinates must be non-negative");
		}
		if (aPayload.mSeedType < 0 || aPayload.mImitaterSeedType < -1)
		{
			return MakeValidationResult(NetProtocolError::NET_PROTOCOL_INVALID_PAYLOAD, "seed types are invalid");
		}
		break;
	}

	case NetCommandType::NET_COMMAND_REMOVE_PLANT:
	{
		if (!std::holds_alternative<NetRemovePlantPayload>(theCommand.mPayload))
		{
			return MakeValidationResult(NetProtocolError::NET_PROTOCOL_COMMAND_TYPE_PAYLOAD_MISMATCH, "remove plant command payload mismatch");
		}

		const NetRemovePlantPayload& aPayload = std::get<NetRemovePlantPayload>(theCommand.mPayload);
		if (aPayload.mGridX < 0 || aPayload.mGridY < 0)
		{
			return MakeValidationResult(NetProtocolError::NET_PROTOCOL_INVALID_PAYLOAD, "grid coordinates must be non-negative");
		}
		break;
	}

	case NetCommandType::NET_COMMAND_SEND_PVP_ZOMBIES:
	{
		if (!std::holds_alternative<NetSendPvpZombiesPayload>(theCommand.mPayload))
		{
			return MakeValidationResult(NetProtocolError::NET_PROTOCOL_COMMAND_TYPE_PAYLOAD_MISMATCH, "send pvp zombies command payload mismatch");
		}

		const NetSendPvpZombiesPayload& aPayload = std::get<NetSendPvpZombiesPayload>(theCommand.mPayload);
		if (aPayload.mTargetPlayerId == 0 || aPayload.mTargetPlayerId == theCommand.mEnvelope.mPlayerId)
		{
			return MakeValidationResult(NetProtocolError::NET_PROTOCOL_INVALID_PAYLOAD, "target player must be non-zero and different from sender");
		}
		if (aPayload.mZombieType < 0)
		{
			return MakeValidationResult(NetProtocolError::NET_PROTOCOL_INVALID_PAYLOAD, "zombie type must be non-negative");
		}
		if (aPayload.mZombieCount <= 0 || aPayload.mZombieCount > 100)
		{
			return MakeValidationResult(NetProtocolError::NET_PROTOCOL_INVALID_PAYLOAD, "zombie count must be in [1, 100]");
		}
		break;
	}

	case NetCommandType::NET_COMMAND_STORE_PURCHASE_INTENT:
	{
		if (!std::holds_alternative<NetStorePurchaseIntentPayload>(theCommand.mPayload))
		{
			return MakeValidationResult(NetProtocolError::NET_PROTOCOL_COMMAND_TYPE_PAYLOAD_MISMATCH, "store purchase command payload mismatch");
		}

		const NetStorePurchaseIntentPayload& aPayload = std::get<NetStorePurchaseIntentPayload>(theCommand.mPayload);
		if (aPayload.mSku.empty() || aPayload.mSku.size() > 64)
		{
			return MakeValidationResult(NetProtocolError::NET_PROTOCOL_INVALID_PAYLOAD, "sku must be non-empty and <= 64 chars");
		}
		if (aPayload.mQuantity <= 0 || aPayload.mQuantity > 100)
		{
			return MakeValidationResult(NetProtocolError::NET_PROTOCOL_INVALID_PAYLOAD, "quantity must be in [1, 100]");
		}
		break;
	}

	case NetCommandType::NET_COMMAND_HEARTBEAT:
	{
		if (!std::holds_alternative<NetHeartbeatPayload>(theCommand.mPayload))
		{
			return MakeValidationResult(NetProtocolError::NET_PROTOCOL_COMMAND_TYPE_PAYLOAD_MISMATCH, "heartbeat command payload mismatch");
		}
		break;
	}

	case NetCommandType::NET_COMMAND_INVALID:
	default:
		return MakeValidationResult(NetProtocolError::NET_PROTOCOL_UNSUPPORTED_COMMAND, "unsupported command type");
	}

	return MakeValidationResult(NetProtocolError::NET_PROTOCOL_OK);
}

NetAuthorityOwner NetGetAuthorityOwner(NetAuthorityDomain theDomain)
{
	switch (theDomain)
	{
	case NetAuthorityDomain::NET_AUTH_WAVE_PROGRESSION:
	case NetAuthorityDomain::NET_AUTH_SUN_GENERATION:
	case NetAuthorityDomain::NET_AUTH_DAMAGE_RESOLUTION:
	case NetAuthorityDomain::NET_AUTH_PVP_SPAWN_LIMITS:
	case NetAuthorityDomain::NET_AUTH_REWARD_CALCULATION:
	case NetAuthorityDomain::NET_AUTH_MMR_CALCULATION:
	case NetAuthorityDomain::NET_AUTH_DIAMONDS_STORAGE:
	case NetAuthorityDomain::NET_AUTH_COSMETICS_OWNERSHIP:
	case NetAuthorityDomain::NET_AUTH_WEEKLY_SCORE:
		return NetAuthorityOwner::NET_OWNER_SERVER;

	case NetAuthorityDomain::NET_AUTH_PLAYER_INPUT_CAPTURE:
	case NetAuthorityDomain::NET_AUTH_RENDERING:
	case NetAuthorityDomain::NET_AUTH_LOCAL_VISUAL_PREDICTION:
		return NetAuthorityOwner::NET_OWNER_CLIENT;
	}

	return NetAuthorityOwner::NET_OWNER_SERVER;
}

bool NetIsServerAuthoritative(NetAuthorityDomain theDomain)
{
	return NetGetAuthorityOwner(theDomain) == NetAuthorityOwner::NET_OWNER_SERVER;
}

std::vector<NetAuthorityDomain> NetGetServerAuthoritativeDomains()
{
	return {
		NetAuthorityDomain::NET_AUTH_WAVE_PROGRESSION,
		NetAuthorityDomain::NET_AUTH_SUN_GENERATION,
		NetAuthorityDomain::NET_AUTH_DAMAGE_RESOLUTION,
		NetAuthorityDomain::NET_AUTH_PVP_SPAWN_LIMITS,
		NetAuthorityDomain::NET_AUTH_REWARD_CALCULATION,
		NetAuthorityDomain::NET_AUTH_MMR_CALCULATION,
		NetAuthorityDomain::NET_AUTH_DIAMONDS_STORAGE,
		NetAuthorityDomain::NET_AUTH_COSMETICS_OWNERSHIP,
		NetAuthorityDomain::NET_AUTH_WEEKLY_SCORE
	};
}

NetCommandIdempotencyKey NetMakeCommandIdempotencyKey(const NetCommandEnvelope& theEnvelope)
{
	NetCommandIdempotencyKey aKey;
	aKey.mMatchId = theEnvelope.mMatchId;
	aKey.mPlayerId = theEnvelope.mPlayerId;
	aKey.mCommandId = theEnvelope.mCommandId;
	aKey.mClientTick = theEnvelope.mClientTick;
	return aKey;
}

bool NetIsValidCommandIdempotencyKey(const NetCommandIdempotencyKey& theKey)
{
	return theKey.mMatchId != 0 && theKey.mPlayerId != 0 && theKey.mCommandId != 0;
}

std::string NetIdempotencyKeyToString(const NetCommandIdempotencyKey& theKey)
{
	std::ostringstream aStream;
	aStream << "match=" << theKey.mMatchId
		<< ",player=" << theKey.mPlayerId
		<< ",command=" << theKey.mCommandId
		<< ",tick=" << theKey.mClientTick;
	return aStream.str();
}
