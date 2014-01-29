/*  dynamo:- Event driven molecular dynamics simulator 
    http://www.dynamomd.org
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

#include <dynamo/globals/cells.hpp>
#include <dynamo/globals/globEvent.hpp>
#include <dynamo/NparticleEventData.hpp>
#include <dynamo/dynamics/dynamics.hpp>
#include <dynamo/units/units.hpp>
#include <dynamo/ranges/IDRangeAll.hpp>
#include <dynamo/schedulers/scheduler.hpp>
#include <dynamo/BC/LEBC.hpp>
#include <dynamo/ranges/IDRangeList.hpp>
#include <magnet/xmlwriter.hpp>
#include <magnet/xmlreader.hpp>
#include <cstdio>
#include <set>
#include <algorithm>

namespace dynamo {
  GCells::GCells(dynamo::Simulation* nSim, const std::string& name):
    GNeighbourList(nSim, "CellNeighbourList"),
    cellDimension(1,1,1),
    _inConfig(true),
    _oversizeCells(1.0),
    overlink(1)
  {
    globName = name;
    dout << "Cells Loaded" << std::endl;
  }

  GCells::GCells(const magnet::xml::Node& XML, dynamo::Simulation* ptrSim):
    GNeighbourList(ptrSim, "CellNeighbourList"),
    cellDimension(1,1,1),
    _inConfig(true),
    _oversizeCells(1.0),
    overlink(1)
  {
    operator<<(XML);

    dout << "Cells Loaded" << std::endl;
  }

  void 
  GCells::operator<<(const magnet::xml::Node& XML)
  {
    if (XML.hasAttribute("OverLink"))
      overlink = XML.getAttribute("OverLink").as<size_t>();
    
    if (XML.hasAttribute("NeighbourhoodRange"))
      _maxInteractionRange = XML.getAttribute("NeighbourhoodRange").as<double>() * Sim->units.unitLength();

    if (XML.hasAttribute("Oversize"))
      _oversizeCells = XML.getAttribute("Oversize").as<double>();
    
    if (_oversizeCells < 1.0)
      M_throw() << "You must specify an Oversize greater than 1.0, otherwise your cells are too small!";
    
    globName = XML.getAttribute("Name");
    
    range = shared_ptr<IDRange>(IDRange::getClass(XML.getNode("IDRange"), Sim));
  }

  GlobalEvent 
  GCells::getEvent(const Particle& part) const
  {
#ifdef ISSS_DEBUG
    if (!Sim->dynamics->isUpToDate(part))
      M_throw() << "Particle is not up to date";
#endif

    //Sim->dynamics->updateParticle(part); is not required as we
    //compensate for the delay using
    //Sim->dynamics->getParticleDelay(part)
    return GlobalEvent(part, Sim->dynamics->getSquareCellCollision2(part, calcPosition(_cellData.getCellID(part.getID()), part), cellDimension) - Sim->dynamics->getParticleDelay(part), CELL, *this);

  }

  void
  GCells::runEvent(Particle& part, const double) const
  {
    //Despite the system not being streamed this must be done.  This is
    //because the scheduler and all interactions, locals and systems
    //expect the particle to be up to date.
    Sim->dynamics->updateParticle(part);

    //Get rid of the virtual event we're running, an updated event is
    //pushed after the callbacks are complete (the callbacks may also
    //add events so this must be done first).
    Sim->ptrScheduler->popNextEvent();

    const size_t oldCellIndex = _cellData.getCellID(part.getID());
    const auto oldCellCoord = _ordering.toCoord(oldCellIndex);

    //Determine the cell transition direction
    const int cellDirectionInt(Sim->dynamics->getSquareCellCollision3(part, calcPosition(oldCellIndex, part), cellDimension));
    const size_t cellDirection = abs(cellDirectionInt) - 1;

    //Calculate which cell the particle ends up in
    auto newCellCoord = oldCellCoord;
    newCellCoord[cellDirection] += _ordering.getDimensions()[cellDirection] + ((cellDirectionInt > 0) ? 1 : -1);
    newCellCoord[cellDirection] %= _ordering.getDimensions()[cellDirection];

    _cellData.moveTo(oldCellIndex, _ordering.toIndex(newCellCoord), part.getID());

    //Particle has just arrived into a new cell, check the new
    //neighbours for particles
    auto newCenterNBCellCoord = newCellCoord;
    newCenterNBCellCoord[cellDirection] += _ordering.getDimensions()[cellDirection] + ((cellDirectionInt > 0) ? 1 : -1);
    newCenterNBCellCoord[cellDirection] %= _ordering.getDimensions()[cellDirection];
    std::array<size_t, 3> steps{{overlink, overlink, overlink}};
    steps[cellDirection] = 0;

    for (auto cellIndex : _ordering.getSurroundingIndices(newCenterNBCellCoord, steps))
      for (const size_t& next : _cellData.getCellContents(cellIndex))
	_sigNewNeighbour(part, next);
  
    //Push the next virtual event, this is the reason the scheduler
    //doesn't need a second callback
    Sim->ptrScheduler->pushEvent(part, getEvent(part));
    Sim->ptrScheduler->sort(part);
    _sigCellChange(part, oldCellIndex);
  }

  void 
  GCells::initialise(size_t nID)
  { 
    ID=nID;
    reinitialise();
  }

  void
  GCells::reinitialise()
  {
    GNeighbourList::reinitialise();
      
    dout << "Reinitialising on collision " << Sim->eventCount << std::endl;

    //Create the cells
    addCells(_maxInteractionRange * (1.0 + 10 * std::numeric_limits<double>::epsilon()) * _oversizeCells / overlink);

    _sigReInitialise();

    if (isUsedInScheduler)
      Sim->ptrScheduler->initialise();
  }

  void
  GCells::outputXML(magnet::xml::XmlStream& XML) const
  { 
    if (!_inConfig) return;
    XML << magnet::xml::tag("Global")
	<< magnet::xml::attr("Type") << "Cells"
	<< magnet::xml::attr("Name") << globName
	<< magnet::xml::attr("NeighbourhoodRange") 
	<< _maxInteractionRange / Sim->units.unitLength();
    
    if (overlink > 1)   XML << magnet::xml::attr("OverLink") << overlink;
    if (_oversizeCells != 1.0) XML << magnet::xml::attr("Oversize") << _oversizeCells;
    
    XML << range
	<< magnet::xml::endtag("Global");
  }

  void
  GCells::addCells(double maxdiam)
  {
    _cellData.clear();
    
    std::array<size_t, 3> cellCount;
    for (size_t iDim = 0; iDim < NDIM; iDim++)
      {
	cellCount[iDim] = int(Sim->primaryCellSize[iDim] / (maxdiam * (1.0 + 10 * std::numeric_limits<double>::epsilon())));
      
	if (cellCount[iDim] < 2 * overlink + 1)
	  cellCount[iDim] = 2 * overlink + 1;
	cellLatticeWidth[iDim] = Sim->primaryCellSize[iDim] / cellCount[iDim];
	cellDimension[iDim] = cellLatticeWidth[iDim] + (cellLatticeWidth[iDim] - maxdiam) * lambda;
	cellOffset[iDim] = -(cellLatticeWidth[iDim] - maxdiam) * lambda * 0.5;
      }

    //Set up the ordering
    _ordering = Ordering(cellCount);
    _cellData.resize(_ordering.length(), Sim->particles.size()); //Empty Cells created!

    dout << "Cells <x,y,z> " << _ordering.getDimensions()[0] << ","
	 << _ordering.getDimensions()[1] << "," << _ordering.getDimensions()[2]
	 << "\nCell Offset "
	 << cellOffset[0] / Sim->units.unitLength() << ","
	 << cellOffset[1] / Sim->units.unitLength() << ","
	 << cellOffset[2] / Sim->units.unitLength()
	 << "\nCells Dimension " 
	 << cellDimension[0] / Sim->units.unitLength()
	 << ","
	 << cellDimension[1] / Sim->units.unitLength()
	 << "," 
	 << cellDimension[2] / Sim->units.unitLength()
	 << "\nLattice spacing " 
	 << cellLatticeWidth[0] / Sim->units.unitLength()
	 << ","
	 << cellLatticeWidth[1] / Sim->units.unitLength()
	 << "," 
	 << cellLatticeWidth[2] / Sim->units.unitLength()
	 << "\nRequested interaction range " << overlink * maxdiam / Sim->units.unitLength()
	 << "\nSupported range " << getMaxSupportedInteractionLength() / Sim->units.unitLength()
	 << std::endl;

    if (getMaxSupportedInteractionLength() < maxdiam)
      M_throw() << "The system size is too small to support the range of interactions specified (i.e. the system is smaller than the interaction diameter of one particle).";
  
    ////Add all the particles 
    //Required so particles find the right owning cell
    Sim->dynamics->updateAllParticles();
    for (const size_t& pid : *range)
      {
	Particle& p = Sim->particles[pid];
	_cellData.add(_ordering.toIndex(getCellCoords(p.getPosition())), pid);
      }
  }

  std::array<size_t, 3>
  GCells::getCellCoords(Vector pos) const
  {
    Sim->BCs->applyBC(pos);

    std::array<size_t, 3> retval;

    for (size_t iDim = 0; iDim < NDIM; iDim++)
      {
	long coord = std::floor((pos[iDim] - cellOffset[iDim]) / cellLatticeWidth[iDim] + 0.5 * _ordering.getDimensions()[iDim]);
	coord %= long(_ordering.getDimensions()[iDim]);
	if (coord < 0) coord += _ordering.getDimensions()[iDim];
	retval[iDim] = coord;
      }

    return retval;
  }

  void
  GCells::getParticleNeighbours(const std::array<size_t, 3>& particle_cell_coords, std::vector<size_t>& retlist) const
  {
    for (auto cellIndex : _ordering.getSurroundingIndices(particle_cell_coords, std::array<size_t, 3>{{overlink, overlink, overlink}}))
      {
	const auto& neighbours = _cellData.getCellContents(cellIndex);
	retlist.insert(retlist.end(), neighbours.begin(), neighbours.end());
      }
  }
  
  void
  GCells::getParticleNeighbours(const Particle& part, std::vector<size_t>& retlist) const {
    getParticleNeighbours(_ordering.toCoord(_cellData.getCellID(part.getID())), retlist);
  }

  void
  GCells::getParticleNeighbours(const Vector& vec, std::vector<size_t>& retlist) const {
    return getParticleNeighbours(getCellCoords(vec), retlist);
  }

  double 
  GCells::getMaxSupportedInteractionLength() const
  {
    double retval(HUGE_VAL);

    for (size_t i = 0; i < NDIM; ++i)
      {
	double supported_length = cellLatticeWidth[i] * overlink
	  + lambda * (cellLatticeWidth[i] - cellDimension[i]);

	//Test if, in this dimension, one neighbourhood of cells spans
	//the system. If so, the maximum interaction supported is the
	//system width.
	if (_ordering.getDimensions()[i] == 2 * overlink + 1)
	  supported_length = Sim->primaryCellSize[i];

	retval = std::min(retval, supported_length);
      }

    return retval;
  }

  Vector 
  GCells::calcPosition(const std::array<size_t, 3>& coords, const Particle& part) const
  {
    //We always return the cell that is periodically nearest to the particle
    Vector primaryCell = calcPosition(coords);
    Vector imageCell;
  
    for (size_t i = 0; i < NDIM; ++i)
      imageCell[i] = primaryCell[i] - Sim->primaryCellSize[i] * lrint((primaryCell[i] - part.getPosition()[i]) / Sim->primaryCellSize[i]);

    return imageCell;
  }

  Vector 
  GCells::calcPosition(const std::array<size_t, 3>& coords) const
  {
    Vector primaryCell;
  
    for (size_t i(0); i < NDIM; ++i)
      primaryCell[i] = coords[i] * cellLatticeWidth[i] - 0.5 * Sim->primaryCellSize[i] + cellOffset[i];
  
    return primaryCell;
  }
}
