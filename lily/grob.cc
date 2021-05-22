/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 1997--2020 Han-Wen Nienhuys <hanwen@xs4all.nl>

  LilyPond is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  LilyPond is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with LilyPond.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "grob.hh"

#include "align-interface.hh"
#include "axis-group-interface.hh"
#include "input.hh"
#include "international.hh"
#include "item.hh"
#include "main.hh"
#include "misc.hh"
#include "music.hh"
#include "output-def.hh"
#include "pointer-group-interface.hh"
#include "program-option.hh"
#include "stencil.hh"
#include "stream-event.hh"
#include "system.hh"
#include "unpure-pure-container.hh"
#include "warn.hh"
#include "lily-imports.hh"

#include <cstring>
#include <set>
#include <unordered_set>

using std::set;
using std::string;
using std::vector;

Grob::Grob (SCM basicprops)
{

  /* FIXME: default should be no callback.  */
  layout_ = 0;
  original_ = 0;
  interfaces_ = SCM_EOL;
  immutable_property_alist_ = basicprops;
  mutable_property_alist_ = SCM_EOL;
  object_alist_ = SCM_EOL;

  /* We do smobify_self () as the first step.  Since the object lives
     on the heap, none of its SCM variables are protected from
     GC. After smobify_self (), they are.  */
  smobify_self ();

  SCM meta = get_property (this, "meta");
  if (scm_is_pair (meta))
    {
      interfaces_ = scm_cdr (scm_assq (ly_symbol2scm ("interfaces"), meta));

      SCM object_cbs = scm_assq (ly_symbol2scm ("object-callbacks"), meta);
      if (scm_is_pair (object_cbs))
        {
          for (SCM s = scm_cdr (object_cbs); scm_is_pair (s); s = scm_cdr (s))
            set_object (this, scm_caar (s), scm_cdar (s));
        }
    }

  if (scm_is_null (get_property_data (this, "X-extent")))
    set_property (this, "X-extent", Grob::stencil_width_proc);
  if (scm_is_null (get_property_data (this, "Y-extent")))
    set_property (this, "Y-extent",
                  Unpure_pure_container::make_smob (Grob::stencil_height_proc,
                                                    Grob::pure_stencil_height_proc));
  if (scm_is_null (get_property_data (this, "vertical-skylines")))
    set_property (this, "vertical-skylines",
                  Unpure_pure_container::make_smob (Grob::simple_vertical_skylines_from_extents_proc,
                                                    Grob::pure_simple_vertical_skylines_from_extents_proc));
  if (scm_is_null (get_property_data (this, "horizontal-skylines")))
    set_property (this, "horizontal-skylines",
                  Unpure_pure_container::make_smob (Grob::simple_horizontal_skylines_from_extents_proc,
                                                    Grob::pure_simple_horizontal_skylines_from_extents_proc));
}

Grob::Grob (Grob const &s)
  : Smob<Grob> ()
{
  original_ = (Grob *) & s;

  immutable_property_alist_ = s.immutable_property_alist_;
  mutable_property_alist_ = SCM_EOL;

  for (Axis a = X_AXIS; a < NO_AXES; incr (a))
    dim_cache_ [a] = s.dim_cache_ [a];

  interfaces_ = s.interfaces_;
  object_alist_ = SCM_EOL;

  layout_ = 0;

  smobify_self ();

  mutable_property_alist_ = ly_alist_copy (s.mutable_property_alist_);
}

Grob::~Grob ()
{
}
/****************************************************************
  STENCILS
****************************************************************/

Stencil *
Grob::get_stencil () const
{
  if (!is_live ())
    return 0;

  SCM stil = get_property (this, "stencil");
  return unsmob<Stencil> (stil);
}

Stencil
Grob::get_print_stencil () const
{
  SCM stil = get_property (this, "stencil");

  Stencil retval;
  if (Stencil *m = unsmob<Stencil> (stil))
    {
      retval = *m;
      bool transparent = from_scm<bool> (get_property (this, "transparent"));

      /* Process whiteout before color and grob-cause to prevent colored */
      /* whiteout background and larger file sizes with \pointAndClickOn. */
      /* A grob has to be visible, otherwise the whiteout property has no effect. */
      /* Calls the scheme procedure stencil-whiteout in scm/stencils.scm */
      if (!transparent && (scm_is_number (get_property (this, "whiteout"))
                           || from_scm<bool> (get_property (this, "whiteout"))))
        {
          Real line_thickness = layout ()->get_dimension (ly_symbol2scm ("line-thickness"));
          retval = *unsmob<Stencil>
                   (Lily::stencil_whiteout (retval.smobbed_copy (),
                                            get_property (this, "whiteout-style"),
                                            get_property (this, "whiteout"),
                                            to_scm (line_thickness)));
        }

      if (transparent)
        retval = Stencil (m->extent_box (), SCM_EOL);
      else
        {
          SCM expr = scm_list_3 (ly_symbol2scm ("grob-cause"),
                                 self_scm (),
                                 retval.expr ());

          retval = Stencil (retval.extent_box (), expr);
        }

      SCM rot = get_property (this, "rotation");
      if (scm_is_pair (rot))
        {
          Real angle = scm_to_double (scm_car (rot));
          Real x = scm_to_double (scm_cadr (rot));
          Real y = scm_to_double (scm_caddr (rot));

          retval.rotate_degrees (angle, Offset (x, y));
        }

      /* color support... see interpret_stencil_expression () for more... */
      SCM color = get_property (this, "color");
      if (scm_is_pair (color) || scm_is_string (color))
        {
          SCM expr = scm_list_3 (ly_symbol2scm ("color"),
                                 color,
                                 retval.expr ());

          retval = Stencil (retval.extent_box (), expr);
        }

      SCM attributes = get_property (this, "output-attributes");
      if (scm_is_pair (attributes))
        {
          SCM expr = scm_list_3 (ly_symbol2scm ("output-attributes"),
                                 attributes,
                                 retval.expr ());

          retval = Stencil (retval.extent_box (), expr);
        }

    }

  return retval;
}

/****************************************************************
  VIRTUAL STUBS
****************************************************************/
void
Grob::do_break_processing ()
{
}

void
Grob::break_breakable_item (System *)
{
}

System *
Grob::get_system () const
{
  return 0;
}

/* This version of get_system is more reliable than this->get_system ()
   before line-breaking has been done, at which point there is only
   one system in the whole score and we can find it just by following
   parent pointers. */
System *
Grob::get_system (Grob *me)
{
  Grob *p = me->get_x_parent ();
  return p ? get_system (p) : dynamic_cast<System *>(me);
}

void
Grob::handle_broken_dependencies ()
{
  Spanner *sp = dynamic_cast<Spanner *> (this);
  if (original () && sp)
    return;

  if (sp)
    /* THIS, SP is the original spanner.  We use a special function
       because some Spanners have enormously long lists in their
       properties, and a special function fixes FOO  */
    {
      for (SCM s = object_alist_; scm_is_pair (s); s = scm_cdr (s))
        sp->substitute_one_mutable_property (scm_caar (s), scm_cdar (s));
    }
  System *system = get_system ();

  if (is_live ()
      && system
      && common_refpoint (system, X_AXIS)
      && common_refpoint (system, Y_AXIS))
    substitute_object_links (system->self_scm (), object_alist_);
  else if (dynamic_cast<System *> (this))
    substitute_object_links (SCM_UNDEFINED, object_alist_);
  else
    /* THIS element is `invalid'; it has been removed from all
       dependencies, so let's junk the element itself.

       Do not do this for System, since that would remove references
       to the originals of score-grobs, which get then GC'd (a bad
       thing).  */
    suicide ();
}

/* Note that we still want references to this element to be
   rearranged, and not silently thrown away, so we keep pointers like
   {broken_into_{drul, array}, original}
*/
void
Grob::suicide ()
{
  if (!is_live ())
    return;

  for (int a = X_AXIS; a < NO_AXES; a++)
    dim_cache_[a].clear ();

  mutable_property_alist_ = SCM_EOL;
  object_alist_ = SCM_EOL;
  immutable_property_alist_ = SCM_EOL;
  interfaces_ = SCM_EOL;
}

void
Grob::handle_prebroken_dependencies ()
{
  /* Don't do this in the derived method, since we want to keep access to
     object_alist_ centralized.  */
  if (original ())
    {
      Item *it = dynamic_cast<Item *> (this);
      substitute_object_links (to_scm (it->break_status_dir ()),
                               original ()->object_alist_);
    }
}

Grob *
Grob::find_broken_piece (System *) const
{
  return 0;
}

/****************************************************************
   OFFSETS
****************************************************************/

void
Grob::translate_axis (Real y, Axis a)
{
  if (!std::isfinite (y))
    {
      programming_error ("Infinity or NaN encountered");
      return;
    }

  if (!dim_cache_[a].offset_)
    dim_cache_[a].offset_ = y;
  else
    *dim_cache_[a].offset_ += y;
}

/* Find the offset relative to D.  If D equals THIS, then it is 0.
   Otherwise, it recursively defd as

   OFFSET_ + PARENT_L_->relative_coordinate (D) */
Real
Grob::relative_coordinate (Grob const *refp, Axis a) const
{
  /* eaa - hmmm, should we do a programming_error() here? */
  if (refp == this)
    return 0.0;

  /* We catch PARENT_L_ == nil case with this, but we crash if we did
     not ask for the absolute coordinate (ie. REFP == nil.)  */

  return get_offset (a) + parent_relative (refp, a);
}

Real
Grob::parent_relative (Grob const *refp, Axis a) const
{
  if (Grob *p = get_parent (a))
    return p->relative_coordinate (refp, a);
  return 0.0;
}

Real
Grob::pure_relative_y_coordinate (Grob const *refp, vsize start, vsize end)
{
  if (refp == this)
    return 0.0;

  Real off = 0;

  if (dim_cache_[Y_AXIS].offset_)
    {
      if (from_scm<bool> (get_property (this, "pure-Y-offset-in-progress")))
        programming_error ("cyclic chain in pure-Y-offset callbacks");

      off = *dim_cache_[Y_AXIS].offset_;
    }
  else
    {
      SCM proc = get_property_data (this, "Y-offset");

      dim_cache_[Y_AXIS].offset_ = 0;
      set_property (this, "pure-Y-offset-in-progress", SCM_BOOL_T);
      off = from_scm<double> (call_pure_function (proc,
                                                  scm_list_1 (self_scm ()),
                                                  start, end),
                              0.0);
      del_property (this, "pure-Y-offset-in-progress");
      dim_cache_[Y_AXIS].offset_.reset ();
    }

  /* we simulate positioning-done if we are the child of a VerticalAlignment,
     but only if we don't have a cached offset. If we do have a cached offset,
     it probably means that the Alignment was fixed and it has already been
     calculated.
  */
  if (Grob *p = get_y_parent ())
    {
      Real trans = 0;
      if (has_interface<Align_interface> (p) && !dim_cache_[Y_AXIS].offset_)
        trans = Align_interface::get_pure_child_y_translation (p, this, start, end);

      return off + trans + p->pure_relative_y_coordinate (refp, start, end);
    }
  return off;
}

/* Invoke callbacks to get offset relative to parent.  */
Real
Grob::get_offset (Axis a) const
{
  if (dim_cache_[a].offset_)
    return *dim_cache_[a].offset_;

  SCM sym = axis_offset_symbol (a);
  dim_cache_[a].offset_ = 0;

  /*
    UGH: can't fold next 2 statements together. Apparently GCC thinks
    dim_cache_[a].offset_ is unaliased.
  */
  Real off = from_scm<double> (get_property (this, sym), 0.0);
  if (dim_cache_[a].offset_)
    {
      *dim_cache_[a].offset_ += off;
      del_property (const_cast<Grob *> (this), sym);
      return *dim_cache_[a].offset_;
    }
  else
    return 0.0;
}

Real
Grob::maybe_pure_coordinate (Grob const *refp, Axis a, bool pure,
                             vsize start, vsize end)
{
  if (pure && a != Y_AXIS)
    programming_error ("tried to get pure X-offset");
  return (pure && a == Y_AXIS) ? pure_relative_y_coordinate (refp, start, end)
         : relative_coordinate (refp, a);
}

/****************************************************************
  extents
****************************************************************/

void
Grob::flush_extent_cache (Axis axis)
{
  if (dim_cache_[axis].extent_)
    {
      /*
        Ugh, this is not accurate; will flush property, causing
        callback to be called if.
       */
      if (axis == X_AXIS)
        del_property (this, "X-extent");
      else
        del_property (this, "Y-extent");
      dim_cache_[axis].extent_.reset ();
      if (get_parent (axis))
        get_parent (axis)->flush_extent_cache (axis);
    }
}

Interval
Grob::extent (Grob const *refp, Axis a) const
{
  Real offset = relative_coordinate (refp, a);
  Interval real_ext;
  if (dim_cache_[a].extent_)
    {
      real_ext = *dim_cache_[a].extent_;
    }
  else
    {
      /*
        Order is significant: ?-extent may trigger suicide.
       */
      SCM ext = (a == X_AXIS)
                ? get_property (this, "X-extent")
                : get_property (this, "Y-extent");
      if (is_number_pair (ext))
        real_ext.unite (from_scm<Interval> (ext));

      SCM min_ext = (a == X_AXIS)
                    ? get_property (this, "minimum-X-extent")
                    : get_property (this, "minimum-Y-extent");
      if (is_number_pair (min_ext))
        real_ext.unite (from_scm<Interval> (min_ext));

      dim_cache_[a].extent_ = real_ext;
    }

  // We never want nan, so we avoid shifting infinite values.
  if (!std::isinf (offset))
    real_ext.translate (offset);
  else
    warning (_f ("ignored infinite %s-offset",
                 a == X_AXIS ? "X" : "Y"));

  return real_ext;
}

Interval
Grob::pure_y_extent (Grob *refp, vsize start, vsize end)
{
  SCM iv_scm = get_pure_property (this, "Y-extent", start, end);
  Interval iv = from_scm (iv_scm, Interval ());
  Real offset = pure_relative_y_coordinate (refp, start, end);

  SCM min_ext = get_property (this, "minimum-Y-extent");

  /* we don't add minimum-Y-extent if the extent is empty. This solves
     a problem with Hara-kiri spanners. They would request_suicide and
     return empty extents, but we would force them here to be large. */
  if (!iv.is_empty () && is_number_pair (min_ext))
    iv.unite (from_scm<Interval> (min_ext));

  if (!iv.is_empty ())
    iv.translate (offset);
  return iv;
}

Interval
Grob::maybe_pure_extent (Grob *refp, Axis a, bool pure, vsize start, vsize end)
{
  return (pure && a == Y_AXIS) ? pure_y_extent (refp, start, end) : extent (refp, a);
}

/* Sort grobs according to their starting column. */
bool
Grob::less (Grob *g1, Grob *g2)
{
  return g1->spanned_rank_interval ()[LEFT] < g2->spanned_rank_interval ()[LEFT];
}

/****************************************************************
  REFPOINTS
****************************************************************/

/* Find the group-element which has both #this# and #s#  */
Grob *
Grob::common_refpoint (Grob const *s, Axis a) const
{

  /* Catching the trivial cases is likely costlier than just running
     through: one can't avoid going to the respective chain ends
     anyway.  We might save the second run through when the chain ends
     differ, but keeping track of the ends makes the loop more costly.
  */

  int balance = 0;
  Grob const *c;
  Grob const *d;

  for (c = this; c; ++balance)
    c = c->dim_cache_[a].parent_;

  for (d = s; d; --balance)
    d = d->dim_cache_[a].parent_;

  /* Cut down ancestry to same size */

  for (c = this; balance > 0; --balance)
    c = c->dim_cache_[a].parent_;

  for (d = s; balance < 0; ++balance)
    d = d->dim_cache_[a].parent_;

  /* Now find point where our lineages converge */
  while (c != d)
    {
      c = c->dim_cache_[a].parent_;
      d = d->dim_cache_[a].parent_;
    }

  return (Grob *) c;
}

void
Grob::fixup_refpoint ()
{
  for (int a = X_AXIS; a < NO_AXES; a++)
    {
      Axis ax = (Axis)a;
      Grob *parent = get_parent (ax);

      if (!parent)
        continue;

      if (parent->get_system () != get_system () && get_system ())
        {
          Grob *newparent = parent->find_broken_piece (get_system ());
          set_parent (newparent, ax);
        }

      if (Item *i = dynamic_cast<Item *> (this))
        {
          Item *parenti = dynamic_cast<Item *> (parent);

          if (parenti && i)
            {
              Direction my_dir = i->break_status_dir ();
              if (my_dir != parenti->break_status_dir ())
                {
                  Item *newparent = parenti->find_prebroken_piece (my_dir);
                  set_parent (newparent, ax);
                }
            }
        }
    }
}

/****************************************************************
  VERTICAL ORDERING
****************************************************************/

Grob *
get_maybe_root_vertical_alignment (Grob *g, Grob *maybe)
{
  if (!g)
    return maybe;
  if (has_interface<Align_interface> (g))
    return get_maybe_root_vertical_alignment (g->get_y_parent (), g);
  return get_maybe_root_vertical_alignment (g->get_y_parent (), maybe);

}

Grob *
Grob::get_root_vertical_alignment (Grob *g)
{
  return get_maybe_root_vertical_alignment (g, 0);
}

Grob *
Grob::get_vertical_axis_group (Grob *g)
{
  if (!g)
    return 0;
  if (!g->get_y_parent ())
    return 0;
  if (has_interface<Axis_group_interface> (g)
      && has_interface<Align_interface> (g->get_y_parent ()))
    return g;
  return get_vertical_axis_group (g->get_y_parent ());

}

int
Grob::get_vertical_axis_group_index (Grob *g)
{
  Grob *val = get_root_vertical_alignment (g);
  if (!val)
    return -1;
  Grob *vax = get_vertical_axis_group (g);
  extract_grob_set (val, "elements", elts);
  for (vsize i = 0; i < elts.size (); i++)
    if (elts[i] == vax)
      return (int) i;
  g->programming_error ("could not find this grob's vertical axis group in the vertical alignment");
  return -1;
}

bool
Grob::vertical_less (Grob *g1, Grob *g2)
{
  return internal_vertical_less (g1, g2, false);
}

bool
Grob::pure_vertical_less (Grob *g1, Grob *g2)
{
  return internal_vertical_less (g1, g2, true);
}

bool
Grob::internal_vertical_less (Grob *g1, Grob *g2, bool pure)
{
  Grob *vag = get_root_vertical_alignment (g1);
  if (!vag)
    {
      g1->programming_error ("grob does not belong to a VerticalAlignment?");
      return false;
    }

  Grob *ag1 = get_vertical_axis_group (g1);
  Grob *ag2 = get_vertical_axis_group (g2);

  extract_grob_set (vag, "elements", elts);

  if (ag1 == ag2 && !pure)
    {
      Grob *common = g1->common_refpoint (g2, Y_AXIS);
      return g1->relative_coordinate (common, Y_AXIS) > g2->relative_coordinate (common, Y_AXIS);
    }

  for (vsize i = 0; i < elts.size (); i++)
    {
      if (elts[i] == ag1)
        return true;
      if (elts[i] == ag2)
        return false;
    }

  g1->programming_error ("could not place this grob in its axis group");
  return false;
}

/****************************************************************
  CAUSES
****************************************************************/
Stream_event *
Grob::event_cause () const
{
  SCM cause = get_property (this, "cause");
  return unsmob<Stream_event> (cause);
}

Stream_event *
Grob::ultimate_event_cause () const
{
  SCM cause = get_property (this, "cause");
  while (Grob *g = unsmob<Grob> (cause))
    {
      cause = get_property (g, "cause");
    }
  return unsmob<Stream_event> (cause);
}

/****************************************************************
  MESSAGES
****************************************************************/
Input *
Grob::origin () const
{
  if (Stream_event *ev = ultimate_event_cause ())
    return ev->origin ();
  return nullptr;
}

string
Grob::name () const
{
  SCM meta = get_property (this, "meta");
  SCM nm = scm_assq (ly_symbol2scm ("name"), meta);
  nm = (scm_is_pair (nm)) ? scm_cdr (nm) : SCM_EOL;
  return scm_is_symbol (nm) ? ly_symbol2string (nm) : class_name ();
}

ADD_INTERFACE (Grob,
               "A grob represents a piece of music notation.\n"
               "\n"
               "All grobs have an X and Y@tie{}position on the page.  These"
               " X and Y@tie{}positions are stored in a relative format, thus"
               " they can easily be combined by stacking them, hanging one"
               " grob to the side of another, or coupling them into grouping"
               " objects.\n"
               "\n"
               "Each grob has a reference point (a.k.a.@: parent): The"
               " position of a grob is stored relative to that reference"
               " point.  For example, the X@tie{}reference point of a staccato"
               " dot usually is the note head that it applies to.  When the"
               " note head is moved, the staccato dot moves along"
               " automatically.\n"
               "\n"
               "A grob is often associated with a symbol, but some grobs do"
               " not print any symbols.  They take care of grouping objects."
               " For example, there is a separate grob that stacks staves"
               " vertically.  The @ref{NoteCollision} object is also an"
               " abstract grob: It only moves around chords, but doesn't print"
               " anything.\n"
               "\n"
               "Grobs have properties (Scheme variables) that can be read and"
               " set.  Two types of them exist: immutable and mutable."
               "  Immutable variables define the default style and behavior."
               "  They are shared between many objects.  They can be changed"
               " using @code{\\override} and @code{\\revert}.  Mutable"
               " properties are variables that are specific to one grob."
               "  Typically, lists of other objects, or results from"
               " computations are stored in mutable properties.  In"
               " particular, every call to @code{ly:grob-set-property!}"
               " (or its C++ equivalent) sets a mutable property.\n"
               "\n"
               "The properties @code{after-line-breaking} and"
               " @code{before-line-breaking} are dummies that are not"
               " user-serviceable.",

               /* properties */
               "X-extent "
               "X-offset "
               "Y-extent "
               "Y-offset "
               "after-line-breaking "
               "avoid-slur "
               "axis-group-parent-X "
               "axis-group-parent-Y "
               "before-line-breaking "
               "cause "
               "color "
               "cross-staff "
               "extra-offset "
               "footnote-music "
               "forced-spacing "
               "horizontal-skylines "
               "id "
               "interfaces "
               "layer "
               "meta "
               "minimum-X-extent "
               "minimum-Y-extent "
               "output-attributes "
               "parenthesis-friends "
               "pure-Y-offset-in-progress "
               "rotation "
               "skyline-horizontal-padding "
               "springs-and-rods "
               "staff-symbol "
               "stencil "
               "transparent "
               "vertical-skylines "
               "whiteout "
               "whiteout-style "
              );

/****************************************************************
  CALLBACKS
****************************************************************/

static SCM
grob_stencil_extent (Grob *me, Axis a)
{
  Stencil *m = me->get_stencil ();
  Interval e;
  if (m)
    e = m->extent (a);
  return to_scm (e);
}

MAKE_SCHEME_CALLBACK (Grob, stencil_height, 1);
SCM
Grob::stencil_height (SCM smob)
{
  Grob *me = unsmob<Grob> (smob);
  return grob_stencil_extent (me, Y_AXIS);
}

MAKE_SCHEME_CALLBACK (Grob, pure_stencil_height, 3);
SCM
Grob::pure_stencil_height (SCM smob, SCM /* beg */, SCM /* end */)
{
  Grob *me = unsmob<Grob> (smob);
  if (unsmob<Stencil> (get_property_data (me, "stencil")))
    return grob_stencil_extent (me, Y_AXIS);

  return to_scm (Interval ());

}

MAKE_SCHEME_CALLBACK (Grob, y_parent_positioning, 1);
SCM
Grob::y_parent_positioning (SCM smob)
{
  Grob *me = unsmob<Grob> (smob);
  Grob *par = me->get_y_parent ();
  if (par)
    (void) get_property (par, "positioning-done");

  return to_scm (0.0);
}

MAKE_SCHEME_CALLBACK (Grob, x_parent_positioning, 1);
SCM
Grob::x_parent_positioning (SCM smob)
{
  Grob *me = unsmob<Grob> (smob);

  Grob *par = me->get_x_parent ();
  if (par)
    (void) get_property (par, "positioning-done");

  return to_scm (0.0);
}

MAKE_SCHEME_CALLBACK (Grob, stencil_width, 1);
SCM
Grob::stencil_width (SCM smob)
{
  Grob *me = unsmob<Grob> (smob);
  return grob_stencil_extent (me, X_AXIS);
}

Grob *
common_refpoint_of_list (SCM elist, Grob *common, Axis a)
{
  for (; scm_is_pair (elist); elist = scm_cdr (elist))
    if (Grob *s = unsmob<Grob> (scm_car (elist)))
      {
        if (common)
          common = common->common_refpoint (s, a);
        else
          common = s;
      }

  return common;
}

Grob *
common_refpoint_of_array (vector<Grob *> const &arr, Grob *common, Axis a)
{
  for (vsize i = 0; i < arr.size (); i++)
    if (common)
      common = common->common_refpoint (arr[i], a);
    else
      common = arr[i];

  return common;
}

Grob *
common_refpoint_of_array (set<Grob *> const &arr, Grob *common, Axis a)
{
  set<Grob *>::iterator it;

  for (it = arr.begin (); it != arr.end (); it++)
    if (common)
      common = common->common_refpoint (*it, a);
    else
      common = *it;

  return common;
}

Interval
robust_relative_extent (Grob *me, Grob *refpoint, Axis a)
{
  Interval ext = me->extent (refpoint, a);
  if (ext.is_empty ())
    ext.add_point (me->relative_coordinate (refpoint, a));

  return ext;
}

Interval
robust_relative_pure_y_extent (Grob *me, Grob *refpoint, vsize start, vsize end)
{
  Interval ext = me->pure_y_extent (refpoint, start, end);
  if (ext.is_empty ())
    ext.add_point (me->pure_relative_y_coordinate (refpoint, start, end));

  return ext;
}

// Checks whether there is a vertical alignment in the chain of
// parents between this and commony.
bool
Grob::check_cross_staff (Grob *commony)
{
  if (has_interface<Align_interface> (commony))
    return true;

  for (Grob *g = this; g && g != commony; g = g->get_y_parent ())
    if (has_interface<Align_interface> (g))
      return true;

  return false;
}

void
uniquify (vector <Grob *> &grobs)
{
  std::unordered_set<Grob *> seen (grobs.size ());
  vsize j = 0;
  for (vsize i = 0; i < grobs.size (); i++)
    {
      if (seen.insert (grobs[i]).second)
        {
          grobs[j++] = grobs[i];
        }
    }

  grobs.resize (j);
}
