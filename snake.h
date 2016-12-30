#pragma once
extern uint64_t snake_score;
extern uint32_t snake_scores[2];
extern uint32_t top_snake_scores[2];
extern uint64_t top_coop_snake_score;
extern uint64_t top_wide_snake_score;

extern uint8_t snake_players;
extern uint8_t snake_speed CCM_MEMORY;
extern uint8_t snake_bullet_speed;
extern int32_t snake_starting_size;
extern int32_t snake_food_count;

int32_t option_increment(int v);
int32_t option_decrement(int v);

void snake_line();
void snake_controls();

void snake_init();
void snake_start();
void snake_finalize();
