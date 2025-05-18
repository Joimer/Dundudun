#include <raylib.h>
#include "text.h"
#include "lib.h"
#include "item.h"

// This is a static global in Raylib,,,
// Need to call SetTextLineSpacing(textLineSpacing) somewhere
static float textLineSpacing = 2;

static float GetLineSpacing() {
	return textLineSpacing;
}

static Font gameFont;

Font GetGameFont() {
	if (gameFont.texture.id == 0) {
		gameFont = LoadFont("src/resources/fonts/pixantiqua.png");
	}
	if (gameFont.texture.id == 0) return GetFontDefault();
	return gameFont;
}

const Color gameColours[TOTAL_GCOLOURS] = {
	[C_RED] = { 210, 31, 45, 255 },
	[C_GREEN] = { 0, 111, 231, 255 },
	[C_LIGHTBLUE] = { 0, 121, 241, 255 },
	[C_YELLOW] = { 253, 249, 0, 255 },
};

static Color GetGameColor(int code, Color defColour) {
	if (code == '0') {
		return gameColours[C_RED];
	}
	if (code == '1') {
		return gameColours[C_GREEN];
	}
	if (code == '2') {
		return gameColours[C_LIGHTBLUE];
	}
	if (code == '3') {
		return gameColours[C_YELLOW];
	}
	return defColour;
}

// Reimplements DrawTextEx to use various different colours within the same text.
void DrawColourText(
	const char *text, int posX, int posY, float fontSize, Color baseTint
) {
	Font font = GetGameFont();
	float spacing = GetLineSpacing();
	int size = TextLength(text);
	float textOffsetY = 0;
	float textOffsetX = 0.0f;
	float scaleFactor = fontSize / font.baseSize;
	Color currentTint = baseTint;
	bool colourSwap = false;

	for (int i = 0; i < size;) {
		// Get next codepoint from byte string and glyph index in font
		int codepointByteCount = 0;
		int codepoint = GetCodepointNext(&text[i], &codepointByteCount);
		int index = GetGlyphIndex(font, codepoint);

		// Check for colour swap mode.
		if (codepoint == '{' && !colourSwap) {
			colourSwap = true;
			i += codepointByteCount;
			continue;
		}
		if (colourSwap) {
			if (codepoint == '}') {
				colourSwap = false;
			} else {
				if (codepoint == 'r') {
					currentTint = baseTint;
				} else {
					currentTint = GetGameColor(codepoint, baseTint);
					currentTint.a = baseTint.a;
				}
			}
			i += codepointByteCount;
			continue;
		}

		// Regular text;
		if (codepoint == '\n') {
			textOffsetY += (fontSize + textLineSpacing);
			textOffsetX = 0.0f;
		} else {
			if ((codepoint != ' ') && (codepoint != '\t')) {
				DrawTextCodepoint(font, codepoint, (Vector2){ posX + textOffsetX, posY + textOffsetY }, fontSize, currentTint);
			}

			if (font.glyphs[index].advanceX == 0) {
				textOffsetX += ((float)font.recs[index].width * scaleFactor + spacing);
			} else {
				textOffsetX += ((float)font.glyphs[index].advanceX * scaleFactor + spacing);
			}
		}

		i += codepointByteCount;
	}
}

static const char* INVALID_STRING = "INVALID_STRING";

static const char* texts[TOTAL_TEXTS] = {
	[HP_LABEL] = "%d HP",
	[BOMB_LABEL] = "Bombs: %d",
	[KEYS_LABEL] = "Keys: %d",
	[EXP_LABEL] = "EXP: %d",
	[LEFTOVER_LUNCH_LABEL] = "Leftover Lunch",
	[LEFTOVER_LUNCH_TOOLTIP] = "Adds 2 {1}poison{r} on hit.",
	[MACHINE_COFFEE_LABEL] = "Machine Coffee",
	[MACHINE_COFFEE_TOOLTIP] = "Adds 5 {0}burn{r} on hit.",
	[HR_HEART_LABEL] = "HR Heart",
	[HR_HEART_TOOLTIP] = "Adds {2}freeze{r} on hit.",
	[SHOCKING_PIC_LABEL] = "Shocking Picture",
	[SHOCKING_PIC_TOOLTIP] = "Adds {3}paralyse{r} on hit.",
};

const char* GetText(Language lang, GameText text) {
	// TODO: Usage of lang XD!
	if (text >= TOTAL_TEXTS) {
		LogDebug("Attempted to get invalid string!!");
		return INVALID_STRING;
	}

	return texts[text];
}

const char* GetRelicLabel(Language lang, int relic) {
	if (relic == LEFTOVER_LUNCH) {
		return GetText(lang, LEFTOVER_LUNCH_LABEL);
	}
	if (relic == MACHINE_COFFEE) {
		return GetText(lang, MACHINE_COFFEE_LABEL);
	}
	if (relic == HR_HEART) {
		return GetText(lang, HR_HEART_LABEL);
	}
	if (relic == SHOCKING_PIC) {
		return GetText(lang, SHOCKING_PIC_LABEL);
	}

	return INVALID_STRING;
}

const char* GetRelicTooltip(Language lang, int relic) {
	if (relic == LEFTOVER_LUNCH) {
		return GetText(lang, LEFTOVER_LUNCH_TOOLTIP);
	}
	if (relic == MACHINE_COFFEE) {
		return GetText(lang, MACHINE_COFFEE_TOOLTIP);
	}
	if (relic == HR_HEART) {
		return GetText(lang, HR_HEART_TOOLTIP);
	}
	if (relic == SHOCKING_PIC) {
		return GetText(lang, SHOCKING_PIC_TOOLTIP);
	}

	return INVALID_STRING;
}
