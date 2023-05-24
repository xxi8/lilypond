\version "2.25.6"

\header {
  lsrtags = "headword"

  texidoc = "
Keyboard headword
"

  doctitle = "Keyboard headword"
}


% M. Ravel, Sonatine (1905)
% First movement
\include "english.ly"

\layout {
  \context {
    \Score
    \remove "Bar_number_engraver"
  }
}

fermataLong = \markup {
  \override #'(direction . 1)
  \override #'(baseline-skip . 2) {
    \dir-column {
      \fermata
      \serif \italic \center-align long
    }
  }
}

\new PianoStaff <<
  \set PianoStaff.connectArpeggios = ##t
  \new Staff {
    \time 2/4
    \key fs \major
    <<
      \new Voice {
        \voiceOne
        fs''8 ( ^\markup {
          \override #'(baseline-skip . 2.4) \column {
            \line \bold { Un peu retenu }
            \line \italic { très expressif }
          }
        }
        es''16
        cs''16
        as'4 )
        |
        fs''8 (
        es''16
        cs''16
        as'4 )
        |
        fs''8 (
        es''16
        cs''16
        as'8
        cs''8 )
        |
      }
      \new Voice {
        \voiceTwo
        gs'8\rest \ppp
        fs'4 (
        es'8 )
        |
        gs'8\rest
        fs'4 (
        es'8 )
        |
        gs'8\rest
        fs'4 (
        es'8 )
        |
      }
    >>
    \clef bass
    <ds b! es'>4 ( ^ \markup \bold { Rall. }
    \override Script.stencil = #(lambda (grob)
      (grob-interpret-markup grob fermataLong))
    <ds' as'>8 ) \fermata
    \noBeam
    \clef treble
    \slurUp
    \once \override Hairpin.to-barline = ##f
    <as fs'>8 ( \pp \>
    |
    <gs b cs'>4. \! ) ^\markup \bold { a Tempo }
    \slurUp
    <as fs'>8 ^\( \>
    |
    <gs b cs'>4. \! \)
    <<
      \new Voice {
        \voiceOne
        <as fs'>8 (
        |
        cs'8 ^\markup \bold { Rallentando }
        b16
        cs'16
        d'8
        e'16
        fs'16
        |
        <as! cs' gs'>4. )
        s8
        |
        r8
        <cs'' as'' cs'''>4 \arpeggio
        e''16 ( ^\markup \bold { Lent }
        fs''16
        |
        \voiceTwo
        <as'! cs'' gs''>2 )
        |
      }
      \new Voice {
        \voiceTwo
        s8
        |
        <gs b>4 \<
        <fs bs>4 \>
        |
        s4. \!
        \slurUp
        \once \override Script.direction = #UP
        <a bs e'>8 ( \accent
        |
        <as! cs' gs'>4. )
        \once \override Hairpin.to-barline = ##f
        <a' bs'>8 \ppp \>
        |
        s8 \!
        \stemDown
        \once \override Script.direction = #UP
        \ottava #1
        \voiceOne
        \once \override PianoStaff.Arpeggio.padding = #0.8
        <cs''' as''' cs''''>4. \arpeggio \fermata
        \ottava #0
        \bar "|."
      }
    >>
  }
  \new Staff <<
    \set Staff.pedalSustainStyle = #'bracket
    \key fs \major
    \clef bass
    \new Voice {
      \voiceOne
      ds'4 \tenuto
      cs'4 \tenuto
      |
      ds'4 \tenuto
      cs'4 \tenuto
      |
      ds'4 \tenuto
      cs'4 \tenuto
      |
      s8
      \clef treble
      <b' cs''>8 [
      \clef bass
      <es b cs'>8 \fermata ]
      s8
      |
      fs8\rest
      \clef treble
      <b' cs''>4 \tenuto
      s8
      |
      fs8\rest
      \clef treble
      <b' cs''>4 \tenuto
      s8
      |
      s2
      |
      ds8\rest
      \clef treble
      <as' cs''>4
      \clef bass
      s8
      |
      s8
      \clef treble
      <as'>4 \arpeggio
      \clef bass
      s8
      |
      s8
      \clef treble
      <as''>4. \arpeggio \fermata
      |
    }
    \new Voice {
      \voiceTwo
      ds'8 [ (
      < ds bs >8
      cs'8
      < ds as >8 ] )
      |
      ds'8 [ (
      < ds bs >8
      cs'8
      < ds as >8 ] )
      |
      ds'8 [ (
      < ds bs >8
      cs'8
      < ds as >8 ] )
      |
      \once \override Script.outside-staff-priority = #100
      \once \override TextScript.outside-staff-priority = #500
      <cs, gs,>4. \fermata _\markup \italic { ped. }
      <fs, cs>8 (
      |
      <e, b,>4. ) \sustainOn
      \clef bass
      <fs, cs>8 \( \sustainOff
      |
      <e, b,>4. \) \sustainOn
      \clef bass
      <fs, cs>8 ( \sustainOff
      |
      <e, b,>4
      <d, a,>4
      |
      <fs,, cs,>4. ) \sustainOn
      <a, e>8 ( \sustainOff
      |
      <fs, cs>4. ) \sustainOn
      \slurUp
      <a e'>8 ( \sustainOff \sustainOn
      |
      <fs cs'>2 ) \sustainOff \sustainOn
      |
    }
  >>
>>
