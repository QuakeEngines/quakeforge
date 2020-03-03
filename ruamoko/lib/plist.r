#include <plist.h>

plitem_t PL_GetFromFile (QFile file) = #0;
plitem_t PL_GetPropertyList (string str) = #0;
string PL_WritePropertyList (plitem_t pl) = #0;
pltype_t PL_Type (plitem_t str) = #0;
string PL_String (plitem_t str) = #0;
plitem_t PL_ObjectForKey (plitem_t item, string key) = #0;
plitem_t PL_RemoveObjectForKey (plitem_t item, string key) = #0;
plitem_t PL_ObjectAtIndex (plitem_t item, int index) = #0;
plitem_t PL_D_AllKeys (plitem_t item) = #0;
int PL_D_NumKeys (plitem_t item) = #0;
int PL_D_AddObject (plitem_t dict, string key, plitem_t value) = #0;
int PL_A_AddObject (plitem_t array_item, plitem_t item) = #0;
int PL_A_NumObjects (plitem_t item) = #0;
int PL_A_InsertObjectAtIndex (plitem_t array_item, plitem_t item, int index) = #0;
plitem_t PL_RemoveObjectAtIndex (plitem_t array_item, int index) = #0;
plitem_t PL_NewDictionary (void) = #0;
plitem_t PL_NewArray (void) = #0;
plitem_t PL_NewData (void *data, int len) = #0;
plitem_t PL_NewString (string str) = #0;
void PL_Free (plitem_t pl) = #0;
