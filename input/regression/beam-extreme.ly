
\version "2.3.22"
\header{
texidoc="
Beams should behave reasonably well, even under extreme circumstances.
Stems may be short, but noteheads should never touch the beam.  Note that
under normal circumstances, these beams would get knees here
@code{Beam.auto-knee-gap} was set to false.
"
}

extreme = \relative c'' {
  \stemNeutral
   g8[ c c,]
   c16[ c'' a f]
  \stemUp 
   c,,32[ c'' a f]

  %%%%%%%
  \stemNeutral
  \times 2/3{ d16[ fis' d,]} \times 2/3{ cis[ g'' cis,,]}
  a'16 cis a, g''' % Used to give a nice beam directed upwards.
  \stemNeutral

  \transpose c c {
	  \stemDown  e'8[ e e']
      }
}

\score {
  {
    % If we want to test extreme beams,
    % we should not have them auto-kneed
    \override Beam  #'auto-knee-gap = ##f
    \extreme


    %% what does this test? --hwn
%{
    \override Beam  #'slope-limit = #1000
    \extreme
    %}
  }
  \layout{
%    raggedright = ##t
    raggedright = ##t
  }
}
