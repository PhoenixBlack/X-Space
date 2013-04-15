#include <stdlib.h>
#include <string.h>
//#include <assert.h>
#include <stdarg.h>
#include <math.h>
#include "vessel.h"

typedef struct dragheat_obj_triangle_tag {
	float x[3];
	float y[3];
	float z[3];
	int creates_drag;
} dragheat_obj_triangle;

extern dragheat_obj_triangle* dragheat_obj_triangles;
extern int dragheat_num_triangles;

//based on http://rosettacode.org/wiki/Catmull%E2%80%93Clark_subdivision_surface/C ...


#define new_struct(type, var) type var = calloc(sizeof(type##_t), 1)
#define len(l) ((l)->n)
#define elem(l, i) ((l)->buf[i])
#define foreach(i, e, l) for (i = 0, e = len(l) ? elem(l,i) : 0;	\
				i < len(l);				\
				e = (++i == len(l) ? 0 : elem(l, i)))

typedef struct {
	int n, alloc;
	void **buf;
} cm_list_t, *cm_list;
 
cm_list list_new()
{
	new_struct(cm_list, r);
	return r;
}
void list_del(cm_list l)
{
	if (!l) return;
	if (l->alloc) free(l->buf);
	free(l);
}
 
int list_push(cm_list lst, void *e)
{
	void **p;
	int na;
 
	if (len(lst) >= lst->alloc) {
		na = lst->alloc * 2;
		if (!na) na = 4;
 
		//assert(p = realloc(lst->buf, sizeof(void*) * na));
		p = realloc(lst->buf, sizeof(void*) * na);
 
		lst->alloc = na;
		lst->buf = p;
	}
	elem(lst, len(lst)) = e;
	return len(lst)++;
}
 
typedef struct { double x, y, z; } cm_coord_t, *cm_coord;
 
#define vadd(a, b) vadd_p(&(a), &(b))
cm_coord vadd_p(cm_coord a, cm_coord b)
{
	a->x += b->x;
	a->y += b->y;
	a->z += b->z;
	return a;
}
 
cm_coord vsub(cm_coord a, cm_coord b, cm_coord c)
{
	c->x = a->x - b->x;
	c->y = a->y - b->y;
	c->z = a->z - b->z;
	return c;
}
 
cm_coord vcross(cm_coord a, cm_coord b, cm_coord c)
{
	c->x = a->y * b->z - a->z * b->y;
	c->y = a->z * b->x - a->x * b->z;
	c->z = a->x * b->y - a->y * b->x;
	return c;
}
 
#define vdiv(a, b) vdiv_p(&(a), b)
cm_coord vdiv_p(cm_coord a, double l)
{
	a->x /= l;
	a->y /= l;
	a->z /= l;
	return a;
}
 
cm_coord vnormalize(cm_coord a)
{
	return vdiv_p(a, sqrt(a->x * a->x + a->y * a->y + a->z * a->z));
}
 
cm_coord vneg(cm_coord a)
{
	a->x = -a->x; a->y = -a->y; a->z = -a->z;
	return a;
}
 
#define vmadd(a, b, s, c) vmadd_p(&(a), &(b), s, &(c))
cm_coord vmadd_p(cm_coord a, cm_coord b, double s, cm_coord c)
{
	c->x = a->x + s * b->x;
	c->y = a->y + s * b->y;
	c->z = a->z + s * b->z;
	return c;
}
 
typedef struct cm_vertex_t {
	cm_coord_t pos, avg_norm;
	cm_list e, f;
	struct cm_vertex_t * v_new;
	int idx;
} *cm_vertex, cm_vertex_t;
 
typedef struct {
	cm_list f;
	cm_vertex v[2];
	cm_vertex e_pt; /* cm_edge point for catmul */
	cm_coord_t avg;
} *cm_edge, cm_edge_t;
 
typedef struct {
	cm_list e, v;
	cm_coord_t norm;
	cm_vertex avg; /* average of all vertices, i.e. cm_face point */
} *cm_face, cm_face_t;
 
typedef struct { cm_list e, v, f; } cm_model_t, *cm_model;
 
cm_model model_new()
{
	new_struct(cm_model, m);
	m->e = list_new();
	m->v = list_new();
	m->f = list_new();
	return m;
}
 
cm_vertex vertex_new()
{
	new_struct(cm_vertex, v);
	v->e = list_new();
	v->f = list_new();
	v->idx = -1;
	return v;
}
 
void vertex_del(cm_vertex v) {
	list_del(v->e);
	list_del(v->f);
	free(v);
}
 
cm_edge edge_new()
{
	new_struct(cm_edge, e);
	e->f = list_new();
	return e;
}
void edge_del(cm_edge e) {
	list_del(e->f);
	free(e);
}
 
cm_face face_new()
{
	new_struct(cm_face, f);
	f->e = list_new();
	f->v = list_new();
	return f;
}
void face_del(cm_face f) {
	list_del(f->e);
	list_del(f->v);
	free(f);
}
 
void model_del(cm_model m)
{
	int i;
	void *p;
 
	foreach(i, p, m->v) vertex_del(p);
	foreach(i, p, m->e) edge_del(p);
	foreach(i, p, m->f) face_del(p);
 
	list_del(m->e);
	list_del(m->v);
	list_del(m->f);
 
	free(m);
}
 
int model_add_vertex(cm_model m, double x, double y, double z)
{
	cm_vertex v = vertex_new();
	v->pos.x = x; v->pos.y = y; v->pos.z = z;
	return v->idx = list_push(m->v, v);
}
 
int model_add_edge(cm_model m, cm_vertex v1, cm_vertex v2)
{
	cm_edge e = edge_new();
 
	e->v[0] = v1;
	e->v[1] = v2;
	list_push(v1->e, e);
	list_push(v2->e, e);
 
	return list_push(m->e, e);
}
 
int model_add_edge_i(cm_model m, int i1, int i2)
{
	//assert(i1 < len(m->v) && i2 < len(m->v));
	return model_add_edge(m, elem(m->v, i1), elem(m->v, i2));
}
 
cm_edge model_edge_by_v(cm_model m, cm_vertex v1, cm_vertex v2)
{
	int i;
	cm_edge e;
	foreach(i, e, v1->e) {
		if ((e->v[0] == v2 || e->v[1] == v2))
			return e;
	}
	i = model_add_edge(m, v1, v2);
	return elem(m->e, i);
}
 
#define quad_face(m, a, b, c, d) model_add_face(m, 4, a, b, c, d)
#define tri_face(m, a, b, c) model_add_face(m, 3, a, b, c)
#define face_v(f, i) ((cm_vertex)(f->v->buf[i]))
int model_add_face_v(cm_model m, cm_list vl)
{
	int i, n = len(vl);
	cm_vertex v0, v1;
	cm_edge e;
	cm_face f = face_new();
 
	v0 = elem(vl, 0);
	for (i = 1; i <= n; i++, v0 = v1) {
		v1 = elem(vl, i % len(vl));
		list_push(v1->f, f);
 
		e = model_edge_by_v(m, v0, v1);
 
		list_push(e->f, f);
		list_push(f->e, e);
		list_push(f->v, v1);
	}
 
	return list_push(m->f, f);
}
 
int model_add_face(cm_model m, int n, ...)
{
	int i, x;
	cm_list lst = list_new();
 
	va_list ap;
	va_start(ap, n);
 
	for (i = 0; i < n; i++) {
		x = va_arg(ap, int);
		list_push(lst, elem(m->v, x));
	}
 
	va_end(ap);
	x = model_add_face_v(m, lst);
	list_del(lst);
	return x;
}
 
void model_norms(cm_model m)
{
	int i, j, n;
	cm_face f;
	cm_vertex v, v0, v1;
 
	cm_coord_t d1, d2, norm;
	foreach(j, f, m->f) {
		n = len(f->v);
		foreach(i, v, f->v) {
			v0 = elem(f->v, i ? i - 1 : n - 1);
			v1 = elem(f->v, (i + 1) % n);
			vsub(&(v->pos), &(v0->pos), &d1);
			vsub(&(v1->pos), &(v->pos), &d2);
			vcross(&d1, &d2, &norm);
			vadd(f->norm, norm);
		}
		if (i > 1) vnormalize(&f->norm);
	}
 
	foreach(i, v, m->v) {
		foreach(j, f, v->f)
			vadd(v->avg_norm, f->norm);
		if (j > 1) vnormalize(&(v->avg_norm));
	}
}
 
cm_vertex face_point(cm_face f)
{
	int i;
	cm_vertex v;
 
	if (!f->avg) {
		f->avg = vertex_new();
		foreach(i, v, f->v)
			if (!i) f->avg->pos = v->pos;
			else    vadd(f->avg->pos, v->pos);
 
		vdiv(f->avg->pos, len(f->v));
	}
	return f->avg;
}
 
#define hole_edge(e) (len(e->f)==1)
cm_vertex edge_point(cm_edge e)
{
	int i;
	cm_face f;
 
	if (!e->e_pt) {
		e->e_pt = vertex_new();
		e->avg = e->v[0]->pos;
		vadd(e->avg, e->v[1]->pos);
		e->e_pt->pos = e->avg;
 
		if (!hole_edge(e)) {
			foreach (i, f, e->f)
				vadd(e->e_pt->pos, face_point(f)->pos);
			vdiv(e->e_pt->pos, 4);
		} else
			vdiv(e->e_pt->pos, 2);
 
		vdiv(e->avg, 2);
	}
 
	return e->e_pt;
}
 
#define hole_vertex(v) (len((v)->f) != len((v)->e))
cm_vertex updated_point(cm_vertex v)
{
	int i, n = 0;
	cm_edge e;
	cm_face f;
	cm_coord_t sum = {0, 0, 0};
 
	if (v->v_new) return v->v_new;
 
	v->v_new = vertex_new();
	if (hole_vertex(v)) {
		v->v_new->pos = v->pos;
		foreach(i, e, v->e) {
			if (!hole_edge(e)) continue;
			vadd(v->v_new->pos, edge_point(e)->pos);
			n++;
		}
		vdiv(v->v_new->pos, n + 1);
	} else {
		n = len(v->f);
		foreach(i, f, v->f)
			vadd(sum, face_point(f)->pos);
		foreach(i, e, v->e)
			vmadd(sum, edge_point(e)->pos, 2, sum);
		vdiv(sum, n);
		vmadd(sum, v->pos, n - 3, sum);
		vdiv(sum, n);
		v->v_new->pos = sum;
	}
 
	return v->v_new;
}
 
#define _get_idx(a, b) x = b;				\
		if ((a = x->idx) == -1)		\
			a = x->idx = list_push(nm->v, x)
cm_model catmull(cm_model m)
{
	int i, j, a, b, c, d;
	cm_face f;
	cm_vertex v, x;
 
	cm_model nm = model_new();
	foreach (i, f, m->f) {
		foreach(j, v, f->v) {
			_get_idx(a, updated_point(v));
			_get_idx(b, edge_point(elem(f->e, (j + 1) % len(f->e))));
			_get_idx(c, face_point(f));
			_get_idx(d, edge_point(elem(f->e, j)));
			model_add_face(nm, 4, a, b, c, d);
		}
	}
 
	//model_norms(nm);
	return nm;
}








void subdivide_dragheat_catmull_clark() {
	int i, j;
	double* vertices = (double*)malloc(1); //Array of XYZ
	int num_vertices = 0;
	cm_model m = model_new();
	cm_model t;
	cm_vertex vv;
	cm_face ff;

	//Convert data into a more useable format
	for (i = 0; i < dragheat_num_triangles; i++) {
		int v0 = -1;
		int v1 = -1;
		int v2 = -1;
		
		//Skip wings
		//if (!dragheat_obj_triangles[i].creates_drag) continue;

		//Skip bad triangles
		{
			double ax,ay,az,bx,by,bz,nx,ny,nz,area;
		
			ax = dragheat_obj_triangles[i].x[0] - dragheat_obj_triangles[i].x[1];
			ay = dragheat_obj_triangles[i].y[0] - dragheat_obj_triangles[i].y[1];
			az = dragheat_obj_triangles[i].z[0] - dragheat_obj_triangles[i].z[1];
			bx = dragheat_obj_triangles[i].x[2] - dragheat_obj_triangles[i].x[1];
			by = dragheat_obj_triangles[i].y[2] - dragheat_obj_triangles[i].y[1];
			bz = dragheat_obj_triangles[i].z[2] - dragheat_obj_triangles[i].z[1];

			nx = ay*bz-az*by;
			ny = az*bx-ax*bz;
			nz = ax*by-ay*bx;

			area = 0.5*sqrt(nx*nx+ny*ny+nz*nz);
			if (area < 0.01) continue;
		}

		//Find three vertices
		for (j = 0; j < num_vertices; j++) {
			double d0,d1,d2;
			double x = vertices[j*3+0];
			double y = vertices[j*3+1];
			double z = vertices[j*3+2];

			d0 = sqrt((dragheat_obj_triangles[i].x[0]-x)*(dragheat_obj_triangles[i].x[0]-x)+
			          (dragheat_obj_triangles[i].y[0]-y)*(dragheat_obj_triangles[i].y[0]-y)+
			          (dragheat_obj_triangles[i].z[0]-z)*(dragheat_obj_triangles[i].z[0]-z));
			d1 = sqrt((dragheat_obj_triangles[i].x[1]-x)*(dragheat_obj_triangles[i].x[1]-x)+
			          (dragheat_obj_triangles[i].y[1]-y)*(dragheat_obj_triangles[i].y[1]-y)+
			          (dragheat_obj_triangles[i].z[1]-z)*(dragheat_obj_triangles[i].z[1]-z));
			d2 = sqrt((dragheat_obj_triangles[i].x[2]-x)*(dragheat_obj_triangles[i].x[2]-x)+
			          (dragheat_obj_triangles[i].y[2]-y)*(dragheat_obj_triangles[i].y[2]-y)+
			          (dragheat_obj_triangles[i].z[2]-z)*(dragheat_obj_triangles[i].z[2]-z));

			if (d0 < 0.001) v0 = j;
			if (d1 < 0.001) v1 = j;
			if (d2 < 0.001) v2 = j;
		}

		//Add the missing vertices
		if (v0 < 0) {
			vertices = (double*)realloc(vertices,sizeof(double)*3*(num_vertices+1));
			vertices[num_vertices*3+0] = dragheat_obj_triangles[i].x[0];
			vertices[num_vertices*3+1] = dragheat_obj_triangles[i].y[0];
			vertices[num_vertices*3+2] = dragheat_obj_triangles[i].z[0];
			model_add_vertex(m,vertices[num_vertices*3+0],vertices[num_vertices*3+1],vertices[num_vertices*3+2]);
			v0 = num_vertices;
			num_vertices++;
		}
		if (v1 < 0) {
			vertices = (double*)realloc(vertices,sizeof(double)*3*(num_vertices+1));
			vertices[num_vertices*3+0] = dragheat_obj_triangles[i].x[1];
			vertices[num_vertices*3+1] = dragheat_obj_triangles[i].y[1];
			vertices[num_vertices*3+2] = dragheat_obj_triangles[i].z[1];
			model_add_vertex(m,vertices[num_vertices*3+0],vertices[num_vertices*3+1],vertices[num_vertices*3+2]);
			v1 = num_vertices;
			num_vertices++;
		}
		if (v2 < 0) {
			vertices = (double*)realloc(vertices,sizeof(double)*3*(num_vertices+1));
			vertices[num_vertices*3+0] = dragheat_obj_triangles[i].x[2];
			vertices[num_vertices*3+1] = dragheat_obj_triangles[i].y[2];
			vertices[num_vertices*3+2] = dragheat_obj_triangles[i].z[2];
			model_add_vertex(m,vertices[num_vertices*3+0],vertices[num_vertices*3+1],vertices[num_vertices*3+2]);
			v2 = num_vertices;
			num_vertices++;
		}

		//Add triangle
		tri_face(m,v0,v1,v2);
	}

	//Subdivide
	//model_norms(m);
	t = catmull(m);

	//Write it back
	free(dragheat_obj_triangles);
	dragheat_num_triangles = 0;
	dragheat_obj_triangles = malloc(1);

	
		/*int ctr = 0;
		foreach(j, v, f->v) {
			ctr ++;
			log_write("VERTEX %p %d\n",v,ctr);
		}*/

	free(dragheat_obj_triangles);
	dragheat_num_triangles = (t->f->n)*2;
	dragheat_obj_triangles = malloc(dragheat_num_triangles*sizeof(dragheat_obj_triangle));
	foreach(i, ff, t->f) {
		cm_vertex v[4];
		foreach(j, vv, ff->v) {
			v[j] = vv;
		}

		dragheat_obj_triangles[i*2+0].x[0] = (float)v[0]->pos.x;
		dragheat_obj_triangles[i*2+0].y[0] = (float)v[0]->pos.y;
		dragheat_obj_triangles[i*2+0].z[0] = (float)v[0]->pos.z;
		dragheat_obj_triangles[i*2+0].x[1] = (float)v[1]->pos.x;
		dragheat_obj_triangles[i*2+0].y[1] = (float)v[1]->pos.y;
		dragheat_obj_triangles[i*2+0].z[1] = (float)v[1]->pos.z;
		dragheat_obj_triangles[i*2+0].x[2] = (float)v[2]->pos.x;
		dragheat_obj_triangles[i*2+0].y[2] = (float)v[2]->pos.y;
		dragheat_obj_triangles[i*2+0].z[2] = (float)v[2]->pos.z;
		dragheat_obj_triangles[i*2+0].creates_drag = 0;

		dragheat_obj_triangles[i*2+1].x[0] = (float)v[0]->pos.x;
		dragheat_obj_triangles[i*2+1].y[0] = (float)v[0]->pos.y;
		dragheat_obj_triangles[i*2+1].z[0] = (float)v[0]->pos.z;
		dragheat_obj_triangles[i*2+1].x[1] = (float)v[2]->pos.x;
		dragheat_obj_triangles[i*2+1].y[1] = (float)v[2]->pos.y;
		dragheat_obj_triangles[i*2+1].z[1] = (float)v[2]->pos.z;
		dragheat_obj_triangles[i*2+1].x[2] = (float)v[3]->pos.x;
		dragheat_obj_triangles[i*2+1].y[2] = (float)v[3]->pos.y;
		dragheat_obj_triangles[i*2+1].z[2] = (float)v[3]->pos.z;
		dragheat_obj_triangles[i*2+1].creates_drag = 0;
	}
}