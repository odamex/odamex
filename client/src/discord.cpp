#include "doomtype.h"
#include "g_warmup.h"
#include "discord.h"
#include "cl_demo.h"

CVAR (cl_discord_enable, "1", "Enables Discord Integration systems", CVARTYPE_BOOL, CVAR_ARCHIVE )

static const char* APPLICATION_ID = "378566834523209728";
extern NetDemo	netdemo;

static void handleDiscordReady(const DiscordUser* connectedUser)
{
	Printf(PRINT_HIGH, "\nDiscord: connected to user %s#%s - %s\n",
		connectedUser->username,
		connectedUser->discriminator,
		connectedUser->userId);
}

static void handleDiscordDisconnected(int errcode, const char* message)
{
	Printf(PRINT_HIGH, "\nDiscord: disconnected (%d: %s)\n", errcode, message);
}

static void handleDiscordError(int errcode, const char* message)
{
	Printf(PRINT_HIGH, "\nDiscord: error (%d: %s)\n", errcode, message);
}

static char* DISCORD_EventToString(discord_state_s state)
{
	char *strValue;

	switch (state)
	{
	case DISCORD_INMENU:		strValue = "In the menu"; break;
	case DISCORD_DEMOPLAYING:	strValue = "Watching a demo"; break;
	case DISCORD_SOLOPLAY:		strValue = "Playing Singleplayer"; break;
	case DISCORD_INTERMISSION:	strValue = "In intermission"; break;
	case DISCORD_SPECTATING:	strValue = "Spectating a game"; break;
	case DISCORD_INGAME:		strValue = "In a game"; break;
	case DISCORD_WARMUP:		strValue = "Warming-up"; break;
	case DISCORD_COUNTDOWN:		strValue = "Match countdown..."; break;
	case DISCORD_INMATCH:		strValue = "In a match"; break;
	default:
		strValue = "Unknown info"; break;
	}

	DPrintf("[DISCORD] Updating to this state => %s", strValue);
	return strValue;
}

void DISCORD_UpdateState(std::string state, std::string details, discord_logotype_s logotype, std::string logoname)
{
	char buffer[256];
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
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
}

void DISCORD_UpdateInGameState(discord_state_s state, std::string details, discord_logotype_s logotype, std::string logoname)
{
	if (democlassic || demoplayback || netdemo.isPlaying())
		DISCORD_UpdateState(DISCORD_DEMOPLAYING, "", DLOGO_LARGEPIC, "odamex-demo");	// GROSS HACK BUT WORKS
	else
		DISCORD_UpdateState(state, details, logotype, logoname);
}

void DISCORD_UpdateState(discord_state_s state, std::string details, discord_logotype_s logotype, std::string logoname)
{
	if (!cl_discord_enable)
		return;
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	discordPresence.details = DISCORD_EventToString(state);
	discordPresence.state =  details.c_str();

	if (logotype != DLOGO_NONE && logoname != "")
	{
		if (logotype == DLOGO_LARGEPIC)
			discordPresence.largeImageKey = logoname.c_str();
		else if (logotype == DLOGO_SMALLPIC)
			discordPresence.smallImageKey = logoname.c_str();
	}
	discordPresence.instance = 0;
	Discord_UpdatePresence(&discordPresence);
}

void DISCORD_Init()
{
		DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	handlers.ready = handleDiscordReady;
	handlers.disconnected = handleDiscordDisconnected;
	handlers.errored = handleDiscordError;
	/*handlers.joinGame = handleDiscordJoin;
	handlers.spectateGame = handleDiscordSpectate;
	handlers.joinRequest = handleDiscordJoinRequest;*/
	Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);

	DPrintf("Initializing Discord");
}