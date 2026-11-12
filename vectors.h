typedef struct {
    double x;
    double y;
} Vec2;

typedef struct {
    double x;
    double y;
    double z;
} Vec3;

typedef struct {
    double x;
    double y;
    double z;
    double w;
} Vec4;

Vec2 addVec2(Vec2 v1, Vec2 v2);
Vec2 restVec2(Vec2 v1, Vec2 v2);

Vec3 addVec3(Vec3 v1, Vec3 v2);
Vec3 restVec3(Vec3 v1, Vec3 v2);

Vec3 cross3(Vec3 v1, Vec3 v2);

Vec4 addVec4(Vec4 v1, Vec4 v2);
Vec4 restVec4(Vec4 v1, Vec4 v2);

double dot2(Vec2 v1, Vec2 v2);
double dot3(Vec3 v1, Vec3 v2);
double dot4(Vec4 v1, Vec4 v2);

#ifdef VECTOR_LIB_IMPLMENETATION

Vec2 addVec2(Vec2 v1, Vec2 v2) {
    Vec2 result;
    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    return result;
}

Vec2 restVec2(Vec2 v1, Vec2 v2) {
    Vec2 result;
    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    return result;
}

Vec3 addVec3(Vec3 v1, Vec3 v2) {
    Vec3 result;
    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    result.z = v1.z + v2.z;
    return result;
}

Vec3 restVec3(Vec3 v1, Vec3 v2) {
    Vec3 result;
    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    result.z = v1.z - v2.z;
    return result;
}

Vec3 cross3(Vec3 v1, Vec3 v2) {
    Vec3 cross;
    cross.x = v1.y*v2.z - v1.z*v2.y;
    cross.y = v1.z*v2.x - v1.x*v2.z;
    cross.z = v1.x*v2.y - v1.y*v2.x;

    return cross;
}

Vec4 addVec4(Vec4 v1, Vec4 v2) {
    Vec4 result;
    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    result.z = v1.z + v2.z;
    result.w = v1.w + v2.w;
    return result;
}

Vec4 restVec4(Vec4 v1, Vec4 v2) {
    Vec4 result;
    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    result.z = v1.z - v2.z;
    result.w = v1.w - v2.w;
    return result;
}

double dot2(Vec2 v1, Vec2 v2) {
    return v1.x * v2.x +
           v1.y * v2.y;
}

double dot3(Vec3 v1, Vec3 v2) {
    return v1.x * v2.x +
           v1.y * v2.y +
           v1.z * v2.z;
}

double dot4(Vec4 v1, Vec4 v2) {
    return v1.x * v2.x +
           v1.y * v2.y +
           v1.z * v2.z +
           v1.w * v2.w;
}

#endif
