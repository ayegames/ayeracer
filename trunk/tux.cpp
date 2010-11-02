/* --------------------------------------------------------------------
EXTREME TUXRACER

Copyright (C) 1999-2001 Jasmin F. Patry (Tuxracer)
Copyright (C) 2010 Extreme Tuxracer Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.


This module has been completely rewritten. Remember that the way of 
defining the character has radically changed though the character is 
still shaped with spheres.
---------------------------------------------------------------------*/

#include "tux.h"
#include "ogl.h"
#include "spx.h"
#include "textures.h"
#include "course.h"

#define MAX_ARM_ANGLE2 30.0
#define MAX_PADDLING_ANGLE2 35.0
#define MAX_EXT_PADDLING_ANGLE2 30.0
#define MAX_KICK_PADDLING_ANGLE2 20.0
#define TO_AIR_TIME 0.5			
#define TO_TIME 0.14			

#define SHADOW_HEIGHT 0.03 // ->0.05

#ifdef USE_STENCIL_BUFFER
	static TColor shad_col = { 0.0, 0.0, 0.0, 0.3 };
#else 
	static TColor shad_col = { 0.0, 0.0, 0.0, 0.1 };
#endif 

TCharMaterial TuxDefMat = {{0.5, 0.5, 0.5, 1.0}, {0.0, 0.0, 0.0, 1.0}, 0.0};

CChar Tux;

CChar::CChar () {
	for (int i=0; i<MAX_CHAR_NODES; i++) {
		Nodes[i] = NULL;
		Actions[i] = NULL;
		Index[i] = -1;
	}
	for (int i=0; i<MAX_CHAR_MAT; i++) Matlines[i] = "";
	NodeIndex = "";
	MaterialIndex = "";
	numNodes = 0;
	numMaterials = 0;
	numMatlines = 0;

	useActions = false;
	newActions = false;
	useMaterials = true;
}

CChar::~CChar () {}

// --------------------------------------------------------------------
//				nodes
// --------------------------------------------------------------------

int CChar::GetNodeIdx (int node_name) {
	if (node_name < 0 || node_name >= MAX_CHAR_NODES) return -1;
	return Index[node_name];
}

TCharNode *CChar::GetNode (int node_name) {
	int idx = GetNodeIdx (node_name);
	if (idx < 0 || idx >= numNodes) return NULL;
	return Nodes[idx];
}

bool CChar::GetNode (int node_name, TCharNode **node) {
	int idx = GetNodeIdx (node_name);
	if (idx < 0 || idx >= numNodes) {
		*node = NULL;
		return false;
	} else {
		*node = Nodes[idx];
		return true;
	}
}

void CChar::CreateRootNode () {
    TCharNode *node = new TCharNode;
    node->parent = NULL;
    node->next = NULL;
    node->child = NULL;
    node->mat = NULL;
    node->joint = "root";
    node->render_shadow = false;
	node->visible = false;
    MakeIdentityMatrix (node->trans);
	MakeIdentityMatrix (node->invtrans);

	NodeIndex = "[root]0";
	Index[0] = 0;
	Nodes[0] = node;
	numNodes = 1;
}

bool CChar::CreateCharNode 
		(int parent_name, int node_name, const string joint, 
		string name, string order, bool shadow) {

	TCharNode *parent = GetNode (parent_name);
	if (parent == NULL) { 
		Message ("wrong parent node");
		return false;
	}
    TCharNode *node = new TCharNode;
    node->parent = parent;
    node->next  = NULL;
    node->child = NULL;
    node->mat   = NULL;
	node->parent_name = parent_name; 
	node->node_name = node_name;
	node->node_idx = numNodes; 
	node->visible = false;
		node->render_shadow = shadow;
    node->joint = joint;
	
    MakeIdentityMatrix (node->trans);
    MakeIdentityMatrix (node->invtrans);

	SPAddIntN (NodeIndex, joint, node_name);
	Nodes[numNodes] = node;
	Index[node_name] = numNodes;

	if (parent->child == NULL) {
		parent->child = node;
	} else {
		for (parent = parent->child; parent->next != NULL; parent = parent->next) {} 
		parent->next = node;
	} 

	if (useActions) {
		Actions[numNodes] = new TAction;
		Actions[numNodes]->num = 0;
		Actions[numNodes]->name = name;
		Actions[numNodes]->order = order;
		Actions[numNodes]->mat = "";
		
	}
	numNodes++;
    return true;
} 

void CChar::AddAction (int node_name, int type, TVector3 vec, double val) {
	int idx = GetNodeIdx (node_name);
	if (idx < 0) return;
	TAction *act = Actions[idx];
	act->type[act->num] = type;
	act->vec[act->num] = vec;
	act->dval[act->num] = val;
	act->num++;
}

bool CChar::TranslateNode (int node_name, TVector3 vec) {
    TCharNode *node;
    TMatrix TransMatrix;

    if (GetNode (node_name, &node) == false) return false;
    MakeTranslationMatrix (TransMatrix, vec.x, vec.y, vec.z);
    MultiplyMatrices (node->trans, node->trans, TransMatrix);
	MakeTranslationMatrix (TransMatrix, -vec.x, -vec.y, -vec.z);
	MultiplyMatrices (node->invtrans, TransMatrix, node->invtrans);

	if (newActions && useActions) AddAction (node_name, 0, vec, 0);
    return true;
}

bool CChar::RotateNode (int node_name, double axis, double angle) {
	TCharNode *node;
    TMatrix rotMatrix;
	char caxis = '0';

    if (GetNode (node_name, &node) == false) return false;

	int a = (int) axis;
	if (axis > 3) return false;
	switch (a) {
		case 1: caxis = 'x'; break;
		case 2: caxis = 'y'; break;
		case 3: caxis = 'z'; break;
	}

    MakeRotationMatrix (rotMatrix, angle, caxis);
    MultiplyMatrices (node->trans, node->trans, rotMatrix);
	MakeRotationMatrix (rotMatrix, -angle, caxis);
	MultiplyMatrices (node->invtrans, rotMatrix, node->invtrans);
	TVector3 vvv = MakeVector (angle, 0, 0);

	if (newActions && useActions) AddAction (node_name, axis, NullVec, angle);	
    return true;
}

bool CChar::RotateNode (string node_trivialname, double axis, double angle) {
	int node_name = SPIntN (NodeIndex, node_trivialname, -1);
	if (node_name < 0) return false;
	return RotateNode (node_name, axis, angle);
}

void CChar::ScaleNode (int node_name, TVector3 vec) {
    TCharNode *node;
    TMatrix matrix;

    if  (GetNode (node_name, &node) == false) return;

	MakeIdentityMatrix (matrix);
    MultiplyMatrices (node->trans, node->trans, matrix);
	MakeIdentityMatrix (matrix);
	MultiplyMatrices (node->invtrans, matrix, node->invtrans);

    MakeScalingMatrix (matrix, vec.x, vec.y, vec.z);
    MultiplyMatrices (node->trans, node->trans, matrix);
	MakeScalingMatrix (matrix, 1.0 / vec.x, 1.0 / vec.y, 1.0 / vec.z);
	MultiplyMatrices (node->invtrans, matrix, node->invtrans);

	MakeIdentityMatrix (matrix);
    MultiplyMatrices (node->trans, node->trans, matrix);
	MakeIdentityMatrix (matrix);
	MultiplyMatrices (node->invtrans, matrix, node->invtrans);

	if (newActions && useActions) AddAction (node_name, 4, vec, 0);
}

bool CChar::VisibleNode (int node_name, float level) {
    TCharNode *node;
    if (GetNode (node_name, &node) == false) return false;

	node->visible = (level > 0);

	if (node->visible) {
    	node->divisions = 
			MIN (MAX_SPHERE_DIV, MAX (MIN_SPHERE_DIV, 
			ROUND_TO_NEAREST (param.tux_sphere_divisions * level / 10)));
	    node->radius = 1.0;
	} 
	if (newActions && useActions) AddAction (node_name, 5, NullVec, level);
	return true;
}

bool CChar::MaterialNode (int node_name, string mat_name) {
    TCharMaterial *mat;
    TCharNode *node;
	int idx = GetNodeIdx (node_name);
    if (GetNode (node_name, &node) == false) return false;
    if (GetMaterial (mat_name.c_str(), &mat) == false) return false;
	node->mat = mat;
 	if (newActions && useActions) Actions[idx]->mat = mat_name;
	return true;
}

bool CChar::ResetNode (int node_name) {  
    TCharNode *node;
    if (GetNode (node_name, &node) == false) return false;

    MakeIdentityMatrix (node->trans);
    MakeIdentityMatrix (node->invtrans);
	return true;
}

bool CChar::ResetNode (string node_trivialname) {
	int node_name = SPIntN (NodeIndex, node_trivialname, -1);
	if (node_name < 0) return false;
	return ResetNode (node_name);
}

bool CChar::TransformNode (int node_name, TMatrix mat, TMatrix invmat) {
    TCharNode *node;
    if (GetNode (node_name, &node) == false) return false;

    MultiplyMatrices (node->trans, node->trans, mat);
	MultiplyMatrices (node->invtrans, invmat, node->invtrans);
    return true;
}

void CChar::ResetRoot () { 
	ResetNode (0);
}

void CChar::ResetJoints () {
    ResetNode ("left_shldr");
    ResetNode ("right_shldr");
    ResetNode ("left_hip");
    ResetNode ("right_hip");
    ResetNode ("left_knee");
    ResetNode ("right_knee");
    ResetNode ("left_ankle");
    ResetNode ("right_ankle");
    ResetNode ("tail");
    ResetNode ("neck");
    ResetNode ("head");
}

// --------------------------------------------------------------------
//				materials
// --------------------------------------------------------------------

bool CChar::GetMaterial (const char *mat_name, TCharMaterial **mat) {
	int idx;
	idx = SPIntN (MaterialIndex, mat_name, -1);
	if (idx >= 0 && idx < numMaterials) {
		*mat = Materials[idx];
		return true;
	} else {
		*mat = 0;
		return false;
	}
}

void CChar::CreateMaterial (const char *line) {
	char matName[32];
	TVector3 diff = {0,0,0}; 
	TVector3 spec = {0,0,0};
	float exp = 100;

	string lin = line;	
	SPCharN (lin, "mat", matName);
	diff = SPVector3N (lin, "diff", MakeVector (0,0,0));
	spec = SPVector3N (lin, "spec", MakeVector (0,0,0));
	exp = SPFloatN (lin, "exp", 50);

	TCharMaterial *matPtr = (TCharMaterial *) malloc (sizeof (TCharMaterial));

    matPtr->diffuse.r = diff.x;
    matPtr->diffuse.g = diff.y;
    matPtr->diffuse.b = diff.z;
    matPtr->diffuse.a = 1.0;
    matPtr->specular.r = spec.x;
    matPtr->specular.g = spec.y;
    matPtr->specular.b = spec.z;
    matPtr->specular.a = 1.0;
    matPtr->exp = exp;

	TCharMaterial *test;
    if (GetMaterial (matName, &test)) {
		free (matPtr);
	} else {
		SPAddIntN (MaterialIndex, matName, numMaterials);
		Materials[numMaterials] = matPtr;
		numMaterials++;
	}
}

// --------------------------------------------------------------------
//				drawing
// --------------------------------------------------------------------

void CChar::DrawCharSphere (int num_divisions) {
    GLUquadricObj *qobj;
    qobj = gluNewQuadric();
    gluQuadricDrawStyle (qobj, GLU_FILL);
    gluQuadricOrientation (qobj, GLU_OUTSIDE);
    gluQuadricNormals (qobj, GLU_SMOOTH);
    gluSphere (qobj, 1.0, (GLint)2.0 * num_divisions, num_divisions);
    gluDeleteQuadric (qobj);
}

GLuint CChar::GetDisplayList (int divisions) {
    static bool initialized = false;
    static int num_display_lists;
    static GLuint *display_lists = NULL;
    int base_divisions;
    int i, idx;

    if  (!initialized) {
		initialized = true;
		base_divisions = param.tux_sphere_divisions;
		num_display_lists = MAX_SPHERE_DIV - MIN_SPHERE_DIV + 1;
		display_lists = (GLuint*) malloc (sizeof(GLuint) * num_display_lists);
		for (i=0; i<num_display_lists; i++) display_lists[i] = 0;
    }

    idx = divisions - MIN_SPHERE_DIV;
    if  (display_lists[idx] == 0) {
		display_lists[idx] = glGenLists (1);
		glNewList (display_lists[idx], GL_COMPILE);
		DrawCharSphere (divisions);
		glEndList ();
    }
    return display_lists[idx];
}

void CChar::DrawNodes (TCharNode *node, TCharMaterial *mat) {
    TCharNode *child;
    glPushMatrix();
    glMultMatrixd ((double *) node->trans);

    if (node->mat != NULL) mat = node->mat;
    if (node->visible == true) {
		if (useMaterials) set_material (mat->diffuse, mat->specular, mat->exp);
			else set_material (TuxDefMat.diffuse, TuxDefMat.specular, TuxDefMat.exp);
		if (USE_CHAR_DISPLAY_LIST) glCallList (GetDisplayList (node->divisions));
			else DrawCharSphere (node->divisions);
    } 

    child = node->child;
    while (child != NULL) {
        DrawNodes (child, mat);
        child = child->next;
    } 
    glPopMatrix();
} 

void CChar::Draw () {
    TCharNode *node;
    float dummy_color[]  = {0.0, 0.0, 0.0, 1.0};

    glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, dummy_color);
    set_gl_options (TUX);
	glEnable (GL_NORMALIZE);
	
	if (!GetNode (0, &node)) return;
	DrawNodes (node, &TuxDefMat);
	glDisable (GL_NORMALIZE);
	if (param.perf_level > 2 && g_game.argument == 0) DrawShadow ();
} 

// --------------------------------------------------------------------

bool CChar::Load (string filename, bool with_actions) {
	CSPList list (500);
	int i, ii, act;
	string line, order, name, mat_name, fullname;
	TVector3 scale, trans, rot;
	double visible;
	bool shadow;
	int node_name, parent_name;

	useActions = with_actions;
	CreateRootNode ();
	newActions = true;

	file_name = filename;
 	if (!list.Load (param.char_dir, filename)) {
		Message ("could not load character", filename.c_str());
		return false;
	}

	for (i=0; i<list.Count(); i++) {
		line = list.Line (i);
		node_name = SPIntN (line, "node", -1); 
		parent_name = SPIntN (line, "par", -1);		
		mat_name = SPStrN (line, "mat", "");
		name = SPStrN (line, "joint", "");	
		fullname = SPStrN (line, "name", "");

		if (SPIntN (line, "material", 0) > 0) {
			CreateMaterial (line.c_str());
			if (useActions) {
				Matlines[numMatlines] = line;
				numMatlines++;
			}
		} else {
			visible = SPFloatN (line, "vis", -1.0);	
			shadow = SPBoolN (line, "shad", false);
			order = SPStrN (line, "order", "");
 			CreateCharNode (parent_name, node_name, name, fullname, order, shadow);					
			rot = SPVector3N (line, "rot", NullVec);
			MaterialNode (node_name, mat_name);
			for (ii=0; ii<(int)order.size(); ii++) {
				act = order.at(ii)-48;	
				switch (act) {
					case 0:
						trans = SPVector3N (line, "trans", MakeVector (0,0,0));
						TranslateNode (node_name, trans);
						break;
					case 1: RotateNode (node_name, 1, rot.x); break;
					case 2: RotateNode (node_name, 2, rot.y); break;
					case 3: RotateNode (node_name, 3, rot.z); break;
					case 4:
						scale = SPVector3N (line, "scale", MakeVector (1,1,1));
						ScaleNode (node_name, scale);
						break;
					case 5: VisibleNode (node_name, visible); break;
					case 9: RotateNode (node_name, 2, rot.z); break;
					default: break;
				}
			}
		}
	}
	newActions = false;
	return true;
}

TVector3 CChar::AdjustRollvector (CControl *ctrl, TVector3 vel, TVector3 zvec) {
    TMatrix rot_mat; 
    vel = ProjectToPlane (zvec, vel);
    NormVector (&vel);
    if (ctrl->is_braking) {
		RotateAboutVectorMatrix (rot_mat, vel, ctrl->turn_fact * BRAKING_ROLL_ANGLE);
    } else {
		RotateAboutVectorMatrix (rot_mat, vel, ctrl->turn_fact * MAX_ROLL_ANGLE);
    }
    return TransformVector (rot_mat, zvec);
}

void CChar::AdjustOrientation (CControl *ctrl, double dtime,
		 double dist_from_surface, TVector3 surf_nml) {
    TVector3 new_x, new_y, new_z; 
    TMatrix cob_mat, inv_cob_mat;
    TMatrix rot_mat;
    TQuaternion new_orient;
    double time_constant;
    static TVector3 minus_z_vec = { 0, 0, -1};
    static TVector3 y_vec = { 0, 1, 0 };

    if  (dist_from_surface > 0) {
		new_y = ScaleVector (1, ctrl->cvel);
		NormVector (&new_y);
		new_z = ProjectToPlane (new_y, MakeVector(0, -1, 0));
		NormVector (&new_z);
		new_z = AdjustRollvector (ctrl, ctrl->cvel, new_z);
    } else { 
		new_z = ScaleVector (-1, surf_nml);
		new_z = AdjustRollvector (ctrl, ctrl->cvel, new_z);
		new_y = ProjectToPlane (surf_nml, ScaleVector (1, ctrl->cvel));
		NormVector(&new_y);
    }

    new_x = CrossProduct (new_y, new_z);
    MakeBasismatrix_Inv (cob_mat, inv_cob_mat, new_x, new_y, new_z);
    new_orient = MakeQuaternionFromMatrix (cob_mat);

    if (!ctrl->orientation_initialized) {
		ctrl->orientation_initialized = true;
		ctrl->corientation = new_orient;
    }

    time_constant = dist_from_surface > 0 ? TO_AIR_TIME : TO_TIME;

    ctrl->corientation = InterpolateQuaternions (
			ctrl->corientation, new_orient, 
			min (dtime / time_constant, 1.0));

    ctrl->plane_nml = RotateVector (ctrl->corientation, minus_z_vec);
    ctrl->cdirection = RotateVector (ctrl->corientation, y_vec);
    MakeMatrixFromQuaternion (cob_mat, ctrl->corientation);

    // Trick rotations 
    new_y = MakeVector (cob_mat[1][0], cob_mat[1][1], cob_mat[1][2]); 
    RotateAboutVectorMatrix (rot_mat, new_y, (ctrl->roll_factor * 360));
    MultiplyMatrices (cob_mat, rot_mat, cob_mat);
    new_x = MakeVector (cob_mat[0][0], cob_mat[0][1], cob_mat[0][2]); 
    RotateAboutVectorMatrix (rot_mat, new_x, ctrl->flip_factor * 360);
    MultiplyMatrices (cob_mat, rot_mat, cob_mat);
    TransposeMatrix (cob_mat, inv_cob_mat);
	TransformNode (0, cob_mat, inv_cob_mat); 
}

void CChar::AdjustJoints (double turnFact, bool isBraking, 
			double paddling_factor, double speed,
			TVector3 net_force, double flap_factor) {
    double turning_angle[2] = {0, 0};
    double paddling_angle = 0;
    double ext_paddling_angle = 0; 
    double kick_paddling_angle = 0;
    double braking_angle = 0;
    double force_angle = 0;
    double turn_leg_angle = 0;
    double flap_angle = 0;

    if (isBraking) braking_angle = MAX_ARM_ANGLE2;

    paddling_angle = MAX_PADDLING_ANGLE2 * sin(paddling_factor * M_PI);
    ext_paddling_angle = MAX_EXT_PADDLING_ANGLE2 * sin(paddling_factor * M_PI);
    kick_paddling_angle = MAX_KICK_PADDLING_ANGLE2 * sin(paddling_factor * M_PI * 2.0);

    turning_angle[0] = MAX(-turnFact,0.0) * MAX_ARM_ANGLE2;
    turning_angle[1] = MAX(turnFact,0.0) * MAX_ARM_ANGLE2;
    flap_angle = MAX_ARM_ANGLE2 * (0.5 + 0.5 * sin (M_PI * flap_factor * 6 - M_PI / 2));
    force_angle = max (-20.0, min (20.0, -net_force.z / 300.0));
    turn_leg_angle = turnFact * 10;
    
	ResetJoints ();

    RotateNode ("left_shldr", 3, 
		    MIN (braking_angle + paddling_angle + turning_angle[0], MAX_ARM_ANGLE2) + flap_angle);
    RotateNode ("right_shldr", 3,
		    MIN (braking_angle + paddling_angle + turning_angle[1], MAX_ARM_ANGLE2) + flap_angle);

    RotateNode ("left_shldr", 2, -ext_paddling_angle);
    RotateNode ("right_shldr", 2, ext_paddling_angle);
    RotateNode ("left_hip", 3, -20 + turn_leg_angle + force_angle);
    RotateNode ("right_hip", 3, -20 - turn_leg_angle + force_angle);
	
    RotateNode ("left_knee", 3, 
		-10 + turn_leg_angle - MIN (35, speed) + kick_paddling_angle + force_angle);
    RotateNode ("right_knee", 3, 
		-10 - turn_leg_angle - MIN (35, speed) - kick_paddling_angle + force_angle);

    RotateNode ("left_ankle", 3, -20 + MIN (50, speed));
    RotateNode ("right_ankle", 3, -20 + MIN (50, speed));
    RotateNode ("tail", 3, turnFact * 20);
    RotateNode ("neck", 3, -50);
    RotateNode ("head", 3, -30);
    RotateNode ("head", 2, -turnFact * 70);
}

// --------------------------------------------------------------------
//				collision
// --------------------------------------------------------------------

bool CChar::CheckPolyhedronCollision (TCharNode *node, TMatrix modelMatrix, 
		TMatrix invModelMatrix, TPolyhedron ph) {

    TMatrix newModelMatrix, newInvModelMatrix;
    TCharNode *child;
    TPolyhedron newph;
    bool hit = false;

    MultiplyMatrices (newModelMatrix, modelMatrix, node->trans);
    MultiplyMatrices (newInvModelMatrix, node->invtrans, invModelMatrix);

    if  (node->visible) {
        newph = CopyPolyhedron (ph);
        TransPolyhedron (newInvModelMatrix, newph);
        hit = IntersectPolyhedron (newph);
        FreePolyhedron (newph);
    } 

    if (hit == true) return hit;
    child = node->child;
    while (child != NULL) {
        hit = CheckPolyhedronCollision (child, newModelMatrix, newInvModelMatrix, ph);
        if  (hit == true) return hit;
        child = child->next;
    } 
    return false;
}

bool CChar::CheckCollision (TPolyhedron ph) {
    TCharNode *node;
    TMatrix mat, invmat;

    MakeIdentityMatrix (mat);
    MakeIdentityMatrix (invmat);

	if (GetNode (0, &node) == false) return false;
    return CheckPolyhedronCollision (node, mat, invmat, ph);
} 

bool CChar::Collision (TVector3 pos, TPolyhedron ph) {
	ResetNode (0);
	TranslateNode (0, MakeVector (pos.x, pos.y, pos.z));	
	return CheckCollision (ph);
}

// --------------------------------------------------------------------
//				shadow
// --------------------------------------------------------------------

void CChar::DrawShadowVertex (double x, double y, double z, TMatrix mat) {
    TVector3 pt;
    double old_y;
    TVector3 nml;

    pt = MakeVector (x, y, z);
    pt = TransformPoint (mat, pt);
    old_y = pt.y;
    nml = Course.FindCourseNormal (pt.x, pt.z);
    pt.y = Course.FindYCoord (pt.x, pt.z) + SHADOW_HEIGHT;
    if  (pt.y > old_y) pt.y = old_y;
    glNormal3f (nml.x, nml.y, nml.z);
    glVertex3f (pt.x, pt.y, pt.z);
}

void CChar::DrawShadowSphere (TMatrix mat) {
    double theta, phi, d_theta, d_phi, eps, twopi;
    double x, y, z;
    int div = param.tux_shadow_sphere_divisions;
    
    eps = 1e-15;
    twopi = M_PI * 2.0;
    d_theta = d_phi = M_PI / div;

    for  (phi = 0.0; phi + eps < M_PI; phi += d_phi) {
		double cos_theta, sin_theta;
		double sin_phi, cos_phi;
		double sin_phi_d_phi, cos_phi_d_phi;

		sin_phi = sin (phi);
		cos_phi = cos (phi);
		sin_phi_d_phi = sin (phi + d_phi);
		cos_phi_d_phi = cos (phi + d_phi);
        
        if  (phi <= eps) {
			glBegin (GL_TRIANGLE_FAN);
				DrawShadowVertex (0., 0., 1., mat);

				for  (theta = 0.0; theta + eps < twopi; theta += d_theta) {
					sin_theta = sin (theta);
					cos_theta = cos (theta);

					x = cos_theta * sin_phi_d_phi;
					y = sin_theta * sin_phi_d_phi;
					z = cos_phi_d_phi;
					DrawShadowVertex (x, y, z, mat);
				} 
				x = sin_phi_d_phi;
				y = 0.0;
				z = cos_phi_d_phi;
				DrawShadowVertex (x, y, z, mat);
            glEnd();
        } else if  (phi + d_phi + eps >= M_PI) {
			glBegin (GL_TRIANGLE_FAN);
				DrawShadowVertex (0., 0., -1., mat);
                for  (theta = twopi; theta - eps > 0; theta -= d_theta) {
					sin_theta = sin (theta);
					cos_theta = cos (theta);
                    x = cos_theta * sin_phi;
                    y = sin_theta * sin_phi;
                    z = cos_phi;
					DrawShadowVertex (x, y, z, mat);
                } 
                x = sin_phi;
                y = 0.0;
                z = cos_phi;
				DrawShadowVertex (x, y, z, mat);
            glEnd();
        } else {
            glBegin (GL_TRIANGLE_STRIP);
				for (theta = 0.0; theta + eps < twopi; theta += d_theta) {
					sin_theta = sin (theta);
					cos_theta = cos (theta);
					x = cos_theta * sin_phi;
					y = sin_theta * sin_phi;
					z = cos_phi;
					DrawShadowVertex (x, y, z, mat);

					x = cos_theta * sin_phi_d_phi;
					y = sin_theta * sin_phi_d_phi;
					z = cos_phi_d_phi;
					DrawShadowVertex (x, y, z, mat);
                } 
                x = sin_phi;
                y = 0.0;
                z = cos_phi;
				DrawShadowVertex (x, y, z, mat);
                x = sin_phi_d_phi;
                y = 0.0;
                z = cos_phi_d_phi;
				DrawShadowVertex (x, y, z, mat);
            glEnd();
        } 
    } 
} 

void CChar::TraverseDagForShadow (TCharNode *node, TMatrix mat) {
    TMatrix new_matrix;
    TCharNode *child;

    MultiplyMatrices (new_matrix, mat, node->trans);
	if (node->visible && node->render_shadow)
		DrawShadowSphere (new_matrix);

    child = node->child;
    while (child != NULL) {
        TraverseDagForShadow (child, new_matrix);
        child = child->next;
    } 
}

void CChar::DrawShadow () {
    TMatrix model_matrix;
    TCharNode *node;

	if (g_game.light_id == 1 || g_game.light_id == 3) return;

    set_gl_options (TUX_SHADOW); 
    glColor4f (shad_col.r, shad_col.g, shad_col.b, shad_col.a);
    MakeIdentityMatrix (model_matrix);

    if (GetNode (0, &node) == false) {
		Message ("couldn't find tux's root node", "");
		return;
    } 
    TraverseDagForShadow (node, model_matrix);
}

// --------------------------------------------------------------------
//				testing and tools
// --------------------------------------------------------------------

string CChar::GetNodeName (int idx) {
	if (idx < 0 || idx >= numNodes) return "";
	TCharNode *node = Nodes[idx];
	if (node == NULL) return "";
	if (node->joint.size() > 0) return node->joint;
	else return Int_StrN (node->node_name);
}

void CChar::RefreshNode (int idx) {
	if (idx < 0 || idx >= numNodes) return;
    TMatrix TempMatrix;
	char caxis;
	double angle;
	
	TCharNode *node = Nodes[idx];
	TAction *act = Actions[idx];
	if (act == NULL) return;
	if (act->num < 1) return;

	MakeIdentityMatrix (node->trans);
	MakeIdentityMatrix (node->invtrans);

	int type;
	TVector3 vec;
	double dval;
	for (int i=0; i<act->num; i++) {
		type = act->type[i];
		vec = act->vec[i];
		dval = act->dval[i];

		switch (type) {
			case 0: 
				MakeTranslationMatrix (TempMatrix, vec.x, vec.y, vec.z);	
				MultiplyMatrices (node->trans, node->trans, TempMatrix);
				MakeTranslationMatrix (TempMatrix, -vec.x, -vec.y, -vec.z);
				MultiplyMatrices (node->invtrans, TempMatrix, node->invtrans);
				break;
			case 1:
				caxis = 'x';
				angle = dval;
				MakeRotationMatrix (TempMatrix, angle, caxis);
				MultiplyMatrices (node->trans, node->trans, TempMatrix);
				MakeRotationMatrix (TempMatrix, -angle, caxis);
				MultiplyMatrices (node->invtrans, TempMatrix, node->invtrans);
				break;
			case 2:
				caxis = 'y';
				angle = dval;
				MakeRotationMatrix (TempMatrix, angle, caxis);
				MultiplyMatrices (node->trans, node->trans, TempMatrix);
				MakeRotationMatrix (TempMatrix, -angle, caxis);
				MultiplyMatrices (node->invtrans, TempMatrix, node->invtrans);
				break;
			case 3:
				caxis = 'z';
				angle = dval;
				MakeRotationMatrix (TempMatrix, angle, caxis);
				MultiplyMatrices (node->trans, node->trans, TempMatrix);
				MakeRotationMatrix (TempMatrix, -angle, caxis);
				MultiplyMatrices (node->invtrans, TempMatrix, node->invtrans);
				break;
			case 4:
				MakeIdentityMatrix (TempMatrix);
				MultiplyMatrices (node->trans, node->trans, TempMatrix);
				MakeIdentityMatrix (TempMatrix);
				MultiplyMatrices (node->invtrans, TempMatrix, node->invtrans);

				MakeScalingMatrix (TempMatrix, vec.x, vec.y, vec.z);
				MultiplyMatrices (node->trans, node->trans, TempMatrix);
				MakeScalingMatrix (TempMatrix, 1.0 / vec.x, 1.0 / vec.y, 1.0 / vec.z);
				MultiplyMatrices (node->invtrans, TempMatrix, node->invtrans);

				MakeIdentityMatrix (TempMatrix);
				MultiplyMatrices (node->trans, node->trans, TempMatrix);
				MakeIdentityMatrix (TempMatrix);
				MultiplyMatrices (node->invtrans, TempMatrix, node->invtrans);
				break;
			case 5: 
				VisibleNode (node->node_name, dval); break;
			default: break;
		}
	}
}

int CChar::GetNumNodes () {
	return numNodes;
}

string CChar::GetNodeFullname (int idx) {
	if (idx < 0 || idx >= numNodes) return "";
	return Actions[idx]->name;
}

int CChar::GetNumActs (int idx) {
	if (idx < 0 || idx >= numNodes) return -1;
	return Actions[idx]->num;
}

TAction *CChar::GetAction (int idx) {
	if (idx < 0 || idx >= numNodes) return NULL;
	return Actions[idx];
}

void CChar::PrintAction (int idx) {
	if (idx < 0 || idx >= numNodes) return;
	TAction *act = Actions[idx];
	PrintInt (act->num);
	for (int i=0; i<act->num; i++) {
		PrintInt (act->type[i]);
		PrintDouble (act->dval[i]);
		PrintVector (act->vec[i]);
	}
}

void CChar::PrintNode (int idx) {
	TCharNode *node = Nodes[idx];
	PrintInt (node->node_name);
	PrintInt (node->parent_name);
}

void CChar::SaveCharNodes () {
	CSPList list (MAX_CHAR_NODES + 10);
	string line, order;
	TCharNode *node;
	TAction *act;
	int  i, ii, aa;
	TVector3 rotation;
	bool rotflag;

	list.Add ("# Generated by Tuxracer tools");
	list.Add ("");
	if (numMatlines > 0) {
		list.Add ("# Materials:");
		for (i=0; i<numMatlines; i++) list.Add (Matlines[i]);
		list.Add ("");
	}

	list.Add ("# Nodes:");
	for (i=1; i<numNodes; i++) {
		node = Nodes[i];
		act = Actions[i];
		if (node->parent_name >= node->node_name) Message ("wrong parent index");
		line = "*[node] " + Int_StrN (node->node_name);
		line += " [par] " + Int_StrN (node->parent_name); 
		order = act->order;
		rotation = NullVec;
		rotflag = false;

		if (order.size() > 0) {
			line += " [order] " + order;
			for (ii=0; ii<(int)order.size(); ii++) {
				aa = order.at(ii)-48;	
				switch (aa) {
					case 0: line += " [trans] " + Vector_StrN (act->vec[ii], 2); break;
					case 4: line += " [scale] " + Vector_StrN (act->vec[ii], 2); break;
 					case 1: rotation.x = act->dval[ii]; rotflag = true; break;
 					case 2: rotation.y = act->dval[ii]; rotflag = true; break;
 					case 3: rotation.z = act->dval[ii]; rotflag = true; break;
					case 5: line += " [vis] " + Float_StrN (act->dval[ii], 0); break;
					case 9: rotation.z = act->dval[ii]; rotflag = true; break;
				}
			}
			if (rotflag) line += " [rot] " + Vector_StrN (rotation, 2);
		}
		if (act->mat.size() > 0) line += " [mat] " + act->mat;
		if (node->joint.size() > 0) line += " [joint] " + node->joint;
		if (act->name.size() > 0) line += " [name] " + act->name;
		if (node->render_shadow) line += " [shad] 1";
		
		list.Add (line);
	}	
	list.Save (param.char_dir, "test.lst");
}
