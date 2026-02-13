#ifndef AUDIO_H
#define AUDIO_H

#include "raylib.h"
#include <stdbool.h>

#define MAX_MUSIC_SECTIONS 16
#define SECTION_NAME_LEN 32

typedef struct MusicSection {
    char name[SECTION_NAME_LEN];
    float start_time;    // seconds
    float end_time;      // seconds
    bool loop;           // seek back to start_time when end_time reached
} MusicSection;

typedef struct AudioManager {
    // Current track
    Music current_music;
    bool music_loaded;
    float music_volume;
    float current_fade;       // effective volume multiplier 0..1

    // Crossfade: track being faded out
    Music fading_music;
    bool fading_loaded;
    float fading_fade;        // fade-out multiplier 1..0
    float fade_duration;      // default 1.5s
    float fade_timer;         // elapsed fade time
    bool fading_active;

    // Track dedup — skip reload if same track requested
    char current_path[256];

    // Sectioned music
    MusicSection sections[MAX_MUSIC_SECTIONS];
    int section_count;
    int active_section;       // -1 = whole track
} AudioManager;

typedef struct EventBus EventBus;

AudioManager *audio_create(void);
void audio_destroy(AudioManager *audio);
void audio_update(AudioManager *audio);

void audio_play_music(AudioManager *audio, const char *path, bool loop);
void audio_stop_music(AudioManager *audio);
void audio_set_music_volume(AudioManager *audio, float volume);

void audio_define_sections(AudioManager *audio, const MusicSection *sections, int count);
void audio_clear_sections(AudioManager *audio);
void audio_play_section(AudioManager *audio, const char *section_name);

void audio_subscribe_events(AudioManager *audio, EventBus *bus);

#endif
