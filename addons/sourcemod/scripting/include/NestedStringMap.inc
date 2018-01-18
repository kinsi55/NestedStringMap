/*
	NestedStringMap.inc - StringMaps with built in nesting and traversal functionality
	Copyright (C) 2018 Kinsi

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
 
#if defined _NestedStringMap_included
 #endinput
#endif
#define _NestedStringMap_included

//#define DEBUG_NESTEDSTRINGMAP

//Special chars that are pretty unlikely to be used in that order by.. you?
#define SM_PARENT_PREF "NSMP~#§°_"
#define SM_CHILDTHIS_PREF "NSMT~#§°_"
#define SM_CHILD_PREF "NSMC~#§°_"
#define SM_ENUM_PREF "NSME~#§°_"

enum NSP_ITERATION {
	Iterator_Inherit, 	//Only enable iteration if parent has done so
	Iterator_Disable, 	//Disable iteration
	Iterator_Enable		//Force iteration even if parent doesnt have it
};

//*not* making it a Handle to prevent people from trying to close it and thus breaking stuff.
methodmap NestedStringMapChildren {
	public NestedStringMapChildren(ArrayList AL = null) {
		//If we call NestedStringMapChildren with no value we handle it like a Constructor
		if(AL == null)
			AL = new ArrayList();
			
		return view_as<NestedStringMapChildren>(AL);
	}
	
	/**
	 * The amount of children in the associated NestedStringMap
	 *
	 * @return						Amount of children
	 */
	property int Length {
		public get() {
			//Index 0 is a reference to the (Nested)StringMap we belong to
			//so we must subtract 1 from the real array length.
			return view_as<ArrayList>(this).Length - 1;
		}
	}
	
	/**
	 * Get a child based off its index. Due to SourceMod limitations i cannot properly return
	 * a NestedStringMap, so you will need to view_as<NestedStringMap>(retval) yourself
	 *
     * @param index					Index of child to get
	 * @return						Underlying StringMap(!) of NestedStringMap
	 */
	public StringMap GetChild(int index) {
		if(index > this.Length || index < 0)
			ThrowError("Child with index %i does not exist", index);

		return view_as<ArrayList>(this).Get(index + 1);
	}
}

methodmap NestedStringMap < StringMap {
	/**
	 * Create new NestedStringMap.
	 *
	 * @param enableIteration		Enable iteration of children.
	 * @param maxChildNameLength	The maximum length a child's name can have. Needs to be only respected if used with iteration.
	 * @return						New NestedStringMap.
	 */
	public NestedStringMap(bool enableIteration = false) {
		NestedStringMap NSM = view_as<NestedStringMap>(new StringMap());
		
		if(enableIteration) {
			NestedStringMapChildren iterator = NestedStringMapChildren();
			//Index 0 of the ArrayList is a reference to this NSM
			view_as<ArrayList>(iterator).Push(NSM);
		
			NSM.SetValue(SM_ENUM_PREF, iterator);
		}
	
		return NSM;
	}
	
	/**
	 * Get the name of this NestedStringMap if it is a child
	 *
	 * @param childName		The buffer to set to the matching substring.
	 * @param maxlen		The maximum string length of the buffer.
	 * @return				True if self is a child, false if not.
	 */
	public bool GetName(char[] childName, int maxlen) {
		if(this.GetString(SM_CHILDTHIS_PREF, childName, maxlen)) {
			//Remove internal prefix from returned name
			strcopy(childName, maxlen, childName[strlen(SM_CHILD_PREF)]);
			
			return true;
		}
		return false;
	}
	
	/**
	 * Get a NestedStringMapChildren object for this NestedStringMap
	 *
	 * @param &childrenKeys			NestedStringMapChildren object or null
	 * @return						True if iteration is enabled
	 */
	public bool GetIterator(NestedStringMapChildren &childrenKeys) {
		return this.GetValue(SM_ENUM_PREF, childrenKeys);
	}
	
	/**
	 * Add an existing NestedStringMap with no parent as a child to this NestedStringMap.
	 *
	 * @param childToAdd			NestedStringMap to Add.
	 * @param childName				Name to give the children.
	 */
	public void AddChild(NestedStringMap childToAdd, const char[] childName) {
		int strLen = strlen(childName);
	
		if(strLen == 0)
			ThrowError("childName cannot be empty");
		
		char[] nestedKey = new char[strLen + 16];
		
		Format(nestedKey, strLen + 16, "%s%s", SM_CHILD_PREF, childName);
		
		if(!childToAdd.SetValue(SM_PARENT_PREF, this) ||
			!childToAdd.SetString(SM_CHILDTHIS_PREF, nestedKey))
			ThrowError("Assigning a child NestedStringMap to multiple Parents is not supported");
		
		this.SetValue(nestedKey, childToAdd);
		
		NestedStringMapChildren cc;
		
		//We push ourself into the Iterator array in position 1 to have a reference back to us within the NestedStringMapChildren
		if(this.GetIterator(cc))
			view_as<ArrayList>(cc).Push(childToAdd);
		
		#if defined DEBUG_NESTEDSTRINGMAP
			LogMessage("Added Children with name %s(%i) to self (%i)", childName, childToAdd, this);
		#endif
	}
	
	/**
	 * Get a child NestedStringMap of this Parent if it exists, otherwise create a new one.
	 *
	 * @param childName				Name of the children.
	 * @param iteration				If we should Inherit, Disable, or Force iteration in this child. Only used if child doesnt exist.
	 * @return						Found or newly created child NestedStringMap.
	 */
	public NestedStringMap Child(const char[] childName, NSP_ITERATION iteration = Iterator_Inherit) {
		int strLen = strlen(childName);
	
		if(strLen == 0)
			ThrowError("childName cannot be empty");
	
		NestedStringMap child;
		
		char[] nestedKey = new char[strLen + 16];
		
		Format(nestedKey, strLen + 16, "%s%s", SM_CHILD_PREF, childName);
		
		if(!this.GetValue(nestedKey, child)) {
			bool bI = iteration == Iterator_Enable;
			
			if(!bI && iteration == Iterator_Inherit) {
				NestedStringMapChildren cc;
				
				bI = this.GetIterator(cc);
			}
			
			child = new NestedStringMap(bI);
			
			this.AddChild(child, childName);
		}
		
		return child;
	}
	
	/**
	 * Get the parent of this NestedStringMap.
	 *
	 * @param errorIfTopLevel		Wether we should error if there is no parent or just return null instead.
	 * @return						Parent NestedStringMap or null.
	 */
	public NestedStringMap Parent(bool errorIfTopLevel = true) {
		NestedStringMap parent;
		
		if(!this.GetValue(SM_PARENT_PREF, parent)) {
			if(errorIfTopLevel)
				ThrowError("NestedStringMap has no Parent");
				
			return view_as<NestedStringMap>(null);
		}
		
		return parent;
	}
	
	/**
	 * Get and return value in Map.
	 *
	 * @param key				Key string.
	 * @param defaultValue		Value to return if key is not set.
	 * @return					Value of key, or defaultValue if not existing.
	 */
	public int GetAndReturnValue(const char[] key, int defaultValue = -1) {
		this.GetValue(key, defaultValue);
		
		return defaultValue;
	}
	
	/**
	 * Close all children NestedStringMap's and afterwards close self.
	 */
	public void Close() { 
		NestedStringMapClose(this);
	}
}

//Needed to break out this function because when calling .Close on a NestedStringMap while
//being in the Close function itself It would not use my function but the default one.
static void NestedStringMapClose(NestedStringMap NSM) {
	char keyName[255];
	NestedStringMap otherNSM;
	
	#if defined DEBUG_NESTEDSTRINGMAP
		LogMessage("");
		LogMessage("[%i] Closing NestedStringMap...", NSM);
	#endif
	
	NestedStringMapChildren NSPC;
	
	//If we have Iteration enabled we might as well make use of it
	if(NSM.GetIterator(NSPC)) {
		//Removing our own Iterator before removing our own children to save unnecessary operations
		NSM.Remove(SM_ENUM_PREF);
		
		for(int i = 0; i < NSPC.Length; i++) {
			otherNSM = view_as<NestedStringMap>(NSPC.GetChild(i));
			
			NestedStringMapClose(otherNSM);
			
			#if defined DEBUG_NESTEDSTRINGMAP
				LogMessage("[%i] Closed nested Children ?... (%i)", NSM, otherNSM);
			#endif
		}
		
		delete view_as<ArrayList>(NSPC);
	} else {
		StringMapSnapshot stSn = NSM.Snapshot();
	
		for(int i = 0; i < stSn.Length; i++) {
			stSn.GetKey(i, keyName, sizeof(keyName));
			
			if(StrContains(keyName, SM_CHILD_PREF) == 0) {
				NSM.GetValue(keyName, otherNSM);
				
				NestedStringMapClose(otherNSM);
				
				#if defined DEBUG_NESTEDSTRINGMAP
					LogMessage("[%i] Closed nested Children %s... (%i)", NSM, keyName, otherNSM);
				#endif
			}
		}
	}
	
	if(NSM.GetValue(SM_PARENT_PREF, otherNSM)) {
		NSM.GetString(SM_CHILDTHIS_PREF, keyName, sizeof(keyName));
		otherNSM.Remove(keyName);
		
		#if defined DEBUG_NESTEDSTRINGMAP
			LogMessage("[%i] Cleared NestedStringMap Key %s in parent (%i)...", NSM, keyName, otherNSM);
		#endif
		
		//Remove ourself from our parents Iterator
		if(otherNSM.GetIterator(NSPC)) {
			int IteratorPos = view_as<ArrayList>(NSPC).FindValue(NSM);
			
			view_as<ArrayList>(NSPC).Erase(IteratorPos);
			
			#if defined DEBUG_NESTEDSTRINGMAP
				LogMessage("[%i] Cleared Self (%s) from parents iterator (Index %i) in parent (%i)...", NSM, keyName, IteratorPos, otherNSM);
			#endif
		}
	}
	
	#if defined DEBUG_NESTEDSTRINGMAP
		LogMessage("[%i] Deleting self...", NSM);
	#endif
	
	delete NSM;
}