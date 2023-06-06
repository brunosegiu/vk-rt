vec3 equirectangularToCartesian(vec2 uv) {
    float lon = uv.x * 2.0 * Pi;
    float lat = uv.y * Pi;

    // float x = cos(lat) * cos(lon);
    // float y = sin(lat);
    // float z = cos(lat) * sin(lon);

    // return vec3(x, y, z);
    return vec3(sin(lat) * cos(lon), cos(lat), sin(lat) * sin(lon));
}

vec2 cartesianToEquirectangular(vec3 cartesian) {
    float lon = atan(cartesian.z, cartesian.x);
    float lat = asin(cartesian.y);

    float u = (lon + Pi) / (2.0 * Pi);
    float v = (lat + Pi * 0.5) / Pi;

    return vec2(u, v);
}