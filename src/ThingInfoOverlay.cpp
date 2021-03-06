
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ThingInfoOverlay.cpp
 * Description: ThingInfoOverlay class - a map editor overlay that
 *              displays information about the currently highlighted
 *              thing in things mode
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "WxStuff.h"
#include "MapThing.h"
#include "ThingInfoOverlay.h"
#include "GameConfiguration.h"
#include "ColourConfiguration.h"
#include "Drawing.h"
#include "MapEditorWindow.h"
#include "OpenGL.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, use_zeth_icons)


/*******************************************************************
 * THINGINFOOVERLAY CLASS FUNCTIONS
 *******************************************************************/

/* ThingInfoOverlay::ThingInfoOverlay
 * ThingInfoOverlay class constructor
 *******************************************************************/
ThingInfoOverlay::ThingInfoOverlay()
{
	text_box = new TextBox("", Drawing::FONT_CONDENSED, 100, 16);
	last_size = 100;
}

/* ThingInfoOverlay::~ThingInfoOverlay
 * ThingInfoOverlay class destructor
 *******************************************************************/
ThingInfoOverlay::~ThingInfoOverlay()
{
	delete text_box;
}

/* ThingInfoOverlay::update
 * Updates the overlay with info from [thing]
 *******************************************************************/
void ThingInfoOverlay::update(MapThing* thing)
{
	if (!thing)
		return;

	string info_text;
	sprite = "";
	translation = "";
	palette = "";
	icon = "";
	int map_format = theMapEditor->currentMapDesc().format;

	// Index + type
	ThingType* tt = theGameConfiguration->thingType(thing->getType());
	string type = S_FMT("%s (Type %d)", tt->getName(), thing->getType());
	if (Global::debug)
		info_text += S_FMT("Thing #%d (%d): %s\n", thing->getIndex(), thing->getId(), type);
	else
		info_text += S_FMT("Thing #%d: %s\n", thing->getIndex(), type);

	// Position
	if (map_format != MAP_DOOM)
		info_text += S_FMT("Position: %d, %d, %d\n", (int)thing->xPos(), (int)thing->yPos(), (int)(thing->floatProperty("height")));
	else
		info_text += S_FMT("Position: %d, %d\n", (int)thing->xPos(), (int)thing->yPos());

	// Direction
	int angle = thing->intProperty("angle");
	string dir = S_FMT("%d degrees", angle);
	if (angle == 0)
		dir = "East";
	else if (angle == 45)
		dir = "Northeast";
	else if (angle == 90)
		dir = "North";
	else if (angle == 135)
		dir = "Northwest";
	else if (angle == 180)
		dir = "West";
	else if (angle == 225)
		dir = "Southwest";
	else if (angle == 270)
		dir = "South";
	else if (angle == 315)
		dir = "Southeast";
	info_text += S_FMT("Direction: %s\n", dir);

	// Special and Args (if in hexen format or udmf with thing args)
	if (map_format == MAP_HEXEN ||
	        (map_format == MAP_UDMF && theGameConfiguration->getUDMFProperty("arg0", MOBJ_THING)))
	{
		int as_id = thing->intProperty("special");
		info_text += S_FMT("Special: %d (%s)\n", as_id, theGameConfiguration->actionSpecialName(as_id));
		int args[5];
		args[0] = thing->intProperty("arg0");
		args[1] = thing->intProperty("arg1");
		args[2] = thing->intProperty("arg2");
		args[3] = thing->intProperty("arg3");
		args[4] = thing->intProperty("arg4");
		string argstr;
		if (tt->getArgspec().count > 0)
			argstr = tt->getArgsString(args);
		else
			argstr = theGameConfiguration->actionSpecial(as_id)->getArgsString(args);

		if (!argstr.IsEmpty())
			info_text += S_FMT("%s\n", argstr);
		else
			info_text += "No Args\n";
	}

	// Flags
	if (map_format != MAP_UDMF)
		info_text += S_FMT("Flags: %s\n", theGameConfiguration->thingFlagsString(thing->intProperty("flags")));

	// TID (if in doom64/hexen/udmf format)
	if (map_format != MAP_DOOM)
		info_text += S_FMT("TID: %i", thing->intProperty("id"));

	if (info_text.EndsWith("\n"))
		info_text.RemoveLast(1);

	// Set sprite and translation
	sprite = tt->getSprite();
	translation = tt->getTranslation();
	palette = tt->getPalette();
	icon = tt->getIcon();
	zeth = tt->getZeth();

	// Setup text box
	text_box->setText(info_text);
}

/* ThingInfoOverlay::draw
 * Draws the overlay at [bottom] from 0 to [right]
 *******************************************************************/
void ThingInfoOverlay::draw(int bottom, int right, float alpha)
{
	// Don't bother if invisible
	if (alpha <= 0.0f)
		return;

	// Init GL stuff
	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);

	// Determine overlay height
	if (last_size != right)
	{
		last_size = right;
		text_box->setSize(right - 68);
	}
	int height = text_box->getHeight() + 4;

	// Slide in/out animation
	float alpha_inv = 1.0f - alpha;
	bottom += height*alpha_inv*alpha_inv;

	// Get colours
	rgba_t col_bg = ColourConfiguration::getColour("map_overlay_background");
	rgba_t col_fg = ColourConfiguration::getColour("map_overlay_foreground");
	col_fg.a = col_fg.a*alpha;
	col_bg.a = col_bg.a*alpha;
	rgba_t col_border(0, 0, 0, 140);

	// Draw overlay background
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Drawing::drawBorderedRect(0, bottom - height - 4, right, bottom+2, col_bg, col_border);

	// Draw info text lines
	text_box->draw(2, bottom - height, col_fg);

	// Draw sprite
	bool isicon = false;
	GLTexture* tex = theMapEditor->textureManager().getSprite(sprite, translation, palette);
	if (!tex)
	{
		if (use_zeth_icons && zeth >= 0)
			tex = theMapEditor->textureManager().getEditorImage(S_FMT("zethicons/zeth%02d", zeth));
		if (!tex)
			tex = theMapEditor->textureManager().getEditorImage(S_FMT("thing/%s", icon));
		isicon = true;
	}
	glEnable(GL_TEXTURE_2D);
	OpenGL::setColour(255, 255, 255, 255*alpha, 0);
	if (tex)
	{
		double width = tex->getWidth();
		double height = tex->getHeight();
		if (isicon)
		{
			width = 64;
			height = 64;
		}
		tex->bind();
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);	glVertex2d(right - 8 - width, bottom - 8 - height);
		glTexCoord2f(0.0f, 1.0f);	glVertex2d(right - 8 - width, bottom - 8);
		glTexCoord2f(1.0f, 1.0f);	glVertex2d(right - 8, bottom - 8);
		glTexCoord2f(1.0f, 0.0f);	glVertex2d(right - 8, bottom - 8 - height);
		glEnd();
	}
	glDisable(GL_TEXTURE_2D);

	// Done
	glEnable(GL_LINE_SMOOTH);
}
