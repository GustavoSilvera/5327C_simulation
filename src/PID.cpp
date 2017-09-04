#include "PID.h"
#include <string>

using namespace ci;
PID::PID(robot *r) {
	pid.isRunning = true;
	isInit = false;
	r->p.acceleration.X = 0;
	r->p.velocity.X = r->p.velocity.Y = 0;
	r->p.mRot = 0;
	r->p.position.Y = 69.6;
	pid.requestedValue = 69.6;
	gr.YAxLength = 250;
	gr.XAxLength = 1000;
	gr.drawX = 200;
	gr.drawY = 700;
	gr.midpoint = ((gr.YAxLength) / 2) + gr.drawY;
	for (int i = 0; i < maxDots; i++) {
		gr.posY[i] = gr.midpoint;
	}
	pidVel = true;
}
void PID::initialize(robot *r) {
	pid.isRunning = true;
	r->reset();
	r->p.mRot = 0;
	r->p.position.Y = 69.6;
	pid.requestedValue = 69.6;
	isInit = true;
	pidVel = true;
}
void PID::textOutput(robot *r) {
	static const int columns = 5;
	int tX[columns], dInBtw = 185;//array for #buttons, bY is y position of each btn, dInBtw is distance in bwtween buttons
							  //tX = 1200;
	int tY = 100;
	std::string STRING;
	float DATA;
	for (int i = 0; i < columns; i++) {
		if (i == 0) { STRING = "Req:"; DATA = pid.requestedValue / ppi; }
		else if (i == 1) { STRING = "Error:"; DATA = pid.error; }
		else if (i == 2) { STRING = "Deriv:"; DATA = pid.derivative; }
		else if (i == 3) { STRING = "Intgrl:"; DATA = pid.integral; }
		else if (i == 4) { STRING = "Full:"; DATA = controller(r); }
		
		tX[0] = 0;//initialize first button
		tX[i] = (i + 1) * dInBtw;//increment x position for each button based off index
		gl::drawString(STRING, Vec2f(tX[i] - 70, tY), Color(1, 1, 1), Font("Arial", 30));
		drawText(DATA, vec3(tX[i], tY), vec3(1, 1, 1), 30);
	}
	gl::color(1, 0, 0);
	int linePos = 400;
	drawText(controller(r) / (127 * ppi), vec3(r->p.position.X*ppi, 200), vec3(1, 1, 1), 30);
	gl::drawLine(cinder::Vec2f(r->p.position.X*ppi, linePos), Vec2f(pid.requestedValue, linePos));
	gl::drawLine(cinder::Vec2f(r->p.position.X*ppi, linePos+10), Vec2f(r->p.position.X*ppi, linePos - 10));//lil end ticks
	gl::drawLine(cinder::Vec2f(pid.requestedValue, linePos + 10), Vec2f(pid.requestedValue, linePos - 10));//lil end ticks

	gl::color(1, 1, 1);
}
void PID::graphPlot() {
	//axis:
	gl::drawSolidRect(Rectf(gr.drawX, gr.drawY, gr.drawX + 2, gr.drawY + gr.YAxLength));
	gl::drawSolidRect(Rectf(gr.drawX, gr.midpoint - 1, gr.drawX + gr.XAxLength, gr.midpoint + 1));
	gl::drawString("127", Vec2f(gr.drawX - 30, gr.drawY), Color(1, 1, 1), Font("Arial", 20));
	gl::drawString("-127", Vec2f(gr.drawX - 35, gr.YAxLength + gr.drawY + 20), Color(1, 1, 1), Font("Arial", 20));
	gl::drawString("0", Vec2f(gr.drawX - 15, gr.midpoint), Color(1, 1, 1), Font("Arial", 20));
	//lines:
	int dSize = 2;
	gl::color(Color(0, 253, 255)); // light blue
	for (int i = 0; i < maxDots; i++) {
		int dotX = gr.drawX + i * (gr.XAxLength) / maxDots;//makes the little intervals for the X axis line
		int dotY = gr.posY[i];
		gl::drawSolidCircle(Vec2f(dotX, dotY), dSize);
	}
	gl::color(Color(1, 0, 0)); // red
	for (int i = 0; i < maxDots; i++) {
		int dotX = gr.drawX + i * (gr.XAxLength) / maxDots;//makes the little intervals for the X axis line
		int dotY = gr.posY2[i];
		gl::drawSolidCircle(Vec2f(dotX, dotY), dSize);
	}
	gl::color(Color(255.0/256.0, 255.0/256.0, 0)); // yellow
	for (int i = 0; i < maxDots; i++) {
		int dotX = gr.drawX + i * (gr.XAxLength) / maxDots;//makes the little intervals for the X axis line
		int dotY = gr.posY3[i];
		gl::drawSolidCircle(Vec2f(dotX, dotY), dSize);
	}
	gl::color(Color::white());//resets the colour 

}
void PID::reset(robot *r) {
	r->p.acceleration.X = 0;
	r->p.velocity.X = r->p.velocity.Y = 0;
	r->p.mRot = 0;
	r->p.position.Y = 69.6;
	r->p.position.X = 69.6;
	pid.requestedValue = 69.6*ppi;
}
float PID::controller(robot *r) {
	float kP = 1;//remove later
	float kI = 5;//remove later
	float kD = 1.0;//remove later
				   // If we are using an encoder then clear it
	pid.lastError = 0;
	pid.integral = 0;

	if (pid.isRunning) {
		//pid.currentPos = current.Ypos;// * sensorScale;idk if i need this, probs not.
		if (abs(r->p.position.X*ppi - pid.requestedValue) > 0.001) {
			pid.error = r->p.position.X*ppi - pid.requestedValue;//calculate error
		}
		else pid.error = 0;
		if (kI != 0) {
			if (abs(pid.error) < 50) pid.integral += pid.error;//used for averaging the integral amount, later in motor power divided by 25
			else pid.integral = 0;
		}
		else pid.integral = 0;
		// calculate the derivative
		pid.derivative = 2*(pid.error - pid.lastError);
		pid.lastError = pid.error;
		// calculate drive (in this case, just for the robot)
		return(getSign(pid.error)*abs((kP * pid.error) + (pid.integral / kI) + (kD * pid.derivative)));
	}
	else {
		// clear all
		pid.error = 0;
		pid.lastError = 0;
		pid.integral = 0;
		pid.derivative = 0;
		return(0);
	}
}
void PID::PIDUpdate(robot *r) {
	if (!isInit) initialize(r);//checks if not initialized
	pid.isRunning = true;
	if(pidVel)r->p.velocity.X = controller(r) / (127);
	else r->p.acceleration.X = controller(r) / (127);

	r->p.mRot = 0;
	r->p.position.Y = 69.6;
	float scale = 0.1;
	for (int i = 0; i < (maxDots - 1); i++) {//blue line CONTROLLER
		gr.posY[i] = gr.posY[i + 1];
	}
	gr.posY[maxDots - 1] = (controller(r)*scale) + gr.midpoint;
	for (int i = 0; i < (maxDots - 1); i++) {//red line ERROR P
		gr.posY2[i] = gr.posY2[i + 1];
	}
	gr.posY2[maxDots - 1] = (pid.error*scale) + gr.midpoint;
	for (int i = 0; i < (maxDots - 1); i++) {//yellow line INTEGRAL
		gr.posY3[i] = gr.posY3[i + 1];
	}
	gr.posY3[maxDots - 1] = (pid.derivative*scale) + gr.midpoint;
}

