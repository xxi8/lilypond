
\header {

    texidoc = "Property overrides and reverts from @code{\\grace} do
    not interfere with the overrides and reverts from polyphony."

}
\layout { raggedright = ##t }

\version "2.3.22"
   \relative c'' {
      <<
	  { \grace e8 d2 }
	  \\ { a2 } >>
  }

