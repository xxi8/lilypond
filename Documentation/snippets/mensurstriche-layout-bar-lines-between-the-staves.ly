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
  lsrtags = "ancient-notation, contexts-and-engravers, staff-notation, tweaks-and-overrides"

  texidoc = "
The @emph{mensurstriche} layout, where bar lines do not appear on
staves but between staves only, can be achieved with a
@code{StaffGroup} instead of a @code{ChoirStaff}.  The bar line on
staves is blanked out using @code{\\hide}.
"

  doctitle = "Mensurstriche layout (bar lines between the staves)"
} % begin verbatim


\layout {
  \context {
    \Staff
    measureBarType = "-span|"
  }
}

music = \fixed c'' {
  c1
  d2 \section e2
  f1 \fine
}

\new StaffGroup <<
  \new Staff \music
  \new Staff \music
>>
