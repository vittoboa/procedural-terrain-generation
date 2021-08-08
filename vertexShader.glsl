#version 460 core

layout(location=0) in vec4 terrain_coordinates;
layout(location=1) in vec4 terrain_color;
layout(location=2) in vec3 terrain_normal;
layout(location=3) in float terrain_shininess;

uniform mat4 projection_matrix;
uniform mat4 model_view_matrix;
uniform mat3 normal_matrix;

out vec4 color;

struct Light
{
   vec4 ambient_colors;
   vec4 diffuse_colors;
   vec4 specular_colors;
   vec4 coords;
};
uniform Light light0;

uniform vec4 global_ambient;

vec3 normal, light_direction, eye_direction, view_direction, halfway;
vec4 ambient, diffuse, specular;

void main(void)
{
   // object normal
   normal = normalize(normal_matrix * terrain_normal);
   // light direction
   light_direction = normalize(vec3(light0.coords));
   // view direction
   eye_direction  = -1.0f * normalize(vec3(model_view_matrix * terrain_coordinates));
   view_direction = light_direction + eye_direction;
   halfway = (length(view_direction) == 0.0f) ? vec3(0.0) : ((view_direction) / length(view_direction));

   // lighting components
   ambient  = global_ambient + light0.ambient_colors;
   diffuse  = max(dot(normal, light_direction), 0.0f) * light0.diffuse_colors;
   specular = pow(max(dot(normal, halfway), 0.0f), terrain_shininess) * light0.specular_colors;

   // determine final color
   color = (ambient + diffuse + specular) * terrain_color;
   color = vec4(vec3(min(color, vec4(1.0))), 1.0);

   gl_Position = projection_matrix * model_view_matrix * terrain_coordinates;
}
