/****************************************************************
 * This file is part of the "2048 Console Clone" game.
 *
 * Author:       migf1 <mig_f1@hotmail.com>
 * Version:      0.3a
 * Date:         July 7, 2014
 * License:      Free Software (see following comments for limitations)
 * Dependencies: common.h, board.h, gs.h, mvhist.h, tui.h
 * --------------------------------------------------------------
 *
 * Description
 * -----------
 *
 * A console clone of the game 2048 ( http://gabrielecirulli.github.io/2048/ )
 * written in ISO C99. It is meant to be cross-platform across Windows, Unix,
 * Linux, and MacOSX (for the latter 3, you should enable ANSI-colors support
 * on your terminal emulation).
 *
 * Compared to the original game, this version additionally supports:
 * - skins (color themes)
 * - undo/redo (it disables best-score tracking)
 * - replays
 * - load/save games (via replays)
 *
 * Moreover, this version of the game is also cloning
 * 3 unofficial variants of the original game, namely:
 *
 * - 5x5 board ( http://2048game.com/variations/5x5.html )
 * - 6x6 board ( http://2048game.com/variations/6x6.html )
 * - 8x8 board ( http://2048game.com/variations/8x8.html )
 *
 * License
 * -------
 *
 * The game is open-source, free software with only 3 limitations:
 *
 * 1. Keep it free and open-source.
 * 2. Do not try to make any kind of profit from it or from any
 *    derivatives of it, unless you have contacted me for special
 *    arrangements ( mig_f1@hotmail.com ).
 * 3. Always re-distribute the original package, along with any
 *    software you distribute that is based on this game.
 *
 * Disclaimer
 * ----------
 *
 * Use the program at your own risk! I take no responsibility for
 * any damage it may cause to your system.
 ****************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "common.h"   /* constants, macros, unctions common across all files*/
#include "board.h"    /* board related functions */
#include "gs.h"       /* game-state */
#include "mvhist.h"   /* moves history (undo, redo, replay) */
#include "tui.h"      /* text-user-interface */

/* Macro for validating an input key as a command
 * for starting a new variant of the game.
 */
#define VALID_VARIANT_KEY(key)      \
(                                   \
	TUI_KEY_BOARD_4 == (key)    \
	|| TUI_KEY_BOARD_5 == (key) \
	|| TUI_KEY_BOARD_6 == (key) \
	|| TUI_KEY_BOARD_8 == (key) \
)

/* Macro for converting an input key to a GS_NEXTMOVE direction.
 */
#define KEY_TO_MVDIR(key)                                   \
(                                                           \
	(key) == TUI_KEY_UP                                 \
		? GS_NEXTMOVE_UP                            \
		: (key) == TUI_KEY_DOWN                     \
			? GS_NEXTMOVE_DOWN                  \
			: (key) == TUI_KEY_LEFT             \
				? GS_NEXTMOVE_LEFT          \
				: (key) == TUI_KEY_RIGHT    \
					? GS_NEXTMOVE_RIGHT \
					: GS_NEXTMOVE_NONE  \
)

/* --------------------------------------------------------------
 * char *_fname_from_clock():
 *
 * --------------------------------------------------------------
 */
char *_fname_from_clock( char *fname )
{
	size_t idxbeg = strlen( REPLAYS_FOLDER "/");/*start of pure fname*/
	size_t szfname = SZMAX_FNAME - idxbeg;      /*avail sz for pure fname*/
	char *cp = NULL;

	if ( NULL == fname ) {
		DBGF( "%s", "NULL pointer argument (fname)!" );
		return NULL;
	}

	time_t nowtime = time( NULL );
	struct tm *tm = localtime( &nowtime );

	strncpy( fname, REPLAYS_FOLDER "/", SZMAX_FNAME-1 );
	fname[ SZMAX_FNAME-1 ] = '\0';

	cp = &fname[idxbeg];
	strncpy( cp, asctime(tm), szfname-1 );
	cp[ szfname-1 ] = '\0';

	s_trim( cp );
	s_strip( cp, ":" );
	s_char_replace( cp, ' ', '_' );
	strncat( cp, REPLAY_FNAME_EXT, szfname-1 );

	return fname;
}

/* --------------------------------------------------------------
 * int _do_play_board():
 *
 * Play the move indicated by the key argument, on the board of the
 * specified game-state (gs), and update accordingly the game-state,
 * the specified text-user-interface (tui), and the specified moves
 * history (mvhist).
 *
 * Return 1 (true) if the move caused game-over, 0 (false) otherwise.
 *
 * NOTE: A game is over either when a tile reaches the sentinel-value
 *       (iswin) or when the board is full and there are no adjacent
 *       tiles with equal, non-0, values.
 * --------------------------------------------------------------
 */
static int _do_play_board(
	int          key,
	GameState    *gs,
	MovesHistory *mvhist,
	Tui          *tui
	)
{
	int moved = 0;    /* was a move successfully played? */
	int iswin = 0;    /* was a winning move played? */

	/* just for brevity later on */
	Board *board;
	long int score, bscore;
	int didundo;

	if ( NULL == gs || NULL == tui || NULL == mvhist ) {
		DBGF( "%s", "NULL pointer argument!" );
		return 1;
	}

	/* do the move */
	board  = gamestate_get_board( gs );
	score  = gamestate_get_score( gs );
	bscore = gamestate_get_bestscore( gs );
	switch( key )
	{
		case TUI_KEY_UP:
			moved = board_move_up( board, &score, &iswin );
			break;

		case TUI_KEY_DOWN:
			moved = board_move_down( board, &score, &iswin );
			break;

		case TUI_KEY_LEFT:
			moved = board_move_left( board, &score, &iswin );
			break;

		case TUI_KEY_RIGHT:
			moved = board_move_right( board, &score, &iswin );
			break;

		default:
			moved = 0;  /* false */
			break;
		}

	/* update gametstae's score, and if needed best-score too */
	didundo = mvhist_get_didundo( mvhist );
	gamestate_set_score(gs, score);
	if ( !didundo && bscore < score ) {
		gamestate_set_bestscore( gs, score ) ;
	}

	/* update gamestate's prevmv */
	gamestate_set_prevmove( gs, KEY_TO_MVDIR(key) );

	/* was it a winning move? */
	if ( iswin ) {
		mvhist_push_undo_stack( mvhist, gs );
		tui_draw_infobar_winmsg( tui );
	}
	/* moved but not won? */
	else if ( moved ) {
		board_generate_ntiles(
			board,
			board_get_nrandom( board )
			);
		mvhist_push_undo_stack( mvhist, gs );
	}

	/* is current game over? */
	return iswin
	       ||
	       !( board_has_adjacent(board) || board_has_room(board) )
	       ;
}

/* --------------------------------------------------------------
 * void _do_reset_game():
 *
 * Start a new game, keeping the current settings of the specified
 * game-state object (gs). Update accordingly the specified moves
 * history (mvhist) and text-user-interface (tui).
 * --------------------------------------------------------------
 */
static void _do_reset_game( GameState *gs, MovesHistory *mvhist, Tui *tui)
{
	if ( NULL == gs || NULL == mvhist || NULL == tui ) {
		DBGF( "%s", "NULL pointer argument!" );
		return;
	}

	if ( 'y' == tolower( tui_draw_iobar_prompt_newgame(tui)) ) {
		gamestate_reset( gs );

		mvhist_reset( mvhist );
		mvhist_push_undo_stack( mvhist, gs );

		tui_clear_infobar( tui );
	}
}

/* --------------------------------------------------------------
 * void _do_new_variant():
 *
 * Start a new variant of the game, according to the specified key.
 * The specified game-state (gs), text-user-interface (tui) and
 * moves-history (mvhist), are updated accordingly.
 *
 * NOTES: ( IMPORTANT! )
 *
 *        Launching a new variant of the game, is usually requiring
 *        to also resize the board. Resizing is done in the function
 *        board_resize_and_reset(), via realloc(), which does NOT
 *        guarantee that the resized board will stay at the same
 *        location in memory.
 *
 *        Thus, it is IMPORTANT to call the function:
 *        tui_update_board_reference() AFTER the board has been
 *        resized, so the text-user-interface gets aware of the
 *        new memory location of the resized board.
 * --------------------------------------------------------------
 */
static void _do_new_variant( int key,
	GameState    *gs,
	MovesHistory *mvhist,
	Tui          *tui
	)
{
	int dim = key - '0';   /* the new single-dimension of the board */
	Board *board;          /* just for brevity later on */

	if ( NULL == gs || NULL == mvhist || NULL == tui ){
		DBGF( "%s", "NULL pointer argument!" );
		return;
	}
	if ( !VALID_VARIANT_KEY(key) ){
		DBGF( "%c (%d) is not a valid variant key!", key, key );
		return;
	}

	board = gamestate_get_board( gs );
	if ( dim != board_get_dim(board) )
	{
		/* prompt for new-game confirmation */
		if ( 'y' == tolower( tui_draw_iobar_prompt_newgame(tui)) )
		{
			board_resize_and_reset( board, dim );
			board_generate_ntiles(
				board,
				2 * board_get_nrandom( board )
				);

			gamestate_set_score( gs, 0 );

			mvhist_reset( mvhist );
			mvhist_push_undo_stack( mvhist, gs );

			tui_update_board_reference( tui, board );
			tui_cls( tui );
		}
	}
}

/* --------------------------------------------------------------
 * void _do_undo():
 *
 * Undo the last move on the board, and update accordingly the
 * specified game-state (gs), text-user-interface (tui), and
 * moves-history (mvhist).
 *
 * NOTES: The very first move (the random generation of the initial
 *        tiles) is done automatically by the game, so it cannot be
 *        undone.
 *
 *        Also, the last move of the game (that is, the move that
 *        causes game-over) cannot be undone.
 *
 *        The first time the player tries to undo his move, he is
 *        asked for confirmation because undo cancels the recording
 *        of the best-score. Subsequent undoing is done without
 *        confirmation.
 *
 *        Best-score cancellation is achieved via a boolean flag
 *        inside the specified moves-history (mvhist), called: didundo.
 *        If the player confirms the first undo, this flag is set
 *        to 1 (true). The function _do_play_board() which updates
 *        the best-score, does so only if this flag is set to 0
 *        (false).
 *
 * --------------------------------------------------------------
 */
static void _do_undo( GameState *gs, MovesHistory *mvhist, Tui *tui)
{
	const GameState *prevgs = NULL;  /* previous game-state */

	if ( NULL == gs || NULL == mvhist || NULL == tui) {
		DBGF( "%s", "NULL pointer argument!" );
		return;
	}

	/* 1st move was done automatically by the game, so ignore it */
	if ( mvhist_isempty_undo_stack( mvhist )
	|| mvhist_peek_undo_stack_count(mvhist) < 2
	){
		tui_sys_beep(1);
		return;
	}

	/* the very 1st time ask for confirmation */
	if ( 0 == mvhist_get_didundo(mvhist) ) {
		if ( 'y' != tolower(tui_draw_iobar_prompt_undo(tui)) ){
			return;
		}
		//gsstack_free( &redo );
	}

	/* remember that the player has done at least 1 undo */
	mvhist_set_didundo( mvhist, 1 );  /* true */

	/* push current game-state onto the redo-stack */
	mvhist_push_redo_stack( mvhist, gs );

	/* remove recorded current-state from undo-stack  */
	mvhist_pop_undo_stack( mvhist );

	/* get previous game-state from undo-stack, and apply it */
	prevgs = mvhist_peek_undo_stack_state( mvhist );
	if ( NULL != prevgs ) {
		gamestate_copy( gs, prevgs );
	}
}

/* --------------------------------------------------------------
 * void _do_redo():
 *
 * Redo the last undone move on the board, and update accordingly
 * the specified game-state (gs), moves-history (mvhist) and text-
 * -user-interface (tui).
 * --------------------------------------------------------------
 */
static void _do_redo( GameState *gs, MovesHistory *mvhist, Tui *tui)
{
	const GameState *nextgs = NULL;   /* next game-state */

	if ( NULL == gs || NULL == mvhist || NULL == tui ) {
		DBGF( "%s", "NULL pointer argument!" );
		return;
	}

	/* no undo has been done ? */
	if ( 0 == mvhist_get_didundo( mvhist )
	|| mvhist_isempty_redo_stack( mvhist )
	){
		tui_sys_beep(1);
		return;
	}

	/* get next game-state from the redo stack, apply it & pop it out */
	nextgs = mvhist_peek_redo_stack_state( mvhist );
	gamestate_copy( gs, nextgs );
	mvhist_pop_redo_stack( mvhist );

	/* push the redone game-state onto the undo-stack */
	mvhist_push_undo_stack( mvhist, gs );
}

/* --------------------------------------------------------------
 * void _do_replay_end():
 *
 * --------------------------------------------------------------
 */
static inline void _do_replay_end(
	const GSNode **it,
	GameState    *gs,
	MovesHistory *mvhist,
	Tui          *tui
	)
{
	if ( gsstack_peek_count(*it) == 1 ) {
		tui_sys_beep(1);
		return;
	}

	*it = mvhist_iter_bottom_replay_stack( mvhist );
	gamestate_copy( gs, gsstack_peek_state(*it) );
	tui_redraw( tui, 0 );  /* 0: disabled commands in help-box */
}

/* --------------------------------------------------------------
 * void _do_replay_beg():
 *
 * --------------------------------------------------------------
 */
static inline void _do_replay_beg(
	const GSNode **it,
	GameState    *gs,
	MovesHistory *mvhist,
	Tui          *tui
	)
{
	if ( gsstack_peek_count(*it) == mvhist_get_replay_nmoves(mvhist) ) {
		tui_sys_beep(1);
		return;
	}

	*it = mvhist_iter_top_replay_stack( mvhist );
	gamestate_copy( gs, gsstack_peek_state(*it) );
	tui_redraw( tui, 0 );  /* 0: disabled commands in help-box */
}

/* --------------------------------------------------------------
 * void _do_replay_next():
 *
 * --------------------------------------------------------------
 */
static inline void _do_replay_next(
	const GSNode **it,
	GameState    *gs,
	MovesHistory *mvhist,
	Tui          *tui
	)
{
	if ( gsstack_peek_count(*it) == 1 ) {
		tui_sys_beep(1);
		return;
	}

	*it = mvhist_iter_down_replay_stack( mvhist, *it );
	gamestate_copy( gs, gsstack_peek_state(*it) );
	tui_redraw( tui, 0 );  /* 0: disabled commands in help-box */
}

/* --------------------------------------------------------------
 * void _do_replay_prev():
 *
 * --------------------------------------------------------------
 */
static inline void _do_replay_prev(
	const GSNode **it,
	GameState    *gs,
	MovesHistory *mvhist,
	Tui          *tui
	)
{
	if ( gsstack_peek_count(*it) == mvhist_get_replay_nmoves(mvhist) ) {
		tui_sys_beep(1);
		return;
	}
	*it = mvhist_iter_up_replay_stack( mvhist, *it );
	gamestate_copy( gs, gsstack_peek_state(*it) );
	tui_redraw( tui, 0 );  /* 0: disabled commands in help-box */
}

/* --------------------------------------------------------------
 * void _do_replay_auto():
 *
 * --------------------------------------------------------------
 */
static inline void _do_replay_auto(
	const GSNode **it,
	GameState    *gs,
	MovesHistory *mvhist,
	Tui          *tui
	)
{
	unsigned int delay = 750;      /* msecs to delay between moves */

	if ( gsstack_peek_count(*it) == 1 ) {
		tui_sys_beep(1);
		return;
	}

	while ( NULL != (*it = mvhist_iter_down_replay_stack(mvhist,*it)) ) {
		gamestate_copy( gs, gsstack_peek_state(*it) );

		tui_redraw( tui, 0 );  /* 0: disabled commands in help-box */
		tui_draw_iobar2_replaynavigation( tui );
		tui_draw_iobar_autoreplayinfo( tui );
		tui_sys_sleep( delay );
	}
	*it = mvhist_iter_bottom_replay_stack( mvhist );
}

/* --------------------------------------------------------------
 * void _do_replay_save():
 *
 * --------------------------------------------------------------
 */
static void _do_replay_save( MovesHistory *mvhist, Tui *tui )
{
	char fname[SZMAX_FNAME] = {'\0'};

	_fname_from_clock( fname );
	tui_draw_iobar2_savereplayname( tui, fname );

	if ( 'y' == tolower( tui_draw_iobar_prompt_savereplay(tui) ) )
	{
		if ( !mvhist_save_to_file(mvhist, fname) ) {
			DBGF( "%s", "mvhist_save_to_file() failed" );
			return;
		}
	}
	return;
}

/* --------------------------------------------------------------
 * void _do_replay_load():
 *
 * --------------------------------------------------------------
 */
static void _do_replay_load(
	const GSNode **it,
	GameState    *gs,
	MovesHistory **mvhist,
	Tui          *tui
	)
{
	MovesHistory *tmp = NULL;

	if ( 'y' == tolower( tui_draw_iobar_prompt_loadreplay(tui) ) )
	{
		char fname[SZMAX_FNAME] = {'\0'};

		tui_prompt_replay_fname_to_load( tui, fname );
		if  ( !f_exists(fname) ) {
			goto ret_nofile;
		}

		tmp = new_mvhist_from_file( fname );
		if ( NULL == tmp ) {
			DBGF( "%s", "new_mvhist_from_file() failed!" );
			goto ret_failure;
		}

		*mvhist = mvhist_free( *mvhist );
		*mvhist = tmp;
		*it = mvhist_iter_top_replay_stack( *mvhist );

		gamestate_copy( gs, gsstack_peek_state(*it) );

		tui_update_mvhist_reference( tui, *mvhist );
		tui_update_board_reference(
			tui,
			gamestate_get_board( gs )
			);

		tui_cls( tui );
		tui_redraw( tui, 0 );  /* 0: disabled commands in help-box */
	}

	return;

ret_nofile:
	tui_cls( tui );
	tui_redraw( tui, 0 );  /* 0: disabled commands in help-box */
	tui_draw_iobar_prompt_loadreplay_nofile( tui );
	return;

ret_failure:
	tui_cls( tui );
	tui_redraw( tui, 0 );  /* 0: disabled commands in help-box */
	return;
}

/* --------------------------------------------------------------
 * void _do_replay():
 *
 * Replay all the moves done so far in the current game, without
 * counting those that have been undone.
 *
 * NOTES: Replaying is not implemented efficiently, since it creates
 *        a new stack, populates it, processes it and finally destroys
 *        it, every time this function is called.
 *
 *        An alternative would be to create the replay stack at game
 *        launch and then update it dynamically (like the undo & redo
 *        stacks). Another alternative would be to use a temporal,
 *        dynamically allocated buffer instead of a stack for the
 *        replay in this function (arrays are indexed much faster
 *        than popping linked-lists).
 *
 *        Anyway, what is happening here is that the replay-stack
 *        is created by duplicating in reverse order the undo-stack
 *        (along with some house-keeping) via the function:
 *        mvhist_new_replay_stack().
 *
 *        Then, until the replay-stack gets exhausted, its elements
 *        are popped out one after the other, replacing the current
 *        game-state in the way (with a fixed time delay).
 * --------------------------------------------------------------
 */
static void _do_replay( GameState *gs, MovesHistory **mvhist, Tui *tui )
{
	int key = TUI_KEY_NUL;
	unsigned int keymask;
	unsigned int delay = 750;      /* msecs to delay between moves */
	const GSNode *it   = NULL;     /* iterator for the replay-stack */

	if ( NULL == gs || NULL == mvhist || NULL == tui ) {
		DBGF( "%s", "NULL pointer argument!" );
		return;
	}
	if ( NULL == *mvhist ) {
		DBGF( "%s", "*mvhist is NULL!" );
		return;
	}

	/* no replay if the automatic initial move is the only one so far */
//	if ( mvhist_isempty_undo_stack(mvhist)
//	|| mvhist_peek_undo_stack_count(mvhist) < 2
//	){
//		tui_sys_beep(1);
//		return;
//	}

	mvhist_new_replay_stack( *mvhist, delay );
	it = mvhist_iter_top_replay_stack( *mvhist );
	if ( NULL == it ) {
		DBGF( "%s", "mvist_iter_replay_stack(*mvhist) is NULL!" );
		mvhist_free_replay_stack( *mvhist );
		return;
	}
	gamestate_copy( gs, gsstack_peek_state(it) );
	tui_redraw( tui, 0 );   /* 0: disabled commands in help-box */
	tui_draw_iobar2_replaynavigation( tui );

	for (;;)
	{
		key = toupper(
			tui_draw_iobar_prompt_replaycommand(tui, &keymask)
			);
		if ( TUI_KEY_ESCAPE == key || TUI_KEY_REPLAY_BACK == key ) {
			mvhist_free_replay_stack( *mvhist );
			gamestate_copy(
				gs,
				mvhist_peek_undo_stack_state(*mvhist)
				);
			return;
		}

		/* arrow or special key? */
		else if ( keymask & TUI_KEYMASK_ARROW )
		{
			switch (key)
			{
				case TUI_KEY_RIGHT:
					_do_replay_next(&it, gs, *mvhist, tui);
					break;

				case TUI_KEY_LEFT:
					_do_replay_prev(&it, gs, *mvhist, tui);
					break;

				case TUI_KEY_REPLAY_END:
					_do_replay_end(&it, gs, *mvhist, tui);
					break;

				case TUI_KEY_REPLAY_BEG:
					_do_replay_beg(&it, gs, *mvhist, tui);
					break;
				default:
					break;
			}
		}

		else if ( TUI_KEY_REPLAY_PLAY == key ) {
			_do_replay_auto( &it, gs, *mvhist, tui );
		}

		else if ( TUI_KEY_REPLAY_SAVE == key ) {
			_do_replay_save( *mvhist, tui );
		}
		else if ( TUI_KEY_REPLAY_LOAD == key ) {
			/* NOTE:
			 * We need to pass mvhist (instead of *mvhist)
			 * because _do_replay_load() may allocate a new
			 * object at a different memory location.
			 */
			_do_replay_load( &it, gs, mvhist, tui );
		}

		tui_draw_iobar2_replaynavigation( tui );
	}
}

/* --------------------------------------------------------------
 * void _do_cycle_skin():
 *
 * Apply the next available skin of the specified tui object.
 *
 * NOTE: The skins and their order are fixed. They are enabled
 *       automatically, during the creation of the tui object.
 *       They are implemented separately in the source-module:
 *       tui_skin.c (which is used by tui.c).
 * --------------------------------------------------------------
 */
static void _do_cycle_skin( Tui *tui )
{
	if ( NULL == tui ){
		DBGF( "%s", "NULL pointer argument!" );
		return;
	}

	tui_cycle_skin( tui );
	tui_cls( tui );
}

/* --------------------------------------------------------------
 * void _cleanup():
 *
 * Release the memory reserved for the specified game-state (gs),
 * moves-history (mvhist) and text-user-interface (tui).
 * --------------------------------------------------------------
 */
static void _cleanup( GameState *gs, MovesHistory *mvhist, Tui *tui )
{
	if ( NULL == gs || NULL == mvhist || NULL == tui ) {
		DBGF( "%s", "NULL pointer argument!" );
		return;
	}
	tui_release( tui );
	mvhist_free( mvhist );
	gamestate_free( gs );
}

/* --------------------------------------------------------------
 * void _alloc():
 *
 * Allocate and initialize to default values the memory initially
 * needed at game launch for the specified game-state (gs), moves
 * history (mvhist) and text-user-interface (tui). Return 0 (false)
 * on error, 1 (true) otherwise.
 *
 * Upon success, the addresses of the specified game-state, moves
 * history and text-user-interface, are pointing to the newly created
 * objects in memory.
 *
 * NOTES: The default values for the creation of both the game-state
 *        and the text-user-interface, are the ones corresponding to
 *        the classic, 4x4 board, version of the game.
 * --------------------------------------------------------------
 */
static int _alloc( GameState **gs, MovesHistory **mvhist, Tui **tui )
{
	if ( NULL == gs || NULL == mvhist || NULL == tui ) {
		DBGF( "%s", "NULL pointer argument " );
		return 0;
	}

	*gs = new_gamestate( BOARD_DIM_4 );
	if ( NULL == *gs ) {
		return 0;
	}

	*mvhist = new_mvhist();
	if ( NULL == *mvhist ) {
		*gs = gamestate_free( *gs );
		return 0;
	}

	*tui = new_tui( *gs, *mvhist );
	if ( NULL == *tui ) {
		*mvhist = mvhist_free( *mvhist );
		*gs = gamestate_free( *gs );
		return 0;
	}

	return 1;
}

/* --------------------------------------------------------------
 * Application's entry point.
 * --------------------------------------------------------------
 */
int main( void )
{
	int gameover  = 0;          /* is current game over? */
	int key       = TUI_KEY_NUL;/* user keypress (see tui.h) */
	unsigned int keymask = 0x00;/* indicates Arrows and/or FKeys */
	Tui          *tui = NULL;   /* text user interface */
	GameState    *gs  = NULL;   /* current game-state */
	MovesHistory *mvhist = NULL;/* undo, redo & replay */

	srand( time(NULL) );

	/* allocate initially needed memory */
	if ( !_alloc(&gs, &mvhist, &tui) ) {
		exit( EXIT_FAILURE );
	}

	/* reset game & play automatically initial move */
	gamestate_reset( gs );

	/* reset moves-history & put initial move onto the undo stack */
	mvhist_reset( mvhist );
	mvhist_push_undo_stack( mvhist, gs );

	/* game loop */
	for (;;)
	{
//		dbg_gsnode_dump( redo );

		gameover = 0;         /* reset to false */

		tui_redraw( tui, 1 ); /* 1: enabled commands in help-box */

		key = toupper( tui_sys_getkey(&keymask) );

		/* esc or quit key */
		if ( TUI_KEY_ESCAPE == key || TUI_KEY_QUIT == key ) {
			break;
		}

		/* arrow key */
		else if ( keymask & TUI_KEYMASK_ARROW ) {
			gameover = _do_play_board( key, gs, mvhist, tui);
		}

		/* cycle-skin key */
		else if ( TUI_KEY_SKIN == key ) {
			_do_cycle_skin( tui );
		}

		/* new-game key */
		else if ( TUI_KEY_RESET == key ) {
			_do_reset_game( gs, mvhist, tui );
		}

		/* new-variant key */
		else if ( VALID_VARIANT_KEY(key) ) {
			_do_new_variant( key, gs, mvhist, tui );
		}

		/* undo key */
		else if ( TUI_KEY_UNDO == key ) {
			_do_undo( gs, mvhist, tui );
		}

		/* redo key */
		else if ( TUI_KEY_REDO == key ) {
			_do_redo( gs, mvhist, tui );
		}

		/* replay key */
		else if ( TUI_KEY_REPLAY == key ) {
			_do_replay( gs, &mvhist, tui );
		}

		/* hint key */
		else if ( TUI_KEY_HINT == key ) {
			tui_draw_iobar_prompt_notyet( tui );
		}

		/* is current game over? */
		if ( gameover ) {
			tui_draw_board( tui );
			tui_draw_scoresbar( tui );
			tui_draw_iobar2_movescounter( tui );

			tui_sys_beep(1);
			if ( 'y' == tolower( tui_draw_iobar_prompt_watchreplay(tui)) ) {
				_do_replay(gs, &mvhist, tui);
			}
			if ( 'y' == tolower( tui_draw_iobar_prompt_newgame(tui)) ) {
				gamestate_reset( gs );

				mvhist_reset( mvhist );
				mvhist_push_undo_stack( mvhist, gs );

				tui_clear_infobar( tui );
			}
			else {
				break;
			}
		}

	}

	_cleanup( gs, mvhist, tui );
	exit( EXIT_SUCCESS );
}
