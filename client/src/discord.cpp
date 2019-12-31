#include "doomtype.h"
#include "g_warmup.h"
#include "discord.h"
#include "cl_demo.h"
#include <sstream>

DiscordRPCStatus discord;

#ifdef USE_DISCORDRPC
CVAR_FUNC_DECL	(cl_discord_enable, "1", "Enables Discord-RPC integration system.", CVARTYPE_BOOL, CVAR_ARCHIVE )

CVAR			(cl_discord_timestamps, "1", "Display time elapsed on Discord.", CVARTYPE_BOOL, CVAR_ARCHIVE )

CVAR_FUNC_IMPL(cl_discord_enable)
{
	if (var) {
		discord.Initialize();
	}	else {
		discord.Shutdown();
	}
}

static const char* APPLICATION_ID = "378566834523209728";
extern NetDemo	netdemo;
static int64_t StartTime;
#endif

EXTERN_CVAR(sv_maxplayers)
size_t P_NumPlayersInGame();
void DiscordRPCStatus::Shutdown()
{
#ifdef USE_DISCORDRPC
	Discord_Shutdown();
#endif
}

const char* DiscordRPCStatus::EventToString(discord_state_s state)
{
	std::string strValue;
	std::ostringstream buffer;

	switch (state)
	{
		case DISCORD_INMENU:		strValue = "In the menu"; break;
		case DISCORD_DEMOPLAYING:	strValue = "Watching a demo"; break;
		case DISCORD_SOLOPLAY:		strValue = "Playing Singleplayer"; break;
		case DISCORD_INTERMISSION:	strValue = "In intermission"; break;
		case DISCORD_SPECTATING:	strValue = "Spectating a game"; break;
		case DISCORD_WARMUP:		strValue = "Warming-up"; break;
		case DISCORD_COUNTDOWN:		strValue = "Match countdown..."; break;
		case DISCORD_INMATCH:		strValue = "In a match"; break;
		case DISCORD_INGAME:	
		{
			buffer << "In-game [" << P_NumPlayersInGame() << "/" << sv_maxplayers <<"]";
			strValue = buffer.str(); break;
		}
		default:
			strValue = "Unknown info"; break;
	}

	return strValue.c_str();
}

void DiscordRPCStatus::SetState(std::string state, std::string details, discord_logotype_s logotype, std::string logoname)
{
#ifdef USE_DISCORDRPC
	char buffer[256];
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));

	if (cl_discord_timestamps)
		discordPresence.startTimestamp = StartTime;

	discordPresence.details = state.c_str();
	discordPresence.state = details.c_str();

	if (logotype != DLOGO_NONE && logoname != "")
	{
		if (logotype == DLOGO_LARGEPIC)
			discordPresence.largeImageKey = logoname.c_str();
		else if (logotype == DLOGO_SMALLPIC)
			discordPresence.smallImageKey = logoname.c_str();
	}

	discordPresence.instance = 0;
	Discord_UpdatePresence(&discordPresence);
#endif
}

void DiscordRPCStatus::SetIngameState(discord_state_s state, std::string details, discord_logotype_s logotype, std::string logoname)
{
#ifdef USE_DISCORDRPC
	if (demoplayback || netdemo.isPlaying())
		SetState(DISCORD_DEMOPLAYING, "", DLOGO_LARGEPIC, "odamex-demo");	// GROSS HACK BUT WORKS
	else
		SetState(state, details, logotype, logoname);
#endif
}

void DiscordRPCStatus::SetState(discord_state_s state, std::string details, discord_logotype_s logotype, std::string logoname)
{
#ifdef USE_DISCORDRPC
	SetState(EventToString(state), details, logotype, logoname);
#endif
}

void DiscordRPCStatus::SetWarmupState(Warmup::status_t, std::string details)
{
	if (warmup.get_status() == Warmup::INGAME)
		discord.SetIngameState(DiscordRPCStatus::DISCORD_INMATCH, details);
	else if (warmup.get_status() == Warmup::WARMUP)
		discord.SetIngameState(DiscordRPCStatus::DISCORD_WARMUP, details);
	else
		discord.SetIngameState(DiscordRPCStatus::DISCORD_INGAME, details);
}

#ifdef USE_DISCORDRPC
static void handleDiscordReady(const DiscordUser* connectedUser)
{
	DPrintf("\nDiscord: connected to user %s#%s - %s\n",
		connectedUser->username,
		connectedUser->discriminator,
		connectedUser->userId);
}

static void handleDiscordDisconnected(int errcode, const char* message)
{
	DPrintf("\nDiscord: disconnected (%d: %s)\n", errcode, message);
}

static void handleDiscordError(int errcode, const char* message)
{
	DPrintf("\nDiscord: error (%d: %s)\n", errcode, message);
}
#endif

void DiscordRPCStatus::Initialize()
{
#ifdef USE_DISCORDRPC
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	handlers.ready = handleDiscordReady;
	handlers.disconnected = handleDiscordDisconnected;
	handlers.errored = handleDiscordError;

	StartTime = time(0);

	Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);

	DPrintf("Initializing Discord-RPC...\n");
#endif
}