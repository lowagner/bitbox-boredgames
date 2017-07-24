#include "bitbox.h"
#include "common.h"
#include "palette.h"
#include "chiptune.h"
#include <string.h> // memset
#include <stdlib.h> // rand

#define BULLETS 3

uint64_t invaders_score;
uint32_t invaders_scores[2];
uint32_t top_invaders_scores[2];
uint64_t top_coop_invaders_score;
uint64_t top_wide_invaders_score;
uint64_t invaders_score;
uint32_t invaders_scores[2];
uint8_t invaders_players CCM_MEMORY;


struct protectors {
    uint8_t y, x;
    uint8_t all_out;
    uint8_t alive;
    uint8_t bullets;
};
struct protectors protectors[2] CCM_MEMORY;

void invaders_init()
{
    invaders_players = 2;
    
    top_invaders_scores[0] = 0;;
    top_invaders_scores[1] = 0;;
    top_coop_invaders_score = 0;
    top_wide_invaders_score = 0;
}

void invaders_start()
{
    // reset score
    invaders_score = 0;
    invaders_scores[0] = 0;
    invaders_scores[1] = 0;

    // get rid of bullets:
    for (int p=0; p<2; ++p)
    for (int b=0; b<BULLETS; ++b)
            bullet[p][b].alive = 0;

    // black everything:
    memset(game.super, 0, sizeof(game.super));

    // setup the invaderss:
    if (invaders_players == 1)
    {
        protectors[0].y = 119;
        protectors[0].x = 160/2;
        protectors[0].alive = 1;
        protectors[1].alive = 0;
    }
    else
    {
        protectors[0].y = 119;
        protectors[0].x = 160/2-10;
        protectors[0].alive = 1;
        protectors[1].y = 119;
        protectors[1].x = 160/2+9;
        protectors[1].alive = 1;
    }
}

void invaders_controls()
{
    if (handle_special_state())
        return;
}

void invaders_line()
{
    static const int lookup[25] = {
        0, 1, 2, 10, 1, // bg, wall, indestructible wall, food, bullet
        4, 4, 4, 4, 4,
        14, 14, 14, 14, 14,
        5, 5, 5, 5, 5,
        15, 15, 15, 15, 15
    };
    int j = vga_line/2;
    uint32_t *dst = (uint32_t *)draw_buffer;
    for (int i=0; i<160; ++i)
        *dst++ = palette2[17*lookup[game.super[j][i]]];
}

void invaders_finalize()
{
    if (game_wide && invaders_players > 1)
    {
        invaders_score = invaders_scores[0]*invaders_scores[1];
        if (invaders_score > top_coop_invaders_score)
        {
            top_coop_invaders_score = invaders_score;
            strcpy((char *)game_message, "new top invaders_score!");
        }
    }
    else
    {
        if (invaders_scores[0] > top_invaders_scores[0])
        {
            top_invaders_scores[0] = invaders_scores[0];
            strcpy((char *)game_message, "new top score!");
        }
        if (invaders_players > 1 && invaders_scores[1] > top_invaders_scores[1])
        {
            top_invaders_scores[1] = invaders_scores[1];
            strcpy((char *)game_message, "new top score!");
        }
    }
}
