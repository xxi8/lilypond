/*
  volta-spanner.cc -- implement Volta_spanner

  source file of the GNU LilyPond music typesetter

  (c)  1997--1998 Jan Nieuwenhuizen <janneke@gnu.org>
*/


#include "box.hh"
#include "debug.hh"
#include "lookup.hh"
#include "molecule.hh"
#include "note-column.hh"
#include "p-col.hh"
#include "bar.hh"
#include "paper-def.hh"
#include "volta-spanner.hh"
#include "stem.hh"
#include "text-def.hh"
#include "pointer.tcc"

template class P<Text_def>;		// UGH

Volta_spanner::Volta_spanner ()
{
  last_b_ = false;
  visible_b_ = true;
  number_p_.set_p (new Text_def);
  number_p_->align_dir_ = LEFT;
}

Molecule*
Volta_spanner::do_brew_molecule_p () const
{
  Molecule* mol_p = new Molecule;

  if (!column_arr_.size ())
    return mol_p;

  if (!visible_b_)
    return mol_p;

  Real internote_f = paper ()->internote_f ();
  Real dx = internote_f;
  Real w = extent (X_AXIS).length () - dx;
  Molecule volta (lookup_l ()->volta (w, last_b_));
  Real h = volta.dim_.y ().length ();
  Molecule num (number_p_->get_molecule (paper (), LEFT));
  Real dy = column_arr_.top ()->extent (Y_AXIS) [UP] > 
     column_arr_[0]->extent (Y_AXIS) [UP];
  dy += 2 * h;

  /*
    UGH.  Must use extent  ()[dir_]
   */
  for (int i = 0; i < note_column_arr_.size (); i++)
    dy = dy >? note_column_arr_[i]->extent (Y_AXIS).max ();
  dy -= h;

  Text_def two_text;
  two_text.text_str_ = "2";
  two_text.style_str_ = number_p_->style_str_;
  Molecule two (two_text.get_molecule (paper (), LEFT));
  Real gap = two.dim_.x ().length () / 2;
  Offset off (num.dim_.x ().length () + gap, 
	      h / internote_f - gap);
  num.translate (off);
  mol_p->add_molecule (volta);
  mol_p->add_molecule (num);
  mol_p->translate (Offset (0, dy));
  return mol_p;
}
  
void
Volta_spanner::do_add_processing ()
{
  if (column_arr_.size ())
    {
      set_bounds (LEFT, column_arr_[0]);
      set_bounds (RIGHT, column_arr_.top ());  
    }

  number_p_->style_str_ = "number"; // number-1
}
  
Interval
Volta_spanner::do_height () const
{
  /*
    in most cases, it's a lot better not no have height...
  */
  Interval i;
  return i;
}

void
Volta_spanner::do_post_processing ()
{
  if (column_arr_.size())
    translate_axis (column_arr_[0]->extent (Y_AXIS)[UP], Y_AXIS);
}

void
Volta_spanner::do_substitute_dependency (Score_element* o, Score_element* n)
{
  if (Note_column* c = dynamic_cast <Note_column*> (o))
    note_column_arr_.substitute (c, dynamic_cast<Note_column*> (n));
  else if (Bar* c = dynamic_cast <Bar*> (o))
    column_arr_.substitute (c, dynamic_cast<Bar*> (n));
}
  
void
Volta_spanner::add_column (Bar* c)
{
  column_arr_.push (c);
  add_dependency (c);
}

void
Volta_spanner::add_column (Note_column* c)
{
  note_column_arr_.push (c);
  add_dependency (c);
}

