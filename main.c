/*
 *  Kevin McMahon Project - Basketball Arena
 *
 *  Key bindings:
 *  l          Toggles lighting on/off
 *  m          Toggles light movement
 *  z/Z        Decrease/increase ambient light
 *  x/X        Decrease/increase diffuse light
 *  c/C        Decrease/increase specular light
 *  v          Change display mode (Orthogonal, Perspective, First Person)
 *  k.         Change ligting mode (test, game, warm-up)
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
int th = 0;           // Azimuth of view angle
int ph = 30;          // Elevation of view angle
double dim = 15.0;    // Zoom level for orbital views (calibrated to fit full court)

// --- FP-Pov Camera State (for mode 2) ---
double eyeX = 0;      // Camera X position
double eyeY = 0.5;    // Camera Y position (locked to walking height)
double eyeZ = 5;      // Camera Z position
float camYaw = -90.0f;  // Camera horizontal rotation (left/right)
float camPitch = 0.0f;  // Camera vertical rotation (up/down)

// --- Unit conversion ---
static const double UNITS_PER_FOOT = 0.2;  // Court tiles are 0.2 units per foot

// --- Mouse Input State ---
int mouse_button = -1;  // Which mouse button is pressed
int prev_mouse_x = 0;   // Previous mouse X coordinate
int prev_mouse_y = 0;   // Previous mouse Y coordinate

// --- Lighting State ---
/* Lighting mode:
 * 0 = orbiting “test” light
 * 1 = arena-style overhead lights on the court
 * 2 = warm-up lighting
 */
int lightingMode = 0;
int light     = 1;      // Lighting enabled/disabled
int move      = 1;      // Move light
int ambient   = 10;     // Ambient intensity (%)
int diffuse   = 50;     // Diffuse intensity (%)
int specular  = 10;     // Specular intensity (%)
int shininess = 0;      // Shininess (power of two)
float shiny   = 1;      // Shininess (value)
int distance  = 8;      // Light distance
double light_zh = 0;    // Azimuth of the light
float ylight  = 2;      // Elevation of light

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
unsigned int texChairCushion = 0;

// --- Material presets ---
// .. cleaning up codes / explpicitly defining len of arrays to avoid previous issues
static const float MAT_WHITE[4] = {1.0f, 1.0f, 1.0f, 1.0f};
static const float MAT_BLACK[4] = {0.0f, 0.0f, 0.0f, 1.0f};
// basketball materials
static const float MAT_BALL_AMB[4] = {0.22f, 0.22f, 0.22f, 1.0f};
static const float MAT_BALL_DIF[4] = {1.00f, 1.00f, 1.00f, 1.0f};
static const float MAT_BALL_SPEC[4] = {0.02f, 0.02f, 0.02f, 1.0f};
// static const float MAT_NO_SPEC[4] = {0.0f, 0.0f, 0.0f, 1.0f};
// metal materials for hoop
static const float MAT_METAL_AMB[4] = {0.10f, 0.10f, 0.10f, 1.0f};
static const float MAT_METAL_DIF[4] = {0.55f, 0.55f, 0.55f, 1.0f};
// static const float MAT_METAL_SPEC[4] = {0.85f, 0.85f, 0.85f, 1.0f};
// court color unders tex
static const float COLOR_DARK_BROWN[3] = {0.60f, 0.41f, 0.22f};

// Scoreboard state
int scoreHome = 0;
int scoreAway = 0;

// Scorebaord - video board textures 
#define NUM_VIDEO_FRAMES 4
unsigned int texVideoFrames[NUM_VIDEO_FRAMES];
int currentVideoFrame = 0;
double videoTime = 0.0;



//  Cosine and Sine in degrees
#define Cos(x) (cos((x)*3.14159265/180))
#define Sin(x) (sin((x)*3.14159265/180))
void reshape(int width, int height);

// Fcn prototypes for loading textures
unsigned int LoadTexBMP(const char* file);

// Set projection mode - introduced in hw4
void Project()
{
   //  Tell OpenGL we want to manipulate the projection matrix
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity(); // reset
   // view volumn tied to current zoom/state
   double dimW = asp*dim;
   double dimH = dim;
   double dimD = 2.5 * dim; // half-depth... bigger to avoid clipping the plane

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
   *  Convenience routine to output raster text
   *  Use VARARGS to make this more flexible
 */
// #define LEN 8192  //  Maximum length of text string
// void Print(const char* format , ...)
// {
//    char    buf[LEN];
//    char*   ch=buf;
//    va_list args;
//    //  Turn the parameters into a character string
//    va_start(args,format);
//    vsnprintf(buf,LEN,format,args);
//    va_end(args);
//    //  Display the characters one at a time at the current raster position
//    while (*ch)
//       glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,*ch++);
// }

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

/*
 *  Draw vertex in polar coordinates with normal
 *  (Used for the light source visualization)
 */
static void Vertex(double th,double ph)
{
   double x = Sin(th)*Cos(ph);
   double z = Cos(th)*Cos(ph);
   double y =         Sin(ph);
   glNormal3d(x,y,z);
   glVertex3d(x,y,z);
}

// draw ball to represent light position
static void ball(double x,double y,double z,double r)
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


/*
 *  Helper function for SolidPlane ... both inspired by ex13
 */
static void Canopy(double th,double ph)
{
   double x = Sin(th)*Cos(ph);
   double y = Sin(ph);
   double z = Cos(th)*Cos(ph);
   
   glNormal3d(x, y, z);

   // wrap tex once 
   float s = (double)th / 360.0;
   float t = (double)(ph + 90) / 180.0;

   glTexCoord2f(s, t);
   glVertex3d(x, y, z);
}


/*
 *  Draw solid airplane (FROM LIGHTING EXAMPLE)
 *    at (x,y,z)
 *    nose towards (dx,dy,dz)
 *    up towards (ux,uy,uz)
 *    size
 */
static void SolidPlane(double x,double y,double z, double dx,double dy,double dz, double ux,double uy, double uz, double s)
{
   // Add shine since plane is metal
   glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
   glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,MAT_WHITE);
   glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,MAT_BLACK);

   // Dimensions used to size airplane
   const double wid=0.05;
   const double nose=+0.50;
   const double cone= 0.20;
   const double wing= 0.00;
   const double strk=-0.20;
   const double tail=-0.50;
   
   //  Unit vector in direction of flght
   double D0 = sqrt(dx*dx+dy*dy+dz*dz);
   double X0 = dx/D0;
   double Y0 = dy/D0;
   double Z0 = dz/D0;
   
   //  Unit vector in "up" direction
   double D1 = sqrt(ux*ux+uy*uy+uz*uz);
   double X1 = ux/D1;
   double Y1 = uy/D1;
   double Z1 = uz/D1;
   
   //  Cross product gives the third vector
   double X2 = Y0*Z1-Y1*Z0;
   double Y2 = Z0*X1-Z1*X0;
   double Z2 = X0*Y1-X1*Y0;
   
   //  Rotation matrix
   double mat[16];
   mat[0] = X0;   mat[4] = X1;   mat[ 8] = X2;   mat[12] = 0;
   mat[1] = Y0;   mat[5] = Y1;   mat[ 9] = Y2;   mat[13] = 0;
   mat[2] = Z0;   mat[6] = Z1;   mat[10] = Z2;   mat[14] = 0;
   mat[3] =  0;   mat[7] =  0;   mat[11] =  0;   mat[15] = 1;

   //  Save current transforms
   glPushMatrix();
   //  Offset, scale and rotate
   glTranslated(x,y,z);
   glMultMatrixd(mat);
   glScaled(s,s,s);
   
   // ADDING TEXTURE 
   glBindTexture(GL_TEXTURE_2D, texFuselage);
   glEnable(GL_TEXTURE_2D);
   // Keep lighting, but avoid color tint
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glColor3f(1,1,1);

   
   //  Nose
   glBegin(GL_TRIANGLE_FAN);
   glTexCoord2f(0.5f, 0.0f); // collapse tex toward 0 at the tip
   glVertex3d(nose, 0.0, 0.0);
   for (int th=0;th<=360;th+=30) {
      double sTex = (double)th / 360.0; 
      // Normals for a cone are complex, this is a simplification
      glNormal3f(1, Cos(th)*0.2, Sin(th)*0.2);
      glTexCoord2f(sTex, 1.0f);
      glVertex3d(cone,wid*Cos(th),wid*Sin(th));
   }
   glEnd();

   //  Fuselage
   glBegin(GL_QUAD_STRIP);
   for (int th=0;th<=360;th+=30)
   {
      double sTex = (double)th / 360.0; // wrap around once
      glNormal3d(0,Cos(th),Sin(th)); // Normal points outwards from cylinder

      glTexCoord2f(sTex, 0.0f); // near cone in front v=0
      glVertex3d(cone,wid*Cos(th),wid*Sin(th));

      glTexCoord2f(sTex, 1.0f); // tail in back v=1
      glVertex3d(tail,wid*Cos(th),wid*Sin(th));
   }
   glEnd();
   
   // Tailpipe no lighting for glow effect
   glDisable(GL_LIGHTING);
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texFire);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glColor3f(1,0.8,0);
   glBegin(GL_TRIANGLE_FAN);
   glTexCoord2f(0.5f, 0.5f); // map center vertex to center of texture
   glVertex3f(tail, 0.0, 0.0);
   for (int th=0;th<=360;th+=30){
      float s = 0.5f + 0.5f*Cos(th); // map outer vertices to edge of texture
      float t = 0.5f + 0.5f*Sin(th);
      glTexCoord2f(s, t);
      glVertex3d(tail,wid*Cos(th),wid*Sin(th));
   }
   glEnd();
   if (light) glEnable(GL_LIGHTING);

   //  Canopy
   glPushMatrix();
   glTranslated(0.15,wid,0);
   glScaled(0.05,0.03,0.03);

   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texCanopy);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glColor3f(1,1,1);
   for (int ph=-30;ph<90;ph+=30)
   {
      glBegin(GL_QUAD_STRIP);
      for (int th=0;th<=360;th+=30)
      {
         Canopy(th,ph);
         Canopy(th,ph+30);
      }
      glEnd();
   }
   glDisable(GL_TEXTURE_2D);
   glPopMatrix();

   
   //  Wings
   glEnable(GL_TEXTURE_2D); // enable texture again
   glBindTexture(GL_TEXTURE_2D, texWings);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glColor3f(1,1,1); // no texture tint
   //  Enable two-sided lighting for flat objects like wings
   glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,1);
   glBegin(GL_TRIANGLES);
   glNormal3f(0,1,0); // Normal points up
   // Right wing
   glTexCoord2f(0.0f, 0.0f);
   glVertex3d(wing, 0.0, wid);
   glTexCoord2f(1.0f, 0.0f);
   glVertex3d(tail, 0.0, wid);
   glTexCoord2f(1.0f, 1.0f);
   glVertex3d(tail, 0.0, 0.5);
   // Left wing
   glTexCoord2f(0.0f, 0.0f);
   glVertex3d(wing, 0.0,-wid);
   glTexCoord2f(1.0f, 0.0f);
   glVertex3d(tail, 0.0,-wid);
   glTexCoord2f(1.0f, 1.0f);
   glVertex3d(tail, 0.0,-0.5);
   glEnd();

   //  Vertical tail
   glColor3f(1,1,1);
   glBegin(GL_TRIANGLES);
   glNormal3f(0,0,1); // Normal points sideways
   glTexCoord2f(0.0f, 0.0f);
   glVertex3d(strk, wid, 0.0);
   glTexCoord2f(1.0f, 0.0f);
   glVertex3d(tail, 0.3, 0.0);
   glTexCoord2f(0.0f, 1.0f);
   glVertex3d(tail, wid, 0.0);
   glEnd();

   //  Disable two-sided lighting
   glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,0);

   //  Undo transformations
   glPopMatrix();
}

// Creating "Torus" for backetball hoop and rings on the basketball
// Updated in hw5 to include normals for lighting
static void drawTorus(double majorRadius, double minorRadius, int majorSegments, int minorSegments)
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
static void drawCylinder(double radius, double height, int segments)
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
static void drawSolidSphereTextured(double radius)
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

// Draw a wavy seam band on a sphere of radius r.
// WORK IN PROGRESS - needs to be refined to actually look like basketball
// but more realistic than what was used before
static void drawSeamBand(double r, double ampDeg, double widthDeg, int segs, double phaseDeg, double lift)
{
   double dth = 360.0 / segs;
   double rLift = r * (1.0 + lift);
   glBegin(GL_TRIANGLE_STRIP);
   for (double th = 0.0; th <= 360.0; th += dth)
   {
      // Center latitude for this longitude
      double phiC = ampDeg * Sin(2.0 * (th + phaseDeg)); // degrees
      // Upper/lower edges of the band (in latitude)
      double phiU = phiC + widthDeg * 0.5;
      double phiL = phiC - widthDeg * 0.5;
      // Precompute trig once per edge
      double s = Sin(th), c = Cos(th);
      double cu = Cos(phiU), su = Sin(phiU);
      double cl = Cos(phiL), sl = Sin(phiL);
      // Upper edge vertex (normals = position on unit sphere)
      glNormal3d(s*cu, su, c*cu);
      glVertex3d(rLift*s*cu, rLift*su, rLift*c*cu);
      // Lower edge vertex
      glNormal3d(s*cl, sl, c*cl);
      glVertex3d(rLift*s*cl, rLift*sl, rLift*c*cl);
   }
   glEnd();
}

static void basketball(double x, double y, double z, double r, double rot)
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
   glColor3f(0.0f, 0.0f, 0.0f);                      // black seams
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, MAT_BALL_SPEC); // lwo spec - not shiny (white appearance) 
   glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);

   // Seam shape params (tweak to taste)
   const double seamAmp  = 18.0;    // waviness from equator (deg)
   const double seamWidth = 3.6;     // band thickness (deg)
   const int seamSegs = 300;     // smoothness
   const double seamLift = 0.0015;  // tiny offset above leather to avoid z-fight

   // Two orthogonal seams 
   drawSeamBand(1.0, seamAmp, seamWidth, seamSegs, 0.0, seamLift);
   drawSeamBand(1.0, seamAmp, seamWidth, seamSegs, 90.0, seamLift);

   glPopMatrix();
}

// Textured cylinder along +Y with UVs that repeat.
// repeatU = repeats around circumference; repeatV = repeats along height.
static void drawTexturedCylinder(unsigned int tex, double radius, double height, int segments, double repeatU, double repeatV)
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

// Drop a cylinder between two points (x1,y1,z1) and (x2,y2,z2)x
static void drawRodBetween(double x1,double y1,double z1, double x2,double y2,double z2, double r,int segs)
{
   double vx  = x2 - x1;
   double vy  = y2 - y1;
   double vz  = z2 - z1;
   double len = sqrt(vx*vx + vy*vy + vz*vz);

   // Rotate (0,1,0) onto v using axis = (0,1,0) x v = (vz,0,-vx)
   double ax =  vz;
   double ay =  0.0;
   double az = -vx;
   double axisLen = sqrt(ax*ax + az*az);
   double angDeg  = 0.0;

   if (axisLen > 1e-6)
   {
      angDeg = acos(vy/len) * 180.0 / M_PI;
      ax    /= axisLen;
      az    /= axisLen;
   }

   glPushMatrix();
   glTranslated(x1,y1,z1);
   if (axisLen > 1e-6)
      glRotated(angDeg, ax, ay, az);

   drawCylinder(r, len, segs); // locally goes y=0 to y=len
   glPopMatrix();
}

// Back panel of the chair
// - centered at (0,0) in X/Y
// - front face at z=0, back at z=-t
// - rounded top corners
static void drawRoundedBackSolid(double w, double h, double t, double cornerR, int arcSegs)
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
static void chair(double x, double y, double z, double dirDeg, double scale)
{
   glPushMatrix();
   glTranslated(x,y,z);
   glRotated(dirDeg,0,1,0);
   glScaled(scale,scale,scale);

   // Chair proportions
   const double seatW   = 1.10;
   const double seatD   = 0.68; //front at z=0, back at z=-seatD 
   const double seatH   = 0.10;
   const double seatY   = 0.58;

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
   const int segs = 16;

   // chair cushion slab
   glColor3f(0.12f,0.12f,0.15f);

   int texWasEnabled = glIsEnabled(GL_TEXTURE_2D);
   glColor3f(1.0f,1.0f,1.0f);   // let texture drive the color
   if(!texWasEnabled)
      glEnable(GL_TEXTURE_2D);
 
   glBindTexture(GL_TEXTURE_2D, texChairCushion);   
   glBegin(GL_QUADS);
   glNormal3f(0,1,0);
   // back-left  
   glTexCoord2f(0.0f, 0.0f); glVertex3d(-seatW*0.5, seatY, -seatD);
   // back-right 
   glTexCoord2f(1.0f, 0.0f); glVertex3d( seatW*0.5, seatY, -seatD);
   // front-right
   glTexCoord2f(1.0f, 1.0f); glVertex3d( seatW*0.5, seatY, 0);
   // front-left
   glTexCoord2f(0.0f, 1.0f); glVertex3d(-seatW*0.5, seatY, 0);
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
static void drawScorersTable(double courtLenHalfX, double courtWidHalfZ, double benchY)
{
   const double distFromEndline = 35.0;  // clear space from each endline to table
   const double tableHeightFeet = 2.0;   // wall height
   const double tableDepthFeet = 2.0;   // front (court) → back (toward benches)
   const double offsetFromSidelineFeet = 2.5;   // offset from sideline toward stands

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
   const double zFrontOuter = -courtWidHalfZ - offsetSideline;  // court-facing front
   const double zBackOuter = zFrontOuter - tableDepth;         // back edge toward benches

   // Thin wall to align texture on sides well... less ugly
   const double wallThickness = 0.03;    // much thinner than before
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

         // --- Textured front face poster 
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

   glEnable(GL_TEXTURE_2D); // re-anbel for rest of the scene
}

static void drawLaptop(double x, double y, double z, double yawDeg, double scale)
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
   const double zFront =  0.0;
   const double hingeZ = zBack + 0.04; // small inset from absolute back

   const double xL = -baseW*0.5;
   const double xR = baseW*0.5;

   // Base sits ON the surface passed in as y (so base bottom is y, not below it)
   const double y0 = 0.0;
   const double y1 = baseH;

   glDisable(GL_TEXTURE_2D);  // we'll selectively turn it on when needed

   // Bottom prism for laptop base
   glColor3f(0.12f,0.12f,0.15f);
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

   // Key board texture (TO FIX)
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texKeyboard);
   glColor3f(1.0f,1.0f,1.0f);  // no tint on the keyboard

   glBegin(GL_QUADS);
   glNormal3f(0,1,0);
   glTexCoord2f(0.0f,0.0f); glVertex3d(xL,y1,zBack);
   glTexCoord2f(1.0f,0.0f); glVertex3d(xR,y1,zBack);
   glTexCoord2f(1.0f,1.0f); glVertex3d(xR,y1,zFront);
   glTexCoord2f(0.0f,1.0f); glVertex3d(xL,y1,zFront);
   glEnd();

   glDisable(GL_TEXTURE_2D);

   // Have a frame for the screen 
   const double screenBottomY = y1;              // sits on top of the base
   const double screenTopY = screenBottomY + screenH;
   const double screenBackZ = hingeZ;          // toward scorers
   const double screenFrontZ = screenBackZ + screenT; // toward court

   const double sxL = -screenW*0.5;
   const double sxR =  screenW*0.5;

   // Frame around the screen
   glColor3f(0.10f,0.10f,0.12f);
   glBegin(GL_QUADS);
   // Back of screen (toward scorers)
   glNormal3f(0,0,-1);
   glVertex3d(sxR,screenBottomY,screenBackZ);
   glVertex3d(sxL,screenBottomY,screenBackZ);
   glVertex3d(sxL,screenTopY,   screenBackZ);
   glVertex3d(sxR,screenTopY,   screenBackZ);

   // Front of screen border (outer ring, untextured)
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
   double vZ  = screenFrontZ + 0.001; // tiny offset to avoid z-fighting
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texLebronDunk);
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
   drawRodBetween(hxL,hy,hz, hxR,hy,hz, 0.03, 10);
   
   // re-enable texture for rest of scene
   glEnable(GL_TEXTURE_2D);
   glPopMatrix();
}

//DRAWING COOLER FOR SIDELINES
// Draw only the SIDE of a truncated cone since sitting on table with lid
// Oriented along +Y so base at y = 0, top at y = height.
static void drawTruncatedConeSide(double baseRadius, double topRadius, double height, int numSlices)
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
// Coordinate frame:
// Body base starts at y = 0
// Body top ends at y = bodyHeight
// Lid sits from y = bodyHeight to y = bodyHeight + lidHeight
static void drawGatoradeCooler(double x, double y, double z, double scale)
{
   // Canonical shape dimensions in the opengl dimensions
   double outerRadius = 0.2; // base radius before scaling
   double bodyHeight = 0.4; // body height before scaling

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

   // Bright orange 
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
   glColor3f(1.0f, 1.0f, 1.0f);   // let the texture drive the color

   glBegin(GL_TRIANGLE_FAN);
   glNormal3d(0.0, 1.0, 0.0);   // facing up
   glTexCoord2f(0.5f, 0.5f);    // texture center
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

static void drawCoolerTable(double centerX, double floorY, double centerZ)
{
   glPushMatrix();
   glTranslated(centerX, floorY, centerZ);
   
   // Coolers sit with base at y = benchY + 0.25
   // So table top should reach 0.25 above floorY
   const double topTopY = 0.25;
   const double topThickness = 0.05;   // thickness of table slab for gatorade texture 
   const double topBottomY = topTopY - topThickness;
   const double legHeight = topBottomY; 
   
   // Footprint of the table 
   const double halfLengthX = 0.25; // along X (left-right)
   const double halfDepthZ   = 0.15;   // along Z (front-back)
   
   // Leg parameters (rods)
   const double legRadius = 0.015;
   const int legSlices = 20;
   double xL = -halfLengthX;
   double xR =  halfLengthX;
   double zF = -halfDepthZ;
   double zB =  halfDepthZ;
   
   // TABLE TOP - rectangular prism
   glDisable(GL_TEXTURE_2D); 
   glColor3f(0.9f, 0.9f, 0.9f); // neutral gray
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
   
   // Front face (good candidate for branding texture later)
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
   glColor3f(0.9f, 0.9f, 0.9f); 
   
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

// Draw the scorer's table with the chairs behind for reporters
// Chairs: one centered, then every 4 feet out along the table span
static void drawScorersTableWithChairs(double courtLenHalfX, double courtWidHalfZ, double benchY, double chairScale)
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

   // Laptop position on table
   const double zLaptop = zBackTable - 0.0075;

   // Chair spacing one then every 4 ft along table in both directions
   const double chairStep = 4.0 * UNITS_PER_FOOT;
   int maxIndex = (int)floor(tableHalfLen / chairStep);

   const double laptopScale = 0.15;  // scale for laptops on global scael

   for (int i = -maxIndex; i <= maxIndex; ++i)
   {
      double x = i * chairStep;
      if (x < xL || x > xR) continue;

      // Scorer chair same bench height facing the court, +z towards the court
      chair(x, benchY, zChairs, 0.0, chairScale);
      // Laptop on the table directly in front of that chair, facing the court as well
      drawLaptop(x, yWoodTop+0.03, zLaptop, 180.0, laptopScale);
   }
}



// DRAWS W.R.T to the court 
// Draw one straight row of courtside chairs, anchored at a baseline and marching toward mid-court along +/−X
// count - num of chairs
// halfCourtX - court length / 2  real world units (ft instead of openGL units)
// halfCourtZ - court width  / 2  real workd units
// baselineSign +1 = right baseline +X for away bench,  -1 = left baseline -X for away bench 
// sidelineSign -1 = near sideline -Z for fan chairs,  +1 = far sideline +Z for team benches
// y - height chair... second row of fan seating
// offsetFeet - distance from sideline in ft
// spacingFeet - distnace from chair to chair (center of chair) in ft 
// chairScale - passed to chair()
// yawDeg - chair facing; pass 0 for near sideline facing +Z, 180 for far
static void drawBenchRow(int count, double halfCourtX, double halfCourtZ, int baselineSign, int sidelineSign,double y, double offsetFeet, double spacingFeet, double chairScale, double yawDeg)
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
static void drawFarSidelineSecondRowRiser(double courtLenHalfX, double courtWidHalfZ, double benchY, double offCourtFeet, double secondRowOffsetFeet, int   sidelineFarPosZ)
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

   // Front face (toward the court)
   {
      float nz = (sidelineFarPosZ > 0) ? -1.0f : 1.0f;
      glNormal3f(0,0,nz);
      glVertex3d(xL,y0,zFront);
      glVertex3d(xR,y0,zFront);
      glVertex3d(xR,y1,zFront);
      glVertex3d(xL,y1,zFront);
   }

   // Back face (toward the upper stands)
   {
      float nz = (sidelineFarPosZ > 0) ? 1.0f : -1.0f;
      glNormal3f(0,0,nz);
      glVertex3d(xR,y0,zBack);
      glVertex3d(xL,y0,zBack);
      glVertex3d(xL,y1,zBack);
      glVertex3d(xR,y1,zBack);
   }

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



// Draws a complete Basketball Hoop object with a solid backboard
// have backboard depth to be more realistic and not have the light
// flow through it in hw5
static void basketballHoop(double x, double y, double z, double s, double rot, double poleSetbackWorld)
{
   glPushMatrix();
   glTranslated(x, y, z);
   glRotated(rot, 0, 1, 0);
   glScaled(s, s, s);

   // tunables
   const double unitsPerFoot = UNITS_PER_FOOT;
   const double invScale     = (s != 0.0) ? 1.0 / s : 0.0;

   // Real-world target dimensions (feet) so that it is to scale
   const double boardWidthFeet      = 6.0;
   const double boardHeightFeet     = 3.5;
   const double boardThicknessFeet  = 0.167;   // ~2 inches
   const double rimRadiusFeet       = 0.75;    // 18 in diameter
   const double rimTubeFeet         = 0.0625;  // ~0.75 in bar thickness
   const double rimHeightFeet       = 10.0;
   const double boardBottomFeet     = rimHeightFeet - 0.9; // bottom of backboard
   const double poleRadiusFeet      = 0.25;    // 6 in diameter support pole
   const double poleHeightFeet      = 10.2;
   const double bracketLengthFeet   = 2.0;     // extension arm from pole to board

   const double bbThick  = (boardThicknessFeet * unitsPerFoot) * invScale;
   const double bbWidth  = (boardWidthFeet     * unitsPerFoot) * invScale;
   const double bbHeight = (boardHeightFeet    * unitsPerFoot) * invScale;

   const double rimMajor = (rimRadiusFeet * unitsPerFoot) * invScale;
   const double rimMinor = (rimTubeFeet   * unitsPerFoot) * invScale;
   const double rimY     = (rimHeightFeet * unitsPerFoot) * invScale;

   // Pole textured metalllic cylinder
   const double poleRadius = (poleRadiusFeet * unitsPerFoot) * invScale;
   const double poleHeight = (poleHeightFeet * unitsPerFoot) * invScale;

   // Backboard offset from the pole
   const double bbOffsetWorld = (poleRadiusFeet + bracketLengthFeet) * unitsPerFoot + 0.5 * (boardThicknessFeet * unitsPerFoot);
   const double bbOffset = bbOffsetWorld * invScale;
   const double bbBase = (boardBottomFeet * unitsPerFoot) * invScale;

   // Metal material subtle diffuse, strong specular, higher shininess
   glDisable(GL_COLOR_MATERIAL);  // set material explicitly
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT , MAT_METAL_AMB);
   glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE , MAT_METAL_DIF);
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, MAT_METAL_DIF);
   glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 64.0f);

   // Don’t tint the texture
   glColor3f(1,1,1);

   const double poleSetbackLocal = poleSetbackWorld * invScale;

   glPushMatrix();
   glTranslated(0.0, 0.0, -poleSetbackLocal);
   // Wrap the texture fully around (U=1) and repeat 3× vertically 
   drawTexturedCylinder(texPole, poleRadius, poleHeight, 48, 1.0, 3.0);
   glPopMatrix();

   glEnable(GL_COLOR_MATERIAL);


   const double yArm = rimY;
   const double zStart = poleRadius - poleSetbackLocal;                   // from pole surface
   const double zBack = bbOffset - bbThick*0.5; // back face of board
   double armLen = zBack - zStart;
   if (armLen < 0.0) armLen = 0.0;
   glColor3f(1,1,1);
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

   // Texture on the front
   glBindTexture(GL_TEXTURE_2D, texBackboard);
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

   glColor3f(1,1,1);
   glBegin(GL_QUADS);
   glNormal3f(0,0,1);
   glTexCoord2f(0.0f,0.0f); glVertex3d(bx1, by1, bz2);
   glTexCoord2f(1.0f,0.0f); glVertex3d(bx2, by1, bz2);
   glTexCoord2f(1.0f,1.0f); glVertex3d(bx2, by2, bz2);
   glTexCoord2f(0.0f,1.0f); glVertex3d(bx1, by2, bz2);
   glEnd();
   glDisable(GL_TEXTURE_2D);

   // Other faces (solid white, lit)
   glColor3f(1,1,1);
   glBegin(GL_QUADS);
   // Back
   glNormal3f(0,0,-1);
   glVertex3d(bx2, by1, bz1); glVertex3d(bx1, by1, bz1);
   glVertex3d(bx1, by2, bz1); glVertex3d(bx2, by2, bz1);
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
   const double bbFrontZ = bbOffset + bbThick*0.5;            // = 0.15 + 0.025 = 0.175
   const double rimZ = bbFrontZ + rimMajor + rimMinor;        // touches board

   glColor3f(1,0,0);
   glPushMatrix();
   glTranslated(0, rimY, rimZ);
   glRotatef(90, 1, 0, 0); // axis → Y (horizontal rim)
   drawTorus(rimMajor, rimMinor, 64, 24); // smoother & thinner

   glPopMatrix();
   glPopMatrix(); // end hoop assembly
}


// Drawing a checkered basketball court similar to the Boston Celtics
// Next three functions do not implement lightning since they are court markings
// Feet-aware circle: radius is given in FEET; we convert to world units
static void drawCircleFeet(double cx, double cz, double radiusFeet, int segments, double y, double feetPerX, double feetPerZ)
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
static void drawSemicircleTowardCenterFeet(double cx, double cz, double radiusFeet, int segments, double y, double feetPerX, double feetPerZ)
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
static void drawCourtEndFeet(double court_width, double y, double feetPerX, double feetPerZ, double keyWidthFeet, double ftFromBaselineFeet, double circleRadiusFeet)
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

   /// Free-throw semicircle (6' radius)... opening towards the baseline. Leave 2ft on sides for larger lane
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
   const double dx_feet = (ft_apex_x - hoop_cx) / feetPerX; // dz_feet = 0
   const double threePtRadiusFeet = fabs(dx_feet);

   // Draw the 3-pt semicircle (opening inward)
   drawSemicircleTowardCenterFeet(hoop_cx, hoop_cz, threePtRadiusFeet, 128, y, feetPerX, feetPerZ);

   // Corner connectors: from the ends of the 3-pt arc to the baseline parallel to the sideline (constant z, increasing x)
   const double end_x = hoop_cx;
   const double end_z_1 = hoop_cz - threePtRadiusFeet * feetPerZ; // lower (−z)
   const double end_z_2 = hoop_cz + threePtRadiusFeet * feetPerZ; // upper (+z)

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
static void drawCourtMarkings(double court_width, double court_depth)
{
   // Real dimensions easier to work with for scale
   const double KEY_WIDTH_FEET = 16.0; // lane width nba/ncaa
   const double FT_FROM_BASELINE_FEET = 19.0; // baseline -> FT line
   const double CIRCLE_RADIUS_FEET = 6.0;  // center & FT circle radius (12' diameter)

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

   // Center circle (6' radius)
   drawCircleFeet(0.0, 0.0, CIRCLE_RADIUS_FEET, 96, y, feetPerX, feetPerZ);
   // One basket end (at +x)
   drawCourtEndFeet(court_width, y, feetPerX, feetPerZ, KEY_WIDTH_FEET, FT_FROM_BASELINE_FEET, CIRCLE_RADIUS_FEET);

   // Mirror for the opposite end (at -x)
   glPushMatrix();
   glScalef(-1, 1, 1);
   drawCourtEndFeet(court_width, y, feetPerX, feetPerZ, KEY_WIDTH_FEET, FT_FROM_BASELINE_FEET, CIRCLE_RADIUS_FEET);
   glPopMatrix();

   glLineWidth(1.0);
}

// Draws checkerboard floor with normals for lighting (all pointed in +y)
// checkerboard w/ small squares required for the spot light effect
static void drawCheckerboard(int rows, int cols, double tileSize)
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
   //Settings number of times for texture to repeat across the court
   const double texRepeatU = 8.0; // Repeat 4 times across width
   const double texRepeatV = 4.0; // Repeat 4 times across depth

   // lifted to y = court_height
   glBegin(GL_QUADS);
   glNormal3f(0, 1, 0); // Normal for the top is UP
   
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
   glColor3fv(COLOR_DARK_BROWN);
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
static void drawBasketballCourt(int rows, int cols, double tileSize)
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

   double innerLoop[5][2] = {
      {-innerHalfX, -innerHalfZ},
      { innerHalfX, -innerHalfZ},
      { innerHalfX,  innerHalfZ},
      {-innerHalfX,  innerHalfZ},
      {-innerHalfX, -innerHalfZ}
   };
   double midLoop[5][2] = {
      {-midHalfX, -midHalfZ},
      { midHalfX, -midHalfZ},
      { midHalfX,  midHalfZ},
      {-midHalfX,  midHalfZ},
      {-midHalfX, -midHalfZ}
   };
   double outerLoop[5][2] = {
      {-outerHalfX, -outerHalfZ},
      { outerHalfX, -outerHalfZ},
      { outerHalfX,  outerHalfZ},
      {-outerHalfX,  outerHalfZ},
      {-outerHalfX, -outerHalfZ}
   };

   glDisable(GL_TEXTURE_2D);

   // Courtside padding first 5 ft in dark grey 
   glColor3f(0.25f, 0.25f, 0.25f);
   glBegin(GL_QUAD_STRIP);
   glNormal3f(0, 1, 0);
   for (int i = 0; i < 5; ++i)
   {
      glVertex3d(midLoop[i][0], walkwayY, midLoop[i][1]);
      glVertex3d(innerLoop[i][0], walkwayY, innerLoop[i][1]);
   }
   glEnd();

   // Remaining apron in grey
   glColor3f(0.35f, 0.35f, 0.38f);
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

   glEnable(GL_POLYGON_OFFSET_LINE);
   glPolygonOffset(-1.0, -1.0); // Negative values pull the lines toward the camera

   // Disable lighting for the lines so always white
   glDisable(GL_LIGHTING);
   // Disable texturing for the lines
   glDisable(GL_TEXTURE_2D);
   
   drawCourtMarkings(cols * tileSize, rows * tileSize);

   // Re-enable lighting to be on for rest of scene
   if (light) glEnable(GL_LIGHTING);
   glDisable(GL_POLYGON_OFFSET_LINE);
}


// Generic 4-wall shell around a rectangle defined by inner X/Z.
// u define inner bounds, vertical span, thickness, and color;
// it builds the full rectangular "ring" (inner + outer faces, edges, top).
static void drawWallShellCore(double xInnerL, double xInnerR, double zInnerF, double zInnerB, double y0, double y1, double wallThickness, float r, float g, float b)
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

static void drawArenaBoundaryWalls(double courtLenHalfX, double courtWidHalfZ, double baseY)
{
   // Must match the walkway drew in drawBasketballCourt
   const double walkwayFeet = 20.0;   // feet from court to end of walkway
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
static void drawLowerBowlWalls(double courtLenHalfX, double courtWidHalfZ, double baseY, double offCourtFeet, double secondRowOffsetFeet)
{
   // How far from the court is the second row of seats?
   const double seatRadialFeet = offCourtFeet + secondRowOffsetFeet;

   // How far behind the riser do we put the low wall (in feet)?
   const double behindRiserFeet = 2.0; 

   const double radialFeet = seatRadialFeet + behindRiserFeet;
   const double wallHeightFeet = 2.5;    // short wall / rail height
   const double wallThickness = 0.20;   // a bit thinner than the big shell

   const double wallHeight = wallHeightFeet * UNITS_PER_FOOT;
   const double offset = radialFeet * UNITS_PER_FOOT;

   // Inner face of this low wall relative to the playing surface
   const double xInnerL = -(courtLenHalfX + offset);
   const double xInnerR =  (courtLenHalfX + offset);
   const double zInnerF = -(courtWidHalfZ + offset);
   const double zInnerB =  (courtWidHalfZ + offset);

   const double y0 = baseY;
   const double y1 = baseY + wallHeight;

   // Slightly lighter so it reads as "lower bowl rail" vs outer shell
   drawWallShellCore(xInnerL, xInnerR, zInnerF, zInnerB, y0, y1, wallThickness, 0.18f, 0.18f, 0.20f);
}

static void drawBleacherFanPlanes(double courtLenHalfX, double courtWidHalfZ, double baseY, double offCourtFeet, double secondRowOffsetFeet)
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
   const double xLowL = xHighL + deltaOff; // closer to center on left side
   const double xLowR = xHighR - deltaOff; // closer to center on right side
   const double zLowF = zHighF + deltaOff; // closer on near sideline
   const double zLowB = zHighB - deltaOff; // closer on far sideline

   // Panel counts: 8 along each sideline, 4 along each baseline
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

      CROWD_QUAD(0,0,1,
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

      CROWD_QUAD(0,0,-1,
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

      CROWD_QUAD(1,0,0,
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

      CROWD_QUAD(-1,0,0,
                 xLowR,  yLowTop,  zb0,
                 xLowR,  yLowTop,  zb1,
                 xHighR, yHighTop, zt1,
                 xHighR, yHighTop, zt0);
   }

#undef CROWD_QUAD

}

// Arena fan shell:
static void drawArenaBowlAndCrowd(double courtLenHalfX, double courtWidHalfZ, double baseY, double offCourtFeet, double secondRowOffsetFeet)
{
   // Short low wall just behind the 2-row riser
   drawLowerBowlWalls(courtLenHalfX, courtWidHalfZ, baseY, offCourtFeet, secondRowOffsetFeet);
   // Big outer arena shell
   drawArenaBoundaryWalls(courtLenHalfX, courtWidHalfZ, baseY);
   // Crowd / bleacher planes between them
   drawBleacherFanPlanes(courtLenHalfX, courtWidHalfZ, baseY, offCourtFeet, secondRowOffsetFeet);
}

// MASTER BASKETBALL COURT FUNCTION: Draws the entire basketball court scene.
// Standard version drawing full court with lines
static void drawCompleteBasketballCourt(double x, double y, double z, double scale)
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
   double leftTableX  = 0.5 * (leftCoolerX1  + leftCoolerX2);
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

   // Ball dribbling at center court 
   double ball_radius = 0.2;
   double dribble_height = 1.0;
   double dribble_speed = 1.25;
   double time = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
   double ball_y = ball_radius + dribble_height * fabs(sin(time * dribble_speed));
   basketball(0, ball_y, 0, ball_radius, zh);

   glPopMatrix();
}

// INTRODUCING DIFFERENT LIGHTING MODES
// Turn off all lights so switching modes doesn't bleed into each other 
static void killAllLights(void)
{
   for (int i = 0; i < 8; ++i)
      glDisable(GL_LIGHT0 + i);
}

// Fuction to handle all lighitng
// 0: standard for de-bugging, 1: game lighting, 2: pre-game spotlights
static void setupLighting(void)
{
   glShadeModel(GL_SMOOTH);

   // Global light toggle
   if (!light)
   {
      killAllLights();
      glDisable(GL_LIGHTING);
      return;
   }

   // These are not defined at the top b/c they change based on runtime slider values
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
      // Mode 0: de-bugging standard light from hw5. To be removed in final revision
      // case 0:
      // {
      //    float Position[] = {
      //       distance*Cos(light_zh),   // circle around origin in XZ
      //       ylight,                   // height set by [ / ] keys
      //       distance*Sin(light_zh),
      //       1.0f                      // positional light
      //    };

      //    // Little marker so we can see where this light actually is
      //    glDisable(GL_LIGHTING);
      //    glColor3f(1,1,1);
      //    ball(Position[0],Position[1],Position[2],0.1);
      //    glEnable(GL_LIGHTING);

      //    glLightfv(GL_LIGHT0,GL_AMBIENT ,Ambient);
      //    glLightfv(GL_LIGHT0,GL_DIFFUSE ,Diffuse);
      //    glLightfv(GL_LIGHT0,GL_SPECULAR,Specular);
      //    glLightfv(GL_LIGHT0,GL_POSITION,Position);
      //    glEnable(GL_LIGHT0);
      //    break;
      // }
      case 0: // warm-up mode
      {
         const float ySpot = 12.0f;  // height of the fixtures
         const float beamRadius = 2.5f; // radius of the sweep on the floor

         // Centers of the two free-throw circles in world space (x,z)
         const float spotCenterX[2] = {-10.0f, 10.0f};
         const float spotCenterZ[2] = {0.0f, 0.0f};

         // Spotlight parameters - tighter beam with higher intensity toward center
         const float spotCutoff = 40.0f;
         const float spotExponent = 5.0f;

         // Keep the beams bright out to their edges 
         const float attC_spot = 1.0f;
         const float attL_spot = 0.0f;
         const float attQ_spot = 0.0f;

         // Spin them a bit faster
         float baseAngle = light_zh * 2;

         for (int i = 0; i < 2; ++i)
         {
            int lid = GL_LIGHT0 + i;
            float cx = spotCenterX[i];
            float cz = spotCenterZ[i];

            // Where this spotlight fixture hangs above the FT line
            float Pos[4] = {cx, ySpot, cz, 1.0f};

            // Phase offset keeps the two beams out of sync
            float aDeg = baseAngle + (i * 180.0f);
            float floorX = cx + beamRadius * Cos(aDeg);
            float floorZ = cz + beamRadius * Sin(aDeg);

            // Point the beam from the fixture down to the moving hotspot
            float Dir[3] = {floorX - Pos[0], -Pos[1], floorZ - Pos[2]};

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

            // Tiny markers so we can see where the fixtures are hanging
            glDisable(GL_LIGHTING);
            if (i == 0) glColor3f(1,1,0); // left FT
            else glColor3f(1,0.5f,0.0f); // right FT
            ball(Pos[0],Pos[1],Pos[2],0.12);
            glEnable(GL_LIGHTING);
         }

         break;
      }
      // Game lighting, focus on court. Not much lighting into the stands
      case 1:
      {
         // Rough footprint of the court in world units
         const float courtHalfX = 9.4f;   // along length
         const float courtHalfZ = 5.0f;   // along width
         const float yArena = 12.0f;  // main lighting truss

         // Attenuation set so court is bright and stands fade out
         const float attC = 1.0f;
         const float attL = 0.005f;
         const float attQ = 0.0005f;

         // Wide cones so the whole playing surface gets coverage
         const float spotCutoff = 85.0f;
         const float spotExponent = 1.5f;

#define SETUP_ARENA_SPOT(idx, POSX, POSY, POSZ)                            \
         do {                                                              \
            float Pos[4] = { (POSX), (POSY), (POSZ), 1.0f };                \
            float Dir[3] = { -Pos[0], -Pos[1], -Pos[2] };                   \
            glLightfv(GL_LIGHT##idx, GL_AMBIENT, Ambient);                  \
            glLightfv(GL_LIGHT##idx, GL_DIFFUSE, Diffuse);                  \
            glLightfv(GL_LIGHT##idx, GL_SPECULAR,Specular);                 \
            glLightfv(GL_LIGHT##idx, GL_POSITION,Pos);                      \
            glLightfv(GL_LIGHT##idx, GL_SPOT_DIRECTION,Dir);                \
            glLightf (GL_LIGHT##idx, GL_SPOT_CUTOFF, spotCutoff);          \
            glLightf (GL_LIGHT##idx, GL_SPOT_EXPONENT,spotExponent);        \
            glLightf (GL_LIGHT##idx, GL_CONSTANT_ATTENUATION, attC);       \
            glLightf (GL_LIGHT##idx, GL_LINEAR_ATTENUATION, attL);       \
            glLightf (GL_LIGHT##idx, GL_QUADRATIC_ATTENUATION, attQ);       \
            glEnable(GL_LIGHT##idx);                                       \
         } while(0)

         // One spot light from each corner of the court 
         SETUP_ARENA_SPOT(0, -courtHalfX, yArena, courtHalfZ);
         SETUP_ARENA_SPOT(1, courtHalfX, yArena, courtHalfZ);
         SETUP_ARENA_SPOT(2, -courtHalfX, yArena, -courtHalfZ);
         SETUP_ARENA_SPOT(3, courtHalfX, yArena, -courtHalfZ);

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

   if (sceneMode == 0)
   {
      // Full arena set up
      drawCompleteBasketballCourt(0, 0, 0, 1.5);
   }
   else
   {
      /* Simple plane-only mode (barebones test scene) */
      SolidPlane(0,0,0,
                 1,0,0,
                 0,1,0,
                 3.0);
   }

   // // Diable lighting for drawing the axis
   // glDisable(GL_LIGHTING);

   // // Axes helper to undersatnd axis transforms
   // glColor3f(1,1,1);
   // if (axes)
   // {
   //    const double len = 2.5;
   //    glBegin(GL_LINES);
   //    glVertex3d(0.0,0.0,0.0); glVertex3d(len,0.0,0.0);
   //    glVertex3d(0.0,0.0,0.0); glVertex3d(0.0,len,0.0);
   //    glVertex3d(0.0,0.0,0.0); glVertex3d(0.0,0.0,len);
   //    glEnd();
   //    glRasterPos3d(len,0.0,0.0); Print("X");
   //    glRasterPos3d(0.0,len,0.0); Print("Y");
   //    glRasterPos3d(0.0,0.0,len); Print("Z");
   // }

   // // Small HUD with camera + lighting state
   // glWindowPos2i(5,5);
   // Print("Angle=%d,%d  Dim=%.1f  View=%s", th,ph,dim,viewText[viewMode]);

   // if (light)
   // {
   //    glWindowPos2i(5,25);
   //    Print("Ambient=%d  Diffuse=%d  Specular=%d  Shininess=%.0f",
   //          ambient,diffuse,specular,shiny);

   //    glWindowPos2i(5,45);
   //    Print("Light=%s  Move=%s  LMode=%d",
   //          light?"On":"Off",
   //          move?"On":"Off",
   //          lightingMode);
   // }

   // swap buffers & check for errors
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
      if (key == GLUT_KEY_RIGHT)      th += 5;
      else if (key == GLUT_KEY_LEFT)  th -= 5;
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
      // Cycle through lighting modes. warm-ups, game, de-bugging (disabled)
      lightingMode = (lightingMode + 1) % 2;
   }
   else if (ch=='z' && ambient>0)
   {
      ambient -= 5;
   }
   else if (ch=='Z' && ambient<100)
   {
      ambient += 5;
   }
   else if (ch=='x' && diffuse>0)
   {
      diffuse -= 5;
   }
   else if (ch=='X' && diffuse<100)
   {
      diffuse += 5;
   }
   else if (ch=='c' && specular>0)
   {
      specular -= 5;
   }
   else if (ch=='C' && specular<100)
   {
      specular += 5;
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

   texBackboard = LoadTexBMP("textures/backboardTex.bmp");
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
   texChairCushion = LoadTexBMP("textures/cuChairLogo.bmp");

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
