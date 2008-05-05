#ifndef MATRIX_H
#define MATRIX_H

#include "vectors.h"

class matrix2d
{
   public:
   matrix2d() 
   {
      setIdentity();
   }

   matrix2d(const matrix2d &matrix)
   {
      int i;
      for(i = 0; i < 9; i++)
         mat[i] = matrix.mat[i];
   }


   void setIdentity()
   {
      mat[0] = mat[4] = mat[8] = 1;
      mat[1] = mat[2] = mat[3] = mat [5] = mat[6] = mat[7] = 0;
   }


   void translate(vector2f offsetvec)
   {
      mat[6] += offsetvec.x * mat[0] + offsetvec.y * mat[3];
      mat[7] += offsetvec.x * mat[1] + offsetvec.y * mat[4];
   }


   void rotateAngle(float angle)
   {
      matrix2d m;

      m.mat[0] = m.mat[4] = cos(angle);
      m.mat[1] = sin(angle);
      m.mat[3] = -m.mat[1];

      combineWith(m);
   }


   void combineWith(const matrix2d &matrix)
   {
      float temp[9];

      temp[0] = mat[0] * matrix.mat[0] + mat[3] * matrix.mat[1] + mat[6] * matrix.mat[2];      
      temp[1] = mat[1] * matrix.mat[0] + mat[4] * matrix.mat[1] + mat[7] * matrix.mat[2];      
      temp[2] = mat[2] * matrix.mat[0] + mat[5] * matrix.mat[1] + mat[8] * matrix.mat[2];      

      temp[3] = mat[0] * matrix.mat[3] + mat[3] * matrix.mat[4] + mat[6] * matrix.mat[5];      
      temp[4] = mat[1] * matrix.mat[3] + mat[4] * matrix.mat[4] + mat[7] * matrix.mat[5];      
      temp[5] = mat[2] * matrix.mat[3] + mat[5] * matrix.mat[4] + mat[8] * matrix.mat[5];      

      temp[6] = mat[0] * matrix.mat[6] + mat[3] * matrix.mat[7] + mat[6] * matrix.mat[8];      
      temp[7] = mat[1] * matrix.mat[6] + mat[4] * matrix.mat[7] + mat[7] * matrix.mat[8];      
      temp[8] = mat[2] * matrix.mat[6] + mat[5] * matrix.mat[7] + mat[8] * matrix.mat[8];
      
      for(int i = 0; i < 9; i++)
         mat[i] = temp[i];
   }

   inline vector2f execute(const vector2f &vec)
   {
      return vector2f(vec.x * mat[0] + vec.y * mat[3] + mat[6],
                      vec.x * mat[1] + vec.y * mat[4] + mat[7]);
   }


   float mat[9];
};


class matrix3d
{
   public:
   matrix3d() 
   {
      setIdentity();
   }

   matrix3d(const matrix3d &matrix)
   {
      int i;
      for(i = 0; i < 16; i++)
         mat[i] = matrix.mat[i];
   }


   void setIdentity()
   {
      mat[0] = mat[5] = mat[10] = mat[15] = 1.0;
      mat[1] = mat[2] = mat[3] = mat[4] = mat[6] = mat[7] = mat[8] = mat[9] = mat[11] = mat[12] = mat[13] = mat[14] = 0.0;
   }

   void translate(vector3f vec)
   {
      mat[12] += vec.x * mat[0] + vec.y * mat[4] + vec.z * mat[8];
      mat[13] += vec.x * mat[1] + vec.y * mat[5] + vec.z * mat[9];
      mat[14] += vec.x * mat[2] + vec.y * mat[6] + vec.z * mat[10];
   }

   void rotatex(float angle)
   {
      matrix3d m;

   }

   void rotatey(float angle)
   {
   }

   void rotatez(float angle)
   {
   }


   float mat[16];
};

#endif
