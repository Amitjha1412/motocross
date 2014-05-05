#include <iostream>
#include <stdlib.h>
#include<math.h>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include<GL/freeglut.h>
#include "imageloader.h"
#include "vec3f.h"
#include<string.h>
#include<stdio.h>
#include <sstream>
#include<vector>
#include<time.h>
#define ESC 27
#define PI 3.14159265358979324
#define DEG2RAD(deg) (deg * PI / 180)
#define RAD2DEG(rad) (rad * 180 / PI)

using namespace std;
void draw_bike();
void Set_Camera();
GLUquadricObj *quadratic;
static int isWire = 0;
static int distance = 10;
static float angleH = 0;
static float angleV = 0;

static float R = 2.0; // Radius of hemisphere.
static int p = 4; // Number of longitudinal slices.
static int q = 6; // Number of latitudinal slices.



static unsigned int pipe, seat, cover, wheel, wheelCenter, cycles; // parts of the motorcycle to make as display lists.
GLUquadricObj *cylinder;

void drawCoordinates();
void drawMotorcycle();
void drawTrailer();
void drawHemisphere();
void drawCylinder(float x, float y, float z);
void drawHandle(float x, float y, float z);
void drawLight();
void drawBase();
void setup();
int ti=60;
int paused=0;
int gameover=0;
time_t prev=time(0);
time_t curr;
char disscore[100000];
GLuint _textureId; //The id of the texture
GLuint _textureId1; //The id of the texture
GLuint _skybox[5];
void printtext(float x, float y, string String)
{
    //(x,y) is from the bottom left of the window
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glPushAttrib(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);
    glRasterPos2f(x,y);
    for (size_t i=0; i<String.size(); i++)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, String[i]);
    }
    glPopAttrib();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
//Makes the image into a texture, and returns the id of the texture
GLuint loadTexture(Image* image) {
    GLuint textureId;
    glGenTextures(1, &textureId); //Make room for our texture
    glBindTexture(GL_TEXTURE_2D, textureId); //Tell OpenGL which texture to edit
    //Map the image to the texture
    glTexImage2D(GL_TEXTURE_2D,                //Always GL_TEXTURE_2D
            0,                            //0 for now
            GL_RGB,                       //Format OpenGL uses for image
            image->width, image->height,  //Width and height
            0,                            //The border of the image
            GL_RGB, //GL_RGB, because pixels are stored in RGB format
            GL_UNSIGNED_BYTE, //GL_UNSIGNED_BYTE, because pixels are stored
            //as unsigned numbers
            image->pixels);               //The actual pixel data
    return textureId; //Returns the id of the texture
}
//Draws a string at the top of the screen indicating that the specified number
//of collisions have occurred
//Represents a terrain, by storing a set of heights and normals at 2D locations
class Terrain {
    private:
        int w; //Width
        int l; //Length
        float** hs; //Heights
        Vec3f** normals;
        bool computedNormals; //Whether normals is up-to-date
    public:
        Terrain(int w2, int l2) {
            w = w2;
            l = l2;

            hs = new float*[l];
            for(int i = 0; i < l; i++) {
                hs[i] = new float[w];
            }

            normals = new Vec3f*[l];
            for(int i = 0; i < l; i++) {
                normals[i] = new Vec3f[w];
            }

            computedNormals = false;
        }

        ~Terrain() {
            for(int i = 0; i < l; i++) {
                delete[] hs[i];
            }
            delete[] hs;

            for(int i = 0; i < l; i++) {
                delete[] normals[i];
            }
            delete[] normals;
        }

        int width() {
            return w;
        }

        int length() {
            return l;
        }

        //Sets the height at (x, z) to y
        void setHeight(int x, int z, float y) {
            hs[z][x] = y;
            computedNormals = false;
        }

        //Returns the height at (x, z)
        float getHeight(int x, int z) {
            if(z>=0 && x>=0)
                return hs[z][x];
            else
                return 0;
        }

        //Computes the normals, if they haven't been computed yet
        void computeNormals() {
            if (computedNormals) {
                return;
            }

            //Compute the rough version of the normals
            Vec3f** normals2 = new Vec3f*[l];
            for(int i = 0; i < l; i++) {
                normals2[i] = new Vec3f[w];
            }

            for(int z = 0; z < l; z++) {
                for(int x = 0; x < w; x++) {
                    Vec3f sum(0.0f, 0.0f, 0.0f);

                    Vec3f out;
                    if (z > 0) {
                        out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
                    }
                    Vec3f in;
                    if (z < l - 1) {
                        in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
                    }
                    Vec3f left;
                    if (x > 0) {
                        left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
                    }
                    Vec3f right;
                    if (x < w - 1) {
                        right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
                    }

                    if (x > 0 && z > 0) {
                        sum += out.cross(left).normalize();
                    }
                    if (x > 0 && z < l - 1) {
                        sum += left.cross(in).normalize();
                    }
                    if (x < w - 1 && z < l - 1) {
                        sum += in.cross(right).normalize();
                    }
                    if (x < w - 1 && z > 0) {
                        sum += right.cross(out).normalize();
                    }

                    normals2[z][x] = sum;
                }
            }

            //Smooth out the normals
            const float FALLOUT_RATIO = 0.5f;
            for(int z = 0; z < l; z++) {
                for(int x = 0; x < w; x++) {
                    Vec3f sum = normals2[z][x];

                    if (x > 0) {
                        sum += normals2[z][x - 1] * FALLOUT_RATIO;
                    }
                    if (x < w - 1) {
                        sum += normals2[z][x + 1] * FALLOUT_RATIO;
                    }
                    if (z > 0) {
                        sum += normals2[z - 1][x] * FALLOUT_RATIO;
                    }
                    if (z < l - 1) {
                        sum += normals2[z + 1][x] * FALLOUT_RATIO;
                    }

                    if (sum.magnitude() == 0) {
                        sum = Vec3f(0.0f, 1.0f, 0.0f);
                    }
                    normals[z][x] = sum;
                }
            }

            for(int i = 0; i < l; i++) {
                delete[] normals2[i];
            }
            delete[] normals2;

            computedNormals = true;
        }

        //Returns the normal at (x, z)
        Vec3f getNormal(int x, int z) {
            if (!computedNormals) {
                computeNormals();
            }
            return normals[z][x];
        }
};

//Loads a terrain from a heightmap.  The heights of the terrain range from
//-height / 2 to height / 2.
Terrain* loadTerrain(const char* filename, float height) {
    Image* image = loadBMP(filename);
    int  span=2;
    Terrain* t = new Terrain(image->width*span, image->height*span);
    for(int y = 0; y < image->height*span; y++) {
        for(int x = 0; x < image->width*span; x++) {
            unsigned char color =
                (unsigned char)image->pixels[3 * ((y%image->height) * image->width + x%image->width)];
            float h = height * ((color / 255.0f) - 0.5f);
            if(h<0)
                t->setHeight(x, y, 0);
            else
                t->setHeight(x, y, h);

        }
    }

    delete image;
    t->computeNormals();
    return t;
}

float _angle = 60.0f;
Terrain* _terrain;

void cleanup() {
    delete _terrain;
}

//Returns the approximate height of the terrain at the specified (x, z) position
float heightAt(Terrain* terrain, float x, float z) {
    //Make (x, z) lie within the bounds of the terrain
    if (x < 0) {
        x = 0;
    }
    else if (x > terrain->width() - 1) {
        x = terrain->width() - 1;
    }
    if (z < 0) {
        z = 0;
    }
    else if (z > terrain->length() - 1) {
        z = terrain->length() - 1;
    }

    //Compute the grid cell in which (x, z) lies and how close we are to the
    //left and outward edges
    int leftX = (int)x;
    if (leftX == terrain->width() - 1) {
        leftX--;
    }
    float fracX = x - leftX;

    int outZ = (int)z;
    if (outZ == terrain->width() - 1) {
        outZ--;
    }
    float fracZ = z - outZ;

    //Compute the four heights for the grid cell
    float h11 = terrain->getHeight(leftX, outZ);
    float h12 = terrain->getHeight(leftX, outZ + 1);
    float h21 = terrain->getHeight(leftX + 1, outZ);
    float h22 = terrain->getHeight(leftX + 1, outZ + 1);

    //Take a weighted average of the four heights
    return (1 - fracX) * ((1 - fracZ) * h11 + fracZ * h12) +
        fracX * ((1 - fracZ) * h21 + fracZ * h22);
}
bool jump=0;
float prevheight=-1;
bool brake=0;
float prevpitch=0;
float acc=0,ret=0;
float vel=0;
float yaw=0; //angle about y to tell where it is going
float pitch=0; //angle to increase hight
float roll=0;//tilt
int cammode=5;
float bike_x=0,bike_z=0,bike_y=0.375;
float blook_x=0.0,blook_y=1.0,blook_z=0.0;
float x = 0.0, y = -5.0; // initially 5 units south of origin
float deltaMove = 0.0; // initially camera doesn't move
int ifrolled;
// Camera direction
float lx = 0.0, ly = 1.0; // camera points initially along y-axis
float angle = 0.0; // angle of rotation for the camera direction
float deltaAngle = 0.0; // additional angle change when dragging
float deltaRotate=0;
float avg_h,h1,h2;//,h2,h3,h4,h5,h6,h7,h8,h9;
vector < pair<int,int> >fossil;
float fossil_size=0.5f;
float tyre_angle=0;

void initRendering() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    Image* image = loadBMP("grass.bmp");
    _textureId = loadTexture(image);
    delete image;
    Image* image1 = loadBMP("water.bmp");
    _textureId1 = loadTexture(image1);
    delete image1;
    Image* image2 = loadBMP("skybox/nx.bmp");
    _skybox[0] = loadTexture(image2);
    delete image2;
    Image* image3 = loadBMP("skybox/nz.bmp");
    _skybox[1] = loadTexture(image3);
    delete image3;
    Image* image4 = loadBMP("skybox/px.bmp");
    _skybox[2] = loadTexture(image4);
    delete image4;
    Image* image5 = loadBMP("skybox/py.bmp");
    _skybox[3] = loadTexture(image5);
    delete image5;
    Image* image6 = loadBMP("skybox/pz.bmp");
    _skybox[4] = loadTexture(image6);
    delete image6;
}

void handleResize(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 1.0, 200.0);
}

float getmagnitude(float a,float b,float c)
{
    return sqrt((a*a)+(b*b)+(c*c));
}



void drawScene() {
    glClearColor(0.0,0.0,0.0,1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, 0.0f);
    if(gameover==0)
    {
    if(brake)
    {
        vel=0;
        ret=0;
        acc=0;
    }
    if(vel>=0 || pitch!=0)
    {
        bike_z+=vel*cosf(DEG2RAD(yaw));
        bike_x+=vel*sinf(DEG2RAD(yaw));
    }
    else if(vel<0)
    {
        vel=0;
        acc=0;
        ret=0;
    }
    if(bike_z<0)
        bike_z=0;
    if(bike_x<0)
        bike_x=0;
    if(bike_z>117)
        bike_z=117;
    if(bike_x>117)
        bike_x=117;

    h1=heightAt(_terrain,bike_x,bike_z);
    h2=h1;
    if(jump==1)
        h1=prevheight-0.1;
    if(prevheight-(h2)>0.2)
        jump=1;
    else if(h1<0)
        jump=0;
    prevheight=h1;
    Vec3f tnormal = _terrain->getNormal(bike_x, bike_z);
    if(!jump)
    {
        float tryc,ab,ac,ad;
        ab=sinf(DEG2RAD(yaw));
        ac=0;
        ad=cosf(DEG2RAD(yaw));
        tryc=((tnormal[0]*ab+tnormal[2]*ad+tnormal[1]*ac)/(getmagnitude(tnormal[0],tnormal[1],tnormal[2])*getmagnitude(ab,ac,ad)));
        tryc=RAD2DEG(acos(tryc));
        pitch=-(90-tryc);
    }
    bike_y=h1+0.25;

    float cx = _terrain->length();
    float cy = 50;
    float cz = _terrain->width();
    Set_Camera();
    GLfloat ambientColor[] = {0.25f, 0.25f, 0.25f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);
    float am1=ti;
    am1=am1/60;
    glColor3f(am1,am1,am1);

    GLfloat lightColor0[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat lightPos0[] = {(cx+cz)/2, cy/2, (cx+cz)/2, 0.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);
    glEnable(GL_TEXTURE_2D);
    /*glColor3f(0.0f,0.1f,0.8f);
      glBegin(GL_QUADS);
      glVertex3f(33.0f,0.0f,3.0f);
      glVertex3f(117.0f,0.0f,43.0f);
      glVertex3f(51.0f,0.0f,0.6f);
      glVertex3f(117.0f,0.0f,31.0f);
      glEnd();*/
    //starting skybox
    //front
    glBindTexture(GL_TEXTURE_2D, _skybox[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(  cx, -0.5f, 0.0f );
    glTexCoord2f(1, 0); glVertex3f( 0.0f, -0.5f, 0.0f );
    glTexCoord2f(1, 1); glVertex3f( 0.0f,  cy, 0.0f );
    glTexCoord2f(0, 1); glVertex3f(  cx,  cy, 0.0f );
    glEnd();
    //left
    glBindTexture(GL_TEXTURE_2D, _skybox[4]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(  cx, -0.5f,  cz );
    glTexCoord2f(1, 0); glVertex3f(  cx, -0.5f, 0.0f );
    glTexCoord2f(1, 1); glVertex3f(  cx,  cy, 0.0f );
    glTexCoord2f(0, 1); glVertex3f(  cx,  cy,  cz );
    glEnd();
    //right
    glBindTexture(GL_TEXTURE_2D, _skybox[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f( 0.0f, -0.5f, 0.0f );
    glTexCoord2f(1, 0); glVertex3f( 0.0f, -0.5f,  cz );
    glTexCoord2f(1, 1); glVertex3f( 0.0f,  cy,  cz );
    glTexCoord2f(0, 1); glVertex3f( 0.0f,  cy, 0.0f );
    glEnd();
    //back
    glBindTexture(GL_TEXTURE_2D, _skybox[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f( 0.0f, -0.5f,  cz );
    glTexCoord2f(1, 0); glVertex3f(  cx, -0.5f,  cz );
    glTexCoord2f(1, 1); glVertex3f(  cx,  cy,  cz );
    glTexCoord2f(0, 1); glVertex3f( 0.0f,  cy,  cz );
    glEnd();
    //up
    glBindTexture(GL_TEXTURE_2D, _skybox[3]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex3f( 0.0f,  cy, 0.0f );
    glTexCoord2f(0, 0); glVertex3f( 0.0f,  cy,  cz );
    glTexCoord2f(1, 0); glVertex3f(  cx,  cy,  cz );
    glTexCoord2f(1, 1); glVertex3f(  cx,  cy, 0.0f );
    glEnd();
    //ending skybox
    for(int z = 0; z < _terrain->length() - 1; z++) {
        glBindTexture(GL_TEXTURE_2D, _textureId);
        //Makes OpenGL draw a triangle at every three consecutive vertices
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBegin(GL_TRIANGLE_STRIP);
        for(int x = 0; x < _terrain->width(); x++) {
            if(z<0.42* _terrain->width()&&z>0.35*_terrain->width()&&(x<0.2*_terrain->length()||x>0.4*_terrain->length()))
            {
                //glBindTexture(GL_TEXTURE_2D,0);
                //glBindTexture(GL_TEXTURE_2D, _textureId1);
                int temp=rand()%50;
                if(temp<15)
                {
                    glColor3f(0.0f,0.5f,1.0f);
                }
                else
                    glColor3f(0.0f,0.5f,0.5f);
            }
            float fet = (x%5)*1.0f;
            Vec3f normal = _terrain->getNormal(x, z);
            glNormal3f(normal[0], normal[1], normal[2]);
            glTexCoord2f(0.0f+fet, 0.0f+fet);
            glVertex3f(x, _terrain->getHeight(x, z), z);
            normal = _terrain->getNormal(x, z + 1);
            glTexCoord2f(10.0f+fet, 10.0f+fet);
            glNormal3f(normal[0], normal[1], normal[2]);
            glTexCoord2f(0.0f+fet, 20.0f+fet);
            glVertex3f(x, _terrain->getHeight(x, z + 1), z + 1);
            glColor3f(1.0f, 1.0f, 1.0f);
        }
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.0f,0.2f,0.6f);

    /* glColor3f(0.3f, 0.9f, 0.0f);
       for(int z = 0; z < _terrain->length() - 1; z++) {
    //Makes OpenGL draw a triangle at every three consecutive vertices
    glBegin(GL_TRIANGLE_STRIP);
    for(int x = 0; x < _terrain->width(); x++) {
    if(z<0.42* _terrain->width()&&z>0.35*_terrain->width()&&(x<0.2*_terrain->length()||x>0.4*_terrain->length()))
    {
    int temp=rand()%50;
    if(temp<15)
    {
    glColor3f(0.0f,0.3f,1.0f);
    }
    else
    glColor3f(0.0f,0.1f,0.5f);
    }
    Vec3f normal = _terrain->getNormal(x, z);
    glNormal3f(normal[0], normal[1], normal[2]);
    glVertex3f(x, _terrain->getHeight(x, z), z);
    normal = _terrain->getNormal(x, z + 1);
    glNormal3f(normal[0], normal[1], normal[2]);
    glVertex3f(x, _terrain->getHeight(x, z + 1), z + 1);
    glColor3f(0.3f, 0.9f, 0.0f);
    }
    glEnd();
    }
    //glPushMatrix();
    //glPopMatrix();*/
    draw_bike();
    }
    if(ti<=0)
        ti=0;
    glPushMatrix();
    glColor3f(1.0f,1.0f,1.0f);
    sprintf(disscore,"Time Left: %d",ti);
    printtext(0.6,0.9,string(disscore));
    int szf=fossil.size();
    sprintf(disscore,"Fossil's Left: %d",szf);
    printtext(0.6,0.8,string(disscore));
    glPopMatrix();
    if(fossil.size()==0)
    {
        glPushMatrix();
        glColor3f(1.0f,1.0f,1.0f);
        sprintf(disscore,"You Won");
        printtext(.0f,.0f,string(disscore));
        sprintf(disscore,"Score: %d",60-ti);
        printtext(.0f,-.1f,string(disscore));
        gameover=1;
        glPopMatrix();
        glutSwapBuffers();
        return;
    }
    else if(ti<=0)
    {
        glPushMatrix();
        glColor3f(1.0f,1.0f,1.0f);
        int sz=fossil.size();
        sprintf(disscore,"You Lost");
        printtext(.0f,.0f,string(disscore));
        sprintf(disscore,"Fossil's Left: %d",sz);
        printtext(0.0,-0.1,string(disscore));
        gameover=1;
        glPopMatrix();
        glutSwapBuffers();
        return;
    }
    for(size_t k=0;k<fossil.size();k++)
    {
        glPushMatrix();
        glTranslatef(fossil[k].first,heightAt(_terrain,fossil[k].first,fossil[k].second)+0.5,fossil[k].second);
        glColor3f(0.6f, 0.0f, 0.1f);
        glRotatef(tyre_angle,0.0f,1.0f,0.0f);
        glutSolidCylinder(0.5,0.2,20,20);
        glPopMatrix();
    }
    glutSwapBuffers();
}
void update()
{
    if(paused==1 && gameover==1)
        return;
    //t++;
    if(jump)
    {
        glutPostRedisplay(); // redisplay everything
        return;
    }
    if(prevpitch<0 && pitch==0)
    {
        ret=-0.005;
    }
    prevpitch=pitch;
    if(vel>0)
        vel=vel+acc+ret-(0.001*sinf(DEG2RAD(pitch)));
    else //if(vel==0)
    {
        ret=0;
        //
        //if(pitch<0)
        // {
        vel=vel+acc-(0.001*sinf(DEG2RAD(pitch)));
        //  }
        //  else
        //vel=vel+acc;
    }
    if(deltaRotate==-1)
        yaw+=5;
    else if(deltaRotate==1)
        yaw-=5;
    if( -45< (roll+(5*ifrolled)) && roll+(5*ifrolled)<45)
        roll=roll+(5*ifrolled);
    if(ifrolled)
    {
        if(vel!=0 && vel>0.05)
        {
            yaw+=-0.7*tanf(DEG2RAD(roll))/vel;
        }
    }
    for(int i=0;i<fossil.size();i++)
    {
        //cout<<fossil[i].first<<" "<<fossil[i].second<<" "<<bike_x<<" "<<bike_z<<endl;
        if(fossil[i].first<bike_x+1.0f && fossil[i].first>bike_x-1.0f && fossil[i].second<bike_z+1.0f && fossil[i].second>bike_z-1.0f)
            fossil.erase(fossil.begin()+i);
    }
    time(&curr);
    if(difftime(curr,prev)>1)
    {
        prev=curr;
        ti--;
    }

    glutPostRedisplay(); // redisplay everything
}

//----------------------------------------------------------------------
// Draw the entire scene
//
// We first update the camera location based on its distance from the
// origin and its direction.
//----------------------------------------------------------------------

void draw_bike()
{
    glColor3f(1.0,0.0,0.0);
    glPushMatrix();
    glTranslatef(bike_x,0,bike_z);
    glRotatef(yaw,0,1,0);
    glRotatef(-pitch,1,0,0);
    glRotatef(roll,0,0,1);
    glTranslatef(0,bike_y+0.05,0);//to keep it till ground
    glPushMatrix();
    glTranslatef(0.0,0.0,-0.125);
    glutSolidCube(0.35);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0,0.0,0.125);
    glutSolidCube(0.25);
    glTranslatef(0.0,0.0,0.125);

    //headlight
    glPushMatrix();
    glColor3f(1.0,1.0,1.0);
    glTranslatef(0.0,0.175,0.0);
    GLfloat light1_ambient[] = { 1, 1, 1, 1.0 };
    GLfloat light1_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light1_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light1_position[] = { -0.2, 0.0, 0.0, 1.0 };
    GLfloat spot_direction[] = { 0.0f, 0.0, 1.0f };

    glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);
    glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.5);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.2);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.1);

    glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 45.0);
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, spot_direction);
    glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 2.0);

    glEnable(GL_LIGHT1);
    glutSolidCone(0.05,-0.1,20,20);

    glPopMatrix();

    //handle
    glPushMatrix();
    glColor3f(1.0,1.0,0.8);
    glLineWidth(10);
    glBegin(GL_LINES);
    glVertex3f(0.125,0.125,0.0);
    glVertex3f(0.2,0.2,0.0);
    glVertex3f(0.2,0.2,0.0);
    glVertex3f(0.4,0.25,0.0);
    glVertex3f(-0.2,0.2,0.0);
    glVertex3f(-0.4,0.25,0.0);
    glVertex3f(-0.125,0.125,0.0);
    glVertex3f(-0.2,0.2,0.0);
    glEnd();
    glPopMatrix();

    glPopMatrix();


    tyre_angle+=vel/0.06;

    //back wheel
    glPushMatrix();
    glColor3f(0.0,0.0,0.0);
    glTranslatef(0.0,-0.125,-0.375);
    glRotatef(-90,1,0,0);
    glRotatef(90,0,1,0);
    glRotatef(RAD2DEG(tyre_angle),0,0,1);//wheel rotation
    glutSolidTorus(0.06,0.12,10,10);
    glPopMatrix();

    //front wheel
    glPushMatrix();
    glColor3f(0.0,0.0,0.0);
    glTranslatef(0.0,-0.125,0.375);
    glRotatef(-90,1,0,0);
    glRotatef(90,0,1,0);
    glRotatef(RAD2DEG(tyre_angle),0,0,1);//wheel rotation
    glutSolidTorus(0.06,0.12,10,10);
    glPopMatrix();

    glPopMatrix();
}
float  view=1;
void Set_Camera()
{
    if(cammode==5)
    {
        gluLookAt(
                bike_x-(2*sin(DEG2RAD(yaw))), bike_y+1,bike_z-(2*cos(DEG2RAD(yaw))),
                bike_x,      bike_y+1,      bike_z,
                0.0,    1.0,    0.0
                );

    }
    else if(cammode==1)
    {
        gluLookAt(
                bike_x,      bike_y+0.25,      bike_z,
                bike_x+2*sinf(DEG2RAD(yaw)), bike_y+0.25,bike_z+2*cosf(DEG2RAD(yaw)),
                0.0,    1.0,    0.0
                );

    }
    else if(cammode==2)
    {
        gluLookAt(
                bike_x,      bike_y+10,      bike_z,
                bike_x+sin(DEG2RAD(yaw)), bike_y,bike_z+cos(DEG2RAD(yaw)),
                0.0,    1.0,    0.0
                );
    }
    else if(cammode==3)
    {
        gluLookAt(
                bike_x-3,     bike_y,bike_z,
                bike_x, bike_y+0.25, bike_z,
                0.0,    1.0,    0.0
                );
    }
    else if(cammode==4)
    {
        gluLookAt(
                bike_x-(3.5*sin(DEG2RAD(yaw))), bike_y+1,bike_z-(3.5*cos(DEG2RAD(yaw))),
                bike_x,      bike_y+1,      bike_z,
                0.0,    1.0,    0.0
                );

    }
}
void processNormalKeys(unsigned char key, int xx, int yy)
{
    if (key == ESC || key == 'q' || key == 'Q'){
        cleanup();
        exit(0);
    }
    if(key=='p')
    {
        if(paused==1)
            paused=0;
        else
            paused=1;
    }
    if(paused==0 && gameover==0)
    {
        if(key=='1')
        {
            cammode=1;
        }
        if(key=='2')
        {
            cammode=2;
        }
        if(key=='3')
        {
            cammode=3;
        }
        if(key=='4')
        {
            cammode=4;
        }
        if(key=='5')
        {
            cammode=5;
        }
        if(!jump && key=='a')
        {
            deltaRotate=-1;
            yaw-=5;
        }
        else if(!jump && key=='d')
        {
            deltaRotate = 1.0;
            yaw+=5;
        }
    }
}
void KeyUp(unsigned char key, int x, int y)
{
    switch (key) {
        case 'a' :
            deltaRotate = 0.0;
            break;
        case 'd':
            deltaRotate = 0.0;
            break;
    }
}
void pressSpecialKey(int key, int xx, int yy)
{
    if(paused==1 && gameover==1)
        return;
    switch (key) {
        case GLUT_KEY_UP :
            deltaMove = 1.0;
            acc=0.005;
            break;
        case GLUT_KEY_DOWN :
            deltaMove = -1.0;
            brake=1;
            acc=-0.010;
            break;
        case GLUT_KEY_RIGHT :
            ifrolled=1;
            break;
        case GLUT_KEY_LEFT :
            ifrolled=-1;
            break;
    }
}

void releaseSpecialKey(int key, int x, int y)
{
    if(paused==1 && gameover==1)
        return;
    switch (key) {
        case GLUT_KEY_UP :
            deltaMove = 0.0;
            ret=-0.005;
            acc=0;
            break;
        case GLUT_KEY_DOWN :
            deltaMove = 0.0;
            acc=0;
            brake=0;
            break;
        case GLUT_KEY_LEFT :
            deltaRotate = 0.0;
            roll=0;
            ifrolled=0;
            break;
        case GLUT_KEY_RIGHT :
            deltaRotate = 0.0;
            roll=0;
            ifrolled=0;
            break;
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(800, 400);

    glutCreateWindow("MotoCross");
    initRendering();

    _terrain = loadTerrain("heightmap.bmp", 20);

    glutDisplayFunc(drawScene);
    glutReshapeFunc(handleResize);
    glutIdleFunc(update); // incremental update
    glutIgnoreKeyRepeat(1); // ignore key repeat when holding key down
    glutKeyboardFunc(processNormalKeys); // process standard key clicks
    glutSpecialFunc(pressSpecialKey); // process special key pressed
    glutKeyboardUpFunc(KeyUp);
    // Warning: Nonstandard function! Delete if desired.
    glutSpecialUpFunc(releaseSpecialKey); // process special key release
    srand(time(NULL));
    for(int i =0; i<36;i++)
    {
        fossil.push_back(make_pair((rand()%115 + 1),(rand()%115 + 1)));
    }
    glutMainLoop();
    return 0;
}
void setup()
{
    // PARTS
    pipe = glGenLists(1);
    seat = glGenLists(1);
    cover = glGenLists(1);
    wheel = glGenLists(1);
    wheelCenter = glGenLists(1);
    cycles = glGenLists(1);

    glNewList(pipe, GL_COMPILE);   // Any cylinder on the motorcycle
    GLUquadricObj *cylinder;
    cylinder=gluNewQuadric();
    glPushMatrix();
    glColor3f(.5,.5,.5);
    gluCylinder(cylinder,0.2,0.2,3,5,5);
    glPushMatrix();
    //drawCoordinates();
    glPopMatrix();
    glPopMatrix();
    glEndList();

    glNewList(seat, GL_COMPILE);
    glPushMatrix();
    glColor3f(0.5, 0.35, 0.05);
    glutSolidCube(1);
    glPushMatrix();
    //drawCoordinates();
    glPopMatrix();
    glPopMatrix();
    glEndList();

    glNewList(cover, GL_COMPILE);
    glPushMatrix();
    glColor3f(0.5, 0.0, 0.0);
    drawHemisphere();
    glPushMatrix();
    //drawCoordinates();
    glPopMatrix();
    glPopMatrix();
    glEndList();

    glNewList(wheel, GL_COMPILE);
    glPushMatrix();
    glColor3f(0.1, 0.1, 0.1);
    glutSolidTorus(0.2, 1.2, 20, 20);
    glPushMatrix();
    //drawCoordinates();
    glPopMatrix();
    glPopMatrix();
    glEndList();

    glNewList(wheelCenter, GL_COMPILE);
    glPushMatrix();
    glColor3f(0.4, 0.5, 0.5);
    glScalef(1.0, 0.5, 1.0);
    glutSolidSphere(0.8, 5, 5);
    glPushMatrix();
    //drawCoordinates();
    glPopMatrix();
    glPopMatrix();
    glEndList();

    glNewList(cycles, GL_COMPILE);
    glColor3f(0.5, 0.5, 0.5);
    glScalef(0.25, 0.25, 0.25);
    cylinder=gluNewQuadric();
    gluCylinder(cylinder,0.5,0.5,5,15,5);
    glEndList();
}


void drawHemisphere()
{
    for(int j = 0; j < q; j++)
    {
        // One latitudinal triangle strip.
        glBegin(GL_TRIANGLE_STRIP);
        for(int i = 0; i <= p; i++)
        {
            glVertex3f( R * cos( (float)(j+1)/q * PI/2.0 ) * cos( 2.0 * (float)i/p * PI ),
                    R * sin( (float)(j+1)/q * PI/2.0 ),
                    R * cos( (float)(j+1)/q * PI/2.0 ) * sin( 2.0 * (float)i/p * PI ) );
            glVertex3f( R * cos( (float)j/q * PI/2.0 ) * cos( 2.0 * (float)i/p * PI ),
                    R * sin( (float)j/q * PI/2.0 ),
                    R * cos( (float)j/q * PI/2.0 ) * sin( 2.0 * (float)i/p * PI ) );
        }
        glEnd();
    }
}

