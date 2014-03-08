#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

// settings
#define ACCEL 1.8
#define DECEL 1
#define GRAVITY 10
#define GRAVITY_ACCEL 0.05
#define JUMP_ACCEL -3.4
#define MAX_VEL 9
#define KICK_TIME 2
#define MAX_SNOW 12
#define SNOW_DRIFT 10
#define BELL_SCROLL_RATE 1
#define MAX_BELLS 25
#define BELL_DIST 20
#define BELL_OFFSCREEN (5*BELL_DIST)
#define BIRD_SPEED 1
#define ANI_LENGTH 8
#define BIRD_PERIOD 16
#define BELLS_PER_LEVEL 35
#define MAX_EXPLODES 10
#define EXPLODE_DIST 8
#define EXPLODE_RADIUS 6

// bunny dimensions
#define BUNNY_HEIGHT 13
#define BUNNY_WIDTH 13
#define BUNNY_UP_H 13
#define BUNNY_DOWN_H 11
#define BUNNY_STAND_H 12

// 8 bit timer interrupt handler 
INT_HANDLER oldint5 = NULL;
volatile int counter;

DEFINE_INT_HANDLER(cint5handler)
{
	counter++;
	ExecuteHandler(oldint5);
}

// save status interrupt location
INT_HANDLER save_int_1 = NULL;

// top score
double top_score = 123880758507679.; // my actual top score
double top_score_killer = 0.;
double score, cur_score_inc;
unsigned int bells_hit;
char bonus_level;
struct bell * target;

// misc functions
void draw_str(short int x, short int y, const char *str);
void draw_dec(short int x, short int y, int number);
void draw_f(short int x, short int y, double number);
void draw_hline(short int y, short int x1, short int x2, short int type);
void draw_sprite(short x, short y, short w, short h, void * sprite);
void sleepc(short t);

// sprites
unsigned short sprite_bunny_ul[] = {0x0c00, 0x1400, 0x1800, 0x3000, 0x4fd8, 0x9068, 0xf010, 0x0a90, 0x1320, 0x1d20, 0x00d0, 0x0050, 0x0030, 0x0000, 0x0000, 0x0000};
unsigned short sprite_bunny_ur[] = {0x0180, 0x0140, 0x00c0, 0x0060, 0xdf90, 0xb048, 0x4078, 0x4a80, 0x2640, 0x25c0, 0x5800, 0x5000, 0x6000, 0x0000, 0x0000, 0x0000};
unsigned short sprite_bunny_dl[] = {0x0c00, 0x1418, 0x19e8, 0x3e10, 0x4890, 0x9110, 0xf120, 0x0ba0, 0x0b40, 0x1580, 0x1800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};
unsigned short sprite_bunny_dr[] = {0x0180, 0xc140, 0xbcc0, 0x43e0, 0x4890, 0x4448, 0x2478, 0x2e80, 0x1680, 0x0d40, 0x00c0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};
unsigned short sprite_bunny_s[] = {0x6300, 0x5500, 0x3600, 0x1c00, 0x2200, 0x2a00, 0x5d00, 0x4100, 0x5500, 0x7700, 0x8880, 0xff80, 0x0000, 0x0000, 0x0000, 0x0000};
unsigned short sprite_bunny_jump[] = {0xee00, 0xaa00, 0x7c00, 0x4400, 0x5400, 0xba00, 0x8200, 0xaa00, 0x6c00, 0x5400, 0x5400, 0x5400, 0x3800, 0x0000, 0x0000, 0x0000};
unsigned char sprite_bell0[] = {0x3c, 0x42, 0x42, 0x42, 0x42, 0x42, 0x81, 0xff};
unsigned char sprite_bell1[] = {0x38, 0x44, 0x44, 0x44, 0x44, 0x82, 0xfe, 0x00};
unsigned char sprite_bell2[] = {0x30, 0x48, 0x48, 0x48, 0x84, 0xfc, 0x00, 0x00};
unsigned char sprite_bell3[] = {0x20, 0x50, 0x50, 0x88, 0xf8, 0x00, 0x00, 0x00};
unsigned char sprite_birdl1[] = {0x0c, 0x18, 0x18, 0x7f, 0xfe, 0x00, 0x00, 0x00};
unsigned char sprite_birdl2[] = {0x00, 0x00, 0x00, 0x7f, 0xfe, 0x0c, 0x00, 0x00};
unsigned char sprite_birdr1[] = {0x30, 0x18, 0x18, 0xfe, 0x7f, 0x00, 0x00, 0x00};
unsigned char sprite_birdr2[] = {0x00, 0x00, 0x00, 0xfe, 0x7f, 0x30, 0x00, 0x00};

unsigned char * sprite_bell[] = {sprite_bell0, sprite_bell1, sprite_bell2, sprite_bell3};
unsigned short sprite_bell_dim[] = {8, 7, 6, 5};
unsigned char * sprite_bird[] = {sprite_birdl1, sprite_birdl2, sprite_birdr1, sprite_birdr2};
short ani_cnt;

// bunny
struct bunny {
	float x, y, x_vel, y_vel;
	short xi, yi;
	char state, dir, vdir, kick, hit_bell;
	int fall_time;
};
void draw_bunny(struct bunny * this);
enum bunny_states {bunny_jump, bunny_start, bunny_falling};
enum bunny_dir {bunny_left, bunny_right, bunny_up, bunny_down, bunny_sit};
	
// bells and birds
struct bell
{
	short x, y, size, dir;
	char bird, active;
};
int add_bell(struct bell * bells, short x, short size, char is_bird);
void draw_bells(struct bell * bells, struct bunny * bunny, char update);

// snow
struct snow
{
	short x, y, c;
};
void draw_snow(struct snow * sn, char update);

// explode
struct explode
{
	short x, y, count;
	char active;
};
struct explode * explodes;
short explode_xpos[] = {3, 2, 0, -2, -3, -2, 0, 2};
short explode_ypos[] = {0, 2, 3, 2, 0, -2, -3, -2};
void add_explode(struct explode * explodes, short x, short y);
void draw_explodes(struct explode * explodes, char update);

// "times two" text
struct times_two
{
	short x, y, c;
} tt;
void draw_tt(struct times_two * tt);

// screen
enum scroll {scroll_start, scroll_follow};
short screen_scroll_state;
short screen_scroll_rate;

// god mode
void god_mode(struct bunny * b, struct bell * bells);

void _main(void)
{
	// init timer and counter interrupt
	counter = 0;
	oldint5 = GetIntVec(AUTO_INT_5);
	SetIntVec(AUTO_INT_5, cint5handler);
	
	// redirect auto-int 1 to "nothing"
	// keeps the "2nd" and "alpha" from showing
	save_int_1 = GetIntVec (AUTO_INT_1);
	SetIntVec (AUTO_INT_1, DUMMY_HANDLER);
	
	// variables
	char score_buf[64];
	int bell_counter, bird_counter;
	short i;
	char exit_game;
	bonus_level = 0;
	
	// bunny
	struct bunny b;
	
	// create bell array
	struct bell * bells = malloc(MAX_BELLS * sizeof(struct bell));
	
	// create snow
	struct snow * snows = malloc(MAX_SNOW * sizeof(struct snow));
	for (i = 0; i < MAX_SNOW; i++) {
		snows[i].x = rand()%LCD_WIDTH;
		snows[i].y = rand()%LCD_HEIGHT;
		snows[i].c = rand()%(2*SNOW_DRIFT);
	}
	
	// create explode array
	explodes = malloc(MAX_EXPLODES * sizeof(struct explode));
	
	// title screen
	ClrScr();
	FontSetSys(F_8x10);
	draw_str(LCD_WIDTH/2, 15, "Winterbells");
	FontSetSys(F_4x6);
	draw_str(LCD_WIDTH/2, LCD_HEIGHT - 30, "Craig Harrison (2009)");
	draw_str(LCD_WIDTH/2, LCD_HEIGHT - 23, "Bunny Pixel Art by Aaron McCollough");
	draw_sprite(LCD_WIDTH/2 - BUNNY_WIDTH/2, 40, 16, BUNNY_HEIGHT, sprite_bunny_ur);
	draw_str(LCD_WIDTH / 2, LCD_HEIGHT - 7, "2nd unleashes killer rabbit mode");
	
	// short pause
	for (i=0; i<45; i++) {
		draw_snow(snows, 1);
		sleepc(1);
		draw_snow(snows, 0);
	}
	
	while (!_keytest(RR_ESC)) {
	
		// setup screen for gameplay
		FontSetSys(F_6x8);
		ClrScr();
		counter = 0;
		
		// set up gameplay
		b.x = LCD_WIDTH / 2;
		b.y = LCD_HEIGHT - BUNNY_HEIGHT;
		b.x_vel = 0;
		b.y_vel = 0;
		b.state = bunny_start;
		b.dir = bunny_sit;
		b.vdir = bunny_up;
		b.fall_time = 0;
		b.kick = 0;
		b.hit_bell = 0;
		target = NULL;
		
		tt.c = 0;
		
		screen_scroll_state = scroll_start;
		screen_scroll_rate = 0;
		
		// clear misc variables
		score = 0;
		bell_counter = 0;
		bird_counter = 0;
		ani_cnt = 0;
		cur_score_inc = 1;
		bells_hit = 0;
		exit_game = 0;
		
		// clear bell array
		for (i = 0; i < MAX_BELLS; i++) bells[i].active = 0;
		
		// clear explodes array
		for (i = 0; i < MAX_EXPLODES; i++) explodes[i].active = 0;
		
		// game loop
		while (!exit_game) {
			
			// read key input
			if (bonus_level == 2) god_mode(&b, bells);
			else if (_keytest(RR_LEFT)) b.x_vel -= ACCEL;
			else if (_keytest(RR_RIGHT)) b.x_vel += ACCEL;
			else if (b.x_vel >= DECEL) b.x_vel -= DECEL;
			else if (b.x_vel <= -DECEL) b.x_vel += DECEL;
			else b.x_vel = 0;
			
			// bunny movement
			if (b.x_vel > MAX_VEL) b.x_vel = MAX_VEL;
			else if (b.x_vel < -MAX_VEL) b.x_vel = -MAX_VEL;
			if (b.state != bunny_start) b.y_vel += 0.2;
			if (b.y_vel > GRAVITY) b.y_vel = GRAVITY;
			else if (b.y_vel < JUMP_ACCEL) b.y_vel = JUMP_ACCEL;
			b.x += b.x_vel;
			b.y += b.y_vel - screen_scroll_rate;
			if (b.x < 0) {
				b.x = 0;
				b.x_vel = 0;
			} else if (b.x > LCD_WIDTH - 16) {
				b.x = LCD_WIDTH - 16;
				b.x_vel = 0;
			}
			
			// bunny direction
			if (b.state == bunny_start && b.x_vel == 0) b.dir = bunny_sit;
			if (b.x_vel > 0) b.dir = bunny_right;
			else if (b.x_vel < 0) b.dir = bunny_left;
			if (b.y_vel > 0) b.vdir = bunny_down;
			else if (b.y_vel < 0) b.vdir = bunny_up;
			
			// bunny kick
			if (b.kick) b.kick--;
			
			// bunny start
			if (b.state == bunny_start && bonus_level != 2 && (_keytest(RR_2ND) || _keytest(RR_UP))) {
				b.y_vel = 1.5*JUMP_ACCEL;
				b.state = bunny_jump;
			} else if (b.state == bunny_falling) { // bunny fall
				b.fall_time++;
			}
			
			// update bunny integer status
			// must be last bunny thing done
			b.xi = (short)b.x;
			b.yi = (short)b.y;
			
			// screen logic
			switch (screen_scroll_state) {
			case scroll_start:
				screen_scroll_rate = 0;
				// change state
				if (b.yi < LCD_HEIGHT / 2) screen_scroll_state = scroll_follow;
				// reset to ground
				if (b.yi > LCD_HEIGHT - BUNNY_HEIGHT) {
					b.y = LCD_HEIGHT - BUNNY_HEIGHT;
					b.y_vel = 0;
					b.state = bunny_start;
				}
				break;
			case scroll_follow:
				screen_scroll_rate = b.yi - LCD_HEIGHT / 2;
				break;
			}
			
			// create bells
			if (b.state == bunny_start) bell_counter += 1; 
			else bell_counter -= screen_scroll_rate - 1;
			// add bell
			if (bell_counter > BELL_DIST) {
				bell_counter = 0;
				add_bell(bells, rand()%(LCD_WIDTH - 8), bells_hit / BELLS_PER_LEVEL, 0);
				// add bird
				if (b.state != bunny_start && ++bird_counter > BIRD_PERIOD) {
					bird_counter = 0;
					add_bell(bells, rand()%(LCD_WIDTH - 8), 0, 1);
				}
			}
			
			// animation
			if (++ani_cnt >= 2*ANI_LENGTH) ani_cnt = 0;
			
			// draw "times two"
			if (tt.c) {
				tt.y -= screen_scroll_rate;
				draw_tt(&tt);
			}
			
			// draw screen
			draw_bells(bells, &b, 1);
			draw_bunny(&b);
			draw_snow(snows, 1);
			draw_explodes(explodes, 1);
			
			// draw score
			sprintf(score_buf, "%f", score);
			DrawStr(3, 3, score_buf, A_XOR);
			
			// timer wait and clear
			while (counter < 1) ;
			counter = 0;
			
			// check if exit condition exists
			exit_game = (_keytest(RR_ESC) || (b.state == bunny_falling && b.fall_time > 2*bells_hit) || b.yi >= LCD_HEIGHT);
			
			// clear/update "times two"
			if (tt.c) {
				draw_tt(&tt);
				tt.c--;
			}
			
			// clear sprites
			draw_bells(bells, &b, 0);
			if (!exit_game || b.state == bunny_start) draw_bunny(&b);
			if (!exit_game) draw_snow(snows, 0);
			draw_explodes(explodes, 0);
			DrawStr(3, 3, score_buf, A_XOR);
		}
		
		// score screen
		while (_keytest(RR_ESC)) ;
		FontSetSys(F_8x10);
		// draw title
		char * tstr;
		switch (bonus_level) {
		case 2 :
			tstr = "Superbunny";
			break;
		case 3 :
			tstr = "Killer Rabbit Mode";
			break;
		default:
			tstr = "Bunny Splat!";
			break;
		}
		draw_str(LCD_WIDTH/2, 10, tstr);
		// display score
		FontSetSys(F_6x8);
		draw_str(LCD_WIDTH/2, 27, "Score");
		draw_f(LCD_WIDTH/2, 37, score);
		// top score
		draw_f(LCD_WIDTH/2, 90, (bonus_level != 3) ? top_score : top_score_killer);
		if ((score > top_score && !bonus_level || score > top_score_killer && bonus_level) && bonus_level != 2) {
			if (!bonus_level) top_score = score;
			else top_score_killer = score;
			draw_str(LCD_WIDTH/2, 80, "Old Top Score");
		} else draw_str(LCD_WIDTH/2, 80, (bonus_level != 2) ? "Top Score" : "Top Human Score");
		// wait for keypress
		while (!_keytest(RR_ESC) && !_keytest(RR_ENTER) && !_keytest(RR_2ND)) ;
		while (_keytest(RR_ENTER) || _keytest(RR_2ND)) ;
		
	}
	
	// shutdown
	// release dynamic data
	free(bells);
	free(snows);
	free(explodes);
	// restore old timer interrupt handler
	SetIntVec(AUTO_INT_5, oldint5);
	// restore old auto_int_1 interrupt handler
	SetIntVec(AUTO_INT_1, save_int_1);
	// empty keyboard buffer
	GKeyFlush();
}

// text functions (a bit slow)

void draw_str(short int x, short int y, const char *str)
{
	unsigned char f = FontGetSys();
	DrawStr(x - DrawStrWidth(str, f)/2, y, str, A_XOR);
}

void draw_dec(short int x, short int y, int number)
{
	char sbuf[64];
	unsigned char f = FontGetSys();
	sprintf(sbuf, "%d", number);
	DrawStr(x - DrawStrWidth(sbuf, f)/2, y, sbuf, A_XOR);
}

void draw_f(short int x, short int y, double number)
{
	char sbuf[64];
	unsigned char f = FontGetSys();
	sprintf(sbuf, "%f", number);
	DrawStr(x - DrawStrWidth(sbuf, f)/2, y, sbuf, A_XOR);
}

void draw_sprite(short x, short y, short w, short h, void * sprite)
{
	if (y <= -h || y >= LCD_HEIGHT) return; // offscreen vertically
	short dy2 = (y + h > LCD_HEIGHT) ? LCD_HEIGHT : y + h;
	short dy1 = (y < 0) ? 0 : y;
	
	switch (w) {
	case 8:
		Sprite8(x, dy1, dy2 - dy1, (unsigned char *)sprite + (dy1 - y)*sizeof(unsigned char), LCD_MEM, SPRT_XOR);
		break;
	case 16:
		Sprite16(x, dy1, dy2 - dy1, (unsigned short *)sprite + (dy1 - y)*sizeof(unsigned short), LCD_MEM, SPRT_XOR);
		break;
	case 32:
		Sprite32(x, dy1, dy2 - dy1, (unsigned long *)sprite + (dy1 - y)*sizeof(unsigned long), LCD_MEM, SPRT_XOR);
		break;
	default:
		break;
	}
}

// game functions

int add_bell(struct bell * bells, short x, short size, char is_bird)
{
	short i = 0;
	while (bells[i].active && i<MAX_BELLS) i++;
	if (i == MAX_BELLS) return -1;
	struct bell * b = &bells[i];
	b->active = 1;
	b->x = x;
	b->y = -7 + screen_scroll_rate;
	b->size = (size > 3) ? 3 : size;
	b->bird = is_bird;
	// birds
	if (is_bird) {
		b->y -= BELL_DIST / 2;
		b->size = 0;
	}
	b->dir = (rand()%2) ? -BIRD_SPEED : BIRD_SPEED;
	
	return i;
}

void draw_bells(struct bell * bells, struct bunny * bunny, char update)
{
	short w;
	int lowest = -4*LCD_HEIGHT;
	struct bell * b;
	for (b = bells; b != bells + MAX_BELLS; b++) {
		if (!b->active) continue;
		w = sprite_bell_dim[b->size];
		if (update) {
			// move
			b->y += BELL_SCROLL_RATE - screen_scroll_rate;
			if (b->y > LCD_HEIGHT + BELL_OFFSCREEN || (screen_scroll_state == scroll_start && b->y > 2*LCD_HEIGHT/3)) b->active = 0;
			// birds
			if (b->bird || bonus_level == 3) {
				b->x += b->dir;
				if (b->x < 0) {
					b->dir = BIRD_SPEED;
					b->x = 0;
				} else if (b->x + 8 > LCD_WIDTH) {
					b->dir = -BIRD_SPEED;
					b->x = LCD_WIDTH - 8;
				}
			}
			// hit by bunny
			if (b->x + w > bunny->xi && b->x < bunny->xi + BUNNY_WIDTH && b->y + w > bunny->yi && b->y < bunny->yi + BUNNY_HEIGHT) {
				bunny->y_vel = JUMP_ACCEL;
				if (!b->bird) score += cur_score_inc++;
				else {
					score *= 2;
					// draw "times two"
					if (tt.c) draw_tt(&tt);
					tt.x = b->x;
					tt.y = b->y + w - 4;
					tt.c = 2*EXPLODE_DIST;
					draw_tt(&tt);
				}
				if (bells_hit != MAXINT) bells_hit++;
				b->active = 0;
				bunny->kick = KICK_TIME;
				bunny->hit_bell = 1;
				add_explode(explodes, b->x + w / 2, b->y + w / 2);
			}
			if (b->active && b->y + w > lowest) lowest = b->y + w;
		}
		// draw
		if (b->active) {
			if (!b->bird) draw_sprite(b->x, b->y, 8, w, sprite_bell[b->size]);
			else draw_sprite(b->x, b->y, 8, 8, sprite_bird[(b->dir < 0 ? 0 : 2) + (ani_cnt / ANI_LENGTH)]);
		}
	}
	
	// change bunny state to falling if below all bells and headed down
	if (update && bunny->state == bunny_jump && lowest < -2*LCD_HEIGHT && bunny->y_vel > 0) bunny->state = bunny_falling;
}

void draw_bunny(struct bunny * this)
{
	short * a = NULL;
	// bunny start
	if (this->state == bunny_start || this->dir == bunny_sit) {
		if (this->dir == bunny_sit) {
			if (this->y_vel >= 0) a = sprite_bunny_s;
			else a = sprite_bunny_jump;
		} else if ((3*ani_cnt/ANI_LENGTH)%2) { // animated movement
			if (this->dir == bunny_right) a = sprite_bunny_dr;
			else a = sprite_bunny_dl;
		} else {
			if (this->dir == bunny_right) a = sprite_bunny_ur;
			else a = sprite_bunny_ul;
		}
	} else { // bunny jumping
		if (this->vdir == bunny_down || this->kick) {
			if (this->dir == bunny_right) a = sprite_bunny_dr;
			else a = sprite_bunny_dl;
		} else {
			if (this->dir == bunny_right) a = sprite_bunny_ur;
			else a = sprite_bunny_ul;
		}
	}
	// draw
	draw_sprite(this->xi, this->yi, 16, BUNNY_HEIGHT, a);
}

void draw_snow(struct snow * sn, char update)
{
	short dx;
	struct snow * s;
	for (s = sn; s != sn + MAX_SNOW; s++) {
		if (update) {
			// move
			s->y += BELL_SCROLL_RATE - screen_scroll_rate;
			if (++s->c >= 2*SNOW_DRIFT) s->c = 0;
			// offscreen
			if (s->y < 0) s->y = LCD_HEIGHT - 1;
			else if (s->y >= LCD_HEIGHT) s->y = 0;
		}
		// draw
		dx = s->x + (s->c > SNOW_DRIFT ? s->c : 2*SNOW_DRIFT - s->c)/2;
		if (dx >= 0 && dx < LCD_WIDTH && s->y >= 0 && s->y < LCD_HEIGHT) DrawPix(dx, s->y, A_XOR);
	}
}

void add_explode(struct explode * explodes, short x, short y)
{
	short i = 0;
	while (explodes[i].active && i<MAX_EXPLODES) i++;
	if (i == MAX_EXPLODES) return;
	struct explode * e = &explodes[i];
	e->active = 1;
	e->x = x;
	e->y = y;
	e->count = 0;
}

void draw_explodes(struct explode * explodes, char update)
{
	short dx, dy, i, h;
	struct explode * e;
	for (e = explodes; e != explodes + MAX_EXPLODES; e++) {
		if (!e->active) continue;
		if (update) {
			e->y += BELL_SCROLL_RATE - screen_scroll_rate;
			e->count++;
			if (e->count > EXPLODE_DIST) e->active = 0;
		}
		// draw
		if (e->active) {
			for (i = 0; i < 8; i++) {
				h = (e->count < EXPLODE_RADIUS) ? e->count : EXPLODE_RADIUS;
				dx = e->x + explode_xpos[i] * h;
				dy = e->y + explode_ypos[i] * h;
				if (dx >= 0 && dx + 1 < LCD_WIDTH && dy >= 0 && dy + 1 < LCD_HEIGHT) {
					DrawPix(dx, dy, A_XOR);
					DrawPix(dx + 1, dy, A_XOR);
					DrawPix(dx + 1, dy + 1, A_XOR);
					DrawPix(dx, dy + 1, A_XOR);
				}
			}
		}
	}
}

void god_mode(struct bunny * b, struct bell * bells)
{
	// select target bell
	struct bell * nl = NULL;
	struct bell * cl = NULL;
	struct bell * be;
	if (b->hit_bell || !target || !target->active || target->y > LCD_HEIGHT || (target->y + 8 < b->y && b->y_vel > 0)) {
		b->hit_bell = 0;
		for (be = bells; be != bells + MAX_BELLS; be++) {
			if (!be->active) continue;
			if ((!cl || be->y > cl->y) && be->y - sprite_bell_dim[be->size] < b->y) cl = be;
			if ((!nl || be->y < nl->y) && be->y > b->y) nl = be;
		}
		target = (!cl || (target->y + 8 < b->y && b->y_vel > 0)) ? nl : cl;
	}
	if (!target) return;

	// movement
	if (b->state == bunny_start && target->y > LCD_HEIGHT / 2) {
		// start
		b->y_vel = 1.5*JUMP_ACCEL;
		b->state = bunny_jump;
	} else if (target->x + sprite_bell_dim[target->size] - 1 < b->x) b->x_vel -= ACCEL;
	else if (target->x > b->x + BUNNY_WIDTH - 1) b->x_vel += ACCEL;
	else if (b->x_vel >= DECEL) b->x_vel -= DECEL;
	else if (b->x_vel <= -DECEL) b->x_vel += DECEL;
	else b->x_vel = 0;
}

void draw_tt(struct times_two * tt)
{
	DrawStr(tt->x - 8, tt->y, "x2", A_XOR);
}

void draw_hline(short int y, short int x1, short int x2, short int type) // type = A_NORMAL, A_REVERSE, A_XOR
{
	short int i;
	for (i = x1; i < x2; i++) DrawPix(i, y, type);
}

void sleepc(short t)
{
	counter = 0;
	while (counter < t) {
		if (_keytest(RR_ESC)) break; // exit
		// god mode
		if (!bonus_level && _keytest(RR_4)) bonus_level = 1;
		if (bonus_level == 1 && _keytest(RR_2)) {
			bonus_level++;
			FontSetSys(F_4x6);
			draw_str(LCD_WIDTH / 2, LCD_HEIGHT - 7, "2nd unleashes killer rabbit mode");
			FontSetSys(F_8x10);
			draw_str(LCD_WIDTH / 2, LCD_HEIGHT - 12, "Superbunny Enabled");
		}
		// bonus level
		if (bonus_level != 3 && _keytest(RR_2ND)) {
			bonus_level = 3;
			FontSetSys(F_4x6);
			draw_str(LCD_WIDTH / 2, LCD_HEIGHT - 7, "2nd unleashes killer rabbit mode");
			FontSetSys(F_6x8);
			draw_str(LCD_WIDTH / 2, LCD_HEIGHT - 12, "Killer Rabbit Unleashed");
		}
	}
}
