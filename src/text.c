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

void LoadGameFont() {
	gameFont = LoadFont("src/resources/fonts/pixantiqua.png");
}

void UnloadGameFont() {
	UnloadFont(gameFont);
}

Font GetGameFont() {
	if (gameFont.texture.id == 0) {
		return GetFontDefault();
	}
	return gameFont;
}

const Color gameColours[TOTAL_GCOLOURS] = {
	[C_RED] = { 210, 31, 45, 255 },
	[C_GREEN] = { 0, 111, 231, 255 },
	[C_LIGHTBLUE] = { 0, 121, 241, 255 },
	[C_YELLOW] = { 253, 249, 0, 255 },
	[C_GOLD] = { 255, 203, 0, 255 },
	[C_DARKPURPLE] = DARKPURPLE,
	[C_SKYBLUE] = SKYBLUE,
};

Color GetGameColorAlpha(GameColours c, unsigned char a) {
	Color gc = gameColours[c];
	gc.a = a;
	return gc;
}

static Color GetGameColor(int code, Color defColour) {
	if (code == '0') {
		return GetGameColorAlpha(C_RED, defColour.a);
	}
	if (code == '1') {
		return GetGameColorAlpha(C_GREEN, defColour.a);
	}
	if (code == '2') {
		return GetGameColorAlpha(C_LIGHTBLUE, defColour.a);
	}
	if (code == '3') {
		return GetGameColorAlpha(C_YELLOW, defColour.a);
	}
	if (code == '4') {
		return GetGameColorAlpha(C_GOLD, defColour.a);
	}
	if (code == '5') {
		return GetGameColorAlpha(C_DARKPURPLE, defColour.a);
	}
	if (code == '6') {
		return GetGameColorAlpha(C_SKYBLUE, defColour.a);
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
	[IDESC_KEY] = "{3}key{r}",
	[IDESC_KEYS] = "{3}keys{r}",
	[BUY_ITEM] = "Do you want to buy %d %s?",
	[IDESC_EXP] = "{4}exposure{r}",
	[IDESC_BOMB] = "{5}bomb{r}",
	[IDESC_BOMBS] = "{5}bombs{r}",
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

const char* GetBuyItemText(Language lang, ItemType itype, int count) {
	switch (itype) {
		case I_KEY:
			return TextFormat(GetText(lang, BUY_ITEM), count, GetText(lang, count == 1 ? IDESC_KEY : IDESC_KEYS));
		case I_EXP:
			return TextFormat(GetText(lang, BUY_ITEM), count, GetText(lang, IDESC_EXP));
		case I_BOMB:
			return TextFormat(GetText(lang, BUY_ITEM), count, GetText(lang, count == 1 ? IDESC_BOMB : IDESC_BOMBS));
		case I_RELIC:
			return "relic";
		case I_CONSUMABLE:
			return "consumable";
		case I_GEAR:
			return "gear";
		default: return INVALID_STRING;
	}
}
