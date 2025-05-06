/*****************************
 * Game scenes configuration *
 *****************************/

#ifndef SCREENS_H
#define SCREENS_H

#define LOGO_DURATION 4.0f
#define LOGO_FADE_TIME 1.0f

// These are the screens the game can be in.
typedef enum { LOADING = -1, LOGO = 0, TITLE, OPTIONS, GAMEPLAY, ENDING } GameScreen;

#endif
