#ifndef TEXT_H
#define TEXT_H

#include "game.h"
#include "item.h"
#include <raylib.h>

#define TOTAL_TEXTS 35
#define TOTAL_GCOLOURS 7

typedef enum {
	C_RED, C_GREEN, C_LIGHTBLUE, C_YELLOW, C_GOLD,
	C_DARKPURPLE, C_SKYBLUE,
} GameColours;

extern const Color gameColours[TOTAL_GCOLOURS];

typedef enum {
	M_NEW_GAME, M_CONTINUE, M_OPTIONS, M_EXIT,
	HP_LABEL, BOMB_LABEL, KEYS_LABEL, EXP_LABEL,
	LEFTOVER_LUNCH_LABEL, LEFTOVER_LUNCH_TOOLTIP,
	MACHINE_COFFEE_LABEL, MACHINE_COFFEE_TOOLTIP,
	HR_HEART_LABEL, HR_HEART_TOOLTIP,
	SHOCKING_PIC_LABEL, SHOCKING_PIC_TOOLTIP,
	IDESC_KEY, IDESC_KEYS, BUY_ITEM, IDESC_EXP,
	IDESC_BOMB, IDESC_BOMBS, LETTER_OPENER_LABEL,
	CLIP_BOX_LABEL, YES_LABEL, NO_LABEL,
} GameText;

void LoadGameFont();
void UnloadGameFont();
Font GetGameFont();
Color GetGameColorAlpha(GameColours c, unsigned char a);
void DrawColourText(const char *text, int posX, int posY, float fontSize, Color baseTint);
const char* GetText(Language lang, GameText text);
// TODO etc. game.c loads text.h so we cannot use the enums from other places here uuuuh DOUSHIOU
const char* GetRelicLabel(Language lang, int relic);
const char* GetRelicTooltip(Language lang, int relic);
const char* GetBuyItemText(Language lang, ItemType itype, int count);

#endif
