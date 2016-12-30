#include "bitbox.h"
#include "common.h"
#include "chiptune.h"
#include "tetris.h"
#include "snake.h"
#include "palette.h"
#include "font.h"
#include "name.h"
#include "io.h"
#include <stdint.h>
#include <stdlib.h> // abs
#include <string.h> // memset

#define BG_COLOR 141
#define SELECT_COLOR RGB(255,150,250)
#define MENU_OPTIONS 9
#define NUMBER_LINES 20

uint8_t menu_index CCM_MEMORY;
uint8_t game_wide CCM_MEMORY;
uint8_t tetris_fall_speed CCM_MEMORY;
uint8_t tetris_raise_speed CCM_MEMORY;
uint8_t tetris_start_height CCM_MEMORY;
uint8_t game_torus CCM_MEMORY;

void menu_init()
{
    menu_index = 0;
    tetris_players = 2;
    game_wide = 1;
    tetris_fall_speed = 5;
    tetris_raise_speed = 1;
    tetris_start_height = 1;
    game_torus = 1;
    game_to_play = MaybeTetris;
}

void show_big_number(uint8_t *c3, int v)
{
    if (v < 1000)
    {
        c3[0] = '0' + v/100;
        c3[1] = '0' + (v/10)%10;
        c3[2] = '0' + v%10;
    }
    else
    {
        c3[0] = '0' + v/10000;
        c3[1] = '0' + (v/1000)%10;
        c3[2] = 'k';
    }
}

void menu_line()
{
    if (vga_line < 16)
    {
        if (vga_line/2 == 0)
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        return;
    }
    else if (vga_line >= 16 + NUMBER_LINES*10)
        return;

    int line = vga_line-16;
    int internal_line = line%10;
    line /= 10;
    if (internal_line >= 8)
    {
        memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        return;
    }
    switch (line)
    {
        case 1:
        {
            uint8_t msg[32] = { 'L', '/', 'R', ':', ' ', 0 };
            switch (game_to_play)
            {
                case MaybeTetris:
                    strcpy((char *)msg+5, "tetribox");
                    break;
                case MaybeSnake:
                    strcpy((char *)msg+5, "snake");
                    break;
                case MaybeHex:
                    strcpy((char *)msg+5, "hex");
                    break;
                case MaybeGreeble:
                    strcpy((char *)msg+5, "greebles");
                    break;
            }
            font_render_line_doubled(msg, 34, internal_line, 65535, BG_COLOR*257);
        }
        break;
        case 3:
        {
            int p = 2;
            if (game_to_play == MaybeTetris)
                p = tetris_players;
            else if (game_to_play == MaybeSnake)
                p = snake_players;
            uint8_t msg[] = { '0'+p, ' ', 'p', 'l', 'a', 'y', 'e', 'r', 0 };
            font_render_line_doubled(msg, 52, internal_line, menu_index == 0 ? SELECT_COLOR :
                65535, BG_COLOR*257);

            font_render_line_doubled((const uint8_t *)"top score:", 162, internal_line, 
                65535, BG_COLOR*257);
        }
        break;
        case 4:
        {
            int p = 2;
            uint32_t top_scores[2] = {0, 0};
            uint64_t top_coop_score = 0;
            uint64_t top_wide_score = 0;
            if (game_to_play == MaybeTetris)
            {
                p = tetris_players;
                top_scores[0] = top_tetris_scores[0];
                top_scores[1] = top_tetris_scores[1];
                top_coop_score = top_coop_tetris_score;
                top_wide_score = top_wide_tetris_score;
            }
            else if (game_to_play == MaybeSnake)
            {
                p = snake_players;
                top_scores[0] = top_snake_scores[0];
                top_scores[1] = top_snake_scores[1];
                top_coop_score = top_coop_snake_score;
                top_wide_score = top_wide_snake_score;
            }
            if (p == 1)
            {
                if (game_wide)
                {
                    font_render_line_doubled((const uint8_t *)"wide", 52, internal_line, menu_index == 1 ? SELECT_COLOR :
                        65535, BG_COLOR*257);
                    uint8_t msg[13];
                    font_render_line_doubled(write_hex(msg+12, top_wide_score), 162, internal_line, 
                        65535, BG_COLOR*257);
                }
                else
                {
                    font_render_line_doubled((const uint8_t *)"normal", 52, internal_line, menu_index == 1 ? SELECT_COLOR :
                        65535, BG_COLOR*257);
                    uint8_t msg[13];
                    font_render_line_doubled(write_hex(msg+12, top_scores[0] > top_scores[1] ?
                        top_scores[0] : top_scores[1]), 162, internal_line, 65535, BG_COLOR*257);
                }
            }
            else
            {
                if (game_wide)
                {
                    font_render_line_doubled((const uint8_t *)"coop", 52, internal_line, menu_index == 1 ? SELECT_COLOR :
                        65535, BG_COLOR*257);
                    uint8_t msg[13];
                    font_render_line_doubled(write_hex(msg+12, top_coop_score), 162, internal_line, 
                        65535, BG_COLOR*257);
                }
                else
                {
                    font_render_line_doubled((const uint8_t *)"duel", 52, internal_line, menu_index == 1 ? SELECT_COLOR :
                        65535, BG_COLOR*257);
                    uint8_t msg[15];
                    uint8_t *center = write_hex(msg+14, top_scores[1]);
                    uint8_t *begin = write_hex(--center, top_scores[0]);
                    *center = '/';
                    font_render_line_doubled(begin, 162, internal_line, 65535, BG_COLOR*257);
                }
            }
        }
        break;
        case 6:
        switch (game_to_play)
        {
            case MaybeTetris:
            {
                uint8_t msg[] = { 'f', 'a', 'l', 'l', ' ', 's', 'p', 'e', 'e', 'd', ' ', 
                    '0'+tetris_fall_speed%10, 
                0 };
                font_render_line_doubled(msg, 52, internal_line, menu_index == 2 ? SELECT_COLOR :
                    65535, BG_COLOR*257);
            }
            break;
            case MaybeSnake:
            {
                uint8_t msg[] = { 's', 'p', 'e', 'e', 'd', ' ', 
                    '0'+10-snake_speed, 
                0 };
                font_render_line_doubled(msg, 52, internal_line, menu_index == 2 ? SELECT_COLOR :
                    65535, BG_COLOR*257);
            }
            break;
        }
        break;
        case 7:
        switch (game_to_play)
        {
            case MaybeTetris:
            {
                uint8_t msg[] = { 'r', 'a', 'i', 's', 'e', ' ', 's', 'p', 'e', 'e', 'd', ' ', 
                    '0'+tetris_raise_speed%10, 
                0 };
                font_render_line_doubled(msg, 52, internal_line, menu_index == 3 ? SELECT_COLOR :
                    65535, BG_COLOR*257);
            }
            break;
            case MaybeSnake:
            {
                uint8_t msg[] = { 'g', 'u', 'n', ' ', 's', 'p', 'e', 'e', 'd', ' ',
                    '0'+snake_bullet_speed%10, 
                0 };
                font_render_line_doubled(msg, 52, internal_line, menu_index == 3 ? SELECT_COLOR :
                    65535, BG_COLOR*257);
            }
            break;
        }
        break;
        case 8:
        switch (game_to_play)
        {
            case MaybeTetris:
            {
                uint8_t msg[] = { 's', 't', 'a', 'r', 't', ' ', 'h', 'e', 'i', 'g', 'h', 't', ' ', 
                    '0'+tetris_start_height%10, 
                0 };
                font_render_line_doubled(msg, 52, internal_line, menu_index == 4 ? SELECT_COLOR :
                    65535, BG_COLOR*257);
            }
            break;
            case MaybeSnake:
            {
                uint8_t msg[] = { 's', 't', 'a', 'r', 't', ' ', 'l', 'e', 'n', 'g', 't', 'h', ' ', 
                    ' ', ' ', ' ', 
                0 };
                show_big_number(msg+13, snake_starting_size);
                font_render_line_doubled(msg, 52, internal_line, menu_index == 4 ? SELECT_COLOR :
                    65535, BG_COLOR*257);
            }
            break;
        }
        break;
        case 9:
        switch (game_to_play)
        {
            case MaybeTetris:
            if (game_torus)
            {
                font_render_line_doubled((const uint8_t *)"torus", 52, internal_line, menu_index == 5 ? SELECT_COLOR :
                    65535, BG_COLOR*257);
            }
            else
            {
                font_render_line_doubled((const uint8_t *)"walls", 52, internal_line, menu_index == 5 ? SELECT_COLOR :
                    65535, BG_COLOR*257);
            }
            break;
            case MaybeSnake:
            {
                uint8_t msg[] = { 'f', 'o', 'o', 'd', ' ', 'c', 'o', 'u', 'n', 't', ' ', 
                    ' ', ' ', ' ', 
                0 };
                show_big_number(msg+11, snake_food_count);
                font_render_line_doubled(msg, 52, internal_line, menu_index == 5 ? SELECT_COLOR :
                    65535, BG_COLOR*257);
            }
            break;
        }
        break;
        case 11:
        if (available_count)
        {
            uint8_t msg[24] = { 'm', 'u', 's', 'i', 'c', ' ' };
            strcpy((char *)msg+6, available_filenames[available_index]);
            font_render_line_doubled(msg, 52, internal_line, menu_index == 6 ? SELECT_COLOR :
                65535, BG_COLOR*257);
        }
        break;
        case 12:
        if (available_count)
        {
            uint8_t msg[] = { 'v', 'o', 'l', 'u', 'm', 'e', ' ', 
                hex[chip_volume/16], hex[chip_volume % 16], 
            0 };
            font_render_line_doubled(msg, 52, internal_line, menu_index == 7 ? SELECT_COLOR :
                65535, BG_COLOR*257);
        }
        break;
        case 14:
        if (available_count)
        {
            font_render_line_doubled((const uint8_t *)"palette", 52, internal_line, menu_index == 8 ? SELECT_COLOR :
                65535, BG_COLOR*257);
            uint32_t *dst = (uint32_t *)draw_buffer + (52+8*9)/2;
            for (int i=0; i<16; ++i)
            {
                *++dst = palette2[i*17];
                *++dst = palette2[i*17];
                *++dst = palette2[i*17];
                *++dst = palette2[i*17];
            }
        }
        break;
        case 16:
            font_render_line_doubled((const uint8_t *)"dpad:change options", 44, internal_line, 65535, BG_COLOR*257);
        break;
        case 17:
            font_render_line_doubled((const uint8_t *)"start:play", 44, internal_line, 65535, BG_COLOR*257);
        break;
        case 18:
            if (menu_index == 8)
                font_render_line_doubled((const uint8_t *)"select:edit palette", 44, internal_line, 65535, BG_COLOR*257);
            else if (menu_index >= 6)
                font_render_line_doubled((const uint8_t *)"select:edit music", 44, internal_line, 65535, BG_COLOR*257);
            else
                font_render_line_doubled((const uint8_t *)"select:music test", 44, internal_line, 65535, BG_COLOR*257);
        break;
    }
}

void load_song(int init_also)
{
    chip_kill();
    if (strcmp(base_filename, available_filenames[available_index]))
    {
        strcpy(base_filename, available_filenames[available_index]);
        io_load_instrument(16);
        io_load_verse(16);
        io_load_anthem();
    }
    if (init_also)
        chip_play_init(0);
}

void menu_controls()
{
    if (GAMEPAD_PRESS(0, L))
    {
        if (game_to_play)
            --game_to_play;
        else
            game_to_play = MaybeSnake; // TODO: change this when we get more games
        return;
    }
    else if (GAMEPAD_PRESS(0, R))
    {
        if (game_to_play < MaybeSnake) // TODO: see above
            ++game_to_play;
        else
            game_to_play = MaybeTetris;
        return;
    }

    if (GAMEPAD_PRESS(0, down))
    {
        ++menu_index;
        if (available_count)
        {
            if (menu_index >= MENU_OPTIONS)
                menu_index = 0;
        }
        else
        {
            if (menu_index >= MENU_OPTIONS-3)
                menu_index = 0;
        }
    }
    if (GAMEPAD_PRESS(0, up))
    {
        if (menu_index)
            --menu_index;
        else if (available_count)
            menu_index = MENU_OPTIONS-1;
        else
            menu_index = MENU_OPTIONS-4;
    }
    int modified = 0;
    if (GAMEPAD_PRESS(0, right))
        ++modified;
    if (GAMEPAD_PRESS(0, left))
        --modified;
    if (modified)
    {
        switch (menu_index)
        {
            case 0: // player
            switch (game_to_play)
            {
                case MaybeTetris:
                    tetris_players = 3 - tetris_players;
                break;
                case MaybeSnake:
                    snake_players += modified;
                    if (snake_players > 4)
                        snake_players = 1;
                    else if (snake_players < 1)
                        snake_players = 4;
                break;
            }
            break;
            case 1:
                game_wide = 1 - game_wide;
            break;
            case 2:
            switch (game_to_play)
            {
                case MaybeTetris:
                    tetris_fall_speed += modified;
                    if (tetris_fall_speed > 128)
                        tetris_fall_speed = 9;
                    else if (tetris_fall_speed > 9)
                        tetris_fall_speed = 0;
                break;
                case MaybeSnake:
                    snake_speed -= modified;
                    if (snake_speed > 9)
                        snake_speed = 0;
                    else if (snake_speed < 1)
                        snake_speed = 9;
                break;
            }
            break;
            case 3:
            switch (game_to_play)
            {
                case MaybeTetris:
                    tetris_raise_speed += modified;
                    if (tetris_raise_speed > 128)
                        tetris_raise_speed = 9;
                    else if (tetris_raise_speed > 9)
                        tetris_raise_speed = 0;
                break;
                case MaybeSnake:
                    snake_bullet_speed += modified;
                    if (snake_bullet_speed > 128)
                        snake_bullet_speed = 4;
                    else if (snake_bullet_speed > 4)
                        snake_bullet_speed = 0;
                break;
            }
            break;
            case 4:
            switch (game_to_play)
            {
                case MaybeTetris:
                    tetris_start_height += modified;
                    if (tetris_start_height > 128)
                        tetris_start_height = 9;
                    else if (tetris_start_height > 9)
                        tetris_start_height = 0;
                break;
                case MaybeSnake: // starting length
                    if (modified > 0)
                    {
                        if (snake_starting_size >= 13000)
                            break;
                        snake_starting_size += option_increment(snake_starting_size);
                    }
                    else
                    {
                        if (snake_starting_size <= 1)
                            break;
                        snake_starting_size -= option_decrement(snake_starting_size);
                    }
                break;
            }
            break;
            case 5:
            switch (game_to_play)
            {
                case MaybeTetris:
                    game_torus = 1 - game_torus;
                break;
                case MaybeSnake: // food count
                    if (modified > 0)
                    {
                        if (snake_food_count >= 11000)
                            break;
                        snake_food_count += option_increment(snake_food_count);
                    }
                    else
                    {
                        if (snake_food_count <= 1)
                            break;
                        snake_food_count -= option_decrement(snake_food_count);
                    }
                break;
            }
            break;
            case 6: // switch song
                if (available_count == 0)
                    break;
                chip_kill();
                if (modified > 0)
                {
                    if (++available_index >= available_count)
                        available_index = 0;
                }
                else if (available_index)
                    --available_index;
                else
                    available_index = available_count-1;
                {
                char old[16];
                strcpy(old, base_filename);
                strcpy(base_filename, available_filenames[available_index]);
                io_load_palette();
                strcpy(base_filename, old);
                }
            break;
            case 7: // switch volume
                if (modified > 0)
                {
                    if (chip_volume < 240)
                        chip_volume += 16;
                    else if (chip_volume < 255)
                        ++chip_volume;
                }
                else if (chip_volume >= 16)
                    chip_volume -= 16;
                else if (chip_volume)
                    --chip_volume;
            break;
        }
    }
    if (GAMEPAD_PRESS(0, start))
    {
        // start game
        load_song(0); // load song but don't play
        previous_visual_mode = MainMenu;
        switch (game_to_play)
        {
            case MaybeTetris:
                return game_switch(Tetris);
            case MaybeSnake:
                return game_switch(Snake);
            default:
                return game_switch(None);
        }
    }
    if (GAMEPAD_PRESS(0, select))
    {
        if (menu_index == 8)
            return game_switch(EditPalette);
        load_song(0);
        if (menu_index >= 6)
            return game_switch(EditAnthem);
        chip_play_init(0);
        return;
    }
}
