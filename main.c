/*
 *  Kevin McMahon Project - CU Event Center
 *
 *  Key bindings:
 *  l          Toggles lighting on/off
 *  1/2        Shoot baskets at repective hoop
 *  j          Progress video on jumbotron
 *  k          Change ligting mode (test, game, warm-up)
 *  v          Change display mode (Orthogonal, Perspective, First Person)
 *  +/-        zoom-in/zoom-out
 *  arrows     Change view angle (orbital) or look direction (FP)
 *  w/d/a/s    Move forward/back/left/right (in FP mode)
 *  mouse      Look around
 *  0          Reset view angle
 *  ESC        Exit
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stdlib.h>
#ifdef USEGLEW
#include <GL/glew.h>
#endif
//  OpenGL with prototypes for glext
#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#include <GLUT/glut.h>
// Tell Xcode IDE to not gripe about OpenGL deprecation
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#else
#include <GL/glut.h>
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

/*
 * =======================================================================
 * Global Variables
 * =======================================================================
 */

// --- General State ---
int axes = 1;         // Display axes
double zh = 0;        // Animation angle for plane
double asp = 1;       // Window aspect ratio - updated in reshape fcn

// --- View Mode Control ---
int viewMode = 0;     // 0=Ortho, 1=Perspective, 2=First Person
const char* viewText[] = {"Orthogonal", "Perspective", "First Person"};
// Update for hw5 revision 
int sceneMode = 0; // 0=Basketball Court, 1=Plane only 

// --- Standard Camera State (for modes 0 & 1) ---
int th = 180;           // Azimuth of view angle
int ph = 30;          // Elevation of view angle
double dim = 15.0;    // Zoom level for orbital views (calibrated to fit full court)

// --- FP-Pov Camera State (for mode 2) ---
double eyeX = 0;      // Camera X position
double eyeY = 0.5;    // Camera Y position (locked to walking height)
double eyeZ = 5;      // Camera Z position
float camYaw = -90.0f;  // Camera horizontal rotation (left/right)
float camPitch = 0.0f;  // Camera vertical rotation (up/down)

// --- Unit conversion ---
const double UNITS_PER_FOOT = 0.2;  // Court tiles are 0.2 units per foot

// --- Mouse Input State ---
int mouse_button = -1;  // Which mouse button is pressed
int prev_mouse_x = 0;   // Previous mouse X coordinate
int prev_mouse_y = 0;   // Previous mouse Y coordinate

// --- Lighting State ---
/* Lighting mode:
 * 0 = warm-up / introudctions lighting
 * 1 = arena-style overhead lights on the court
 */
int lightingMode = 0;
int light     = 1;      // Lighting enabled/disabled
int move      = 1;      // Move light
int ambient   = 40;     // Ambient intensity (%)
int diffuse   = 50;     // Diffuse intensity (%)
int specular  = 10;     // Specular intensity (%)
int shininess = 0;      // Shininess (power of two)
float shiny   = 1;      // Shininess (value)
double light_zh = 0;    // Azimuth of the light

// --- Texture state (NEW for hw6) ---
unsigned int texWood = 0;   // Wood texture for basketball court
unsigned int texBasketball = 0; // Basketball texture for basketball
unsigned int texBackboard = 0; // Backboard texture
unsigned int texPole = 0;      // Pole texture
unsigned int texFuselage = 0;   // Fuselage texture for airplane
unsigned int texWings = 0;     // Wings texture for airplane
unsigned int texFire = 0;      // Fire texture for rocket exhaust
unsigned int texCanopy = 0;
unsigned int texCuCourt = 0;
unsigned int texLebronDunk = 0;
unsigned int texKeyboard = 0;
unsigned int texScorersTablePoster = 0;
unsigned int texCuLogo = 0;
unsigned int texSportsFans = 0;
unsigned int texGatoradeLogo = 0;
unsigned int texCoolerLid = 0;
unsigned int texBasketballNet = 0;
unsigned int texScoreBoardLogo = 0;
unsigned int texCenterLogo = 0;
unsigned int texColoradoWordmark = 0;
unsigned int texSidelineLogo = 0;

// basketball materials
const float MAT_BALL_AMB[4] = {0.3f, 0.2f, 0.2f, 1.0f};
const float MAT_BALL_DIF[4] = {1.0f, 0.9f, 0.8f, 1.0f};
const float MAT_BALL_SPEC[4] = {0.3f, 0.3f, 0.3f, 1.0f};

// Scoreboard state
int score[2] = {0,0};

// Net swich animation state - index 0 = hoop at +x, index 1 = hoop at -x
int netAnimating[2] = {0,0};
double netAnimStart[2] = {0.0,0.0};
double netSwayPhase[2] = {0.0,0.0}; // stores offset of net as fcn of t

// Shot animation state - index 0 = hoop at +x, index 1 = hoop at -x
int shotAnimating[2] = {0,0};
double shotStartTime[2] = {0.0,0.0};
double shotParamT[2] = {0.0,0.0};
int shotSwishTriggered[2] = {0,0}; // check to make sure that swish motion occurs once per shot after 0.8 shot progress

// duration of shot in seconds 
const double SHOT_DURATION = 1.2;


// Scorebaord - video board textures 
#define NUM_VIDEO_FRAMES 6
unsigned int texVideoFrames[NUM_VIDEO_FRAMES];
int currentVideoFrame = 0;

//  Cosine and Sine in degrees
#define Cos(x) (cos((x)*3.14159265/180))
#define Sin(x) (sin((x)*3.14159265/180))
void reshape(int width, int height);

// Fcn prototypes for loading textures
unsigned int LoadTexBMP(const char* file);
unsigned int LoadTexBMP32(const char* file);

// Set projection mode - introduced in hw4
void Project()
{
   //  Tell OpenGL we want to manipulate the projection matrix
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity(); // reset
   // view volumn tied to current zoom/state
   double dimW = asp*dim;
   double dimH = dim;
   double dimD = 2.5 * dim; // half-depth... bigger to avoid clipping the plane (issues in hw5)

   if (viewMode == 0) // Orthogonal
   {
      glOrtho(-dimW,+dimW, -dimH,+dimH, -dimD,+dimD);
   }
   else // Perspective (for both orbital and first-person)
   {
      // near a bit larger for precision - scales with scene
      double nearP = 0.2;
      double farP = 4*dim+20.0;
      gluPerspective(60.0,asp, nearP, farP);
   }

   //  Switch to manipulating the model matrix
   glMatrixMode(GL_MODELVIEW);
   //  Undo previous transformations
   glLoadIdentity();
}

/*
 *  Check for OpenGL errors
 */
void ErrCheck(const char* where)
{
   int err = glGetError();
   if (err) fprintf(stderr,"ERROR: %s [%s]\n",gluErrorString(err),where);
}

/*
 * Print message to stderr and exit
 */
void Fatal(const char* format , ...)
{
   va_list args;
   va_start(args,format);
   vfprintf(stderr,format,args);
   va_end(args);
   exit(1);
}

// swish sound
// Got function from AI... looked for simplest method to play sound effect.
void playSwish(void)
{
#ifdef __APPLE__
    system("afplay basketballSwishSound.wav &");
#elif __linux__
    system("aplay basketbalSwishSounds.wav &");
#else
    // Windows (assuming PowerShell + Windows Media Player or similar)
    system("powershell -c (New-Object Media.SoundPlayer 'basketballSwishSound.wav').PlaySync();");
#endif
}

/*
 *  Draw vertex in polar coordinates with normal
 *  (Used for the light source visualization)
 */
void Vertex(double th,double ph)
{
   double x = Sin(th)*Cos(ph);
   double z = Cos(th)*Cos(ph);
   double y =         Sin(ph);
   glNormal3d(x,y,z);
   glVertex3d(x,y,z);
}

// draw ball to represent light position
void ball(double x,double y,double z,double r)
{
   glPushMatrix();
   glTranslated(x,y,z);
   glScaled(r,r,r);
   glColor3f(1,1,1); // White
   //  Bands of latitude
   for (int ph=-90;ph<90;ph+=15)
   {
      glBegin(GL_QUAD_STRIP);
      for (int th=0;th<=360;th+=30)
      {
         Vertex(th,ph);
         Vertex(th,ph+15);
      }
      glEnd();
   }
   glPopMatrix();
}

// Creating "Torus" for rim
// Updated in hw5 to include normals for lighting
void drawTorus(double majorRadius, double minorRadius, int majorSegments, int minorSegments)
{
   // majorRadius is the radius of the whole donut
    // minorRadius is the radius of the tube itself 
   for (int i = 0; i < majorSegments; i++) {
        double theta1 = (double)i / majorSegments * 2.0 * M_PI;
        double theta2 = (double)(i + 1) / majorSegments * 2.0 * M_PI;

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= minorSegments; j++) {
            double phi = (double)j / minorSegments * 2.0 * M_PI;

            double cos_phi = cos(phi);
            double sin_phi = sin(phi);
            
            // Calculate normal for surface
            double nx = cos(theta1) * cos_phi;
            double ny = sin(theta1) * cos_phi;
            double nz = sin_phi;

            // Vertex 1
            double x1 = (majorRadius + minorRadius * cos_phi) * cos(theta1);
            double y1 = (majorRadius + minorRadius * cos_phi) * sin(theta1);
            double z1 = minorRadius * sin_phi;
            glNormal3d(nx, ny, nz); // Set normal for lighting
            glVertex3d(x1, y1, z1);

            // Vertex 2 (with normal for the next major segment)
            nx = cos(theta2) * cos_phi;
            ny = sin(theta2) * cos_phi;
            glNormal3d(nx, ny, nz);
            double x2 = (majorRadius + minorRadius * cos_phi) * cos(theta2);
            double y2 = (majorRadius + minorRadius * cos_phi) * sin(theta2);
            double z2 = minorRadius * sin_phi;
            glVertex3d(x2, y2, z2);
        }
        glEnd();
    }
}

/*
 *  Generic function to draw a Cylinder along the Y-axis though used for the rim of the basketball hoop
 *  Updated in hw5 to include normals for lighting 
 */
void drawCylinder(double radius, double height, int segments)
{
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; i++) {
        double angle = (double)i / segments * 2.0 * M_PI;
        double x = cos(angle); // Normal x is same as vertex x (normalized)
        double z = sin(angle); // Normal z is same as vertex z (normalized)
        
        glNormal3d(x, 0, z);
        glVertex3d(radius * x, 0, radius * z); // Bottom vertex
        glVertex3d(radius * x, height, radius * z); // Top vertex
    }
    glEnd();
}

// Updated with normals for hw5 lighting 
// Textured sphere using spherical UVs (s = th/360, t = (ph+90)/180)
void drawSolidSphereTextured(double radius)
{
   const int d = 15; // same resolution as before

   for (int ph = -90; ph < 90; ph += d)
   {
       glBegin(GL_QUAD_STRIP);
       for (int th = 0; th <= 360; th += d)
       {
           // ring 1
           double nx1 = Sin(th) * Cos(ph);
           double ny1 = Sin(ph);
           double nz1 = Cos(th) * Cos(ph);
           double s1  = (double)th / 360.0;
           double t1  = (double)(ph + 90) / 180.0;
           glNormal3d(nx1, ny1, nz1);
           glTexCoord2d(s1, t1);
           glVertex3d(radius*nx1, radius*ny1, radius*nz1);

           // ring 2
           double nx2 = Sin(th) * Cos(ph + d);
           double ny2 = Sin(ph + d);
           double nz2 = Cos(th) * Cos(ph + d);
           double t2  = (double)(ph + d + 90) / 180.0;
           glNormal3d(nx2, ny2, nz2);
           glTexCoord2d(s1, t2);   // same s, next t
           glVertex3d(radius*nx2, radius*ny2, radius*nz2);
       }
       glEnd();
   }
}

void basketball(double x, double y, double z, double r, double rot)
{
   glPushMatrix();
   glTranslated(x, y, z);
   glRotated(rot, 0, 1, 0);
   glScaled(r, r, r);

   // leather material on ball... not shiny or specular
   glDisable(GL_COLOR_MATERIAL); // set explicit material for the ball
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT , MAT_BALL_AMB);
   glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE , MAT_BALL_DIF);
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, MAT_BALL_SPEC);
   glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 8.0f);  // small highlight

   // leather textured sphere
   glBindTexture(GL_TEXTURE_2D, texBasketball);
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glColor3f(1,1,1);                                 // do not tint texture
   drawSolidSphereTextured(1.0);
   glDisable(GL_TEXTURE_2D);

   // seams matte black 
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL);
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, MAT_BALL_SPEC); // low
   glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);

   // offset to avoid z-fighting
   const double rLine = 1.001;
   const int segs = 128;

   glLineWidth(2.5f);
   glColor3f(0.0f, 0.0f, 0.0f);

   // seam around y-axis
   glBegin(GL_LINE_LOOP);
   for (int i = 0; i < segs; ++i)
   {
      double th = 360.0 * i / segs;
      double s = Sin(th);
      double c = Cos(th);
      glVertex3d(rLine * c, 0.0, rLine * s);
   }
   glEnd();

   // seam around x-axis
   glBegin(GL_LINE_LOOP);
   for (int i = 0; i < segs; ++i)
   {
      double th = 360.0 * i / segs;
      double s = Sin(th);
      double c = Cos(th);
      glVertex3d(0.0, rLine * c, rLine * s);
   }
   glEnd();

   glPopMatrix();
}

// Textured cylinder along +Y with tex that repeat.
// repeatU around circumference, reatV around height
// originally used for floor mounted hoop
void drawTexturedCylinder(unsigned int tex, double radius, double height, int segments, double repeatU, double repeatV)
{
   glBindTexture(GL_TEXTURE_2D, tex);
   glEnable(GL_TEXTURE_2D);
   glBegin(GL_QUAD_STRIP);
   for (int i = 0; i <= segments; ++i)
   {
      double t  = (double)i / segments;
      double ang = t * 2.0 * M_PI;
      double x = cos(ang);
      double z = sin(ang);
      double s = t * repeatU; // wrap around
      glNormal3d(x, 0.0, z);
      // bottom
      glTexCoord2d(s, 0.0);
      glVertex3d(radius * x, 0.0, radius * z);
      // top
      glTexCoord2d(s, repeatV);
      glVertex3d(radius * x, height, radius * z);
   }
   glEnd();
   glDisable(GL_TEXTURE_2D);
}

// Draw a textured cylinder from (x1,y1,z1) to (x2,y2,z2)
// Used for hoop support.
// Uses drawTexturedCylinder which draws along +Y
// Simple progression from boring vertical floor mounted hoop support to ceiling mounted as found in CUEC
void drawTexturedCylinderBetween(unsigned int tex, double radius, double x1,double y1,double z1, double x2,double y2,double z2, int segments, double repeatU, double repeatV)
{
   double vx = x2 - x1;
   double vy = y2 - y1;
   double vz = z2 - z1;

   double len = sqrt(vx*vx + vy*vy + vz*vz);

   // Normalize direction
   double invLen = 1.0 / len;
   vx *= invLen;
   vy *= invLen;
   vz *= invLen;

   // rotate +y to v (diagonal hoop support)
   // axis = (0,1,0) x v = (vz, 0, -vx)
   double ax = vz;
   double ay = 0.0;
   double az = -vx;
   double axisLen = sqrt(ax*ax + ay*ay + az*az);

   glPushMatrix();
   glTranslated(x1, y1, z1);

   if (axisLen > 1e-6)
   {
      ax /= axisLen;
      az /= axisLen;
      double dot = vy;
      if (dot >  1.0) dot = 1.0;
      if (dot < -1.0) dot = -1.0;
      double angDeg = acos(dot) * 180.0 / M_PI;
      glRotated(angDeg, ax, ay, az);
   }
   else if (vy < 0.0) // if not then already almost upward
   {
      glRotated(180.0, 1.0, 0.0, 0.0);
   }
   // otherwise small rotation

   drawTexturedCylinder(tex, radius, len, segments, repeatU, repeatV);

   glPopMatrix();
}

// Drop a cylinder between two points (x1,y1,z1) and (x2,y2,z2)... used for chairs and table
// super similar to above but no texture and when tryign to implement toggle for texture the 
// scene looked very off and just brute forced w two almost identical fcns
void drawRodBetween(double x1,double y1,double z1, double x2,double y2,double z2, double r,int segs)
{
   double vx = x2 - x1;
   double vy = y2 - y1;
   double vz = z2 - z1;
   double len = sqrt(vx*vx + vy*vy + vz*vz);

   // rotate (0,1,0) onto v using axis = (0,1,0) x v = (vz,0,-vx)
   double ax =  vz;
   double ay =  0.0;
   double az = -vx;
   double axisLen = sqrt(ax*ax + az*az);
   double angDeg  = 0.0;

   if (axisLen > 1e-6) // avoid deiving my 0 when already vertical
   {
      angDeg = acos(vy/len) * 180.0 / M_PI;
      ax /= axisLen;
      az /= axisLen;
   }

   glPushMatrix();
   glTranslated(x1,y1,z1);
   if (axisLen > 1e-6)
      glRotated(angDeg, ax, ay, az);

   // circular walls of cylinder
   drawCylinder(r, len, segs);

   // circle end caps
   double angleStep = 2.0 * M_PI / segs;

   // Bottom cap at y = 0 odwn facing normal
   glBegin(GL_TRIANGLE_FAN);
   glNormal3f(0.0f, -1.0f, 0.0f);
   glVertex3f(0.0f, 0.0f, 0.0f); // center vert
   for (int i = 0; i <= segs; ++i)
   {
      double a = i * angleStep;
      double cx = r * cos(a);
      double cz = r * sin(a);
      glVertex3f(cx, 0.0f, cz);
   }
   glEnd();

   // Top cap
   glBegin(GL_TRIANGLE_FAN);
   glNormal3f(0.0f, 1.0f, 0.0f);
   glVertex3f(0.0f, (float)len, 0.0f);
   for (int i = 0; i <= segs; ++i)
   {
      double a = i * angleStep;
      double cx = r * cos(a);
      double cz = r * sin(a);
      glVertex3f(cx, (float)len, cz);
   }
   glEnd();

   glPopMatrix();
}

// Back panel of the chair
// - centered at (0,0) in X/Y
// - front face at z=0, back at z=-t
// - rounded top corners
void drawRoundedBackSolid(double w, double h, double t, double cornerR, int arcSegs)
{
   const double zFront = 0.0;
   const double zBack = -t;

   const double halfW = 0.5 * w;
   const double halfH = 0.5 * h;

   const double xLeft = -halfW;
   const double xRight = halfW;
   const double yBottom = -halfH;
   const double yTop = halfH;
   const double yTopInner = yTop - cornerR;

   // front face of the chair
   glNormal3f(0,0,1);

   // main rectangular portion of back of chiar
   glBegin(GL_QUADS);
   glVertex3d(xLeft, yBottom, zFront);
   glVertex3d(xRight, yBottom, zFront);
   glVertex3d(xRight, yTopInner, zFront);
   glVertex3d(xLeft, yTopInner, zFront);
   glEnd();

   // top flat strip between rounded corners
   glBegin(GL_QUADS);
   glVertex3d(xLeft + cornerR, yTopInner, zFront);
   glVertex3d(xRight - cornerR, yTopInner, zFront);
   glVertex3d(xRight - cornerR, yTop, zFront);
   glVertex3d(xLeft + cornerR, yTop, zFront);
   glEnd();

   // top left rounded corner 
   glBegin(GL_TRIANGLE_FAN);
   glVertex3d(xLeft + cornerR, yTopInner, zFront);  // inside corner
   for (int i = 0; i <= arcSegs; ++i)
   {
      double a  = M_PI - (i * (M_PI/2.0) / arcSegs);  // 180° → 90°
      double vx = xLeft + cornerR + cornerR * cos(a);
      double vy = yTopInner + cornerR + cornerR * sin(a);
      glVertex3d(vx, vy, zFront);
   }
   glEnd();

   // top right rounded corner
   glBegin(GL_TRIANGLE_FAN);
   glVertex3d(xRight - cornerR, yTopInner, zFront);
   for (int i = 0; i <= arcSegs; ++i)
   {
      double a  = (M_PI/2.0) - (i * (M_PI/2.0) / arcSegs);  // 90° → 0°
      double vx = xRight - cornerR + cornerR * cos(a);
      double vy = yTopInner + cornerR + cornerR * sin(a);
      glVertex3d(vx, vy, zFront);
   }
   glEnd();

   // BACK FACE of chair
   glNormal3f(0,0,1);
   glBegin(GL_QUADS);
   glVertex3d(xLeft, yBottom, zBack);
   glVertex3d(xRight, yBottom, zBack);
   glVertex3d(xRight, yTop, zBack);
   glVertex3d(xLeft, yTop, zBack);
   glEnd();

   // side edges between front and back planes of chair
   // left edge
   glNormal3f(-1,0,0);
   glBegin(GL_QUADS);
   glVertex3d(xLeft, yBottom, zFront);
   glVertex3d(xLeft, yBottom, zBack);
   glVertex3d(xLeft, yTop, zBack);
   glVertex3d(xLeft, yTop, zFront);
   glEnd();

   // right edge
   glNormal3f(1,0,0);
   glBegin(GL_QUADS);
   glVertex3d(xRight, yBottom, zFront);
   glVertex3d(xRight, yTop, zFront);
   glVertex3d(xRight, yTop, zBack);
   glVertex3d(xRight, yBottom, zBack);
   glEnd();

   // bottom edge
   glNormal3f(0,-1,0);
   glBegin(GL_QUADS);
   glVertex3d(xLeft , yBottom, zFront);
   glVertex3d(xRight, yBottom, zFront);
   glVertex3d(xRight, yBottom, zBack );
   glVertex3d(xLeft , yBottom, zBack );
   glEnd();

   // flat top strip between rounded corners
   glNormal3f(0,1,0);
   glBegin(GL_QUADS);
   glVertex3d(xLeft  + cornerR, yTop, zFront);
   glVertex3d(xRight - cornerR, yTop, zFront);
   glVertex3d(xRight - cornerR, yTop, zBack);
   glVertex3d(xLeft  + cornerR, yTop, zBack);
   glEnd();

   // rounded top edges (same shape as before)
   const double topY = yTop;
   const double xArcLeft = -halfW + cornerR;
   const double xArcRight = halfW - cornerR;

   // left rounded top edge
   glBegin(GL_QUAD_STRIP);
   for (int i = 0; i <= arcSegs; ++i)
   {
      double a  = M_PI - (i * (M_PI/2.0) / arcSegs);
      double nx = cos(a);
      double ny = sin(a);
      double vx = xArcLeft + cornerR * nx;
      double vy = (topY - cornerR) + cornerR * ny;
      glNormal3f(nx, ny, 0);
      glVertex3d(vx, vy, zFront);
      glVertex3d(vx, vy, zBack);
   }
   glEnd();

   // right rounded top edge
   glBegin(GL_QUAD_STRIP);
   for (int i = 0; i <= arcSegs; ++i)
   {
      double a  = (M_PI/2.0) - (i * (M_PI/2.0) / arcSegs);
      double nx = cos(a);
      double ny = sin(a);
      double vx = xArcRight + cornerR * nx;
      double vy = (topY - cornerR) + cornerR * ny;
      glNormal3f(nx, ny, 0);
      glVertex3d(vx, vy, zFront);
      glVertex3d(vx, vy, zBack);
   }
   glEnd();
}

// Full chair:
// - seat box
// - tilted backrest with handle
// - metal frame/legs built from rods
// dirDeg   = yaw around Y (which way the chair faces)
void chair(double x, double y, double z, double dirDeg, double scale)
{
   glPushMatrix();
   glTranslated(x,y,z);
   glRotated(dirDeg,0,1,0);
   glScaled(scale,scale,scale);

   // Chair proportions
   const double seatW = 1.10;
   const double seatD = 0.70; //front at z=0, back at z=-seatD 
   const double seatH = 0.10;
   const double seatY = 0.60;

   const double backW = 0.98;
   const double backH = 0.55;
   const double backT = 0.04;
   const double backR = 0.14;
   const double backTiltDeg = 16.0; //lean the backrest a bit

   // Hinge line where the backrest “pins” into the frame
   const double hingeRise = 0.18; //how far above the seat plane
   const double backGap = 0.08; // how far behind the seat back edge
   const double Hy = seatY + hingeRise;
   const double Hz = -seatD - backGap;
   const double hingeFromBottom = 0.10; // hinge is a bit above bottom of back panel 

   const double legR = 0.03;
   const int segs = 6;

   int texWasEnabled = glIsEnabled(GL_TEXTURE_2D);
   glColor3f(1.0f,1.0f,1.0f);   // let texture drive the color
   if(!texWasEnabled)
      glEnable(GL_TEXTURE_2D);
 
   glBindTexture(GL_TEXTURE_2D, texScoreBoardLogo);   
   glBegin(GL_QUADS);
   glNormal3f(0,1,0);
   glTexCoord2f(0.0f, 1.0f); glVertex3d(-seatW*0.5, seatY, -seatD); // top left 
   glTexCoord2f(1.0f, 1.0f); glVertex3d( seatW*0.5, seatY, -seatD); // top right
   glTexCoord2f(1.0f, 0.0f); glVertex3d( seatW*0.5, seatY, 0); // bottom right
   glTexCoord2f(0.0f, 0.0f); glVertex3d(-seatW*0.5, seatY, 0); // bottom left
   glEnd();

   // bottom
   glColor3f(0.10f,0.10f,0.13f);
   glBegin(GL_QUADS);
   glNormal3f(0,-1,0);
   glVertex3d(-seatW*0.5, seatY-seatH, -seatD);
   glVertex3d(seatW*0.5, seatY-seatH, -seatD);
   glVertex3d(seatW*0.5, seatY-seatH, 0);
   glVertex3d(-seatW*0.5, seatY-seatH, 0);

   // front edge
   glNormal3f(0,0,1);
   glVertex3d(-seatW*0.5, seatY-seatH, 0);
   glVertex3d(seatW*0.5, seatY-seatH, 0);
   glVertex3d(seatW*0.5, seatY, 0);
   glVertex3d(-seatW*0.5, seatY, 0);

   // back edge
   glNormal3f(0,0,-1);
   glVertex3d(seatW*0.5, seatY-seatH, -seatD);
   glVertex3d(-seatW*0.5, seatY-seatH, -seatD);
   glVertex3d(-seatW*0.5, seatY, -seatD);
   glVertex3d(seatW*0.5, seatY, -seatD);

   // left side
   glNormal3f(-1,0,0);
   glVertex3d(-seatW*0.5, seatY-seatH, -seatD);
   glVertex3d(-seatW*0.5, seatY-seatH, 0);
   glVertex3d(-seatW*0.5, seatY, 0);
   glVertex3d(-seatW*0.5, seatY, -seatD);

   /* right side */
   glNormal3f(1,0,0);
   glVertex3d(seatW*0.5, seatY-seatH, 0);
   glVertex3d(seatW*0.5, seatY-seatH, -seatD);
   glVertex3d(seatW*0.5, seatY, -seatD);
   glVertex3d(seatW*0.5, seatY, 0);

   glEnd();

   // BACKREST
   //
   glPushMatrix();

   // Hinge behind the seat
   glTranslated(0.0, Hy, Hz);

   // Tilt backrest around hinge line
   glRotated(-backTiltDeg, 1,0,0);

   // Shift panel so hinge lands at y=0 in panel space
   glTranslated(0.0, backH*0.5 - hingeFromBottom, 0.0);

   glColor3f(0.10f,0.10f,0.13f);
   drawRoundedBackSolid(backW, backH, backT, backR, 10);
   glPopMatrix();

   // METAL FRAME / LEGS
   glColor3f(0.08f,0.08f,0.09f);

   // Side-to-side spacing that lines up with the backrest width
   const double xSide = backW*0.5 - 0.06;

   // Foot positions and front/under-seat anchor points
   const double yFoot = 0.02;
   const double zFootFront = -0.02;
   const double zFootBack = -seatD + 0.06;
   const double yTopFront = seatY - seatH + 0.01;
   const double zTopFront = -0.02;

   // Back leg tops on the hinge line
   const double Lhx = -xSide, Rhx =  xSide;
   const double Lhy = Hy, Rhy = Hy;
   const double Lhz = Hz, Rhz = Hz;

   // Chair legs
   // Back left leg
   drawRodBetween(Lhx, yFoot, zFootBack, Lhx, Lhy,  Lhz, legR, segs);
   // Front left leg
   drawRodBetween(-xSide, yFoot, zFootFront, -xSide, yTopFront, zTopFront, legR, segs);
   // Back right leg
   drawRodBetween(Rhx, yFoot, zFootBack, Rhx, Rhy,  Rhz, legR, segs);
   // Front rihgt leg
   drawRodBetween( xSide, yFoot, zFootFront, xSide, yTopFront, zTopFront, legR, segs);

   // Bars in front from structural support (front to back)
   drawRodBetween(-xSide, yTopFront, zTopFront, -xSide, yFoot, zFootBack, legR, segs);
   drawRodBetween( xSide, yTopFront, zTopFront, xSide, yFoot, zFootBack, legR, segs);

   // Cross bar in front
   const double yCross = yFoot + 0.55*(yTopFront - yFoot);
   const double zCross = -seatD * 0.45;

   // Middle crossbar
   drawRodBetween(-xSide, yCross, zCross, xSide, yCross, zCross, legR*0.8, segs);

   glPopMatrix();
}

// Simple scorer's table centered on the near sideline.
// Front (court side) is closed, back (bench side) is open so can see under it
void drawScorersTable(double courtLenHalfX, double courtWidHalfZ, double benchY)
{
   const double distFromEndline = 35.0; // clear space from each endline to table
   const double tableHeightFeet = 2.0;
   const double tableDepthFeet = 2.0; 
   const double offsetFromSidelineFeet = 2.5;

   const double clearX = distFromEndline * UNITS_PER_FOOT;
   const double tableHalfLen = courtLenHalfX - clearX;

   const double tableHeight = tableHeightFeet * UNITS_PER_FOOT;
   const double tableDepth = tableDepthFeet * UNITS_PER_FOOT;
   const double offsetSideline = offsetFromSidelineFeet * UNITS_PER_FOOT;

   // X span of the table (centered at midcourt)
   const double xL = -tableHalfLen;
   const double xR = tableHalfLen;

   // Y: base + top of the wall frame
   const double y0 = benchY;
   const double y1 = benchY + tableHeight;

   // Z: near sideline is at -courtWidHalfZ. Table pushes toward the benches.
   const double zFrontOuter = -courtWidHalfZ - offsetSideline; // court-facing front
   const double zBackOuter = zFrontOuter - tableDepth; // back edge toward benches

   // thin front plane to align good enough with the sides 
   const double wallThickness = 0.03;    
   const double zFrontInner = zFrontOuter - wallThickness;
   const double zBackInner = zBackOuter  + wallThickness;
   const double xInnerL = xL + wallThickness;
   const double xInnerR = xR - wallThickness;

   // Wooden slab that sits inside the wall frame
   const double topDrop = 0.18;
   const double topThickness = 0.08;
   const double yWoodTop = y1 - topDrop;
   const double yWoodBottom = yWoodTop - topThickness;

   // Desktop slab, textured w wood for now 
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texWood);
   glColor3f(1.0f, 1.0f, 1.0f);

   glBegin(GL_QUADS);
   // Top face
   glNormal3f(0,1,0);
   glTexCoord2f(0.0f, 0.0f); glVertex3d(xInnerL, yWoodTop, zBackInner);
   glTexCoord2f(1.0f, 0.0f); glVertex3d(xInnerR, yWoodTop, zBackInner);
   glTexCoord2f(1.0f, 1.0f); glVertex3d(xInnerR, yWoodTop, zFrontInner);
   glTexCoord2f(0.0f, 1.0f); glVertex3d(xInnerL, yWoodTop, zFrontInner);

   // Bottom face
   glNormal3f(0,-1,0);
   glTexCoord2f(0.0f, 0.0f); glVertex3d(xInnerL, yWoodBottom, zFrontInner);
   glTexCoord2f(1.0f, 0.0f); glVertex3d(xInnerR, yWoodBottom, zFrontInner);
   glTexCoord2f(1.0f, 1.0f); glVertex3d(xInnerR, yWoodBottom, zBackInner);
   glTexCoord2f(0.0f, 1.0f); glVertex3d(xInnerL, yWoodBottom, zBackInner);

   // Front edge
   glNormal3f(0,0,1);
   glTexCoord2f(0.0f, 0.0f); glVertex3d(xInnerL, yWoodBottom, zFrontInner);
   glTexCoord2f(1.0f, 0.0f); glVertex3d(xInnerR, yWoodBottom, zFrontInner);
   glTexCoord2f(1.0f, 1.0f); glVertex3d(xInnerR, yWoodTop, zFrontInner);
   glTexCoord2f(0.0f, 1.0f); glVertex3d(xInnerL, yWoodTop, zFrontInner);

   // Back edge
   glNormal3f(0,0,-1);
   glTexCoord2f(0.0f, 0.0f); glVertex3d(xInnerR, yWoodBottom, zBackInner);
   glTexCoord2f(1.0f, 0.0f); glVertex3d(xInnerL, yWoodBottom, zBackInner);
   glTexCoord2f(1.0f, 1.0f); glVertex3d(xInnerL, yWoodTop, zBackInner);
   glTexCoord2f(0.0f, 1.0f); glVertex3d(xInnerR, yWoodTop, zBackInner);

   // Left edge
   glNormal3f(-1,0,0);
   glTexCoord2f(0.0f, 0.0f); glVertex3d(xInnerL, yWoodBottom, zBackInner);
   glTexCoord2f(1.0f, 0.0f); glVertex3d(xInnerL, yWoodBottom, zFrontInner);
   glTexCoord2f(1.0f, 1.0f); glVertex3d(xInnerL, yWoodTop, zFrontInner);
   glTexCoord2f(0.0f, 1.0f); glVertex3d(xInnerL, yWoodTop, zBackInner);

   // Right edge
   glNormal3f(1,0,0);
   glTexCoord2f(0.0f, 0.0f); glVertex3d(xInnerR, yWoodBottom, zFrontInner);
   glTexCoord2f(1.0f, 0.0f); glVertex3d(xInnerR, yWoodBottom, zBackInner);
   glTexCoord2f(1.0f, 1.0f); glVertex3d(xInnerR, yWoodTop, zBackInner);
   glTexCoord2f(0.0f, 1.0f); glVertex3d(xInnerR, yWoodTop, zFrontInner);
   glEnd();

   // Front wall w multiple prisms to not stretch ads
   {
      double segWidth = (xR - xL) / 4.0;

      for (int i = 0; i < 4; ++i)
      {
         double x0 = xL + i * segWidth;
         double x1 = x0 + segWidth;

         // Textured front face poster 
         glEnable(GL_TEXTURE_2D);
         glBindTexture(GL_TEXTURE_2D, texScorersTablePoster);
         glColor3f(1.0f,1.0f,1.0f);

         glBegin(GL_QUADS);
         glNormal3f(0,0,1);
         glTexCoord2f(0.0f, 0.0f); glVertex3d(x0, y0, zFrontOuter);
         glTexCoord2f(1.0f, 0.0f); glVertex3d(x1, y0, zFrontOuter);
         glTexCoord2f(1.0f, 1.0f); glVertex3d(x1, y1, zFrontOuter);
         glTexCoord2f(0.0f, 1.0f); glVertex3d(x0, y1, zFrontOuter);
         glEnd();

         // Thin block behind the poster 
         glDisable(GL_TEXTURE_2D);
         glColor3f(0.40f,0.40f,0.42f);

         glBegin(GL_QUADS);
         // Inner face at zFrontInner (so the block has thickness)
         glNormal3f(0,0,-1);
         glVertex3d(x1, y0, zFrontInner);
         glVertex3d(x0, y0, zFrontInner);
         glVertex3d(x0, y1, zFrontInner);
         glVertex3d(x1, y1, zFrontInner);

         // Left side of ad block
         glNormal3f(-1,0,0);
         glVertex3d(x0, y0, zFrontInner);
         glVertex3d(x0, y0, zFrontOuter);
         glVertex3d(x0, y1, zFrontOuter);
         glVertex3d(x0, y1, zFrontInner);

         // Right side
         glNormal3f(1,0,0);
         glVertex3d(x1, y0, zFrontOuter);
         glVertex3d(x1, y0, zFrontInner);
         glVertex3d(x1, y1, zFrontInner);
         glVertex3d(x1, y1, zFrontOuter);

         // Top edge of this ad block
         glNormal3f(0,1,0);
         glVertex3d(x0, y1, zFrontInner);
         glVertex3d(x1, y1, zFrontInner);
         glVertex3d(x1, y1, zFrontOuter);
         glVertex3d(x0, y1, zFrontOuter);
         glEnd();
      }
   }

   // Outer walls on Left and Right sides
   // LEFT side outer face: CU logo
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texCuLogo);
   glColor3f(1.0f,1.0f,1.0f);

   glBegin(GL_QUADS);
   glNormal3f(-1,0,0);
   glTexCoord2f(0.0f,0.0f); glVertex3d(xL, y0, zBackOuter);
   glTexCoord2f(1.0f,0.0f); glVertex3d(xL, y0, zFrontOuter);
   glTexCoord2f(1.0f,1.0f); glVertex3d(xL, y1, zFrontOuter);
   glTexCoord2f(0.0f,1.0f); glVertex3d(xL, y1, zBackOuter);
   glEnd();

   glDisable(GL_TEXTURE_2D);
   glColor3f(0.40f,0.40f,0.42f);

   glBegin(GL_QUADS);
   // Left inner face
   glNormal3f(1,0,0);
   glVertex3d(xInnerL, y0, zFrontInner);
   glVertex3d(xInnerL, y0, zBackInner);
   glVertex3d(xInnerL, y1, zBackInner);
   glVertex3d(xInnerL, y1, zFrontInner);

   // Front edge tying into ad blocks
   glNormal3f(0,0,1);
   glVertex3d(xInnerL, y0, zFrontInner);
   glVertex3d(xL, y0, zFrontOuter);
   glVertex3d(xL, y1, zFrontOuter);
   glVertex3d(xInnerL, y1, zFrontInner);

   // Back edge
   glNormal3f(0,0,-1);
   glVertex3d(xL, y0, zBackOuter);
   glVertex3d(xInnerL, y0, zBackInner);
   glVertex3d(xInnerL, y1, zBackInner);
   glVertex3d(xL, y1, zBackOuter);

   // Top
   glNormal3f(0,1,0);
   glVertex3d(xL, y1, zBackOuter);
   glVertex3d(xL, y1, zFrontOuter);
   glVertex3d(xInnerL, y1, zFrontInner);
   glVertex3d(xInnerL, y1, zBackInner);
   glEnd();

   // RIGHT side outer face: CU logo
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texCuLogo);
   glColor3f(1.0f,1.0f,1.0f);

   glBegin(GL_QUADS);
   glNormal3f(1,0,0);
   glTexCoord2f(0.0f,0.0f); glVertex3d(xR, y0, zFrontOuter);
   glTexCoord2f(1.0f,0.0f); glVertex3d(xR, y0, zBackOuter);
   glTexCoord2f(1.0f,1.0f); glVertex3d(xR, y1, zBackOuter);
   glTexCoord2f(0.0f,1.0f); glVertex3d(xR, y1, zFrontOuter);
   glEnd();

   glDisable(GL_TEXTURE_2D);
   glColor3f(0.40f,0.40f,0.42f);

   glBegin(GL_QUADS);
   // Right inner face
   glNormal3f(-1,0,0);
   glVertex3d(xInnerR, y0, zBackInner);
   glVertex3d(xInnerR, y0, zFrontInner);
   glVertex3d(xInnerR, y1, zFrontInner);
   glVertex3d(xInnerR, y1, zBackInner);

   // Front edge
   glNormal3f(0,0,1);
   glVertex3d(xInnerR, y0, zFrontInner);
   glVertex3d(xR, y0, zFrontOuter);
   glVertex3d(xR, y1, zFrontOuter);
   glVertex3d(xInnerR, y1, zFrontInner);

   // Back edge
   glNormal3f(0,0,-1);
   glVertex3d(xR, y0, zBackOuter);
   glVertex3d(xInnerR, y0, zBackInner);
   glVertex3d(xInnerR, y1, zBackInner);
   glVertex3d(xR, y1, zBackOuter);

   // Top
   glNormal3f(0,1,0);
   glVertex3d(xInnerR, y1, zBackInner);
   glVertex3d(xInnerR, y1, zFrontInner);
   glVertex3d(xR, y1, zFrontOuter);
   glVertex3d(xR, y1, zBackOuter);
   glEnd();

   glEnable(GL_TEXTURE_2D); // re-enable for rest of the scene
}

void drawLaptop(double x, double y, double z, double yawDeg, double scale)
{
   glPushMatrix();
   glTranslated(x,y,z);
   glRotated(yawDeg,0,1,0);
   glScaled(scale,scale,scale);

   // Local dimensions (tweak until it looks right)
   const double baseW = 1.4; // left to  right
   const double baseD = 1.0; // back (toward scorer) to front (toward court)
   const double baseH = 0.06; // base thickness

   const double screenW = 1.3;
   const double screenH = 0.9;
   const double screenT = 0.05; // screen thickness

   // Back edge (hinge line) is the more "negative Z" side of the base
   const double zBack = -baseD;
   const double zFront = 0.0;
   const double hingeZ = zBack + 0.04; // small inset from absolute back

   const double xL = -baseW*0.5;
   const double xR = baseW*0.5;

   // Base sits ON the surface passed in as y (so base bottom is y, not below it)
   const double y0 = 0.0;
   const double y1 = baseH;

   glDisable(GL_TEXTURE_2D);  // we'll selectively turn it on when needed

   // Bottom prism for laptop base
   glColor3f(0.10f,0.10f,0.10f);
   glBegin(GL_QUADS);
   // Bottom
   glNormal3f(0,-1,0);
   glVertex3d(xL,y0,zFront);
   glVertex3d(xR,y0,zFront);
   glVertex3d(xR,y0,zBack);
   glVertex3d(xL,y0,zBack);

   // Front edge
   glNormal3f(0,0,1);
   glVertex3d(xL,y0,zFront);
   glVertex3d(xR,y0,zFront);
   glVertex3d(xR,y1,zFront);
   glVertex3d(xL,y1,zFront);

   // Back edge
   glNormal3f(0,0,-1);
   glVertex3d(xR,y0,zBack);
   glVertex3d(xL,y0,zBack);
   glVertex3d(xL,y1,zBack);
   glVertex3d(xR,y1,zBack);

   // Left edge
   glNormal3f(-1,0,0);
   glVertex3d(xL,y0,zBack);
   glVertex3d(xL,y0,zFront);
   glVertex3d(xL,y1,zFront);
   glVertex3d(xL,y1,zBack);

   // Right edge
   glNormal3f(1,0,0);
   glVertex3d(xR,y0,zFront);
   glVertex3d(xR,y0,zBack);
   glVertex3d(xR,y1,zBack);
   glVertex3d(xR,y1,zFront);
   glEnd();


   // Keyboard
   glBegin(GL_QUADS);
   glNormal3f(0,1,0);
   glTexCoord2f(0.0f,0.0f); glVertex3d(xL,y1,zBack);
   glTexCoord2f(1.0f,0.0f); glVertex3d(xR,y1,zBack);
   glTexCoord2f(1.0f,1.0f); glVertex3d(xR,y1,zFront);
   glTexCoord2f(0.0f,1.0f); glVertex3d(xL,y1,zFront);
   glEnd();

   // Have a frame for the screen 
   const double screenBottomY = y1; // sits on top of the base
   const double screenTopY = screenBottomY + screenH;
   const double screenBackZ = hingeZ; // toward scorers
   const double screenFrontZ = screenBackZ + screenT; // toward court

   const double sxL = -screenW*0.5;
   const double sxR =  screenW*0.5;

   // Frame around the screen
   glColor3f(0.10f,0.10f,0.10f);
   glBegin(GL_QUADS);
   // Back of screen 
   glNormal3f(0,0,-1);
   glVertex3d(sxR,screenBottomY,screenBackZ);
   glVertex3d(sxL,screenBottomY,screenBackZ);
   glVertex3d(sxL,screenTopY,   screenBackZ);
   glVertex3d(sxR,screenTopY,   screenBackZ);

   // Front of screen border
   glNormal3f(0,0,1);
   glVertex3d(sxL,screenBottomY,screenFrontZ);
   glVertex3d(sxR,screenBottomY,screenFrontZ);
   glVertex3d(sxR,screenTopY,   screenFrontZ);
   glVertex3d(sxL,screenTopY,   screenFrontZ);

   // Left edge
   glNormal3f(-1,0,0);
   glVertex3d(sxL,screenBottomY,screenBackZ);
   glVertex3d(sxL,screenBottomY,screenFrontZ);
   glVertex3d(sxL,screenTopY,   screenFrontZ);
   glVertex3d(sxL,screenTopY,   screenBackZ);

   // Right edge
   glNormal3f(1,0,0);
   glVertex3d(sxR,screenBottomY,screenFrontZ);
   glVertex3d(sxR,screenBottomY,screenBackZ);
   glVertex3d(sxR,screenTopY,   screenBackZ);
   glVertex3d(sxR,screenTopY,   screenFrontZ);

   // Top edge
   glNormal3f(0,1,0);
   glVertex3d(sxL,screenTopY,screenBackZ);
   glVertex3d(sxR,screenTopY,screenBackZ);
   glVertex3d(sxR,screenTopY,screenFrontZ);
   glVertex3d(sxL,screenTopY,screenFrontZ);
   glEnd();

   const double marginX = 0.05;
   const double marginY = 0.05;
   double vxL = sxL + marginX;
   double vxR = sxR - marginX;
   double vyB = screenBottomY + marginY;
   double vyT = screenTopY - marginY;
   double vZ  = screenFrontZ + 0.001; // avoid z-fighting
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texVideoFrames[currentVideoFrame]);
   glColor3f(1.0f,1.0f,1.0f);
   glBegin(GL_QUADS);
   glNormal3f(0,0,1);
   glTexCoord2f(0.0f,0.0f); glVertex3d(vxL, vyB, vZ);
   glTexCoord2f(1.0f,0.0f); glVertex3d(vxR, vyB, vZ);
   glTexCoord2f(1.0f,1.0f); glVertex3d(vxR, vyT, vZ);
   glTexCoord2f(0.0f,1.0f); glVertex3d(vxL, vyT, vZ);
   glEnd();
   glDisable(GL_TEXTURE_2D);

   // Rod hinge along back to connect screen to base
   double hxL = sxL;
   double hxR = sxR;
   double hy  = screenBottomY - 0.02;
   double hz  = hingeZ + screenT*0.5;
   drawRodBetween(hxL,hy,hz, hxR,hy,hz, 0.03,6);
   
   glEnable(GL_TEXTURE_2D);
   glPopMatrix();
}

//DRAWING COOLER FOR SIDELINES
// Draw only the SIDE of a truncated cone since sitting on table with lid
// Oriented along +y
void drawTruncatedConeSide(double baseRadius, double topRadius, double height, int numSlices)
{
   if (numSlices < 3) numSlices = 3;

   double angleStepDeg = 360.0 / numSlices;

   glBegin(GL_QUAD_STRIP);
   for (double angleDeg = 0.0; angleDeg <= 360.0 + 0.1; angleDeg += angleStepDeg)
   {
      double cosAngle = Cos(angleDeg);
      double sinAngle = Sin(angleDeg);

      // Normal for the sloped side
      double normalX = cosAngle * height;
      double normalY = (baseRadius - topRadius);
      double normalZ = sinAngle * height;

      double normalLen = sqrt(normalX*normalX + normalY*normalY + normalZ*normalZ);

      // normalize
      normalX /= normalLen;
      normalY /= normalLen;
      normalZ /= normalLen;

      glNormal3d(normalX, normalY, normalZ);
      // Bottom ring
      glVertex3d(baseRadius * cosAngle, 0.0, baseRadius * sinAngle);
      // Top ring
      glVertex3d(topRadius * cosAngle, height, topRadius * sinAngle);
   }
   glEnd();
}


// Draw a gatorade coooler for benches 
void drawGatoradeCooler(double x, double y, double z, double scale)
{
   double outerRadius = 0.2;
   double bodyHeight = 0.4;

   int    numSlices = 60;
   double lidHeight = 0.05 * bodyHeight;
   double innerRadius = 0.8  * outerRadius;

   innerRadius = outerRadius * 0.7;

   double halfBodyHeight = bodyHeight * 0.5;

   // Lid a bit wider than body so its like the real thing
   double lidRadius = outerRadius * 1.05;

   double angleStepDeg = 360.0 / numSlices;

   // Qucik fix for issues with table
   int wasTexEnabled = glIsEnabled(GL_TEXTURE_2D);
   if (wasTexEnabled)
      glDisable(GL_TEXTURE_2D);

   glPushMatrix();
   glTranslated(x, y, z);
   glScaled(scale, scale, scale);

   // Bright orange as in real life
   glColor3f(1.0f, 0.45f, 0.05f); 

   // Bottom cone
   drawTruncatedConeSide(outerRadius, innerRadius, halfBodyHeight, numSlices);
   // Top cone
   glTranslated(0.0, halfBodyHeight, 0.0); // move up half way
   drawTruncatedConeSide(innerRadius, outerRadius, halfBodyHeight, numSlices);

   // Drawing lid in white, short cylinder above 
   // Move to y to top of cooler
   glTranslated(0.0, halfBodyHeight, 0.0);

   glColor3f(1.0f, 1.0f, 1.0f); 

   // Side of lid... drawing cylinder
   glBegin(GL_QUAD_STRIP);
   for (double angleDeg = 0.0; angleDeg <= 360.0 + 0.1; angleDeg += angleStepDeg)
   {
      double cosAngle = Cos(angleDeg);
      double sinAngle = Sin(angleDeg);

      // Cylinder side normal: straight out from center
      glNormal3d(cosAngle, 0.0, sinAngle);
      // Bottom ring of lid
      glVertex3d(lidRadius * cosAngle, 0.0, lidRadius * sinAngle);
      // Top ring of lid
      glVertex3d(lidRadius * cosAngle, lidHeight, lidRadius * sinAngle);
   }
   glEnd();

   // Top cap of lid - no need to draw bottom since use will never see or look for it
   // Turn textures on just for a logo decal on the lid top
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texCoolerLid);
   glColor3f(1.0f, 1.0f, 1.0f); // let the texture drive the color

   glBegin(GL_TRIANGLE_FAN);
   glNormal3d(0.0, 1.0, 0.0); // facing up
   glTexCoord2f(0.5f, 0.5f); // texture center
   glVertex3d(0.0, lidHeight, 0.0);

   for (double angleDeg = 0.0; angleDeg <= 360.0 + 0.1; angleDeg += angleStepDeg)
   {
      double cosAngle = Cos(angleDeg);
      double sinAngle = Sin(angleDeg);

      // Map circle into 1x1 so logo sits correct direction
      float u = 0.5f - 0.5f * (float)cosAngle;
      float v = 0.5f + 0.5f * (float)sinAngle;

      glTexCoord2f(u, v);
      glVertex3d(lidRadius * cosAngle, lidHeight, lidRadius * sinAngle);
   }
   glEnd();

   glPopMatrix();

   // If caller had textures disabled, put them back the way we found them
   if(!wasTexEnabled)
      glDisable(GL_TEXTURE_2D);
}

void drawCoolerTable(double centerX, double floorY, double centerZ)
{
   glPushMatrix();
   glTranslated(centerX, floorY, centerZ);
   
   // Coolers sit with base at y = benchY + 0.25
   const double topTopY = 0.25;
   const double topThickness = 0.05;  // thickness of table slab for gatorade texture 
   const double topBottomY = topTopY - topThickness;
   const double legHeight = topBottomY; 
   
   // Footprint of the table 
   const double halfLengthX = 0.25; // along X (length of court)
   const double halfDepthZ   = 0.15;   // along Z (w.r.t. width of court)
   
   // Leg parameters
   const double legRadius = 0.015;
   const int legSlices = 12;
   double xL = -halfLengthX;
   double xR =  halfLengthX;
   double zF = -halfDepthZ;
   double zB =  halfDepthZ;
   
   // TABLE TOP - rectangular prism
   glDisable(GL_TEXTURE_2D); 
   glColor3f(1.0f, 1.0f, 1.0f); // neutral gray
   glBegin(GL_QUADS);
   // Top
   glNormal3f(0,1,0);
   glVertex3d(xL, topTopY, zF);
   glVertex3d(xR, topTopY, zF);
   glVertex3d(xR, topTopY, zB);
   glVertex3d(xL, topTopY, zB);
   // Bottom
   glNormal3f(0,-1,0);
   glVertex3d(xL, topBottomY, zB);
   glVertex3d(xR, topBottomY, zB);
   glVertex3d(xR, topBottomY, zF);
   glVertex3d(xL, topBottomY, zF);
   glEnd();
   
   // Texure on the out facing edges of the table top
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texGatoradeLogo);
   glColor3f(1.0f, 1.0f, 1.0f);
   glBegin(GL_QUADS);
   
   // Front face
   glNormal3f(0,0,1);
   glTexCoord2f(1.0f, 0.0f); glVertex3d(xL, topBottomY, zF);
   glTexCoord2f(0.0f, 0.0f); glVertex3d(xR, topBottomY, zF);
   glTexCoord2f(0.0f, 1.0f); glVertex3d(xR, topTopY, zF);
   glTexCoord2f(1.0f, 1.0f); glVertex3d(xL, topTopY, zF);
   // Back face
   glNormal3f(0,0,-1);
   glTexCoord2f(1.0f, 0.0f); glVertex3d(xR, topBottomY, zB);
   glTexCoord2f(0.0f, 0.0f); glVertex3d(xL, topBottomY, zB);
   glTexCoord2f(0.0f, 1.0f); glVertex3d(xL, topTopY,    zB);
   glTexCoord2f(1.0f, 1.0f); glVertex3d(xR, topTopY,    zB);
   // Left face
   glNormal3f(-1,0,0);
   glTexCoord2f(1.0f, 0.0f); glVertex3d(xL, topBottomY, zB);
   glTexCoord2f(0.0f, 0.0f); glVertex3d(xL, topBottomY, zF);
   glTexCoord2f(0.0f, 1.0f); glVertex3d(xL, topTopY,    zF);
   glTexCoord2f(1.0f, 1.0f); glVertex3d(xL, topTopY,    zB);
   // Right face
   glNormal3f(1,0,0);
   glTexCoord2f(1.0f, 0.0f); glVertex3d(xR, topBottomY, zF);
   glTexCoord2f(0.0f, 0.0f); glVertex3d(xR, topBottomY, zB);
   glTexCoord2f(0.0f, 1.0f); glVertex3d(xR, topTopY,    zB);
   glTexCoord2f(1.0f, 1.0f); glVertex3d(xR, topTopY,    zF);
   glEnd();
   
   // LEGS - cylinder rods (keep untextured)
   // Using truncated cone w same radius top & bottom to draw cylinder since I just impl the coolers and can impl this quicker
   glDisable(GL_TEXTURE_2D);
   glColor3f(1.0, 1.0, 1.0f); 
   
   // Corner offsets so legs aren’t exactly on the edge
   const double legOffsetX = 0.08;
   const double legOffsetZ = 0.08;
   
   // Front-right leg
   glPushMatrix();
   glTranslated(xR - legOffsetX, 0.0, zF + legOffsetZ);
   drawTruncatedConeSide(legRadius, legRadius, legHeight, legSlices); // cylinder
   glPopMatrix();
   
   // Front-left leg
   glPushMatrix();
   glTranslated(xL + legOffsetX, 0.0, zF + legOffsetZ);
   drawTruncatedConeSide(legRadius, legRadius, legHeight, legSlices);
   glPopMatrix();
   
   // Back-right leg
   glPushMatrix();
   glTranslated(xR - legOffsetX, 0.0, zB - legOffsetZ);
   drawTruncatedConeSide(legRadius, legRadius, legHeight, legSlices);
   glPopMatrix();
   
   // Back-left leg
   glPushMatrix();
   glTranslated(xL + legOffsetX, 0.0, zB - legOffsetZ);
   drawTruncatedConeSide(legRadius, legRadius, legHeight, legSlices);
   glPopMatrix();
   glEnable(GL_TEXTURE_2D);
   glPopMatrix();
}

void drawBallRack(double centerX, double floorY, double centerZ)
{
   glPushMatrix();
   glTranslated(centerX, floorY, centerZ);
   
   // height of top rack 
   const double topY = 0.6;  

   // length / width... tuned to have balls "rest" on it
   const double halfLengthX = 0.44;  
   const double halfDepthZ = 0.1; 

   // rod params
   const double legRadius = 0.02;
   const int legSlices = 10;

   // Base corner positions (same idea as cooler table)
   double xL = -halfLengthX;
   double xR =  halfLengthX;
   double zF = -halfDepthZ;
   double zB =  halfDepthZ;

   // Actual leg positions (feet)
   double legFLx = xL;
   double legFLz = zF;

   double legFRx = xR;
   double legFRz = zF;

   double legBLx = xL;
   double legBLz = zB;

   double legBRx = xR;
   double legBRz = zB;

   glDisable(GL_TEXTURE_2D);
   glColor3f(1.0f, 1.0f, 1.0f); // light metal-style color

   // vertical legs
   // front left leg
   drawRodBetween(xL, 0.0, zF, xL, topY, zF, legRadius, legSlices);

   // front right leg
   drawRodBetween(xR, 0.0, zF, xR, topY, zF,legRadius, legSlices);

   // back left leg
   drawRodBetween(xL, 0.0, zB, xL, topY, zB, legRadius, legSlices);

   // back right leg
   drawRodBetween(xR, 0.0, zB, xR, topY, zB, legRadius, legSlices);

   //top of frame
   // front
   drawRodBetween(legFLx-0.03, topY, legFLz, legFRx+0.03, topY, legFRz, legRadius, legSlices);
   // right
   drawRodBetween(legFRx, topY, legFRz, legBRx, topY, legBRz, legRadius, legSlices);
   // back
   drawRodBetween(legBRx+0.03, topY, legBRz, legBLx-0.03, topY, legBLz, legRadius, legSlices);
   // left
   drawRodBetween(legBLx, topY, legBLz, legFLx, topY, legFLz, legRadius, legSlices);

   // Middle bars for lower shelf of balls
   double midY = 0.35;
   drawRodBetween(legFLx, midY, legFLz, legFRx, midY, legFRz, legRadius, legSlices);
   drawRodBetween(legFRx, midY, legFRz, legBRx, midY, legBRz, legRadius, legSlices);
   drawRodBetween(legBLx, midY, legBLz, legBRx, midY, legBRz, legRadius, legSlices);
   drawRodBetween(legBLx, midY, legBLz, legFLx, midY, legFLz, legRadius, legSlices);

   double ballR = 0.11;
   double usableWidth = (xR - xL);

   // center of the balls on both shelves
   double bottomY = midY + ballR; // resting on mid cross-bar
   double topYBall = topY + ballR; // resting on top frame
   double zCenter = 0.0; // single row down the middle in Z

   for (int i = 0; i < 4; ++i)
   {
      double t = (i + 0.5) / 4.0; // 4 even spaces
      double x = xL + t * usableWidth;

      // bottom row
      basketball(x, bottomY,  zCenter, ballR, 15.0 * i); // diff rotations for 5th param
      // top row
      basketball(x, topYBall, zCenter, ballR, 37.0 * i); 
   }

   glEnable(GL_TEXTURE_2D);
   glPopMatrix();
}

// Draw the scorer's table with the chairs behind for reporters
// Chairs: one centered, then every 4 feet out along the table span
void drawScorersTableWithChairs(double courtLenHalfX, double courtWidHalfZ, double benchY, double chairScale)
{
   // Drawing table
   drawScorersTable(courtLenHalfX, courtWidHalfZ, benchY);

   // Reuse the same layout numbers the table function uses
   const double distFromEndline = 35.0; // clear from endlines
   const double tableHeightFeet = 2.0;
   const double tableDepthFeet = 2.0;
   const double offsetFromSidelineFeet = 2.5;
   const double offCourtFeetBenches = 5.0;   // where team benches live
   const double topDrop = 0.25;  // same as in drawScorersTable

   const double clearX = distFromEndline * UNITS_PER_FOOT;
   const double tableHalfLen = courtLenHalfX - clearX;
   const double tableHeight = tableHeightFeet * UNITS_PER_FOOT;

   // X span of the table (centered on midcourt)
   const double xL = -tableHalfLen;
   const double xR =  tableHalfLen;

   // Z positions
   const double zFrontTable = -courtWidHalfZ - offsetFromSidelineFeet * UNITS_PER_FOOT;
   const double zBackTable  = zFrontTable   - tableDepthFeet        * UNITS_PER_FOOT;
   const double zBenches    = -courtWidHalfZ - offCourtFeetBenches  * UNITS_PER_FOOT;

   // Aligned 
   const double zChairs = 0.5 * (zBackTable + zBenches);
   const double yWoodTop = benchY + tableHeight - topDrop + 0.05;

   // Laptop position in terms of how close to the back
   const double zLaptop = zBackTable - 0.0075;

   // Chair spacing one then every 4 ft along table in both directions
   const double chairStep = 4.0 * UNITS_PER_FOOT;
   int maxIndex = (int)floor(tableHalfLen / chairStep);

   const double laptopScale = 0.15; // scale for laptops on global scael

   for (int i = -maxIndex; i <= maxIndex; ++i)
   {
      double x = i * chairStep;
      if (x < xL || x > xR) continue;

      // Scorer chair same bench height facing the court
      chair(x, benchY, zChairs, 0.0, chairScale);
      // Laptop on the table directly in front of that chair
      drawLaptop(x, yWoodTop+0.03, zLaptop, 180.0, laptopScale);
   }
}



// DRAWS W.R.T to the court 
// Draw one straight row of courtside chairs, anchored at a baseline and marching toward mid-court along +/−X
// baselineSign +1 = right baseline +X for away bench,  -1 = left baseline -X for away bench 
// sidelineSign -1 = near sideline -Z for fan chairs,  +1 = far sideline +Z for team benches
// yawDeg - chair facing; pass 0 for near sideline facing +Z, 180 for far
void drawBenchRow(int count, double halfCourtX, double halfCourtZ, int baselineSign, int sidelineSign,double y, double offsetFeet, double spacingFeet, double chairScale, double yawDeg)
{
    const double z = sidelineSign * (halfCourtZ + offsetFeet * UNITS_PER_FOOT);
    const double spacing = spacingFeet * UNITS_PER_FOOT;

    // Start with the first chair centered just inside the baseline line
    double x = baselineSign * halfCourtX - baselineSign * (spacing * 0.5);

    for (int i = 0; i < count; ++i)
    {
        chair(x, y, z, yawDeg, chairScale);
        x -= baselineSign * spacing;  // lay out chairs towards center court 
    }
}

// Simple riser under the second row of far-sideline chairs so they aren't just floating in space
//
void drawFarSidelineSecondRowRiser(double courtLenHalfX, double courtWidHalfZ, double benchY, double offCourtFeet, double secondRowOffsetFeet, int   sidelineFarPosZ)
{
   // Height bump matches the vertical offset used for the second row
   const double riserHeight = 0.10;    // same as benchY+0.1 in chair call
   const double y0 = benchY;          // base level (walkway)
   const double y1 = benchY + riserHeight;

   // Distance from chairs an sideline
   const double secondRowFeet = offCourtFeet + secondRowOffsetFeet; // from sideline to second row
   const double zSecondRow = sidelineFarPosZ * (courtWidHalfZ + secondRowFeet * UNITS_PER_FOOT); // Z of second row chairs

   // Let the riser run from a bit in front of the second row
   // out into the stands behind it.
   const double frontExtraFeet = 0.5; // toward the court
   const double backExtraFeet = 2.0;// deeper into the stands

   double zFront, zBack;
   if (sidelineFarPosZ > 0)
   {
      // Far side is +Z: court is "down" (smaller Z), stands are "up" (larger Z)
      zFront = zSecondRow - frontExtraFeet * UNITS_PER_FOOT;
      zBack  = zSecondRow + backExtraFeet  * UNITS_PER_FOOT;
   }
   else
   {
      // If we ever mirror this to the -Z side, flip the logic
      zFront = zSecondRow + frontExtraFeet * UNITS_PER_FOOT;
      zBack  = zSecondRow - backExtraFeet  * UNITS_PER_FOOT;
   }

   // Run the riser along essentially the full sideline span
   const double xL = -courtLenHalfX;
   const double xR =  courtLenHalfX;

   glDisable(GL_TEXTURE_2D);
   glColor3f(0.32f,0.32f,0.35f);  // slightly darker grey so it reads as a step

   glBegin(GL_QUADS);
   // Top surface where the chairs "live"
   glNormal3f(0,1,0);
   glVertex3d(xL,y1,zFront);
   glVertex3d(xR,y1,zFront);
   glVertex3d(xR,y1,zBack);
   glVertex3d(xL,y1,zBack);

   // Bottom of the block
   glNormal3f(0,-1,0);
   glVertex3d(xL,y0,zBack);
   glVertex3d(xR,y0,zBack);
   glVertex3d(xR,y0,zFront);
   glVertex3d(xL,y0,zFront);

   glNormal3f(0,0,-1);
   glVertex3d(xL,y0,zFront);
   glVertex3d(xR,y0,zFront);
   glVertex3d(xR,y1,zFront);
   glVertex3d(xL,y1,zFront);

   // Back face (toward the upper stands)
   glNormal3f(0,0,+1);
   glVertex3d(xR,y0,zBack);
   glVertex3d(xL,y0,zBack);
   glVertex3d(xL,y1,zBack);
   glVertex3d(xR,y1,zBack);

   // Left side
   glNormal3f(-1,0,0);
   glVertex3d(xL,y0,zBack);
   glVertex3d(xL,y0,zFront);
   glVertex3d(xL,y1,zFront);
   glVertex3d(xL,y1,zBack);

   // Right side
   glNormal3f(1,0,0);
   glVertex3d(xR,y0,zFront);
   glVertex3d(xR,y0,zBack);
   glVertex3d(xR,y1,zBack);
   glVertex3d(xR,y1,zFront);
   glEnd();

   glEnable(GL_TEXTURE_2D);
}


// Drawing net for basketball hoop (upside down trucated cone w/ texture)
void drawBasketballHoopNet(double baseRadius, double topRadius, double height, int numSlices, double swayPhase)
{
   double angleStepDeg = 360.0 / numSlices;

   // Maximum lateral offset for the bottom of cone... as a fraction of the radius
   double maxSway = 0.5*baseRadius;
   double swayOffset = maxSway * swayPhase;

   glBegin(GL_QUAD_STRIP);
   for(double angleDeg = 0.0; angleDeg <= 360.1; angleDeg += angleStepDeg)
   {
      double cosAngle = Cos(angleDeg);
      double sinAngle = Sin(angleDeg);

      // Normal for the sloped side
      double normalX = cosAngle * height;
      double normalY = (baseRadius - topRadius);
      double normalZ = sinAngle * height;

      double normalLen = sqrt(normalX*normalX + normalY*normalY + normalZ*normalZ);
      if(normalLen==0.0) normalLen=1.0;
      normalX /= normalLen;
      normalY /= normalLen;
      normalZ /= normalLen;

      glNormal3d(normalX, normalY, normalZ);

      // Texture coords 
      // s wraps around 0 to 1, representing 0 to 360 degs
      float s = (float)(angleDeg / 360.0);
      float tBottom = 1.0f;
      float tTop = 0.0f;

      // Bottom ring 
      glTexCoord2f(s, tBottom); glVertex3d(baseRadius*cosAngle, 0.0, baseRadius*sinAngle);
      // Bottom ring - gets inverted when applied to hoop in basketballHoop()
      glTexCoord2f(s, tTop); glVertex3d(topRadius*cosAngle, height, topRadius*sinAngle + swayOffset);
   }
   glEnd();
}

// draws entire hoop... updated with tranparent backboard 
void basketballHoop(double x, double y, double z, double s, double rot, double poleSetbackWorld)
{
   glPushMatrix();
   glTranslated(x, y, z);
   glRotated(rot, 0, 1, 0);
   glScaled(s, s, s);

   // DEFINING WHICH HOOP WE ARE DRAWING
   int hoopIndex = (x>=0.0) ? 0 : 1;

   // tunables
   const double unitsPerFoot = UNITS_PER_FOOT;
   const double invScale     = (s != 0.0) ? 1.0 / s : 0.0;

   // Real-world target dimensions (feet) so that it is to scale
   const double boardWidthFeet = 6.0;
   const double boardHeightFeet = 3.5;
   const double boardThicknessFeet = 0.167; // 2 inches
   const double rimRadiusFeet = 0.75; // 18 in diameter
   const double rimTubeFeet = 0.0625; //~0.75 in bar thickness
   const double rimHeightFeet = 10.0;
   const double boardBottomFeet = rimHeightFeet - 0.9; // bottom of backboard
   const double poleRadiusFeet = 0.25; // 6 in diameter support pole
   const double bracketLengthFeet = 0.75; // extension arm from pole to board

   const double bbThick = (boardThicknessFeet * unitsPerFoot) * invScale;
   const double bbWidth = (boardWidthFeet * unitsPerFoot) * invScale;
   const double bbHeight = (boardHeightFeet * unitsPerFoot) * invScale;

   const double rimMajor = (rimRadiusFeet * unitsPerFoot) * invScale;
   const double rimMinor = (rimTubeFeet * unitsPerFoot) * invScale;
   const double rimY = (rimHeightFeet * unitsPerFoot) * invScale;

   const double poleRadius = (poleRadiusFeet * unitsPerFoot) * invScale;

   // Backboard offset from the pole
   const double bbOffsetWorld = (poleRadiusFeet + bracketLengthFeet) * unitsPerFoot + 0.5 * (boardThicknessFeet * unitsPerFoot);
   const double bbOffset = bbOffsetWorld * invScale;
   const double bbBase = (boardBottomFeet * unitsPerFoot) * invScale;

   glEnable(GL_COLOR_MATERIAL);
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glEnable(GL_NORMALIZE);

   const double poleSetbackLocal = poleSetbackWorld * invScale;

   // Bottom joint where supports and arm attach
   const double yArm   = rimY;
   const double zStart = poleRadius - poleSetbackLocal;

   double bottomX = 0.0;
   double bottomY = yArm;
   double bottomZ = zStart;

   // Tunables for ceiling supports
   const double supportTopFeet = 45.0; // height of roof above floor
   const double supportSpreadFeet = 10.0; // how far left/right the V branches
   const double supportRadiusFeet = 0.20; 

   const double supportTopY = (supportTopFeet    * unitsPerFoot) * invScale;
   const double supportSpread = (supportSpreadFeet * unitsPerFoot) * invScale;
   const double supportRadius = (supportRadiusFeet * unitsPerFoot) * invScale;

   // Two anchor points on the ceiling, forming a V
   double topLeftX = -supportSpread;
   double topLeftY = supportTopY;
   double topLeftZ = bottomZ;  // same Z as arm joint; change to tilt forward/back

   double topRightX = supportSpread;
   double topRightY = supportTopY;
   double topRightZ = bottomZ;

   // Left bar of the V
   drawTexturedCylinderBetween(texPole,supportRadius,bottomX, bottomY, bottomZ,topLeftX, topLeftY, topLeftZ,32, 1.0, 2.0);
   // right bar of the v support
   drawTexturedCylinderBetween(texPole, supportRadius, bottomX, bottomY, bottomZ, topRightX, topRightY, topRightZ, 32, 1.0, 2.0);
   // Top bar connecting the hoop supports
   drawTexturedCylinderBetween(texPole, supportRadius, topLeftX,  topLeftY,  topLeftZ, topRightX, topRightY, topRightZ, 32, 1.0, 2.0);
   
   // Middle horizontal support bar
   const double tMid = 0.5; 
   double midLeftX = bottomX + tMid * (topLeftX - bottomX);
   double midLeftY = bottomY + tMid * (topLeftY - bottomY);
   double midLeftZ = bottomZ + tMid * (topLeftZ - bottomZ);
   double midRightX = bottomX + tMid * (topRightX - bottomX);
   double midRightY = bottomY + tMid * (topRightY - bottomY);
   double midRightZ = bottomZ + tMid * (topRightZ - bottomZ);

   drawTexturedCylinderBetween(texPole, supportRadius, midLeftX,  midLeftY,  midLeftZ, midRightX, midRightY, midRightZ, 32, 1.0, 2.0);


   const double zBack = bbOffset - bbThick*0.5; // back face of board
   double armLen = zBack - zStart;
   glColor3f(1.0f,1.0f,1.0f);
   glPushMatrix();
   glTranslated(0.0, yArm, zStart);
   glRotatef(90, 1, 0, 0); // make horizontal 
   drawTexturedCylinder(texPole, 0.06, armLen, 20, 1.0, 1.0);
   glPopMatrix();

   // Backboard
   glPushMatrix();
   glTranslated(0, bbBase, bbOffset);

   const double bx1 = -bbWidth/2, bx2 = +bbWidth/2;
   const double by1 = 0, by2 = bbHeight;
   const double bz1 = -bbThick/2, bz2 = +bbThick/2;

   glEnable(GL_BLEND);
   // Texture on the front
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texBackboard);
   glColor3f(1.0f,1.0f,1.0f);
   glBegin(GL_QUADS);
   glNormal3f(0,0,1);
   glTexCoord2f(0.0f,0.0f); glVertex3d(bx1, by1, bz2);
   glTexCoord2f(1.0f,0.0f); glVertex3d(bx2, by1, bz2);
   glTexCoord2f(1.0f,1.0f); glVertex3d(bx2, by2, bz2);
   glTexCoord2f(0.0f,1.0f); glVertex3d(bx1, by2, bz2);
   glEnd();
   glDisable(GL_TEXTURE_2D);
   glDisable(GL_BLEND);

   // Other faces (solid white, lit)
   glColor3f(1,1,1);
   glBegin(GL_QUADS);
   // No Back so see through but still sides
   // Top
   glNormal3f(0,1,0);
   glVertex3d(bx1, by2, bz2); glVertex3d(bx2, by2, bz2);
   glVertex3d(bx2, by2, bz1); glVertex3d(bx1, by2, bz1);
   // Bottom
   glNormal3f(0,-1,0);
   glVertex3d(bx1, by1, bz1); glVertex3d(bx2, by1, bz1);
   glVertex3d(bx2, by1, bz2); glVertex3d(bx1, by1, bz2);
   // Right
   glNormal3f(1,0,0);
   glVertex3d(bx2, by1, bz2); glVertex3d(bx2, by1, bz1);
   glVertex3d(bx2, by2, bz1); glVertex3d(bx2, by2, bz2);
   // Left
   glNormal3f(-1,0,0);
   glVertex3d(bx1, by1, bz1); glVertex3d(bx1, by1, bz2);
   glVertex3d(bx1, by2, bz2); glVertex3d(bx1, by2, bz1);
   glEnd();
   glPopMatrix(); // end backboard

   // Rim... net to come sooon given work on alpha channel of tex
   const double bbFrontZ = bbOffset + bbThick*0.5; // = 0.15 + 0.025 = 0.175
   const double rimZ = bbFrontZ + rimMajor + rimMinor; // touches board

   glColor3f(1.0f,0.4f,0.0f);
   glPushMatrix();
   glTranslated(0, rimY, rimZ);
   glRotatef(90, 1, 0, 0); // axis → Y (horizontal rim)
   drawTorus(rimMajor, rimMinor, 64, 24); // smoother & thinner

   glPopMatrix();
  
   // Drwing net to the hoop
   const double netTopRadius = rimMajor * 0.99; // connects to the inside of the rim
   const double netBottomRadius = netTopRadius * 0.65;
   const double netHeightFeet = 1.2;
   const double netHeight = (netHeightFeet*unitsPerFoot) * invScale;
   
   glPushMatrix();

   // center under the rim
   glTranslated(0, rimY, rimZ);
   // Cone extends downward (-y)
   glScaled(1.0,-1.0,1.0); // height goes downwards

   // push just below the rim
   glTranslated(0.0, 0.02, 0.0); 

   // Enabling blenmding since 32-bit bmp transparency works
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texBasketballNet);

   glColor3f(0.8f,0.8f,0.8f);
   drawBasketballHoopNet(netTopRadius, netBottomRadius, netHeight, 24, netSwayPhase[hoopIndex]);

   glDisable(GL_TEXTURE_2D);
   glDisable(GL_BLEND);

   glPopMatrix();
   
   // End hoop assembly
   glPopMatrix();
}


// Drawing a checkered basketball court similar to the Boston Celtics
// helpful for moving spotlights to form circle
void drawCircleFeet(double cx, double cz, double radiusFeet, int segments, double y, double feetPerX, double feetPerZ)
{
   glBegin(GL_LINE_STRIP);
   for (int i = 0; i <= segments; ++i) {
      double t = (double)i / segments * 2.0 * M_PI;
      double x = (radiusFeet * feetPerX) * cos(t);
      double z = (radiusFeet * feetPerZ) * sin(t);
      glVertex3d(cx + x, y, cz + z);
   }
   glEnd();
}

// Semicircle opening toward center court (−x direction), radius in FEET
void drawSemicircleTowardCenterFeet(double cx, double cz, double radiusFeet, int segments, double y, double feetPerX, double feetPerZ)
{
   glBegin(GL_LINE_STRIP);
   for (int i = 0; i <= segments; ++i) {
      double t = (-90.0 + (180.0 * i) / segments) * (M_PI / 180.0); // -90 deg to +90 deg
      double x = cx - (radiusFeet * feetPerX) * cos(t);
      double z = cz + (radiusFeet * feetPerZ) * sin(t);
      glVertex3d(x, y, z);
   }
   glEnd();
}

// Draw one basket end (key + FT semicircle + 3-pt semicircle)
void drawCourtEndFeet(double court_width, double y, double feetPerX, double feetPerZ, double keyWidthFeet, double ftFromBaselineFeet, double circleRadiusFeet)
{
   const double baseline_x = court_width / 2.0;
   const double ft_line_x  = baseline_x - ftFromBaselineFeet * feetPerX;
   const double half_key_z = (keyWidthFeet * 0.5) * feetPerZ;

   // Key of basketball court. 16ft wide
   glBegin(GL_LINE_LOOP);
   glVertex3d(baseline_x, y, -half_key_z);
   glVertex3d(ft_line_x,  y, -half_key_z);
   glVertex3d(ft_line_x,  y,  half_key_z);
   glVertex3d(baseline_x, y,  half_key_z);
   glEnd();

   /// Free-throw semicircle (6ft radius)... opening towards the baseline. Leave 2ft on sides for larger lane
   glBegin(GL_LINE_STRIP);
   for (int deg = -90; deg <= 90; ++deg) {
      double t = deg * (M_PI / 180.0);
      double x = ft_line_x - (circleRadiusFeet * feetPerX) * cos(t); // open inward (−x)
      double z = (circleRadiusFeet * feetPerZ) * sin(t);
      glVertex3d(x, y, z);
   }
   glEnd();

   // 3-point semicircle (centered at hoop), passes through FT-semirc apex 
   const double HOOP_CENTER_FROM_BASELINE_FEET = 5.25; // ~63 inches
   const double hoop_cx = baseline_x - HOOP_CENTER_FROM_BASELINE_FEET * feetPerX;
   const double hoop_cz = 0.0;

   // FT semicircle apex (farthest toward center court)
   const double ft_apex_x = ft_line_x - circleRadiusFeet * feetPerX;

   // Radius so 3-pt arc hits the FT apex
   const double dx_feet = (ft_apex_x - hoop_cx) / feetPerX;
   const double threePtRadiusFeet = fabs(dx_feet);

   // Draw the 3-pt semicircle (opening inward)
   drawSemicircleTowardCenterFeet(hoop_cx, hoop_cz, threePtRadiusFeet, 128, y, feetPerX, feetPerZ);

   // Corner connectors from the ends of the 3-pt arc to the baseline parallel to the sideline (constant z, increasing x)
   const double end_x = hoop_cx;
   const double end_z_1 = hoop_cz - threePtRadiusFeet * feetPerZ; // lower -z  
   const double end_z_2 = hoop_cz + threePtRadiusFeet * feetPerZ; // upper +z

   glBegin(GL_LINES);
   // lower corner connector
   glVertex3d(end_x, y, end_z_1);
   glVertex3d(baseline_x, y, end_z_1);
   // upper corner connector
   glVertex3d(end_x, y, end_z_2);
   glVertex3d(baseline_x, y, end_z_2);
   glEnd();
}

// Draws court markings (lines are not lit)
void drawCourtMarkings(double court_width, double court_depth)
{
   // Real dimensions easier to work with for scale
   const double KEY_WIDTH_FEET = 16.0; // lane width nba/ncaa
   const double FT_FROM_BASELINE_FEET = 19.0; // baseline to FT line
   const double CIRCLE_RADIUS_FEET = 6.0;  // center & FT circle radius (12ft diameter)

   // Convert feet -> world units along each axis
   // x spans 94 ft, z spans 50 ft
   const double feetPerX = court_width / 94.0;
   const double feetPerZ = court_depth / 50.0;

   // Slightly above the floor to avoid z-fighting
   const double y = 0.102;

   glColor3f(1, 1, 1);
   glLineWidth(2.0);

   // Half-court line
   glBegin(GL_LINES);
   glVertex3d(0, y, -court_depth / 2.0);
   glVertex3d(0, y,  court_depth / 2.0);
   glEnd();

   // +x basket 
   drawCourtEndFeet(court_width, y, feetPerX, feetPerZ, KEY_WIDTH_FEET, FT_FROM_BASELINE_FEET, CIRCLE_RADIUS_FEET);
   // -x basket
   glPushMatrix();
   glScalef(-1, 1, 1);
   drawCourtEndFeet(court_width, y, feetPerX, feetPerZ, KEY_WIDTH_FEET, FT_FROM_BASELINE_FEET, CIRCLE_RADIUS_FEET);
   glPopMatrix();

   glLineWidth(1.0);
}

// Draws checkerboard floor with normals for lighting (all pointed in +y)
// checkerboard w/ small squares required for the spot light effect
void drawCheckerboard(int rows, int cols, double tileSize)
{
   double totalWidth = cols * tileSize;
   double totalDepth = rows * tileSize;
   // Define the thickness of the court.
   double court_height = 0.1;

   glPushMatrix();
   // Translate the entire grid to center it at the origin.
   glTranslated(-totalWidth/2.0, 0, -totalDepth/2.0);

   
   // Draw the Top Surface (the checkerboard- now texture in hw6
   glBindTexture(GL_TEXTURE_2D, texWood);
   glEnable(GL_TEXTURE_2D);

   //Set color to white to show texture colors
   glColor3f(1, 1, 1);
   //Settings number of times for texture to repeat across the court so not stretched w unrealistic massive floor panels
   const double texRepeatU = 8.0; 
   const double texRepeatV = 4.0;

   // lifted to y = court_height
   glBegin(GL_QUADS);
   glNormal3f(0, 1, 0);
   
   for (int i = 0; i < rows; i++)
   {
      for (int j = 0; j < cols; j++)
      {
         double x0 = j * tileSize;
         double x1 = (j + 1) * tileSize;
         double z0 = i * tileSize;
         double z1 = (i + 1) * tileSize;

         // Map each tile into the repeating texture domain
         double u0 = (double)j / cols * texRepeatU;
         double u1 = (double)(j + 1)/ cols * texRepeatU;
         double v0 = (double)i / rows * texRepeatV;
         double v1 = (double)(i + 1)/ rows * texRepeatV;

         glTexCoord2d(u0, v0); glVertex3d(x0, court_height, z0);
         glTexCoord2d(u1, v0); glVertex3d(x1, court_height, z0);
         glTexCoord2d(u1, v1); glVertex3d(x1, court_height, z1);
         glTexCoord2d(u0, v1); glVertex3d(x0, court_height, z1);
      }
   }

   glEnd();
   glDisable(GL_TEXTURE_2D);

   // Draw the Sides and Bottom to give it thickness
   glColor3f(0.60f, 0.41f, 0.22f); // dark brown
   glBegin(GL_QUADS);
   // Front Edge
   glNormal3f(0, 0, 1);
   glVertex3d(0, 0, totalDepth);
   glVertex3d(totalWidth, 0, totalDepth);
   glVertex3d(totalWidth, court_height, totalDepth);
   glVertex3d(0, court_height, totalDepth);
   // Back Edge
   glNormal3f(0, 0, -1);
   glVertex3d(totalWidth, 0, 0);
   glVertex3d(0, 0, 0);
   glVertex3d(0, court_height, 0);
   glVertex3d(totalWidth, court_height, 0);
   // Left Edge
   glNormal3f(-1, 0, 0);
   glVertex3d(0, 0, 0);
   glVertex3d(0, 0, totalDepth);
   glVertex3d(0, court_height, totalDepth);
   glVertex3d(0, court_height, 0);
   // Right Edge
   glNormal3f(1, 0, 0);
   glVertex3d(totalWidth, 0, totalDepth);
   glVertex3d(totalWidth, 0, 0);
   glVertex3d(totalWidth, court_height, 0);
   glVertex3d(totalWidth, court_height, totalDepth);
   // Bottom Face
   glNormal3f(0, -1, 0);
   glVertex3d(0, 0, 0);
   glVertex3d(totalWidth, 0, 0);
   glVertex3d(totalWidth, 0, totalDepth);
   glVertex3d(0, 0, totalDepth);
   glEnd();

   glPopMatrix();
}

// Draws the entire court (floor & lines)
void drawBasketballCourt(int rows, int cols, double tileSize)
{
   const double walkwayFeet = 20.0;
   const double walkwayInnerFeet = 5.0;
   const double walkwayPad = walkwayFeet * UNITS_PER_FOOT;
   const double walkwayInnerPad = walkwayInnerFeet * UNITS_PER_FOOT;
   const double courtWidth = cols * tileSize;
   const double courtDepth = rows * tileSize;
   const double innerHalfX = courtWidth * 0.5;
   const double innerHalfZ = courtDepth * 0.5;
   const double midHalfX = innerHalfX + walkwayInnerPad;
   const double midHalfZ = innerHalfZ + walkwayInnerPad;
   const double outerHalfX = innerHalfX + walkwayPad;
   const double outerHalfZ = innerHalfZ + walkwayPad;
   const double walkwayY = 0.1;  // align with court surface

   // boundary of court
   double innerLoop[5][2] = {
      {-innerHalfX, -innerHalfZ},
      { innerHalfX, -innerHalfZ},
      { innerHalfX,  innerHalfZ},
      {-innerHalfX,  innerHalfZ},
      {-innerHalfX, -innerHalfZ}
   };
   // white to gray separator
   double midLoop[5][2] = {
      {-midHalfX, -midHalfZ},
      { midHalfX, -midHalfZ},
      { midHalfX,  midHalfZ},
      {-midHalfX,  midHalfZ},
      {-midHalfX, -midHalfZ}
   };
   // outer edge of grey boundary
   double outerLoop[5][2] = {
      {-outerHalfX, -outerHalfZ},
      { outerHalfX, -outerHalfZ},
      { outerHalfX,  outerHalfZ},
      {-outerHalfX,  outerHalfZ},
      {-outerHalfX, -outerHalfZ}
   };

   glDisable(GL_TEXTURE_2D);

   const int apronSegs = 32;

   glColor3f(1.0f, 1.0f, 1.0f); 
   glNormal3f(0, 1, 0);         

   // Inner 5ft padding made up of multiple segment to works well w/ lighting
   // Polygons not as dense as court but enough to react to lighting
   for (int edge = 0; edge < 4; ++edge)
   {
      int i0 = edge;
      int i1 = edge + 1;

      glBegin(GL_QUAD_STRIP);
      for (int k = 0; k <= apronSegs; ++k)
      {
         double t = (double)k / apronSegs;

         double innerX = innerLoop[i0][0] + t * (innerLoop[i1][0] - innerLoop[i0][0]);
         double innerZ = innerLoop[i0][1] + t * (innerLoop[i1][1] - innerLoop[i0][1]);

         double midX   = midLoop[i0][0] + t * (midLoop[i1][0]   - midLoop[i0][0]);
         double midZ   = midLoop[i0][1] + t * (midLoop[i1][1]   - midLoop[i0][1]);

         glVertex3d(midX,   walkwayY, midZ);
         glVertex3d(innerX, walkwayY, innerZ);
      }
      glEnd();
   }

   // Remaining apron
   glColor3f(0.35f, 0.35f, 0.35f);
   glBegin(GL_QUAD_STRIP);
   glNormal3f(0, 1, 0);
   for (int i = 0; i < 5; ++i)
   {
      glVertex3d(outerLoop[i][0], walkwayY, outerLoop[i][1]);
      glVertex3d(midLoop[i][0],   walkwayY, midLoop[i][1]);
   }
   glEnd();

   glEnable(GL_TEXTURE_2D);

   // Draw floor tiles first
   drawCheckerboard(rows, cols, tileSize); 

   // Drawing geo mountiains on the court level with a sideline
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texSidelineLogo); 
   glColor3f(1,1,1);
   glNormal3f(0,1,0);

   // Choose physical size of the mountain (ft on court)
   const double logoFeetWidthX = 48.0;  
   const double logoFeetDepthZ = 16.0;

   const double logoHalfX  = (logoFeetWidthX * UNITS_PER_FOOT) * 0.5;
   const double logoDepthZ = logoFeetDepthZ * UNITS_PER_FOOT;

   // Lift slightly above court to avoid z-fighting with texWood
   const double logoY = walkwayY + 0.001;
   double logoZ0b = -innerHalfZ; // directly on sideline
   double logoZ1b = logoZ0b + logoDepthZ;        

   glBegin(GL_QUADS);
   glTexCoord2f(0.0f, 0.0f); glVertex3d(-logoHalfX, logoY, logoZ0b);
   glTexCoord2f(1.0f, 0.0f); glVertex3d( logoHalfX, logoY, logoZ0b);
   glTexCoord2f(1.0f, 1.0f); glVertex3d( logoHalfX, logoY, logoZ1b);
   glTexCoord2f(0.0f, 1.0f); glVertex3d(-logoHalfX, logoY, logoZ1b);
   glEnd();

   glDisable(GL_TEXTURE_2D);
   glDisable(GL_BLEND);


   // Drawing the "COLORADO" word marks on the baseline
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texColoradoWordmark);
   glColor3f(1,1,1);

   glNormal3f(0,1,0);
   const double wordFeetLengthZ = 32.0;  
   const double wordFeetHeightX = 4.0;  

   const double wordHalfZ   = (wordFeetLengthZ * UNITS_PER_FOOT) * 0.5;
   const double wordHeightX = (wordFeetHeightX * UNITS_PER_FOOT);

   const double wordY = walkwayY + 0.001; // tiny lift to avoid z-fighting
   const double baselineOffset = 0.1; // not directly next to court

   // right baseline
   double rightX0 = innerHalfX + baselineOffset;  
   double rightX1 = rightX0 + wordHeightX;         

   glBegin(GL_QUADS);
   glTexCoord2f(0.0f, 0.0f); glVertex3d(rightX0, wordY, -wordHalfZ);
   glTexCoord2f(1.0f, 0.0f); glVertex3d(rightX0, wordY,  wordHalfZ);
   glTexCoord2f(1.0f, 1.0f); glVertex3d(rightX1, wordY,  wordHalfZ);
   glTexCoord2f(0.0f, 1.0f); glVertex3d(rightX1, wordY, -wordHalfZ);
   glEnd();

   // left baseline
   double leftX1 = -innerHalfX - baselineOffset; 
   double leftX0 = leftX1 - wordHeightX;         

   glBegin(GL_QUADS);
   glTexCoord2f(1.0f, 0.0f); glVertex3d(leftX1, wordY, -wordHalfZ);
   glTexCoord2f(0.0f, 0.0f); glVertex3d(leftX1, wordY,  wordHalfZ);
   glTexCoord2f(0.0f, 1.0f); glVertex3d(leftX0, wordY,  wordHalfZ);
   glTexCoord2f(1.0f, 1.0f); glVertex3d(leftX0, wordY, -wordHalfZ);
   glEnd();

   glDisable(GL_BLEND);
   glDisable(GL_TEXTURE_2D);

   glEnable(GL_POLYGON_OFFSET_LINE);
   glPolygonOffset(-1.0, -1.0); // Negative values pull the lines toward the camera

   // Disable lighting for the lines so always white
   glDisable(GL_LIGHTING);
   glDisable(GL_TEXTURE_2D);
   
   drawCourtMarkings(cols * tileSize, rows * tileSize);

   // Reset lighting
   if (light) glEnable(GL_LIGHTING);
   glDisable(GL_POLYGON_OFFSET_LINE);
}


// Generic 4-wall shell around a rectangle defined by inner X/Z.
// u define inner bounds, vertical span, thickness, and color
// it builds the full rectangular "ring" (inner + outer faces, edges, top).
void drawWallShellCore(double xInnerL, double xInnerR, double zInnerF, double zInnerB, double y0, double y1, double wallThickness, float r, float g, float b)
{
   // Outer line so the walls have thickness
   const double xOuterL = xInnerL - wallThickness;
   const double xOuterR = xInnerR + wallThickness;
   const double zOuterF = zInnerF - wallThickness;
   const double zOuterB = zInnerB + wallThickness;

   glDisable(GL_TEXTURE_2D);
   glColor3f(r,g,b);

   glBegin(GL_QUADS);

   // Left baseline wall, -X side
   // Outer face (outside the arena)
   glNormal3f(-1,0,0);
   glVertex3d(xOuterL, y0, zOuterB);
   glVertex3d(xOuterL, y0, zOuterF);
   glVertex3d(xOuterL, y1, zOuterF);
   glVertex3d(xOuterL, y1, zOuterB);

   // Inner face (faces seats/court)
   glNormal3f(1,0,0);
   glVertex3d(xInnerL, y0, zInnerF);
   glVertex3d(xInnerL, y0, zInnerB);
   glVertex3d(xInnerL, y1, zInnerB);
   glVertex3d(xInnerL, y1, zInnerF);

   // Front edge (ties to front wall)
   glNormal3f(0,0,1);
   glVertex3d(xInnerL, y0, zInnerF);
   glVertex3d(xOuterL, y0, zOuterF);
   glVertex3d(xOuterL, y1, zOuterF);
   glVertex3d(xInnerL, y1, zInnerF);

   // Back edge (ties to back wall)
   glNormal3f(0,0,-1);
   glVertex3d(xOuterL, y0, zOuterB);
   glVertex3d(xInnerL, y0, zInnerB);
   glVertex3d(xInnerL, y1, zInnerB);
   glVertex3d(xOuterL, y1, zOuterB);

   // Top of left wall
   glNormal3f(0,1,0);
   glVertex3d(xOuterL, y1, zOuterB);
   glVertex3d(xOuterL, y1, zOuterF);
   glVertex3d(xInnerL, y1, zInnerF);
   glVertex3d(xInnerL, y1, zInnerB);

   // Right bseline wall, +X side
   // Outer face
   glNormal3f(1,0,0);
   glVertex3d(xOuterR, y0, zOuterF);
   glVertex3d(xOuterR, y0, zOuterB);
   glVertex3d(xOuterR, y1, zOuterB);
   glVertex3d(xOuterR, y1, zOuterF);

   // Inner face
   glNormal3f(-1,0,0);
   glVertex3d(xInnerR, y0, zInnerB);
   glVertex3d(xInnerR, y0, zInnerF);
   glVertex3d(xInnerR, y1, zInnerF);
   glVertex3d(xInnerR, y1, zInnerB);

   // Front edge
   glNormal3f(0,0,1);
   glVertex3d(xInnerR, y0, zInnerF);
   glVertex3d(xOuterR, y0, zOuterF);
   glVertex3d(xOuterR, y1, zOuterF);
   glVertex3d(xInnerR, y1, zInnerF);

   // Back edge
   glNormal3f(0,0,-1);
   glVertex3d(xOuterR, y0, zOuterB);
   glVertex3d(xInnerR, y0, zInnerB);
   glVertex3d(xInnerR, y1, zInnerB);
   glVertex3d(xOuterR, y1, zOuterB);

   // Top of right wall
   glNormal3f(0,1,0);
   glVertex3d(xInnerR, y1, zInnerB);
   glVertex3d(xInnerR, y1, zInnerF);
   glVertex3d(xOuterR, y1, zOuterF);
   glVertex3d(xOuterR, y1, zOuterB);


   // Front wall fans side line, +Z side 
   // Inner face (faces court)
   glNormal3f(0,0,1);
   glVertex3d(xInnerL, y0, zInnerF);
   glVertex3d(xInnerR, y0, zInnerF);
   glVertex3d(xInnerR, y1, zInnerF);
   glVertex3d(xInnerL, y1, zInnerF);

   // Outer face
   glNormal3f(0,0,-1);
   glVertex3d(xOuterR, y0, zOuterF);
   glVertex3d(xOuterL, y0, zOuterF);
   glVertex3d(xOuterL, y1, zOuterF);
   glVertex3d(xOuterR, y1, zOuterF);

   // Left edge tie-in
   glNormal3f(-1,0,0);
   glVertex3d(xOuterL, y0, zOuterF);
   glVertex3d(xInnerL, y0, zInnerF);
   glVertex3d(xInnerL, y1, zInnerF);
   glVertex3d(xOuterL, y1, zOuterF);

   // Right edge tie-in
   glNormal3f(1,0,0);
   glVertex3d(xInnerR, y0, zInnerF);
   glVertex3d(xOuterR, y0, zOuterF);
   glVertex3d(xOuterR, y1, zOuterF);
   glVertex3d(xInnerR, y1, zInnerF);

   // Top of front wall
   glNormal3f(0,1,0);
   glVertex3d(xInnerL, y1, zInnerF);
   glVertex3d(xInnerR, y1, zInnerF);
   glVertex3d(xOuterR, y1, zOuterF);
   glVertex3d(xOuterL, y1, zOuterF);

   // Team bench sideline, −Z side
   // Inner face
   glNormal3f(0,0,-1);
   glVertex3d(xInnerR, y0, zInnerB);
   glVertex3d(xInnerL, y0, zInnerB);
   glVertex3d(xInnerL, y1, zInnerB);
   glVertex3d(xInnerR, y1, zInnerB);

   // Outer face
   glNormal3f(0,0,1);
   glVertex3d(xOuterL, y0, zOuterB);
   glVertex3d(xOuterR, y0, zOuterB);
   glVertex3d(xOuterR, y1, zOuterB);
   glVertex3d(xOuterL, y1, zOuterB);

   // Left edge
   glNormal3f(-1,0,0);
   glVertex3d(xOuterL, y0, zOuterB);
   glVertex3d(xInnerL, y0, zInnerB);
   glVertex3d(xInnerL, y1, zInnerB);
   glVertex3d(xOuterL, y1, zOuterB);

   // Right edge
   glNormal3f(1,0,0);
   glVertex3d(xInnerR, y0, zInnerB);
   glVertex3d(xOuterR, y0, zOuterB);
   glVertex3d(xOuterR, y1, zOuterB);
   glVertex3d(xInnerR, y1, zInnerB);

   // Top of back wall
   glNormal3f(0,1,0);
   glVertex3d(xOuterL, y1, zOuterB);
   glVertex3d(xOuterR, y1, zOuterB);
   glVertex3d(xInnerR, y1, zInnerB);
   glVertex3d(xInnerL, y1, zInnerB);

   glEnd();

   glEnable(GL_TEXTURE_2D);
}

void drawArenaBoundaryWalls(double courtLenHalfX, double courtWidHalfZ, double baseY)
{
   // Must match the walkway drew in drawBasketballCourt
   const double walkwayFeet = 20.0; // feet from court to end of walkway
   const double wallOffsetFeet = 20.0; // extra distance beyond walkway
   const double wallHeightFeet = 18.0; // tall shell
   const double wallThickness = 0.30; // thickness of the shell

   const double wallHeight = wallHeightFeet * UNITS_PER_FOOT;
   const double radialFeet = walkwayFeet + wallOffsetFeet;
   const double offset = radialFeet * UNITS_PER_FOOT;

   // Inner line where the wall faces the arena
   const double xInnerL = -(courtLenHalfX + offset);
   const double xInnerR =  (courtLenHalfX + offset);
   const double zInnerF = -(courtWidHalfZ + offset);
   const double zInnerB =  (courtWidHalfZ + offset);

   const double y0 = baseY; // sits on same level as benches/walkway
   const double y1 = baseY + wallHeight;

   // Darker color for the outer bowl
   drawWallShellCore(xInnerL, xInnerR, zInnerF, zInnerB, y0, y1, wallThickness, 0.10f, 0.10f, 0.12f);
}

// Shorter inner wall that runs all the way around the court,
// just behind the 2-row riser distance everywhere.
void drawLowerBowlWalls(double courtLenHalfX, double courtWidHalfZ, double baseY, double offCourtFeet, double secondRowOffsetFeet)
{
   // How far from the court is the second row of seats?
   const double seatRadialFeet = offCourtFeet + secondRowOffsetFeet;

   // How far behind the riser do we put the low wall (in feet)
   const double behindRiserFeet = 2.0; 

   const double radialFeet = seatRadialFeet + behindRiserFeet;
   const double wallHeightFeet = 2.5; // short wall / rail height
   const double wallThickness = 0.20; // a bit thinner than the big shell

   const double wallHeight = wallHeightFeet * UNITS_PER_FOOT;
   const double offset = radialFeet * UNITS_PER_FOOT;

   // Inner face of this low wall relative to the playing surface
   const double xInnerL = -(courtLenHalfX + offset);
   const double xInnerR =  (courtLenHalfX + offset);
   const double zInnerF = -(courtWidHalfZ + offset);
   const double zInnerB =  (courtWidHalfZ + offset);

   const double y0 = baseY;
   const double y1 = baseY + wallHeight;

   drawWallShellCore(xInnerL, xInnerR, zInnerF, zInnerB, y0, y1, wallThickness, 0.20f, 0.20f, 0.20f);
}

void drawBleacherFanPlanes(double courtLenHalfX, double courtWidHalfZ, double baseY, double offCourtFeet, double secondRowOffsetFeet)
{
   // These MUST stay in sync w/ wall code... brittle (FIX LATER)
   const double lowWallHeightFeet = 2.5; // from drawLowerBowlWalls
   const double highWallHeightFeet = 18.0;  // from drawArenaBoundaryWalls

   const double walkwayFeet = 20.0; // from drawArenaBoundaryWalls
   const double wallOffsetFeet = 20.0; // from drawArenaBoundaryWalls
   const double behindRiserFeet = 2.0; // from drawLowerBowlWalls

   // Vertical span: from top of low rail to top of tall shell
   const double yLowTop  = baseY + lowWallHeightFeet  * UNITS_PER_FOOT;
   const double yHighTop = baseY + highWallHeightFeet * UNITS_PER_FOOT;

   // Radial position of the low rail (just behind the 2-row riser)
   const double radialFeetLow  = offCourtFeet + secondRowOffsetFeet + behindRiserFeet;
   // Radial position of the tall outer wall
   const double radialFeetHigh = walkwayFeet + wallOffsetFeet;

   const double offsetLow = radialFeetLow  * UNITS_PER_FOOT;
   const double offsetHigh = radialFeetHigh * UNITS_PER_FOOT;
   const double deltaOff = offsetHigh - offsetLow; // how far between them

   // Inner face of the tall bowl (same as drawArenaBoundaryWalls)
   const double xHighL = -(courtLenHalfX + offsetHigh);
   const double xHighR =  (courtLenHalfX + offsetHigh);
   const double zHighF = -(courtWidHalfZ + offsetHigh);
   const double zHighB =  (courtWidHalfZ + offsetHigh);

   // Line at the top of the low rail (pulled in toward the court by deltaOff)
   const double xLowL = xHighL + deltaOff; // left side
   const double xLowR = xHighR - deltaOff; // right side
   const double zLowF = zHighF + deltaOff; // near sideline
   const double zLowB = zHighB - deltaOff; //far sideline

   const int sideCols = 8;
   const int baseCols = 4;

   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texSportsFans);  
   glColor3f(1.0f, 1.0f, 1.0f);

#define CROWD_QUAD(nx,ny,nz,  x0,y0,z0,  x1,y1,z1,  x2,y2,z2,  x3,y3,z3) \
   do {                                                                  \
      glBegin(GL_QUADS);                                                 \
      glNormal3f((nx),(ny),(nz));                                        \
      glTexCoord2f(0.0f,0.0f); glVertex3d((x0),(y0),(z0));               \
      glTexCoord2f(1.0f,0.0f); glVertex3d((x1),(y1),(z1));               \
      glTexCoord2f(1.0f,1.0f); glVertex3d((x2),(y2),(z2));               \
      glTexCoord2f(0.0f,1.0f); glVertex3d((x3),(y3),(z3));               \
      glEnd();                                                           \
   } while(0)

   // Team bench sideline -Z, facing +Z
   for (int c = 0; c < sideCols; ++c)
   {
      double u0 = (double)c / sideCols;
      double u1 = (double)(c + 1) / sideCols;

      // Bottom edge (on low rail line)
      double xb0 = xLowL + u0 * (xLowR - xLowL);
      double xb1 = xLowL + u1 * (xLowR - xLowL);
      // Top edge (on tall wall line)
      double xt0 = xHighL + u0 * (xHighR - xHighL);
      double xt1 = xHighL + u1 * (xHighR - xHighL);

      CROWD_QUAD(0,1,1,
                 xb0, yLowTop,  zLowF,
                 xb1, yLowTop,  zLowF,
                 xt1, yHighTop, zHighF,
                 xt0, yHighTop, zHighF);
   }

   // Fan sideline +Z, facing -Z
   for (int c = 0; c < sideCols; ++c)
   {
      double u0 = (double)c / sideCols;
      double u1 = (double)(c + 1) / sideCols;

      double xb0 = xLowR  - u0 * (xLowR - xLowL);
      double xb1 = xLowR  - u1 * (xLowR - xLowL);
      double xt0 = xHighR - u0 * (xHighR - xHighL);
      double xt1 = xHighR - u1 * (xHighR - xHighL);

      CROWD_QUAD(0,1,-1,
                 xb0, yLowTop,  zLowB,
                 xb1, yLowTop,  zLowB,
                 xt1, yHighTop, zHighB,
                 xt0, yHighTop, zHighB);
   }

   // Left baseline -X, facing +X
   for (int c = 0; c < baseCols; ++c)
   {
      double u0 = (double)c / baseCols;
      double u1 = (double)(c + 1) / baseCols;

      double zb0 = zLowF  + u0 * (zLowB  - zLowF);
      double zb1 = zLowF  + u1 * (zLowB  - zLowF);
      double zt0 = zHighF + u0 * (zHighB - zHighF);
      double zt1 = zHighF + u1 * (zHighB - zHighF);

      CROWD_QUAD(1,1,0,
                 xLowL,  yLowTop,  zb1,
                 xLowL,  yLowTop,  zb0,
                 xHighL, yHighTop, zt0,
                 xHighL, yHighTop, zt1);
   }

   // Right baseline +X, facing -X
   for (int c = 0; c < baseCols; ++c)
   {
      double u0 = (double)c / baseCols;
      double u1 = (double)(c + 1) /baseCols;

      double zb0 = zLowB  - u0 * (zLowB  - zLowF);
      double zb1 = zLowB  - u1 * (zLowB  - zLowF);
      double zt0 = zHighB - u0 * (zHighB - zHighF);
      double zt1 = zHighB - u1 * (zHighB - zHighF);

      CROWD_QUAD(-1,1,0,
                 xLowR,  yLowTop,  zb0,
                 xLowR,  yLowTop,  zb1,
                 xHighR, yHighTop, zt1,
                 xHighR, yHighTop, zt0);
   }

#undef CROWD_QUAD

}

// Arena fan shell:
void drawArenaBowlAndCrowd(double courtLenHalfX, double courtWidHalfZ, double baseY, double offCourtFeet, double secondRowOffsetFeet)
{
   // Short low wall just behind the 2-row riser
   drawLowerBowlWalls(courtLenHalfX, courtWidHalfZ, baseY, offCourtFeet, secondRowOffsetFeet);
   // Big outer arena shell
   drawArenaBoundaryWalls(courtLenHalfX, courtWidHalfZ, baseY);
   // Crowd / bleacher planes between them
   drawBleacherFanPlanes(courtLenHalfX, courtWidHalfZ, baseY, offCourtFeet, secondRowOffsetFeet);
}

// SCORE BOARD FUNCTIONS
void drawScoreboardFace()
{
   // Dimensions of the scoreboard 
   const double width = 6.0;
   const double height = 3.0;
   const double depth = 0.0;

   // Scaling dim to draw board around local origin
   double hw = width/2;
   double hh = height/2;
   double hd = depth/2;

   // Enabling lighting for the socreboard
   glEnable(GL_LIGHTING);
   glEnable(GL_TEXTURE_2D);
   glColor3f(0.15f, 0.15f, 0.15f);

   // Front main face... simple quads
   glBegin(GL_QUADS);
   glNormal3f(0,0,1);

   glVertex3d(-hw, -hh, hd);
   glVertex3d(hw, -hh, hd);
   glVertex3d(hw, hh, hd);
   glVertex3d(-hw, hh, hd);
   glEnd();

   // Draw the screen as a textured quad (front pane)
   const double screenMargin = 0.25;
   const double scoreHeight = 0.8;

   // Score region of board
   double scoreBottom = -hh + screenMargin;
   double scoreTop = scoreBottom + scoreHeight;
   double scoreMiddle = 0.5 * (scoreBottom + scoreTop) - 0.5; // shift down 

   // Video region of board
   double videoBottom = scoreTop;
   double videoTop = hh - screenMargin;

   // Screen dimensions
   double sw = width - 2.0*screenMargin;
   double sx0 = -sw/2;
   double sx1 = sw/2;

   double sy0 = videoBottom;
   double sy1 = videoTop;

   // VIDEO SCREEN SECTION
   if(texVideoFrames[currentVideoFrame]) 
   {
      glBindTexture(GL_TEXTURE_2D, texVideoFrames[currentVideoFrame]);

      // no lighitng on screen so the "video" looks fine
      // white color to not bleed into video 
      glDisable(GL_LIGHTING);
      glColor3f(1.0f, 1.0f, 1.0f);

      glBegin(GL_QUADS);
      glTexCoord2f(0,0); glVertex3d(sx0, sy0, hd+0.01);
      glTexCoord2f(1,0); glVertex3d(sx1, sy0, hd+0.01);
      glTexCoord2f(1,1); glVertex3d(sx1, sy1, hd+0.01);
      glTexCoord2f(0,1); glVertex3d(sx0, sy1, hd+0.01);
      glEnd();

      glDisable(GL_TEXTURE_2D);
      glEnable(GL_LIGHTING);
   }

   // Score section of the video board
   // String that stores the score 
   glLineWidth(1.0f);
   char scoreStr[32];
   snprintf(scoreStr, sizeof(scoreStr), "Away %d - %d Home", score[0], score[1]);

   // Measure the width of the score so that centered on scoreboard
   float textWidth = 0.0f;
   for(char *p = scoreStr; *p; ++p)
      textWidth += glutStrokeWidth(GLUT_STROKE_ROMAN, *p); // GLUT_STROKE_ROMAN draws the characters as line segments... by-passing glutBitMap
   
   if(textWidth > 0) // score to display
   {
      // use middle 60% of the screen
      double targetWidth = sw * 0.8;
      double textScale = targetWidth / textWidth;

      glDisable(GL_LIGHTING);
      glDisable(GL_TEXTURE_2D);
      glColor3f(1.0f, 1.0f, 1.0f);

      glPushMatrix();

      // Centering height in score region
      glTranslated(0.0, scoreMiddle, hd+0.02);
      glScaled(textScale, textScale, textScale); // font units to universal units

      // Centering width wise... close as possible since measured from first character
      glTranslated(-textWidth*0.5, -0.0, 0.0);

      for(char *p = scoreStr; *p; ++p)
      {
         glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
      }

      glPopMatrix();

      glEnable(GL_TEXTURE_2D);
      glEnable(GL_LIGHTING);
   }
}

// Draw complete scoreboard cube
void drawCompleteScoreboard(double x, double y, double z) 
{
   // determing radius of scoreboard 
   const double width = 6.0; // MUST MATCH drawScoreboardFace()
   const double height = 3.0;
   const double globalHeight = 7.5;
   
   const double radius = width/2;
   const double topSbHeight = globalHeight + height/2;
   const double bottomSbHeight = globalHeight - height/2;
   
   glPushMatrix();
   glTranslated(x, y, z);

   // Facing +z
   glPushMatrix();
   glTranslated(0.0, 0.0, radius);
   drawScoreboardFace();
   glPopMatrix();

   // Facing -x
   glPushMatrix();
   glRotated(90, 0, 1, 0);
   glTranslated(0.0, 0.0, radius);
   drawScoreboardFace();
   glPopMatrix();

   // Facing -z
   glPushMatrix();
   glRotated(180, 0, 1, 0);
   glTranslated(0.0, 0.0, radius);
   drawScoreboardFace();
   glPopMatrix();

   // Facing +x
   glPushMatrix();
   glRotated(270, 0, 1, 0);
   glTranslated(0.0, 0.0, radius);
   drawScoreboardFace();
   glPopMatrix();

   glPopMatrix();

   // TOP & BOTTOM CAPS ON SCOREBOARD 
   glDisable(GL_LIGHTING);
   glEnable(GL_TEXTURE_2D);
   glColor3f(1.0f, 1.0f, 1.0f);

   if(texScoreBoardLogo)
   {
      glBindTexture(GL_TEXTURE_2D, texScoreBoardLogo);

      glBegin(GL_QUADS);
      glNormal3f(0,1,0);
      glTexCoord2f(1,0); glVertex3d(-radius, topSbHeight, -radius); 
      glTexCoord2f(0,0); glVertex3d(radius, topSbHeight, -radius); 
      glTexCoord2f(0,1); glVertex3d(radius, topSbHeight, radius); 
      glTexCoord2f(1,1); glVertex3d(-radius, topSbHeight, radius);
      glEnd();

      glBegin(GL_QUADS);
      glNormal3f(0, -1, 0);
      glTexCoord2f(0,0); glVertex3d(-radius, bottomSbHeight,  radius);
      glTexCoord2f(1,0); glVertex3d( radius, bottomSbHeight,  radius);
      glTexCoord2f(1,1); glVertex3d( radius, bottomSbHeight, -radius);
      glTexCoord2f(0,1); glVertex3d(-radius, bottomSbHeight, -radius);
      glEnd();
   }
}


// center court logo 
void drawCenterLogo()
{
   double radius = 1.3;
   double logoHeight = 0.11;

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glBindTexture(GL_TEXTURE_2D, texCenterLogo);
   glBegin(GL_QUADS);
   glNormal3f(0,1,0);
   glTexCoord2f(0.0, 1.0); glVertex3d(radius, logoHeight, radius); // top right
   glTexCoord2f(1.0, 1.0); glVertex3d(-radius, logoHeight, radius); // top left
   glTexCoord2f(1.0, 0.0); glVertex3d(-radius, logoHeight, -radius); // bottom left
   glTexCoord2f(0.0, 0.0); glVertex3d(radius, logoHeight, -radius); // bottom right
   glEnd();

   glDisable(GL_BLEND);
}


// MASTER BASKETBALL COURT FUNCTION: Draws the entire basketball court scene.
// Standard version drawing full court with lines
void drawCompleteBasketballCourt(double x, double y, double z, double scale)
{
   glPushMatrix();
   glTranslated(x, y, z);
   glScalef(scale, scale, scale);

   // Lots of tiles for spot lights to flow 
   const int rows = 400;
   const int cols = 768;
   const double tileSize = 0.025;
   const double court_width = cols * tileSize;

   drawBasketballCourt(rows, cols, tileSize);

   const double courtLenHalfX = (cols * tileSize) * 0.5;  // baseline at ±X
   const double courtWidHalfZ = (rows * tileSize) * 0.5;  // sideline at ±Z

   // hoops
   const double hoop_x_pos = court_width * 0.5;
   const double hoop_scale = 0.6; 
   const double hoopPoleSetback = 3.0 * UNITS_PER_FOOT;
   basketballHoop( hoop_x_pos, 0, 0, hoop_scale, -90, hoopPoleSetback);
   basketballHoop(-hoop_x_pos, 0, 0, hoop_scale,  90, hoopPoleSetback);

   // reconstructing rim positioning scheme using same real-world values from basketballHoop()
   const double rimHeightFeet = 10.0;
   const double boardThicknessFeet = 0.167;
   const double rimRadiusFeet = 0.75;
   const double rimTubeFeet = 0.25;
   const double poleRadiusFeet = 0.25;
   const double bracketLengthFeet = 2.0;

   const double rimY_world = rimHeightFeet*UNITS_PER_FOOT;

   const double bbThick_world = boardThicknessFeet*UNITS_PER_FOOT;
   const double bbOffset_world = (poleRadiusFeet+bracketLengthFeet)*UNITS_PER_FOOT + .5*(boardThicknessFeet*UNITS_PER_FOOT);
   const double bbFrontZ_world = bbOffset_world + 0.5 *bbThick_world;
   const double rimMajor_world = rimRadiusFeet*UNITS_PER_FOOT;
   const double rimMinor_world = rimTubeFeet*UNITS_PER_FOOT;
   const double rimZ_world = bbFrontZ_world + rimMajor_world + rimMinor_world;

   // Center of rim for each hoop
   // Define as own vars to not get confused
   const double rimX_plus = hoop_x_pos - rimZ_world;
   const double rimX_minus = -hoop_x_pos + rimZ_world;
   const double rimY = rimY_world;
   const double rimZ = 0.0;

   // Starting position of the shot
   const double shotDistanceFeet = 22.0;
   const double shotOffsetX = shotDistanceFeet * UNITS_PER_FOOT;
   
   const double start0_x = rimX_plus - shotOffsetX;
   const double start0_y = 5.0 * UNITS_PER_FOOT;
   const double start0_z = 0.0;

   const double start1_x = rimX_minus + shotOffsetX;
   const double start1_y = start0_y;
   const double start1_z = 0.0;

   // Locations for apex of the shooting arc for both sides of the court
   const double peak0_x = 0.5 * (start0_x + rimX_plus);
   const double peak1_x = 0.5* (start1_x + rimX_minus);
   const double peak_y = rimY + 12.0 * UNITS_PER_FOOT; // 5ft above the rim is the apex
   const double peak_z  = 0.0;

   // size of basketball
   const double ballRadiusFeet = 0.40;
   const double ballRadius = ballRadiusFeet *UNITS_PER_FOOT;

   //Net depth for ending position so that ball goes through the net
   const double netHeightFeet = 1.2;
   const double netHeightWorld = netHeightFeet*UNITS_PER_FOOT;

   const double end0_x = rimX_plus+0.5;
   const double end0_y = rimY - 1.5*netHeightWorld;
   const double end0_z = rimZ;

   const double end1_x = rimX_minus-0.5;
   const double end1_y = rimY - 1.5*netHeightWorld;
   const double end1_z = rimZ;


   // Lambda-like macro for quadratic Bezier interpolation ... bezier function recieved from ChatGPT
   // Bezier: B(t) = (1-t)^2 * p) + 2(1-t)t * P1 + t^2 *P2
   #define BEZIER(p0, p1, p2, t) \
      (((1.0-(t))*(1.0-(t))*(p0))+(2.0*(1.0-(t))*(t)*(p1)) + ((t)*(t)*(p2)))

   // ball path for hoop +x
   double t0 = shotParamT[0];
   double bx0 = BEZIER(start0_x, peak0_x, end0_x, t0);
   double by0 = BEZIER(start0_y, peak_y, end0_y, t0);
   double bz0 = BEZIER(start0_z, peak_z, end0_z, t0);
   basketball(bx0, by0, bz0, ballRadius, 0.0);

   // ball path for hoop -x
   double t1 = shotParamT[1];
   double bx1 = BEZIER(start1_x, peak1_x, end1_x, t1);
   double by1 = BEZIER(start1_y, peak_y, end1_y, t1);
   double bz1 = BEZIER(start1_z, peak_z, end1_z, t1);
   basketball(bx1, by1, bz1, ballRadius, 0.0);

   #undef BEZIER

   // AROUND THE COURT ACCESSORIES
   // benches, seating, scorer's table
   const double benchY = 0.10; // walkway height
   const double offCourtFeet = 5.0; // 5 ft off the sideline
   const double chairSpacingFeet = 2.2; // just over 2ft btwn center of chairs
   const double chairScale = 0.30;

   // Near sideline, -Z, two 12-chair team benches facing +Z
   drawBenchRow(12, courtLenHalfX, courtWidHalfZ, +1, -1, benchY, offCourtFeet, chairSpacingFeet, chairScale, 0.0);
   drawBenchRow(12, courtLenHalfX, courtWidHalfZ, -1, -1, benchY, offCourtFeet, chairSpacingFeet, chairScale, 0.0);

   // DScorer's table between benches 
   drawScorersTableWithChairs(courtLenHalfX, courtWidHalfZ, benchY, chairScale);

   // Far sideline, +Z, chairs 
   const int sidelineChairCount = 21; // chairs from each baseline toward midcourt
   const double yawFacingFar = 180.0; // face toward −Z, direction of court
   const int sidelineFarPosZ= +1; // far side

   // Right baseline (+X) toward center
   drawBenchRow(sidelineChairCount, courtLenHalfX, courtWidHalfZ, +1, sidelineFarPosZ, benchY, offCourtFeet, chairSpacingFeet, chairScale, yawFacingFar);
   // Left baseline (−X) toward center
   drawBenchRow(sidelineChairCount, courtLenHalfX, courtWidHalfZ, -1, sidelineFarPosZ, benchY, offCourtFeet, chairSpacingFeet, chairScale, yawFacingFar);
   // SECOND ROW OF CHAIRS (behind first)
   const double secondRowOffsetFeet = 3.0; // 2 ft behind first
   // Right baseline (+X) toward center
   drawBenchRow(sidelineChairCount, courtLenHalfX, courtWidHalfZ, +1, sidelineFarPosZ, benchY+0.1, offCourtFeet + secondRowOffsetFeet, chairSpacingFeet, chairScale, yawFacingFar);
   // Left baseline (−X) toward center
   drawBenchRow(sidelineChairCount, courtLenHalfX, courtWidHalfZ, -1, sidelineFarPosZ, benchY+0.1, offCourtFeet + secondRowOffsetFeet, chairSpacingFeet, chairScale, yawFacingFar);
   // Riser for 2nd row of chairs 
   drawFarSidelineSecondRowRiser(courtLenHalfX, courtWidHalfZ, benchY, offCourtFeet, secondRowOffsetFeet, sidelineFarPosZ);   
   /// Arena bowl walls + fake crowd planes
   drawArenaBowlAndCrowd(courtLenHalfX, courtWidHalfZ, benchY, offCourtFeet, secondRowOffsetFeet);

   // Drawing coolers on sidelines behind benches 
   double coolerInFromBaselineFeet = 20.0;
   double coolerBehindBenchFeet = 2.5;
   double coolerHeight = benchY+0.25;
   double coolerInFromBaseline = coolerInFromBaselineFeet * UNITS_PER_FOOT;
   double coolerBehindBench = coolerBehindBenchFeet * UNITS_PER_FOOT;
   double coolerZ = -(courtWidHalfZ + offCourtFeet * UNITS_PER_FOOT + coolerBehindBench);

   // X positions for the four coolers, 2 behind each bench
   // Right bench, +X, away bench
   double rightCoolerX1 = courtLenHalfX - coolerInFromBaseline;
   double rightCoolerX2 = courtLenHalfX - coolerInFromBaseline - 0.25;

   // Left bench, -X side, home bench
   double leftCoolerX1 = -courtLenHalfX + coolerInFromBaseline;
   double leftCoolerX2 = -courtLenHalfX + coolerInFromBaseline + 0.25;

   // Table centers, one per pair of coolers
   double rightTableX = 0.5 * (rightCoolerX1 + rightCoolerX2);
   double leftTableX  = 0.5 * (leftCoolerX1+ leftCoolerX2);
   double tableFloorY = benchY; // legs rest on the bench walkway
   double tableZ = coolerZ;  // same Z as coolers

   // Draw tables first so coolers appear to sit on them
   drawCoolerTable(rightTableX, tableFloorY, tableZ);
   drawCoolerTable(leftTableX,  tableFloorY, tableZ);

   // Coolers themselves 
   drawGatoradeCooler(rightCoolerX1, coolerHeight, coolerZ, 0.5);
   drawGatoradeCooler(rightCoolerX2, coolerHeight, coolerZ, 0.5);

   drawGatoradeCooler(leftCoolerX1,  coolerHeight, coolerZ, 0.5);
   drawGatoradeCooler(leftCoolerX2,  coolerHeight, coolerZ, 0.5);

   // Adding center-hung score board
   double sbX = 0.0;
   double sbY = 7.5;
   double sbZ = 0.0;
   drawCompleteScoreboard(sbX, sbY, sbZ);

   drawCenterLogo();

   glPopMatrix();
}

// INTRODUCING DIFFERENT LIGHTING MODES
// Turn off all lights so switching modes doesn't bleed into each other 
void killAllLights(void)
{
   for (int i = 0; i < 8; ++i)
      glDisable(GL_LIGHT0 + i);
}

// Fuction to handle all lighitng
// 0: standard for de-bugging, 1: game lighting, 2: pre-game spotlights
void setupLighting(void)
{
   glShadeModel(GL_SMOOTH);

   // Global light toggle
   if (!light)
   {
      killAllLights();
      glDisable(GL_LIGHTING);
      return;
   }

   float Ambient[4] = {0.01f*ambient ,0.01f*ambient ,0.01f*ambient ,1.0f};
   float Diffuse[4] = {0.01f*diffuse ,0.01f*diffuse ,0.01f*diffuse ,1.0f};
   float Specular[4] = {0.01f*specular,0.01f*specular,0.01f*specular,1.0f};

   glEnable(GL_NORMALIZE);
   glEnable(GL_LIGHTING);
   glEnable(GL_COLOR_MATERIAL);
   glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);

   // Clean slate each frame so modes don’t bleed into each other
   killAllLights();

   switch (lightingMode)
   {
      case 0: // warm-up mode
      {
         const float ySpot = 12.0f;  // height of the fixtures
         const float beamRadius = 2.0f; // radius of the sweep on the floor

         // Centers of the two free-throw circles in world space (x,z)
         const float spotCenterX[2] = {-10.0f, 10.0f};
         const float spotCenterZ[2] = {0.0f, 0.0f};

         const float spotCutoff = 20.0f; // angle of light - narrow cone
         const float spotExponent = 5.0f; // strong beam since intensity drops off fast outside of center

         // no distance fall off at all so circle always appear on floor
         const float attC_spot = 1.0f; // constant
         const float attL_spot = 0.0f; // linear 
         const float attQ_spot = 0.0f; // quadratic

         // Spin them a bit faster
         float baseAngle = light_zh * 2;

         for (int i = 0; i < 2; ++i)
         {
            int lid = GL_LIGHT0 + i;
            float cx = spotCenterX[i];
            float cz = spotCenterZ[i];

            // fixture above ft-line. w=1.0 since fixture not inf sun
            float Pos[4] = {cx, ySpot, cz, 1.0f}; 

            // cur pos
            float aDeg = baseAngle + (i * 180.0f); // dif starting poisiton
            // center of beam location on the floor 
            float floorX = cx + beamRadius * Cos(aDeg);
            float floorZ = cz + beamRadius * Sin(aDeg);

            // must be 4-element dir array in OpenGL 2.0
            float Dir[4] = {floorX - Pos[0], -Pos[1], floorZ - Pos[2], 1.0f};

            glLightfv(lid,GL_AMBIENT, Ambient);
            glLightfv(lid,GL_DIFFUSE, Diffuse);
            glLightfv(lid,GL_SPECULAR, Specular);
            glLightfv(lid,GL_POSITION, Pos);
            glLightfv(lid,GL_SPOT_DIRECTION, Dir);

            glLightf(lid,GL_SPOT_CUTOFF, spotCutoff);
            glLightf(lid,GL_SPOT_EXPONENT, spotExponent);

            glLightf(lid,GL_CONSTANT_ATTENUATION, attC_spot);
            glLightf(lid,GL_LINEAR_ATTENUATION, attL_spot);
            glLightf(lid,GL_QUADRATIC_ATTENUATION, attQ_spot);

            glEnable(lid);
         }

         drawBallRack(-6.0,0.0,-0.75);
         drawBallRack(6.0, 0.0,0.75);

         break;
      }
      // Game lighting, focus on court. Not much lighting into the stands
      case 1:
      {
         // Rough footprint of the court in world units
         const float courtHalfX = 9.4f;   // along length
         const float courtHalfZ = 5.0f;   // along width
         const float yArena = 12.0f;  // main lighting truss

         // Attenuation set so that the court stands out and stands are dimly lit
         // distance fall off since all lights pointed towards center
         const float attC = 1.0f;
         const float attL = 0.05f;
         const float attQ = 0.005f;

         // Wide cones
         const float spotCutoff = 67.0f;
         const float spotExponent = 1.5f; // softer edges, less of a cutoff

#define SETUP_ARENA_SPOT(idx, POSX, POSY, POSZ)                            \
         do {                                                              \
            float Pos[4] = {(POSX), (POSY), (POSZ), 1.0f};                \
            float Dir[4] = {-Pos[0], -Pos[1], -Pos[2], 1.0f};                   \
            glLightfv(GL_LIGHT##idx, GL_AMBIENT, Ambient);                  \
            glLightfv(GL_LIGHT##idx, GL_DIFFUSE, Diffuse);                  \
            glLightfv(GL_LIGHT##idx, GL_SPECULAR,Specular);                 \
            glLightfv(GL_LIGHT##idx, GL_POSITION,Pos);                      \
            glLightfv(GL_LIGHT##idx, GL_SPOT_DIRECTION,Dir);                \
            glLightf(GL_LIGHT##idx, GL_SPOT_CUTOFF, spotCutoff);          \
            glLightf(GL_LIGHT##idx, GL_SPOT_EXPONENT,spotExponent);        \
            glLightf(GL_LIGHT##idx, GL_CONSTANT_ATTENUATION, attC);       \
            glLightf(GL_LIGHT##idx, GL_LINEAR_ATTENUATION, attL);       \
            glLightf(GL_LIGHT##idx, GL_QUADRATIC_ATTENUATION, attQ);       \
            glEnable(GL_LIGHT##idx);                                       \
         } while(0)

         // One spot light from each corner of the court 
         SETUP_ARENA_SPOT(0, -courtHalfX, yArena, courtHalfZ-1.0);
         SETUP_ARENA_SPOT(1, courtHalfX, yArena, courtHalfZ-1.0);
         SETUP_ARENA_SPOT(2, -courtHalfX, yArena, -courtHalfZ+1.0);
         SETUP_ARENA_SPOT(3, courtHalfX, yArena, -courtHalfZ+1.0);

#undef SETUP_ARENA_SPOT
         break;
      }
      // pre-game spinning spotlights 

      default:
         break;
   }
}

/*
 *  OpenGL (GLUT) calls this routine to display the scene
 */
void display(void)
{
   /* ================= FRAME SETUP =================
    * Clear buffers, reset transforms, and get the camera where it needs to be.
    */
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glEnable(GL_DEPTH_TEST);
   glLoadIdentity();

   /* --- Camera selection: spin-around, perspective, or FP view --- */
   if (viewMode == 0)  // Orthogonal “spin around the origin”
   {
      glRotatef(ph,1,0,0);
      glRotatef(th,0,1,0);
   }
   else if (viewMode == 1)  // Perspective camera looking at the center
   {
      double Ex = -2*dim*Sin(th)*Cos(ph);
      double Ey = +2*dim*Sin(ph);
      double Ez = +2*dim*Cos(th)*Cos(ph);
      gluLookAt(Ex,Ey,Ez , 0,0,0 , 0,Cos(ph),0);
   }
   else  // viewMode == 2, FP-POV for walking around the area 
   {
      double lookX = eyeX + Cos(camYaw) * Cos(camPitch);
      double lookY = eyeY + Sin(camPitch);
      double lookZ = eyeZ + Sin(camYaw) * Cos(camPitch);
      gluLookAt(eyeX, eyeY, eyeZ,
                lookX, lookY, lookZ,
                0, 5, 0);
   }

   // Lighting handled on its own given the multiple lighting modes 
   setupLighting();
   drawCompleteBasketballCourt(0, 0, 0, 1.5);
   
   ErrCheck("display");
   glFlush();
   glutSwapBuffers();
}

/*
 *  GLUT calls this routine when an arrow key is pressed
 */
void special(int key,int x,int y)
{
   /* ================= ARROW KEYS =================
    * - In FP mode: look around (yaw/pitch).
    * - In orbital views: spin the camera around the origin.
    */
   if (viewMode == 2)
   {
      /* First-person camera: arrows = head movement */
      float rotate_speed = 5.0f; // rotation speed when walking around

      if (key == GLUT_KEY_RIGHT)      camYaw   += rotate_speed;
      else if (key == GLUT_KEY_LEFT)  camYaw   -= rotate_speed;
      else if (key == GLUT_KEY_UP)    camPitch += rotate_speed;
      else if (key == GLUT_KEY_DOWN)  camPitch -= rotate_speed;

      /* Clamp vertical pitch so we don’t break our neck and flip the view */
      if (camPitch >  89.0f) camPitch =  89.0f;
      if (camPitch < -89.0f) camPitch = -89.0f;
   }
   else
   {
      /* Orbital camera: spin around the whole scene */
      if (key == GLUT_KEY_RIGHT)      th -= 5;
      else if (key == GLUT_KEY_LEFT)  th += 5;
      else if (key == GLUT_KEY_UP)    ph += 5;
      else if (key == GLUT_KEY_DOWN)  ph -= 5;
   }

   /* Keep the orbital angles wrapped into something sane */
   th %= 360;
   ph %= 360;

   // Recompute projection + redraw
   Project();
   glutPostRedisplay();
}

// Glut calls this when a key is pressed
void key(unsigned char ch,int x,int y)
{
   // GLOBAL CONTROLS
   if (ch == 27)  /* ESC = bail out */
   {
      exit(0);
   }
   else if (ch == '0')
   {
      /* Reset orbital camera angles */
      th = 0;
      ph = 0;
   }
   else if (ch == 'v' || ch == 'V')
   {
      /* Cycle view mode: ortho → perspective → FP → back around */
      viewMode = (viewMode+1) % 3;
      if (viewMode == 2)
         eyeY = 1.0;  /* put FP camera at roughly eye height */
   }

   // LIGHTING HOTKEYS
   else if (ch == 'l' || ch == 'L')
   {
      /* Master light switch: everything on/off */
      light = 1 - light;
   }
   else if (ch == 'm' || ch == 'M')
   {
      /* Move = animate the light angle for the orbiting mode (mode 0) */
      move = 1 - move;
   }
   else if (ch == 'k' || ch == 'K')
   {
      // flip between lighting modes
      lightingMode = (lightingMode + 1) % 2;
   }
   else if (ch=='1') // shoot on home team hoop
   {
      shotAnimating[1] = 1;
      shotStartTime[1] = glutGet(GLUT_ELAPSED_TIME)/1000.0;
      shotSwishTriggered[1] = 0;
   }
   else if (ch=='2') // shoot on away team hoop
   {
      shotAnimating[0] = 1;
      shotStartTime[0] = glutGet(GLUT_ELAPSED_TIME)/1000.0;
      shotSwishTriggered[0] = 0;
   }
   else if(ch=='`'||ch=='~')
   {
      score[0] = 0;
      score[1] = 0;
   }
   else if(ch=='j'||ch=='J')
   {
      currentVideoFrame=(currentVideoFrame+1)%6;
   }

   // Translate shininess power to actual OpenGL value
   shiny = shininess < 0 ? 0 : pow(2.0, shininess);

   // Mode specifice controls 
   if (viewMode == 2)
   {
      // FP-pov 
      axes = 0;  // Hide axes in FP mode so 'a' is free for movement visually

      float  speed    = 0.35f;
      double forwardX = Cos(camYaw) * speed;
      double forwardZ = Sin(camYaw) * speed;

      if (ch == 'w') { eyeX += forwardX; eyeZ += forwardZ; }  // forward 
      if (ch == 's') { eyeX -= forwardX; eyeZ -= forwardZ; }  // backward 
      if (ch == 'a') { eyeX += forwardZ; eyeZ -= forwardX; }  // strafe left 
      if (ch == 'd') { eyeX -= forwardZ; eyeZ += forwardX; }  // strafe right
   }
   else
   {
      /* Orbital cameras: zoom + axes toggle */
      if (ch == 'a' || ch == 'A')
         axes = 1 - axes;  /* show/hide axes */

      if (ch == '+' || ch == '=')
         dim += 0.2;

      if (ch == '-' || ch == '_')
         dim -= 0.2;
   }

   /* Recompute projection + redraw */
   Project();
   glutPostRedisplay();
}

/*
 *  GLUT calls this routine when the window is resized
 */
void reshape(int width,int height)
{
   // Ratio of the width to th eheight of the window
   asp = (height>0) ? (double)width/height : 1;
   // Set the viewport to the entire window
   glViewport(0,0, width,height);
   // Set projection 
   Project();
}

// Mouse state for view angle 
/*
 *  GLUT calls this routine when a mouse button is pressed or released
 */
void mouse(int button, int state, int x, int y)
{
   if (state == GLUT_DOWN)
   {
      mouse_button = button;
      prev_mouse_x = x;
      prev_mouse_y = y;
   }
   else if (state == GLUT_UP)
   {
      // button released
      mouse_button = -1;
   }
}

/*
 *  GLUT calls this routine when the mouse is moved while a button is pressed
 */
void motion(int x, int y)
{
   if (mouse_button != -1) // Mouse being held down
   {
      int dx = x - prev_mouse_x;
      int dy = y - prev_mouse_y;

      if (viewMode == 2) // special cases for FP-pov
      {
         float sensitivity = 0.1f;
         camYaw += dx * sensitivity;
         camPitch -= dy * sensitivity; // reversed since y is top to bottom

         // Prevent screen flipping / weird behavior
         if(camPitch > 89.0f) camPitch = 89.0f;
         if(camPitch < -89.0f) camPitch = -89.0f;
      }
      else
      {
         // Standard logic from last hw3
         th += dx;
         ph += dy;
         th %= 360;
         ph %= 360;
      }

      prev_mouse_x = x;
      prev_mouse_y = y;
      glutPostRedisplay();
   }
}

/*
 *  GLUT calls this routine when there is nothing else to do
 */
void idle()
{
   double t = glutGet(GLUT_ELAPSED_TIME)/1000.0;
   // Animate the plane
   zh = fmod(90*t,360);
   // Animate the light if move is enabled
   if (move) {
      light_zh = fmod(45*t,360);
   }

   // Updating for net swish animations
   const double swishDuration = 0.7;
   for(int i=0; i<2; ++i)
   {
      if(netAnimating[i])
      {
         double runningTime = t - netAnimStart[i];
         if(runningTime>=swishDuration)
         {
            netAnimating[i] = 0;
            netSwayPhase[i] = 0.0;
         } else {
            double swishProgress = runningTime / swishDuration; // 0 to 1 over duration of swish
            // first back, slightly forward then back to rest
            // exact function gotten from chatGPT since wasn't sure how to model back, slightly forward, back to rest
            double swing = -sin(M_PI * swishProgress) * (swishProgress);
            netSwayPhase[i] = swing; 
         }
      }
   }

   // Updating shot animations w/ swish motion
   const double rimHitTime = 0.80; // progress through shot which ball is going through hoop
   for(int i=0; i<2; ++i)
   {
      if(shotAnimating[i])
      {
         double timeProgress = t - shotStartTime[i];
         double shotProgress = timeProgress / SHOT_DURATION;

         if(shotProgress>1.0) shotProgress = 1.0;
         
         // swish movement / sound only play once
         if(!shotSwishTriggered[i]&&shotProgress>=rimHitTime)
         {
            netAnimating[i] = 1;
            netAnimStart[i] = t;
            shotSwishTriggered[i] = 1;
            playSwish();
         }
         
         if(timeProgress>=SHOT_DURATION)
         {
            shotAnimating[i] = 0;
            if(score[i]>199) score[i] = 0;
            else score[i]++;
         }

         shotParamT[i] = timeProgress;

      } else {
         shotParamT[i] = 0.0;
      }
   }
   glutPostRedisplay();
}

/*
 *  Start up GLUT and tell it what to do
 */
int main(int argc,char* argv[])
{
   //  Initialize GLUT and process user parameters
   glutInit(&argc,argv);
   //  Request double buffered, true color window with Z buffering at 600x600
   glutInitWindowSize(800,800);
   glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
   //  Create the window
   glutCreateWindow("Kevin McMahon - Project");
   // enabling texture globally 
   glEnable(GL_TEXTURE_2D);
   // safe row alignment for reading bmp files
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   // Loading textures
   texWood = LoadTexBMP("textures/WoodFloor2.bmp");
   // Make sure that the texture interacts w lighting (don't replace fragment color)
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   // Wrap texture so it tiles instead of stretches
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   // Use linear filter when image is smaller than texture
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   // Use linear filter when image is larger than texture
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   texBasketball = LoadTexBMP("textures/basketballLeather.bmp");
   texFuselage = LoadTexBMP("textures/airplane.bmp");
   texCanopy = LoadTexBMP("textures/texCanopy.bmp");
   texWings = LoadTexBMP("textures/airplane_wings.bmp");
   texFire = LoadTexBMP("textures/fire.bmp");
   texPole = LoadTexBMP("textures/HoopPoleTex.bmp");
   texLebronDunk = LoadTexBMP("textures/lebronDunk.bmp");
   texScorersTablePoster = LoadTexBMP("textures/scorersTablePoster.bmp");
   texCuLogo = LoadTexBMP("textures/ColoradoLogo.bmp");
   texSportsFans = LoadTexBMP("textures/SportsArenaFans.bmp");
   texGatoradeLogo = LoadTexBMP("textures/gatoradeLogo.bmp");
   texCoolerLid = LoadTexBMP("textures/whiteGatoradeLogo.bmp");
   texScoreBoardLogo = LoadTexBMP("textures/cuChairLogo.bmp");


   // Video board frames
   texVideoFrames[0] = LoadTexBMP("textures/videoBoard/CSCI5229_VideoBoard1.bmp");
   texVideoFrames[1] = LoadTexBMP("textures/videoBoard/CSCI5229_VideoBoard2.bmp");
   texVideoFrames[2] = LoadTexBMP("textures/videoBoard/CSCI5229_VideoBoard3.bmp");
   texVideoFrames[3] = LoadTexBMP("textures/videoBoard/CSCI5229_VideoBoard4.bmp");
   texVideoFrames[4] = LoadTexBMP("textures/videoBoard/CSCI5229_VideoBoard5.bmp");
   texVideoFrames[5] = LoadTexBMP("textures/videoBoard/CSCI5229_VideoBoard6.bmp");

   // Loading net (32-bits)
   texBasketballNet = LoadTexBMP32("textures/basketballNet.bmp");
   // Center logo is cirlce 
   texCenterLogo = LoadTexBMP32("textures/centerCourtLogo.bmp");
   texBackboard = LoadTexBMP32("textures/transparentBackboard.bmp");
   texColoradoWordmark = LoadTexBMP32("textures/coloradoWordmark.bmp");
   texSidelineLogo = LoadTexBMP32("textures/geometric_mountains.bmp");

#ifdef USEGLEW
   //  Initialize GLEW
   if (glewInit()!=GLEW_OK) Fatal("Error initializing GLEW\n");
#endif
   //  Tell GLUT to call "idle" when there is nothing else to do
   glutIdleFunc(idle);
   //  Tell GLUT to call "display" when the scene should be drawn
   glutDisplayFunc(display);
   //  Tell GLUT to call "reshape" when the window is resized
   glutReshapeFunc(reshape);
   //  Tell GLUT to call "special" when an arrow key is pressed
   glutSpecialFunc(special);
   //  Tell GLUT to call "key" when a key is pressed
   glutKeyboardFunc(key);
   // NEW: Tell GLUT to call "mouse" when a mouse button is pressed
   glutMouseFunc(mouse);
   glutMotionFunc(motion);
   //  Pass control to GLUT so it can interact with the user
   glutMainLoop();
   return 0;
}
