/* shtools.c */
/* signal handling tools */

/* TaniDVR
 * Copyright (c) 2011-2015 Daniel Mealha Cabrita
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "shtools.h"
#include "log.h"

/* boolean flags set upon signal receiving.
   those are READ-ONLY outside the signal handling routines scope */
volatile sig_atomic_t sht_fl_sigchld;
volatile sig_atomic_t sht_fl_sigpipe;
volatile sig_atomic_t sht_fl_terminate_nicely;

static struct sigaction sht_sa;
static struct sigaction sht_sa_fatal;
static struct sigaction sht_sa_ignored;

static struct sigaction sht_sa_tmp;
static struct sigaction sht_sa_fatal_tmp;
static struct sigaction sht_sa_ignored_tmp;

/* context (what kind of process is this) */
static sht_context_t sht_context;

static bool sht_initialized_once = false;

void sht_handler (int sn)
{
	switch (sn) {
	case SIGCHLD:
		log_printf (LOGT_DETAIL, "Got SIGCHLD.\n");
		sht_fl_sigchld = 1;
		break;
	case SIGPIPE:
		log_printf (LOGT_DETAIL, "Got SIGPIPE.\n");
		sht_fl_sigpipe = 1;
		break;
	case SIGTERM:
	case SIGHUP:
	case SIGXCPU:
		log_printf (LOGT_DETAIL, "Got SIGTERM|SIGHUP|SIGXCPU.\n");
		sht_fl_terminate_nicely = 1;
		break;
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
	case SIGBUS:
		log_printf (LOGT_FATAL, "SOFTWARE FAILURE (got signal: %d).\n", sn);
		abort ();
		break;
	}
	return;
}

/* this applies ONLY to the managed signals. */
/* do NOT call this before sht_init(). */
void sht_set_sighandlers (void)
{
	struct sigaction *sa = &sht_sa;
	struct sigaction *sa_fatal = &sht_sa_fatal;
	struct sigaction *sa_ignored = &sht_sa_ignored;
	struct sigaction *sa_tmp = &sht_sa_tmp;
	struct sigaction *sa_fatal_tmp = &sht_sa_fatal_tmp;
	struct sigaction *sa_ignored_tmp = &sht_sa_ignored_tmp;

	/* this may be unnecessary, but let's take no risks
	   with sigaction() behavior */
	if (sht_initialized_once == false) {
		sa = NULL;
		sa_fatal = NULL;
		sa_ignored = NULL;
		sa_tmp = &sht_sa;
		sa_fatal_tmp = &sht_sa_fatal;
		sa_ignored_tmp = &sht_sa_ignored;
	}

	sigaction (SIGCHLD, sa_tmp, sa);

	/* deal with broken pipes with IO functions instead */
	sigaction (SIGPIPE, sa_ignored_tmp, sa_ignored);

	/* "terminate nicely" signals */
	sigaction (SIGHUP, sa_tmp, sa);
	sigaction (SIGTERM, sa_tmp, sa);
	sigaction (SIGXCPU, sa_tmp, sa);

	/* intercept software failure */
	sigaction (SIGILL, sa_fatal_tmp, sa_fatal);
	sigaction (SIGFPE, sa_fatal_tmp, sa_fatal);
	sigaction (SIGSEGV, sa_fatal_tmp, sa_fatal);
	sigaction (SIGBUS, sa_fatal_tmp, sa_fatal);
}

/* blocks/unblocks all managed signals (see parm 'sf').
   op: SHT_OP_BLOCK  or SHT_OP_UNBLOCK
   sf: which signals to manage (from the already managed ones). */
void sht_signalblock_mgr (sht_sigmgr_op_t op, uint32_t sf)
{
	sigset_t bmask;

	sigemptyset (&bmask);
	if (sf & SHT_F_SIGBLOCK_SIGSTD) {
		sigaddset (&bmask, SIGCHLD);
		sigaddset (&bmask, SIGTERM);
		sigaddset (&bmask, SIGHUP);
		sigaddset (&bmask, SIGXCPU);
		sigaddset (&bmask, SIGPIPE);
	}
	if (sf & SHT_F_SIGBLOCK_SIGFATAL) {
		sigaddset (&bmask, SIGILL);
		sigaddset (&bmask, SIGFPE);
		sigaddset (&bmask, SIGSEGV);
		sigaddset (&bmask, SIGBUS);
	}
	sigprocmask (((op == SHT_OP_BLOCK) ? SIG_BLOCK : SIG_UNBLOCK), &bmask, NULL);
}

/* call this before anything */
void sht_init (sht_context_t ctx)
{
	struct sigaction *sa = &sht_sa_tmp;
	struct sigaction *sa_fatal = &sht_sa_fatal_tmp;
	struct sigaction *sa_ignored = &sht_sa_ignored_tmp;

	/* this may be unnecessary, but let's take no risks
	   with sigaction() behavior */
	if (sht_initialized_once == false) {
		sa = &sht_sa;
		sa_fatal = &sht_sa_fatal;
		sa_ignored = &sht_sa_ignored;
	}

	/* block managed signals including the fatal ones */
	sht_signalblock_mgr (SHT_OP_BLOCK, (SHT_F_SIGBLOCK_SIGSTD | SHT_F_SIGBLOCK_SIGFATAL));

	sht_context = ctx;

	/* new context information is set, unblock the fatal flags now */
	sht_signalblock_mgr (SHT_OP_UNBLOCK, (SHT_F_SIGBLOCK_SIGFATAL));

	/* reset flags */
	sht_fl_sigchld = 0;
	sht_fl_sigpipe = 0;
	sht_fl_terminate_nicely = 0;

	/* context-agnostic parms for sht_sa */
	sa->sa_handler = sht_handler;
	sigemptyset (&(sa->sa_mask));

	/* context-agnostic parms for sht_sa_fatal */
	sa_fatal->sa_handler = sht_handler;
	sa_fatal->sa_flags = SA_RESETHAND | SA_NODEFER;	/* make software break at once if it gets 2x a fatal signal */
	/* its a fatal situation, forget the rest
	   of the signals we usually intercept. */
	sigemptyset (&(sa_fatal->sa_mask));
	sigaddset (&(sa_fatal->sa_mask), SIGCHLD);
	sigaddset (&(sa_fatal->sa_mask), SIGTERM);
	sigaddset (&(sa_fatal->sa_mask), SIGHUP);
	sigaddset (&(sa_fatal->sa_mask), SIGXCPU);
	sigaddset (&(sa_fatal->sa_mask), SIGPIPE);

	/* context-agnostic parms for sht_sa_ignored */
	sa_ignored->sa_handler = SIG_IGN;
	sa_ignored->sa_flags = 0;
	sigemptyset (&(sa_ignored->sa_mask));

	/* set context-specific struct sigaction parms */
	switch (ctx) {
	case SHT_BASE_PROC:
	case SHT_BUFFER_PROC:
		sa->sa_flags = 0;
		break;
	case SHT_DVRCTL_PROC:
		/*dvrctl IO does not deal well with interruptions */
		sa->sa_flags = SA_RESTART;

		/* FIXME: kludge to retain capability to quit,
		   since this process does not care when asked nicely by SIGTERM. */
		sa->sa_handler = SIG_DFL;
		break;
	}

	/* assign signals */
	sht_set_sighandlers ();

	sht_initialized_once = true;

	/* unblock managed signals */
	sht_signalblock_mgr (SHT_OP_UNBLOCK, (SHT_F_SIGBLOCK_SIGSTD));
}

