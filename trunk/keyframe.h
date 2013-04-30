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
---------------------------------------------------------------------*/

#ifndef KEYFRAME_H
#define KEYFRAME_H

#include "bh.h"
#include "tux.h"
#include <vector>

#define MAX_FRAME_VALUES 32

struct TKeyframe2 {
	double val[MAX_FRAME_VALUES];
};

class CKeyframe {
private:
	double keytime;
	vector<TKeyframe2> frames;
	TVector3 refpos;
	double heightcorr;
	int keyidx;
	string loadedfile;
	TKeyframe2 clipboard;

	double interp (double frac, double v1, double v2);
	void InterpolateKeyframe (size_t idx, double frac, CCharShape *shape);

	// test and editing
	void ResetFrame2 (TKeyframe2 *frame);
public:
	CKeyframe ();
	string jointname;
	bool loaded;

	bool active;
	void Init (const TVector3& ref_position, double height_correction);
	void Init (const TVector3& ref_position, double height_correction, CCharShape *shape);
	void InitTest (const TVector3& ref_position, CCharShape *shape);
	void Reset ();
	void Update (double timestep, CControl *ctrl);
	void UpdateTest (double timestep, CCharShape *shape);
	bool Load (const string& dir, const string& filename);
	void CalcKeyframe (size_t idx, CCharShape *shape, TVector3 refpos);

	// test and editing
	TKeyframe2 *GetFrame (size_t idx);
	static string GetHighlightName (size_t idx);
	static string GetJointName (size_t idx);
	int GetNumJoints () const;
	void SaveTest (const string& dir, const string& filename);
	void CopyFrame (size_t prim_idx, size_t sec_idx);
	void AddFrame ();
	size_t  DeleteFrame (size_t idx);
	void InsertFrame (size_t idx);
	void CopyToClipboard (size_t idx);
	void PasteFromClipboard (size_t idx);
	void ClearFrame (size_t idx);
	size_t numFrames() const { return frames.size(); }
};

extern CKeyframe TestFrame;

#endif
