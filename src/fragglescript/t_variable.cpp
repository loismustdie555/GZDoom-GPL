// Copyright(C) 2000 Simon Howard, 2002-2008 Christoph Oelckers
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// Variables.
//
// Variable code: create new variables, look up variables, get value,
// set value
//
// variables are stored inside the individual scripts, to allow for
// 'local' and 'global' variables. This way, individual scripts cannot
// access variables in other scripts. However, 'global' variables can
// be made which can be accessed by all scripts. These are stored inside
// a dedicated DFsScript which exists only to hold all of these global
// variables.
//
// functions are also stored as variables, these are kept in the global
// script so they can be accessed by all scripts. function variables
// cannot be set or changed inside the scripts themselves.
//
//---------------------------------------------------------------------------
//
// FraggleScript is from SMMU which is under the GPL. Technically, 
// therefore, combining the FraggleScript code with the non-free 
// ZDoom code is a violation of the GPL.
//
// As this may be a problem for you, I hereby grant an exception to my 
// copyright on the SMMU source (including FraggleScript). You may use 
// any code from SMMU in (G)ZDoom, provided that:
//
//    * For any binary release of the port, the source code is also made 
//      available.
//    * The copyright notice is kept on any file containing my code.


#include "t_script.h"
#include "a_pickups.h"
#include "farchive.h"


//==========================================================================
//
//
//
//==========================================================================

int intvalue(const svalue_t &v)
{
	return (v.type == svt_string ? atoi(v.string) :       
	v.type == svt_fixed ? (int)(v.value.f / FRACUNIT) : 
	v.type == svt_mobj ? -1 : v.value.i );
}

//==========================================================================
//
//
//
//==========================================================================

fixed_t fixedvalue(const svalue_t &v)
{
	return (v.type == svt_fixed ? v.value.f :
	v.type == svt_string ? (fixed_t)(atof(v.string) * FRACUNIT) :
	v.type == svt_mobj ? -1*FRACUNIT : v.value.i * FRACUNIT );
}

//==========================================================================
//
//
//
//==========================================================================

double floatvalue(const svalue_t &v)
{
	return 
		v.type == svt_string ? atof(v.string) :       
		v.type == svt_fixed ? FIXED2DBL(v.value.f) : 
		v.type == svt_mobj ? -1. : (double)v.value.i;
}

//==========================================================================
//
// sf: string value of an svalue_t
//
//==========================================================================

const char *stringvalue(const svalue_t & v)
{
	static char buffer[256];
	
	switch(v.type)
    {
	case svt_string:
		return v.string;
		
	case svt_mobj:
		// return the class name
		return (const char *)v.value.mobj->GetClass()->TypeName;
		
	case svt_fixed:
		{
			double val = ((double)v.value.f) / FRACUNIT;
			mysnprintf(buffer, countof(buffer), "%g", val);
			return buffer;
		}
		
	case svt_int:
	default:
        mysnprintf(buffer, countof(buffer), "%i", v.value.i);
		return buffer;	
    }
}

//==========================================================================
//
//
//==========================================================================

AActor* actorvalue(const svalue_t &svalue)
{
	int intval;

	if(svalue.type == svt_mobj) 
	{
		// Inventory items in the player's inventory have to be considered non-present.
		if (svalue.value.mobj != NULL && 
			svalue.value.mobj->IsKindOf(RUNTIME_CLASS(AInventory)) && 
			static_cast<AInventory*>(svalue.value.mobj)->Owner != NULL)
		{
			return NULL;
		}

		return svalue.value.mobj;
	}
	else
	{
		TArray<TObjPtr<AActor> > &SpawnedThings = DFraggleThinker::ActiveThinker->SpawnedThings;
		// this requires some creativity. We use the intvalue
		// as the thing number of a thing in the level.
		intval = intvalue(svalue);
		
		if(intval < 0 || intval >= (int)SpawnedThings.Size())
		{ 
			return NULL;
		}
		// Inventory items in the player's inventory have to be considered non-present.
		if (SpawnedThings[intval] != NULL &&
			SpawnedThings[intval]->IsKindOf(RUNTIME_CLASS(AInventory)) && 
			barrier_cast<AInventory*>(SpawnedThings[intval])->Owner != NULL)
		{
			return NULL;
		}

		return SpawnedThings[intval];
	}
}

//==========================================================================
//
//
//==========================================================================

IMPLEMENT_POINTY_CLASS(DFsVariable)
 DECLARE_POINTER (next)
 DECLARE_POINTER (actor)
END_POINTERS

//==========================================================================
//
//
//==========================================================================

DFsVariable::DFsVariable(const char * _name)
{
	Name=_name;
	type=svt_int;
	actor = NULL;
	value.i=0;
	next=NULL;
}

//==========================================================================
//
// returns an svalue_t holding the current
// value of a particular variable.
//
//==========================================================================

void DFsVariable::GetValue(svalue_t &returnvar)
{
	switch (type)
	{
	case svt_pInt:
		returnvar.type = svt_int;
		returnvar.value.i = *value.pI;
		break;

	case svt_pMobj:
		returnvar.type = svt_mobj;
		returnvar.value.mobj = *value.pMobj;
		break;

	case svt_mobj:
		returnvar.type = type;
		returnvar.value.mobj = actor;
		break;

	case svt_linespec:
		returnvar.type = svt_int;
		returnvar.value.i = value.ls->number;
		break;

	case svt_string:
		returnvar.type = type;
		returnvar.string = string;
		break;

	default:
		// copy the value (also handles fixed)
		returnvar.type = type;
		returnvar.value.i = value.i;
		break;
    }
}


//==========================================================================
//
// set a variable to a value from an svalue_t
//
//==========================================================================

void DFsVariable::SetValue(const svalue_t &newvalue)
{
	if(type == svt_const)
    {
		// const adapts to the value it is set to
		type = newvalue.type;
    }

	switch (type)
	{
	case svt_int:
		value.i = intvalue(newvalue);
		break;

	case svt_string:
		if (newvalue.type == svt_string)
		{
			string = newvalue.string;
		}
		else
		{
			string = stringvalue(newvalue);
		}
		break;

	case svt_fixed:
		value.fixed = fixedvalue(newvalue);
		break;
	
	case svt_mobj:
		actor = actorvalue(newvalue);
		break;
	
	case svt_pInt:
		*value.pI = intvalue(newvalue);
		break;
	
	case svt_pMobj:
		*value.pMobj = actorvalue(newvalue);
		break;
	
	case svt_function:
		script_error("attempt to set function to a value\n");
		break;

	default:
		script_error("invalid variable type\n");
		break;
	}
}

//==========================================================================
//
// Archive one script variable
//
//==========================================================================

void DFsVariable::Serialize(FArchive & ar)
{
	Super::Serialize(ar);
	ar << Name << type << string << actor << value.i << next;
}


//==========================================================================
//
// From here: variable related functions inside DFsScript
//
//==========================================================================
//==========================================================================
//
// create a new variable in a particular script.
// returns a pointer to the new variable.
//
//==========================================================================

DFsVariable *DFsScript::NewVariable(const char *name, int vtype)
{
	DFsVariable *newvar = new DFsVariable(name);
	newvar->type = vtype;
	
	int n = variable_hash(name);
	newvar->next = variables[n];
	variables[n] = newvar;
	GC::WriteBarrier(this, newvar);
	return newvar;
}


void DFsScript::NewFunction(const char *name, void (FParser::*handler)() )
{
	NewVariable (name, svt_function)->value.handler = handler;
}

//==========================================================================
//
// search a particular script for a variable, which
// is returned if it exists
//
//==========================================================================

DFsVariable *DFsScript::VariableForName(const char *name)
{
	int n = variable_hash(name);
	DFsVariable *current = variables[n];
	
	while(current)
    {
		if(!strcmp(name, current->Name))        // found it?
			return current;         
		current = current->next;        // check next in chain
    }
	
	return NULL;
}


//==========================================================================
//
// find_variable checks through the current script, level script
// and global script to try to find the variable of the name wanted
//
//==========================================================================

DFsVariable *DFsScript::FindVariable(const char *name)
{
	DFsVariable *var;
	DFsScript *current = this;
	
	while(current)
    {
		// check this script
		if ((var = current->VariableForName(name)))
			return var;
		current = current->parent;    // try the parent of this one
    }
	
	return NULL;    // no variable
}


//==========================================================================
//
// free all the variables in a given script
//
//==========================================================================

void DFsScript::ClearVariables(bool complete)
{
	int i;
	DFsVariable *current, *next;
	
	for(i=0; i<VARIABLESLOTS; i++)
    {
		current = variables[i];
		
		// go thru this chain
		while(current)
		{
			// labels are added before variables, during
			// preprocessing, so will be at the end of the chain
			// we can be sure there are no more variables to free
			if(current->type == svt_label && !complete) break;
			
			next = current->next; // save for after freeing
			
			current->Destroy();
			current = next; // go to next in chain
		}
		// start of labels or NULL
		variables[i] = current;
    }
}

//==========================================================================
//
//
//
//==========================================================================

char *DFsScript::LabelValue(const svalue_t &v)
{
	if (v.type == svt_label) return data + v.value.i;
	else return NULL;
}


