

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define ROWS 50
#define COLS 80
#define FILL '*'
#define BG   ' '
#define MAX_OBJECTS 50

char canvas[ROWS][COLS];

void init_canvas(void) {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            canvas[r][c] = BG;
}

void display_canvas(void) {
    printf("\n  ");
    for (int c = 0; c < COLS; c++) printf("%d", c % 10);
    printf("\n");
    for (int r = 0; r < ROWS; r++) {
        printf("%2d", r);
        for (int c = 0; c < COLS; c++) printf("%c", canvas[r][c]);
        printf("\n");
    }
    printf("\n");
}

/* safe pixel setter */
static void set_pixel(int r, int c, char ch) {
    if (r >= 0 && r < ROWS && c >= 0 && c < COLS)
        canvas[r][c] = ch;
}

/* ─── Shape Types ────────────────────────────────────────── */
typedef enum { SHAPE_LINE, SHAPE_RECT, SHAPE_CIRCLE, SHAPE_TRIANGLE } ShapeType;

typedef struct {
    ShapeType type;
    int active;        
    int x1, y1;        
    int x2, y2;        
    int x3, y3;      
} Object;

Object objects[MAX_OBJECTS];
int obj_count = 0;

/* ─── Drawing Primitives ─────────────────────────────────── */

/* Bresenham line */
void draw_line_pixels(int r1, int c1, int r2, int c2, char ch) {
    int dr = abs(r2 - r1), dc = abs(c2 - c1);
    int sr = (r1 < r2) ? 1 : -1;
    int sc = (c1 < c2) ? 1 : -1;
    int err = dr - dc;
    while (1) {
        set_pixel(r1, c1, ch);
        if (r1 == r2 && c1 == c2) break;
        int e2 = 2 * err;
        if (e2 > -dc) { err -= dc; r1 += sr; }
        if (e2 <  dr) { err += dr; c1 += sc; }
    }
}

/* Rectangle (border only) */
void draw_rect_pixels(int r1, int c1, int r2, int c2, char ch) {
    for (int c = c1; c <= c2; c++) { set_pixel(r1, c, ch); set_pixel(r2, c, ch); }
    for (int r = r1; r <= r2; r++) { set_pixel(r, c1, ch); set_pixel(r, c2, ch); }
}

/* Midpoint circle */
void draw_circle_pixels(int cr, int cc, int radius, char ch) {
    int x = 0, y = radius, d = 1 - radius;
    while (x <= y) {
        set_pixel(cr + x, cc + y, ch); set_pixel(cr + x, cc - y, ch);
        set_pixel(cr - x, cc + y, ch); set_pixel(cr - x, cc - y, ch);
        set_pixel(cr + y, cc + x, ch); set_pixel(cr + y, cc - x, ch);
        set_pixel(cr - y, cc + x, ch); set_pixel(cr - y, cc - x, ch);
        if (d < 0) d += 2 * x + 3;
        else { d += 2 * (x - y) + 5; y--; }
        x++;
    }
}

/* Triangle (three sides) */
void draw_triangle_pixels(int r1, int c1, int r2, int c2, int r3, int c3, char ch) {
    draw_line_pixels(r1, c1, r2, c2, ch);
    draw_line_pixels(r2, c2, r3, c3, ch);
    draw_line_pixels(r3, c3, r1, c1, ch);
}

/* ─── Redraw all active objects ──────────────────────────── */
void redraw_all(void) {
    init_canvas();
    for (int i = 0; i < obj_count; i++) {
        if (!objects[i].active) continue;
        Object *o = &objects[i];
        switch (o->type) {
            case SHAPE_LINE:
                draw_line_pixels(o->y1, o->x1, o->y2, o->x2, FILL); break;
            case SHAPE_RECT:
                draw_rect_pixels(o->y1, o->x1, o->y2, o->x2, FILL); break;
            case SHAPE_CIRCLE:
                draw_circle_pixels(o->y1, o->x1, o->x3, FILL); break;
            case SHAPE_TRIANGLE:
                draw_triangle_pixels(o->y1, o->x1, o->y2, o->x2, o->y3, o->x3, FILL); break;
        }
    }
}

/* ─── Bounds hint ────────────────────────────────────────── */
static void warn_oob(int r, int c) {
    if (r < 0 || r >= ROWS || c < 0 || c >= COLS)
        printf("  [WARNING] (%d,%d) is outside canvas (rows 0-%d, cols 0-%d)."
               " Shape may be clipped.\n", r, c, ROWS-1, COLS-1);
}

/* ─── Add Objects ────────────────────────────────────────── */
int add_line(int r1, int c1, int r2, int c2) {
    if (obj_count >= MAX_OBJECTS) { printf("Object limit reached.\n"); return -1; }
    warn_oob(r1, c1); warn_oob(r2, c2);
    Object *o = &objects[obj_count];
    o->type = SHAPE_LINE; o->active = 1;
    o->x1 = c1; o->y1 = r1; o->x2 = c2; o->y2 = r2;
    printf("Line added as object #%d\n", obj_count);
    return obj_count++;
}

int add_rect(int r1, int c1, int r2, int c2) {
    if (obj_count >= MAX_OBJECTS) { printf("Object limit reached.\n"); return -1; }
    warn_oob(r1, c1); warn_oob(r2, c2);
    Object *o = &objects[obj_count];
    o->type = SHAPE_RECT; o->active = 1;
    o->x1 = c1; o->y1 = r1; o->x2 = c2; o->y2 = r2;
    printf("Rectangle added as object #%d\n", obj_count);
    return obj_count++;
}

int add_circle(int cr, int cc, int radius) {
    if (obj_count >= MAX_OBJECTS) { printf("Object limit reached.\n"); return -1; }
    if (cr < 0 || cr >= ROWS || cc < 0 || cc >= COLS)
        printf("  [WARNING] Center (%d,%d) is outside canvas (rows 0-%d, cols 0-%d)."
               " Circle will not be visible.\n", cr, cc, ROWS-1, COLS-1);
    else if (cr - radius < 0 || cr + radius >= ROWS ||
             cc - radius < 0 || cc + radius >= COLS)
        printf("  [WARNING] Circle extends outside canvas and will be clipped.\n");
    Object *o = &objects[obj_count];
    o->type = SHAPE_CIRCLE; o->active = 1;
    o->y1 = cr; o->x1 = cc; o->x3 = radius;
    printf("Circle added as object #%d\n", obj_count);
    return obj_count++;
}

int add_triangle(int r1, int c1, int r2, int c2, int r3, int c3) {
    if (obj_count >= MAX_OBJECTS) { printf("Object limit reached.\n"); return -1; }
    warn_oob(r1, c1); warn_oob(r2, c2); warn_oob(r3, c3);
    Object *o = &objects[obj_count];
    o->type = SHAPE_TRIANGLE; o->active = 1;
    o->x1 = c1; o->y1 = r1; o->x2 = c2; o->y2 = r2; o->x3 = c3; o->y3 = r3;
    printf("Triangle added as object #%d\n", obj_count);
    return obj_count++;
}

/* ─── Delete Object ──────────────────────────────────────── */
void delete_object(int id) {
    if (id < 0 || id >= obj_count || !objects[id].active) {
        printf("Invalid or already-deleted object #%d\n", id);
        return;
    }
    objects[id].active = 0;
    printf("Object #%d deleted.\n", id);
}

/* ─── Modify Object ──────────────────────────────────────── */
/* For simplicity: modify replaces parameters completely for the same shape type */
void modify_line(int id, int r1, int c1, int r2, int c2) {
    if (id < 0 || id >= obj_count || !objects[id].active || objects[id].type != SHAPE_LINE) {
        printf("Object #%d is not an active line.\n", id); return;
    }
    objects[id].y1 = r1; objects[id].x1 = c1;
    objects[id].y2 = r2; objects[id].x2 = c2;
    printf("Line #%d modified.\n", id);
}

void modify_rect(int id, int r1, int c1, int r2, int c2) {
    if (id < 0 || id >= obj_count || !objects[id].active || objects[id].type != SHAPE_RECT) {
        printf("Object #%d is not an active rectangle.\n", id); return;
    }
    objects[id].y1 = r1; objects[id].x1 = c1;
    objects[id].y2 = r2; objects[id].x2 = c2;
    printf("Rectangle #%d modified.\n", id);
}

void modify_circle(int id, int cr, int cc, int radius) {
    if (id < 0 || id >= obj_count || !objects[id].active || objects[id].type != SHAPE_CIRCLE) {
        printf("Object #%d is not an active circle.\n", id); return;
    }
    objects[id].y1 = cr; objects[id].x1 = cc; objects[id].x3 = radius;
    printf("Circle #%d modified.\n", id);
}

void modify_triangle(int id, int r1, int c1, int r2, int c2, int r3, int c3) {
    if (id < 0 || id >= obj_count || !objects[id].active || objects[id].type != SHAPE_TRIANGLE) {
        printf("Object #%d is not an active triangle.\n", id); return;
    }
    objects[id].y1 = r1; objects[id].x1 = c1;
    objects[id].y2 = r2; objects[id].x2 = c2;
    objects[id].y3 = r3; objects[id].x3 = c3;
    printf("Triangle #%d modified.\n", id);
}

/* ─── List Objects ───────────────────────────────────────── */
void list_objects(void) {
    const char *names[] = {"Line", "Rectangle", "Circle", "Triangle"};
    printf("\n--- Object List ---\n");
    int found = 0;
    for (int i = 0; i < obj_count; i++) {
        if (!objects[i].active) continue;
        found = 1;
        Object *o = &objects[i];
        printf("  #%d %s: ", i, names[o->type]);
        switch (o->type) {
            case SHAPE_LINE:
                printf("(%d,%d) -> (%d,%d)\n", o->y1, o->x1, o->y2, o->x2); break;
            case SHAPE_RECT:
                printf("top-left(%d,%d) bottom-right(%d,%d)\n", o->y1, o->x1, o->y2, o->x2); break;
            case SHAPE_CIRCLE:
                printf("center(%d,%d) radius=%d\n", o->y1, o->x1, o->x3); break;
            case SHAPE_TRIANGLE:
                printf("(%d,%d) (%d,%d) (%d,%d)\n", o->y1, o->x1, o->y2, o->x2, o->y3, o->x3); break;
        }
    }
    if (!found) printf("  (no active objects)\n");
    printf("-------------------\n\n");
}

/* ─── Interactive Menu ───────────────────────────────────── */
void print_menu(void) {
    printf("========== 2D Graphics Editor ==========\n");
    printf(" 1. Add Line\n");
    printf(" 2. Add Rectangle\n");
    printf(" 3. Add Circle\n");
    printf(" 4. Add Triangle\n");
    printf(" 5. Delete Object\n");
    printf(" 6. Modify Line\n");
    printf(" 7. Modify Rectangle\n");
    printf(" 8. Modify Circle\n");
    printf(" 9. Modify Triangle\n");
    printf("10. Display Canvas\n");
    printf("11. List Objects\n");
    printf(" 0. Exit\n");
    printf("=========================================\n");
    printf("Choice: ");
}

int main(void) {
    init_canvas();
    int choice, id;

    while (1) {
        print_menu();
        if (scanf("%d", &choice) != 1) break;

        int r1, c1, r2, c2, r3, c3, radius;

        switch (choice) {
            case 1:
                printf("Enter r1 c1 r2 c2: ");
                scanf("%d %d %d %d", &r1, &c1, &r2, &c2);
                add_line(r1, c1, r2, c2);
                redraw_all(); display_canvas(); break;

            case 2:
                printf("Enter top-left r1 c1 and bottom-right r2 c2: ");
                scanf("%d %d %d %d", &r1, &c1, &r2, &c2);
                add_rect(r1, c1, r2, c2);
                redraw_all(); display_canvas(); break;

            case 3:
                printf("Enter center_row center_col radius: ");
                scanf("%d %d %d", &r1, &c1, &radius);
                add_circle(r1, c1, radius);
                redraw_all(); display_canvas(); break;

            case 4:
                printf("Enter v1(r c) v2(r c) v3(r c): ");
                scanf("%d %d %d %d %d %d", &r1, &c1, &r2, &c2, &r3, &c3);
                add_triangle(r1, c1, r2, c2, r3, c3);
                redraw_all(); display_canvas(); break;

            case 5:
                list_objects();
                printf("Enter object id to delete: ");
                scanf("%d", &id);
                delete_object(id);
                redraw_all(); display_canvas(); break;

            case 6:
                list_objects();
                printf("Enter line id, then r1 c1 r2 c2: ");
                scanf("%d %d %d %d %d", &id, &r1, &c1, &r2, &c2);
                modify_line(id, r1, c1, r2, c2);
                redraw_all(); display_canvas(); break;

            case 7:
                list_objects();
                printf("Enter rect id, then r1 c1 r2 c2: ");
                scanf("%d %d %d %d %d", &id, &r1, &c1, &r2, &c2);
                modify_rect(id, r1, c1, r2, c2);
                redraw_all(); display_canvas(); break;

            case 8:
                list_objects();
                printf("Enter circle id, then center_row center_col radius: ");
                scanf("%d %d %d %d", &id, &r1, &c1, &radius);
                modify_circle(id, r1, c1, radius);
                redraw_all(); display_canvas(); break;

            case 9:
                list_objects();
                printf("Enter triangle id, then v1(r c) v2(r c) v3(r c): ");
                scanf("%d %d %d %d %d %d %d", &id, &r1, &c1, &r2, &c2, &r3, &c3);
                modify_triangle(id, r1, c1, r2, c2, r3, c3);
                redraw_all(); display_canvas(); break;

            case 10:
                redraw_all(); display_canvas(); break;

            case 11:
                list_objects(); break;

            case 0:
                printf("Exiting.\n"); return 0;

            default:
                printf("Invalid choice.\n");
        }
    }
    return 0;
}