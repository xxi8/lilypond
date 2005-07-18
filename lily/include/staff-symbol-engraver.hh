/*
  staff-symbol-engraver.hh -- declare Staff_symbol_engraver

  source file of the GNU LilyPond music typesetter

  (c) 2005 Han-Wen Nienhuys <hanwen@xs4all.nl>

*/

#ifndef STAFF_SYMBOL_ENGRAVER_HH
#define STAFF_SYMBOL_ENGRAVER_HH

#include "engraver.hh"
#include "drul-array.hh"

class Staff_symbol_engraver : public Engraver
{
public:
  TRANSLATOR_DECLARATIONS (Staff_symbol_engraver);

private:

protected:
  Drul_array<Music *> span_events_;
  Spanner *span_;
  Spanner *finished_span_;
  bool first_start_;
  
protected:
  virtual void start_spanner ();
  virtual void stop_spanner ();

  PRECOMPUTED_VIRTUAL void stop_translation_timestep ();
  virtual bool try_music (Music *);
  virtual ~Staff_symbol_engraver ();
  virtual void acknowledge_grob (Grob_info);
  virtual void finalize ();
  PRECOMPUTED_VIRTUAL void process_music ();
};

#endif /* STAFF_SYMBOL_ENGRAVER_HH */
