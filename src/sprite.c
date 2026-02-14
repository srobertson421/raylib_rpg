#include "sprite.h"
#include <stdlib.h>
#include <string.h>

AnimatedSprite *sprite_create(const char *texture_path, int frame_w, int frame_h) {
    AnimatedSprite *sprite = calloc(1, sizeof(AnimatedSprite));
    if (!sprite) return NULL;

    sprite->texture = LoadTexture(texture_path);
    sprite->frame_width = frame_w;
    sprite->frame_height = frame_h;
    sprite->columns = sprite->texture.width / frame_w;

    return sprite;
}

void sprite_destroy(AnimatedSprite *sprite) {
    if (!sprite) return;
    UnloadTexture(sprite->texture);
    free(sprite);
}

int sprite_add_animation(AnimatedSprite *sprite, const char *name,
                         int start_frame, int frame_count, int idle_frame,
                         float fps, bool loop) {
    if (sprite->animation_count >= SPRITE_MAX_ANIMATIONS) return -1;

    int idx = sprite->animation_count++;
    SpriteAnimation *anim = &sprite->animations[idx];
    strncpy(anim->name, name, sizeof(anim->name) - 1);
    anim->name[sizeof(anim->name) - 1] = '\0';
    anim->start_frame = start_frame;
    anim->frame_count = frame_count;
    anim->idle_frame = idle_frame;
    anim->fps = fps;
    anim->loop = loop;

    return idx;
}

void sprite_play(AnimatedSprite *sprite, int anim_index) {
    if (anim_index < 0 || anim_index >= sprite->animation_count) return;
    if (anim_index == sprite->current_animation && sprite->playing) return;

    sprite->current_animation = anim_index;
    sprite->current_frame = 0;
    sprite->frame_timer = 0.0f;
    sprite->playing = true;
}

void sprite_stop(AnimatedSprite *sprite) {
    sprite->playing = false;
}

void sprite_update(AnimatedSprite *sprite, float dt) {
    if (!sprite->playing) return;
    if (sprite->animation_count == 0) return;

    SpriteAnimation *anim = &sprite->animations[sprite->current_animation];
    if (anim->frame_count <= 1) return;

    sprite->frame_timer += dt;
    float frame_duration = 1.0f / anim->fps;

    while (sprite->frame_timer >= frame_duration) {
        sprite->frame_timer -= frame_duration;
        sprite->current_frame++;

        if (sprite->current_frame >= anim->frame_count) {
            if (anim->loop) {
                sprite->current_frame = 0;
            } else {
                sprite->current_frame = anim->frame_count - 1;
                sprite->playing = false;
                break;
            }
        }
    }
}

void sprite_draw(AnimatedSprite *sprite, float x, float y, Color tint) {
    sprite_draw_ex(sprite, x, y, 1.0f, tint);
}

void sprite_draw_ex(AnimatedSprite *sprite, float x, float y, float scale, Color tint) {
    if (sprite->animation_count == 0) return;

    SpriteAnimation *anim = &sprite->animations[sprite->current_animation];

    int flat_frame;
    if (sprite->playing) {
        flat_frame = anim->start_frame + sprite->current_frame;
    } else {
        flat_frame = anim->start_frame + anim->idle_frame;
    }

    int col = flat_frame % sprite->columns;
    int row = flat_frame / sprite->columns;

    Rectangle src = {
        (float)(col * sprite->frame_width),
        (float)(row * sprite->frame_height),
        (float)sprite->frame_width,
        (float)sprite->frame_height
    };
    Rectangle dst = {
        x, y,
        (float)sprite->frame_width * scale,
        (float)sprite->frame_height * scale
    };

    DrawTexturePro(sprite->texture, src, dst, (Vector2){0, 0}, 0.0f, tint);
}

void sprite_draw_reflected(AnimatedSprite *sprite, float x, float y, Color tint) {
    if (sprite->animation_count == 0) return;

    SpriteAnimation *anim = &sprite->animations[sprite->current_animation];

    int flat_frame;
    if (sprite->playing) {
        flat_frame = anim->start_frame + sprite->current_frame;
    } else {
        flat_frame = anim->start_frame + anim->idle_frame;
    }

    int col = flat_frame % sprite->columns;
    int row = flat_frame / sprite->columns;

    Rectangle src = {
        (float)(col * sprite->frame_width),
        (float)(row * sprite->frame_height),
        (float)sprite->frame_width,
        (float)(-sprite->frame_height)  // Negative = vertical flip
    };
    Rectangle dst = {
        x, y,
        (float)sprite->frame_width,
        (float)sprite->frame_height
    };

    DrawTexturePro(sprite->texture, src, dst, (Vector2){0, 0}, 0.0f, tint);
}
