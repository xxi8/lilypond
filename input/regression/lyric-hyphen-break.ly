\version "2.1.20"

\header {

    texidoc = "Hyphens only print at the beginning of the line, when
they go past the first note. "

    }

\score {
<<    \notes \new Staff \relative c'' { \time 1/4 c16[ c c  c]
\time 1/4
c16[ c c c]
\time 1/4
r c16[ c c]

}
    \lyrics \new LyricsVoice {
	bla16 -- bla -- bla -- bla --
	bla -- bla -- bla -- bla8 --
	       bla16 -- bla -- bla 
       }>>
    \paper   {
	indent = 0.0 \cm
	linewidth =  3.4 \cm

	\translator {
	    \StaffContext \remove "Time_signature_engraver"
	}
	
    }
      
}

	
