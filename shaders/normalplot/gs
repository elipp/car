#version 400

layout(points) in;
//layout(line_strip) out;
layout(line_strip) out;
layout(max_vertices = 2) out;

precision highp float;

uniform mat4 ModelView;
uniform mat4 Projection;

//in vec2 vtexcoord;
in vec3 vnormal[];

out vec3 fcolor;

void main()
{
	//vec4 centr = gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position;
	vec4 pos = gl_in[0].gl_Position;

	//vec3 normal_avg = vnormal[0] + vnormal[1] + vnormal[2];
	//normal_avg = normal_avg/3;
	fcolor = vec3(0.5, 0.5, 0.5);
	gl_Position = Projection*ModelView*pos;
	EmitVertex();
	
	fcolor = vec3(1.0, 1.0, 1.0);
	gl_Position = Projection*ModelView*(pos + 0.05*normalize(vec4(vnormal[0], 1.0)));
	EmitVertex();
	EndPrimitive();
}