/*
 *  Copyright (c) 2009 Holger Hans Peter Freyther <zecke@openmoko.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <lsearcher.h>
#include "search.h"
#include "msg.h"
#include "keyboard.h"

#include <guilib.h>
#include <glyph.h>

static lindex global_search;
static struct search_state state;
static struct search_state last_first_hit;

static int search_index;
static char search_string[MAXSTR];

static char first_hit = 0;
static char trigram_loaded = 0;

static const char search_result[] = "Search results:";
static const int search_result_len = 15;

#define RESULT_START 20
#define RESULT_HEIGHT 10
#define NUMBER_OF_RESULTS 19
#define NUMBER_OF_RESULTS_KEYBOARD 10
#define PIXEL_START (RESULT_START - RESULT_HEIGHT + 2)
static char search_pointers[NUMBER_OF_RESULTS][8];
static int search_found = 0;

/* -1 is unselected */
static int search_current = -1;

static void invert_selection(int old_pos, int new_pos)
{
	int start = PIXEL_START;

	guilib_fb_lock();

	if (old_pos != -1) {
		guilib_invert(start + old_pos * RESULT_HEIGHT, RESULT_HEIGHT);
	}

	if (new_pos != -1 ) {
		guilib_invert(start + new_pos * RESULT_HEIGHT, RESULT_HEIGHT);
	}

	guilib_fb_unlock();
}


void search_init()
{
	int result = load_index(&global_search, "/pedia.idx", "/pedia.jmp");
	if (!result) {
		msg(MSG_ERROR, "Failed to initialize search.\n");
		/* XXX, FIXME, handle the error */
	}
}

int search_load_trigram(void)
{
	if (!trigram_loaded) {
		trigram_loaded = !load_trigram_chunk(&global_search);
	}

	return trigram_loaded;
}

void search_reset()
{
	search_index = 0;
	first_hit = 0;
}

void search_reload()
{
	int old_current = search_current;
	guilib_fb_lock();

	reset_state(&global_search, &state, &last_first_hit);
	search_display_results();
	invert_selection(-1, old_current);
	search_current = old_current;

	guilib_fb_unlock();
}

void search_add(char c)
{
	if (search_index >= MAXSTR - 2)
		return;

	search_string[search_index++] = c;
	search_string[search_index] = '\0';
	if (search_index >= 4) {
		state.pattern_len = search_index;
		if (first_hit) {
			reset_state(&global_search, &state, &last_first_hit);
		}

		first_hit = 0;
		return;
	}

	first_hit = 0;
	prepare_search(&global_search, search_string, &state);
}

/*
 * TODO: Optimize going back... For the first three entries
 * one could remember the &state and then seek back to the
 * position and continue the search from there... For now do it
 * the easy way and wait for user testing.
 */
void search_remove_char(void)
{
	if (search_index == 0)
		return;

	search_index -= 1;

	search_string[search_index] = '\0';
	memset(&state, 0, sizeof(state));
	first_hit = 0;
	prepare_search(&global_search, search_string, &state);
}

char* search_fetch_result()
{
	char *result;

	if (search_index == 0)
		return NULL;

	result = search_fast(&global_search, search_string, &state);
	if (!first_hit) {
		store_state(&global_search, &state, &last_first_hit);
		first_hit = 1;
	}

	return result;
}

/*
 * paint the results of the search...
 * 
 * - Every item gets put at a fixed position.
 * - We will clear the content ony atfer the first the resul was fetched.
 *   This will leave the old content on the screen until we have something
 *   to show.
 */
void search_display_results(void)
{
	char *result;
	int y_pos = 0;
	const int results = keyboard_is_visible() ?
				NUMBER_OF_RESULTS_KEYBOARD : NUMBER_OF_RESULTS;

	guilib_fb_lock();
	search_found = 0;
	search_current = -1;

	while (search_found < results && (result = search_fetch_result())) {
		if (!search_found) {
			guilib_clear();
			render_string(0, 1, 10, search_result, search_result_len);
			y_pos = RESULT_START;
		}

		const int len = strlen(result);
		render_string(0, 1, y_pos, result, len - 7);
		memcpy(&search_pointers[search_found][0], result + len - 6, 8);
		y_pos += RESULT_HEIGHT;
		++search_found;
	}

	if (!search_found) {
		guilib_clear();
		render_string(0, 1, 10, search_result, search_result_len);
	}

	guilib_fb_unlock();
}

void search_select_down(void)
{
	/* bottom reached, not wrapping around */
	if (search_current + 1 == search_found)
		return;
    
	invert_selection(search_current, search_current + 1);
	++search_current;
}

void search_select_up(void)
{
	/* top reached, not wrapping around */
	if (search_current <= 0)
		return;

	invert_selection(search_current, search_current - 1);
	--search_current;
}

const char *search_current_target(void)
{
	if (search_current < 0 || search_current >= search_found)
		return NULL;

	return &search_pointers[search_current][0];
}

/*
 * We get the title by seeking back to the start and search again
 * and copying out the result we want. The question is if this is
 * faster or always copying the text. For now we try this approach
 * because the blocks should be already cached.
 *
 */
const char *search_current_title(void)
{
	static char current_title[MAXSTR];
	char *result = NULL;
	int number = 0;

	if (search_found < 0 || search_current >= search_found)
		return NULL;

	reset_state(&global_search, &state, &last_first_hit);
	while (number < NUMBER_OF_RESULTS && (result = search_fetch_result())) {
		if (number == search_current) {
			memcpy(&current_title[0], result, MAXSTR);
			return &current_title[0];
		}

		++number;
	}

	return NULL;
}

/*
 *  - Check which item was hit
 *  - Update the selection so search_current_title returns the
 *    right item.
 *  - return the right item
 *
 *  In practice updating search_current depending on the x,y
 *  should be enough and then search_current_target can be returned.
 */
const char *search_release(int x, int y)
{
	int i;
	int result_start = PIXEL_START;

	for (i = 0; i < search_found; ++i, result_start += RESULT_HEIGHT) {
		if (y >= result_start && y < result_start + RESULT_HEIGHT) {
			search_current = i;
			return search_current_target();
		}
	}

	return NULL;
}
