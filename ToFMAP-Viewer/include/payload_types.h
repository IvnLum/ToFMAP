#ifndef __PAYLOAD_TYPES__
#define __PAYLOAD_TYPES__

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct Vehicle_data_t
{
  int loc_x, loc_y;
  int dir_x, dir_y;
  int inc, azm;
  int dist;
  char batt;  
} Vehicle_data_t;

typedef struct Controller_udata_t
{
    char r2     : 1;
    char cross  : 1;
    char lx     : 2;
    char ly     : 2;
} Controller_udata_t;

Vehicle_data_t get_Vehicle_data_t(  
    int loc_x,  
    int loc_y,  
    int dir_x,  
    int dir_y,  
    int inc,  
    int azm,  
    int dist,  
    char batt);

void print_Vehicle_data_t(Vehicle_data_t t);
void print_Controller_udata_t(Controller_udata_t data);

#ifdef __cplusplus
}
#endif

#endif /* !__PAYLOAD_TYPES__ */
