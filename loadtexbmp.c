//  CSCIx229 library
//  Willem A. (Vlakkies) Schreuder
//  Kevin McMahon - LoadTexBMP32()
#include "CSCIx229.h"

//
//  Load texture from BMP file
//

//
//  Reverse n bytes
//
static void Reverse(void* x,const int n)
{
   char* ch = (char*)x;
   for (int k=0;k<n/2;k++)
   {
      char tmp = ch[k];
      ch[k] = ch[n-1-k];
      ch[n-1-k] = tmp;
   }
}

//
//  Load texture from BMP file
//
unsigned int LoadTexBMP(const char* file)
{
   //  Open file
   FILE* f = fopen(file,"rb");
   if (!f) Fatal("Cannot open file %s\n",file);
   //  Check image magic
   unsigned short magic;
   if (fread(&magic,2,1,f)!=1) Fatal("Cannot read magic from %s\n",file);
   if (magic!=0x4D42 && magic!=0x424D) Fatal("Image magic not BMP in %s\n",file);
   //  Read header
   unsigned int dx,dy,off,k; // Image dimensions, offset and compression
   unsigned short nbp,bpp;   // Planes and bits per pixel
   if (fseek(f,8,SEEK_CUR) || fread(&off,4,1,f)!=1 ||
       fseek(f,4,SEEK_CUR) || fread(&dx,4,1,f)!=1 || fread(&dy,4,1,f)!=1 ||
       fread(&nbp,2,1,f)!=1 || fread(&bpp,2,1,f)!=1 || fread(&k,4,1,f)!=1)
     Fatal("Cannot read header from %s\n",file);
   //  Reverse bytes on big endian hardware (detected by backwards magic)
   if (magic==0x424D)
   {
      Reverse(&off,4);
      Reverse(&dx,4);
      Reverse(&dy,4);
      Reverse(&nbp,2);
      Reverse(&bpp,2);
      Reverse(&k,4);
   }
   //  Check image parameters
   unsigned int max;
   glGetIntegerv(GL_MAX_TEXTURE_SIZE,(int*)&max);
   if (dx<1 || dx>max) Fatal("%s image width %d out of range 1-%d\n",file,dx,max);
   if (dy<1 || dy>max) Fatal("%s image height %d out of range 1-%d\n",file,dy,max);
   if (nbp!=1)  Fatal("%s bit planes is not 1: %d\n",file,nbp);
   if (bpp!=24) Fatal("%s bits per pixel is not 24: %d\n",file,bpp);
   if (k!=0)    Fatal("%s compressed files not supported\n",file);
#ifndef GL_VERSION_2_0
   //  OpenGL 2.0 lifts the restriction that texture size must be a power of two
   for (k=1;k<dx;k*=2);
   if (k!=dx) Fatal("%s image width not a power of two: %d\n",file,dx);
   for (k=1;k<dy;k*=2);
   if (k!=dy) Fatal("%s image height not a power of two: %d\n",file,dy);
#endif

   //  Allocate image memory
   unsigned int size = 3*dx*dy;
   unsigned char* image = (unsigned char*) malloc(size);
   if (!image) Fatal("Cannot allocate %d bytes of memory for image %s\n",size,file);
   //  Seek to and read image
   if (fseek(f,off,SEEK_SET) || fread(image,size,1,f)!=1) Fatal("Error reading data from image %s\n",file);
   fclose(f);
   //  Reverse colors (BGR -> RGB)
   for (k=0;k<size;k+=3)
   {
      unsigned char temp = image[k];
      image[k]   = image[k+2];
      image[k+2] = temp;
   }

   //  Sanity check
   ErrCheck("LoadTexBMP");
   //  Generate 2D texture
   unsigned int texture;
   glGenTextures(1,&texture);
   glBindTexture(GL_TEXTURE_2D,texture);
   //  Copy image
   glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,dx,dy,0,GL_RGB,GL_UNSIGNED_BYTE,image);
   if (glGetError()) Fatal("Error in glTexImage2D %s %dx%d\n",file,dx,dy);
   //  Scale linearly when image size doesn't match
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

   //  Free image memory
   free(image);
   //  Return texture name
   return texture;
}

// Loads 32-bit texture w/ aplha for basketball hoop net 
unsigned int LoadTexBMP32(const char* file)
{
   // Open the file 
   FILE *f = fopen(file, "rb");
   if(!f) Fatal("Cannot open file %s\n", file);

   // Check image magic
   unsigned short magic;
   if(fread(&magic,2,1,f)!=1) Fatal("Cannot read magic from%s\n", file);
   if(magic!=0x4D42 && magic!=0x424D) Fatal("Image magic is not BMP in %s\n", file);

   // Read header (same pattern as LoadTexBMP)
   unsigned int dx, dy, off, k; // IMage dimensions, offset, compression
   unsigned short nbp, bpp; // Planes and bits per pixel
   if(fseek(f,8,SEEK_CUR) || fread(&off,4,1,f)!=1 ||
      fseek(f,4,SEEK_CUR) || fread(&dx,4,1,f)!=1 || fread(&dy,4,1,f)!=1 ||
      fread(&nbp,2,1,f)!=1 || fread(&bpp,2,1,f)!=1 || fread(&k,4,1,f)!=1)
      Fatal("Cannot read header from %s\n", file);

   // Reverse butes on big endian hardware (detected by backwards magic)
   if(magic==0x424D)
   {
      Reverse(&off,4);
      Reverse(&dx,4);
      Reverse(&dy,4);
      Reverse(&nbp,2);
      Reverse(&bpp,2);
      Reverse(&k,4);
   }

   //  Check image parameters
   unsigned int max;
   glGetIntegerv(GL_MAX_TEXTURE_SIZE,(int*)&max);
   if (dx<1 || dx>max) Fatal("%s image width %d out of range 1-%d\n",file,dx,max);
   if (dy<1 || dy>max) Fatal("%s image height %d out of range 1-%d\n",file,dy,max);
   if (nbp!=1)  Fatal("%s bit planes is not 1: %d\n",file,nbp);
   if (bpp!=32) Fatal("%s bits per pixel is not 32: %d\n",file,bpp); // updated for 32-bit
   if (k!=3)    Fatal("%s compressed files not supported\n",file);
#ifndef GL_VERSION_2_0
   //  OpenGL 2.0 lifts the restriction that texture size must be a power of two
   for (k=1;k<dx;k*=2);
   if (k!=dx) Fatal("%s image width not a power of two: %d\n",file,dx);
   for (k=1;k<dy;k*=2);
   if (k!=dy) Fatal("%s image height not a power of two: %d\n",file,dy);
#endif

   // Allocate image memory for BGRA
   unsigned int size = 4*dx*dy; // 4 instead of 3... 32 bit .bmp files
   unsigned char* image = (unsigned char*) malloc(size);
   if(!image) Fatal("Cannot allocate %d bytes of memory for image %s\n",size,file);

   // Seek to and read image data
   if(fseek(f,off,SEEK_SET) || fread(image,size,1,f)!=1) Fatal("Error reading data from image %s\n", file);
   fclose(f);

   // Reverse colors in place
   for(k=0;k<size;k+=4)
   {
      unsigned char B = image[k+0];
      unsigned char G = image[k+1];
      unsigned char R = image[k+2];
      unsigned char A = image[k+3];

      image[k+0] = R;
      image[k+1] = G;
      image[k+2] = B;
      image[k+3] = A;
   }

   // Sanity check
   ErrCheck("LoadTexBMP32");

   // Generate 2D texture
   unsigned int texture;
   glGenTextures(1,&texture);
   glBindTexture(GL_TEXTURE_2D,texture);

   // Copy image as RGBA
   glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,dx,dy,0,GL_RGBA,GL_UNSIGNED_BYTE,image);
   if (glGetError()) Fatal("Error in glTexImage2D (32-bit) %s %dx%d\n",file,dx,dy);

   // Simple linear filtering
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

   // Free image memory
   free(image);

   // Return texture name
   return texture;
}