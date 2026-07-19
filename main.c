// main.c — NightFall (climb fixes: correct bounds, wider probes, thicker wood cap, climb-only sprite)

#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>

// -------------------- Global platform/floor textures --------------------
Texture2D platform1_texture = {0};
Texture2D platform2_texture = {0};
Texture2D floor_piece_texture = {0};

// -------------------- Wood platform config --------------------
#define WOOD_PLATFORM_WIDTH 120
#define WOOD_PLATFORM_HEIGHT (300*2)
#define WOOD_PLATFORM_COUNT 5

// -------------------- Level textures reload --------------------
static void reload_platform_and_floor_textures(int level) {
    if (platform1_texture.id) UnloadTexture(platform1_texture);
    if (platform2_texture.id) UnloadTexture(platform2_texture);
    if (floor_piece_texture.id) UnloadTexture(floor_piece_texture);

    Image platform1_image, platform2_image, floor_image;
    if (level == 1) {
        platform1_image = LoadImage("Resources/platform1.png");
        platform1_texture = LoadTextureFromImage(platform1_image); UnloadImage(platform1_image);
        platform2_image = LoadImage("Resources/platform2.png");
        platform2_texture = LoadTextureFromImage(platform2_image); UnloadImage(platform2_image);
        floor_image = LoadImage("Resources/floor.png");
        floor_piece_texture = LoadTextureFromImage(floor_image); UnloadImage(floor_image);
    } else { // level 2
        platform1_image = LoadImage("Resources/platform1cp.png");
        platform1_texture = LoadTextureFromImage(platform1_image); UnloadImage(platform1_image);
        platform2_image = LoadImage("Resources/platform2cp.png");
        platform2_texture = LoadTextureFromImage(platform2_image); UnloadImage(platform2_image);
        floor_image = LoadImage("Resources/floorcp.png");
        floor_piece_texture = LoadTextureFromImage(floor_image); UnloadImage(floor_image);
    }
}

// -------------------- Tunables --------------------
#define SCALE_FACTOR 1.6f
#define BULLET_SPEED 900
#define MAX_BULLETS 40
#define MAX_HEARTS 10

// Level pacing
#define MONSTER_DELAY_TIME 10.0f
#define STONE_DELAY_TIME   5.0f
#define LASER_DELAY_TIME   5.0f

// Level finish
#define LEVEL_FINISH_EPS 2

// Backgrounds
#define BG_LVL1 "Resources/background_lvl1_jungle.png"
#define BG_LVL2 "Resources/background_lvl2_cyberpunk.png"

// Splash images
#define IMG_MENU_START    "Resources/menu_start_jungle.png"
#define IMG_LVL1_COMPLETE "Resources/level1_complete.png"
#define IMG_LVL2_START    "Resources/level2_start_cyberpunk.png"
#define IMG_GAME_END      "Resources/game_end.png"

// -------------------- Bullets --------------------
typedef struct {
    Vector2 position;
    int size;
    int direction;
} Bullet;

static void bullet_draw(Bullet *bullet) {
    if (bullet->size <= 0) return;
    DrawCircleV(bullet->position, bullet->size, YELLOW);
}

typedef struct {
    Texture2D stand_right, stand_left;
    Texture2D jump_right,  jump_left;
    Texture2D walk1_right, walk1_left;
    Texture2D walk2_right, walk2_left;
    Texture2D climb_right, climb_left;
    Vector2 *gun_position; // optional
} CharacterImage;

static void character_image_load(
    CharacterImage *ci,
    const char *standing_image_path,
    const char *jumping_image_path,
    const char *walking_image_path1,
    const char *walking_image_path2,
    Vector2 *gun_position
) {
    Image img = LoadImage(standing_image_path);
    ci->stand_right = LoadTextureFromImage(img);
    ImageFlipHorizontal(&img);
    ci->stand_left = LoadTextureFromImage(img);
    UnloadImage(img);

    img = LoadImage(jumping_image_path);
    ci->jump_right = LoadTextureFromImage(img);
    ImageFlipHorizontal(&img);
    ci->jump_left = LoadTextureFromImage(img);
    UnloadImage(img);

    img = LoadImage(walking_image_path1);
    ci->walk1_right = LoadTextureFromImage(img);
    ImageFlipHorizontal(&img);
    ci->walk1_left = LoadTextureFromImage(img);
    UnloadImage(img);

    img = LoadImage(walking_image_path2);
    ci->walk2_right = LoadTextureFromImage(img);
    ImageFlipHorizontal(&img);
    ci->walk2_left = LoadTextureFromImage(img);
    UnloadImage(img);

    ci->climb_left.id = 0;
    ci->climb_right.id = 0;
    ci->gun_position = gun_position;
}

static void character_image_load_with_climbing(
    CharacterImage *ci,
    const char *standing_image_path,
    const char *jumping_image_path,
    const char *walking_image_path1,
    const char *walking_image_path2,
    const char *climbing_image_path,
    Vector2 *gun_position
) {
    character_image_load(ci, standing_image_path, jumping_image_path, walking_image_path1, walking_image_path2, gun_position);
    Image climb = LoadImage(climbing_image_path);
    ci->climb_right = LoadTextureFromImage(climb);
    ImageFlipHorizontal(&climb);
    ci->climb_left = LoadTextureFromImage(climb);
    UnloadImage(climb);
}

typedef struct {
    Bullet bullets[MAX_BULLETS];
    int index;
} Bullets;

typedef struct {
    int hearts;
    float invincible_timer;
    bool is_invincible;
} PlayerHealth;

typedef struct {
    int x, y, width, height;
    int velocity;           // vertical velocity (px/s)
    int direction;          // -1 left, +1 right
    int speed, base_speed;
    int jump_strength, base_jump_strength;
    bool jumping, walking, climbing;
    int  climb_speed;
    CharacterImage image;
    int  jump_key;
    bool should_reappear;
    Bullets *bullets;
    PlayerHealth *health; // NULL for enemies
    bool is_active;

    // Climb state
    bool canClimb;
    bool facingRight;
} Character;

static void draw_hearts(PlayerHealth *health, int window_width) {
    (void)window_width;
    int heart_size = 30, heart_spacing = 40, start_x = 20, start_y = 20;
    for (int i = 0; i < MAX_HEARTS; i++) {
        Color c = (i < health->hearts) ? RED : GRAY;
        Vector2 pos = {start_x + i * heart_spacing + heart_size/2.0f, start_y + heart_size/2.0f};
        DrawCircle(pos.x - 8, pos.y - 5, 10, c);
        DrawCircle(pos.x + 8, pos.y - 5, 10, c);
        DrawTriangle((Vector2){pos.x - 15, pos.y + 2}, (Vector2){pos.x + 15, pos.y + 2}, (Vector2){pos.x, pos.y + 18}, c);
    }
}

static void take_damage_ex(PlayerHealth *health, float duration) {
    if (!health->is_invincible && health->hearts > 0) {
        health->hearts--;
        health->is_invincible = true;
        health->invincible_timer = duration;
        printf("Character took damage! Hearts remaining: %d\n", health->hearts);
    }
}

static void take_damage(PlayerHealth *health) {
    take_damage_ex(health, 2.0f);
}

static void update_player_health(PlayerHealth *health, float dt) {
    if (health->is_invincible) {
        health->invincible_timer -= dt;
        if (health->invincible_timer <= 0) {
            health->is_invincible = false;
            printf("Character is no longer invincible\n");
        }
    }
}

static void character_shoot(Character *ch) {
    if (!ch->image.gun_position) return;
    int gx = (int)ch->image.gun_position->x;
    if (ch->direction == -1) gx = ch->width - gx;
    Bullet b = { .position = (Vector2){ch->x + gx, ch->y + ch->image.gun_position->y}, .direction = ch->direction, .size = 15 };
    ch->bullets->bullets[ch->bullets->index] = b;
    ch->bullets->index = (ch->bullets->index + 1) % MAX_BULLETS;
}

static void character_destroy(Character *c) {
    UnloadTexture(c->image.stand_right);
    UnloadTexture(c->image.stand_left);
    UnloadTexture(c->image.jump_right);
    UnloadTexture(c->image.jump_left);
    UnloadTexture(c->image.walk1_right);
    UnloadTexture(c->image.walk1_left);
    UnloadTexture(c->image.walk2_right);
    UnloadTexture(c->image.walk2_left);
    if (c->image.climb_right.id) UnloadTexture(c->image.climb_right);
    if (c->image.climb_left.id) UnloadTexture(c->image.climb_left);
}

typedef enum { PLATFORM, FLOOR, CLIMBABLE_WOOD } PlatformType;

typedef struct {
    int x, y, width, height;
    PlatformType type;
} Platform;

static float rand_float(void) { return (float)rand() / (float)RAND_MAX; }

// Foot/landing probe (make wood top cap thicker = easier to land)
static int character_on_platform(Character character, Platform *platforms, int count) {
    for (int i = 0; i < count; i++) {
        Rectangle platform_rec;
        if (platforms[i].type == CLIMBABLE_WOOD) {
            platform_rec = (Rectangle){ (float)platforms[i].x, (float)platforms[i].y - 12.0f,
                                        (float)platforms[i].width, 12.0f };
        } else {
            platform_rec = (Rectangle){ (float)platforms[i].x, (float)platforms[i].y,
                                        (float)platforms[i].width, (float)platforms[i].height };
        }

        Rectangle feet_rec = {
            .x = (float)(character.x + 10),
            .y = (float)(character.y + character.height - character.height * 0.2f),
            .width = (float)(character.width - 15),
            .height = (float)(character.height * 0.2f + 1),
        };

        if (CheckCollisionRecs(feet_rec, platform_rec)) return i;
    }
    return -1;
}

// Side-probe climb detector (wider probes; no x snapping)
static int wood_can_climb_probe(Character *ch, Platform *plats, int count, bool *climbOnRightSide) {
    Rectangle checkRight = (Rectangle){ ch->x + ch->width - 8, ch->y + 6, 22, ch->height - 12 };
    Rectangle checkLeft  = (Rectangle){ ch->x - 14,             ch->y + 6, 22, ch->height - 12 };
    for (int i = 0; i < count; i++) {
        if (plats[i].type != CLIMBABLE_WOOD) continue;
        Rectangle wood = (Rectangle){ (float)plats[i].x, (float)plats[i].y, (float)plats[i].width, (float)plats[i].height };
        float playerBottom = ch->y + ch->height;
        float woodTop = wood.y;
        if (CheckCollisionRecs(checkRight, wood) && playerBottom > woodTop + 10) {
            *climbOnRightSide = true;
            return i;
        }
        if (CheckCollisionRecs(checkLeft, wood) && playerBottom > woodTop + 10) {
            *climbOnRightSide = false;
            return i;
        }
    }
    return -1;
}

/* Rolling Stone */
typedef struct {
    Texture2D texture;
    float x, y, width, height, speed, rotation_deg;
    bool should_reappear, is_active;
} RollingStone;

static void stone_spawn_at_right(RollingStone *s, float camera_offset_x, int window_width, int ground_y) {
    s->x = -camera_offset_x + window_width + 200.0f;
    s->y = (float)ground_y - s->height + 100;
    s->rotation_deg = 0.0f;
    s->is_active = true;
}
static void stone_move_and_draw(RollingStone *s, float dt) {
    s->x -= s->speed * dt;
    float circumference = (s->width > 1 ? s->width : (float)s->texture.width);
    float rot_per_px = -360.0f / circumference;
    s->rotation_deg += s->speed * dt * rot_per_px;
    Rectangle src = {0,0,(float)s->texture.width,(float)s->texture.height};
    Rectangle dst = {s->x + s->width/2.0f, s->y + s->height/2.0f, s->width, s->height};
    Vector2 origin = {s->width/2.0f, s->height/2.0f};
    DrawTexturePro(s->texture, src, dst, origin, s->rotation_deg, WHITE);
}
static Rectangle stone_hitbox(RollingStone *s) {
    float pad = fminf(s->width, s->height) * 0.12f;
    return (Rectangle){ s->x + pad, s->y + pad, s->width - 2*pad, s->height - 2*pad };
}

/* Laser Wall */
typedef struct {
    Texture2D texture;
    float x, y, width, height, speed, rotation_deg;
    bool should_reappear, is_active;
} Laser;

static void laser_spawn_at_right(Laser *s, float camera_offset_x, int window_width, int ground_y) {
    s->x = -camera_offset_x + window_width + 200.0f;
    s->y = (float)ground_y - s->height + 100;
    s->is_active = true;
}
static void laser_move_and_draw(Laser *s, float dt) {
    s->x -= s->speed * dt;
    Rectangle src = {0,0,(float)s->texture.width,(float)s->texture.height};
    Rectangle dst = {s->x + s->width/2.0f, s->y + s->height/2.0f, s->width, s->height};
    Vector2 origin = {s->width/2.0f, s->height/2.0f};
    DrawTexturePro(s->texture, src, dst, origin, 0.0f, WHITE);
}
static Rectangle laser_hitbox(Laser *s) {
    float pad = fminf(s->width, s->height) * 0.12f;
    return (Rectangle){ s->x + pad, s->y + pad, s->width - 2*pad, s->height - 2*pad };
}

/* Level/State helpers */
typedef enum {
    STATE_MENU = 0, STATE_LEVEL1, STATE_LEVEL1_COMPLETE,
    STATE_LEVEL2_INTRO, STATE_LEVEL2, STATE_GAME_END
} GameState;

static void clear_bullets(Bullets *b) {
    if (!b) return;
    for (int i = 0; i < MAX_BULLETS; i++) b->bullets[i].size = 0;
    b->index = 0;
}

static void reload_background(Texture2D *bg, int levelId) {
    if (bg->id > 0) UnloadTexture(*bg);
    *bg = LoadTexture((levelId == 1) ? BG_LVL1 : BG_LVL2);
}

static void reset_for_level(
    int levelId,
    Texture2D *background_texture,
    Camera2D *camera,
    Character *player,
    Character *monster,
    RollingStone *stone,
    Laser *laser,
    PlayerHealth *health,
    Bullets *player_bullets,
    Bullets *monster_bullets,
    float *game_time,
    bool *game_over,
    int window_width,
    int window_height
) {
    reload_background(background_texture, levelId);
    camera->offset = (Vector2){0,0};
    camera->target = (Vector2){0,0};
    *game_time = 0.0f; *game_over = false;

    player->x = window_width / 2; player->y = window_height / 2;
    player->velocity = (int)(200.0f * SCALE_FACTOR);
    player->speed = player->base_speed;
    player->walking = player->jumping = player->climbing = false;
    player->canClimb = false; player->facingRight = true;
    player->climb_speed = 150; player->direction = 1; player->is_active = true;

    health->hearts = MAX_HEARTS; health->invincible_timer = 0.0f; health->is_invincible = false;

    monster->x = window_width + 500; monster->y = window_height / 2;
    monster->velocity = (int)(200.0f * SCALE_FACTOR);
    monster->walking = true; monster->jumping = false; monster->direction = -1; monster->is_active = false;
    if (monster->health) {
        monster->health->hearts = 15;
        monster->health->invincible_timer = 0.0f;
        monster->health->is_invincible = false;
    }

    if (levelId == 1) {
        if (stone->texture.id) UnloadTexture(stone->texture);
        stone->texture = LoadTexture("Resources/stone_rolling.png");
        stone->width = 360.0f; stone->height = 360.0f; stone->speed = 450.0f;
        stone->rotation_deg = -0.2f; stone->should_reappear = true; stone->is_active = false;
        if (laser->texture.id) UnloadTexture(laser->texture);
        laser->is_active = false; laser->rotation_deg = 0.0f;
    } else { // level 2
        if (laser->texture.id) UnloadTexture(laser->texture);
        laser->texture = LoadTexture("Resources/laser.png");
        laser->width = 60.0f; laser->height = 360.0f; laser->speed = 450.0f;
        laser->rotation_deg = -0.2f; laser->should_reappear = true; laser->is_active = false;
        if (stone->texture.id) UnloadTexture(stone->texture);
        stone->is_active = false; stone->rotation_deg = 0.0f;
    }

    clear_bullets(player_bullets); clear_bullets(monster_bullets);
}

static void draw_fullscreen(Texture2D tex, int w, int h) {
    if (tex.id == 0) return;
    Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
    Rectangle dst = {0, 0, (float)w, (float)h};
    DrawTexturePro(tex, src, dst, (Vector2){0,0}, 0.0f, WHITE);
}

int main(void) {
    srand((unsigned int)time(NULL));
    const int window_width = 1920, window_height = 1080;

    InitWindow(window_width, window_height, "NightFall");
    SetTargetFPS(60);

    GameState state = STATE_MENU;
    int LevelID = 1;
    bool game_over = false;
    float game_time = 0.0f;

    PlayerHealth player_health = { .hearts = MAX_HEARTS, .invincible_timer = 0.0f, .is_invincible = false };
    PlayerHealth monster_health = { .hearts = 15, .invincible_timer = 0.0f, .is_invincible = false };

    CharacterImage character_image = {0};
    Vector2 player_gun_position = {.x = 80, .y = 110};
    character_image_load_with_climbing(&character_image,
        "Resources/standing.png", "Resources/jumping.png",
        "Resources/walk1.png", "Resources/walk1.png",
        "Resources/climbing.png", &player_gun_position);

    CharacterImage monster_image = {0};
    Vector2 monster_gun_position = {.x = 400, .y = 170};
    character_image_load(&monster_image,
        "Resources/monster_right_1.png", "Resources/monster_right_1.png",
        "Resources/monster_right_1.png", "Resources/monster_right_2.png",
        &monster_gun_position);

    Bullets character_bullets = {0}, monster_bullets = {0};

    Character character = {
        .x = window_width / 2, .y = window_height / 2,
        .width = 101, .height = 240,
        .velocity = (int)(200.0f * SCALE_FACTOR),
        .speed = 250, .base_speed = 250,
        .jump_strength = 1000, .base_jump_strength = 1000,
        .jumping = false, .walking = false, .climbing = false, .climb_speed = 150,
        .image = character_image, .jump_key = KEY_SPACE, .direction = 1,
        .should_reappear = false, .bullets = &character_bullets, .health = &player_health,
        .is_active = true, .canClimb = false, .facingRight = true
    };

    int monster_width = 406;
    Character monster = {
        .x = window_width + 500, .y = window_height / 2,
        .width = monster_width, .height = 339,
        .velocity = (int)(200.0f * SCALE_FACTOR),
        .speed = 200, .base_speed = 200,
        .jump_strength = 1000, .base_jump_strength = 1000,
        .jumping = false, .walking = true, .climb_speed = 0, .climbing = false,
        .image = monster_image, .jump_key = 0, .direction = -1,
        .should_reappear = true, .bullets = &monster_bullets, .health = &monster_health,
        .is_active = false, .canClimb = false, .facingRight = false
    };

    float monster_shoot_cooldown = 0.0f, monster_shoot_interval = 2.0f;
    int gravity = (int)(3000.0f * SCALE_FACTOR);

    Camera2D camera = { .offset = (Vector2){0,0}, .target = (Vector2){0,0}, .rotation = 0, .zoom = 1 };

    // World/platforms
    float platform_spacing = 0.06f;
    int world_width = window_width * 10;
    int platform_width, platform_height;
    int platform_count;
    int floor_piece_width, floor_piece_height;
    int floor_piece_count;
    int floor_whitespace;
    int platform_min_y;
    int platform_max_y;

    if (LevelID == 1) {
        platform_width = 180, platform_height = 50;
        platform_count = world_width / (int)(platform_width + platform_spacing * window_width);
        floor_piece_width = 490, floor_piece_height = 190;
        floor_piece_count = (int)ceilf((float)world_width / (float)floor_piece_width);
        floor_whitespace = 33;
        platform_min_y = (int)(window_height * 0.2f);
        platform_max_y = window_height - floor_piece_height - platform_height - platform_min_y;
    } else {
        platform_width = 180, platform_height = 50;
        platform_count = world_width / (int)(platform_width + platform_spacing * window_width);
        floor_piece_width = 490, floor_piece_height = 190;
        floor_piece_count = (int)ceilf((float)world_width / (float)floor_piece_width);
        floor_whitespace = 20;
        platform_min_y = (int)(window_height * 0.3f);
        platform_max_y = window_height - floor_piece_height - platform_height - platform_min_y;
    }

    int platform1_whitespace = 45, platform2_whitespace = 20;

    platform_count += floor_piece_count;

    Platform *platforms = (Platform*)malloc(sizeof(Platform) * (platform_count + WOOD_PLATFORM_COUNT + 2));
    if (!platforms) { CloseWindow(); return 1; }

    // Textures
    Texture2D background_texture;
    Texture2D tex_menu_start;
    Texture2D tex_lvl1_complete;
    Texture2D tex_lvl2_start;
    Texture2D tex_game_end;
    Texture2D wood_texture;

    if (LevelID == 1) {
        background_texture = LoadTexture(BG_LVL1);
    } else {
        background_texture = LoadTexture(BG_LVL2);
    }
    tex_menu_start    = LoadTexture(IMG_MENU_START);
    tex_lvl1_complete = LoadTexture(IMG_LVL1_COMPLETE);
    tex_lvl2_start    = LoadTexture(IMG_LVL2_START);
    tex_game_end      = LoadTexture(IMG_GAME_END);
    wood_texture      = LoadTexture("Resources/wood.png");
    reload_platform_and_floor_textures(LevelID);

    // Build floor
    int i = 0, floor_x = 0;
    for (; i < floor_piece_count; i++) {
        platforms[i] = (Platform){ .x = floor_x, .y = window_height - floor_piece_height + floor_whitespace,
                                   .width = floor_piece_width, .height = floor_piece_height, .type = FLOOR };
        floor_x += platforms[i].width;
    }

    // Build normal platforms
    int platform_x = (int)(window_width * 0.1f);
    for (; i < platform_count; i++) {
        platforms[i].x = platform_x;
        platforms[i].y = (int)(rand_float() * platform_max_y + platform_min_y);
        platforms[i].width = platform_width; platforms[i].height = platform_height;
        platforms[i].type = PLATFORM;
        platform_x += platforms[i].width + (int)(window_width * platform_spacing);
    }

    // Build wood platforms for climbing
    int wood_platform_spacing = world_width / (WOOD_PLATFORM_COUNT + 1);
    for (int w = 0; w < WOOD_PLATFORM_COUNT; w++) {
        platforms[i] = (Platform){
            .x = wood_platform_spacing * (w + 1) - WOOD_PLATFORM_WIDTH/2,
            .y = (int)(rand_float() * (platform_max_y - WOOD_PLATFORM_HEIGHT) + platform_min_y) - 20,
            .width = WOOD_PLATFORM_WIDTH,
            .height = WOOD_PLATFORM_HEIGHT,
            .type = CLIMBABLE_WOOD
        };
        i++;
    }
    platform_count += WOOD_PLATFORM_COUNT;

    RollingStone stone = (RollingStone){0};
    Laser        laser = (Laser){0};
    if (LevelID == 1) {
        stone.texture = LoadTexture("Resources/stone_rolling.png");
        stone.width = 360.0f; stone.height = 360.0f; stone.speed = 450.0f;
        stone.rotation_deg = -0.2f; stone.should_reappear = true; stone.is_active = false;
    } else {
        laser.texture = LoadTexture("Resources/laser.png");
        laser.width = 60.0f; laser.height = 360.0f; laser.speed = 450.0f;
        laser.rotation_deg = -0.2f; laser.should_reappear = true; laser.is_active = false;
    }

    while (!WindowShouldClose()) {
        BeginDrawing();

        if (state == STATE_MENU) {
            ClearBackground(BLACK);
            draw_fullscreen(tex_menu_start, window_width, window_height);
            DrawText("Press SPACE to Start", 40, window_height - 80, 30, RAYWHITE);
            if (IsKeyPressed(KEY_SPACE)) {
                LevelID = 1;
                reload_platform_and_floor_textures(LevelID);
                reset_for_level(LevelID, &background_texture, &camera, &character, &monster, &stone, &laser,
                                &player_health, &character_bullets, &monster_bullets,
                                &game_time, &game_over, window_width, window_height);
                monster_shoot_cooldown = 0.0f;
                state = STATE_LEVEL1;
            }
            EndDrawing(); continue;
        }

        if (state == STATE_LEVEL1_COMPLETE) {
            ClearBackground(DARKGREEN);
            draw_fullscreen(tex_lvl1_complete, window_width, window_height);
            DrawText("Press SPACE", 40, window_height - 80, 30, WHITE);
            if (IsKeyPressed(KEY_SPACE)) state = STATE_LEVEL2_INTRO;
            EndDrawing(); continue;
        }

        if (state == STATE_LEVEL2_INTRO) {
            ClearBackground(BLACK);
            draw_fullscreen(tex_lvl2_start, window_width, window_height);
            DrawText("Press SPACE to Begin", 40, window_height - 80, 30, RAYWHITE);
            if (IsKeyPressed(KEY_SPACE)) {
                LevelID = 2;
                reload_platform_and_floor_textures(LevelID);
                reset_for_level(LevelID, &background_texture, &camera, &character, &monster, &stone, &laser,
                                &player_health, &character_bullets, &monster_bullets,
                                &game_time, &game_over, window_width, window_height);
                monster_shoot_cooldown = 0.0f;
                state = STATE_LEVEL2;
            }
            EndDrawing(); continue;
        }

        if (state == STATE_GAME_END) {
            ClearBackground((Color){30,30,60,255});
            draw_fullscreen(tex_game_end, window_width, window_height);
            DrawText("Press R to return to Menu or ESC to exit.", 40, window_height - 80, 24, LIGHTGRAY);
            if (IsKeyPressed(KEY_R)) state = STATE_MENU;
            EndDrawing(); continue;
        }

        if (game_over) {
            ClearBackground(RED);
            DrawText("Game Over", window_width/2 - 100, window_height/2 - 50, 40, WHITE);
            DrawText("Press ESC to exit", window_width/2 - 120, window_height/2, 20, WHITE);
            EndDrawing(); continue;
        }

        float dt = GetFrameTime();
        game_time += dt;

        // Probe for climb possibility & facing
        bool climbOnRight = true;
        int climbIdx = wood_can_climb_probe(&character, platforms, platform_count, &climbOnRight);
        character.canClimb = (climbIdx != -1);
        if (character.canClimb) character.facingRight = climbOnRight;

        // Enter/exit climbing via input (Arrow keys or W/S)
        bool up = IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);
        bool down = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);

        if (character.canClimb && (up || down)) {
            character.climbing = true;
            character.walking = false;
            character.jumping = false;
        }
        if (!character.canClimb || (!up && !down)) {
            character.climbing = false;
        }

        // Hazards pause ONLY while actually climbing
        bool hazards_paused = character.climbing;

        // Delayed activation (blocked when paused)
        if (!hazards_paused && game_time >= MONSTER_DELAY_TIME && !monster.is_active) {
            monster.is_active = true;
            printf("Monster has appeared!\n");
        }
        if (!hazards_paused && LevelID == 1 && game_time >= STONE_DELAY_TIME && !stone.is_active) {
            int ground_y = window_height - floor_piece_height + floor_whitespace;
            stone_spawn_at_right(&stone, camera.offset.x, window_width, ground_y);
            printf("Rolling stone has appeared!\n");
        } else if (!hazards_paused && LevelID == 2 && game_time >= LASER_DELAY_TIME && !laser.is_active) {
            int ground_y = window_height - floor_piece_height + floor_whitespace;
            laser_spawn_at_right(&laser, camera.offset.x, window_width, ground_y);
            printf("Laser wall has appeared!\n");
        }

        // Health update
        update_player_health(&player_health, dt);
        if (monster.is_active && monster.health) {
            update_player_health(monster.health, dt);
        }
        if (monster.is_active && !hazards_paused) monster_shoot_cooldown -= dt;

        if (player_health.hearts <= 0) game_over = true;

        // Camera follow (x only)
        if (character.x > (int)(window_width * 0.1f)) camera.offset.x = -(character.x - (int)(window_width * 0.1f));
        else if (character.x < (int)(window_width * 0.05f)) camera.offset.x = -(character.x - (int)(window_width * 0.05f));
        if (camera.offset.x > 0) camera.offset.x = 0;
        if (camera.offset.x < -(world_width - window_width)) camera.offset.x = -(world_width - window_width);

        BeginMode2D(camera);

        // Background tiling with parallax scrolling (50% speed)
        float background_scale = (float)window_height / (float)background_texture.height;
        int scaled_bg_w = (int)(background_texture.width * background_scale);
        int draw_offset = (int)(-camera.offset.x * 0.5f);
        int cam_left = draw_offset, cam_right = draw_offset + window_width;
        int first_tile = cam_left / scaled_bg_w, last_tile = (cam_right / scaled_bg_w) + 1;
        for (int t = first_tile - 1; t <= last_tile; t++) {
            int bgx = t * scaled_bg_w;
            Rectangle src = {0,0,(float)background_texture.width,(float)background_texture.height};
            Rectangle dst = {(float)bgx - camera.offset.x * 0.5f, 0, (float)scaled_bg_w, (float)window_height};
            DrawTexturePro(background_texture, src, dst, (Vector2){0,0}, 0.0f, WHITE);
        }

        // Controls (LEFT/RIGHT disabled while actively climbing)
        bool shift_pressed = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        character.speed = character.base_speed;

        if (!character.climbing) {
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
                character.walking = true; character.direction = -1;
                if (shift_pressed) character.speed = character.base_speed * 2;
            } else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
                character.walking = true; character.direction = 1;
                if (shift_pressed) character.speed = character.base_speed * 2;
            } else {
                character.walking = false;
            }
        } else {
            character.walking = false;
        }

        // Player shooting controls (F key or Left Mouse Click)
        if (IsKeyPressed(KEY_F) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            character_shoot(&character);
        }

        // Update characters (physics)
        Character *chars[] = {&character, &monster};
        int chars_count = 2;

        for (int c = 0; c < chars_count; c++) {
            if (!chars[c]->is_active) continue;

            if (c == 0 && character.climbing) {
                // Climbing physics: suppress gravity, Y controlled by input
                character.velocity = 0;
                int v = (int)(character.climb_speed * dt * SCALE_FACTOR);
                if (up)   character.y -= v;
                if (down) character.y += v;

                // Top/bottom exit rules relative to current wood
                if (climbIdx != -1) {
                    Platform *w = &platforms[climbIdx];
                    if (character.y <= w->y - character.height) {
                        character.y = w->y - character.height;
                        character.climbing = false;
                    }
                    if (character.y + character.height >= w->y + w->height) {
                        character.y = w->y + w->height - character.height + 1;
                        character.climbing = false;
                    }
                }
                // Optional wall-jump push-off
                if (IsKeyPressed(character.jump_key)) {
                    int away = character.facingRight ? -1 : 1;
                    character.y -= (int)(character.base_jump_strength * 0.85f * SCALE_FACTOR * 0.016f);
                    character.x += (int)(away * character.base_speed * 1.5f * dt * SCALE_FACTOR * 60.0f);
                    character.jumping = true;
                    character.climbing = false;
                }
            } else {
                if (chars[c]->walking) {
                    chars[c]->x += (int)(chars[c]->direction * chars[c]->speed * dt * SCALE_FACTOR);
                }
                float prev_y = (float)chars[c]->y;
                chars[c]->y += chars[c]->velocity * dt;
                chars[c]->velocity += gravity * dt;

                int pi = character_on_platform(*(chars[c]), platforms, platform_count);
                if (pi != -1 && chars[c]->velocity > 0) {
                    // land only if approaching from above
                    if (prev_y + chars[c]->height <= platforms[pi].y + 5) {
                        chars[c]->y = platforms[pi].y - chars[c]->height;
                        chars[c]->velocity = 0; chars[c]->jumping = false;
                    }
                }

                // Jump
                if (!(c == 0 && character.climbing)) {
                    if (chars[c]->jump_key && IsKeyPressed(chars[c]->jump_key)) {
                        if (c == 0 && shift_pressed)
                            chars[c]->velocity = -(int)((chars[c]->base_jump_strength * 1.5f) * SCALE_FACTOR);
                        else
                            chars[c]->velocity = -(int)(chars[c]->jump_strength * SCALE_FACTOR);
                        chars[c]->jumping = true;
                    }
                }
            }
        }

        ClearBackground(WHITE);

        // Draw platforms & floor
        for (int p = 0; p < platform_count; p++) {
            if (platforms[p].type == FLOOR) {
                DrawTexture(floor_piece_texture, platforms[p].x, platforms[p].y - floor_whitespace, WHITE);
            } else if (platforms[p].type == CLIMBABLE_WOOD) {
                if (LevelID == 1) {
                    Rectangle src = {0,0,(float)wood_texture.width,(float)wood_texture.height};
                    Rectangle dst = {(float)platforms[p].x, (float)platforms[p].y,
                                     (float)platforms[p].width, (float)platforms[p].height};
                    DrawTexturePro(wood_texture, src, dst, (Vector2){0,0}, 0.0f, WHITE);
                } else {
                    // Level 2: Neon cyberpunk ladder drawn programmatically
                    // Left and right rails
                    DrawRectangle(platforms[p].x, platforms[p].y, 8, platforms[p].height, (Color){ 0, 255, 240, 255 });
                    DrawRectangle(platforms[p].x + platforms[p].width - 8, platforms[p].y, 8, platforms[p].height, (Color){ 0, 255, 240, 255 });
                    
                    // Neon rungs
                    for (int ry = platforms[p].y + 15; ry < platforms[p].y + platforms[p].height; ry += 35) {
                        DrawRectangle(platforms[p].x + 8, ry, platforms[p].width - 16, 6, (Color){ 0, 255, 240, 255 });
                    }
                }
            } else {
                Texture2D tex = (p % 2 == 0) ? platform2_texture : platform1_texture;
                int ws = (p % 2 == 0) ? platform2_whitespace : platform1_whitespace;
                DrawTexture(tex, platforms[p].x - 10, platforms[p].y - ws, WHITE);
            }
        }

        // Draw characters
        for (int c = 0; c < chars_count; c++) {
            if (!chars[c]->is_active) continue;

            Texture2D tex;
            if (c == 0 && character.climbing) {
                tex = (character.facingRight) ? character.image.climb_right : character.image.climb_left;
            } else if (chars[c]->jumping) {
                tex = (chars[c]->direction == -1) ? chars[c]->image.jump_left : chars[c]->image.jump_right;
            } else if (chars[c]->walking) {
                double t = GetTime() * 10 * (chars[c]->speed > chars[c]->base_speed ? 2.0 : 1.0);
                if (chars[c]->direction == -1)
                    tex = (((int)t) % 2 == 0) ? chars[c]->image.walk1_left : chars[c]->image.walk2_left;
                else
                    tex = (((int)t) % 2 == 0) ? chars[c]->image.walk1_right : chars[c]->image.walk2_right;
            } else {
                tex = (chars[c]->direction == -1) ? chars[c]->image.stand_left : chars[c]->image.stand_right;
            }

            if (chars[c]->should_reappear) {
                if (chars[c]->x < -camera.offset.x - 1200) chars[c]->x = (int)(-camera.offset.x + window_width);
            } else {
                if (chars[c]->x < 0) chars[c]->x = 0;
            }
            if (chars[c]->x > world_width - chars[c]->width) chars[c]->x = world_width - chars[c]->width;

            Color tint = WHITE;
            if (chars[c]->health && chars[c]->health->is_invincible) {
                double flash = GetTime() * 10;
                if (((int)flash) % 2 == 0) tint = (Color){255,255,255,128};
            }
            if (c == 0 && character.speed > character.base_speed && !character.climbing) tint = (Color){200,220,255,255};

            DrawTexture(tex, chars[c]->x, chars[c]->y, tint);
        }

        // Boss shooting (skipped while paused)
        if (monster.is_active && !hazards_paused && monster_shoot_cooldown <= 0.0f) {
            character_shoot(&monster);
            monster_shoot_cooldown = monster_shoot_interval;
        }

        // Boss collision with player (skipped while paused)
        if (monster.is_active && !hazards_paused) {
            Rectangle player_rec = { (float)(character.x + 20), (float)(character.y + 20),
                                     (float)(character.width - 40), (float)(character.height - 20) };
            Rectangle monster_rec = { (float)(monster.x + 50), (float)(monster.y + 30),
                                      (float)(monster.width - 100), (float)(monster.height - 30) };
            if (CheckCollisionRecs(player_rec, monster_rec)) take_damage(&player_health);
        }

        // Bullets (skipped while paused)
        if (!hazards_paused) {
            for (int j = 0; j < chars_count; j++) {
                if (!chars[j]->is_active) continue;
                for (int b = 0; b < MAX_BULLETS; b++) {
                    Bullet *blt = &chars[j]->bullets->bullets[b];
                    if (blt->size <= 0) continue;
                    blt->position.x += blt->direction * BULLET_SPEED * dt;
                    bullet_draw(blt);

                    if (j == 0) { // player bullets hitting monster
                        if (monster.is_active && monster.health) {
                            Rectangle monster_rec = { (float)(monster.x + 50), (float)(monster.y + 30),
                                                      (float)(monster.width - 100), (float)(monster.height - 30) };
                            if (CheckCollisionCircleRec(blt->position, (float)blt->size, monster_rec)) {
                                take_damage_ex(monster.health, 0.3f);
                                blt->size = 0;
                                if (monster.health->hearts <= 0) {
                                    monster.is_active = false;
                                    printf("Monster defeated!\n");
                                }
                            }
                        }
                    } else if (j == 1) { // monster bullets hitting player
                        Rectangle player_rec = { (float)(character.x + 20), (float)(character.y + 20),
                                                 (float)(character.width - 40), (float)(character.height - 20) };
                        if (CheckCollisionCircleRec(blt->position, (float)blt->size, player_rec)) {
                            take_damage(&player_health);
                            blt->size = 0;
                        }
                    }
                }
            }
        }

        // Rolling stone (skipped while paused)
        if (LevelID == 1) {
            if (stone.is_active && !hazards_paused) {
                stone_move_and_draw(&stone, dt);
                if (stone.should_reappear) {
                    if (stone.x + stone.width < (-camera.offset.x - 200.0f)) {
                        int ground_y = window_height - floor_piece_height + floor_whitespace;
                        stone_spawn_at_right(&stone, camera.offset.x, window_width, ground_y);
                    }
                }
                Rectangle player_rec = { (float)(character.x + 20), (float)(character.y + 20),
                                         (float)(character.width - 40), (float)(character.height - 20) };
                if (CheckCollisionRecs(stone_hitbox(&stone), player_rec)) take_damage(&player_health);
            }
        }

        // Laser wall (skipped while paused)
        if (LevelID == 2) {
            if (laser.is_active && !hazards_paused) {
                laser_move_and_draw(&laser, dt);
                if (laser.should_reappear) {
                    if (laser.x + laser.width < (-camera.offset.x - 200.0f)) {
                        int ground_y = window_height - floor_piece_height + floor_whitespace;
                        laser_spawn_at_right(&laser, camera.offset.x, window_width, ground_y);
                    }
                }
                Rectangle player_rec = { (float)(character.x + 20), (float)(character.y + 20),
                                         (float)(character.width - 40), (float)(character.height - 20) };
                if (CheckCollisionRecs(laser_hitbox(&laser), player_rec)) take_damage(&player_health);
            }
        }

        EndMode2D();

        // UI
        draw_hearts(&player_health, window_width);

        // Debug Coordinates
        char debug_coords[128];
        snprintf(debug_coords, sizeof(debug_coords), "Player: X=%d, Y=%d, Active=%d, Climbing=%d, Jumping=%d, Walking=%d", character.x, character.y, character.is_active, character.climbing, character.jumping, character.walking);
        DrawText(debug_coords, 20, 100, 20, BLACK);

        // Boss Health Bar
        if (monster.is_active && monster.health && monster.health->hearts > 0) {
            int bar_width = 500;
            int bar_height = 24;
            int bar_x = (window_width - bar_width) / 2;
            int bar_y = 30;
            
            // Draw health bar container (border/bg)
            DrawRectangle(bar_x - 4, bar_y - 4, bar_width + 8, bar_height + 8, DARKGRAY);
            DrawRectangle(bar_x, bar_y, bar_width, bar_height, BLACK);
            
            // Draw health bar fill (red)
            float health_pct = (float)monster.health->hearts / 15.0f;
            DrawRectangle(bar_x, bar_y, (int)(bar_width * health_pct), bar_height, RED);
            
            // Draw boss name label
            const char* boss_name = (LevelID == 1) ? "CORRUPTED JUNGLE BEAST" : "CYBERNETIC WRAITH";
            DrawText(boss_name, bar_x + 10, bar_y + 4, 16, RAYWHITE);
            
            // Draw numeric health overlay
            char hp_text[32];
            snprintf(hp_text, sizeof(hp_text), "%d / 15", monster.health->hearts);
            int text_w = MeasureText(hp_text, 16);
            DrawText(hp_text, bar_x + bar_width - text_w - 10, bar_y + 4, 16, RAYWHITE);
        }

        if (!hazards_paused && game_time < MONSTER_DELAY_TIME) {
            char buf[64]; snprintf(buf, sizeof(buf), "Boss appears in: %.1f", (MONSTER_DELAY_TIME - game_time));
            DrawText(buf, window_width - 320, 20, 20, RED);
        } else if (hazards_paused) {
            DrawText("Hazards Paused (Climbing)", window_width - 420, 20, 20, DARKGREEN);
        }
        if (!hazards_paused && (game_time < STONE_DELAY_TIME) && LevelID == 1) {
            char buf[64]; snprintf(buf, sizeof(buf), "Stone rolls in: %.1f", (STONE_DELAY_TIME - game_time));
            DrawText(buf, window_width - 320, 48, 20, DARKGRAY);
        } else if (!hazards_paused && (game_time < LASER_DELAY_TIME) && LevelID == 2) {
            char buf[64]; snprintf(buf, sizeof(buf), "Laser wall in: %.1f", (LASER_DELAY_TIME - game_time));
            DrawText(buf, window_width - 320, 48, 20, DARKGRAY);
        }

        DrawText("Controls:", 20, window_height - 120, 20, BLACK);
        DrawText("Shift+Left/Right = Double Speed", 20, window_height - 100, 16, DARKGRAY);
        DrawText("Shift+Space = Super Jump (1.5x height)", 20, window_height - 80, 16, DARKGRAY);
        DrawText("Left/Right = Normal Movement", 20, window_height - 60, 16, DARKGRAY);
        DrawText("UP/DOWN near wood = Climb (hazards pause)", 20, window_height - 40, 16, DARKGRAY);

        // Level completion checks
        if (state == STATE_LEVEL1) {
            if (character.x >= world_width - character.width - LEVEL_FINISH_EPS) state = STATE_LEVEL1_COMPLETE;
        } else if (state == STATE_LEVEL2) {
            if (character.x >= world_width - character.width - LEVEL_FINISH_EPS) state = STATE_GAME_END;
        }

        EndDrawing();
    }

    // Cleanup
    character_destroy(&character);
    character_destroy(&monster);
    UnloadTexture(floor_piece_texture);
    UnloadTexture(platform1_texture);
    UnloadTexture(platform2_texture);
    UnloadTexture(background_texture);
    UnloadTexture(tex_menu_start);
    UnloadTexture(tex_lvl1_complete);
    UnloadTexture(tex_lvl2_start);
    UnloadTexture(tex_game_end);
    UnloadTexture(wood_texture);
    UnloadTexture(stone.texture);
    UnloadTexture(laser.texture);
    free(platforms);
    CloseWindow();
    return 0;
}
