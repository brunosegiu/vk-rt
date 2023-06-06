vec3 equirectangularToCartesian(vec2 uv) {
    float lon = uv.x * 2.0 * Pi;
    float lat = uv.y * Pi;

    float x = sin(lat) * cos(lon);
    float y = cos(lat);
    float z = sin(lat) * sin(lon);

    return vec3(x, y, z);
}

vec2 cartesianToEquirectangular(vec3 cartesian) {
    float lon = atan(cartesian.z, cartesian.x);
    float lat = acos(cartesian.y);
    return vec2(lon / (2.0 * Pi), lat / Pi);
}