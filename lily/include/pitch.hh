/*   
  pitch.hh -- declare Pitch
  
  source file of the GNU LilyPond music typesetter
  
  (c) 1998--2002 Han-Wen Nienhuys <hanwen@cs.uu.nl>
  
 */

#ifndef MUSICAL_PITCH_HH
#define MUSICAL_PITCH_HH

#include "lily-proto.hh"
#include "smobs.hh"

/** A "tonal" pitch. This is a pitch used in diatonal western
   music (12 semitones in an octave), as opposed to a frequency in Hz
   or a integer number of semitones.

  Pitch is lexicographically ordered by (octave, notename,
    alteration).    
  

   TODO:

   - add indeterminate octaves, so it can be used as a key in keySigature

   - abstract out the representation of alteration_, so we can
   put micropitches (quartertones, etc.) in the Pitch object
*/
class Pitch
{
private:				// fixme
  /*
    TODO: use SCM
   */

    /// 0 is c, 6 is b
  int notename_;
  
  /// 0 natural, 1 sharp, etc
  int alteration_;

  /// 0 is central c
  int octave_;
 
  void transpose (Pitch);
  void up_to (int);
  void down_to (int);
  void normalise ();

public:
  
  int get_octave () const;
  int get_notename () const;
  int get_alteration () const;

  Pitch (int octave, int notename,int accidental);
  Pitch ();

  Pitch transposed (Pitch) const;
  Pitch to_relative_octave (Pitch) const;

  static int compare (Pitch const&,Pitch const&);
  /// return large part of interval from central c
  int steps () const;
  /// return pitch from central c (in halfnotes)
  int semitone_pitch () const; 
  String string () const;

  SCM smobbed_copy () const;
  DECLARE_SCHEME_CALLBACK (less_p, (SCM a, SCM b));
  DECLARE_SIMPLE_SMOBS (Pitch,);
};

SCM ly_pitch_transpose (SCM p, SCM delta);
DECLARE_UNSMOB(Pitch,pitch);

#include "compare.hh"
INSTANTIATE_COMPARE (Pitch, Pitch::compare);

int compare (Array<Pitch>*, Array<Pitch>*);
extern SCM pitch_less_proc;
Pitch interval (Pitch const & from , Pitch const & to );

#endif /* MUSICAL_PITCH_HH */


