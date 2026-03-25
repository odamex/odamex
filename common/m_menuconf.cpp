// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2006-2026 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "m_menuconf.h"

#include "gi.h"

namespace
{
	menuconfdatabase_t gMenuConf;

	jsonlumpresult_t M_ParseMenuConfStringMap(const Json::Value& elem,
	                                          std::unordered_map<std::string, std::string>& output)
	{
		if (!(elem.isObject() || elem.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		output.clear();
		if (elem.isNull())
		{
			return jsonlumpresult_t::SUCCESS;
		}

		for (const auto& key : elem.getMemberNames())
		{
			const Json::Value& value = elem[key];
			if (!value.isString())
			{
				return jsonlumpresult_t::PARSEERROR;
			}

			output[key] = value.asString();
		}

		return jsonlumpresult_t::SUCCESS;
	}

	jsonlumpresult_t M_ParseMenuConfStringArray(const Json::Value& elem,
	                                            std::vector<std::string>& output)
	{
		if (!(elem.isArray() || elem.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		output.clear();
		if (elem.isNull())
		{
			return jsonlumpresult_t::SUCCESS;
		}

		for (const Json::Value& value : elem)
		{
			if (!value.isString())
			{
				return jsonlumpresult_t::PARSEERROR;
			}

			output.push_back(value.asString());
		}

		return jsonlumpresult_t::SUCCESS;
	}

	jsonlumpresult_t M_ParseMenuConfHeaderSide(const Json::Value& elem, menuconfheadertside_t& output)
	{
		if (!(elem.isObject() || elem.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		output = menuconfheadertside_t();
		if (elem.isNull())
		{
			return jsonlumpresult_t::SUCCESS;
		}

		const Json::Value& basePatch = elem["basePatch"];
		const Json::Value& frameCount = elem["frameCount"];
		const Json::Value& xpos = elem["x"];
		const Json::Value& ypos = elem["y"];
		const Json::Value& animateDirection = elem["animateDirection"];

		if (!(basePatch.isString() || basePatch.isNull())
			|| !(frameCount.isInt() || frameCount.isNull())
			|| !(xpos.isInt() || xpos.isNull())
			|| !(ypos.isInt() || ypos.isNull())
			|| !(animateDirection.isString() || animateDirection.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		if (!basePatch.isNull()) output.basePatch = basePatch.asString();
		if (!frameCount.isNull()) output.frameCount = frameCount.asInt();
		if (!xpos.isNull()) output.x = xpos.asInt();
		if (!ypos.isNull()) output.y = ypos.asInt();
		if (!animateDirection.isNull()) output.animateDirection = animateDirection.asString();

		return jsonlumpresult_t::SUCCESS;
	}

	jsonlumpresult_t M_ParseMenuConfHeaderDecorations(const Json::Value& elem,
	                                                  menuconfheaderdecorations_t& output)
	{
		if (!(elem.isObject() || elem.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		output = menuconfheaderdecorations_t();
		if (elem.isNull())
		{
			return jsonlumpresult_t::SUCCESS;
		}

		jsonlumpresult_t res = M_ParseMenuConfHeaderSide(elem["left"], output.left);
		if (res != jsonlumpresult_t::SUCCESS)
		{
			return res;
		}

		res = M_ParseMenuConfHeaderSide(elem["right"], output.right);
		if (res != jsonlumpresult_t::SUCCESS)
		{
			return res;
		}

		const Json::Value& frameTics = elem["frameTics"];
		if (!(frameTics.isInt() || frameTics.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		output.defined = true;
		if (!frameTics.isNull()) output.frameTics = frameTics.asInt();
		return jsonlumpresult_t::SUCCESS;
	}

	jsonlumpresult_t M_ParseMenuConfHeader(const Json::Value& elem, menuconfheader_t& output)
	{
		if (!(elem.isObject() || elem.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		output = menuconfheader_t();
		if (elem.isNull())
		{
			return jsonlumpresult_t::SUCCESS;
		}

		const Json::Value& patch = elem["patch"];
		const Json::Value& text = elem["text"];
		const Json::Value& languageKey = elem["languageKey"];
		const Json::Value& align = elem["align"];
		const Json::Value& xpos = elem["x"];
		const Json::Value& ypos = elem["y"];

		if (!(patch.isString() || patch.isNull())
			|| !(text.isString() || text.isNull())
			|| !(languageKey.isString() || languageKey.isNull())
			|| !(align.isString() || align.isNull())
			|| !(xpos.isInt() || xpos.isNull())
			|| !(ypos.isInt() || ypos.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		if (!patch.isNull()) output.patch = patch.asString();
		if (!text.isNull()) output.text = text.asString();
		if (!languageKey.isNull()) output.languageKey = languageKey.asString();
		if (!align.isNull()) output.align = align.asString();
		if (!xpos.isNull()) output.x = xpos.asInt();
		if (!ypos.isNull()) output.y = ypos.asInt();

		return M_ParseMenuConfHeaderDecorations(elem["decorations"], output.decorations);
	}

	jsonlumpresult_t M_ParseMenuConfLayout(const Json::Value& elem, menuconflayout_t& output)
	{
		if (!(elem.isObject() || elem.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		output = menuconflayout_t();
		if (elem.isNull())
		{
			return jsonlumpresult_t::SUCCESS;
		}

		const Json::Value& style = elem["style"];
		const Json::Value& xpos = elem["x"];
		const Json::Value& ypos = elem["y"];
		const Json::Value& indent = elem["indent"];
		const Json::Value& lineHeight = elem["lineHeight"];
		const Json::Value& scroll = elem["scroll"];
		const Json::Value& topPadding = elem["topPadding"];
		const Json::Value& itemSpacing = elem["itemSpacing"];

		if (!(style.isString() || style.isNull())
			|| !(xpos.isInt() || xpos.isNull())
			|| !(ypos.isInt() || ypos.isNull())
			|| !(indent.isInt() || indent.isNull())
			|| !(lineHeight.isString() || lineHeight.isInt() || lineHeight.isNull())
			|| !(scroll.isBool() || scroll.isNull())
			|| !(topPadding.isInt() || topPadding.isNull())
			|| !(itemSpacing.isString() || itemSpacing.isInt() || itemSpacing.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		if (!style.isNull()) output.style = style.asString();
		if (!xpos.isNull()) output.x = xpos.asInt();
		if (!ypos.isNull()) output.y = ypos.asInt();
		if (!indent.isNull()) output.indent = indent.asInt();
		if (!lineHeight.isNull()) output.lineHeight = lineHeight.isString() ? lineHeight.asString() : std::to_string(lineHeight.asInt());
		if (!scroll.isNull()) output.scroll = scroll.asBool();
		if (!topPadding.isNull()) output.topPadding = topPadding.asInt();
		if (!itemSpacing.isNull()) output.itemSpacing = itemSpacing.isString() ? itemSpacing.asString() : std::to_string(itemSpacing.asInt());

		return jsonlumpresult_t::SUCCESS;
	}

	jsonlumpresult_t M_ParseMenuConfIndicator(const Json::Value& elem, menuconfindicator_t& output)
	{
		if (!(elem.isObject() || elem.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		output = menuconfindicator_t();
		if (elem.isNull())
		{
			return jsonlumpresult_t::SUCCESS;
		}

		const Json::Value& offsetX = elem["offsetX"];
		const Json::Value& offsetY = elem["offsetY"];
		if (!(offsetX.isInt() || offsetX.isNull()) || !(offsetY.isInt() || offsetY.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		jsonlumpresult_t res = M_ParseMenuConfStringArray(elem["patches"], output.patches);
		if (res != jsonlumpresult_t::SUCCESS)
		{
			return res;
		}

		if (!offsetX.isNull()) output.offsetX = offsetX.asInt();
		if (!offsetY.isNull()) output.offsetY = offsetY.asInt();
		return jsonlumpresult_t::SUCCESS;
	}

	jsonlumpresult_t M_ParseMenuConfSlider(const Json::Value& elem, menuconfslider_t& output)
	{
		if (!(elem.isObject() || elem.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		output = menuconfslider_t();
		if (elem.isNull())
		{
			return jsonlumpresult_t::SUCCESS;
		}

		const Json::Value& leftPatch = elem["leftPatch"];
		const Json::Value& middlePatch = elem["middlePatch"];
		const Json::Value& rightPatch = elem["rightPatch"];
		const Json::Value& knobPatch = elem["knobPatch"];
		const Json::Value& greenKnobPatch = elem["greenKnobPatch"];
		const Json::Value& overlayPatch = elem["overlayPatch"];

		if (!(leftPatch.isString() || leftPatch.isNull())
			|| !(middlePatch.isString() || middlePatch.isNull())
			|| !(rightPatch.isString() || rightPatch.isNull())
			|| !(knobPatch.isString() || knobPatch.isNull())
			|| !(greenKnobPatch.isString() || greenKnobPatch.isNull())
			|| !(overlayPatch.isString() || overlayPatch.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		if (!leftPatch.isNull()) output.leftPatch = leftPatch.asString();
		if (!middlePatch.isNull()) output.middlePatch = middlePatch.asString();
		if (!rightPatch.isNull()) output.rightPatch = rightPatch.asString();
		if (!knobPatch.isNull()) output.knobPatch = knobPatch.asString();
		if (!greenKnobPatch.isNull()) output.greenKnobPatch = greenKnobPatch.asString();
		if (!overlayPatch.isNull()) output.overlayPatch = overlayPatch.asString();
		return jsonlumpresult_t::SUCCESS;
	}

	jsonlumpresult_t M_ParseMenuConfInputBox(const Json::Value& elem, menuconfinputbox_t& output)
	{
		if (!(elem.isObject() || elem.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		output = menuconfinputbox_t();
		if (elem.isNull())
		{
			return jsonlumpresult_t::SUCCESS;
		}

		const Json::Value& fullPatch = elem["fullPatch"];
		const Json::Value& leftPatch = elem["leftPatch"];
		const Json::Value& middlePatch = elem["middlePatch"];
		const Json::Value& rightPatch = elem["rightPatch"];

		if (!(fullPatch.isString() || fullPatch.isNull())
			|| !(leftPatch.isString() || leftPatch.isNull())
			|| !(middlePatch.isString() || middlePatch.isNull())
			|| !(rightPatch.isString() || rightPatch.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		if (!fullPatch.isNull()) output.fullPatch = fullPatch.asString();
		if (!leftPatch.isNull()) output.leftPatch = leftPatch.asString();
		if (!middlePatch.isNull()) output.middlePatch = middlePatch.asString();
		if (!rightPatch.isNull()) output.rightPatch = rightPatch.asString();

		const bool hasFull = !output.fullPatch.empty();
		const bool hasParts = !output.leftPatch.empty() || !output.middlePatch.empty() || !output.rightPatch.empty();
		if (hasFull && hasParts)
		{
			return jsonlumpresult_t::PARSEERROR;
		}
		if (hasParts && (output.leftPatch.empty() || output.middlePatch.empty() || output.rightPatch.empty()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		return jsonlumpresult_t::SUCCESS;
	}

	jsonlumpresult_t M_ParseMenuConfTheme(const Json::Value& elem, menuconftheme_t& output)
	{
		if (!(elem.isObject() || elem.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		output = menuconftheme_t();
		if (elem.isNull())
		{
			return jsonlumpresult_t::SUCCESS;
		}

		const Json::Value& cursorPatch = elem["cursorPatch"];
		const Json::Value& cursorOffsetY = elem["cursorOffsetY"];
		const Json::Value& upPatch = elem["upPatch"];
		const Json::Value& downPatch = elem["downPatch"];

		if (!(cursorPatch.isString() || cursorPatch.isNull())
			|| !(cursorOffsetY.isInt() || cursorOffsetY.isNull())
			|| !(upPatch.isString() || upPatch.isNull())
			|| !(downPatch.isString() || downPatch.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		jsonlumpresult_t res = M_ParseMenuConfIndicator(elem["indicator"], output.indicator);
		if (res != jsonlumpresult_t::SUCCESS) return res;

		res = M_ParseMenuConfStringMap(elem["sounds"], output.sounds);
		if (res != jsonlumpresult_t::SUCCESS) return res;

		res = M_ParseMenuConfSlider(elem["slider"], output.slider);
		if (res != jsonlumpresult_t::SUCCESS) return res;

		res = M_ParseMenuConfInputBox(elem["inputBox"], output.inputBox);
		if (res != jsonlumpresult_t::SUCCESS) return res;

		res = M_ParseMenuConfStringMap(elem["fonts"], output.fonts);
		if (res != jsonlumpresult_t::SUCCESS) return res;

		res = M_ParseMenuConfStringMap(elem["colors"], output.colors);
		if (res != jsonlumpresult_t::SUCCESS) return res;

		res = M_ParseMenuConfLayout(elem["layout"], output.layout);
		if (res != jsonlumpresult_t::SUCCESS) return res;

		if (!cursorPatch.isNull()) output.cursorPatch = cursorPatch.asString();
		if (!cursorOffsetY.isNull()) output.cursorOffsetY = cursorOffsetY.asInt();
		if (!upPatch.isNull()) output.upPatch = upPatch.asString();
		if (!downPatch.isNull()) output.downPatch = downPatch.asString();
		return jsonlumpresult_t::SUCCESS;
	}

	jsonlumpresult_t M_ParseMenuConfItemKind(const Json::Value& elem, menuconfitemkind_t& output)
	{
		if (!elem.isString())
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		const std::string kind = elem.asString();
		if (kind == "submenu") output = menuconfitemkind_t::submenu;
		else if (kind == "action") output = menuconfitemkind_t::action;
		else if (kind == "cvarDiscrete") output = menuconfitemkind_t::cvarDiscrete;
		else if (kind == "cvarSlider") output = menuconfitemkind_t::cvarSlider;
		else if (kind == "command") output = menuconfitemkind_t::command;
		else if (kind == "controlBinding") output = menuconfitemkind_t::controlBinding;
		else if (kind == "label") output = menuconfitemkind_t::label;
		else if (kind == "separator") output = menuconfitemkind_t::separator;
		else if (kind == "dynamic") output = menuconfitemkind_t::dynamic;
		else return jsonlumpresult_t::PARSEERROR;

		return jsonlumpresult_t::SUCCESS;
	}

	jsonlumpresult_t M_ParseMenuConfItem(const Json::Value& elem, menuconfitem_t& output)
	{
		if (!elem.isObject())
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		output = menuconfitem_t();
		jsonlumpresult_t res = M_ParseMenuConfItemKind(elem["kind"], output.kind);
		if (res != jsonlumpresult_t::SUCCESS)
		{
			return res;
		}

		const Json::Value& id = elem["id"];
		const Json::Value& text = elem["text"];
		const Json::Value& languageKey = elem["languageKey"];
		const Json::Value& textProvider = elem["textProvider"];
		const Json::Value& patch = elem["patch"];
		const Json::Value& target = elem["target"];
		const Json::Value& action = elem["action"];
		const Json::Value& hotkey = elem["hotkey"];
		const Json::Value& sound = elem["sound"];
		const Json::Value& color = elem["color"];
		const Json::Value& highlightColor = elem["highlightColor"];
		const Json::Value& help = elem["help"];
		const Json::Value& cvar = elem["cvar"];
		const Json::Value& values = elem["values"];
		const Json::Value& widget = elem["widget"];
		const Json::Value& channel = elem["channel"];
		const Json::Value& command = elem["command"];
		const Json::Value& bindingSet = elem["bindingSet"];
		const Json::Value& style = elem["style"];
		const Json::Value& provider = elem["provider"];
		const Json::Value& min = elem["min"];
		const Json::Value& max = elem["max"];
		const Json::Value& step = elem["step"];
		const Json::Value& params = elem["params"];

		if (!(id.isString() || id.isNull())
			|| !(text.isString() || text.isNull())
			|| !(languageKey.isString() || languageKey.isNull())
			|| !(textProvider.isString() || textProvider.isNull())
			|| !(patch.isString() || patch.isNull())
			|| !(target.isString() || target.isNull())
			|| !(action.isString() || action.isNull())
			|| !(hotkey.isString() || hotkey.isNull())
			|| !(sound.isString() || sound.isNull())
			|| !(color.isString() || color.isNull())
			|| !(highlightColor.isString() || highlightColor.isNull())
			|| !(help.isString() || help.isNull())
			|| !(cvar.isString() || cvar.isNull())
			|| !(values.isString() || values.isNull())
			|| !(widget.isString() || widget.isNull())
			|| !(channel.isString() || channel.isNull())
			|| !(command.isString() || command.isNull())
			|| !(bindingSet.isString() || bindingSet.isNull())
			|| !(style.isString() || style.isNull())
			|| !(provider.isString() || provider.isNull())
			|| !(min.isNumeric() || min.isNull())
			|| !(max.isNumeric() || max.isNull())
			|| !(step.isNumeric() || step.isNull())
			|| !(params.isObject() || params.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		if (!id.isNull()) output.id = id.asString();
		if (!text.isNull()) output.text = text.asString();
		if (!languageKey.isNull()) output.languageKey = languageKey.asString();
		if (!textProvider.isNull()) output.textProvider = textProvider.asString();
		if (!patch.isNull()) output.patch = patch.asString();
		if (!target.isNull()) output.target = target.asString();
		if (!action.isNull()) output.action = action.asString();
		if (!hotkey.isNull()) output.hotkey = hotkey.asString();
		if (!sound.isNull()) output.sound = sound.asString();
		if (!color.isNull()) output.color = color.asString();
		if (!highlightColor.isNull()) output.highlightColor = highlightColor.asString();
		if (!help.isNull()) output.help = help.asString();
		if (!cvar.isNull()) output.cvar = cvar.asString();
		if (!values.isNull()) output.values = values.asString();
		if (!widget.isNull()) output.widget = widget.asString();
		if (!channel.isNull()) output.channel = channel.asString();
		if (!command.isNull()) output.command = command.asString();
		if (!bindingSet.isNull()) output.bindingSet = bindingSet.asString();
		if (!style.isNull()) output.style = style.asString();
		if (!provider.isNull()) output.provider = provider.asString();
		if (!min.isNull()) output.min = min.asDouble();
		if (!max.isNull()) output.max = max.asDouble();
		if (!step.isNull()) output.step = step.asDouble();
		output.params = params;

		switch (output.kind)
		{
		case menuconfitemkind_t::submenu:
			return output.target.empty() ? jsonlumpresult_t::PARSEERROR : jsonlumpresult_t::SUCCESS;

		case menuconfitemkind_t::action:
			return (output.action.empty() && output.target.empty()) ? jsonlumpresult_t::PARSEERROR : jsonlumpresult_t::SUCCESS;

		case menuconfitemkind_t::cvarDiscrete:
			return (output.cvar.empty() || output.values.empty()) ? jsonlumpresult_t::PARSEERROR : jsonlumpresult_t::SUCCESS;

		case menuconfitemkind_t::cvarSlider:
			return output.cvar.empty() ? jsonlumpresult_t::PARSEERROR : jsonlumpresult_t::SUCCESS;

		case menuconfitemkind_t::command:
			return output.command.empty() ? jsonlumpresult_t::PARSEERROR : jsonlumpresult_t::SUCCESS;

		case menuconfitemkind_t::controlBinding:
			return (output.bindingSet.empty() || output.command.empty()) ? jsonlumpresult_t::PARSEERROR : jsonlumpresult_t::SUCCESS;

		case menuconfitemkind_t::label:
		case menuconfitemkind_t::separator:
		case menuconfitemkind_t::dynamic:
			return jsonlumpresult_t::SUCCESS;
		}

		return jsonlumpresult_t::PARSEERROR;
	}

	jsonlumpresult_t M_ParseMenuConfItems(const Json::Value& elem, std::vector<menuconfitem_t>& output)
	{
		if (!(elem.isArray() || elem.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		output.clear();
		if (elem.isNull())
		{
			return jsonlumpresult_t::SUCCESS;
		}

		for (const Json::Value& itemelem : elem)
		{
			menuconfitem_t item;
			jsonlumpresult_t res = M_ParseMenuConfItem(itemelem, item);
			if (res != jsonlumpresult_t::SUCCESS)
			{
				return res;
			}

			output.push_back(std::move(item));
		}

		return jsonlumpresult_t::SUCCESS;
	}

	jsonlumpresult_t M_ParseMenuConfMenu(const Json::Value& elem, menuconfmenu_t& output)
	{
		if (!elem.isObject())
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		jsonlumpresult_t res = M_ParseMenuConfHeader(elem["header"], output.header);
		if (res != jsonlumpresult_t::SUCCESS) return res;

		res = M_ParseMenuConfLayout(elem["layout"], output.layout);
		if (res != jsonlumpresult_t::SUCCESS) return res;

		res = M_ParseMenuConfStringMap(elem["sounds"], output.sounds);
		if (res != jsonlumpresult_t::SUCCESS) return res;

		res = M_ParseMenuConfStringMap(elem["colors"], output.colors);
		if (res != jsonlumpresult_t::SUCCESS) return res;

		return M_ParseMenuConfItems(elem["items"], output.items);
	}

	jsonlumpresult_t M_ParseMenuConfMenus(const Json::Value& elem,
	                                      std::unordered_map<std::string, menuconfmenu_t>& output)
	{
		if (!(elem.isObject() || elem.isNull()))
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		output.clear();
		if (elem.isNull())
		{
			return jsonlumpresult_t::SUCCESS;
		}

		for (const auto& key : elem.getMemberNames())
		{
			menuconfmenu_t menu;
			jsonlumpresult_t res = M_ParseMenuConfMenu(elem[key], menu);
			if (res != jsonlumpresult_t::SUCCESS)
			{
				return res;
			}

			output[key] = std::move(menu);
		}

		return jsonlumpresult_t::SUCCESS;
	}
}

void menuconfdatabase_t::clear()
{
	theme = menuconftheme_t();
	menus.clear();
	entrypoints.clear();
}

void menuconfdatabase_t::merge(const menuconfdatabase_t& other)
{
	if (!other.theme.indicator.patches.empty()) theme.indicator = other.theme.indicator;
	if (!other.theme.cursorPatch.empty()) theme.cursorPatch = other.theme.cursorPatch;
	if (other.theme.cursorOffsetY != 0) theme.cursorOffsetY = other.theme.cursorOffsetY;
	for (const auto& [key, value] : other.theme.sounds) theme.sounds[key] = value;
	if (!other.theme.upPatch.empty()) theme.upPatch = other.theme.upPatch;
	if (!other.theme.downPatch.empty()) theme.downPatch = other.theme.downPatch;
	if (!other.theme.slider.leftPatch.empty() || !other.theme.slider.middlePatch.empty()
		|| !other.theme.slider.rightPatch.empty() || !other.theme.slider.knobPatch.empty()
		|| !other.theme.slider.greenKnobPatch.empty() || !other.theme.slider.overlayPatch.empty())
	{
		theme.slider = other.theme.slider;
	}
	if (!other.theme.inputBox.fullPatch.empty() || !other.theme.inputBox.leftPatch.empty()
		|| !other.theme.inputBox.middlePatch.empty() || !other.theme.inputBox.rightPatch.empty())
	{
		theme.inputBox = other.theme.inputBox;
	}
	for (const auto& [key, value] : other.theme.fonts) theme.fonts[key] = value;
	for (const auto& [key, value] : other.theme.colors) theme.colors[key] = value;
	if (!other.theme.layout.style.empty() || other.theme.layout.x != 0 || other.theme.layout.y != 0
		|| other.theme.layout.indent != 0 || other.theme.layout.lineHeight != "auto"
		|| other.theme.layout.scroll || other.theme.layout.topPadding != 0
		|| other.theme.layout.itemSpacing != "font")
	{
		theme.layout = other.theme.layout;
	}

	for (const auto& [key, value] : other.menus) menus[key] = value;
	for (const auto& [key, value] : other.entrypoints) entrypoints[key] = value;
}

jsonlumpresult_t M_ParseMenuConf(menuconfdatabase_t& out, int lumpindex)
{
	out.clear();
	return M_ParseJSONLump(lumpindex, MENUCONF_LUMPTYPE.data(), MENUCONF_VERSION,
		[&out](const Json::Value& data, const JSONLumpVersion&) -> jsonlumpresult_t
		{
			if (!data.isObject())
			{
				return jsonlumpresult_t::MALFORMEDROOT;
			}

			jsonlumpresult_t res = M_ParseMenuConfTheme(data["theme"], out.theme);
			if (res != jsonlumpresult_t::SUCCESS) return res;

			res = M_ParseMenuConfMenus(data["menus"], out.menus);
			if (res != jsonlumpresult_t::SUCCESS) return res;

			return M_ParseMenuConfStringMap(data["entrypoints"], out.entrypoints);
		});
}

void M_ClearMenuConf()
{
	gMenuConf.clear();
}

bool M_LoadMenuConf()
{
	menuconfdatabase_t database;
	menuconfdatabase_t parsed;

	const int builtinLump = W_FindLump(MENUCONF_BASE_LUMPNAME.data(), -1);
	if (builtinLump < 0) 
	{
		I_Error("M_LoadMenuConf: missing built-in {} lump", MENUCONF_BASE_LUMPNAME);
		return false;
	}

	jsonlumpresult_t result = M_ParseMenuConf(parsed, builtinLump);
	if (result != jsonlumpresult_t::SUCCESS)
	{
		I_Error("M_LoadMenuConf: built-in {} JSON error: {}",
		        MENUCONF_BASE_LUMPNAME,
		        M_JSONLumpResultToString(result));
		return false;
	}
	database.merge(parsed);

	if (!gameinfo.baseMenuConfLump.empty())
	{
		const int lump = W_CheckNumForName(gameinfo.baseMenuConfLump);
		if (lump >= 0)
		{
			result = M_ParseMenuConf(parsed, lump);
			if (result != jsonlumpresult_t::SUCCESS)
			{
				I_Error("M_LoadMenuConf: {} JSON error: {}", gameinfo.baseMenuConfLump,
				        M_JSONLumpResultToString(result));
				return false;
			}
			database.merge(parsed);
		}
	}

	if (!gameinfo.overrideMenuConfLump.empty())
	{
		const int lump = W_CheckNumForName(gameinfo.overrideMenuConfLump);
		if (lump >= 0)
		{
			result = M_ParseMenuConf(parsed, lump);
			if (result != jsonlumpresult_t::SUCCESS)
			{
				I_Error("M_LoadMenuConf: {} JSON error: {}", gameinfo.overrideMenuConfLump,
				        M_JSONLumpResultToString(result));
				return false;
			}
			database.merge(parsed);
		}
	}

	int lump = -1;
	while ((lump = W_FindLump(MENUCONF_OVERRIDE_LUMPNAME.data(), lump)) != -1)
	{
		result = M_ParseMenuConf(parsed, lump);
		if (result != jsonlumpresult_t::SUCCESS)
		{
			I_Error("M_LoadMenuConf: {} JSON error in lump {}: {}",
			        MENUCONF_OVERRIDE_LUMPNAME,
			        W_LumpName(lump),
			        M_JSONLumpResultToString(result));
			return false;
		}
		database.merge(parsed);
	}

	gMenuConf = std::move(database);
	return true;
}

menuconfdatabase_t& M_MenuConf()
{
	return gMenuConf;
}
