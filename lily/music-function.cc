/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 2004--2015 Han-Wen Nienhuys <hanwen@xs4all.nl>

  LilyPond is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  LilyPond is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with LilyPond.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "music-function.hh"
#include "lily-parser.hh"
#include "input.hh"
#include "music.hh"
#include "fluid.hh"

const char Music_function::type_p_name_[] = "ly:music-function?";

/* Print a textual represenation of the smob to a given port.  */
int
Music_function::print_smob (SCM port, scm_print_state *) const
{
  scm_puts ("#<Music function ", port);
  scm_write (get_function (), port);
  scm_puts (">", port);

  /* Non-zero means success.  */
  return 1;
}

// Used for attaching location information to music expressions in
// default arguments and return values.  Music expressions taken from
// the call signature need to be cloned since they are not suitable
// for multiple use.

static SCM
with_loc (SCM arg, Fluid &loc, bool clone = true)
{
  if (Music *m = unsmob<Music> (arg))
    {
      if (clone)
        {
          m = m->clone ();
          arg = m->unprotect ();
        }
      if (Input *in = unsmob<Input> (loc))
        m->set_spot (*in);
    }
  return arg;
}

// A music function call implies walking through the call signature
// and matching the actual argument list to the signature.  This
// process is not 1:1 due to the possible presence of optional
// arguments which are handled quite differently from how GUILE/Scheme
// usually deal with optional arguments.
//
// The argument matching here intentionally closely tracks the
// semantics of calls via the LilyPond parser as described in
// <URL:lilypond.org/doc/Documentation/extending/scheme-function-usage>:
// if an optional argument predicate does not match the next argument
// from the actual argument list, the default given in the call
// signature is used instead and all following optional arguments are
// unconditionally substituted in a similar manner.
//
// This skipping of optional arguments can be explicitly initiated by
// using \default in LilyPond.  The respective value to use for a call
// via Scheme is *unspecified*.

SCM
Music_function::call (SCM rest)
{
  Fluid location (ly_lily_module_constant ("%location"));

  // (car (ly:music-signature self_scm())) is the return type, skip it
  SCM signature = scm_cdr (get_signature ());

  // In order to keep the loop structure and particularly the handling
  // of terminating conditions reasonably straightforward and avoid
  // scattering the code around, we don't use a separate loop when
  // skipping optional arguments.  Instead we use "skipping" to
  // indicate whether we are currently filling in optional arguments
  // from their defaults.
  bool skipping = false;

  // The main loop just processes the signature in sequence, and the
  // resulting actual arguments are accumulated in reverse order in args

  SCM args = SCM_EOL;

  for (; scm_is_pair (signature); signature = scm_cdr (signature))
    {
      SCM pred = scm_car (signature);

      if (scm_is_pair (pred))
        {
          // If the predicate is not a function but a pair, it
          // signifies an optional argument.  This is not quite the
          // form declared to define-music-function (which is always
          // a proper list) but a pair of predicate function and
          // default value.

          if (!skipping)
            {
              if (!scm_is_pair (rest))
                scm_wrong_num_args (self_scm ());

              SCM arg = scm_car (rest);
              if (scm_is_true (scm_call_1 (scm_car (pred), arg)))
                {
                  args = scm_cons (arg, args);
                  rest = scm_cdr (rest);
                  continue;
                }
              skipping = true;
              // Remove at most one tentative "\default"
              if (scm_is_eq (SCM_UNSPECIFIED, arg))
                rest = scm_cdr (rest);
            }
          args = scm_cons (with_loc (scm_cdr (pred), location), args);
          continue;
        }

      // We have a mandatory argument here.
      skipping = false;

      if (!scm_is_pair (rest))
        scm_wrong_num_args (self_scm ());

      SCM arg = scm_car (rest);
      rest = scm_cdr (rest);
      args = scm_cons (arg, args);

      if (scm_is_false (scm_call_1 (pred, arg)))
        {
          scm_call_4 (ly_lily_module_constant ("argument-error"),
                      location,
                      scm_from_int (scm_ilength (args)),
                      pred, arg);
          SCM val = scm_car (get_signature ());
          val = scm_is_pair (val) ? scm_cdr (val) : SCM_BOOL_F;
          return with_loc (val, location);
        }
    }

  if (scm_is_pair (rest))
    scm_wrong_num_args (self_scm ());

  SCM res = scm_apply_0 (get_function (), scm_reverse_x (args, SCM_EOL));

  SCM pred = scm_car (get_signature ());
  // The return type predicate may have the form of a pair in which
  // the car is the actual predicate and the cdr is the surrogate
  // return value in the error case, to be extracted by
  // music-function-call-error.
  if (scm_is_pair (pred))
    pred = scm_car (pred);

  if (scm_is_true (scm_call_1 (pred, res)))
    return with_loc (res, location, false);

  return scm_call_2 (ly_lily_module_constant ("music-function-call-error"),
                     self_scm (), res);
}
