/*  dynamo:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2011  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    version 3 as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include "system.hpp"
#include "../../base/is_simdata.hpp"
#include "../ranges/1range.hpp"
#include <boost/random/variate_generator.hpp>
#include <boost/random/uniform_real.hpp>
#include <magnet/cloneptr.hpp>

/*! \brief A rescaling thermostat.
 *
 * This event "attempts" to thermostat the system by simply rescaling
 * the kinetic energy periodically. It does this by multiplying all
 * velocities (linear and angular) with a factor calculated like so
 * \f[ F = \sqrt{\frac{k_b\,T_{desired}}{k_b\,T_{current}}} \f] such
 * that the velocities after the event are related to the velocities
 * before by \f[ {\bf v}_{new} = F\, {\bf v}_{old} \f].
 */
class SysRescale: public System
{
public:
  SysRescale(const magnet::xml::Node& XML, dynamo::SimData*);
  SysRescale(dynamo::SimData*, size_t frequency, std::string name, double kT = 1);

  virtual System* Clone() const { return new SysRescale(*this); }

  virtual void runEvent() const;

  virtual void initialise(size_t);

  virtual void operator<<(const magnet::xml::Node&);

  void checker(const NEventData&);
  
  inline const long double& getScaleFactor() const {return scaleFactor; }

protected:
  virtual void outputXML(magnet::xml::XmlStream&) const;

  size_t _frequency;

  double _kT, _timestep;

  mutable long double scaleFactor;

  mutable long double LastTime, RealTime;
  
};
