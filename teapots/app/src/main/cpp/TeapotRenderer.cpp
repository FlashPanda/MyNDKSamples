//
// Created by czd on 2018/4/27.
//

/*
 * 渲染茶壶
 */

#include "TeapotRenderer.h"

// 茶壶模型数据
#include "teapot.inl"

TeapotRenderer::TeapotRenderer() {}

TeapotRenderer::~TeapotRenderer() { Unload(); }

void TeapotRenderer::Init() {
    // 设置
    glFrontFace(GL_CCW);

    // 加载着色器
    LoadShaders(&shader_param_, "Shaders/VS_ShaderPlain.vsh",
                "Shaders/ShaderPlain.fsh");

    // 创建索引缓存
    num_indices_ = sizeof(teapotIndices) / sizeof(teapotIndices[0]);
    glGenBuffers(1, &ibo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(teapotIndices), teapotIndices,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // 创建VBO
    num_vertices_ = sizeof(teapotPositions) / sizeof(teapotPositions[0]) / 3;
    int32_t stride = sizeof(TEAPOT_VERTEX);
    int32_t index = 0;
    TEAPOT_VERTEX* p = new TEAPOT_VERTEX[num_vertices_];
    for (int32_t i = 0; i < num_vertices_; ++i) {
        p[i].pos[0] = teapotPositions[index];
        p[i].pos[1] = teapotPositions[index + 1];
        p[i].pos[2] = teapotPositions[index + 2];

        p[i].normal[0] = teapotNormals[index];
        p[i].normal[1] = teapotNormals[index + 1];
        p[i].normal[2] = teapotNormals[index + 2];
        index += 3;
    }
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, stride * num_vertices_, p, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    delete[] p;

    UpdateViewport();
    mat_model_ = ndk_helper::Mat4::Translation(0, 0, -15.0f);

    ndk_helper::Mat4 mat = ndk_helper::Mat4::RotationX(M_PI / 3);
    mat_model_ = mat * mat_model_;
}

void TeapotRenderer::UpdateViewport() {
    // 初始化投影矩阵
    int32_t viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    const float CAM_NEAR = 5.0f;
    const float CAM_FAR = 10000.0f;
    if (viewport[2] < viewport[3]) {
        float aspect = static_cast<float>(viewport[2]) / static_cast<float>(viewport[3]);
        mat_projection_ = ndk_helper::Mat4::Perspective(aspect, 1.0f, CAM_NEAR, CAM_FAR);
    }
    else {
        float aspect = static_cast<float>(viewport[3]) / static_cast<float>(viewport[2]);
        mat_projection_ = ndk_helper::Mat4::Perspective(1.0f, aspect, CAM_NEAR, CAM_FAR);
    }
}

void TeapotRenderer::Unload() {
    if (vbo_) {
        glDeleteBuffers (1, &vbo_);
        vbo_ = 0;
    }

    if (ibo_) {
        glDeleteBuffers (1, &ibo_);
        ibo_ = 0;
    }

    if (shader_param_.program_) {
        glDeleteProgram(shader_param_.program_);
        shader_param_.program_ = 0;
    }
}

void TeapotRenderer::Update(float dTime) {
    const float CAM_X = 0.0f;
    const float CAM_Y = 0.0f;
    const float CAM_Z = 700.0f;

    mat_view_ = ndk_helper::Mat4::LookAt(ndk_helper::Vec3(CAM_X, CAM_Y, CAM_Z),
                                         ndk_helper::Vec3 (0.0f, 0.0f, 0.0f),
                                         ndk_helper::Vec3 (0.0f, 1.0f, 0.0f));

    if (camera_) {
        camera_ -> Update();
        mat_view_ = camera_ -> GetTransformMatrix() * mat_view_ * camera_ -> GetRotationMatrix() * mat_model_;
    }
    else
        mat_view_ = mat_view_ * mat_model_;
}

void TeapotRenderer::Render () {
    ndk_helper::Mat4 mat_vp = mat_projection_ * mat_view_;

    glBindBuffer (GL_ARRAY_BUFFER, vbo_);

    int32_t iStride = sizeof (TEAPOT_VERTEX);

    glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, GL_FALSE, iStride, BUFFER_OFFSET (0));
    glEnableVertexAttribArray (ATTRIB_VERTEX);

    glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, iStride, BUFFER_OFFSET (3 * sizeof (GLfloat)));
    glEnableVertexAttribArray(ATTRIB_NORMAL);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glUseProgram(shader_param_.program_);

    TEAPOT_MATERIALS material = {
            {1.0f, 0.5f, 0.5f},
            {1.0f, 1.0f, 1.0f, 10.0f},
            {0.1f, 0.1f, 0.1f}
    };

    glUniform4f(shader_param_.material_diffuse_, material.diffuse_color[0],
                material.diffuse_color[1], material.diffuse_color[2], 1.0f);
    glUniform4f(shader_param_.material_specular_, material.specular_color[0],
                material.specular_color[1], material.specular_color[2],
                material.specular_color[3]);

    glUniform3f(shader_param_.material_ambient_, material.ambient_color[0],
                material.ambient_color[1], material.ambient_color[2]);

    glUniformMatrix4fv(shader_param_.matrix_projection_, 1, GL_FALSE, mat_vp.Ptr());
    glUniformMatrix4fv(shader_param_.matrix_projection_, 1, GL_FALSE, mat_view_.Ptr());
    glUniform3f(shader_param_.light0_, 100.0f, -200.0f, -600.0f);

    glDrawElements(GL_TRIANGLES, num_indices_, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

bool TeapotRenderer::LoadShaders(SHADER_PARAMS* params, const char* strVsh,
                                  const char* strFsh) {
    GLuint program;
    GLuint vert_shader, frag_shader;

    // 创建着色器程序
    program = glCreateProgram();
    LOGI("Created Shader %d", program);

    // 创建编译着色器
    if (!ndk_helper::shader::CompileShader(&vert_shader, GL_VERTEX_SHADER, strVsh)) {
        LOGI("Failed to compile vertex shader");
        glDeleteProgram(program);
        return false;
    }

    // 创建编译片元着色器
    if (!ndk_helper::shader::CompileShader(&frag_shader, GL_FRAGMENT_SHADER, strFsh)) {
        LOGI("Failed to compile fragment shader");
        glDeleteProgram(program);
        return false;
    }

    glAttachShader(program, vert_shader);
    glAttachShader(program, frag_shader);

    glBindAttribLocation(program, ATTRIB_VERTEX, "myVertex");
    glBindAttribLocation(program, ATTRIB_NORMAL, "myNormal");
    glBindAttribLocation(program, ATTRIB_UV, "myUV");

    if (!ndk_helper::shader::LinkProgram(program)) {
        LOGI("Failed to link program: %d", program);

        if (vert_shader) {
            glDeleteShader(vert_shader);
            vert_shader = 0;
        }
        if (frag_shader) {
            glDeleteShader(frag_shader);
            frag_shader = 0;
        }
        if (program)
            glDeleteProgram(program);

        return false;
    }

    // 获取uniform变量位置
    params -> matrix_projection_ = glGetUniformLocation (program, "uPMatrix");
    params->matrix_view_ = glGetUniformLocation(program, "uMVMatrix");

    params -> light0_ = glGetUniformLocation(program, "vLight0");
    params -> material_diffuse_ = glGetUniformLocation(program, "vMaterialDiffuse");
    params -> material_ambient_ = glGetUniformLocation(program, "vMaterialAmbient");
    params -> material_specular_ = glGetUniformLocation(program, "vMaterialSpecular");

    // 释放定点和片元着色器
    if (vert_shader)
        glDeleteShader(vert_shader);
    if (frag_shader)
        glDeleteShader(frag_shader);

    params -> program_ = program;
    return true;
}

bool TeapotRenderer::Bind(ndk_helper::TapCamera* camera) {
    camera_ = camera;
    return true;
}