#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libdragon.h>

#define W 9
#define H 9

#define MINE_COUNT 8 //((2*W)-1)

// board values
#define SAFE 0
#define MINE 1

// state values
#define CLEARED 2
#define FLAGGED 3
#define FUCKED 4
#define UNCLEARED 5

char board[H][W];
char state[H][W];

display_context_t _dc;

int dead;
int mine_colors[10];
int mines[MINE_COUNT][3];
int move_count;
int px,py;
int rx,ry;

int get_number(int x, int y);

display_context_t lockVideo(int wait)
{
    display_context_t dc;

    if (wait)
    {
        while (!(dc = display_lock()));
    }
    else
    {
        dc = display_lock();
    }

    return dc;
}

void unlockVideo(display_context_t dc)
{
    if (dc)
    {
        display_show(dc);
    }
}

#define screen_x(tilex) (60 + (tilex*22))
#define screen_y(tiley) (20 + (tiley*22))

void fill_circle(int ox, int oy, int r, int color) {
	int r2 = r * r;
	int area = r2 << 2;
	int rr = r << 1;

	for (int i = 0; i < area; i++)
	{
		int tx = (i % rr) - r;
		int ty = (i / rr) - r;

		if (tx * tx + ty * ty <= r2) {
			graphics_draw_pixel(_dc,ox + tx, oy + ty, color);
		}
	}
}

void draw_mine(int x, int y) {
	fill_circle((x*22) + 60 + 11, (y*22) + 20 + 11, 6, graphics_make_color(0x00,0x00,0x00,0x00));
	graphics_draw_line(_dc, (x*22) + 60+5  , (y*22) + 20+5, (x*22) + 60 + 22-5, (y*22) + 20 + 22-5, 0);
	graphics_draw_line(_dc, (x*22) + 60+5  , (y*22) + 20+11, (x*22) + 60 + 22-5, (y*22) + 20 + 11, 0);
	graphics_draw_line(_dc, (x*22) + 60+22-5  , (y*22) + 20+5, (x*22) + 60+5, (y*22) + 20 + 22-5, 0);
	fill_circle((x*22) + 60 + 9	, (y*22) + 20 + 9, 1, graphics_make_color(0xff,0xff,0xff,0x00));
}

void draw_flag(int x, int y) {
	// tile is 22x22 pixels
	// if 16x16, flag is 8 pixels wide
	// 8 pixels tall
	
	// so 11 pixels wide
	// 11 pixels tall
	uint32_t flag_color = graphics_make_color(0xaa,0x00,0x00,0xff);
	uint32_t base_color = graphics_make_color(0x55,0x55,0x55,0xff);

	// top of flag
	graphics_draw_line(_dc, screen_x(x) + 11, screen_y(y) +  6, screen_x(x) + 13, screen_y(y) +  6, flag_color);
	graphics_draw_line(_dc, screen_x(x) +  9, screen_y(y) +  7, screen_x(x) + 13, screen_y(y) +  7, flag_color);
	graphics_draw_line(_dc, screen_x(x) +  8, screen_y(y) +  8, screen_x(x) + 13, screen_y(y) +  8, flag_color);
	graphics_draw_line(_dc, screen_x(x) +  9, screen_y(y) +  9, screen_x(x) + 13, screen_y(y) +  9, flag_color);
	graphics_draw_line(_dc, screen_x(x) + 10, screen_y(y) + 10, screen_x(x) + 13, screen_y(y) + 10, flag_color);
	graphics_draw_line(_dc, screen_x(x) + 11, screen_y(y) + 11, screen_x(x) + 13, screen_y(y) + 11, flag_color);
	// base of flag
	graphics_draw_line(_dc, screen_x(x) + 11, screen_y(y) + 12, screen_x(x) + 13, screen_y(y) + 12, base_color);
	graphics_draw_line(_dc, screen_x(x) + 11, screen_y(y) + 13, screen_x(x) + 13, screen_y(y) + 13, base_color);
	graphics_draw_line(_dc, screen_x(x) +  9, screen_y(y) + 14, screen_x(x) + 15, screen_y(y) + 14, base_color);
	graphics_draw_line(_dc, screen_x(x) +  8, screen_y(y) + 15, screen_x(x) + 16, screen_y(y) + 15, base_color);
}


void render_board(void) {
	_dc = lockVideo(1);

	// board tiles
	for(int y=0;y<H;y++) {
		for(int x=0;x<W;x++) {
			// exploded a mine
			if (state[y][x] == FUCKED) {
				graphics_draw_box(_dc, screen_x(x), screen_y(y), 22, 22, graphics_make_color(0xff,0x00,0x00,0xff)); 
				graphics_set_color(0,graphics_make_color(0,0,0,0));
				//graphics_draw_character(_dc, screen_x(x)+8, screen_y(y)+8, 'X');
				draw_mine(x,y);
			}
			else if (dead && board[y][x] == MINE) {
				draw_mine(x,y);
			}
			else if (state[y][x] == FLAGGED) {
				graphics_draw_box(_dc, screen_x(x), screen_y(y), 22, 22, graphics_make_color(0xaa,0xaa,0xaa,0xff)); 
				graphics_draw_box(_dc, screen_x(x) + 4, screen_y(y) + 4, 22-4, 22-4, graphics_make_color(0x77,0x77,0x77,0xff)); 
				draw_flag(x,y);
			}
			// unrevealed space
			else if (state[y][x] == UNCLEARED) {
				graphics_draw_box(_dc, screen_x(x), 20 + (y*22), 22, 22, graphics_make_color(0xaa,0xaa,0xaa,0xff)); 
				graphics_draw_box(_dc, screen_x(x) + 4, 20 + (y*22) + 4, 22-4, 22-4, graphics_make_color(0x77,0x77,0x77,0xff)); 
			}
			// empty revealed space
			else if (state[y][x] == CLEARED && (get_number(x,y) == 0)) {
				graphics_draw_box(_dc, screen_x(x), 20 + (y*22), 22, 22, graphics_make_color(0x77,0x77,0x77,0xff)); 
			}
			// revealed space that borders a mine
			else if (state[y][x] == CLEARED && (get_number(x,y))) {
				graphics_draw_box(_dc, screen_x(x), 20 + (y*22), 22, 22, graphics_make_color(0x77,0x77,0x77,0xff)); 
				graphics_set_color(mine_colors[get_number(x,y)],graphics_make_color(0,0,0,0));
				graphics_draw_character(_dc, screen_x(x)+8, 20 + (y*22)+8, get_number(x,y)+'0');
			}
		}
	}

	// grid lines
	for(int x=0;x<W+1;x++) {
		graphics_draw_line(_dc,screen_x(x), 20,screen_x(x),220, graphics_make_color(0,0,0,0xff));
	}
	for(int y=0;y<H+1;y++) {
		graphics_draw_line(_dc,60, 20+(y*22),260,20+(y*22), graphics_make_color(0,0,0,0xff));
	}

	// 'player' / currently selected board tile
	graphics_draw_line(_dc,screen_x(px),20 + (py*22), screen_x(px) + 22,20 + (py*22), graphics_make_color(0xff,0x00,0x00,0x00));
	graphics_draw_line(_dc,screen_x(px),42 + (py*22), screen_x(px) + 22,42 + (py*22), graphics_make_color(0xff,0x00,0x00,0x00));
	graphics_draw_line(_dc,screen_x(px),20 + (py*22), screen_x(px),42 + (py*22), graphics_make_color(0xff,0x00,0x00,0x00));
	graphics_draw_line(_dc,screen_x(px) + 22,20 + (py*22), screen_x(px) + 22,42 + (py*22), graphics_make_color(0xff,0x00,0x00,0x00));

	// used to highlight which board tile the reveal algorithm is currently traversing
	if(rx != -1) {
		graphics_draw_line(_dc,screen_x(rx),20 + (ry*22), screen_x(rx) + 22,20 + (ry*22), graphics_make_color(0xff,0x00,0xff,0x00));
		graphics_draw_line(_dc,screen_x(rx),42 + (ry*22), screen_x(rx) + 22,42 + (ry*22), graphics_make_color(0xff,0x00,0xff,0x00));
		graphics_draw_line(_dc,screen_x(rx),20 + (ry*22), screen_x(rx),42 + (ry*22), graphics_make_color(0xff,0x00,0xff,0x00));
		graphics_draw_line(_dc,screen_x(rx) + 22,20 + (ry*22), screen_x(rx) + 22,42 + (ry*22), graphics_make_color(0xff,0x00,0xff,0x00));
	}

	unlockVideo(_dc);
}

#define up(x,y) (board[y-1][x] == MINE)
#define up_right(x,y) (board[y-1][x+1] == MINE)
#define right(x,y) (board[y][x+1] == MINE)
#define down_right(x,y) (board[y+1][x+1] == MINE)
#define down(x,y) (board[y+1][x] == MINE)
#define down_left(x,y) (board[y+1][x-1] == MINE)
#define left(x,y) (board[y][x-1] == MINE)
#define up_left(x,y) (board[y-1][x-1] == MINE)

// returns number of mines adjacent to board position
int get_number(int x, int y) {
	// left-hand side of board
	if (x == 0) {
		// somewhere between top and bottom rows
		if ((y > 0) && (y < H-1)) {
			return up(x,y) + up_right(x,y) + right(x,y) + down_right(x,y) + down(x,y);
		}
		// top row
		else if (y == 0) {
			return right(x,y) + down_right(x,y) + down(x,y);
		}
		// bottom row
		else if (y == H-1) {
			return up(x,y) + up_right(x,y) + right(x,y);
		}	
	}
	// right-hand side of board
	else if (x == W-1) {
		// somewhere between top and bottom rows
		if ((y > 0) && (y < H-1)) {
			return up(x,y) + down(x,y) + down_left(x,y) + left(x,y) + up_left(x,y);
		}
		// top row
		else if (y == 0) {
			return down(x,y) + down_left(x,y) + left(x,y);
		}
		// bottom row
		else if (y == H-1) {
			return up(x,y) + up_left(x,y) + left(x,y);
		}
	}
	// anywhere between left-hand and right-hand sides of board
	else {
		// top row
		if (y == 0) {
			return right(x,y) + down_right(x,y) + down(x,y) + down_left(x,y) + left(x,y);
		}
		// bottom row
		else if (y == H-1) {
			return up(x,y) + up_right(x,y) + right(x,y) + left(x,y) + up_left(x,y);
		}
		// anywhere between top and bottom rows
		else {
			return up(x,y) + up_right(x,y) + right(x,y) + down_right(x,y) + down(x,y) + down_left(x,y) + left(x,y) + up_left(x,y);
		}
	}
	
	return 0;
}

int reveal_from(int x, int y, int first) {
	/*
	algorithm
	starting at x,y
	
	if x,y is a mine, you lost the game
	
	recursively check neighbors in each direction
	up, up right, right, down right, down, down left, left, up left
		if neighbor mine count is 0, space is CLEARED and go in same direction
		if neighbor mine count is > 0, stop, go back, check next direction
	*/
	move_count++;
	rx = x;
	ry = y;
	render_board();

	if (board[y][x] == MINE) {
		if (move_count == 1) {
			board[y][x] = SAFE;
		}
		else {
			if (first) {
				state[y][x] = FUCKED;
				return 256;
			}
			else {
				return 0;
			}
		}
	}	

	else if ((state[y][x] == CLEARED) || get_number(x,y)) {
		state[y][x] = CLEARED;
		return 0;
	}

	else if (state[y][x] == FLAGGED) {
		return 0;
	}

	state[y][x] = CLEARED;

	if (!first) {
		return 0;
	}

	// up left
	if (y != 0 && x != 0) {
		reveal_from(x-1,y-1,0);		
	}

	// up
	if (y != 0) {
		reveal_from(x,y-1,0);	
	}

	// up right
	if (y != 0 && x != (W-1)) {
		reveal_from(x+1,y-1,0);		
	}

	// right
	if (x != (W-1)) {
		reveal_from(x+1,y,0);
	}

	// down right
	if (x != (W-1) && y != (H-1)) {
		reveal_from(x+1,y+1,0);
	}

	// down
	if (y != (H-1)) {
		reveal_from(x,y+1,0);
	}

	// down left
	if (x != 0 && y != (H-1)) {
		reveal_from(x-1,y+1,0);
	}

	// left
	if (x != 0) {
		reveal_from(x-1,y,0);
	}

	return 1;
}

void reset_game() {
	// used for random mine placement
	int setup_mines[W*H];
	int i;

	// start at top-left
	px = py = 0;
	// don't show algorithm progress
	rx = ry = -1;

	// how many times the player has picked a board tile
	move_count = 0;
	// whether or not the game is over
	dead = 0;

	// clear the board and the game state over the board
	for(int y=0;y<H;y++) {
		for (int x=0; x<W; x++) {
			board[y][x] = SAFE;
			state[y][x] = UNCLEARED;
			// clear the random mine placement state
			setup_mines[y+(x*W)] = 0;
		}
	}

#if 0
	// static mine placement
	board[0][0] = MINE;
	board[H-1][W-1] = MINE;
	board[2][2] = MINE;
	board[4][4] = MINE;
	board[6][6] = MINE;
#endif
	
	i = 0;
	while (i < MINE_COUNT)
	{
		int random = rand() % (W*H);
		random = rand() % (W*H);
		random = rand() % (W*H);
		int x = random / W;
		int y = random % H;
 
		// Add the mine if no mine is placed at this position on the board
		if (!setup_mines[random])
		{
			setup_mines[random] = 1;
			mines[i][0] = x;
			mines[i][1] = y;
			mines[i][2] = 0;
			board[y][x]= MINE;
			i++;
		}
	}
}

int main(int argc, char **argv) {
	console_init();
	console_set_render_mode(RENDER_AUTOMATIC);
	controller_init();

	mine_colors[0] = 0;
	// indexed by number of mines
	mine_colors[1] = graphics_make_color(0,0,0xff,0);
	mine_colors[2] = graphics_make_color(0,0xff,0,0);
	mine_colors[3] = graphics_make_color(0xff,0,0,0);

	display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);
	reset_game();
	
	while (1) {
		controller_scan();
		struct controller_data keys_pressed = get_keys_down();

		// B button, reset game to new random starting state
		// works even if game has ended
		if (keys_pressed.c[0].B) {
			reset_game();
		}

		// game hasn't ended yet, accept other player inputs
		if (!dead) {
			// dpad left, not already on left-hand side of board
			if (keys_pressed.c[0].left && (px > 0)) {
				px--;
			}
			// dpad right, not already on right-hand side of board
			if (keys_pressed.c[0].right && (px < (W-1))) {
				px++;
			}
			// dpad up, not already on top side of board
			if (keys_pressed.c[0].up && (py > 0)) {
				py--;
			}
			// dpad down, not already on bottom side of board
			if (keys_pressed.c[0].down && (py < (H-1))) {
				py++;
			}
			// A button, reveal tiles starting at current player selected board position
			if (keys_pressed.c[0].A && (reveal_from(px,py,1) == 256)) {
				// return code 256 from reveal_from indicates player selected a mine, game over
				dead = 1;
			}
			// L button, flag tile at current player selected board position
			if (keys_pressed.c[0].L && (state[py][px] == UNCLEARED)) {
				state[py][px] = FLAGGED;
			}
			else if (keys_pressed.c[0].L && (state[py][px] == FLAGGED)) {
				state[py][px] = UNCLEARED;
			}
		}

		// when rx,ry == -1, not currently revealing tiles, so don't render purple "algorithm progress" outline
		rx = -1;
		ry = -1;

		render_board();

		// checking to see if every non-mine board tile has been revealed
		// W*H total board tiles
		int remaining_cells = W*H;
		for(int y=0;y<H;y++) {
			for(int x=0;x<W;x++) {
				// decrement remaining count when a revealed board tile is found
				if(state[y][x] == CLEARED) {
					remaining_cells--;
				}
			}
		}
		
		// if the remaining count is equal to the starting mine count, the game is over
		if(remaining_cells == MINE_COUNT) {
			printf("YOU WON\n");
			dead = 1;
		}
	}

	return 0;
}