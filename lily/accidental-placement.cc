/*
  accidental-placement.cc -- implement Accidental_placement

  source file of the GNU LilyPond music typesetter

  (c) 2002--2006 Han-Wen Nienhuys <hanwen@xs4all.nl>
*/



#include "accidental-placement.hh"
#include "skyline.hh"
#include "music.hh"
#include "pitch.hh"
#include "warn.hh"
#include "note-column.hh"
#include "pointer-group-interface.hh"
#include "note-collision.hh"
#include "accidental-interface.hh"


void
Accidental_placement::add_accidental (Grob *me, Grob *a)
{
  a->set_parent (me, X_AXIS);
  a->set_property ("X-offset", Grob::x_parent_positioning_proc);
  SCM cause = a->get_parent (Y_AXIS)->get_property ("cause");

  Music *mcause = unsmob_music (cause);
  if (!mcause)
    {
      programming_error ("note head has no music cause");
      return;
    }

  Pitch *p = unsmob_pitch (mcause->get_property ("pitch"));

  int n = p->get_notename ();

  SCM accs = me->get_object ("accidental-grobs");
  SCM key = scm_from_int (n);
  SCM entry = scm_assq (key, accs);
  if (entry == SCM_BOOL_F)
    entry = SCM_EOL;
  else
    entry = scm_cdr (entry);

  entry = scm_cons (a->self_scm (), entry);

  accs = scm_assq_set_x (accs, key, entry);

  me->set_object ("accidental-grobs", accs);
}

/*
  Split into break reminders.
*/
void
Accidental_placement::split_accidentals (Grob *accs,
					 vector<Grob*> *break_reminder,
					 vector<Grob*> *real_acc)
{
  for (SCM acs = accs->get_object ("accidental-grobs"); scm_is_pair (acs);
       acs = scm_cdr (acs))
    for (SCM s = scm_cdar (acs); scm_is_pair (s); s = scm_cdr (s))
      {
	Grob *a = unsmob_grob (scm_car (s));

	if (unsmob_grob (a->get_object ("tie")))
	  break_reminder->push_back (a);
	else
	  real_acc->push_back (a);
      }
}

/*
  Accidentals are special, because they appear and disappear after
  ties at will.
*/
Interval
Accidental_placement::get_relevant_accidental_extent (Grob *me,
						      Item *item_col,
						      Grob *left_object)
{
  vector<Grob*> br, ra;
  vector<Grob*> *which = 0;

  Accidental_placement::split_accidentals (me, &br, &ra);
  concat (br, ra);

  if (dynamic_cast<Item *> (left_object)->break_status_dir () == RIGHT)
    which = &br;
  else
    which = &ra;

  Interval extent;
  for (vsize i = 0; i < which->size (); i++)
    extent.unite (which->at (i)->extent (item_col, X_AXIS));

  if (!extent.is_empty ())
    {
      Real p = robust_scm2double (me->get_property ("left-padding"), 0.2);
      extent[LEFT] -= p;
    }

  return extent;
}

struct Accidental_placement_entry
{
  vector<Skyline_entry> left_skyline_;
  vector<Skyline_entry> right_skyline_;
  Interval vertical_extent_;
  vector<Box> extents_;
  vector<Grob*> grobs_;
  Real offset_;
  int notename_;
  Accidental_placement_entry ()
  {
    offset_ = 0.0;
    notename_ = -1;
  }
};

static Interval all_accidental_vertical_extent;
Real ape_priority (Accidental_placement_entry const *a)
{
  return a->vertical_extent_[UP];
}

int ape_compare (Accidental_placement_entry *const &a,
		 Accidental_placement_entry *const &b)
{
  return sign (ape_priority (a) - ape_priority (b));
}

int ape_rcompare (Accidental_placement_entry *const &a,
		  Accidental_placement_entry *const &b)
{
  return -sign (ape_priority (a) - ape_priority (b));
}

/*
  TODO: should favor

  b
  b

  placement
*/
void
stagger_apes (vector<Accidental_placement_entry*> *apes)
{
  vector<Accidental_placement_entry*> asc = *apes;

  vector_sort (asc, &ape_compare);

  apes->clear ();

  int parity = 1;
  for (vsize i = 0; i < asc.size ();)
    {
      Accidental_placement_entry *a = 0;
      if (parity)
	{
	  a = asc.back ();
	  asc.pop_back ();
	}
      else
	a = asc[i++];

      apes->push_back (a);
      parity = !parity;
    }

  reverse (*apes);
}

/*
  This routine computes placements of accidentals. During
  add_accidental (), accidentals are already grouped by note, so that
  octaves are placed above each other; they form columns. Then the
  columns are sorted: the biggest columns go closest to the note.
  Then the columns are spaced as closely as possible (using skyline
  spacing).


  TODO: more advanced placement. Typically, the accs should be placed
  to form a C shape, like this


  ##
  b b
  # #
  b
  b b

  The naturals should be left of the C as well; they should
  be separate accs.

  Note that this placement problem looks NP hard, so we just use a
  simple strategy, not an optimal choice.
*/

/*
  TODO: there should be more space in the following situation


  Natural + downstem

  *
  *  |_
  *  | |    X
  *  |_|   |
  *    |   |
  *

*/

MAKE_SCHEME_CALLBACK(Accidental_placement, calc_positioning_done, 1);
SCM
Accidental_placement::calc_positioning_done (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  if (!me->is_live ())
    return SCM_BOOL_T;

  SCM accs = me->get_object ("accidental-grobs");
  if (!scm_is_pair (accs))
    return SCM_BOOL_T;

  /*
    TODO: there is a bug in this code. If two accs are on the same
    Y-position, they share an Ape, and will be printed in overstrike.
  */
  vector<Accidental_placement_entry*> apes;
  for (SCM s = accs; scm_is_pair (s); s = scm_cdr (s))
    {
      Accidental_placement_entry *ape = new Accidental_placement_entry;
      ape->notename_ = scm_to_int (scm_caar (s));

      for (SCM t = scm_cdar (s); scm_is_pair (t); t = scm_cdr (t))
	ape->grobs_.push_back (unsmob_grob (scm_car (t)));

      apes.push_back (ape);
    }

  Grob *common[] = {me, 0};

  /*
    First we must extract *all* pointers. We can only determine
    extents if we're sure that we've found the right common refpoint
  */
  vector<Grob*> note_cols, heads;
  for (vsize i = apes.size (); i--;)
    {
      Accidental_placement_entry *ape = apes[i];
      for (vsize j = ape->grobs_.size (); j--;)
	{
	  Grob *a = ape->grobs_[j];

	  if (common[Y_AXIS])
	    common[Y_AXIS] = common[Y_AXIS]->common_refpoint (a, Y_AXIS);
	  else
	    common[Y_AXIS] = a;

	  Grob *head = a->get_parent (Y_AXIS);

	  Grob *col = head->get_parent (X_AXIS);
	  if (Note_column::has_interface (col))
	    note_cols.push_back (col);
	  else
	    heads.push_back (head);
	}
    }

  /*
    This is a little kludgy: to get all notes, we look if there are
    collisions as well.
  */
  for (vsize i = note_cols.size (); i--;)
    {
      Grob *c = note_cols[i]->get_parent (X_AXIS);
      if (Note_collision_interface::has_interface (c))
	{
	  extract_grob_set (c, "elements", gs);

	  concat (note_cols, gs);
	}
    }

  for (vsize i = note_cols.size (); i--;)
    concat (heads, extract_grob_array (note_cols[i], "note-heads"));

  vector_sort (heads, default_compare);
  uniq (heads);
  common[Y_AXIS] = common_refpoint_of_array (heads, common[Y_AXIS], Y_AXIS);

  for (vsize i = apes.size (); i--;)
    {
      Accidental_placement_entry *ape = apes[i];
      ape->left_skyline_ = empty_skyline (LEFT);
      ape->right_skyline_ = empty_skyline (RIGHT);

      for (vsize j = apes[i]->grobs_.size (); j--;)
	{
	  Grob *a = apes[i]->grobs_[j];

	  vector<Box> boxes = Accidental_interface::accurate_boxes (a, common);

	  ape->extents_.insert (ape->extents_.end (), boxes.begin (), boxes.end ());
	  for (vsize j = boxes.size (); j--;)
	    {
	      insert_extent_into_skyline (&ape->left_skyline_, boxes[j], Y_AXIS, LEFT);
	      insert_extent_into_skyline (&ape->right_skyline_, boxes[j], Y_AXIS, RIGHT);
	    }
	}
    }

  Interval total;
  for (vsize i = apes.size (); i--;)
    {
      Interval y;

      for (vsize j = apes[i]->extents_.size (); j--;)
	y.unite (apes[i]->extents_[j][Y_AXIS]);
      apes[i]->vertical_extent_ = y;
      total.unite (y);
    }
  all_accidental_vertical_extent = total;
  stagger_apes (&apes);

  Accidental_placement_entry *head_ape = new Accidental_placement_entry;
  common[X_AXIS] = common_refpoint_of_array (heads, common[X_AXIS], X_AXIS);
  vector<Skyline_entry> head_skyline (empty_skyline (LEFT));
  vector<Box> head_extents;
  for (vsize i = heads.size (); i--;)
    {
      Box b (heads[i]->extent (common[X_AXIS], X_AXIS),
	     heads[i]->extent (common[Y_AXIS], Y_AXIS));

      insert_extent_into_skyline (&head_skyline, b, Y_AXIS, LEFT);
    }

  head_ape->left_skyline_ = head_skyline;
  head_ape->offset_ = 0.0;

  Real padding = robust_scm2double (me->get_property ("padding"), 0.2);

  vector<Skyline_entry> left_skyline = head_ape->left_skyline_;
  heighten_skyline (&left_skyline,
		    -robust_scm2double (me->get_property ("right-padding"), 0));
  /*
    Add accs entries right-to-left.
  */
  for (vsize i = apes.size (); i-- > 0;)
    {
      Real offset
	= -skyline_meshing_distance (apes[i]->right_skyline_, left_skyline);
      if (isinf (offset))
	offset = (i < apes.size () - 1) ? apes[i + 1]->offset_ : 0.0;
      else
	offset -= padding;

      apes[i]->offset_ = offset;

      vector<Skyline_entry> new_left_skyline = apes[i]->left_skyline_;
      heighten_skyline (&new_left_skyline, apes[i]->offset_);
      merge_skyline (&new_left_skyline, left_skyline, LEFT);
      left_skyline = new_left_skyline;
    }

  for (vsize i = apes.size (); i--;)
    {
      Accidental_placement_entry *ape = apes[i];
      for (vsize j = ape->grobs_.size (); j--;)
	ape->grobs_[j]->translate_axis (ape->offset_, X_AXIS);
    }

  Interval left_extent, right_extent;
  Accidental_placement_entry *ape = apes[0];

  for (vsize i = ape->extents_.size (); i--;)
    left_extent.unite (ape->offset_ + ape->extents_[i][X_AXIS]);

  ape = apes.back ();
  for (vsize i = ape->extents_.size (); i--;)
    right_extent.unite (ape->offset_ + ape->extents_[i][X_AXIS]);

  left_extent[LEFT] -= robust_scm2double (me->get_property ("left-padding"), 0);
  Interval width (left_extent[LEFT], right_extent[RIGHT]);

  SCM scm_width = ly_interval2scm (width);
  me->flush_extent_cache (X_AXIS);
  me->set_property ("X-extent", scm_width);

  for (vsize i = apes.size (); i--;)
    delete apes[i];

  return SCM_BOOL_T;
}

ADD_INTERFACE (Accidental_placement,
	       "accidental-placement-interface",
	       "Resolve accidental collisions.",

	       /* properties */
	       "accidental-grobs "
	       "left-padding "
	       "padding "
	       "positioning-done "
	       "right-padding ")
