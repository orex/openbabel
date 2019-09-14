/**********************************************************************
Copyright (C) 2002 by Steffen Reith <streit@streit.cc>
Some portions Copyright (C) 2003-2006 by Geoffrey R. Hutchison
Some portions Copyright (C) 2004 by Chris Morley

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
***********************************************************************/
#include <openbabel/babelconfig.h>

/* ---- C includes ---- */
#include <math.h>
#include <time.h>
#include <stdlib.h>

/* ---- OpenBabel include ---- */
#include <openbabel/obmolecformat.h>
#include <openbabel/mol.h>
#include <openbabel/atom.h>
#include <openbabel/bond.h>
#include <openbabel/elements.h>
#include <openbabel/math/vector3.h>

/* ---- C++ includes ---- */
#include <string>
#include <sstream>
#include <cctype>
#include <functional>
#include <algorithm>

/* ---- Max. length of a atom-label ---- */
#define StrLen 32

/* ---- Define max. length of domainname ---- */
#define MAXDOMAINNAMELEN 256

/* ---- Maximal radius of an atom. Needed for bounding box ---- */
#define MAXRADIUS (double) 3.0

/* ---- Define index of first atom if needed ---- */
#ifndef MIN_ATOM
#define MIN_ATOM 1
#endif

/* ---- Size of time-string ---- */
#define TIME_STR_SIZE 64

/* ---- if x < = EPSILON then x = 0.0 ---- */
#define EPSILON (double) 1e-4

/* ---- Define makro for calculating x^2 ---- */
#ifdef SQUARE
#undef SQUARE
#endif
#define SQUARE(x) ((x) * (x))

/* ---- Define PI (if needed) ---- */
#ifndef PI
#define PI ((double) 3.1415926535897932384626433)
#endif

/* ---- Convert RAD to DEG ---- */
#define RAD2DEG(r) (((double) 180.0 * r) / PI)

//! \return the geometric centroid to an array of coordinates in double* format
// Note: Based on center_coords from obutil.cpp. This function is declared in matrix3x3.h but there is no function
//       definition there.
OpenBabel::vector3 my_center_coords(double *c, unsigned int size)
{
  if (size == 0)
    {
      return(OpenBabel::VZero);
    }
	unsigned int i;
  double x=0.0, y=0.0, z=0.0;
  for (i = 0;i < size;++i)
    {
      x += c[i*3];
      y += c[i*3+1];
      z += c[i*3+2];
    }
  x /= (double) size;
  y /= (double) size;
  z /= (double) size;
  OpenBabel::vector3 v(x,y,z);
  return(v);
}


using namespace std;
namespace OpenBabel
{

  class PovrayFormat : public OBMoleculeFormat
  {
  public:
    //Register this format type ID
    PovrayFormat()
    {
      OBConversion::RegisterFormat("pov",this);
    }

    virtual const char* Description() //required
    {
      return
      "POV-Ray input format\n"
      "Generate an input file for the open source POV-Ray ray tracer.\n\n"
"The POV-Ray file generated by Open Babel should be considered a starting\n"
"point for the user to create a rendered image of a molecule. Although care\n"
"is taken to center the camera on the molecule, the user will probably want\n"
"to adjust the viewpoint, change the lighting, textures, etc.\n\n"
      
"The file :file:`babel_povray3.inc` is required to render the povray file\n"
"generated by Open Babel. This file is included in the Open Babel\n"
"distribution, and it should be copied into the same directory as the\n"
":file:`.pov` file before rendering. By editing the settings in\n"
":file:`babel_povray3.inc` it is possible to tune the appearance\n"
"of the molecule.\n\n"

"For example, the image below was generated by rendering the output from the\n"
"following command after setting the reflection of non-metal atoms to 0\n"
"(line 121 in :file:`babel_povray3.inc`)::\n\n"
"  obabel -:\"CC(=O)Cl acid chloride\" --gen3d -O chloride.pov -xc -xf -xs -m SPF\n\n"

".. image:: ../_static/povray.png\n\n"

      "Write Options e.g. -xt\n"
      " c Add a black and white checkerboard\n"
      " f Add a mirror sphere\n"
      " m <model-type> BAS (ball-and-stick), SPF (space-fill) or CST (capped sticks)\n"
      "    The default option is ball-and-stick. To choose space-fill, you would use\n"
      "    the following command line::\n \n"
      "      obabel aspirin.mol -O aspirin.pov -xm SPF\n \n"
      " s Add a sky (with clouds)\n"
      " t Use transparent textures\n"
      ;
    };

    virtual const char* SpecificationURL()
    {return "http://www.povray.org/";}; //optional

    //Flags() can return be any the following combined by | or be omitted if none apply
    // NOTREADABLE  READONEONLY  NOTWRITABLE  WRITEONEONLY
    virtual unsigned int Flags()
    {
      return NOTREADABLE | WRITEONEONLY;
    };

    ////////////////////////////////////////////////////
    /// The "API" interface functions
    virtual bool WriteMolecule(OBBase* pOb, OBConversion* pConv);

  private:
    string model_type;
    bool sky, sphere, trans_texture, checkerboard;
    void OutputHeader(ostream &ofs, OBMol &mol, string prefix);

  };

  //Make an instance of the format class
  PovrayFormat thePovrayFormat;

  void PovrayFormat::OutputHeader(ostream &ofs, OBMol &mol, string prefix)
  {
    time_t akttime;                              /* Systemtime                        */
    char timestr[TIME_STR_SIZE + 1] = "";        /* Timestring                        */
    size_t time_res;                             /* Result of strftime                */

    /* ---- Get the system-time ---- */
    akttime = time((time_t *) NULL);
    time_res = strftime(timestr,
                        TIME_STR_SIZE,
                        "%a %b %d %H:%M:%S %Z %Y",
                        localtime((time_t *) &akttime)
                        );

    /* ---- Write some header information ---- */
    ofs << "//Povray v3 code generated by Open Babel" << endl;
    ofs << "//Author: Steffen Reith <streit@streit.cc>" << endl;
    ofs << "//Update (2010): Noel O'Boyle and Steven Wathen" << endl;

    /* ---- Include timestamp in header ---- */
    ofs << "//Date: " << timestr << endl << endl;

    /* ---- Set configurable global settings ---- */
    ofs << "//Set some global parameters for display options" << endl;
    ofs << "#declare " << model_type << " = true;" << endl;
    string trans_tex_setting = trans_texture ? "true" : "false";
    ofs << "#declare TRANS = " << trans_tex_setting << ";" << endl << endl;

    /* ---- Background, camera and light source ---- */
    OpenBabel::vector3 centroid = my_center_coords(mol.GetCoordinates(), mol.NumAtoms());

    ofs << "#include \"colors.inc\"\n" << endl;

    ofs <<  "// create a regular point light source\n"
            "light_source {\n"
            "  <" << centroid.x()  + 2.0 << "," << centroid.y() + 3.0 << "," << centroid.z() - 8.0 << ">\n"
            "  color rgb <1,1,1>    // light's color\n"
            "}\n" << endl;

    if (sky) {
      ofs << "// Add some nice sky with clouds\n"
"sky_sphere {\n"
"    pigment {\n"
"      gradient y\n"
"      color_map {\n"
"        [0.0 1.0 color SkyBlue  color NavyBlue]\n"
"      }\n"
"      scale 2\n"
"      translate -1\n"
"    }\n"
"    pigment {\n"
"      bozo\n"
"      turbulence 0.65\n"
"      octaves 6\n"
"      omega 0.7\n"
"      lambda 2\n"
"      color_map {\n"
"          [0.0 0.1 color rgb <0.85, 0.85, 0.85>\n"
"                   color rgb <0.75, 0.75, 0.75>]\n"
"          [0.1 0.5 color rgb <0.75, 0.75, 0.75>\n"
"                   color rgbt <1, 1, 1, 1>]\n"
"          [0.5 1.0 color rgbt <1, 1, 1, 1>\n"
"                   color rgbt <1, 1, 1, 1>]\n"
"      }\n"
"      scale <0.2, 0.5, 0.2>\n"
"    }\n"
"    rotate -135*x\n"
"  }\n" << endl;
    }
    else { // Simple background
      ofs << "// set a color of the background (sky)" << endl;
      ofs << "background { color rgb <0.95 0.95 0.95> }\n" << endl;
    }

    ofs <<  "// perspective (default) camera\n"
            "camera {\n"
            "  location  <" << centroid.x() << "," << centroid.y() << "," << centroid.z() - 10.0 << ">\n"
            "  look_at   <" << centroid.x() << "," << centroid.y() << "," << centroid.z() << ">\n"
            "  right     x*image_width/image_height\n"
            "}\n" << endl;

    /* ---- Checkerboard and mirror sphere ---- */
    if (sphere) {
      ofs <<
"// a mirror sphere\n"
"sphere\n"
"{ <" << centroid.x()  + 8.0 << "," << centroid.y() - 4 << "," << centroid.z() + 8.0 << ">,4\n"
"  pigment { rgb <0,0,0> } // A perfect mirror with no color\n"
"  finish { reflection 1 } // It reflects all\n"
"}\n" << endl;
    }
    if (checkerboard) {
      ofs <<
"// simple Black on White checkerboard... it's a classic\n"
"plane {\n"
" -y, " << -(centroid.y()-8) << "\n"
" pigment {\n"
"  checker color Black color White\n"
"  scale 2\n"
" }\n"
"}\n" << endl;
    }


    /* ---- Include header statement for babel ---- */
    ofs << "//Include header for povray" << endl;
    ofs << "#include \"babel_povray3.inc\"" << endl << endl;

    /* ---- You should do a spacefill model for molecules without bonds ---- */
    if (mol.NumBonds() == 0)
      {

        /* ---- Check if a spacefill-model is selected ---- */
        ofs << "#if (BAS | CST)\"" << endl;
        ofs << "#warning \"Molecule without bonds!\"" << endl;
        ofs << "#warning \"You should do a spacefill-model\"" << endl;
        ofs << "#end" << endl << endl;

      }

    /* ---- Set version ---- */
    ofs << "//Use PovRay3.6" << endl;
    ofs << "#version 3.6;" << endl << endl;

    /* ---- Print of name of molecule (#\b depends on size of babel.inc!) ---- */
    ofs << "//Print name of molecule while rendering" << endl;
    ofs << "#render \"\\b\\b " << mol.GetTitle() << "\\n\\n\"" << endl << endl;

  }

  void CalcBoundingBox(OBMol &mol,
                       double &min_x, double &max_x,
                       double &min_y, double &max_y,
                       double &min_z, double &max_z
                       )
  {
    /* ---- Init bounding-box variables ---- */
    min_x = (double) 0.0;
    max_x = (double) 0.0;
    min_y = (double) 0.0;
    max_y = (double) 0.0;
    min_z = (double) 0.0;
    max_z = (double) 0.0;

    /* ---- Check all atoms ---- */
    for(unsigned int i = 1; i <= mol.NumAtoms(); ++i)
      {

        /* ---- Get a pointer to ith atom ---- */
        OBAtom *atom = mol.GetAtom(i);

        /* ---- Check for minimal/maximal x-position ---- */
        if (atom -> GetX() < min_x)
          min_x = atom -> GetX();
        if (atom -> GetX() > max_x)
          max_x = atom -> GetX();

        /* ---- Check for minimal/maximal y-position ---- */
        if (atom -> GetY() < min_y)
          min_y = atom -> GetY();
        if (atom -> GetY() > max_y)
          max_y = atom -> GetY();

        /* ---- Check for minimal/maximal z-position ---- */
        if (atom -> GetZ() < min_z)
          min_z = atom -> GetZ();
        if (atom -> GetZ() > max_z)
          max_z = atom -> GetZ();

      }

  }

  void OutputAtoms(ostream &ofs, OBMol &mol, string prefix)
  {
    /* ---- Write all coordinates ---- */
    ofs << "//Coodinates of atoms 1 - " << mol.NumAtoms() << endl;
    unsigned int i;
    for(i = 1; i <= mol.NumAtoms(); ++i)
      {

        /* ---- Get a pointer to ith atom ---- */
        OBAtom *atom = mol.GetAtom(i);

        /* ---- Write position of atom i ---- */
        ofs << "#declare " << prefix << "_pos_" << i << " = <"
            << atom -> GetX() << ","
            << atom -> GetY() << ","
            << atom -> GetZ()
            << ">;" << endl;

      }

    /* ---- Write povray-description of all atoms ---- */
    ofs << endl << "//Povray-description of atoms 1 - " << mol.NumAtoms() << endl;
    for(i = 1; i <= mol.NumAtoms(); ++i)
      {

        /* ---- Get a pointer to ith atom ---- */
        OBAtom *atom = mol.GetAtom(i);

        /* ---- Write full description of atom i ---- */
        ofs << "#declare " << prefix << "_atom" << i << " = ";
        ofs << "object {" << endl
            << "\t  Atom_" << OBElements::GetSymbol(atom->GetAtomicNum()) << endl
            << "\t  translate " << prefix << "_pos_" << i << endl << "\t }" << endl;

      }

    /* ---- Add empty line ---- */
    ofs << endl;

  }


  void OutputBASBonds(ostream &ofs, OBMol &mol, string prefix)
  {
    /* ---- Write povray-description of all bonds---- */
    for(unsigned int i = 0; i < mol.NumBonds(); ++i)
      {

        double x1,y1,z1,x2,y2,z2; /* Start and stop coordinates of a bond       */
        double dist;              /* Distance between (x1|y1|z1) and (x2|y2|z2) */
        double phi,theta;         /* Angles between (x1|y1|z1) and (x2|y2|z2)   */
        double dy;                /* Distance between (x1|0|z1) and (x2|0|z2)   */

        /* ---- Get a pointer to ith atom ---- */
        OBBond *bond = mol.GetBond(i);

        /* ---- Assign start of bond i ---- */
        x1 = (bond -> GetBeginAtom()) -> GetX();
        y1 = (bond -> GetBeginAtom()) -> GetY();
        z1 = (bond -> GetBeginAtom()) -> GetZ();

        /* ---- Assign end of bond i ---- */
        x2 = (bond -> GetEndAtom()) -> GetX();
        y2 = (bond -> GetEndAtom()) -> GetY();
        z2 = (bond -> GetEndAtom()) -> GetZ();

        /* ---- Calculate length of bond and (x1|0|z1) - (x2|0|z2) ---- */
        dist = sqrt(SQUARE(x2-x1) + SQUARE(y2-y1) + SQUARE(z2-z1));
        dy = sqrt(SQUARE(x2-x1) + SQUARE(z2-z1));

        /* ---- Calculate Phi and Theta ---- */
        phi = (double) 0.0;
        theta = (double) 0.0;
        if (fabs(dist) >= EPSILON)
          phi = acos((y2-y1)/dist);
        if (fabs(dy) >= EPSILON)
          theta = acos((x2-x1)/dy);

        /* ---- Full description of bond i ---- */
        ofs << "#declare " << prefix << "_bond" << i
            << " = object {" << endl << "\t  bond_" << bond -> GetBondOrder() << endl;

        /* ---- Scale bond if needed ---- */
        if (fabs(dist) >= EPSILON)
          {

            /* ---- Print povray scale-statement (x-Axis) ---- */
            ofs << "\t  scale <" << dist << ",1.0000,1.0000>\n";

          }

        /* ---- Rotate (Phi) bond if needed ---- */
        if (fabs(RAD2DEG(-phi) + (double) 90.0) >= EPSILON)
          {

            /* ---- Rotate along z-axis ---- */
            ofs << "\t  rotate <0.0000,0.0000,"
                << RAD2DEG(-phi) + (double) 90.0
                << ">" << endl;


          }

        /* ---- Check angle between (x1|0|z1) and (x2|0|z2) ---- */
        if (theta >= EPSILON)
          {

            /* ---- Check direction ---- */
            if ((z2 - z1) >= (double) 0.0)
              {

                /* ---- Rotate along y-Axis (negative) ---- */
                ofs << "\t  rotate <0.0000,"
                    << RAD2DEG((double) -1.0 * theta) << ",0.0000>"
                    << endl;

              }
            else
              {

                /* ---- Rotate along y-Axis (positive) ---- */
                ofs << "\t  rotate <0.0000,"
                    << RAD2DEG(theta) << ",0.0000>"
                    << endl;

              }

          }

        /* ---- Translate bond to start ---- */
        ofs << "\t  translate " << prefix << "_pos_" << bond -> GetBeginAtomIdx()
            << endl << "\t }" << endl;

      }

  }

  void OutputCSTBonds(ostream &ofs, OBMol &mol, string prefix)
  {
    string bond_type;

    /* ---- Write povray-description of all bonds---- */
    for(unsigned int i = 0; i < mol.NumBonds(); ++i)
      {

        double x1,y1,z1,x2,y2,z2; /* Start and stop coordinates of a bond       */
        double dist;              /* Distance between (x1|y1|z1) and (x2|y2|z2) */
        double phi,theta;         /* Angles between (x1|y1|z1) and (x2|y2|z2)   */
        double dy;                /* Distance between (x1|0|z1) and (x2|0|z2)   */

        /* ---- Get a pointer to ith atom ---- */
        OBBond *bond = mol.GetBond(i);

        /* ---- Assign start of bond i ---- */
        x1 = (bond -> GetBeginAtom()) -> GetX();
        y1 = (bond -> GetBeginAtom()) -> GetY();
        z1 = (bond -> GetBeginAtom()) -> GetZ();

        /* ---- Assign end of bond i ---- */
        x2 = (bond -> GetEndAtom()) -> GetX();
        y2 = (bond -> GetEndAtom()) -> GetY();
        z2 = (bond -> GetEndAtom()) -> GetZ();

        /* ---- Calculate length of bond and (x1|0|z1) - (x2|0|z2) ---- */
        dist = sqrt(SQUARE(x2-x1) + SQUARE(y2-y1) + SQUARE(z2-z1));
        dy = sqrt(SQUARE(x2-x1) + SQUARE(z2-z1));

        /* ---- Calculate Phi and Theta ---- */
        phi = (double) 0.0;
        theta = (double) 0.0;
        if (fabs(dist) >= EPSILON)
          phi = acos((y2-y1)/dist);
        if (fabs(dy) >= EPSILON)
          theta = acos((x2-x1)/dy);

        /* ---- Begin of description of bond i (for a capped sticks model) ---- */
        ofs << "#declare " << prefix << "_bond" << i << " = object {" << endl;
        ofs << "\t  union {" << endl;

        /* ---- Begin of Start-Half of Bond (i) ---- */
        ofs << "\t   object {" << endl << "\t    bond_" << bond -> GetBondOrder()  << "\n";

        /* ---- Add a pigment - statement for start-atom of bond ---- */
        bond_type = bond->GetBeginAtom() -> GetType();
        bond_type.erase(remove_if(bond_type.begin(), bond_type.end(), bind1st(equal_to<char>(), '.')), bond_type.end());
        ofs << "\t    pigment{color Color_"
            << bond_type
            << "}" << endl;

        /* ---- Scale bond if needed ---- */
        if (fabs((double) 2.0 * dist) >= EPSILON)
          {

            /* ---- Print povray scale-statement (x-Axis) ---- */
            ofs << "\t    scale <" << (double) 0.5 * dist << ",1.0000,1.0000>" << endl;

          }

        /* ---- Rotate (Phi) bond if needed ---- */
        if (fabs(RAD2DEG(-phi) + (double) 90.0) >= EPSILON)
          {

            /* ---- Rotate along z-axis ---- */
            ofs << "\t    rotate <0.0000,0.0000,"
                << RAD2DEG(-phi) + (double) 90.0
                << ">" << endl;

          }

        /* ---- Check angle between (x1|0|z1) and (x2|0|z2) ---- */
        if (theta >= EPSILON)
          {

            /* ---- Check direction ---- */
            if ((z2 - z1) >= (double) 0.0)
              {

                /* ---- Rotate along y-Axis (negative) ---- */
                ofs << "\t    rotate <0.0000,"
                    << RAD2DEG((double) -1.0 *theta) << ",0.0000>"
                    << endl;

              }
            else
              {

                /* ---- Rotate along y-Axis (positive) ---- */
                ofs << "\t    rotate <0.0000," << RAD2DEG(theta) << ",0.0000>" << endl;

              }

          }

        /* ---- Translate bond to start ---- */

        ofs << "\t    translate " << prefix << "_pos_" << bond -> GetBeginAtomIdx() << endl;

        /* ---- End of description of Start-Bond ---- */
        ofs << "\t   }" << endl;

        /* ---- Begin of End-Half of Bond i ---- */
        ofs << "\t   object {" << endl << "\t    bond_" << bond -> GetBondOrder() << endl;

        /* ---- Add a pigment - statement for end-atom of bond i ---- */
        bond_type = bond->GetEndAtom() -> GetType();
        bond_type.erase(remove_if(bond_type.begin(), bond_type.end(), bind1st(equal_to<char>(), '.')), bond_type.end());

        ofs << "\t    pigment{color Color_"
            << bond_type
            << "}" << endl;

        /* ---- Scale bond if needed ---- */
        if (fabs((double) 2.0 * dist) >= EPSILON)
          {

            /* ---- Print povray scale-statement (x-Axis) ---- */
            ofs << "\t    scale <" << (double) 0.5 * dist << ",1.0000,1.0000>" << endl;

          }

        /* ---- Rotate (Phi) bond if needed ---- */
        if (fabs(RAD2DEG(-phi) + (double) 270.0) >= EPSILON)
          {

            /* ---- Rotate along z-axis (oposite to start half) ---- */
            ofs << "\t    rotate <0.0000,0.0000,"
                << (RAD2DEG(-phi) + (double) 90.0) + (double) 180.0
                << ">" << endl;

          }

        /* ---- Check angle between (x1|0|z1) and (x2|0|z2) ---- */
        if (fabs(theta) >= EPSILON)
          {

            /* ---- Check direction ---- */
            if ((z2 - z1) >= (double) 0.0)
              {

                /* ---- Rotate along y-Axis (negative) (oposite orientation) ---- */
                ofs << "\t    rotate <0.0000,"
                    << RAD2DEG((double) -1.0 * theta)
                    << ",0.0000>"
                    << endl;

              }
            else
              {

                /* ---- Rotate along y-Axis (positive) (oposite orientation) ---- */
                ofs << "\t    rotate <0.0000," << RAD2DEG(theta) << ",0.0000>" << endl;

              }

          }

        /* ---- Translate bond to end ---- */
        ofs << "\t    translate " << prefix << "_pos_" << bond -> GetEndAtomIdx() << endl;

        /* ---- End of description of End-Bond ---- */
        ofs << "\t   }" << endl;

        /* ---- End of description of bond i ---- */
        ofs << "\t  }" << endl << "\t }" << endl << endl;

      }

  }

  void OutputUnions(ostream &ofs, OBMol &mol, string prefix)
  {
    /* ---- Build union of all atoms ---- */
    ofs << endl << "//All atoms of molecule " << prefix << endl;
    ofs << "#ifdef (TRANS)" << endl;
    ofs << "#declare " << prefix << "_atoms = merge {" << endl;
    ofs << "#else" << endl;
    ofs << "#declare " << prefix << "_atoms = union {" << endl;
    ofs << "#end //(End of TRANS)" << endl;

    /* ---- Write definition of all atoms ---- */
    for(unsigned int i = 1; i <= mol.NumAtoms(); ++i)
      {

        /* ---- Write definition of atom i ---- */
        ofs << "\t  object{" << prefix << "_atom" << i << "}" << endl;

      }
    ofs << "\t }" << endl << endl;

    /* ---- Check for number of bonds ---- */
    if(mol.NumBonds() > 0)
      {

        /* ---- Do a BAS or CST model ? ---- */
        ofs << "//Bonds only needed for ball and sticks or capped sticks models" << endl;
        ofs << "#if (BAS | CST)" << endl;
        ofs << "#declare " << prefix <<"_bonds = union {" << endl;

        /* ---- Description of all bonds ---- */
        for(unsigned int i = 0; i < mol.NumBonds(); ++i)
          {

            /* ---- Write Definition of Bond i ---- */
            ofs << "\t  object{" << prefix << "_bond" << i << "}" << endl;

          }

        /* ---- End of povray-conditional for ball and sticks ---- */
        ofs << "\t }" << endl << "#end" << endl << endl;

      }

  }

  void OutputMoleculeBonds(ostream &ofs,
                           string prefix,
                           double min_x, double max_x,
                           double min_y, double max_y,
                           double min_z, double max_z
                           )
  {
    /* ---- Write a comment ---- */
    ofs << endl << "//Definition of molecule " << prefix << endl;

    /* ---- Check for space-fill model ---- */
    ofs << "#if (SPF)" << endl;
    ofs << "#declare " << prefix << " = object{"
        << endl << "\t  " << prefix << "_atoms" << endl;

    /* ---- Here we do BAS oder CST models ---- */
    ofs << "#else" << endl;
    ofs << "#declare " << prefix << " = union {" << endl;

    /* ---- Add all Atoms ---- */
    ofs << "\t  object{" << prefix << "_atoms}" << endl;

    /* ---- Add difference between bonds and atoms ---- */
    ofs << "#if (BAS | CST)//(Not really needed at moment!)" << endl;

    /* ---- Use disjunct objects for transparent pics? ---- */
    ofs << "#if (TRANS)" << endl;
    ofs << "\t  difference {" << endl;
    ofs << "\t   object{" << prefix << "_bonds}" << endl
        << "\t   object{" << prefix << "_atoms}" << endl
        << "\t  }" << endl;

    /* ---- Do a solid model ? ---- */
    ofs << "#else" << endl;
    ofs << "\t  object{" << prefix << "_bonds}" << endl;
    ofs << "#end //(End of TRANS)" << endl;
    ofs << "#end //(End of (BAS|CST))" << endl;

    /* ---- End of CST or BAS model ---- */
    ofs << "#end //(End of SPF)" << endl;

    /* ---- Add comment (bounding box) ---- */
    ofs << "//\t  bounded_by {" << endl
        << "//\t   box {" << endl
        << "//\t    <"
        << min_x - MAXRADIUS <<  ","
        << min_y - MAXRADIUS <<  ","
        << min_z - MAXRADIUS << ">" << endl;

    ofs << "//\t    <"
        << max_x + MAXRADIUS << ","
        << max_y + MAXRADIUS << ","
        << max_z + MAXRADIUS << ">" << endl;

    ofs << "\t }" << endl << endl;

  }

  void OutputMoleculeNoBonds(ostream &ofs, string prefix)
  {
    /* ---- Print description of molecule without bonds ---- */
    ofs << endl << "//Definition of Molecule " << prefix << " (no bonds)" << endl;
    ofs << "#declare " << prefix << " = object {" << prefix << "_atoms}" << endl << endl;

  }

  void OutputCenterComment(ostream &ofs,
                           string prefix,
                           double min_x, double max_x,
                           double min_y, double max_y,
                           double min_z, double max_z
                           )
  {
    /* ---- Print center comment (Warn: Vector is multiplied by -1.0)---- */
    ofs << "//Center of molecule " << prefix << " (bounding box)" << endl;
    ofs << "#declare " << prefix << "_center = <"
        << (double) -1.0 * (min_x + max_x) / (double) 2.0 << ","
        << (double) -1.0 * (min_y + max_y) / (double) 2.0 << ","
        << (double) -1.0 * (min_z + max_z) / (double) 2.0 << ">;" << endl << endl;
  }

  ////////////////////////////////////////////////////////////////

  bool PovrayFormat::WriteMolecule(OBBase* pOb, OBConversion* pConv)
  {
    OBMol* pmol = dynamic_cast<OBMol*>(pOb);
    if(pmol==NULL)
      return false;

    // Model-type should be one of "bas", "spf" or "cas"
    model_type = "BAS"; // Default is ball-and-stick
    const char* tmp = pConv->IsOption("m");
    if (tmp) {
      model_type = string(tmp);
      // Convert to uppercase
      std::transform(model_type.begin(), model_type.end(), model_type.begin(), static_cast< int(*)(int) >(std::toupper));
      if (model_type != "BAS" && model_type != "SPF" && model_type != "CST") {
        obErrorLog.ThrowError(__FUNCTION__, "Unknown model type specified. Using the default instead (\"BAS\", ball-and-stick).\n",obWarning);
        model_type = "BAS";
      }
    }

    // Set private class variables for options
    trans_texture = pConv->IsOption("t") ? true : false;
    sky = pConv->IsOption("s") ? true : false;
    checkerboard = pConv->IsOption("c") ? true : false;
    sphere = pConv->IsOption("f") ? true : false;

    //Define some references so we can use the old parameter names
    ostream &ofs = *pConv->GetOutStream();
    OBMol &mol = *pmol;
    const char* title = pmol->GetTitle();

    static long num = 0;
    double min_x, max_x, min_y, max_y, min_z, max_z; /* Edges of bounding box */

    /* ---- We use mol_${num}_ as our prefix ---- */
    stringstream ss;
    ss << "mol_" << num;
    string prefix = ss.str();

    /* ---- Check if we have already written a molecule to this file ---- */
    if (num == 0)
      {

        /* ---- Print the header ---- */
        OutputHeader(ofs, mol, prefix);

      }
    else
      {

        /* ---- Convert the unique molecule-number to a string and set the prefix ---- */
        ostringstream numStr;
        numStr << num << ends;
        prefix += numStr.str().c_str();
      }

    /* ---- Print positions and descriptions of all atoms ---- */
    OutputAtoms(ofs, mol, prefix);

    /* ---- Check #bonds ---- */
    if (mol.NumBonds() > 0)
      {

        /* ---- Write an comment ---- */
        ofs << "//Povray-description of bonds 1 - " << mol.NumBonds() << endl;

        /* ---- Do a ball and sticks model? ---- */
        ofs << "#if (BAS)" << endl;

        /* ---- Print bonds using "ball and sticks style" ---- */
        OutputBASBonds(ofs, mol, prefix);

        /* ---- End of povray-conditional for ball and sticks ---- */
        ofs << "#end //(BAS-Bonds)" << endl << endl;

        /* ---- Do a capped-sticks model? ---- */
        ofs << "#if (CST)" << endl;

        /* ---- Print bonds using "capped sticks style" ---- */
        OutputCSTBonds(ofs, mol, prefix);

        /* ---- End of povray-conditional for capped sticks ---- */
        ofs << "#end // (CST-Bonds)" << endl << endl;

      }

    /* ---- Print out unions of atoms and bonds ---- */
    OutputUnions(ofs, mol, prefix);

    /* ---- Calculate bounding-box ---- */
    CalcBoundingBox(mol, min_x, max_x, min_y, max_y, min_z, max_z);

    /* ---- Check #bonds ---- */
    if (mol.NumBonds() > 0)
      {

        /* ---- Print out description of molecule ---- */
        OutputMoleculeBonds(ofs,
                            prefix,
                            min_x, max_x,
                            min_y, max_y,
                            min_z, max_z);

      }
    else
      {

        /* ---- Now we can define the molecule without bonds ---- */
        OutputMoleculeNoBonds(ofs, prefix);

      }

    /* ---- Insert declaration for centering the molecule ---- */
    OutputCenterComment(ofs,
                        prefix,
                        min_x, max_x,
                        min_y, max_y,
                        min_z, max_z);


    /* ---- Insert the molecule ---- */
    ofs << prefix << endl;

    /* ---- Increment the static molecule output-counter ---- */
    num++;

    /* ---- Everything is ok! ---- */
    return(true);
  }

} //namespace OpenBabel
