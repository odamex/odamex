
#ifdef USE_DISCORDRPC
#include "../discord-rpc/include/discord_rpc.h"
#endif

#include "g_warmup.h"

struct DiscordRPCStatus
{

public:

	enum discord_state_s
	{
		DISCORD_NONE,

		DISCORD_INMENU,
		DISCORD_DEMOPLAYING,
		DISCORD_SOLOPLAY,
		DISCORD_INTERMISSION,
		DISCORD_SPECTATING,
		DISCORD_INGAME,
		DISCORD_WARMUP,
		DISCORD_COUNTDOWN,
		DISCORD_INMATCH
	};

	enum discord_logotype_s
	{
		DLOGO_NONE,
		DLOGO_SMALLPIC,
		DLOGO_LARGEPIC
	};

	void Initialize();
	void Shutdown();
	void SetState(std::string state, std::string details, discord_logotype_s logotype = DLOGO_LARGEPIC, std::string logoname = "odamex-logo");
	void SetState(discord_state_s state, std::string details, discord_logotype_s logotype = DLOGO_LARGEPIC, std::string logoname = "odamex-logo");
	void SetIngameState(discord_state_s state, std::string details, discord_logotype_s logotype = DLOGO_LARGEPIC, std::string logoname = "odamex-logo");	// Ch0wW: maybe a better way to do it ?

	void DiscordRPCStatus::SetWarmupState(Warmup::status_t, std::string details);

private:

	const char *EventToString(discord_state_s state);

};

extern DiscordRPCStatus discord;