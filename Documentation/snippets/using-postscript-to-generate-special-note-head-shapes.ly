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
  lsrtags = "editorial-annotations, really-cool, scheme-language, tweaks-and-overrides"

  texidoc = "
When a note head with a special shape cannot easily be generated with
graphic markup, PostScript code can be used to generate the
shape.  This example shows how a parallelogram-shaped note head is
generated.
"

  doctitle = "Using PostScript to generate special note head shapes"
} % begin verbatim


%% Updaters remark:
%% For unkown reasons this snippet returns a gs-error, but only, if compiled
%% with multiple others like: lilypond *.ly
%% Thus changing to a path-stencil.
%% TODO description needs to get adjusted  --harm

parallelogram =
  #(ly:make-stencil
    '(path 0.1
        (rmoveto 0 0.25
         lineto 1.3125 0.75
         lineto 1.3125 -0.25
         lineto 0 -0.75)
         round
         round
         #t)
    (cons 0 1.3125)
    (cons -.75 .75))

myNoteHeads = \override NoteHead.stencil = \parallelogram
normalNoteHeads = \revert NoteHead.stencil

\relative c'' {
  \myNoteHeads
  g4 d'
  \normalNoteHeads
  <f, \tweak stencil \parallelogram b e>4 d
}
