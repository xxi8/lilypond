%% Do not edit this file; it is auto-generated from LSR http://lsr.dsi.unimi.it
%% This file is in the public domain.
\version "2.11.38"

\header {
  lsrtags = "rhythms"
 texidoc = "
In time signature 2/2 or 4/4 the beam are  @code{         _____        
 _   _  Default | | | | I want | | | |. } Use a \"macro\" with
#(override-auto-beam-setting '.....



" }
% begin verbatim
% Automatic beams two per two in 4/4 or 2/2 time signature
%	     _____
% Default   | | | | 
%	     _   _
% I want    | | | |

% The good way adapted from David Bobrof

% macro for beamed two per two in 2/2 and 4/4 time signature
qbeam={  
 	#(override-auto-beam-setting '(end 1 8 * *) 1 4 'Staff)
 	#(override-auto-beam-setting '(end 1 8 * *) 2 4 'Staff)
  	#(override-auto-beam-setting '(end 1 8 * *) 3 4 'Staff) 
 	}
% other macros	
timeFractionstyle = { \override Staff.TimeSignature #'style = #'() }
textn = ^\markup { without the macro }
texty = ^\markup { with the macro }

\score {
 << 
	\new Staff << \relative c'' {  
		\timeFractionstyle
		\time 4/4
		 g8\textn g g g   g g g g   g g g g4  g8 g g
		 }
		>>
		
	%Use the macro 	
	
	\new Staff << \relative c'' {  
		\timeFractionstyle
		\time 4/4
		\qbeam
		g8\texty g g g   g g g g  g g g g4  g8 g g 
		 }
	 >>	 
 >>
}
