// adapted from  https://github.com/matthias-research/pages/blob/master/challenges/PBD.js (MIT license)

#include <rawkit/rawkit.h>
#include "body-chain/pose.h"
#include "body-chain/body.h"
#include "body-chain/joint.h"

struct State {
  float maxRotationPerSubstep = 0.5f;
};


void setup() {}

void loop() {}
/*



// Simulate -----------------------------------------------------------

function simulate(bodies, joints, timeStep, numSubsteps, gravity) {
    let dt = timeStep / numSubsteps;

    for (let i = 0; i < numSubsteps; i++) {
        for (let j = 0; j < bodies.length; j++)
            bodies[j].integrate(dt, gravity);

        for (let j = 0; j < joints.length; j++)
            joints[j].solvePos(dt);

        for (let j = 0; j < bodies.length; j++)
            bodies[j].update(dt);

        for (let j = 0; j < joints.length; j++)
            joints[j].solveVel(dt);
    }
}
*/