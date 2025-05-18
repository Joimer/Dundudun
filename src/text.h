#ifndef TEXT_H
#define TEXT_H

#include <raylib.h>

#define TOTAL_TEXTS 20
#define TOTAL_GCOLOURS 5

typedef enum { C_RED, C_GREEN, C_LIGHTBLUE, C_YELLOW } GameColours;

extern const Color gameColours[TOTAL_GCOLOURS];

typedef enum { EN, ES } Language;

typedef enum {
	HP_LABEL, BOMB_LABEL, KEYS_LABEL, EXP_LABEL,
	LEFTOVER_LUNCH_LABEL, LEFTOVER_LUNCH_TOOLTIP,
	MACHINE_COFFEE_LABEL, MACHINE_COFFEE_TOOLTIP,
	HR_HEART_LABEL, HR_HEART_TOOLTIP,
	SHOCKING_PIC_LABEL, SHOCKING_PIC_TOOLTIP,
} GameText;

void DrawColourText(Font font, const char *text, Vector2 position, float fontSize, Color baseTint);
const char* GetText(Language lang, GameText text);
// TODO etc. game.c loads text.h so we cannot use the enums from other places here uuuuh DOUSHIOU
const char* GetRelicLabel(Language lang, int relic);
const char* GetRelicTooltip(Language lang, int relic);

#endif
