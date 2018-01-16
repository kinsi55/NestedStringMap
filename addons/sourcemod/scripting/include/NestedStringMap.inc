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

methodmap NestedStringMap < StringMap {
	public NestedStringMap() {
		return view_as<NestedStringMap>(new StringMap());
	}
	
	/**
	 * Add an existing children with no parent to this NestedStringMap.
	 *
	 * @param childToAdd			NestedStringMap to Add.
	 * @param childName				Name to give the children.
	 */
	public void AddChild(NestedStringMap childToAdd, const char[] childName) {
		int strLen = strlen(childName);
	
		if(strLen == 0)
			ThrowError("childName cannot be empty");
		
		char[] nestedKey = new char[strLen + 32];
		
		Format(nestedKey, strLen + 32, "NestedStringMapChild__%s", childName);
		
		if(!childToAdd.SetValue("NestedStringMapParent__", this) ||
			!childToAdd.SetString("NestedStringMapThisKey__", nestedKey))
			ThrowError("Assigning a child NestedStringMap to multiple Parents is not supported");
		
		this.SetValue(nestedKey, childToAdd);
		
		#if defined DEBUG_NESTEDSTRINGMAP
			LogMessage("Added Children with name %s(%i) to self (%i)", childName, childToAdd, this);
		#endif
	}
	
	/**
	 * Get a child NestedStringMap of this Parent if it exists, otherwise create a new one.
	 *
	 * @param childName				Name of the children.
	 * @return						Found or newly created child NestedStringMap.
	 */
	public NestedStringMap Child(const char[] childName) {
		int strLen = strlen(childName);
	
		if(strLen == 0)
			ThrowError("childName cannot be empty");
	
		NestedStringMap child;
		
		char[] nestedKey = new char[strLen + 32];
		
		Format(nestedKey, strLen + 32, "NestedStringMapChild__%s", childName);
		
		if(!this.GetValue(nestedKey, child)) {
			child = new NestedStringMap();
			
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
		
		if(!this.GetValue("NestedStringMapParent__", parent)) {
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
	StringMapSnapshot stSn = NSM.Snapshot();
	
	char keyName[255];
	NestedStringMap otherNSM;
	
	#if defined DEBUG_NESTEDSTRINGMAP
		LogMessage("");
		LogMessage("[%i] Closing NestedStringMap...", NSM);
	#endif
	
	#if defined DEBUG_NESTEDSTRINGMAP
		if(stSn.Length > 2)
			PrintToServer("[%i] Closing NestedStringMap Children...", NSM);
	#endif
	
	for(int i = 0; i < stSn.Length; i++) {
		stSn.GetKey(i, keyName, sizeof(keyName));
		
		if(StrContains(keyName, "NestedStringMapChild__") == 0) {
			#if defined DEBUG_NESTEDSTRINGMAP
				PrintToServer("[%i] Closing nested Children %s...", NSM, keyName);
			#endif
		
			NSM.GetValue(keyName, otherNSM);
			
			NestedStringMapClose(otherNSM);
			
			#if defined DEBUG_NESTEDSTRINGMAP
				PrintToServer("[%i] Closed nested Children %s... (%i)", NSM, keyName, otherNSM);
			#endif
		}
	}
	
	if(NSM.GetValue("NestedStringMapParent__", otherNSM)) {
		NSM.GetString("NestedStringMapThisKey__", keyName, sizeof(keyName));
		otherNSM.Remove(keyName);
		
		#if defined DEBUG_NESTEDSTRINGMAP
			PrintToServer("[%i] Cleared NestedStringMap Key %s in parent (%i)...", NSM, keyName, otherNSM);
		#endif
	}
	
	#if defined DEBUG_NESTEDSTRINGMAP
		PrintToServer("[%i] Deleting self...", NSM);
	#endif
	
	#define delet delete
	delet NSM;
}