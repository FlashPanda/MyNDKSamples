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
        glDeleteBuffers (1, &ibo_):
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

    mat_view_ = ndk_helper::Mat4::LookAt(ndk_helper)
}