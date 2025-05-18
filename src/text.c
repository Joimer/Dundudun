#include "text.h"
#include "lib.h"
#include "item.h"

static const char* INVALID_STRING = "INVALID_STRING";

static const char* texts[TOTAL_TEXTS] = {
	[HP_LABEL] = "%d HP",
	[BOMB_LABEL] = "Bombs: %d",
	[KEYS_LABEL] = "Keys: %d",
	[EXP_LABEL] = "EXP: %d",
	[LEFTOVER_LUNCH_LABEL] = "Leftover Lunch",
	[LEFTOVER_LUNCH_TOOLTIP] = "Adds 2 poison on hit.",
	[MACHINE_COFFEE_LABEL] = "Machine Coffee",
	[MACHINE_COFFEE_TOOLTIP] = "Adds 5 burn on hit.",
	[HR_HEART_LABEL] = "HR Heart",
	[HR_HEART_TOOLTIP] = "Adds freeze on hit.",
	[SHOCKING_PIC_LABEL] = "Shocking Picture",
	[SHOCKING_PIC_TOOLTIP] = "Adds paralyse on hit.",
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
