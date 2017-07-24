#pragma once
extern uint64_t invaders_score;
extern uint32_t invaders_scores[2];
extern uint32_t top_invaders_scores[2];
extern uint64_t top_coop_invaders_score;
extern uint64_t top_wide_invaders_score;
extern uint8_t invaders_players;


void invaders_line();
void invaders_controls();

void invaders_init();
void invaders_start();
void invaders_finalize();

