#include <discord_rpc.h>

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

void DISCORD_UpdateState(std::string text, std::string details, discord_logotype_s logotype = DLOGO_LARGEPIC, std::string logoname = "odamex-logo");
void DISCORD_UpdateState(discord_state_s state, std::string details, discord_logotype_s logotype = DLOGO_LARGEPIC, std::string logoname = "odamex-logo");
void DISCORD_UpdateInGameState(discord_state_s state, std::string details, discord_logotype_s logotype = DLOGO_LARGEPIC, std::string logoname = "odamex-logo");
void DISCORD_Init();