#include "bitbox.h"
#include "common.h"
#include "chiptune.h"
#include "instrument.h"
#include "verse.h"
#include "anthem.h"
#include "name.h"
#include "io.h"
#include "font.h"
#include "palette.h"
#include "menu.h"
#include "tetris.h"
#include "snake.h"
#include "invaders.h"

#include <string.h> // memcpy

VisualMode visual_mode CCM_MEMORY; 
VisualMode previous_visual_mode CCM_MEMORY;
VisualMode old_visual_mode CCM_MEMORY;
uint8_t game_message[32] CCM_MEMORY;
int game_message_timeout CCM_MEMORY;
uint8_t player_message[2][16] CCM_MEMORY;
uint8_t *player_message_start[2] CCM_MEMORY;
uint16_t old_gamepad[2] CCM_MEMORY;
uint16_t new_gamepad[2] CCM_MEMORY;
uint8_t gamepad_press_waits[2] CCM_MEMORY;
uint8_t gamepad_press_wait CCM_MEMORY;
uint8_t game_to_play CCM_MEMORY;
struct game game CCM_MEMORY;
struct bullet bullet[MAX_GUNNERS][MAX_BULLETS] CCM_MEMORY;
uint8_t game_paused CCM_MEMORY;
int8_t game_win_state CCM_MEMORY;



#define BSOD 140

uint8_t *write_hex(uint8_t *c, uint64_t number)
{
    *c = 0;
    *--c = hex[number%64];
    number /= 64;
    while (number)
    {
        *--c = hex[number%64];
        number /= 64;
    }
    return c;
}

void game_init()
{
    game_message[0] = 0;
    menu_init();
    font_init();
    anthem_init();
    verse_init();
    chip_init();
    instrument_init();
    palette_init();

    game_wide = 0;
    tetris_init();
    snake_init();

    // now load everything else
    if (io_get_recent_filename())
    {
        message("resetting everything\n");
        // had troubles loading a filename
        base_filename[0] = 'N';
        base_filename[1] = 'O';
        base_filename[2] = 'N';
        base_filename[3] = 'E';
        base_filename[4] = 0;

        // need to reset everything
        palette_load_default();
        anthem_load_default();
        verse_load_default();
        instrument_load_default();
    }
    else // there was a filename to look into
    {
        io_load_palette();
        io_load_anthem();
        io_load_verse(16);
        io_load_instrument(16);
    }
    io_list_games();

    // init game mode
    old_visual_mode = None;
    previous_visual_mode = None;
    game_switch(MainMenu);
}

void game_frame()
{
    new_gamepad[0] = gamepad_buttons[0] & (~old_gamepad[0]);
    new_gamepad[1] = gamepad_buttons[1] & (~old_gamepad[1]); 
    old_gamepad[0] = gamepad_buttons[0];
    old_gamepad[1] = gamepad_buttons[1]; 

    switch (visual_mode)
    {
    case MainMenu:
        menu_controls();
        break;
    case Tetris:
        tetris_controls();
        break;
    case Snake:
        snake_controls();
        break;
    case Invaders:
        invaders_controls();
        break;
    case ChooseFilename:
        name_controls();
        break;
    case EditPalette:
        palette_controls();
        break;
    case EditAnthem:
        anthem_controls();
        break;
    case EditVerse:
        verse_controls();
        break;
    case EditInstrument:
        instrument_controls();
        break;
    default:
        if (GAMEPAD_PRESS(0, select))
            game_switch(MainMenu);
        break;
    }
   
    if (gamepad_press_wait)
        gamepad_press_waits[0] = --gamepad_press_wait;
    else if (gamepad_press_waits[0])
        gamepad_press_wait = --gamepad_press_waits[0];
    if (gamepad_press_waits[1])
        --gamepad_press_waits[1];
    
    if (game_message_timeout && --game_message_timeout == 0)
        game_message[0] = 0; 
}

void graph_line() 
{
    if (vga_odd)
        return;
    switch (visual_mode)
    {
        case MainMenu:
            menu_line();
            break;
        case Tetris:
            tetris_line();
            break;
        case Snake:
            snake_line();
            break;
        case Invaders:
            invaders_line();
            break;
        case ChooseFilename:
            name_line();
            break;
        case EditPalette:
            palette_line();
            break;
        case EditAnthem:
            anthem_line();
            break;
        case EditVerse:
            verse_line();
            break;
        case EditInstrument:
            instrument_line();
            break;
        default:
        {
            int line = vga_line/10;
            int internal_line = vga_line%10;
            if (vga_line/2 == 0 || (internal_line/2 == 4))
            {
                memset(draw_buffer, BSOD, 2*SCREEN_W);
                return;
            }
            if (line >= 4 && line < 20)
            {
                line -= 4;
                uint32_t *dst = (uint32_t *)draw_buffer + 37;
                uint32_t color_choice[2] = { (BSOD*257)|((BSOD*257)<<16), 65535|(65535<<16) };
                int shift = ((internal_line/2))*4;
                for (int c=0; c<16; ++c)
                {
                    uint8_t row = (font[c+line*16] >> shift) & 15;
                    for (int j=0; j<4; ++j)
                    {
                        *(++dst) = color_choice[row&1];
                        row >>= 1;
                    }
                    *(++dst) = color_choice[0];
                }
                return;
            }
            break;
        }
    }
}

void game_switch(VisualMode new_visual_mode)
{
    if (new_visual_mode == visual_mode)
        return;

    chip_kill();
    game_message[0] = 0;

    switch (visual_mode)
    {
    case Tetris:
        tetris_finalize();
        break;
    case Snake:
        snake_finalize();
        break;
    case Invaders:
        invaders_start();
        break;
    default:
        break;
    }

    game_win_state = 0;
    game_paused = 0;
    
    visual_mode = new_visual_mode;
    switch (new_visual_mode)
    {
    case Tetris:
        tetris_start();
        break;
    case Snake:
        snake_start();
        break;
    case Invaders:
        invaders_start();
        break;
    default:
        return;
    }

    chip_play_init(0);
}

void draw_parade(int line, uint8_t bg_color)
{
}

void end_player(int p)
{
    if (game_win_state == 0)
    {
        // if p=1, then player 0 won, make game_win_state even
        // if p=0, then player 1 won, make game_win_state odd
        game_win_state = 3-p + 100;
        if (game_wide)
        {
            game_win_state += 20;
            strcpy((char *)player_message[0], "you lost!");
            player_message_start[0] = player_message[0];
        }
        else
        {
            strcpy((char *)player_message[p], "player   lost!");
            player_message[p][7] = '1'+p;
            player_message_start[p] = player_message[p];
        }
    }
    else if (game_win_state < 0)
        message("already had a tie loss, why is player %d being ended again?\n", p);
    else if (game_win_state % 2 == p)
    {
        strcpy((char *)player_message[p], "you lost, too!");
        player_message_start[0] = player_message[0];
        game_win_state = -100;
    }
    else
        message("already lost, why is player %d being ended again?\n", p);
}

int handle_special_state()
{
    if (GAMEPAD_PRESS(0, start))
    {
        if (GAMEPAD_PRESSED(0, select) || game_win_state)
        {
            player_message[0][0] = 0;
            game_paused = 0;
            previous_visual_mode = None;
            game_switch(MainMenu);
        }
        else
        {
            // pause mode
            chip_play = game_paused;
            game_paused = 1 - game_paused;
        }
        return 1;
    }

    if (game_paused)
        return 1; 
    if (!game_win_state)
        return 0;
    if (game_win_state < 0) // both players lost
    {
        if (game_win_state == -1)
            game_switch(MainMenu); // return to menu
        else if (vga_frame % 8 == 0)
            ++game_win_state;
    }
    else if (game_win_state > 2) // even: player 0 won; odd: player 1 won
    {
        if (vga_frame % 8 == 0)
            game_win_state -= 2;
    }
    else
        game_switch(MainMenu);
    return 1;
}
