#pragma once
extern uint64_t tetris_score;
extern uint32_t tetris_scores[2];
extern uint32_t top_tetris_scores[2];
extern uint64_t top_coop_tetris_score;
extern uint64_t top_wide_tetris_score;

extern uint8_t tetris_players;

extern uint8_t tetris_fall_speed;
extern uint8_t tetris_raise_speed;
extern uint8_t tetris_start_height;

void tetris_line();
void tetris_controls();

void tetris_init();
void tetris_start();
void tetris_finalize();
