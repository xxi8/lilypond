%% DO NOT EDIT this file manually; it was automatically
%% generated from the LilyPond Snippet Repository
%% (http://lsr.di.unimi.it).
%%
%% Make any changes in the LSR itself, or in
%% `Documentation/snippets/new/`, then run
%% `scripts/auxiliar/makelsr.pl`.
%%
%% This file is in the public domain.

\version "2.25.1"

\header {
  lsrtags = "expressive-marks"

  texidoc = "
The shorthands are defined in @samp{ly/script-init.ly}, where the
variables @code{dashHat}, @code{dashPlus}, @code{dashDash},
@code{dashBang}, @code{dashLarger}, @code{dashDot}, and
@code{dashUnderscore} are assigned default values.  The default values
for the shorthands can be modified. For example, to associate the
@code{-+} (@code{dashPlus}) shorthand with the @emph{trill} symbol
instead of the default @emph{+} symbol, assign the value @code{\\trill}
to the variable @code{dashPlus}:
"

  doctitle = "Modifying default values for articulation shorthand notation"
} % begin verbatim


\paper { tagline = ##f }

\relative c'' { c1-+ }

dashPlus = \trill

\relative c'' { c1-+ }
