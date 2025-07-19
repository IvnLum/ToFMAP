#include "payload_types.h"
#include <stdio.h>

Vehicle_data_t get_Vehicle_data_t(  
    int loc_x,  
    int loc_y,  
    int dir_x,  
    int dir_y,  
    int inc,  
    int azm,  
    int dist,  
    char batt)

{                       
  return (Vehicle_data_t) {
    .loc_x = loc_x,
    .loc_y = loc_y,
    .dir_x = dir_x,
    .dir_y = dir_y,
    .inc = inc,
    .azm = azm,
    .dist = dist,
    .batt = batt
  };                       
}

void print_Vehicle_data_t(Vehicle_data_t t)  
{                                        
  printf("loc_x: %d\n", t.loc_x);
  printf("loc_y: %d\n", t.loc_y);
  printf("dir_x: %d\n", t.dir_x);
  printf("dir_y: %d\n", t.dir_y);
  printf("inc: %d\n", t.inc);
  printf("azm: %d\n", t.azm);
  printf("dist: %d\n", t.dist);
  printf("bateria: %d\n", t.batt);
} 

void print_Controller_udata_t(Controller_udata_t data)
{
	printf("%s", data.r2?"[r2]":" r2 ");
  printf("%s", data.cross?"[X]":" X ");
	printf("%d", data.lx);
  printf("%d", data.ly);
	puts("");
}
