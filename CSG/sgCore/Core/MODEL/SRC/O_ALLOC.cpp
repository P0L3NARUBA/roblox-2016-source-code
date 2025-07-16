#include "../../sg.h"
DWORD alloc_obj_count = 0;

hOBJ o_alloc(OBJTYPE type)
{
  lpOBJ obj;
  short onum;
  WORD  size;

  onum = otype_num(type);
  if ( onum == -1 ) return nullptr;

  size = sizeof(OBJ) + geo_size[onum] - 1;

	if ((obj = (lpOBJ)SGMalloc(size)) == nullptr) 	return nullptr;
  memset(obj, 0 ,size);               //  
  obj->type   = type;
	if ( check_type(struct_filter,type) && obj_attrib.flg_transp )
      obj->color  = CTRANSP;
  else
      obj->color  = obj_attrib.color;
	obj->layer  = obj_attrib.layer;
	obj->ltype  = obj_attrib.ltype;
	obj->lthickness  = obj_attrib.lthickness;

	SetModifyFlag(TRUE);			//   
  alloc_obj_count++;
	return (hOBJ)obj;
}
