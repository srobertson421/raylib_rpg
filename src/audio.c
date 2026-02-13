#include "audio.h"
#include "event.h"
#include "game.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

AudioManager *audio_create(void) {
    AudioManager *audio = calloc(1, sizeof(AudioManager));
    audio->music_volume = 0.5f;
    audio->current_fade = 1.0f;
    audio->fade_duration = 1.5f;
    audio->active_section = -1;
    return audio;
}

void audio_destroy(AudioManager *audio) {
    if (!audio) return;
    if (audio->fading_loaded) {
        StopMusicStream(audio->fading_music);
        UnloadMusicStream(audio->fading_music);
    }
    if (audio->music_loaded) {
        StopMusicStream(audio->current_music);
        UnloadMusicStream(audio->current_music);
    }
    free(audio);
}

static void finish_fade(AudioManager *audio) {
    if (audio->fading_loaded) {
        StopMusicStream(audio->fading_music);
        UnloadMusicStream(audio->fading_music);
        audio->fading_loaded = false;
    }
    audio->fading_active = false;
    audio->fading_fade = 0.0f;
    audio->current_fade = 1.0f;
}

void audio_update(AudioManager *audio) {
    if (!audio) return;

    float dt = GetFrameTime();

    // Advance crossfade
    if (audio->fading_active) {
        audio->fade_timer += dt;
        float t = audio->fade_timer / audio->fade_duration;
        if (t >= 1.0f) t = 1.0f;

        audio->current_fade = t;          // new track fades in 0→1
        audio->fading_fade = 1.0f - t;    // old track fades out 1→0

        // Apply volumes
        if (audio->music_loaded) {
            SetMusicVolume(audio->current_music, audio->music_volume * audio->current_fade);
        }
        if (audio->fading_loaded) {
            SetMusicVolume(audio->fading_music, audio->music_volume * audio->fading_fade);
        }

        if (t >= 1.0f) {
            finish_fade(audio);
        }
    }

    // Update music streams
    if (audio->music_loaded) {
        UpdateMusicStream(audio->current_music);

        // Section loop/end check
        if (audio->active_section >= 0 && audio->active_section < audio->section_count) {
            MusicSection *sec = &audio->sections[audio->active_section];
            float played = GetMusicTimePlayed(audio->current_music);
            if (played >= sec->end_time) {
                if (sec->loop) {
                    SeekMusicStream(audio->current_music, sec->start_time);
                } else {
                    audio->active_section = -1;
                }
            }
        }
    }

    if (audio->fading_loaded) {
        UpdateMusicStream(audio->fading_music);
    }
}

void audio_play_music(AudioManager *audio, const char *path, bool loop) {
    if (!audio || !path) return;

    // Dedup: skip if same track already playing
    if (audio->music_loaded && strcmp(audio->current_path, path) == 0) {
        return;
    }

    // If already fading, force-finish old fade
    if (audio->fading_active) {
        finish_fade(audio);
    }

    // Move current → fading slot
    if (audio->music_loaded) {
        audio->fading_music = audio->current_music;
        audio->fading_loaded = true;
        audio->fading_fade = 1.0f;
        audio->fading_active = true;
        audio->fade_timer = 0.0f;
        audio->music_loaded = false;
    }

    // Load new track
    audio->current_music = LoadMusicStream(path);
    if (audio->current_music.frameCount == 0) {
        printf("[audio] WARNING: Could not load music: %s\n", path);
        // If fading was set up, let old track keep playing
        if (audio->fading_active) {
            audio->current_music = audio->fading_music;
            audio->music_loaded = audio->fading_loaded;
            audio->current_fade = audio->fading_fade;
            audio->fading_loaded = false;
            audio->fading_active = false;
        }
        return;
    }

    audio->music_loaded = true;
    audio->current_music.looping = loop;
    snprintf(audio->current_path, sizeof(audio->current_path), "%s", path);

    // Clear sections (they're track-specific)
    audio_clear_sections(audio);

    // Start new track at volume 0 if crossfading, full if not
    if (audio->fading_active) {
        audio->current_fade = 0.0f;
        SetMusicVolume(audio->current_music, 0.0f);
    } else {
        audio->current_fade = 1.0f;
        SetMusicVolume(audio->current_music, audio->music_volume);
    }
    PlayMusicStream(audio->current_music);
    printf("[audio] Crossfading to: %s (loop=%d)\n", path, loop);
}

void audio_stop_music(AudioManager *audio) {
    if (!audio) return;
    if (audio->fading_loaded) {
        StopMusicStream(audio->fading_music);
        UnloadMusicStream(audio->fading_music);
        audio->fading_loaded = false;
        audio->fading_active = false;
    }
    if (audio->music_loaded) {
        StopMusicStream(audio->current_music);
        UnloadMusicStream(audio->current_music);
        audio->music_loaded = false;
    }
    audio->current_path[0] = '\0';
    audio_clear_sections(audio);
}

void audio_set_music_volume(AudioManager *audio, float volume) {
    if (!audio) return;
    audio->music_volume = volume;
    if (audio->music_loaded) {
        SetMusicVolume(audio->current_music, volume * audio->current_fade);
    }
    if (audio->fading_loaded) {
        SetMusicVolume(audio->fading_music, volume * audio->fading_fade);
    }
}

// ---------- Sections ----------

void audio_define_sections(AudioManager *audio, const MusicSection *sections, int count) {
    if (!audio || !sections || count <= 0) return;
    if (count > MAX_MUSIC_SECTIONS) count = MAX_MUSIC_SECTIONS;
    memcpy(audio->sections, sections, count * sizeof(MusicSection));
    audio->section_count = count;
    audio->active_section = -1;
}

void audio_clear_sections(AudioManager *audio) {
    if (!audio) return;
    audio->section_count = 0;
    audio->active_section = -1;
}

void audio_play_section(AudioManager *audio, const char *section_name) {
    if (!audio || !section_name || !audio->music_loaded) return;
    for (int i = 0; i < audio->section_count; i++) {
        if (strcmp(audio->sections[i].name, section_name) == 0) {
            audio->active_section = i;
            SeekMusicStream(audio->current_music, audio->sections[i].start_time);
            printf("[audio] Playing section: %s (%.1f-%.1f, loop=%d)\n",
                   section_name, audio->sections[i].start_time,
                   audio->sections[i].end_time, audio->sections[i].loop);
            return;
        }
    }
    printf("[audio] WARNING: Section not found: %s\n", section_name);
}

// ---------- Event Bus Integration ----------

// Battle section table (placeholder timestamps — tune to actual battle_bgm.mp3)
static const MusicSection battle_sections[] = {
    { "intro",       0.0f,   5.0f,  false },
    { "battle_loop", 5.0f,  45.0f,  true  },
    { "attack_hit", 45.0f,  48.0f,  false },
    { "victory",    48.0f,  58.0f,  false },
    { "defeat",     58.0f,  68.0f,  false },
};
#define BATTLE_SECTION_COUNT (sizeof(battle_sections) / sizeof(battle_sections[0]))

static const char *scene_bgm_path(int scene_id) {
    switch (scene_id) {
        case SCENE_OVERWORLD: return "../assets/overworld_bgm.mp3";
        case SCENE_BATTLE:    return "../assets/battle_bgm.mp3";
        default:              return NULL;
    }
}

static void on_scene_enter(Event event, void *userdata) {
    AudioManager *audio = (AudioManager *)userdata;
    int scene_id = event.entity_id;
    const char *path = scene_bgm_path(scene_id);
    if (path) {
        bool loop = (scene_id != SCENE_BATTLE); // battle uses sections
        audio_play_music(audio, path, loop);

        // Set up battle sections
        if (scene_id == SCENE_BATTLE) {
            audio_define_sections(audio, battle_sections, BATTLE_SECTION_COUNT);
            audio_play_section(audio, "intro");
        }
    }
}

static const char *phase_to_section(int phase) {
    switch (phase) {
        case PHASE_INTRO:              return "intro";
        case PHASE_PLAYER_MENU:        return "battle_loop";
        case PHASE_PLAYER_ATTACK_ANIM: return "attack_hit";
        case PHASE_WIN:                return "victory";
        case PHASE_LOSE:               return "defeat";
        default:                       return NULL;
    }
}

static void on_battle_phase_change(Event event, void *userdata) {
    AudioManager *audio = (AudioManager *)userdata;
    int new_phase = event.entity_id;
    const char *section = phase_to_section(new_phase);
    if (section) {
        audio_play_section(audio, section);
    }
}

void audio_subscribe_events(AudioManager *audio, EventBus *bus) {
    if (!audio || !bus) return;
    event_subscribe(bus, EVT_SCENE_ENTER, on_scene_enter, audio);
    event_subscribe(bus, EVT_BATTLE_PHASE_CHANGE, on_battle_phase_change, audio);
}
