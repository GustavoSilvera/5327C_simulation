#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"
#include "cinder/Vector.h"
#include "cinder/Text.h"
#include "cinder/Font.h"
//my own headers
#include "joystick.h"
#include "field.h"
#include "robot.h"
#include "vec3.h"
#include "randomstuff.h"

//declaration for the main things, simulation and whatnot
using namespace ci;
using namespace ci::app;
using namespace std;

static const int maxDots = 500;//maximum amount of graph particles for the truspeed sim

vec3 startPos;
vec3 mousePos;

class vex {
public:
	robot r;
	field f;
	joystick j;

	vex() : r(vec3(69.6,69.6,0), vec3()) {}
};

//begin
struct simulation {
	bool mouseClicked = false;
	int hovering = 1;//which button is being hovered over. if any
	enum SimulationType {
		PIDCTRL,
		NAVIGATION,
		TRUSPEED,
		FIELD
	};
	SimulationType SimRunning = NAVIGATION;//in accordance to which simulation is running, 1 is PID, 2 is NAV, 3 is truspeed... etc

	struct graph {
		//graph for speed of the speed of the wheels based off position
		//for graph moving
		double RYpos[maxDots];
		double BYpos[maxDots];
		//int Xpos[maxDots];
		//issue: yaxix length not scaling well with particles
		int YAxLength = 300, XAxLength = 500;
		int drawX = 800, drawY = 80;
		int midpoint = ((YAxLength) / 2) + drawY;//midpoint of the graph, if the graph starts 20 points down.tY;
		
		void graphPlot(){
			//axis:
			gl::drawSolidRect(Rectf(drawX, drawY, drawX+2, drawY + YAxLength));
			gl::drawSolidRect(Rectf(drawX, midpoint - 1, drawX + XAxLength, midpoint + 1));
			gl::drawString("127", Vec2f(drawX-30, drawY), Color(1, 1, 1), Font("Arial", 20));
			gl::drawString("-127", Vec2f(drawX-35, YAxLength + drawY + 20), Color(1, 1, 1), Font("Arial", 20));
			gl::drawString("0", Vec2f(drawX-15, midpoint), Color(1, 1, 1), Font("Arial", 20));
			//lines:
			gl::color(Color(1, 0, 0)); // blue
			for (int i = 0; i < maxDots; i++) {
				int dotX = drawX + i * (XAxLength) / maxDots;//makes the little intervals for the X axis line
				int dotY = RYpos[i];
				gl::drawSolidRect(Area(dotX + 1, dotY - 1, dotX - 1, dotY + 1));
			}
			gl::color(Color(0, 253, 255)); // light blue
			for (int i = 0; i < maxDots; i++) {
				int dotX = drawX + i * (XAxLength) / maxDots;//makes the little intervals for the X axis line
				int dotY = BYpos[i];
				gl::drawSolidRect(Area(dotX + 1, dotY - 1, dotX - 1, dotY + 1));
			}
			gl::color(Color::white());//resets the colour 
		}
		void textOutput(vex *v) {
			//title
			//gl::drawString("WRITE TITLE HERE", Vec2f(10, 10), Color(1, 1, 1), Font("Arial", 25));
			//Defining red line
			gl::drawString("X: ", Vec2f(drawX + 40, drawY-15), Color(1, 1, 1), Font("Arial", 20));
			gl::color(Color(1, 0, 0)); // light blue
			gl::drawSolidRect(Rectf(drawX+60, drawY-18, drawX+70, drawY-2));
			//defining blue line
			gl::drawString("Y: ", Vec2f(drawX+90, drawY-15), Color(1, 1, 1), Font("Arial", 20));
			gl::color(Color(0, 253, 255)); // light blue
			gl::drawSolidRect(Rectf(drawX+110, drawY-18, drawX+120, drawY-2));
			gl::color(Color::white());//resets the colour
									  //other information:
									  //actual X
			stringstream actualX;
			string actualX2;
			actualX << v->j.analogX;
			actualX >> actualX2;
			gl::drawString("Actual X:", Vec2f(drawX+30, YAxLength + drawY + 10), Color(1, 1, 1), Font("Arial", 25));
			gl::drawString(actualX2, Vec2f(drawX+130, YAxLength + drawY + 10), Color(1, 1, 1), Font("Arial", 25));
			//modified X
			stringstream truSpedX;
			string truSpedX2;
			truSpedX << setprecision(3) << v->r.truSpeed(3, v->j.analogX);
			truSpedX >> truSpedX2;
			gl::drawString("-->", Vec2f(drawX+170, YAxLength + drawY + 10), Color(1, 1, 1), Font("Arial", 25));
			gl::drawString(truSpedX2, Vec2f(drawX+210, YAxLength + drawY + 10), Color(1, 1, 1), Font("Arial", 25));
			//actual Y
			stringstream actualY;
			string actualY2;
			actualY << v->j.analogY;
			actualY >> actualY2;
			gl::drawString("Actual Y:", Vec2f(drawX+30, YAxLength + drawY + 30), Color(1, 1, 1), Font("Arial", 25));
			gl::drawString(actualY2, Vec2f(drawX + 130, YAxLength + drawY + 30), Color(1, 1, 1), Font("Arial", 25));
			//modified X
			stringstream truSpedY;
			string truSpedY2;
			truSpedY << setprecision(3) << v->r.truSpeed(3, v->j.analogY);
			truSpedY >> truSpedY2;
			gl::drawString("-->", Vec2f(drawX+170, YAxLength + drawY + 30), Color(1, 1, 1), Font("Arial", 25));
			gl::drawString( truSpedY2, Vec2f(drawX+210, YAxLength + drawY + 30), Color(1, 1, 1), Font("Arial", 25));
		}
	};
	struct graph gr;
};
simulation s;

class CimulationApp : public AppNative {
public:
	void setup();
	void mouseDown(MouseEvent event);
	void mouseUp(MouseEvent event);
	void mouseMove(MouseEvent event);
	void keyDown(KeyEvent event);
	void keyUp(KeyEvent event);
	void update();
	void draw();

	vex v;

};

void CimulationApp::setup() {
	srand(time(NULL));
	gl::enableVerticalSync();
	v.r.TankBase = gl::Texture(loadImage(loadAsset("Tank Drive.png")));
	v.f.fieldBare = gl::Texture(loadImage(loadAsset("InTheZoneFieldBare.jpg")));
	v.f.fieldFull = gl::Texture(loadImage(loadAsset("InTheZoneFieldFull.jpg")));
	v.f.coneTexture = gl::Texture(loadImage(loadAsset("InTheZoneCone.png")));
	setWindowSize(WindowWidth, WindowHeight);
	v.r.current.Xpos = 0;
	v.r.current.Ypos = 0;
	v.r.current.deg = 0;
	v.r.encoder1 = 0;
	v.r.encoderLast = 0;
	v.r.PID.isRunning = false;
	v.r.PID.requestedValue = v.r.position.X*ppi;
	v.r.position.X = 69.6;
	v.r.position.Y = 69.6;
	//for graph
	for (int i = 0; i < 30; i++) {
		s.gr.RYpos[i] = s.gr.midpoint;
		s.gr.BYpos[i] = s.gr.midpoint;
	}
}
//cinder::functions
void CimulationApp::mouseDown(MouseEvent event) {
	if (event.isLeft())	s.mouseClicked = true;
	if (s.SimRunning == s.PIDCTRL) { v.r.PID.requestedValue = event.getX(); }
}
void CimulationApp::mouseUp(MouseEvent event) {
	if (event.isLeft())	s.mouseClicked = false;
}
void CimulationApp::mouseMove(MouseEvent event) {
	mousePos.X = event.getX();
	mousePos.Y = event.getY();
	mousePos.Z = 0;
	if (v.j.withinAnalogRange(mousePos)) {
		for (int i = 0; i < (maxDots - 1); i++) {//red line /*XPOS*/
			s.gr.RYpos[i] = s.gr.RYpos[i + 1];
		}
		s.gr.RYpos[maxDots - 1] = (v.r.truSpeed(3, (mousePos.X - (v.j.drawX + v.j.drawSize))) / (s.gr.YAxLength*0.003)) + s.gr.midpoint;

		for (int i = 0; i < (maxDots - 1); i++) {//blue line /*YPOS*/
			s.gr.BYpos[i] = s.gr.BYpos[i + 1];
		}
		s.gr.BYpos[maxDots - 1] = (v.r.truSpeed(3, (mousePos.Y - (v.j.drawY + v.j.drawSize))) / (s.gr.YAxLength*0.003)) + s.gr.midpoint;
	}
}
void CimulationApp::keyDown(KeyEvent event) {


}
void CimulationApp::keyUp(KeyEvent event) {

}
void CimulationApp::update() {
	v.j.getAnalog(mousePos);
	v.r.update();//calls robot update function
	switch (s.SimRunning) {
	case simulation::NAVIGATION:
		v.r.NavigationUpdate();
		v.r.motorPower = v.r.truSpeed(3, v.j.analogY) / 127;
		v.r.forwards(v.r.motorPower);
		if (abs(v.j.analogX) > 10) {//checking to see if rotation should occur.
			v.r.rotating = true;
			v.r.rotateBase(v.j.analogX);
		}
		else {
			v.r.rotating = false;
		}
			break;
	case simulation::PIDCTRL:
		v.r.PIDControlUpdate();
		break;
	case simulation::TRUSPEED:
		v.r.TruSpeedUpdate();
		break;
	case simulation::FIELD:
		v.f.FieldUpdate();
		break;
	}

}
//for buttons
void clicky(int AMOUNT_BUTTON) {//function for clicking the buttons
	for (int i = 0; i < AMOUNT_BUTTON; i++) {//for each button in the array 
		if (mousePos.X > 100 * (i + 1) - (50) + (25 * i) && mousePos.X < 100 * (i + 1) + (50) + (25 * i) && mousePos.Y > 25 && mousePos.Y < 75) {//within boundaries for each button based off their index
			s.hovering = i;
			if (s.mouseClicked) {
				s.SimRunning = simulation::SimulationType(i);
			}
		}
	}
}
void buttons() {//function for drawing the buttons
#define BUTTON_AMOUNT 4//number of buttons
	int bX[BUTTON_AMOUNT], bY = 50, dInBtw = 25;//array for #buttons, bY is y position of each btn, dInBtw is distance in bwtween buttons
	for (int i = 0; i < BUTTON_AMOUNT; i++) {
		bX[0] = 0;//initialize first button
		bX[i] = (i + 1) * 100;//increment x position for each button based off index
		if (i == s.SimRunning) { gl::color(0, 1, 0); }//if the button's index is equal to whichever button's index is being hovered over
		else if (i == s.hovering) { gl::color(1, 0, 0); }//if the button's index is equal to whichever button's index is being hovered over
		else { gl::color(1, 1, 1); }
		gl::drawStrokedRect(Area(bX[i] - 50 + dInBtw*i, bY - 25, bX[i] + 50 + dInBtw*i, bY + 25));
		gl::color(1, 1, 1);
		if (i == 0)gl::drawString("PID", Vec2f(bX[i] - 20, bY - 12.5), Color(1, 1, 1), Font("Arial", 25));
		else if (i == 1)gl::drawString("NAV", Vec2f(bX[i] - 18 + dInBtw*i, bY - 10), Color(1, 1, 1), Font("Arial", 25));
		else if (i == 2)gl::drawString("TRUSpeed", Vec2f(bX[i] - 49 + dInBtw*i, bY - 10), Color(1, 1, 1), Font("Arial", 25));
		else if (i == 3)gl::drawString("Auton", Vec2f(bX[i] - 35 + dInBtw*i, bY - 10), Color(1, 1, 1), Font("Arial", 29));
		clicky(BUTTON_AMOUNT);//function for if a button is being hovered of pressed
	}
}

void CimulationApp::draw() {
	gl::enableAlphaBlending();//good for transparent images
	// clear out the window with black
	gl::clear(Color(0, 0, 0));
	buttons();
	glPushMatrix();
	gl::translate(Vec3f(v.r.position.X*ppi, v.r.position.Y*ppi, 0.0));//origin of rotation
	gl::rotate(Vec3f(0, 0, -v.r.mRot));//something for like 3D rotation.... ugh
	gl::draw(v.r.TankBase, Area((-(v.r.size / 2))*ppi, (-(v.r.size / 2))*ppi, ((v.r.size / 2))*ppi, ((v.r.size / 2))*ppi));
	glPopMatrix();
	//joystick analog drawing
	if (s.SimRunning == s.NAVIGATION || s.SimRunning == s.TRUSPEED || s.SimRunning == s.FIELD) {//only for navigation and truspeed sim
		if (s.SimRunning == s.FIELD) {
			v.j.drawX = v.f.fieldSize*ppi + v.f.dFromEdge+20;
			v.j.drawY = getWindowHeight()/2;
		}
		else {
			v.j.drawX = 600;
			v.j.drawY = 500;
		}
		gl::drawStrokedCircle(Vec2f(v.j.drawX+v.j.drawSize, v.j.drawY+v.j.drawSize), v.j.drawSize);//circle at (800px, 300px) with radius 127px
		gl::drawStrokedRect(Area(v.j.drawX, v.j.drawY, v.j.drawX + 2*v.j.drawSize, v.j.drawY + 2*v.j.drawSize));

		if (v.j.withinAnalogRange(mousePos)) {//defined in hoystick.h, basically if within the drawing of the boundaries
			stringstream Xamount;
			string Xamount2;
			Xamount << setprecision(3) << v.r.truSpeed(3, v.j.analogX);
			Xamount >> Xamount2;
			gl::drawString(Xamount2, Vec2f(mousePos.X - 30, mousePos.Y + 50), Color(1, 1, 1), Font("Arial", 30));

			stringstream Yamount;
			string Yamount2;
			Yamount << setprecision(3) << v.r.truSpeed(3, v.j.analogY);
			Yamount >> Yamount2;
			gl::drawString(Yamount2, Vec2f(mousePos.X + 30, mousePos.Y + 50), Color(1, 1, 1), Font("Arial", 30));
		}
	}
	if (s.SimRunning == s.PIDCTRL) {
		stringstream pidthing;
		string pidthing2;
		pidthing << setprecision(3) << v.r.PID_controller();//amount of power the PID is providing
		pidthing >> pidthing2;
		gl::drawString(pidthing2, Vec2f(v.r.position.X*ppi, v.r.position.Y*ppi - 100), Color(1, 1, 1), Font("Arial", 30));
	}
	if (s.SimRunning == s.TRUSPEED) {
		s.gr.graphPlot();//draws the graph
		s.gr.textOutput(&v);//draws the text for the graph
	}
	if (s.SimRunning == s.FIELD) {//when field button is pressed
		gl::pushModelView();//for drawing the field, had to be rotated based off source
			gl::translate(Vec3f(v.f.dFromEdge + v.f.fieldSize*ppi/2, v.f.dFromEdge + v.f.fieldSize*ppi/2, 0.0));//origin of rotation
			gl::rotate(-90);//easy rotation technique
			gl::draw(v.f.fieldBare, Area(-v.f.fieldSize*ppi/2, -v.f.fieldSize*ppi/2, v.f.fieldSize*ppi/2, v.f.fieldSize*ppi/2));
		gl::popModelView();//finish drawing the field
		//drawing each individual cone. oh my
		float fieldEnd = v.f.dFromEdge + v.f.fieldSize*ppi;//the value for the very end of the field drawing, used to properly display cones because in the reference sheet their values were based off the bottom right vertice
		for (int i = 0; i < v.f.c.size(); i++) {
			//fieldend for where the end of the field is, to subtract values because: http://vexcompetition.es/wp-content/uploads/2017/04/IntheZone-Field-specifications.pdf
			//+-(3*ppi) for sayin that the point pos.X and pos.Y are the center, and the 3*ppi is 3 inches RADIUS away from the center point
			gl::draw(v.f.coneTexture, Area((fieldEnd) - (v.f.c[i].pos.X*ppi) - (3*ppi), (fieldEnd) - (v.f.c[i].pos.Y*ppi) - (3 * ppi), (fieldEnd) - (v.f.c[i].pos.X*ppi) + (3 * ppi), (fieldEnd) - (v.f.c[i].pos.Y*ppi) + (3 * ppi)));
		}
	}


	stringstream actualY;
	string actualY2;
	actualY << v.r.current.Xpos;
	actualY >> actualY2;
	gl::drawString(actualY2, Vec2f(600, 40), Color(1, 1, 1), Font("Arial", 30));

	stringstream thing;
	string thing2;
	thing << v.r.current.Ypos;
	thing >> thing2;
	gl::drawString(thing2, Vec2f(650, 40), Color(1, 1, 1), Font("Arial", 30));

	stringstream thingy;
	string thingy2;
	thingy << v.r.position.X;
	thingy >> thingy2;
	gl::drawString(thingy2, Vec2f(600, 140), Color(1, 1, 1), Font("Arial", 30));

	stringstream thingi;
	string thingi2;
	thingi << v.r.position.Y;
	thingi >> thingi2;
	gl::drawString(thingi2, Vec2f(650, 140), Color(1, 1, 1), Font("Arial", 30));
}

CINDER_APP_NATIVE(CimulationApp, RendererGl)
