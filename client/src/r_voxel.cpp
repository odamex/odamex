// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2026 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

#include "cmdlib.h"
#include "m_bbox.h"
#include "m_fileio.h"
#include "oscanner.h"
#include "r_local.h"
#include "r_voxel.h"
#include "w_wad.h"
#include "z_zone.h"

EXTERN_CVAR(r_voxels)
EXTERN_CVAR(r_voxeldir)
EXTERN_CVAR(sv_allowvoxels)
void R_AddSprites(sector_t* sec, int lightlevel, int fakeside);
extern fixed_t FocalLengthX;
extern fixed_t FocalLengthY;

namespace
{
constexpr int kMaxFrames = 29;
constexpr fixed_t VX_MINZ = 1 * FRACUNIT;
constexpr fixed_t VX_MAX_DIST = 2048 * FRACUNIT;
constexpr fixed_t VX_NEAR_RADIUS = 512 * FRACUNIT;
constexpr fixed_t VX_Z_OFFSET = -3 * FRACUNIT;
constexpr angle_t VX_MAX_VIEWER_YAW = ANG45;
constexpr fixed_t VX_MAX_PITCH_SLOPE = FRACUNIT / 3;
constexpr double VX_MIN_PITCH_DZ = double(FRACUNIT) / 8.0;
constexpr double VX_NEAR_VERTICAL_HORIZ = double(FRACUNIT) / 4.0;

struct VoxelModel
{
	int x_size = 0;
	int y_size = 0;
	int z_size = 0;
	fixed_t x_pivot = 0;
	fixed_t y_pivot = 0;
	fixed_t z_pivot = 0;
	bool fromLocalDirectory = false;
	std::vector<int> offsets;
	std::vector<byte> data;
};

struct VoxelRenderOptions
{
	std::string voxelName;
	angle_t angleOffset = 0;
	int placedSpin = 0;
	int droppedSpin = 0;
	bool hasPlacedSpin = false;
	bool hasDroppedSpin = false;
	fixed_t viewerPitchSlopeLimit = 0;
	bool useActorPitch = false;
	bool fromVoxelDef = false;
};

enum VoxelFace
{
	F_LEFT = 0x01,
	F_RIGHT = 0x02,
	F_BACK = 0x04,
	F_FRONT = 0x08,
	F_TOP = 0x10,
	F_BOTTOM = 0x20,
};

std::unordered_map<uint64_t, VoxelModel> g_voxels;
std::unordered_map<uint64_t, VoxelRenderOptions> g_voxelOptions;
std::deque<r_voxelvis_s> g_visibleVoxels;
fixed_t g_eye_x = 0;
fixed_t g_eye_y = 0;

uint64_t FrameKey(const int32_t spritenum, const int frame)
{
	return (uint64_t(uint32_t(spritenum)) << 32) | uint64_t(uint8_t(frame));
}

angle_t VX_DegreesToAngle(double degrees)
{
	const double unit = std::fmod(degrees, 360.0);
	const double wrapped = unit < 0.0 ? (unit + 360.0) : unit;
	return angle_t((uint64_t(wrapped * 4294967296.0 / 360.0)) & 0xFFFFFFFFu);
}

fixed_t VX_AngleToSlope(int angle)
{
	if (angle > int(ANG90))
		return finetangent[0];
	if (-angle > int(ANG90))
		return finetangent[FINEANGLES / 2 - 1];
	return finetangent[(ANG90 - angle_t(angle)) >> ANGLETOFINESHIFT];
}

fixed_t VX_MomentumToSlope(const AActor* thing)
{
	if (!thing)
		return 0;

	const double mx = static_cast<double>(thing->momx);
	const double my = static_cast<double>(thing->momy);
	const double mz = static_cast<double>(thing->momz);
	if (std::abs(mz) < VX_MIN_PITCH_DZ)
		return 0;

	const double horiz = std::sqrt(mx * mx + my * my);
	if (horiz < VX_NEAR_VERTICAL_HORIZ)
		return mz > 0.0 ? VX_MAX_PITCH_SLOPE : -VX_MAX_PITCH_SLOPE;

	const double slope = mz / std::max(horiz, 1.0);
	const double fixedSlope = slope * static_cast<double>(FRACUNIT);
	return fixed_t(std::clamp(fixedSlope, -static_cast<double>(VX_MAX_PITCH_SLOPE),
	                     static_cast<double>(VX_MAX_PITCH_SLOPE)));
}

fixed_t VX_ActorPitchSlope(const AActor* thing)
{
	if (!thing)
		return 0;
	if (thing->pitch != 0)
		return VX_AngleToSlope(int(thing->pitch));

	// Inferring pitch from vertical displacement makes floating monsters and
	// bobbing pickups snap and shears their voxel rows apart. Only missiles need
	// trajectory-derived pitch; other actors remain rigid and let position
	// interpolation provide smooth vertical motion.
	return thing->flags & MF_MISSILE ? VX_MomentumToSlope(thing) : 0;
}

enum class VoxelRotationMode
{
	ActorAngle,
	FaceView,
	Spin,
};

struct VoxelRotation
{
	VoxelRotationMode mode = VoxelRotationMode::ActorAngle;
	int spin = 0;
};

bool VX_IsSphericalPowerup(const AActor* thing)
{
	switch (thing->sprite)
	{
	case SPR_PINS:
	case SPR_PINV:
	case SPR_SOUL:
	case SPR_MEGA:
		return true;
	default:
		return false;
	}
}

VoxelRotation VX_RotationForThing(const AActor* thing, const VoxelRenderOptions* opts)
{
	const bool dropped = bool(thing->flags & MF_DROPPED);
	if (opts)
	{
		const bool hasSpin = dropped ? opts->hasDroppedSpin : opts->hasPlacedSpin;
		const int spin = dropped ? opts->droppedSpin : opts->placedSpin;
		if (hasSpin)
			return {spin == 0 ? VoxelRotationMode::ActorAngle : VoxelRotationMode::Spin, spin};
	}

	// These spherical pickups look distorted unless they point at the viewer's
	// actual position instead of merely matching the camera's view angle.
	if (VX_IsSphericalPowerup(thing))
		return {VoxelRotationMode::FaceView, 0};

	// Dropped items should retain sprite-like presentation. Check this before
	// the weapon list so dropped weapons do not spin.
	if (dropped)
		return {VoxelRotationMode::FaceView, 0};

	// Match Woof's rotating placed-weapon list.
	switch (thing->sprite)
	{
	case SPR_SHOT:
	case SPR_MGUN:
	case SPR_LAUN:
	case SPR_PLAS:
	case SPR_BFUG:
	case SPR_CSAW:
	case SPR_SGN2:
		return {VoxelRotationMode::Spin, 4};
	default:
		break;
	}

	if (thing->flags & MF_SPECIAL)
		return {VoxelRotationMode::FaceView, 0};

	return {VoxelRotationMode::ActorAngle, 0};
}

angle_t VX_ItemRotationAngle(const int degreesPerTic)
{
	// Interpolate the fraction of the current tic so spinning remains smooth at
	// uncapped frame rates. Negative values rotate in the opposite direction.
	const double time = static_cast<double>(level.time) +
	                    static_cast<double>(render_lerp_amount) / FRACUNIT;
	return VX_DegreesToAngle(time * degreesPerTic);
}

angle_t VX_ViewerFacingAngle(const fixed_t x, const fixed_t y)
{
	const angle_t viewFacing = viewangle + ANG180;
	const angle_t viewerFacing = R_PointToAngle(x, y) + ANG180;
	const int32_t delta = static_cast<int32_t>(viewerFacing - viewFacing);
	const int32_t limit = static_cast<int32_t>(VX_MAX_VIEWER_YAW);
	return viewFacing + angle_t(std::clamp(delta, -limit, limit));
}

fixed_t VX_ViewerPitchSlope(const fixed_t x, const fixed_t y, const fixed_t centerz,
                           const fixed_t limit)
{
	const double dx = static_cast<double>(viewx) - x;
	const double dy = static_cast<double>(viewy) - y;
	const double dz = static_cast<double>(viewz) - centerz;
	const double distance = std::sqrt(dx * dx + dy * dy);
	if (distance < 1.0)
		return dz >= 0.0 ? limit : -limit;

	const double slope = dz / distance * FRACUNIT;
	return fixed_t(std::clamp(slope, -static_cast<double>(limit), static_cast<double>(limit)));
}

int VX_FrameIndexForChar(char frameChar)
{
	const unsigned char raw = static_cast<unsigned char>(frameChar);
	const unsigned char up = static_cast<unsigned char>(std::toupper(raw));
	// VOXELDEF uses '^' as a parser-safe alias for the frame after '[' ('\\').
	if (up == '^')
		return 27;
	const int frame = int(up) - int('A');
	if (frame < 0 || frame >= kMaxFrames)
		return -1;
	return frame;
}

bool VX_ParseNumberToken(const std::string& token, double& out)
{
	char* end = nullptr;
	out = std::strtod(token.c_str(), &end);
	return end != nullptr && *end == '\0';
}

bool VX_ReadNumber(OScanner& os, double& out)
{
	os.mustScan();
	std::string token = os.getToken();
	if (token == "-")
	{
		os.mustScan();
		token = "-" + os.getToken();
	}

	if (!VX_ParseNumberToken(token, out))
	{
		os.warning("Expected numeric value, got '{}'.", token);
		return false;
	}

	return true;
}

void VX_ParseOptions(OScanner& os, VoxelRenderOptions& opts)
{
	while (os.scan())
	{
		if (os.compareToken("}"))
			return;

		const std::string token = os.getToken();
		const std::string option = StdStringToLower(token);
		if (option == "angleoffset")
		{
			os.mustScan();
			if (!os.compareToken("="))
			{
				os.warning("Expected '=' after AngleOffset.");
				continue;
			}
			double degrees = 0.0;
			if (VX_ReadNumber(os, degrees))
				opts.angleOffset = VX_DegreesToAngle(degrees);
			continue;
		}
		if (option == "spin" || option == "placedspin" || option == "droppedspin")
		{
			os.mustScan();
			if (!os.compareToken("="))
			{
				os.warning("Expected '=' after {}.", token);
				continue;
			}

			double value = 0.0;
			if (!VX_ReadNumber(os, value))
				continue;
			const int speed = static_cast<int>(value);
			if (option == "spin" || option == "placedspin")
			{
				opts.placedSpin = speed;
				opts.hasPlacedSpin = true;
			}
			if (option == "spin" || option == "droppedspin")
			{
				opts.droppedSpin = speed;
				opts.hasDroppedSpin = true;
			}
			continue;
		}
		if (option == "faceviewerpitch")
		{
			os.mustScan();
			if (!os.compareToken("="))
			{
				os.warning("Expected '=' after FaceViewerPitch.");
				continue;
			}

			double degrees = 0.0;
			if (VX_ReadNumber(os, degrees))
			{
				degrees = std::clamp(std::abs(degrees), 0.0, 89.0);
				opts.viewerPitchSlopeLimit =
				    fixed_t(std::tan(degrees * 3.14159265358979323846 / 180.0) * FRACUNIT);
			}
			continue;
		}
		if (option == "useactorpitch")
		{
			opts.useActorPitch = true;
			continue;
		}
		if (option == "useactorroll")
		{
			// Parsed for compatibility but intentionally ignored for now.
			continue;
		}

		os.warning("Unknown VOXELDEF option '{}'.", token);
		if (os.scan())
		{
			if (!os.compareToken("}") && os.compareToken("="))
				os.mustScan();
			else
				os.unScan();
		}
	}

	os.warning("Unterminated VOXELDEF option block.");
}

void VX_ParseVoxelDefLump(const int lump)
{
	const char* data = static_cast<const char*>(W_CacheLumpNum(lump, PU_CACHE));
	const int len = W_LumpLength(lump);
	if (!data || len <= 0)
		return;

	const OScannerConfig config = {
	    "VOXELDEF", // lumpName
	    false,      // semiComments
	    true,       // cComments
	    false,      // hashComments
	};
	OScanner os = OScanner::openBuffer(config, data, data + len);

	std::unordered_map<std::string, int32_t> spriteByName;
	spriteByName.reserve(sprnames.size());
	for (const auto& [spritenum, spriteName] : sprnames)
		spriteByName[StdStringToUpper(spriteName)] = spritenum;

	while (os.scan())
	{
		std::vector<std::pair<int32_t, int>> targets;
		for (;;)
		{
			std::string token = os.getToken();
			if (token == "=")
				break;

			// OScanner separates bracket punctuation from identifiers. Recombine the
			// final three Doom sprite-frame characters before treating a bare
			// four-character sprite name as an all-frames mapping.
			if (token.size() == 4 && os.scan())
			{
				const std::string suffix = os.getToken();
				if (suffix == "[" || suffix == "^" || suffix == "]")
					token += suffix;
				else
					os.unScan();
			}

			const std::string spriteRef = StdStringToUpper(token);
			if (spriteRef.size() != 4 && spriteRef.size() != 5)
			{
				os.warning("Invalid sprite token '{}' in VOXELDEF entry.", token);
			}
			else
			{
				const std::string spriteName = spriteRef.substr(0, 4);
				const auto sit = spriteByName.find(spriteName);
				if (sit == spriteByName.end())
				{
					os.warning("Unknown sprite '{}' in VOXELDEF.", spriteName);
				}
				else if (spriteRef.size() == 4)
				{
					for (int frame = 0; frame < kMaxFrames; frame++)
						targets.push_back({sit->second, frame});
				}
				else
				{
					const int frame = VX_FrameIndexForChar(spriteRef[4]);
					if (frame < 0)
						os.warning("Invalid sprite frame '{}' in VOXELDEF token '{}'.", spriteRef[4],
						           token);
					else
						targets.push_back({sit->second, frame});
				}
			}

			if (!os.scan())
				return;
		}

		os.mustScan();
		if (!os.isQuotedString())
		{
			os.warning("Expected quoted voxel name after '='.");
			continue;
		}

		VoxelRenderOptions opts;
		opts.voxelName = os.getToken();
		opts.fromVoxelDef = true;

		if (os.scan())
		{
			if (os.compareToken("{"))
				VX_ParseOptions(os, opts);
			else
				os.unScan();
		}

		for (const auto& [spritenum, frame] : targets)
			g_voxelOptions[FrameKey(spritenum, frame)] = opts;
	}
}

void VX_ParseVoxelDefs()
{
	int lump = -1;
	while ((lump = W_FindLump("VOXELDEF", lump)) != -1)
		VX_ParseVoxelDefLump(lump);
}

int VX_PaletteIndex(const byte* pal, int r, int g, int b)
{
	int best = 0;
	int best_dist = (1 << 30);

	for (int i = 0; i < 256; i++)
	{
		const int dr = r - int(*pal++);
		const int dg = g - int(*pal++);
		const int db = b - int(*pal++);
		const int dist = dr * dr + dg * dg + db * db;

		if (dist < best_dist)
		{
			best = i;
			best_dist = dist;
		}
	}

	return best;
}

void VX_CreateRemapTable(const byte* src, std::array<byte, 256>& table)
{
	const byte* pal = W_CacheLumpName<byte>("PLAYPAL", PU_CACHE);

	for (int c = 0; c < 256; c++)
	{
		const int r = int(*src++) << 2;
		const int g = int(*src++) << 2;
		const int b = int(*src++) << 2;
		table[c] = uint8_t(VX_PaletteIndex(pal, r, g, b));
	}
}

void VX_RemapSlabColors(VoxelModel& v, int x, int y, const std::array<byte, 256>& table)
{
	const int A = v.offsets[y * v.x_size + x];
	const int B = v.offsets[(y + 1) * v.x_size + x];
	if (!(A < B))
		return;

	byte* slab = &v.data[A];
	const byte* end = &v.data[B];

	while (slab < end)
	{
		const byte top = *slab++;
		const byte len = *slab++;
		const byte face = *slab++;
		(void)top;
		(void)face;

		for (byte i = 0; i < len; i++, slab++)
			*slab = table[*slab];
	}
}

bool VX_Decode(const byte* bytes, size_t length, VoxelModel& out)
{
	if (length < 40 + 768)
		return false;

	const byte* p = bytes;
	p += 4; // num_bytes

	out.x_size = int(p[0]);
	p += 4;
	out.y_size = int(p[0]);
	p += 4;
	out.z_size = int(p[0]);
	p += 4;
	if (out.x_size <= 0 || out.y_size <= 0 || out.z_size <= 0)
		return false;
	if (out.x_size > 255 || out.y_size > 255)
		return false;

	out.x_pivot = (p[0] << 8) | (p[1] << 16);
	p += 4;
	out.y_pivot = (p[0] << 8) | (p[1] << 16);
	p += 4;
	out.z_pivot = (p[0] << 8) | (p[1] << 16);
	p += 4;

	std::array<int, 260> xoffsets{};
	for (int x = 0; x <= out.x_size; x++)
	{
		xoffsets[x] = int(p[0]) | (p[1] << 8) | (p[2] << 16);
		p += 4;
	}

	const int num_offsets = out.x_size * (out.y_size + 1);
	out.offsets.resize(num_offsets);

	int min_offset = (1 << 30);
	int max_offset = 0;
	for (int x = 0; x < out.x_size; x++)
	{
		for (int y = 0; y <= out.y_size; y++)
		{
			int offset = int(p[0]) | (p[1] << 8);
			p += 2;
			offset += xoffsets[x];
			out.offsets[y * out.x_size + x] = offset;
			min_offset = std::min(min_offset, offset);
			max_offset = std::max(max_offset, offset);
		}
	}

	const int data_size = max_offset - min_offset;
	if (data_size <= 0)
		return false;

	for (int& offset : out.offsets)
		offset -= min_offset;

	const size_t data_start = (7 * 4) + size_t(min_offset);
	if (data_start + size_t(data_size) > length)
		return false;

	out.data.resize(data_size);
	memcpy(out.data.data(), bytes + data_start, data_size);

	std::array<byte, 256> remap_table{};
	VX_CreateRemapTable(bytes + (length - 768), remap_table);

	for (int x = 0; x < out.x_size; x++)
	{
		for (int y = 0; y < out.y_size; y++)
			VX_RemapSlabColors(out, x, y, remap_table);
	}

	return true;
}

std::string VX_NamePath(const std::string& voxelName)
{
	std::string base = r_voxeldir.str();
	if (base.empty())
		base = "voxels";

	return fmt::format("{}/{}.kvx", base, StdStringToLower(voxelName));
}

bool VX_LoadByName(const int32_t spritenum, const int frame, const std::string& voxelName,
                   const VoxelRenderOptions& opts)
{
	std::string resolvedVoxelName = voxelName;
	// VOXELDEF uses '^' as the printable alias for Doom's post-'[' '\\'
	// frame. Apply the same alias to the referenced KVX name, whose lump uses
	// the literal backslash character.
	if (resolvedVoxelName.size() == 5 && resolvedVoxelName.back() == '^')
		resolvedVoxelName.back() = '\\';
	const std::string lumpName = StdStringToUpper(resolvedVoxelName);

	int start = -1;
	while ((start = W_FindLump("VX_START", start)) != -1)
	{
		int end = -1;
		for (int i = start + 1; i < static_cast<int>(W_NumLumps()); i++)
		{
			if (W_CheckLumpName(i, "VX_END"))
			{
				end = i;
				break;
			}
		}
		if (end == -1)
			break;

		for (int i = start + 1; i < end; i++)
		{
			if (!W_CheckLumpName(i, lumpName.c_str()))
				continue;

			const unsigned len = W_LumpLength(i);
			if (len == 0)
				return false;

			std::vector<byte> bytes(len);
			W_ReadLump(i, bytes.data());

			VoxelModel model;
			if (!VX_Decode(bytes.data(), bytes.size(), model))
			{
				PrintFmt(PRINT_WARNING, "VX_Load: failed to decode lump {}\n", lumpName);
				return false;
			}

			g_voxels[FrameKey(spritenum, frame)] = std::move(model);
			g_voxelOptions[FrameKey(spritenum, frame)] = opts;
			return true;
		}

		start = end;
	}

	// Server-provided WAD resources take precedence above and remain usable even
	// when local voxel replacements are disabled.  This setting only controls
	// loading raw .kvx files from the client's r_voxeldir.
	if (!sv_allowvoxels)
		return false;

	const std::string filename = VX_NamePath(resolvedVoxelName);
	if (!M_FileExists(filename))
		return false;

	byte* buffer = nullptr;
	const size_t len = M_ReadFile(filename, &buffer);
	if (!buffer || len == 0)
		return false;

	VoxelModel model;
	const bool ok = VX_Decode(buffer, len, model);
	Z_Free(buffer);

	if (!ok)
	{
		PrintFmt(PRINT_WARNING, "VX_Load: failed to decode {}\n", filename);
		return false;
	}

	model.fromLocalDirectory = true;
	g_voxels[FrameKey(spritenum, frame)] = std::move(model);
	g_voxelOptions[FrameKey(spritenum, frame)] = opts;
	return true;
}

bool VX_Load(const int32_t spritenum, const std::string& spriteName, const int frame)
{
	const uint64_t key = FrameKey(spritenum, frame);
	const auto dit = g_voxelOptions.find(key);
	if (dit != g_voxelOptions.end() && dit->second.fromVoxelDef)
	{
		if (VX_LoadByName(spritenum, frame, dit->second.voxelName, dit->second))
			return true;

		PrintFmt(PRINT_WARNING,
		         "VX_Load: VOXELDEF entry '{}' for {}{} was not found, falling back.\n",
		         dit->second.voxelName, spriteName, char('A' + frame));
	}

	VoxelRenderOptions fallback;
	fallback.voxelName = fmt::format("{}{}", spriteName, char('A' + frame));
	fallback.fromVoxelDef = false;
	return VX_LoadByName(spritenum, frame, fallback.voxelName, fallback);
}

const VoxelModel* VX_GetModel(const int32_t spritenum, const int frame)
{
	const auto it = g_voxels.find(FrameKey(spritenum, frame));
	return it == g_voxels.end() ? nullptr : &it->second;
}

const VoxelRenderOptions* VX_GetOptions(const int32_t spritenum, const int frame)
{
	const auto it = g_voxelOptions.find(FrameKey(spritenum, frame));
	return it == g_voxelOptions.end() ? nullptr : &it->second;
}

void VX_DrawSolidShadedColumn(vissprite_t* spr, const int screenX, const int yl, const int yh,
                              const byte color)
{
	if (yl > yh)
		return;

	byte src = color;
	dcol.x = screenX;
	dcol.yl = yl;
	dcol.yh = yh;
	dcol.iscale = 0;
	dcol.texturefrac = 0;
	dcol.colormap = spr->colormap;
	dcol.source = &src;

	if (spr->translation)
	{
		dcol.translation = spr->translation;
		R_DrawTranslatedColumn();
	}
	else
	{
		R_DrawColumn();
	}
}

void VX_DrawColumn(vissprite_t* spr, int x, int y)
{
	r_voxelvis_s* vv = spr->voxel;
	const VoxelModel* v = static_cast<const VoxelModel*>(vv->model);

	const int ofs1 = v->offsets[y * v->x_size + x];
	const int ofs2 = v->offsets[(y + 1) * v->x_size + x];
	if (!(ofs1 < ofs2))
		return;

	const int qu_x = g_eye_x < (x << FRACBITS) ? 0 : g_eye_x < ((x + 1) << FRACBITS) ? 1 : 2;
	const int qu_y = g_eye_y < (y << FRACBITS) ? 0 : g_eye_y < ((y + 1) << FRACBITS) ? 1 : 2;
	const int quadrant = qu_y * 3 + qu_x;
	if (quadrant == 4)
		return;

	const fixed_t c = vv->c;
	const fixed_t s = vv->s;

	fixed_t tx[4], ty[4];
	tx[0] = vv->TL_x + x * c + y * s;
	ty[0] = vv->TL_y + x * s - y * c;
	tx[1] = tx[0] + s;
	ty[1] = ty[0] - c;
	tx[2] = tx[1] + c;
	ty[2] = ty[1] + s;
	tx[3] = tx[0] + c;
	ty[3] = ty[0] + s;

	static const int A_corners[9] = {3, 3, 2, 0, -1, 2, 0, 1, 1};
	int idx = A_corners[quadrant];

	const fixed_t Ax0 = tx[idx];
	const fixed_t Ay = ty[idx];
	idx = (idx + 1) & 3;
	const fixed_t Bx0 = tx[idx];
	const fixed_t By = ty[idx];
	idx = (idx + 1) & 3;
	const fixed_t Cx0 = tx[idx];
	const fixed_t Cy = ty[idx];
	idx = (idx + 1) & 3;
	const fixed_t Dx0 = tx[idx];
	const fixed_t Dy = ty[idx];

	if (By < VX_MINZ || Ay < VX_MINZ || Cy < VX_MINZ || Dy < VX_MINZ)
		return;

	const fixed_t A_xscale = FixedDiv(FocalLengthX, Ay);
	const fixed_t B_xscale = FixedDiv(FocalLengthX, By);
	const fixed_t C_xscale = FixedDiv(FocalLengthX, Cy);
	const fixed_t D_xscale = FixedDiv(FocalLengthX, Dy);
	const fixed_t A_yscale = FixedDiv(FocalLengthY, Ay);
	const fixed_t B_yscale = FixedDiv(FocalLengthY, By);
	const fixed_t C_yscale = FixedDiv(FocalLengthY, Cy);
	const fixed_t D_yscale = FixedDiv(FocalLengthY, Dy);

	const fixed_t Ax = centerxfrac + FixedMul(Ax0, A_xscale);
	const fixed_t Bx = centerxfrac + FixedMul(Bx0, B_xscale);
	const fixed_t Cx = centerxfrac + FixedMul(Cx0, C_xscale);
	const fixed_t Dx = centerxfrac + FixedMul(Dx0, D_xscale);

	static const byte A_faces[9] = {F_BACK, F_BACK, F_RIGHT, F_LEFT, 0, F_RIGHT, F_LEFT, F_FRONT, F_FRONT};
	static const byte B_faces[9] = {F_LEFT, 0, F_BACK, 0, 0, 0, F_FRONT, 0, F_RIGHT};
	const byte A_face = A_faces[quadrant];
	const byte B_face = B_faces[quadrant];

	const bool shadow =
	    bool(spr->mobjflags & MF_SHADOW) || bool(spr->statusflags & SF_INVIS);
	const fixed_t local_y = (y << FRACBITS) - v->y_pivot;
	const fixed_t columnTilt = FixedMul(local_y, vv->pitchSlope);

	for (fixed_t ux = ((Ax - 1) | (FRACUNIT - 1)) + 1; ux < std::max(Bx, Cx); ux += FRACUNIT)
	{
		if (ux >= ((spr->x2 + 1) << FRACBITS))
			break;
		if (ux < (spr->x1 << FRACBITS))
			continue;

		const int screenX = ux >> FRACBITS;
		const fixed_t clip_y1 = (mceilingclip[screenX] + 1) << FRACBITS;
		const fixed_t clip_y2 = (mfloorclip[screenX] << FRACBITS) - 1;
		if (clip_y2 <= clip_y1)
			continue;

		fixed_t scale = 0;
		if (ux > Bx)
			scale = B_yscale + FixedMul(C_yscale - B_yscale, FixedDiv(ux - Bx, Cx - Bx));
		else
			scale = A_yscale + FixedMul(B_yscale - A_yscale, FixedDiv(ux - Ax, Bx - Ax));
		if (scale <= 0)
			continue;

		const fixed_t iscale = FixedDiv(FRACUNIT, scale);

		const byte* slab = &v->data[ofs1];
		const byte* end = &v->data[ofs2];

		for (; slab < end;)
		{
			const byte top = *slab++;
			const byte len = *slab++;
			const byte face = *slab++;
			if (len == 0)
				continue;

			const fixed_t top_z = spr->gzt - viewz + columnTilt - (top << FRACBITS);
			fixed_t uy1 = centeryfrac - FixedMul(top_z, scale);
			fixed_t uy2 = uy1 + fixed_t(len) * scale;
			const fixed_t uy0 = uy1;

			if (uy1 >= clip_y2)
				uy1 = clip_y2;
			if (uy2 <= clip_y1)
				uy2 = clip_y1;

			if (uy1 < clip_y1)
				uy1 = clip_y1;
			if (uy2 > clip_y2)
				uy2 = clip_y2;

			// Some viewing angles have no second visible side.  In those cases,
			// rounding can put the center screen column just past Bx; keep using
			// the primary face instead of dropping that column and leaving a seam.
			const byte visible_face = ux > Bx && B_face != 0 ? B_face : A_face;
			const bool has_side = (face & visible_face) != 0 && uy1 < clip_y2 &&
			                      uy2 > clip_y1;
			if (shadow)
			{
				if (has_side)
				{
					dcol.x = screenX;
					dcol.yl = uy1 >> FRACBITS;
					dcol.yh = uy2 >> FRACBITS;
					if (dcol.yl <= dcol.yh)
						R_DrawFuzzColumn();
				}
				slab += len;
				continue;
			}

			const bool has_top = (face & F_TOP) && top_z < 0;
			const bool has_bottom = (face & F_BOTTOM) && top_z > (int(len) << FRACBITS);

			fixed_t wscale = 0;
			if (has_top || has_bottom)
			{
				if (ux > Cx)
					wscale = C_yscale + FixedMul(B_yscale - C_yscale, FixedDiv(ux - Cx, Bx - Cx));
				else if (ux > Dx)
					wscale = D_yscale + FixedMul(C_yscale - D_yscale, FixedDiv(ux - Dx, Cx - Dx));
				else
					wscale = A_yscale + FixedMul(D_yscale - A_yscale, FixedDiv(ux - Ax, Dx - Ax));
			}

			if (has_top)
			{
				fixed_t uy = centeryfrac - FixedMul(top_z, wscale);
				uy = ((uy - 1) | (FRACUNIT - 1)) + 1;
				if (uy < clip_y1)
					uy = clip_y1;

				VX_DrawSolidShadedColumn(spr, screenX, uy >> FRACBITS, (uy1 - 1) >> FRACBITS,
				                         slab[0]);
			}
			else if (has_bottom)
			{
				fixed_t uy = centeryfrac - FixedMul(top_z - (int(len) << FRACBITS), wscale);
				if (uy > clip_y2)
					uy = clip_y2;

				VX_DrawSolidShadedColumn(spr, screenX, (uy2 + 1) >> FRACBITS, uy >> FRACBITS,
				                         slab[len - 1]);
			}

			if (has_side)
			{
				dcol.x = screenX;
				dcol.yl = uy1 >> FRACBITS;
				dcol.yh = uy2 >> FRACBITS;

				dcol.iscale = iscale;
				dcol.texturefrac =
				    FixedMul((((dcol.yl << FRACBITS) - uy0) >> FRACBITS) << FRACBITS, iscale);

				int local_yl = dcol.yl;
				int local_yh = dcol.yh;
				if (dcol.texturefrac < 0)
				{
					const int cnt =
					    (FixedDiv(-dcol.texturefrac, dcol.iscale) + FRACUNIT - 1) >> FRACBITS;
					local_yl += cnt;
					dcol.texturefrac += cnt * dcol.iscale;
				}

				const fixed_t endfrac =
				    dcol.texturefrac + (local_yh - local_yl) * dcol.iscale;
				const fixed_t maxfrac = fixed_t(len) << FRACBITS;
				if (endfrac >= maxfrac)
				{
					const int cnt =
					    (FixedDiv(endfrac - maxfrac - 1, dcol.iscale) + FRACUNIT - 1) >> FRACBITS;
					local_yh -= cnt;
				}

				dcol.yl = local_yl;
				dcol.yh = local_yh;

				if (dcol.yl <= dcol.yh)
				{
					if (spr->translation)
					{
						dcol.translation = spr->translation;
						dcol.colormap = spr->colormap;
						dcol.source = const_cast<byte*>(slab);
						R_DrawTranslatedColumn();
					}
					else
					{
						dcol.colormap = spr->colormap;
						dcol.source = const_cast<byte*>(slab);
						R_DrawColumn();
					}
				}
			}

			slab += len;
		}
	}
}

void VX_RecursiveDraw(vissprite_t* spr, int x, int y, int w, int h)
{
loop:
	if (w == 1 && h == 1)
	{
		VX_DrawColumn(spr, x, y);
		return;
	}

	if (w >= h)
	{
		if (g_eye_x < ((x * 2 + w) << (FRACBITS - 1)))
		{
			VX_RecursiveDraw(spr, x + w / 2, y, (w + 1) / 2, h);
			w = w / 2;
		}
		else
		{
			VX_RecursiveDraw(spr, x, y, w / 2, h);
			x += w / 2;
			w = (w + 1) / 2;
		}
	}
	else
	{
		if (g_eye_y < ((y * 2 + h) << (FRACBITS - 1)))
		{
			VX_RecursiveDraw(spr, x, y + h / 2, w, (h + 1) / 2);
			h = h / 2;
		}
		else
		{
			VX_RecursiveDraw(spr, x, y, w, h / 2);
			y += h / 2;
			h = (h + 1) / 2;
		}
	}

	goto loop;
}

bool VX_CheckBBox(fixed_t* bspcoord)
{
	if (bspcoord[BOXRIGHT] <= viewx - VX_NEAR_RADIUS)
		return false;
	if (bspcoord[BOXLEFT] >= viewx + VX_NEAR_RADIUS)
		return false;
	if (bspcoord[BOXTOP] <= viewy - VX_NEAR_RADIUS)
		return false;
	if (bspcoord[BOXBOTTOM] >= viewy + VX_NEAR_RADIUS)
		return false;
	return true;
}

void VX_SpritesInNode(int bspnum)
{
	for (;;)
	{
		if (bspnum & NF_SUBSECTOR)
		{
			subsector_t* sub = &subsectors[bspnum & ~NF_SUBSECTOR];
			R_AddSprites(sub->sector, sub->sector->lightlevel, FAKED_Center);
			return;
		}

		node_t* bsp = &nodes[bspnum];
		if (VX_CheckBBox(bsp->bbox[0]))
			VX_SpritesInNode(bsp->children[0]);

		if (VX_CheckBBox(bsp->bbox[1]))
			bspnum = bsp->children[1];
		else
			break;
	}
}
} // namespace

void VX_Init()
{
#ifndef ODAMEX_EXPERIMENTAL_VOXELS
	return;
#else
	g_voxels.clear();
	g_voxelOptions.clear();
	g_visibleVoxels.clear();

	VX_ParseVoxelDefs();

	for (const auto& [spritenum, spriteName] : sprnames)
	{
		for (int frame = 0; frame < kMaxFrames; frame++)
		{
			if (!VX_Load(spritenum, spriteName, frame))
				break;
		}
	}

	PrintFmt(PRINT_HIGH, "VX_Init: loaded {} voxel sprite frames ({} VOXELDEF mappings).\n",
	         g_voxels.size(), g_voxelOptions.size());
#endif
}

void VX_ClearVoxels()
{
#ifdef ODAMEX_EXPERIMENTAL_VOXELS
	g_visibleVoxels.clear();
#endif
}

void VX_NearbySprites()
{
#ifdef ODAMEX_EXPERIMENTAL_VOXELS
	if (r_voxels && numnodes > 0)
		VX_SpritesInNode(numnodes - 1);
#endif
}

bool VX_ProjectVoxel(const AActor* thing, const int frame, vissprite_t* vis)
{
#ifndef ODAMEX_EXPERIMENTAL_VOXELS
	(void)thing;
	(void)frame;
	(void)vis;
	return false;
#else
	if (!r_voxels || !thing || !vis || !thing->subsector || !thing->subsector->sector)
		return false;

	const VoxelModel* v = VX_GetModel(thing->sprite, frame);
	if (!v || (!sv_allowvoxels && v->fromLocalDirectory))
		return false;
	const VoxelRenderOptions* opts = VX_GetOptions(thing->sprite, frame);

	const fixed_t gx = vis->gx;
	const fixed_t gy = vis->gy;
	const fixed_t gz = vis->gzb;

	const fixed_t tran_x = gx - viewx;
	const fixed_t tran_y = gy - viewy;
	if (abs(tran_x) > VX_MAX_DIST || abs(tran_y) > VX_MAX_DIST)
		return false;

	fixed_t tx, ty;
	R_RotatePoint(tran_x, tran_y, ANG90 - viewangle, tx, ty);
	if (ty < VX_MINZ)
		return false;

	angle_t angle = thing->angle;
	const VoxelRotation rotation = VX_RotationForThing(thing, opts);
	switch (rotation.mode)
	{
	case VoxelRotationMode::FaceView:
		angle = VX_ViewerFacingAngle(gx, gy);
		break;
	case VoxelRotationMode::Spin:
		angle = VX_ItemRotationAngle(rotation.spin);
		break;
	case VoxelRotationMode::ActorAngle:
		break;
	}
	if (opts)
		angle += opts->angleOffset;

	const angle_t ang2 = ANG180 - viewangle + angle;
	const fixed_t c = finecosine[ang2 >> ANGLETOFINESHIFT];
	const fixed_t s = finesine[ang2 >> ANGLETOFINESHIFT];
	fixed_t pitchSlope = 0;
	if (opts && opts->viewerPitchSlopeLimit > 0)
	{
		const fixed_t centerz =
		    gz + v->z_pivot - (fixed_t(v->z_size) << (FRACBITS - 1)) + VX_Z_OFFSET;
		pitchSlope = VX_ViewerPitchSlope(gx, gy, centerz, opts->viewerPitchSlopeLimit);
	}
	else if (opts && opts->useActorPitch)
		pitchSlope = VX_ActorPitchSlope(thing);

	const fixed_t TL_x = tx - FixedMul(v->x_pivot, c) - FixedMul(v->y_pivot, s);
	const fixed_t TL_y = ty - FixedMul(v->x_pivot, s) + FixedMul(v->y_pivot, c);

	const fixed_t xs = v->x_size;
	const fixed_t ys = v->y_size;
	const fixed_t BL_x = TL_x + ys * s;
	const fixed_t BL_y = TL_y - ys * c;
	const fixed_t TR_x = TL_x + xs * c;
	const fixed_t TR_y = TL_y + xs * s;
	const fixed_t BR_x = BL_x + xs * c;
	const fixed_t BR_y = BL_y + xs * s;

	int x1 = viewwidth - 1;
	int x2 = 0;
	for (int i = 0; i < 4; i++)
	{
		const fixed_t cx = (i == 0) ? BL_x : (i == 1) ? BR_x : (i == 2) ? TL_x : TR_x;
		const fixed_t cy = (i == 0) ? BL_y : (i == 1) ? BR_y : (i == 2) ? TL_y : TR_y;
		if (cy < VX_MINZ)
		{
			x1 = 0;
			x2 = viewwidth - 1;
			break;
		}

		const int sx = R_ProjectPointX(cx, cy);
		x1 = std::min(x1, sx);
		x2 = std::max(x2, sx);
	}

	x1 = std::clamp(x1, 0, viewwidth - 1);
	x2 = std::clamp(x2, 0, viewwidth - 1);
	if (x1 > x2)
		return false;

	g_visibleVoxels.push_back({v, angle, TL_x, TL_y, c, s, pitchSlope});
	vis->voxel = &g_visibleVoxels.back();
	vis->x1 = x1;
	vis->x2 = x2;
	vis->gx = gx;
	vis->gy = gy;
	vis->gzb = gz + VX_Z_OFFSET;
	vis->gzt = gz + v->z_pivot + VX_Z_OFFSET;

	return true;
#endif
}

void VX_DrawVoxel(vissprite_t* spr)
{
#ifndef ODAMEX_EXPERIMENTAL_VOXELS
	(void)spr;
	return;
#else
	if (!spr || !spr->voxel)
		return;

	const VoxelModel* v = static_cast<const VoxelModel*>(spr->voxel->model);
	if (!v)
		return;

	while (spr->x1 <= spr->x2 && mfloorclip[spr->x1] - mceilingclip[spr->x1] < 2)
		spr->x1++;
	while (spr->x2 >= spr->x1 && mfloorclip[spr->x2] - mceilingclip[spr->x2] < 2)
		spr->x2--;
	if (spr->x1 > spr->x2)
		return;

	const unsigned int ang = (spr->voxel->angle + ANG90) >> ANGLETOFINESHIFT;
	const fixed_t c = finecosine[ang];
	const fixed_t s = finesine[ang];

	const fixed_t delta_x = viewx - spr->gx;
	const fixed_t delta_y = viewy - spr->gy;

	g_eye_x = v->x_pivot + FixedMul(delta_x, c) + FixedMul(delta_y, s);
	g_eye_y = v->y_pivot + FixedMul(delta_x, s) - FixedMul(delta_y, c);

	VX_RecursiveDraw(spr, 0, 0, v->x_size, v->y_size);
#endif
}
