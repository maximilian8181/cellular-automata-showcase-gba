#include <tonc.h>

#define NUM_CELLS 101
#define NUM_BITS 8
#define NUM_RULES 6

typedef enum {
	SHOW,
	FADE_OUT,
	FADE_IN
} Transition;

u8 calculateState(u8 a, u8 b, u8 c, int rule);
int getBit(int rule, int position);

int main(void)
{
	irq_init(NULL);
	irq_enable(II_VBLANK);

	u8 automaton[NUM_CELLS] = {0};
	automaton[NUM_CELLS >> 1] = 1;
	u8 generation_buffer[NUM_CELLS] = {0};

	u8 left = 0;
	u8 center = 0;
	u8 right = 0;

	u8 next_cell_state = 0;
	const int generations = 32;
	int automaton_rules[NUM_RULES] = {30, 90, 94, 109, 110, 150};
	int rule_bits_to_display[NUM_BITS] = {0, 0, 0, 1, 1, 1, 1, 0};

	u8 grids[NUM_RULES][generations][NUM_CELLS];
	int color_intensities_1[32] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 
		16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
		29, 30, 31
	};
	int color_intensities_2[32] = {
		31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18,
		17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4,
		3, 2, 1, 0
	};

	int half_screen_width = SCREEN_WIDTH >> 1;
	int half_screen_height = SCREEN_HEIGHT >> 1;
	int half_generation_anchor = generations >> 1;
	int color_temp_1 = 0;
	int color_temp_2 = 0;

	int current_rule_index = 0;
	int color_frames_1 = 0;
	int color_frames_2 = 0;
	int fade_out_frames = 0;
	int fade_in_frames = 0;

	int bit_render_offset = 0;
	int half_bit_screen_anchor = (NUM_BITS << 3) >> 1;

	FIXED ey = 0;

	Transition tr = SHOW;
	s8 left_or_right = 0;

	REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;
	REG_BLDCNT = BLD_BUILD(BLD_BG2, BLD_OFF, 3);

	m3_fill(CLR_GRAY);

	for(int first_run = 0; first_run < NUM_CELLS; first_run++) {
		grids[0][0][first_run] = automaton[first_run];
		grids[1][0][first_run] = automaton[first_run];
		grids[2][0][first_run] = automaton[first_run];
		grids[3][0][first_run] = automaton[first_run];
		grids[4][0][first_run] = automaton[first_run];
		grids[5][0][first_run] = automaton[first_run];
	}

	for(int r = 0; r < NUM_RULES; r++) {
		for(int i = 1; i < generations; i++) {
			for(int k = 0; k < NUM_CELLS; k++) {
				if(k == 0) {
					left = automaton[NUM_CELLS - 1];
					center = automaton[0];
					right = automaton[k + 1];
				}
				else if(k == NUM_CELLS - 1) {
					left = automaton[k - 1];
					center = automaton[k];
					right = automaton[0];
				}
				else {
					left = automaton[k - 1];
					center = automaton[k];
					right = automaton[k + 1];
				}
				next_cell_state = calculateState(left, center, right, automaton_rules[r]);
				generation_buffer[k] = next_cell_state;
				grids[r][i][k] = generation_buffer[k]; 
			}
			for(int q = 0; q < NUM_CELLS; q++) {
				automaton[q] = generation_buffer[q];
			}
		}
		for(int cells = 0; cells < NUM_CELLS; cells++) {
			automaton[cells] = 0;
		}
		automaton[NUM_CELLS >> 1] = 1;
	}

	while (1) {
		VBlankIntrWait();

		key_poll();

		if(tr == SHOW) {
			if(key_hit(KEY_R)) {
				left_or_right = 1;
				tr = FADE_OUT;
			}
			if(key_hit(KEY_L)) {
				left_or_right = -1;
				tr = FADE_OUT;
			}

			color_frames_1++;

			if(color_frames_1 > 3) {
				color_temp_1 = color_intensities_1[0];

				for(int color_index_1 = 1; color_index_1 <= 31; color_index_1++) {
					color_intensities_1[color_index_1 - 1] = color_intensities_1[color_index_1];
				}

				color_intensities_1[31] = color_temp_1;
				color_frames_1 = 0;
			}

			color_frames_2++;

			if(color_frames_2 > 1) {
				color_temp_2 = color_intensities_2[31];

				for(int color_index_2 = 30; color_index_2 >= 0; color_index_2--) {
					color_intensities_2[color_index_2 + 1] = color_intensities_2[color_index_2];
				}

				color_intensities_2[0] = color_temp_2;
				color_frames_2 = 0;
			}
		}
		else if(tr == FADE_OUT) {
			fade_out_frames++;

			if(fade_out_frames > 1) {
				ey = (ey < 17) ? ey + 1 : 16;
				fade_out_frames = 0;
			}

			if(ey == 16) {
				if(left_or_right == 1) {
					current_rule_index = (current_rule_index >= 5) ? 0 : current_rule_index + 1;
					for(int bit_in_rule = 0; bit_in_rule < NUM_BITS; bit_in_rule++) {
						rule_bits_to_display[bit_in_rule] = getBit(automaton_rules[current_rule_index], bit_in_rule);
					}
					tr = FADE_IN;
				}
				else {
					current_rule_index = (current_rule_index <= 0) ? 5 : current_rule_index - 1;
					for(int bit_in_rule = 0; bit_in_rule < NUM_BITS; bit_in_rule++) {
						rule_bits_to_display[bit_in_rule] = getBit(automaton_rules[current_rule_index], bit_in_rule);
					}
					tr = FADE_IN;
				}
			}
		}
		else {
			fade_in_frames++;

			if(fade_in_frames > 1) {
				ey = (ey >= 0) ? ey - 1 : 0;
				fade_in_frames = 0;
			}

			if(ey == 0) {
				left_or_right = 0;
				tr = SHOW;
			}
		}

		for(int bit = 0; bit < NUM_BITS; bit++) {
			COLOR bit_color = (rule_bits_to_display[bit] == 1) ? RGB15(10, 23, 25) : CLR_NAVY;
			m3_rect((bit_render_offset + (half_screen_width - 50)) - half_bit_screen_anchor + 23, half_screen_height + 40, ((half_screen_width - 50) + (8 + bit_render_offset)) - half_bit_screen_anchor + 23, half_screen_height + 48, bit_color);
			bit_render_offset = bit_render_offset + 16;
		}

		bit_render_offset = 0;

		for(int y = 0; y < generations; y++) {
			for(int x = 0; x < NUM_CELLS; x++) {
				if(grids[current_rule_index][y][x] == 1) {
					m3_plot(x + (half_screen_width - 50), y + (half_screen_height - half_generation_anchor), RGB15(0, color_intensities_1[y], 0));
				}
				else {
					m3_plot(x + (half_screen_width - 50), y + (half_screen_height - half_generation_anchor), RGB15(0, 0, color_intensities_2[y]));
				}
			}
		}

		REG_BLDY = BLDY_BUILD(ey);
	}
}

u8 calculateState(u8 a, u8 b, u8 c, int rule) {
	u8 sum = (a << 2) + (b << 1) + (c << 0);
	u8 state = 0;

	switch(sum) {
		case 0:
			if(rule == 109) {
				state = 1;
			}
			else {
				state = 0;
			}
			break;
		case 1:
			if(rule == 109) {
				state = 0;
			}
			else {
				state = 1;
			}
			break;
		case 2:
			if(rule == 90) {
				state = 0;
			}
			else {
				state = 1;
			}
			break;
		case 3:
			if(rule == 150) {
				state = 0;
			}
			else {
				state = 1;
			}
			break;
		case 4:
			if(rule == 109 || rule == 110) {
				state = 0;
			}
			else {
				state = 1;
			}
			break;
		case 5:
			if(rule == 109 || rule == 110) {
				state = 1;
			}
			else {
				state = 0;
			}
			break;
		case 6:
			if(rule == 30 || rule == 150) {
				state = 0;
			}
			else {
				state = 1;
			}
			break;
		case 7:
			if(rule == 150) {
				state = 1;
			}
			else {
				state = 0;
			}
			break;
		default:
			break;
	}

	return state;
}

int getBit(int rule, int position) {
	return (rule >> (7 - position)) & 1;
}