#ifndef PHYSICSSYSTEM_H
#define PHYSICSSYSTEM_H


// an object can have certain physical properties.
// we put them into an extra struct for the sake of clearer code
// and to make integration with falcon easier.
struct PhysProps
{
    float weight; // in kg
    float xspeed; // current speed, x axis. negative means left, positive means right
    float yspeed; // current speed, y axis. negative means up, positive means down
    float xmaxspeed; // max speed. must NOT be negative, ever!
    float ymaxspeed;
    float xaccel; // current x-axis acceleration. negative means left, positive means right
    float yaccel; // current y-axis acceleration. negative means up, positive means down
    float inertia; //
};

// and the environment can have certain physical properties too
struct EnvPhysProps
{
    float airFriction;
    float gravity;


};



#endif
