/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2010  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

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

#ifndef OPMISC_H
#define OPMISC_H

#include "../outputplugin.hpp"
#include <ctime>
#include <time.h>

class OPMisc: public OutputPlugin
{
public:
  OPMisc(const DYNAMO::SimData*, const XMLNode&);
  
  virtual void initialise();
  
  virtual void eventUpdate(const CIntEvent&, const C2ParticleData&);
  
  virtual void eventUpdate(const CGlobEvent&, const CNParticleData&);

  virtual void eventUpdate(const CLocalEvent&, const CNParticleData&);
  
  virtual void eventUpdate(const CSystem&, const CNParticleData&, 
			   const Iflt&);
  
  void output(xmlw::XmlStream &); 
  
  void periodicOutput();
  
  virtual OutputPlugin *Clone() const 
  { return new OPMisc(*this); }
  
  Iflt getMFT() const;
  
  void changeSystem(OutputPlugin*);

protected:
  std::time_t tstartTime;
  timespec acc_tstartTime;

  Iflt oldSysTime;

  unsigned long dualEvents;  
  unsigned long singleEvents;
  unsigned long oldcoll;
};

#endif
