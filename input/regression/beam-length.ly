
\version "1.9.4"

\header{
texidoc="
Beams should look the same.
"
}
    \paper { raggedright= ##t }

    
\score { 
  \context Voice \notes\relative c {
			 d''8[ d d]  d[ g d]
			c c
  }
}
