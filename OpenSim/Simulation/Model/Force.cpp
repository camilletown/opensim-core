/* -------------------------------------------------------------------------- *
 *                            OpenSim:  Force.cpp                             *
 * -------------------------------------------------------------------------- *
 * The OpenSim API is a toolkit for musculoskeletal modeling and simulation.  *
 * See http://opensim.stanford.edu and the NOTICE file for more information.  *
 * OpenSim is developed at Stanford University and supported by the US        *
 * National Institutes of Health (U54 GM072970, R24 HD065690) and by DARPA    *
 * through the Warrior Web program.                                           *
 *                                                                            *
 * Copyright (c) 2005-2012 Stanford University and the Authors                *
 * Author(s): Ajay Seth                                                       *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.         *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

#include "Force.h"
#include "Model.h"
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/ForceAdapter.h>


using namespace SimTK;

namespace OpenSim {

//=============================================================================
// CONSTRUCTOR(S) AND DESTRUCTOR
//=============================================================================
//_____________________________________________________________________________
// Default constructor.
Force::Force()
{
    setNull();
    constructProperties();
}

//=============================================================================
// CONSTRUCTION METHODS
//=============================================================================

//_____________________________________________________________________________
// Set the data members of this Force to their null values.
void Force::setNull()
{
    setAuthors("Peter Eastman, Ajay Seth");
}

//_____________________________________________________________________________
// Define properties.
void Force::constructProperties()
{
    constructProperty_isDisabled(false);
}

// Create an underlying SimTK::Force to represent the OpenSim::Force in the 
// computational system.  Create a SimTK::Force::Custom by default.
void Force::extendAddToSystem(SimTK::MultibodySystem& system) const
{
    Super::extendAddToSystem(system);

    ForceAdapter* adapter = new ForceAdapter(*this);
    SimTK::Force::Custom force(_model->updForceSubsystem(), adapter);

     // Beyond the const Component get the index so we can access the SimTK::Force later
    Force* mutableThis = const_cast<Force *>(this);
    mutableThis->_index = force.getForceIndex();
}


void Force::extendInitStateFromProperties(SimTK::State& s) const
{
    Super::extendInitStateFromProperties(s);

    SimTK::Force& simForce = _model->updForceSubsystem().updForce(_index);

    // Otherwise we have to change the status of the constraint
    if(get_isDisabled())
        simForce.disable(s);
    else
        simForce.enable(s);

}

void Force::extendSetPropertiesFromState(const SimTK::State& state)
{
    Super::extendSetPropertiesFromState(state);

    set_isDisabled(isDisabled(state));
}


//_____________________________________________________________________________
/**
 * Set whether or not this Force is disabled.
 * Simbody multibody system instance is realized every time the isDisabled
 * changes, BUT multiple sets to the same value have no cost.
 *
 * @param isDisabled If true the force is disabled; if false the Force is enabled.
 */
void Force::setDisabled(SimTK::State& s, bool isDisabled) const
{
    if(_index.isValid()){
        SimTK::Force& simtkForce = _model->updForceSubsystem().updForce(_index);
        if(isDisabled)
            simtkForce.disable(s);
        else
            simtkForce.enable(s);
    }
}

bool Force::isDisabled(const SimTK::State& s) const
{
    if(_index.isValid()){
        SimTK::Force& simtkForce = _model->updForceSubsystem().updForce(_index);
        return simtkForce.isDisabled(s);
    }
    return get_isDisabled();
}

//-----------------------------------------------------------------------------
// ABSTRACT METHODS
//-----------------------------------------------------------------------------
//_____________________________________________________________________________
double Force::computePotentialEnergy(const SimTK::State& state) const
{
    return 0.0;
}

//-----------------------------------------------------------------------------
// METHODS TO APPLY FORCES AND TORQUES
//-----------------------------------------------------------------------------
void Force::applyForceToPoint(const SimTK::State &s, const PhysicalFrame &frame,
                              const Vec3& point, const Vec3& forceInG, 
                              Vector_<SpatialVec> &bodyForces) const
{
    // get the point expressed in frame, F, expressed in the base, B.
    auto p_B = frame.findTransformInBaseFrame()*point;

    _model->getMatterSubsystem().addInStationForce(s, 
                                    frame.getMobilizedBodyIndex(),
                                    p_B, forceInG, bodyForces);
}

void Force::applyTorque(const SimTK::State &s, const PhysicalFrame& frame, 
                        const Vec3& torque, Vector_<SpatialVec> &bodyForces) const
{
    _model->getMatterSubsystem().addInBodyTorque(s, frame.getMobilizedBodyIndex(),
                                                 torque, bodyForces);
}

void Force::applyGeneralizedForce(const SimTK::State &s, const Coordinate &coord, 
                                        double force, Vector &mobilityForces) const
{
    _model->getMatterSubsystem().addInMobilityForce(s, 
                                 SimTK::MobilizedBodyIndex(coord.getBodyIndex()), 
                                 SimTK::MobilizerUIndex(coord.getMobilizerQIndex()),
                                 force, mobilityForces);
}


} // end of namespace OpenSim
