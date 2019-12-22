#define _GNU_SOURCE // I need qsort_r here. Otherwise, I would have to create 4 comperator functions...
#include <primitives.h>
#include <dpaw.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

bool dpaw_in_rect(struct dpaw_rect rect, struct dpaw_point point){
  return rect.   top_left .x <= point.x
      && rect.bottom_right.x >  point.x
      && rect.   top_left .y <= point.y
      && rect.bottom_right.y >  point.y;
}

bool dpaw_point_equal(struct dpaw_point A, struct dpaw_point B){
  return A.x == B.x && A.y == B.y;
}

/**
 * \returns The section of the line inside the rectangle, this can't be a single point. Returns a zero filled line in case of no overlap.
 */
struct dpaw_line dpaw_line_clip(struct dpaw_rect rect, struct dpaw_line line){
  struct dpaw_line result = {.A={0,0},.B={0,0}};
  long dbax = line.B.x - line.A.x;
  long dbay = line.B.y - line.A.y;
  if((!dbax && !dbay) || rect.top_left.x == rect.bottom_right.x || rect.top_left.y == rect.bottom_right.y)
    return result;
  unsigned i = 0;
  bool A_inside = dpaw_in_rect(rect, line.A);
  bool B_inside = dpaw_in_rect(rect, line.B);
  if(A_inside)
    result.P[i++] = line.A;
  if(B_inside)
    result.P[i++] = line.B;
  if(i >= 2) goto done;
  if(dbax){
    long lxmin = line.A.x < line.B.x ? line.A.x : line.B.x;
    long lxmax = line.A.x < line.B.x ? line.B.x : line.A.x;
    {
      struct dpaw_point X = {rect.top_left.x, line.A.y+(long long)(rect.top_left.x-line.A.x)*dbay/dbax};
      if(lxmin <= rect.top_left.x && lxmax >= rect.top_left.x && rect.top_left.y <= X.y && X.y <= rect.bottom_right.y && !(i==1 && dpaw_point_equal(result.P[0], X)))
        result.P[i++] = X;
      if(i >= 2) goto done;
    }
    {
      struct dpaw_point X = {rect.bottom_right.x, line.A.y+(long long)(rect.bottom_right.x-line.A.x)*dbay/dbax};
      if(lxmin <= rect.bottom_right.x && lxmax >= rect.bottom_right.x && rect.top_left.y <= X.y && X.y <= rect.bottom_right.y)
        result.P[i++] = X;
      if(i >= 2) goto done;
    }
  }
  if(dbay){
    long lymin = line.A.y < line.B.y ? line.A.y : line.B.y;
    long lymax = line.A.y < line.B.y ? line.B.y : line.A.y;
    {
      struct dpaw_point X = {line.A.x+(long long)(rect.top_left.y-line.A.y)*dbax/dbay, rect.top_left.y};
      if(lymin <= rect.top_left.y && lymax >= rect.top_left.y && rect.top_left.x <= X.x && X.x <= rect.bottom_right.x && !(i==1 && dpaw_point_equal(result.P[0], X)))
        result.P[i++] = X;
      if(i >= 2) goto done;
    }
    {
      struct dpaw_point X = {line.A.x+(long long)(rect.bottom_right.y-line.A.y)*dbax/dbay, rect.bottom_right.y};
      if(lymin <= rect.bottom_right.y && lymax >= rect.bottom_right.y && rect.top_left.x <= X.x && X.x <= rect.bottom_right.x)
        result.P[i++] = X;
      if(i >= 2) goto done;
    }
  }
done:
  if(i != 2 || dpaw_point_equal(result.A, result.B))
    return (struct dpaw_line){.A={0,0},.B={0,0}};
  return result;
}

struct dpaw_calc_distance_edge {
  const struct dpaw_screen_info* screen;
  struct dpaw_point position;
  bool enter;
};

int dpaw_calc_distance_edge_comperator(const void* vA, const void* vB, void* vdba){
  const struct dpaw_calc_distance_edge* A = vA;
  const struct dpaw_calc_distance_edge* B = vB;
  const struct dpaw_point* dba = vdba;
  if(dpaw_point_equal(A->position, B->position)){
    if(A->enter && !B->enter)
      return -1;
    if(!A->enter && B->enter)
      return 1;
    return 0;
  }
  return ((A->position.x <= B->position.x) == (dba->x >= 0)) && ((A->position.y <= B->position.y) == (dba->y >= 0)) ? -1 : 1;
}

struct dpaw_point dpaw_calc_distance(const struct dpaw* dpaw, struct dpaw_point A, struct dpaw_point B, enum dpaw_unit unit){
  struct dpaw_point dba = {
    .x = B.x - A.x,
    .y = B.y - A.y
  };
  if(!dba.x && !dba.y)
    return (struct dpaw_point){0,0};
  switch(unit){
    case DPAW_UNIT_PIXEL: return dba;
    case DPAW_UNIT_MICROMETER: {
      size_t screen_count = dpaw->root.screenchange_detector.screen_list.size;
      struct dpaw_calc_distance_edge* edges = calloc(sizeof(struct dpaw_calc_distance_edge), screen_count*2);
      if(!edges){
        fprintf(stderr, "calloc failed!\n");
        abort();
      }
      size_t edge_count = 0;
      for(struct dpaw_list_entry* it=dpaw->root.screenchange_detector.screen_list.first; it; it=it->next){
        const struct dpaw_screen_info* screen = container_of(it, struct dpaw_screen_info, screen_entry);
        struct dpaw_line line = dpaw_line_clip(screen->boundary, (struct dpaw_line){.A=A, .B=B});
/*        printf("%ldx%ld %ldx%ld  %ldx%ld %ldx%ld  %ldx%ld %ldx%ld\n",
          screen->boundary.top_left.x, screen->boundary.top_left.y, screen->boundary.bottom_right.x, screen->boundary.bottom_right.y,
          A.x, A.y, B.x, B.y,
          line.A.x, line.A.y, line.B.x, line.B.y
        );*/
        if(dpaw_point_equal(line.A, line.B))
          continue;
        bool A_before_B = ((line.A.x <= line.B.x) == (dba.x >= 0)) && ((line.A.y <= line.B.y) == (dba.y >= 0));
        assert(edge_count+2 <= screen_count*2);
        edges[edge_count++] = (struct dpaw_calc_distance_edge){
          .screen = screen,
          .position = line.A,
          .enter = A_before_B
        };
        edges[edge_count++] = (struct dpaw_calc_distance_edge){
          .screen = screen,
          .position = line.B,
          .enter = !A_before_B
        };
      }
      qsort_r(edges, edge_count, sizeof(struct dpaw_calc_distance_edge), dpaw_calc_distance_edge_comperator, &dba);
      struct dpaw_point result = {0,0};
      struct dpaw_point lastpos = A;
      double x_mmppx=0, y_mmppx=0;
      int sei = 0; // In case of overlapping intersected screen sections, the average pixel per millimeter of all screens is used.
      for(size_t i=0; i<edge_count; i++){
        const struct dpaw_calc_distance_edge*const e = &edges[i];
        double s_x_mmppx=0, s_y_mmppx=0;
        if(e->screen->physical_size_mm.x){
          s_x_mmppx = (double)e->screen->physical_size_mm.x / (e->screen->boundary.bottom_right.x - e->screen->boundary.top_left.x);
        }else{
          s_x_mmppx = 1.0/DPAW_DEFAULT_PPMM;
        }
        if(e->screen->physical_size_mm.y){
          s_y_mmppx = (double)e->screen->physical_size_mm.y / (e->screen->boundary.bottom_right.y - e->screen->boundary.top_left.y);
        }else{
          s_y_mmppx = 1.0/DPAW_DEFAULT_PPMM;
        }
        if(e->enter){
          sei += 1;
          x_mmppx += s_x_mmppx;
          y_mmppx += s_y_mmppx;
        }
        double r_x_mmppx=0, r_y_mmppx=0;
        if(sei){
          r_x_mmppx = x_mmppx / sei;
          r_y_mmppx = y_mmppx / sei;
        }else{
          r_x_mmppx = 1.0/DPAW_DEFAULT_PPMM;
          r_y_mmppx = 1.0/DPAW_DEFAULT_PPMM;
        }
        if(!e->enter){
          sei -= 1;
          x_mmppx -= s_x_mmppx;
          y_mmppx -= s_y_mmppx;
        }
        assert(sei >= 0);
        result.x += (e->position.x - lastpos.x) * 1000 * r_x_mmppx;
        result.y += (e->position.y - lastpos.y) * 1000 * r_y_mmppx;
//        printf(": %d %d  %lf %lf  %ld %ld\n", (int)e->enter, sei, r_x_mmppx, r_y_mmppx, result.x, result.y);
        lastpos = e->position;
      }
//      printf("! %ld %ld  %ld %ld\n", B.x, B.y, lastpos.x, lastpos.y);
      result.x += (B.x - lastpos.x) * 1000 / DPAW_DEFAULT_PPMM;
      result.y += (B.y - lastpos.y) * 1000 / DPAW_DEFAULT_PPMM;
      free(edges);
      return result;
    } break;
  }
  return (struct dpaw_point){0,0};
}
