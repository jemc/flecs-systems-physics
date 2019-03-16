#include <include/physics.h>
#include <string.h>
#include <math.h>

/* -- Automatically add colliders to geometry entities with EcsCollider -- */

static
void add_rect_collider(
    EcsWorld *world,
    EcsEntity entity,
    EcsType TEcsPolygon8Collider,
    float w,
    float h)
{
    ecs_set(world, entity, EcsPolygon8Collider, {
        .points = {
            {-w, -h},
            {w, -h},
            {w, h},
            {-w, h},
            {-w, -h}
        },
        .point_count = 5
    });
}

void EcsAddColliderForSquare(EcsRows *rows) {
    EcsWorld *world = rows->world;
    EcsType TEcsPolygon8Collider = ecs_column_type(rows, 3);

    int i;
    for (i = rows->begin; i < rows->end; i ++) {
        EcsSquare *s = ecs_field(rows, EcsSquare, i, 1);

        float w = s->size / 2.0;
        add_rect_collider(world, rows->entities[i], TEcsPolygon8Collider, w, w);
    }
}

void EcsAddColliderForRectangle(EcsRows *rows) {
    EcsWorld *world = rows->world;
    EcsType TEcsPolygon8Collider = ecs_column_type(rows, 3);

    int i;
    for (i = rows->begin; i < rows->end; i ++) {
        EcsRectangle *s = ecs_field(rows, EcsRectangle, i, 1);

        float w = s->width / 2.0;
        float h = s->height / 2.0;

        add_rect_collider(world, rows->entities[i], TEcsPolygon8Collider, w, h);
    }
}

void EcsAddColliderForCircle(EcsRows *rows) {
    EcsWorld *world = rows->world;
    EcsType TEcsCircleCollider = ecs_column_type(rows, 3);

    int i;
    for (i = rows->begin; i < rows->end; i ++) {
        EcsCircle *s = ecs_field(rows, EcsCircle, i, 1);

        ecs_set(world, rows->entities[i], EcsCircleCollider, {
            .position = {0, 0},
            .radius = s->radius
        });
    }
}


/* -- Add world colliders (colliders transformed to world space) -- */

void EcsAddPolygon8ColliderWorld(EcsRows *rows) {
    EcsWorld *world = rows->world;
    EcsType TEcsPolygon8ColliderWorld = ecs_column_type(rows, 2);

    int i;
    for (i = rows->begin; i < rows->end; i ++) {
        EcsPolygon8Collider *collider = ecs_field(rows, EcsPolygon8Collider, i, 1);
        ecs_set_ptr(world, rows->entities[i], EcsPolygon8ColliderWorld, collider);
    }
}

void EcsAddCircleColliderWorld(EcsRows *rows) {
    EcsWorld *world = rows->world;
    EcsType TEcsCircleColliderWorld = ecs_column_type(rows, 2);

    int i;
    for (i = rows->begin; i < rows->end; i ++) {
        EcsCircleCollider *collider = ecs_field(rows, EcsCircleCollider, i, 1);
        ecs_set_ptr(world, rows->entities[i], EcsCircleColliderWorld, collider);
    }
}


/* -- Transform colliders to world space -- */

void EcsTransformPolygon8Colliders(EcsRows *rows) {
    int i;
    for (i = rows->begin; i < rows->end; i ++) {
        EcsMatTransform2D *m = ecs_field(rows, EcsMatTransform2D, i, 1);
        EcsPolygon8Collider *c = ecs_field(rows, EcsPolygon8Collider, i, 2);
        EcsPolygon8ColliderWorld *cw = ecs_field(rows, EcsPolygon8ColliderWorld, i, 3);
        ecs_mat3x3_transform(m, c->points, cw->points, c->point_count);
        cw->point_count = c->point_count;
    }
}

void EcsTransformCircleColliders(EcsRows *rows) {
    int i;
    for (i = rows->begin; i < rows->end; i ++) {
        EcsMatTransform2D *m = ecs_field(rows, EcsMatTransform2D, i, 1);
        EcsCircleCollider *c = ecs_field(rows, EcsCircleCollider, i, 2);
        EcsCircleColliderWorld *cw = ecs_field(rows, EcsCircleColliderWorld, i, 3);
        ecs_mat3x3_transform(m, &c->position, &cw->position, 1);
    }
}

typedef struct TestColliderParam {
    EcsEntity entity;
    EcsEntity collider_component;
    void *collider_data;
} TestColliderParam;

static
EcsPolygonCollider poly8_to_poly(
    EcsPolygon8ColliderWorld *collider)
{
    return (EcsPolygonCollider) {
        .point_count = collider->point_count,
        .points = collider->points
    };
}

void create_collision(
    EcsWorld *world,
    EcsEntity entity_1,
    EcsEntity entity_2,
    EcsCollision2D *collision_data,
    EcsType TEcsCollision2D)
{
    if (entity_2 > entity_1) {
        collision_data->entity_1 = entity_1;
        collision_data->entity_2 = entity_2;
    } else {
        collision_data->entity_1 = entity_2;
        collision_data->entity_2 = entity_1;
    }

    ecs_set_ptr(world, 0, EcsCollision2D, collision_data);
}

void EcsTestColliders(EcsRows *rows) {
    TestColliderParam *param = rows->param;
    EcsWorld *world = rows->world;
    EcsType collider = ecs_column_type(rows, 1);
    EcsType TEcsPolygon8ColliderWorld = ecs_column_type(rows, 2);
    EcsType TEcsCircleColliderWorld = ecs_column_type(rows, 3);
    EcsType TEcsCollision2D = ecs_column_type(rows, 4);
    EcsCollision2D collision;
    int i;

    if (param->collider_component == TEcsPolygon8ColliderWorld) {
        EcsPolygonCollider c1 = poly8_to_poly(param->collider_data);
        if (collider == TEcsPolygon8ColliderWorld) {
            for (i = rows->begin; i < rows->end; i ++) {
                EcsPolygonCollider c2 = poly8_to_poly(
                    ecs_field(rows, EcsPolygon8ColliderWorld, i, 1));

                if (ecs_collider2d_poly(&c1, &c2, &collision)) {
                    create_collision(world, rows->entities[i], param->entity, 
                            &collision, TEcsCollision2D);
                }
            }
        } else if (collider == TEcsCircleColliderWorld) {
            for (i = rows->begin; i < rows->end; i ++) {
                EcsCircleColliderWorld *c2 = ecs_field(rows, EcsCircleColliderWorld, i, 1);

                if (ecs_collider2d_poly_circle(&c1, c2, &collision)) {
                    create_collision(world, rows->entities[i], param->entity, 
                                    &collision, TEcsCollision2D);
                }
            }
        }
    } else if (param->collider_component == TEcsCircleColliderWorld) {
        EcsCircleColliderWorld *c1 = param->collider_data;
        if (collider == TEcsPolygon8ColliderWorld) {
            for (i = rows->begin; i < rows->end; i ++) {
                EcsPolygonCollider c2 = poly8_to_poly(ecs_field(rows, EcsPolygon8ColliderWorld, i, 1));

                if (ecs_collider2d_circle_poly(c1, &c2, &collision)) {
                    create_collision(world, rows->entities[i], param->entity, &collision,
                                     TEcsCollision2D);
                }
            }
        } else if (collider == TEcsCircleColliderWorld) {
            for (i = rows->begin; i < rows->end; i ++) {
                EcsCircleColliderWorld *c2 = ecs_field(rows, EcsCircleColliderWorld, i, 1);

                if (ecs_collider2d_circle(c1, c2, &collision)) {
                    create_collision(world, rows->entities[i], param->entity, 
                                    &collision, TEcsCollision2D);
                }
            }
        }
    }
}

void EcsWalkColliders(EcsRows *rows) {
    EcsWorld *world = rows->world;
    EcsType collider = ecs_column_type(rows, 1);
    EcsType TEcsTestColliders = ecs_column_type(rows, 2);
    int i;
    
    for (i = rows->begin; i < rows->end; i ++) {
        void *data = _ecs_field(rows, i, 1, false);
        TestColliderParam param = {
            .entity = rows->entities[i],
            .collider_component = collider,
            .collider_data = data
        };

        ecs_run_w_filter(
            world,
            TEcsTestColliders,
            rows->delta_time,
            i + 1,
            0,
            0,
            &param);
    }
}

void EcsCleanCollisions(EcsRows *rows) {
    EcsWorld *world = rows->world;

    int i;
    for (i = rows->begin; i < rows->end; i ++) {
        ecs_delete(world, rows->entities[i]);
    }
}
