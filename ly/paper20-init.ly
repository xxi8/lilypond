% paper20-init.ly


\version "1.5.68"

paperTwenty = \paper {
	staffheight = 20.0\pt
	#(define fonts (make-font-list 'paper20))
	
	\include "params-init.ly"
}

\paper { \paperTwenty }
