/*
  completion-note-heads-engraver.cc -- Completion_heads_engraver

  (c) 1997--2004 Han-Wen Nienhuys <hanwen@cs.uu.nl>
*/

#include <ctype.h>

#include "rhythmic-head.hh"
#include "paper-def.hh"
#include "event.hh"
#include "dots.hh"
#include "dot-column.hh"
#include "staff-symbol-referencer.hh"
#include "item.hh"
#include "score-engraver.hh"
#include "warn.hh"
#include "spanner.hh"
#include "tie.hh"
#include "global-context.hh"

/*
TODO: make matching rest engraver.
*/

/*

  How does this work?

  When we catch the note, we predict the end of the note. We keep the
  events living until we reach the predicted end-time.

  Every time process_music () is called and there are note events, we
  figure out how long the note to typeset should be. It should be no
  longer than what's specified, than what is left to do and it should
  not cross barlines.
  
  We copy the reqs into scratch note reqs, to make sure that we get
  all durations exactly right.
*/

class Completion_heads_engraver : public Engraver
{
  Link_array<Item> notes_;
  Link_array<Item> prev_notes_;
  Link_array<Grob> ties_;
  
  Link_array<Item> dots_;
  Link_array<Music> note_reqs_;
  Link_array<Music> scratch_note_reqs_;

  Moment note_end_mom_;
  bool first_b_;
  Rational left_to_do_;
  Rational do_nothing_until_;
  
  Moment next_barline_moment ();
  Duration find_nearest_duration (Rational length);
  
public:
  TRANSLATOR_DECLARATIONS (Completion_heads_engraver);

protected:
  virtual void initialize ();
  virtual void start_translation_timestep ();
  virtual bool try_music (Music *req) ;
  virtual void process_music ();
  virtual void stop_translation_timestep ();
};

void
Completion_heads_engraver::initialize ()
{
  first_b_ = false;
}

bool
Completion_heads_engraver::try_music (Music *m) 
{
  if (m->is_mus_type ("note-event"))
    {
      note_reqs_.push (m);

      first_b_ = true;
      Moment musiclen = m->get_length ();
      Moment now = now_mom ();

      if (now_mom ().grace_part_)
	{
	  musiclen.grace_part_ = musiclen.main_part_ ;
	  musiclen.main_part_ = Rational (0,1);
	}
      note_end_mom_  = note_end_mom_ >? (now + musiclen);
      do_nothing_until_ = Rational (0,0);
      
      return true;
    }
  else if  (m->is_mus_type ("busy-playing-event"))
    {
      return note_reqs_.size ();
    }
  
  return false;
  
}

/*
  The duration _until_ the next barline.
 */
Moment
Completion_heads_engraver::next_barline_moment ( )
{
  Moment *e = unsmob_moment (get_property ("measurePosition"));
  Moment *l = unsmob_moment (get_property ("measureLength"));
  if (!e || !l)
    {
      programming_error ("No timing props set?");
      return Moment (1,1);
    }

  return (*l - *e);
}

Duration  
Completion_heads_engraver::find_nearest_duration (Rational length)
{
  int log_limit= 6;

  Duration d (0,0);

  /*
    this could surely be done more efficient. Left to the reader as an
    excercise.  */
  while (d.get_length () > length && d.duration_log () < log_limit)
    {
      if (d.dot_count ())
	{
	  d = Duration (d.duration_log (), d.dot_count ()- 1);
	  continue;
	}
      else
	{
	  d = Duration (d.duration_log () + 1, 2);
	}
    }

  if (d.duration_log () >= log_limit)
    {
      // junk the dots.
      d = Duration (d.duration_log (), 0);

      // scale up.
      d = d.compressed (length / d.get_length ());
    }
  
  return d;
}

void
Completion_heads_engraver::process_music ()
{
  if (!first_b_ && !left_to_do_)
    return ;
  
  first_b_ = false;

  Moment now =  now_mom ();
  if (do_nothing_until_ > now.main_part_)
    return ;
  
  Duration note_dur;
  Duration *orig = 0;
  if (left_to_do_)
    {
      note_dur = find_nearest_duration (left_to_do_);
    }
  else
    {
      orig = unsmob_duration (note_reqs_[0]->get_property ("duration"));
      note_dur = *orig;
    }
  Moment nb = next_barline_moment ();
  if (nb < note_dur.get_length ())
    {
      note_dur = find_nearest_duration (nb.main_part_);

      Moment next = now;
      next.main_part_ += note_dur.get_length ();
      
      get_global_context ()->add_moment_to_process (next);
      do_nothing_until_ = next.main_part_;
    }

  if (orig)
    {
      left_to_do_ = orig->get_length ();
    }

  if (orig && note_dur.get_length () != orig->get_length ())
    {
      if (!scratch_note_reqs_.size ())
	for (int i = 0; i < note_reqs_.size (); i++)
	  {
	    Music * m = note_reqs_[i]->clone ();
	    scratch_note_reqs_.push (m);
	  }
    }

  
  for (int i = 0;
       left_to_do_ && i < note_reqs_.size (); i++)
    {
      Item *note  = make_item ("NoteHead");
      
      Music * req =  note_reqs_[i];
      if (scratch_note_reqs_.size ())
	{
	  req = scratch_note_reqs_[i];
	  SCM pits = note_reqs_[i]->get_property ("pitch");
	  req->set_property ("pitch",pits);
	}
      
      req->set_property ("duration", note_dur.smobbed_copy ());
      note->set_property ("duration-log",
				 gh_int2scm (note_dur.duration_log ()));
      
      int dots= note_dur.dot_count ();
      if (dots)
	{
	  Item * d = make_item ("Dots");
	  Rhythmic_head::set_dots (note, d);

	  /*
	   measly attempt to save an eeny-weenie bit of memory.
	  */
	  if (dots != gh_scm2int (d->get_property ("dot-count")))
	    d->set_property ("dot-count", gh_int2scm (dots));

	  d->set_parent (note, Y_AXIS);
	  announce_grob (d, SCM_EOL);
	  dots_.push (d);
	}

      Pitch *pit =unsmob_pitch (req->get_property ("pitch"));

      int pos = pit->steps ();
      SCM c0 = get_property ("middleCPosition");
      if (gh_number_p (c0))
	pos += gh_scm2int (c0);

      note->set_property ("staff-position",   gh_int2scm (pos));
      announce_grob (note,req->self_scm ());
      notes_.push (note);
    }
  
  if (prev_notes_.size () == notes_.size ())
    {
      for (int i= 0; i < notes_.size (); i++)
	{
	  Grob * p = make_spanner ("Tie");
	  Tie::set_interface (p); // cannot remove yet!
	  
	  Tie::set_head (p, LEFT, prev_notes_[i]);
	  Tie::set_head (p, RIGHT, notes_[i]);
	  
	  ties_.push (p);
	  announce_grob (p, SCM_EOL);
	}
    }

  left_to_do_ -= note_dur.get_length ();

  /*
    don't do complicated arithmetic with grace notes.
   */
  if (orig
      &&  now_mom ().grace_part_ )
    {
      left_to_do_ = Rational (0,0);
    }
}
 
void
Completion_heads_engraver::stop_translation_timestep ()
{
  for (int i = ties_.size (); i--;)
    typeset_grob (ties_[i]); 
  ties_.clear ();
  
  for (int i=0; i < notes_.size (); i++)
    {
      typeset_grob (notes_[i]);
    }
  if (notes_.size ())
    prev_notes_ = notes_;
  notes_.clear ();
  
  for (int i=0; i < dots_.size (); i++)
    {
      typeset_grob (dots_[i]);
    }
  dots_.clear ();

  for (int i = scratch_note_reqs_.size (); i--;)
    {
      scm_gc_unprotect_object (scratch_note_reqs_[i]->self_scm () );
    }
  
  scratch_note_reqs_.clear ();
}

void
Completion_heads_engraver::start_translation_timestep ()
{
  Moment now = now_mom ();
  if (note_end_mom_.main_part_ <= now.main_part_)
    {
      note_reqs_.clear ();
      prev_notes_.clear ();
    }
}

Completion_heads_engraver::Completion_heads_engraver ()
{
}

ENTER_DESCRIPTION (Completion_heads_engraver,
/* descr */       "This engraver replaces "
"@code{Note_heads_engraver}. It plays some trickery to "
"break long notes and automatically tie them into the next measure.",
/* creats*/       "NoteHead Dots Tie",
/* accepts */     "busy-playing-event note-event",
/* acks  */      "",
/* reads */       "middleCPosition measurePosition measureLength",
/* write */       "");
