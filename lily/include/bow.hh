/*
  bow.hh -- declare Bow

  source file of the GNU LilyPond music typesetter

  (c)  1997--1999 Han-Wen Nienhuys <hanwen@cs.uu.nl>
*/


#ifndef BOW_HH
#define BOW_HH

#include "directional-spanner.hh"
#include "curve.hh"

/**
  Base class for anything that looks like a slur.
  Anybody with a better name?

  UGH. Fixme.  Should junk

    dy_f_drul_ , dx_f_drul_
  
  */
class Bow : public Directional_spanner
{
public:
  Bow ();
  Offset center () const;  

protected:
  virtual Molecule* do_brew_molecule_p () const;
  //  virtual Interval do_width () const;    
  Array<Offset> get_controls () const;
  virtual Array<Offset> get_encompass_offset_arr () const;
  virtual Interval do_height () const;
  Drul_array<Interval> curve_extent_drul () const;

  Drul_array<Real> dy_f_drul_;
  Drul_array<Real> dx_f_drul_;
};

#endif // BOW_HH
