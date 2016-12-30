#include "bitbox.h"
#include "common.h"
#include "palette.h"
#include "chiptune.h"
#include <string.h> // memset
#include <stdlib.h> // rand

#define BULLETS 3
#define BULLET_LIFE 64
#define RIGHT 0
#define UP 1
#define LEFT 2
#define DOWN 3
#define DEAD 4

uint64_t snake_score;
uint32_t snake_scores[2];
uint32_t top_snake_scores[2];
uint64_t top_coop_snake_score;
uint64_t top_wide_snake_score;

// game variables
uint8_t snake_players CCM_MEMORY;
uint8_t snake_speed CCM_MEMORY;
uint8_t snake_bullet_speed;
int32_t snake_starting_size;
int32_t snake_food_count;

struct snake {
    struct {
        uint8_t y, x;
    } head;
    struct {
        uint8_t y, x;
    } tail;
    uint8_t properties;
    uint8_t heading; // direction that the snake is going.
    uint8_t alive;
    uint8_t bullets;
    int32_t tail_wait; // set tail_wait > 0 to grow snake.  set it < 0 to make it shrink.
};
struct snake snake[4] CCM_MEMORY;

struct bullet {
    uint8_t y, x;
    uint8_t heading;
    uint8_t alive;
};
struct bullet bullet[4][BULLETS] CCM_MEMORY;

int32_t option_increment(int v)
{
    if (v < 5)
        return 1;
    else if (v < 115)
        return 5;
    else if (v < 1000)
        return 59;
    else
        return 1000;
}

int32_t option_decrement(int v)
{
    if (v <= 5)
        return 1;
    else if (v <= 115)
        return 5;
    else if (v <= 1000)
        return 59;
    else
        return 1000;
}

void make_food(int how_much)
{
    if (!game_torus)
    {
        while (--how_much >= 0)
        {
            uint8_t y = 1+rand()%((SCREEN_H/2)-2);
            uint8_t x = 1+rand()%((SCREEN_W/2)-2);
            for (int k=0; game.super[y][x] != 0 && k < 50; ++k)
            {
                y = 1+rand()%((SCREEN_H/2)-2);
                x = 1+rand()%((SCREEN_W/2)-2);
            }
            game.super[y][x] = 3;
        }
    }
    else
    {
        while (--how_much >= 0)
        {
            uint8_t y = rand()%(SCREEN_H/2);
            uint8_t x = rand()%(SCREEN_W/2);
            for (int k=0; game.super[y][x] != 0 && k < 50; ++k)
            {
                y = rand()%(SCREEN_H/2);
                x = rand()%(SCREEN_W/2);
            }
            game.super[y][x] = 3;
        }
    }
}

/*
uint16_t encode(uint16_t color, uint8_t heading)
{   
    // returns encoded color
    // encode up to 16 values into the color (4 bits):
    return (color & CODE_MASK) | (heading & 1) | ((heading & 2) << 4) | ((heading & 4) << 8) | ((heading & 8) << 12);
    
}

uint8_t decode(uint16_t color)
{
    return (uint8_t)((color & 1) | ((color >> 4) & 2) | ((color >> 8) & 4) | ((color >> 12) & 8));
}

*/
void init_snake(int p, uint8_t y, uint8_t x, uint8_t heading, int32_t size)
{
    snake[p].head.y = snake[p].tail.y = y;
    snake[p].head.x = snake[p].tail.x = x;
    snake[p].properties = 0;
    snake[p].heading = heading;
    if (size < 0)
        size = 0;
    snake[p].tail_wait = size;
    snake[p].alive = 1;
    game.super[y][x] = 5+5*p + heading;
}

void kill_snake(int p)
{
    snake[p].alive = 0;
    if (p < 2)
        end_player(p);
}

int zip_snake(int p, uint8_t y, uint8_t x, uint8_t color)
{
    // zip up snake p until the tail reaches point y,x.
    // leave a trail of "color" in the wake.
    int i = 0; // counter, once it exceeds the largest possible snake length, it returns...
    while (!(snake[p].tail.y == y && snake[p].tail.x == x))
    {
        // decode the direction the tail is heading from the color it's on:
        uint8_t tail_heading = game.super[snake[p].tail.y][snake[p].tail.x]%5;
        // blank the tail
        game.super[snake[p].tail.y][snake[p].tail.x] = color;
        switch (tail_heading)
        {
        case UP:
            if (snake[p].tail.y)
                --snake[p].tail.y;
            else
                snake[p].tail.y = (SCREEN_H/2)-1; 
            break;
        case DOWN:
            if (snake[p].tail.y < (SCREEN_H/2)-1)
                ++snake[p].tail.y;
            else
                snake[p].tail.y = 0; 
            break;
        case LEFT: 
            if (snake[p].tail.x)
                --snake[p].tail.x;
            else
                snake[p].tail.x = (SCREEN_W/2)-1; 
            break;
        case RIGHT: 
            if (snake[p].tail.x < (SCREEN_W/2)-1)
                ++snake[p].tail.x;
            else
                snake[p].tail.x = 0; 
            break;
        case DEAD:
            return 1;
        }

        if (++i > 19200)
            // something got funny...
            return 1;
    }
    // remove any wait from the tail
    snake[p].tail_wait = 0;
    return 0;
}

void make_walls()
{
    // setup the walls on the torus
    memset(game.super[0], 2, 160);
    memset(game.super[119], 2, 160);
    for (int j=0; j<120; ++j)
        game.super[j][0] = game.super[j][159] = 2;
}

void remove_walls()
{
    memset(game.super[0], 0, 160);
    memset(game.super[119], 0, 160);
    for (int j=0; j<120; ++j)
        game.super[j][0] = game.super[j][159] = 0;
}

void snake_init()
{
    snake_players = 2;
    game_wide = 0;
    snake_speed = 4;
    snake_bullet_speed = 2;
    snake_food_count = 5;
    snake_starting_size = 10;
    
    top_snake_scores[0] = 0;;
    top_snake_scores[1] = 0;;
    top_coop_snake_score = 0;
    top_wide_snake_score = 0;
}

void snake_start()
{
    // get rid of bullets:
    for (int p=0; p<2; ++p)
    for (int b=0; b<BULLETS; ++b)
            bullet[p][b].alive = 0;

    // black everything:
    memset(game.super, 0, sizeof(game.super));

    // setup the snakes:
    if (snake_players == 1)
    {
        init_snake(0, 60,90, UP, snake_starting_size);
        snake[1].alive = 0;
        snake[2].alive = 0;
        snake[3].alive = 0;
    }
    else if (snake_players == 2)
    {
        init_snake(0, 60,90, UP, snake_starting_size);
        init_snake(1, 60,70, DOWN, snake_starting_size);
        snake[2].alive = 0;
        snake[3].alive = 0;
    }
    else if (snake_players == 3)
    {
        init_snake(0, 60,100, UP, snake_starting_size);
        init_snake(1, 60,80, UP, snake_starting_size);
        init_snake(2, 60,60, UP, snake_starting_size);
        snake[3].alive = 0;
    }
    else
    {
        init_snake(0, 60,90, UP, snake_starting_size);
        init_snake(1, 60,70, DOWN, snake_starting_size);
        init_snake(2, 50,80, LEFT, snake_starting_size);
        init_snake(3, 70,80, RIGHT, snake_starting_size);
    }

    if (!game_torus)
        make_walls();
    make_food(snake_food_count);
    chip_play_init(0);
}

void do_snake_dynamics()
{
    for (int p=0; p<snake_players; ++p)
    if (snake[p].alive)
    {
        if (vga_frame % snake_speed == 0)
        {
            if (GAMEPAD_PRESSED(p, up) && (snake[p].heading == LEFT || snake[p].heading == RIGHT))
                snake[p].heading = UP;
            else if (GAMEPAD_PRESSED(p, down) && (snake[p].heading == LEFT || snake[p].heading == RIGHT))
                snake[p].heading = DOWN;
            else if (GAMEPAD_PRESSED(p, left) && (snake[p].heading == UP || snake[p].heading == DOWN))
                snake[p].heading = LEFT;
            else if (GAMEPAD_PRESSED(p, right) && (snake[p].heading == UP || snake[p].heading == DOWN))
                snake[p].heading = RIGHT;
        
            // encode direction you're going onto the current pixel, so tail can follow:
            game.super[snake[p].head.y][snake[p].head.x] = 5 + 5*p + snake[p].heading;

            switch (snake[p].heading)
            {
            case UP:
                if (snake[p].head.y)
                    --snake[p].head.y;
                else
                    snake[p].head.y = SCREEN_H/2-1; // should only happen on torus
                break;
            case DOWN:
                if (snake[p].head.y < SCREEN_H/2-1)
                    ++snake[p].head.y;
                else
                    snake[p].head.y = 0; // should only happen on torus
                break;
            case LEFT: 
                if (snake[p].head.x)
                    --snake[p].head.x;
                else
                    snake[p].head.x = SCREEN_W/2-1; // should only happen on torus
                break;
            case RIGHT: 
                if (snake[p].head.x < SCREEN_W/2-1)
                    ++snake[p].head.x;
                else
                    snake[p].head.x = 0; // should only happen on torus
                break;
            }
            
            // check collisions on new spot first:
            if (game.super[snake[p].head.y][snake[p].head.x] != 0)
            {
                if (game.super[snake[p].head.y][snake[p].head.x] == 3)
                {
                    ++snake[p].tail_wait;
                    make_food(1);
                }
                else
                {
                    for (int other_p=0; other_p<p; ++other_p)
                    if (snake[other_p].alive && snake[p].head.x == snake[other_p].head.x && snake[p].head.y == snake[other_p].head.y )
                    {
                        kill_snake(other_p);
                        message("snake battle!  both die.\n");
                        game.super[snake[p].head.y][snake[p].head.x] = 1;
                    }
                    kill_snake(p);
                    continue;
                }
            }
            
            // then mark the next spot as where you're going!
            // (encode heading for that next time)
            game.super[snake[p].head.y][snake[p].head.x] = 5+5*p;

            // finished with the snake's head, now go onto the snake's tail!
            if (snake[p].tail_wait > 0)
                --snake[p].tail_wait;
            else
            {
                while (snake[p].tail_wait <= 0)
                {
                    // decode the direction the tail is heading from the color it's on:
                    uint8_t tail_heading = game.super[snake[p].tail.y][snake[p].tail.x]%5;
                    // blank the tail
                    game.super[snake[p].tail.y][snake[p].tail.x] = 0;
                    switch (tail_heading)
                    {
                    case UP:
                        if (snake[p].tail.y)
                            --snake[p].tail.y;
                        else
                            snake[p].tail.y = SCREEN_H/2-1; // should only happen on torus
                        break;
                    case DOWN:
                        if (snake[p].tail.y < SCREEN_H/2-1)
                            ++snake[p].tail.y;
                        else
                            snake[p].tail.y = 0; // should only happen on torus
                        break;
                    case LEFT: 
                        if (snake[p].tail.x)
                            --snake[p].tail.x;
                        else
                            snake[p].tail.x = SCREEN_W/2-1; // should only happen on torus
                        break;
                    case RIGHT: 
                        if (snake[p].tail.x < SCREEN_W/2-1)
                            ++snake[p].tail.x;
                        else
                            snake[p].tail.x = 0; // should only happen on torus
                        break;
                    }
                    ++snake[p].tail_wait;
                }
                snake[p].tail_wait = 0;
            }
        } 

        // fire bullets if you want and are able
        if (GAMEPAD_PRESS(p, B) && snake_bullet_speed)
        {   
            message("FIRE!\n");
            // scan bullets for anyone not alive
            int b=0;
            for (; b<BULLETS; ++b)
                if (!bullet[p][b].alive)
                    break;
            if (b < BULLETS && (snake[p].head.x != snake[p].tail.x || snake[p].head.y != snake[p].tail.y))
            {   // found a dead bullet to use
                bullet[p][b].alive = BULLET_LIFE;
                bullet[p][b].heading = snake[p].heading;
                bullet[p][b].y = snake[p].head.y;
                bullet[p][b].x = snake[p].head.x;
                --snake[p].tail_wait;
                // push the bullet in front of player
                switch (bullet[p][b].heading)
                {
                case UP:
                    if (bullet[p][b].y)
                        --bullet[p][b].y;
                    else
                        bullet[p][b].y = SCREEN_H/2-1; // should only happen on torus
                    break;
                case DOWN:
                    if (bullet[p][b].y < SCREEN_H/2-1)
                        ++bullet[p][b].y;
                    else
                        bullet[p][b].y = 0; // should only happen on torus
                    break;
                case LEFT: 
                    if (bullet[p][b].x)
                        --bullet[p][b].x;
                    else
                        bullet[p][b].x = SCREEN_W/2-1; // should only happen on torus
                    break;
                case RIGHT: 
                    if (bullet[p][b].x < SCREEN_W/2-1)
                        ++bullet[p][b].x;
                    else
                        bullet[p][b].x = 0; // should only happen on torus
                    break;
                }
            }
        }
    }
}


int do_bullet_dynamics()
{
    if (vga_frame % snake_speed == 0)
    for (int step=0; step < snake_bullet_speed; ++step)
    for (int b=0; b<BULLETS; ++b)
    for (int p=0; p<2; ++p)
    if (bullet[p][b].alive)
    {
        // remove bullet from its current spot:
        game.super[bullet[p][b].y][bullet[p][b].x] = 0;
    
        // move it forward:
        switch (bullet[p][b].heading)
        {
        case UP:
            if (bullet[p][b].y)
                --bullet[p][b].y;
            else
                bullet[p][b].y = SCREEN_H/2-1; // should only happen on torus
            break;
        case DOWN:
            if (bullet[p][b].y < SCREEN_H/2-1)
                ++bullet[p][b].y;
            else
                bullet[p][b].y = 0; // should only happen on torus
            break;
        case LEFT: 
            if (bullet[p][b].x)
                --bullet[p][b].x;
            else
                bullet[p][b].x = SCREEN_W/2-1; // should only happen on torus
            break;
        case RIGHT: 
            if (bullet[p][b].x < SCREEN_W/2-1)
                ++bullet[p][b].x;
            else
                bullet[p][b].x = 0; // should only happen on torus
            break;
        }

        // check collisions
        if (game.super[bullet[p][b].y][bullet[p][b].x])
        {
            switch (game.super[bullet[p][b].y][bullet[p][b].x])
            {
                case 1: // destructible
                    game.super[bullet[p][b].y][bullet[p][b].x] = 0;
                break;
                case 2: // indestructible, ignore!  can't shoot through, either.
                break;
                case 3: // food
                    game.super[bullet[p][b].y][bullet[p][b].x] = 0;
                break;
                case 4: // bullet
                    // find the other bullet and kill it
                    for (int pb=0; pb<snake_players*BULLETS; ++pb)
                    if (bullet[p][b].y == bullet[pb/BULLETS][pb%BULLETS].y && 
                        bullet[p][b].x == bullet[pb/BULLETS][pb%BULLETS].x)
                    {
                        bullet[pb/BULLETS][pb%BULLETS].alive = 0; 
                        break;
                    }
                    game.super[bullet[p][b].y][bullet[p][b].x] = 0;
                break;
                default: // it was some snake
                {
                    int pc = game.super[bullet[p][b].y][bullet[p][b].x];
                    int other_p = pc/5 - 1;
                    // kill off enemy snake (other_p) if it's his head, otherwise zip him up
                    if ((pc%5 < 4) && snake[other_p].alive)
                    {
                        if (bullet[p][b].y == snake[other_p].head.y && bullet[p][b].x == snake[other_p].head.x)
                            kill_snake(other_p); 
                        else // was not the head, zip up tail to where bullet hit
                        {
                            if (zip_snake(other_p, bullet[p][b].y, bullet[p][b].x, 9+5*other_p))
                                return 1;
                            snake[other_p].tail_wait = -1; // tail will jump forward one
                        }
                        // don't blank the spot, since the tail needs to zip past its encoding
                    }
                    else
                        // dead snake, put a hole in it
                        game.super[bullet[p][b].y][bullet[p][b].x] = 0;
                }
            }
            bullet[p][b].alive = 1; // will get killed here next...
        }
       
        --bullet[p][b].alive;
        // check whether the bullet should continue:
        if (bullet[p][b].alive)
            game.super[bullet[p][b].y][bullet[p][b].x] = 4;
    }
    return 0;
}

void snake_controls()
{
    if (GAMEPAD_PRESS(0, start))
    {
        if (GAMEPAD_PRESSED(0, select) || game_win_state)
        {
            player_message[0][0] = 0;
            game_paused = 0;
            previous_visual_mode = None;
            game_switch(MainMenu);
            return;
        }
        // pause mode
        chip_play = game_paused;
        game_paused = 1 - game_paused;
        return;
    }

    if (handle_special_state())
        return;

    // do bullet dynamics
    if (do_bullet_dynamics())
        return game_switch(MainMenu);
    // do snake dynamics
    do_snake_dynamics();
}

void snake_line()
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

void snake_finalize()
{
}
