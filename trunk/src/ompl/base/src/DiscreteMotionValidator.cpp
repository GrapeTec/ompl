/*********************************************************************
* Software License Agreement (BSD License)
* 
*  Copyright (c) 2010, Rice University
*  All rights reserved.
* 
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
* 
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Rice University nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
* 
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/* Author: Ioan Sucan */

#include "ompl/base/DiscreteMotionValidator.h"
#include <queue>
#include <cmath>
#include <limits>

void ompl::base::DiscreteMotionValidator::defaultSettings(void)
{
    resolution_ = 0.01;
    stateManifold_ = si_->getStateManifold().get();
}

void ompl::base::DiscreteMotionValidator::setStateValidityCheckingResolution(double resolution)
{  
    if (resolution_ < std::numeric_limits<double>::epsilon() || resolution_ > 1.0 - std::numeric_limits<double>::epsilon())
	throw Exception("The specified resolution at which states need to be checked for validity must be larger than 0 and less than 1");
    resolution_ = resolution;
}
	    
bool ompl::base::DiscreteMotionValidator::checkMotion(const State *s1, const State *s2, std::pair<State*, double> &lastValid) const
{
    /* assume motion starts in a valid configuration so s1 is valid */
    if (!si_->isValid(s2))
	return false;

    bool result = true;
    int nd = (int)ceil(stateManifold_->distanceAsFraction(s1, s2) / resolution_);
    
    /* temporary storage for the checked state */
    State *test = si_->allocState();
    
    for (int j = 1 ; j < nd ; ++j)
    {
	stateManifold_->interpolate(s1, s2, (double)j / (double)nd, test);
	if (!si_->isValid(test))
	{
	    if (lastValid.first)
		stateManifold_->interpolate(s1, s2, (double)(j - 1) / (double)nd, lastValid.first);
	    lastValid.second = (double)(j - 1) / (double)nd;
	    result = false;
	    break;
	}
    }
    si_->freeState(test);
    
    return result;
}

bool ompl::base::DiscreteMotionValidator::checkMotion(const State *s1, const State *s2) const
{
    /* assume motion starts in a valid configuration so s1 is valid */
    if (!si_->isValid(s2))
	return false;
    
    bool result = true;
    int nd = (int)ceil(stateManifold_->distanceAsFraction(s1, s2) / resolution_);
    
    /* initialize the queue of test positions */
    std::queue< std::pair<int, int> > pos;
    if (nd >= 2)
    {
	pos.push(std::make_pair(1, nd - 1));
    
	/* temporary storage for the checked state */
	State *test = si_->allocState();
	
	/* repeatedly subdivide the path segment in the middle (and check the middle) */
	while (!pos.empty())
	{
	    std::pair<int, int> x = pos.front();
	    
	    int mid = (x.first + x.second) / 2;
	    stateManifold_->interpolate(s1, s2, (double)mid / (double)nd, test);
	    
	    if (!si_->isValid(test))
	    {
		result = false;
		break;
	    }
	    
	    pos.pop();
	    
	    if (x.first < mid)
		pos.push(std::make_pair(x.first, mid - 1));
	    if (x.second > mid)
		pos.push(std::make_pair(mid + 1, x.second));
	}
	
	si_->freeState(test);
    }
    
    return result;
}