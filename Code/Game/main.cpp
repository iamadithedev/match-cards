#include "glfw/platform_factory.hpp"
#include "glfw/platform.hpp"

#include "render_pass.hpp"
#include "physics_world.hpp"
#include "buffer.hpp"
#include "time.hpp"
#include "resource_manager.hpp"
#include "combine_geometry.hpp"
#include "importers/mesh_importer.hpp"
#include "material.hpp"
#include "light.hpp"
#include "camera.hpp"
#include "transform.hpp"

// ==================================================================================

#include "editor.hpp"

#include "render_pass_window.hpp"

int32_t main()
{
    int32_t width  = 1024;
    int32_t height = 768;

    glfw::PlatformFactory factory;

    auto platform = factory.create_platform();
    auto window = factory.create_window("Match Cards", { width, height});

    if (!platform->init())
    {
        return -1;
    }

    if (!window->create())
    {
        platform->release();
        return -1;
    }

    if (!glfw::Platform::init_context())
    {
        window->destroy();
        platform->release();

        return -1;
    }

    platform->vsync();

    // ==================================================================================// ==================================================================================

    ResourceManager resources;
    resources.init("../Assets/");

    auto diffuse_shader = resources.load<Shader>("diffuse_shader.asset");

    // ==================================================================================// ==================================================================================

    auto match_cards_geometries = MeshImporter::load("../Assets/match_cards.obj");

    CombineGeometry scene_geometry;
    scene_geometry.combine(match_cards_geometries);

    auto& card_submesh = scene_geometry[0];

    // ==================================================================================// ==================================================================================

    vertex_attributes diffuse_vertex_attributes =
    {
        { 0, 3, GL_FLOAT, (int32_t)offsetof(mesh_vertex::diffuse, position) },
        { 1, 3, GL_FLOAT, (int32_t)offsetof(mesh_vertex::diffuse, normal) }
    };

    VertexArray scene_vao;
    scene_vao.create();
    scene_vao.bind();

    Buffer scene_vbo { GL_ARRAY_BUFFER, GL_STATIC_DRAW };
    scene_vbo.create();
    scene_vbo.bind();
    scene_vbo.data(BufferData::make_data(scene_geometry.vertices()));

    Buffer scene_ibo { GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW };
    scene_ibo.create();
    scene_ibo.bind();
    scene_ibo.data(BufferData::make_data(scene_geometry.faces()));

    scene_vao.init_attributes_of_type<mesh_vertex::diffuse>(diffuse_vertex_attributes);

    // ==================================================================================// ==================================================================================

    Buffer matrices_ubo {GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW };
    matrices_ubo.create();
    matrices_ubo.bind_at_location(0);

    Buffer material_ubo {GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW };
    material_ubo.create();
    material_ubo.bind_at_location(1);

    Buffer light_ubo {GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW };
    light_ubo.create();
    light_ubo.bind_at_location(2);

    // ==================================================================================// ==================================================================================

    RenderPass render_pass { GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT };

    render_pass.enable(GL_DEPTH_TEST);
    render_pass.enable(GL_MULTISAMPLE);

    rgb clear_color { 0.047f, 0.485f, 0.598f };
    render_pass.clear_color(clear_color);

    // ==================================================================================// ==================================================================================

    Material card_material { { 1.0f, 0.0f, 0.0f } };

    // ==================================================================================

    Light directional_light { { 0.0f, 0.0f, 5.0f }, { 1.0f, 1.0f, 1.0f } };

    // ==================================================================================

    PhysicsWorld physics;
    physics.init();

    // ==================================================================================

    Camera scene_camera { 60.0f };

    Transform scene_camera_transform;

    vec3 scene_camera_position {0.0f, 0.0f, -20.0f };
    scene_camera_transform.translate(scene_camera_position);

    // ==================================================================================

    std::vector<glm::mat4> matrices { 3 };

    // ==================================================================================

    Transform card_Transform;

    // ==================================================================================

    Editor editor;
    editor.init(window.get(), &physics);

    RenderPassWindow render_pass_window;
    render_pass_window.set_render_pass(&render_pass, clear_color);

    editor.add_window(&render_pass_window);

    // ==================================================================================

    const Time time;

    while (!window->closed())
    {
        const float total_time = time.total_time();

        width  = window->size().width;
        height = window->size().height;

        scene_camera.resize((float)width, (float)height);

        // ==================================================================================

        editor.begin(width, height, total_time);
        editor.end();

        // ==================================================================================

        render_pass.viewport({ 0, 0 }, { width, height });
        render_pass.clear_buffers();

        // ==================================================================================

        matrices[0] = card_Transform.matrix();
        matrices[1] = scene_camera_transform.matrix();
        matrices[2] = scene_camera.projection();

        matrices_ubo.data(BufferData::make_data(matrices));
        material_ubo.data(BufferData::make_data(&card_material));
        light_ubo.data(BufferData::make_data(&directional_light));

        diffuse_shader->bind();
        scene_vao.bind();

        glDrawElements(GL_TRIANGLES, card_submesh.count, GL_UNSIGNED_INT, reinterpret_cast<std::byte*>(card_submesh.index));

        // ==================================================================================

        editor.draw(&matrices_ubo);

        // ==================================================================================

        window->update();
        platform->update();
    }

    window->destroy();
    platform->release();

    return 0;
}