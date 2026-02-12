#ifndef SPRITE_H
#define SPRITE_H

#include "raylib.h"
#include <stdbool.h>

#define SPRITE_MAX_ANIMATIONS 16

typedef struct SpriteAnimation {
    char name[32];
    int start_frame;
    int frame_count;
    int idle_frame;
    float fps;
    bool loop;
} SpriteAnimation;

typedef struct AnimatedSprite {
    Texture2D texture;
    int frame_width;
    int frame_height;
    int columns;

    SpriteAnimation animations[SPRITE_MAX_ANIMATIONS];
    int animation_count;

    int current_animation;
    int current_frame;
    float frame_timer;
    bool playing;
} AnimatedSprite;

AnimatedSprite *sprite_create(const char *texture_path, int frame_w, int frame_h);
void sprite_destroy(AnimatedSprite *sprite);
int sprite_add_animation(AnimatedSprite *sprite, const char *name,
                         int start_frame, int frame_count, int idle_frame,
                         float fps, bool loop);
void sprite_play(AnimatedSprite *sprite, int anim_index);
void sprite_stop(AnimatedSprite *sprite);
void sprite_update(AnimatedSprite *sprite, float dt);
void sprite_draw(AnimatedSprite *sprite, float x, float y, Color tint);
void sprite_draw_ex(AnimatedSprite *sprite, float x, float y, float scale, Color tint);

#endif
