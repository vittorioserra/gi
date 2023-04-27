#pragma once
#include <string>
#include <GL/glew.h>
#include <GL/gl.h>

// ------------------------------------------
// Quad definition

/**
 * @brief Full screen quad for simple blitting of OpenGL textures on screen
 */
class Quad {
public:
    inline Quad();
    inline ~Quad();

    inline void draw(GLuint tex, float exposure = 1.f) const;

private:
    // data
    GLuint vao, vbo, ibo, shader;
};

// ------------------------------------------
// Quad shader source

static const char *const source_vert_quad = R"glsl(
#version 130

in vec3 in_pos;
in vec2 in_tc;

out vec2 tc;

void main() {
	tc = in_tc;
	vec4 pos= vec4(vec3(2.0)*in_pos - vec3(1.0), 1.0);
	pos.z = -1;
	gl_Position = pos;
}
)glsl";

static const char *const source_frag_quad = R"glsl(
#version 140

in vec2 tc;
out vec4 out_col;

uniform float exposure;

uniform int width;
uniform samplerBuffer in_buf;

float rgb_to_srgb(float val) {
    if (val <= 0.0031308f) return 12.92f * val;
    return 1.055f * pow(val, 1.f / 2.4f) - 0.055f;
}
vec3 rgb_to_srgb(vec3 rgb) {
    return vec3(rgb_to_srgb(rgb.x), rgb_to_srgb(rgb.y), rgb_to_srgb(rgb.z));
}

void main() {
    out_col = exposure * texelFetch(in_buf, int(gl_FragCoord.y) * width + int(gl_FragCoord.x));
    out_col.rgb = rgb_to_srgb(out_col.rgb);
}
)glsl";

// ------------------------------------------
// Quad implementation

Quad::Quad() {
    // vertices
    static float quad[20] = {0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1};
    // indices
    static uint32_t idx[6] = {0, 1, 2, 2, 3, 0};

    // push quad
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    std::string name = "QUAD VAO";
    glObjectLabel(GL_VERTEX_ARRAY, vao, name.length(), name.c_str());

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    name = "QUAD VBO";
    glObjectLabel(GL_BUFFER, vbo, name.length(), name.c_str());

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    name = "QUAD IBO";
    glObjectLabel(GL_BUFFER, ibo, name.length(), name.c_str());

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
    glBindVertexArray(0);

    // compile shader, while shamelessly ignoring errors
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &source_vert_quad, NULL);
    glCompileShader(vert);

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &source_frag_quad, NULL);
    glCompileShader(frag);

    shader = glCreateProgram();
    glAttachShader(shader, vert);
    glAttachShader(shader, frag);
    glLinkProgram(shader);
}

Quad::~Quad() {
    glDeleteBuffers(1, &ibo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader);
}

void Quad::draw(GLuint tex, float exposure) const {
    glUseProgram(shader);
    glBindTexture(GL_TEXTURE_BUFFER, tex);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    glUniform1i(glGetUniformLocation(shader, "width"), vp[2]);
    glUniform1i(glGetUniformLocation(shader, "in_tex"), 0);
    glUniform1f(glGetUniformLocation(shader, "exposure"), exposure);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glUseProgram(0);
}
