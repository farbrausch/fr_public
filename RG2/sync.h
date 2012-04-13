// This code is in the public domain. See LICENSE for details.

#ifndef __fr_sync_h__
#define __fr_sync_h__

#include <windows.h>
#include "types.h"

namespace fr
{
  /// Critical Section-Wrapper

  class FR_NOVTABLE cSection
  {
  protected:
    CRITICAL_SECTION  m_cSection;

  public:
    /// Standardkonstruktor
    cSection();

    /// Absolut langweiliger Desktruktor
    ~cSection();

    /// In die Critical Section eintreten
    void enter();

    /// Versuchen, in die Critical Section einzutreten
    sBool tryEnter();

    /// Die Critical Section wieder verlassen
    void leave();
  };

  class FR_NOVTABLE cSectionLocker
  {
  protected:
    cSection  &m_section;

  public:
    cSectionLocker(cSection &section)
      : m_section(section)
    {
      m_section.enter();
    }

    ~cSectionLocker()
    {
      m_section.leave();
    }
  };

  /// Mutex-Wrapper

  class FR_NOVTABLE mutex
  {
  protected:
    HANDLE  m_hMutex;

  public:
    /// Standardkonstruktor. Setzt m_hMutex auf INVALID_HANDLE_VALUE, sonst nichts.
    mutex();

    /// Standard-Kopierkonstruktor. Dupliziert das Handle auf den Mutex.
    mutex(const mutex &x);

    /// Nochmal ein Kopierkonstruktor, diesmal mit Handle.
    mutex(const HANDLE hnd);

    /** Der Destruktor. Wenn noch ein Mutex mit dem Objekt verbunden ist, wird er via
     * destroy() zerstört.
     */
    ~mutex();

    /// Operator = mit anderem Mutex (Handle wird dupliziert)
    mutex &operator =(const mutex &x);

    /// Operator = mit Handle (Achtung, hier wird das Handle *nicht* dupliziert!)
    mutex &operator =(const HANDLE hnd);

    /// Castoperator auf ein Handle
    operator HANDLE() const;

    /** Erzeugt einen neuen (unbenannten) Mutex und verbindet das Objekt damit.
     * \param initial Soll der aufrufende Thread den Mutex schon besitzen oder nicht?
     * \return sTRUE wenn der Mutex erfolgreich erzeugt wurde, sonst sFALSE
     */
    sBool create(sBool initial=sFALSE);

    /** Erzeugt einen benannten Mutex und verbindet das Objekt damit (oder holt ein Handle
     * auf einen bestehenden Mutex mit diesem Namen)
     * \param name Der Name des Mutex
     * \param initial Soll der aufrufende Thread den Mutex schon besitzen oder nicht?
     * \return sTRUE wenn der Mutex erfolgreich erzeugt wurde, sonst sFALSE
     */
    sBool create(const sChar *name, sBool initial=sFALSE);

    /** Zerstört den Mutex
     * \return sTRUE wenn der Mutex erfolgreich zerstört wurde, sonst sFALSE
     */
    sBool destroy();

    /** Holt sich den Mutex (oder bricht nach einem Timeout ab)
     * \param timeout Die Timeout-Zeit (oder INFINITE) in Millisekunden
     * \return sTRUE wenn der Thread den Mutex jetzt besitzt, sonst sFALSE
     */
    sBool get(sU32 timeout=INFINITE);

    /** Gibt den Mutex wieder frei
     * \return sTRUE wenn die Funktion erfolgreich war, sonst sFALSE
     */
    sBool release();
  };

  /// Event-Wrapper
  class event
  {
  protected:
    HANDLE  m_hEvent;

  public:
    /// Standardkonstruktor. Setzt m_hEvent auf INVALID_HANDLE_VALUE, sonst nichts.
    event();

    /// Standard-Kopierkonstruktor. Dupliziert das Handle auf den Event.
    event(const event &x);

    /// Nochmal ein Kopierkonstruktor, diesmal mit Handle.
    event(const HANDLE hnd);

    /** Der Destruktor. Wenn noch ein Event mit dem Objekt verbunden ist, wird er via
     * destroy() zerstört.
     */
    ~event();

    /// Operator = mit anderem Event (Handle wird dupliziert)
    event &operator =(const event &x);

    /// Operator = mit Handle (Achtung, hier wird das Handle *nicht* dupliziert!)
    event &operator =(const HANDLE hnd);

    /// Castoperator auf ein Handle
    operator HANDLE() const;

    /** Erzeugt einen neuen (unbenannten) Event und verbindet das Objekt damit.
     * \param initial Soll der Event anfänglich signalisiert sein oder nicht?
     * \return sTRUE wenn der Event erfolgreich erzeugt wurde, sonst sFALSE
     */
    sBool create(sBool initial=sFALSE);

    /** Erzeugt einen benannten Event und verbindet das Objekt damit (oder holt ein Handle
     * auf einen bestehenden Event mit diesem Namen)
     * \param name Der Name des Events
     * \param initial Soll der Event anfänglich signalisiert sein oder nicht?
     * \return sTRUE wenn der Event erfolgreich erzeugt wurde, sonst sFALSE
     */
    sBool create(const sChar *name, sBool initial=sFALSE);

    /** Zerstört den Event
     * \return sTRUE wenn der Event erfolgreich zerstört wurde, sonst sFALSE
     */
    sBool destroy();

    /** Wartet auf den Event (oder bricht nach einem Timeout ab)
     * \param timeout Die Timeout-Zeit (oder INFINITE) in Millisekunden
     * \return sTRUE wenn kein Timeout stattfand, sonst sFALSE
     */
    sBool wait(sU32 timeout=INFINITE);

    /// Setzt den Event
    void set();

    /// Setzt den Event zurück
    void reset();
  };

  /** Read/Write-Sync. Schema: Beliebig viele Threads dürfen lesen, aber nur einer darf
   * schreiben, und beim Schreiben darf kein lesender Thread aktiv sein.
   */
  class rwSync
  {
  protected:
    sU32    m_counter;
    event   m_counterZero;
    mutex   m_writing;

  public:
    /// einfacher Standardkonstruktor
    rwSync();

    /** read-access requesten
     * \param timeout Timeout-Zeit in Millisekunden
     * \return sTRUE, wenn der Thread jetzt Readaccess hat, sonst sFALSE
     */
    sBool requestRead(sU32 timeout=INFINITE);

    /// read-access wieder freigeben
    void releaseRead();

    /** write-access requesten
     * \param timeout Timeout-Zeit in Millisekunden
     * \return sTRUE, wenn der Thread jetzt Writeaccess hat, sonst sFALSE
     */
    sBool requestWrite(sU32 timeout=INFINITE);

    /// write-access wieder freigeben
    void releaseWrite();
  };
}

#endif
