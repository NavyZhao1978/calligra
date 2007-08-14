/* This file is part of the KDE project
   Copyright (C) 2005 Dag Andersen <danders@get2net.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation;
   version 2 of the License.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KPTAPPOINTMENT_H
#define KPTAPPOINTMENT_H

#include "kptglobal.h"

#include "kptduration.h"
#include "kptdatetime.h"

#include <KoXmlReader.h>

#include <QString>
#include <QList>

#include <kdebug.h>

class QDomElement;

namespace KPlato
{

class Effort;
class Appointment;
class Node;
class Resource;
class ResourceRequest;
class EffortCostMap;
class Schedule;
class XMLLoaderObject;


class AppointmentInterval {
public:
    AppointmentInterval();
    AppointmentInterval(const AppointmentInterval &AppointmentInterval);
    AppointmentInterval(const DateTime &start, const DateTime end, double load=100);
    ~AppointmentInterval();
    
    void set(DateTime &start, DateTime &end, double load=100);
    void set(DateTime &start, Duration &duration, double load=100);
    
    Duration effort() const { return (m_end - m_start) * m_load / 100; }
    Duration effort(const DateTime &start, const DateTime end) const;
    Duration effort(const QDate &time, bool upto) const;
    
    bool loadXML(KoXmlElement &element, XMLLoaderObject &status);
    void saveXML(QDomElement &element) const;
    
    const DateTime &startTime() const { return m_start; }
    void setStartTime(const DateTime &time) { m_start = time; }
    const DateTime &endTime() const { return m_end; }
    void setEndTime(const DateTime &time) { m_end = time; }
    double load() const { return m_load; }
    void setLoad(double load) { m_load = load; }
    
    bool isValid() const;
    AppointmentInterval firstInterval(const AppointmentInterval &interval, const DateTime &from) const;

    //void inSort(AppointmentInterval *a);
    //bool operator<( AppointmentInterval &interval ) const;
private:
    DateTime m_start;
    DateTime m_end;
    double m_load; //percent
};


/**
 * This list is sorted after 1) startdatetime, 2) enddatetime.
 * The intervals do not overlap, an interval does not start before the
 * previous interval ends.
 */
class AppointmentIntervalList : public QMap<QString, AppointmentInterval*> {
public:
    void inSort(AppointmentInterval *a);
};
typedef QListIterator<AppointmentInterval*> AppointmentIntervalListIterator;

/**
 * A resource (@ref Resource) can be scheduled to be used at any time, 
 * this is represented internally with Appointments
 * There is one Appointment per resource-task pair.
 * An appointment can be divided into several intervals, represented with
 * a list of AppointmentInterval.
 * This list is sorted after 1) startdatetime, 2) enddatetime.
 * The intervals do not overlap, an interval does not start before the
 * previous interval ends.
 * An interval is a countinous time interval with the same load. It can span dates.
 */
class Appointment {
public:
    explicit Appointment();
    Appointment(Schedule *resource, Schedule *node, DateTime start, DateTime end, double load);
    Appointment(Schedule *resource, Schedule *node, DateTime start, Duration duration, double load);
    Appointment( const Appointment &app );
    ~Appointment();

    bool isEmpty() const { return m_intervals.isEmpty(); }
    
    // get/set member values.
    Schedule *node() const { return m_node; }
    void setNode(Schedule *n) { m_node = n; }

    Schedule *resource() const { return m_resource; }
    void setResource(Schedule *r) { m_resource = r; }

    DateTime startTime() const;
    DateTime endTime() const;
    double maxLoad() const;
    
    const Duration &repeatInterval() const {return m_repeatInterval;}
    void setRepeatInterval(Duration ri) {m_repeatInterval=ri;}

    int repeatCount() const { return m_repeatCount; }
    void setRepeatCount(int rc) { m_repeatCount=rc; }

    void deleteAppointmentFromRepeatList(DateTime time);
    void addAppointmentToRepeatList(DateTime time);

    bool isBusy(const DateTime &start, const DateTime &end);

    /// attach appointment to resource and node
    bool attach();
    /// detach appointment from resource and node
    void detach();
    
    void addInterval(AppointmentInterval *a);
    void addInterval(AppointmentInterval &a) 
        { addInterval(new AppointmentInterval(a)); }
    void addInterval(const DateTime &start, const DateTime &end, double load=100);
    void addInterval(const DateTime &start, const Duration &duration, double load=100);
    
    const AppointmentIntervalList &intervals() const { return m_intervals; }

    bool loadXML(KoXmlElement &element, XMLLoaderObject &status, Schedule &sch);
    void saveXML(QDomElement &element) const;

    /**
     * Returns the planned effort and cost for the interval start to end (inclusive).
     * Only dates with any planned effort is returned.
     * If start or end is not valid, startTime.date() respectivly endTime().date() is used.
     */
    EffortCostMap plannedPrDay(const QDate& start, const QDate& end) const;
    
    /// Returns the planned effort from start to end
    Duration effort(const DateTime &start, const DateTime &end) const;
    /// Returns the planned effort from start for the duration
    Duration effort(const DateTime &start, const Duration &duration) const;
    /// Returns the planned effort from time onwards
    Duration effortFrom(const QDate &time) const;
    
    /// Returns the total planned effort for this appointment
    Duration plannedEffort() const;
    /// Returns the planned effort on the date
    Duration plannedEffort(const QDate &date) const;
    /// Returns the planned effort upto and including date
    Duration plannedEffortTo(const QDate &date) const;

     /// Calculates the total planned cost for this appointment
    double plannedCost();
    /// Calculates the planned cost on date
    double plannedCost(const QDate &date);
    /// Calculates the planned cost upto and including date
    double plannedCostTo(const QDate &date);

    Appointment &operator=(const Appointment &app);
    Appointment &operator+=(const Appointment &app);
    Appointment operator+(const Appointment &app);
    
    void setCalculationMode( int mode ) { m_calculationMode = mode; }
    int calculationMode() const { return m_calculationMode; }
    
protected:
    void copy(const Appointment &app);
    void merge(const Appointment &app);
    
private:
    Schedule *m_node;
    Schedule *m_resource;
    int m_calculationMode; // Type of appointment
    Duration m_repeatInterval;
    int m_repeatCount;
    QList<Duration*> m_extraRepeats;
    QList<Duration*> m_skipRepeats;

    AppointmentIntervalList m_intervals;
    

#ifndef NDEBUG
public:
        void printDebug(const QString& ident);
#endif
};


}  //KPlato namespace

#endif
