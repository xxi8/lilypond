/*   
  repeated-music.cc --  implement Repeated_music
  
  source file of the GNU LilyPond music typesetter
  
  (c) 1998 Jan Nieuwenhuizen <janneke@gnu.org>
  
 */

#include "repeated-music.hh"
#include "musical-pitch.hh"

Repeated_music::Repeated_music (Music* r, int n, Music_sequence* a)
{
  repeats_i_ = n;
  unfold_b_ = false;
  repeat_p_ = r;
  alternative_p_ = a;
}

Repeated_music::~Repeated_music ()
{
  delete repeat_p_;
  delete alternative_p_;
}

Repeated_music::Repeated_music (Repeated_music const& s)
  : Music (s)
{
  repeats_i_ = s.repeats_i_;
  repeat_p_ = (s.repeat_p_) ? s.repeat_p_->clone () : 0;
  // urg?
  alternative_p_ = (s.alternative_p_) ? dynamic_cast <Music_sequence*> (s.alternative_p_->clone ()) : 0;
}

void
Repeated_music::do_print () const
{
  if (repeat_p_)
    repeat_p_->print ();
  if (alternative_p_)
    alternative_p_->print ();
}

void
Repeated_music::transpose (Musical_pitch p)
{
  if (repeat_p_)
    repeat_p_->transpose (p);
  if (alternative_p_)
    alternative_p_->transpose (p);
}

Moment
Repeated_music::length_mom () const
{
  Moment m;
  if (repeat_p_)
    m += repeat_p_->length_mom ();
  if (alternative_p_)
    m += alternative_p_->length_mom ();
  return m;
}


Musical_pitch
Repeated_music::to_relative_octave (Musical_pitch p)
{
  p = repeat_p_->to_relative_octave (p);

  p = alternative_p_->do_relative_octave (p, false); 
  return p;
  
  /* ugh.  Should 
     \relative c'' \repeat 2  { c4 } { < ... > }

     and 
     
     \relative c'' \repeat 2  { c4 }
     { { ...} }

     behave differently?
   */
}
